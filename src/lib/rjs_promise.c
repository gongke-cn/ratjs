/*****************************************************************************
 *                             Rat Javascript                                *
 *                                                                           *
 * Copyright 2022 Gong Ke                                                    *
 *                                                                           *
 * Permission is hereby granted, free of charge, to any person obtaining a   *
 * copy of this software and associated documentation files (the             *
 * "Software"), to deal in the Software without restriction, including       *
 * without limitation the rights to use, copy, modify, merge, publish,       *
 * distribute, sublicense, and/or sell copies of the Software, and to permit *
 * persons to whom the Software is furnished to do so, subject to the        *
 * following conditions:                                                     *
 *                                                                           *
 * The above copyright notice and this permission notice shall be included   *
 * in all copies or substantial portions of the Software.                    *
 *                                                                           *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN *
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR     *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE *
 * USE OR OTHER DEALINGS IN THE SOFTWARE.                                    *
 *****************************************************************************/

#include "ratjs_internal.h"

/**Then job parameters.*/
typedef struct {
    RJS_Value  promise;  /**< The promise.*/
    RJS_Value  thenable; /**< Thenable object.*/
    RJS_Value  then;     /**< The function is already resolved.*/
} RJS_PromiseThenParams;

/**Reaction job parameters.*/
typedef struct {
    RJS_PromiseReaction reaction; /**< Reaction.*/
    RJS_Value           arg;      /**< Argument.*/
} RJS_PromiseReactionParams;

/**Promise status.*/
typedef struct {
    int         ref;      /**< Reference counter.*/
    RJS_Value   promise;  /**< The promise.*/
    RJS_Bool    resolved; /**< The primise is already resovled.*/
} RJS_PromiseStatus;

/**Promise capability new function.*/
typedef struct {
    RJS_PromiseCapability pc; /**< The promise capability.*/
} RJS_PromiseCapabilityData;

/*Resolve native function.*/
static RJS_Result
promise_resolve_nf (RJS_Runtime *rt, RJS_Value *f, RJS_Value *thiz, RJS_Value *args, size_t argc, RJS_Value *nt, RJS_Value *rv);
/*Reject native function.*/
static RJS_Result
promise_reject_nf (RJS_Runtime *rt, RJS_Value *f, RJS_Value *thiz, RJS_Value *args, size_t argc, RJS_Value *nt, RJS_Value *rv);

/*Scan the referenced things in the reaction.*/
static void
promise_reaction_scan (RJS_Runtime *rt, RJS_PromiseReaction *pr)
{
    rjs_gc_scan_value(rt, &pr->promise);
    rjs_gc_scan_value(rt, &pr->resolve);
    rjs_gc_scan_value(rt, &pr->reject);
    rjs_gc_scan_value(rt, &pr->handler);
}

/*Scan the referenced things in the promise.*/
static void
promise_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_Promise         *p = ptr;
    RJS_PromiseReaction *pr;

    rjs_object_op_gc_scan(rt, ptr);

    rjs_gc_scan_value(rt, &p->result);

    rjs_list_foreach_c(&p->fulfill_reactions, pr, RJS_PromiseReaction, ln) {
        promise_reaction_scan(rt, pr);
    }

    rjs_list_foreach_c(&p->reject_reactions, pr, RJS_PromiseReaction, ln) {
        promise_reaction_scan(rt, pr);
    }
}

/*Clear the promise reaction list.*/
static void
promise_reaction_list_clear (RJS_Runtime *rt, RJS_List *l)
{
    RJS_PromiseReaction *pr, *npr;

    rjs_list_foreach_safe_c(l, pr, npr, RJS_PromiseReaction, ln) {
        rjs_promise_capability_deinit(rt, &pr->capability);
        RJS_DEL(rt, pr);
    }

    rjs_list_init(l);
}

/*Free the promise.*/
static void
promise_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_Promise *p = ptr;
    
    rjs_object_deinit(rt, &p->object);

    promise_reaction_list_clear(rt, &p->fulfill_reactions);
    promise_reaction_list_clear(rt, &p->reject_reactions);

    RJS_DEL(rt, p);
}

/*Promise object operation functions.*/
static const RJS_ObjectOps
promise_ops = {
    {
        RJS_GC_THING_PROMISE,
        promise_op_gc_scan,
        promise_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS
};

/*Scan the referenced things in the promise status.*/
static void
promise_status_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_PromiseStatus *ps = ptr;

    rjs_gc_scan_value(rt, &ps->promise);
}

/*Free the promise status.*/
static void
promise_status_free (RJS_Runtime *rt, void *ptr)
{
    RJS_PromiseStatus *ps = ptr;

    ps->ref --;

    if (ps->ref == 0) {
        RJS_DEL(rt, ps);
    }
}

/*Create a new promise status.*/
static RJS_PromiseStatus*
promise_status_new (RJS_Runtime *rt, RJS_Value *promise)
{
    RJS_PromiseStatus *ps;

    RJS_NEW(rt, ps);

    rjs_value_copy(rt, &ps->promise, promise);
    ps->resolved = RJS_FALSE;
    ps->ref      = 1;

    return ps;
}

/*Create resolve function.*/
static RJS_Result
create_resolving_function (RJS_Runtime *rt, RJS_Value *f, RJS_NativeFunc nf,
        RJS_PromiseStatus *status)
{
    RJS_Realm *realm = rjs_realm_current(rt);
    RJS_Result r;

    r = rjs_create_native_function(rt, NULL, nf, 1, rjs_s_empty(rt), realm, NULL, NULL, f);
    if (r == RJS_ERR)
        return r;

    rjs_native_object_set_data(rt, f, NULL, status, promise_status_scan, promise_status_free);
    status->ref ++;
    return RJS_OK;
}

/*Then job.*/
static void
promise_then_job (RJS_Runtime *rt, void *data)
{
    RJS_PromiseThenParams *p       = data;
    size_t                 top     = rjs_value_stack_save(rt);
    RJS_Value             *resolve = rjs_value_stack_push(rt);
    RJS_Value             *reject  = rjs_value_stack_push(rt);
    RJS_Value             *err     = rjs_value_stack_push(rt);
    RJS_PromiseStatus     *ps;
    RJS_Result             r;

    ps = promise_status_new(rt, &p->promise);

    create_resolving_function(rt, resolve, promise_resolve_nf, ps);
    create_resolving_function(rt, reject, promise_reject_nf, ps);

    if ((r = rjs_call(rt, &p->then, &p->thenable, resolve, 2, NULL)) == RJS_ERR) {
        rjs_catch(rt, err);

        if ((r = rjs_call(rt, reject, rjs_v_undefined(rt), err, 1, NULL)) == RJS_ERR)
            goto end;
    }
end:
    promise_status_free(rt, ps);
    rjs_value_stack_restore(rt, top);
}

/*Scan the referenced things in reaction parameters.*/
static void
promise_reaction_params_scan (RJS_Runtime *rt, void *data)
{
    RJS_PromiseReactionParams *prp = data;

    promise_reaction_scan(rt, &prp->reaction);

    rjs_gc_scan_value(rt, &prp->arg);
}

/*Free the reaction parameters.*/
static void
promise_reaction_params_free (RJS_Runtime *rt, void *data)
{
    RJS_PromiseReactionParams *prp = data;

    rjs_promise_capability_deinit(rt, &prp->reaction.capability);

    RJS_DEL(rt, prp);
}

/*Reaction job.*/
static void
promise_reaction_job (RJS_Runtime *rt, void *data)
{
    RJS_PromiseReactionParams *prp    = data;
    size_t                     top    = rjs_value_stack_save(rt);
    RJS_Value                 *result = rjs_value_stack_push(rt);
    RJS_Result                 r;

    if (prp->reaction.type == RJS_PROMISE_REACTION_REJECT)
        rt->error_flag = RJS_FALSE;

    if (rjs_value_is_undefined(rt, &prp->reaction.handler)) {
        rjs_value_copy(rt, result, &prp->arg);
        r = (prp->reaction.type == RJS_PROMISE_REACTION_FULFILL) ? RJS_OK : RJS_ERR;
    } else {
        r = rjs_call(rt, &prp->reaction.handler, rjs_v_undefined(rt), &prp->arg, 1, result);
        if (r == RJS_ERR)
            rjs_catch(rt, result);
    }

    if (!rjs_value_is_undefined(rt, prp->reaction.capability.promise)) {
        if (r == RJS_OK) {
            rjs_call(rt, prp->reaction.capability.resolve, rjs_v_undefined(rt), result, 1, NULL);
        } else {
            rjs_call(rt, prp->reaction.capability.reject, rjs_v_undefined(rt), result, 1, NULL);
        }
    }

    rjs_value_stack_restore(rt, top);
}

/*Trigger the reactions.*/
static RJS_Result
trigger_promise_reactions (RJS_Runtime *rt, RJS_List *list, RJS_Value *reason)
{
    RJS_PromiseReaction *pr;

    rjs_list_foreach_c(list, pr, RJS_PromiseReaction, ln) {
        RJS_PromiseReactionParams *prp;
        RJS_Realm                 *realm;

        RJS_NEW(rt, prp);

        rjs_value_set_undefined(rt, &prp->reaction.promise);
        rjs_value_set_undefined(rt, &prp->reaction.resolve);
        rjs_value_set_undefined(rt, &prp->reaction.reject);

        rjs_promise_capability_init_vp(rt,
                &prp->reaction.capability,
                &prp->reaction.promise,
                &prp->reaction.resolve,
                &prp->reaction.reject);

        prp->reaction.type = pr->type;

        rjs_promise_capability_copy(rt, &prp->reaction.capability, &pr->capability);
        rjs_value_copy(rt, &prp->reaction.handler, &pr->handler);
        rjs_value_copy(rt, &prp->arg, reason);

        realm = rjs_get_function_realm(rt, &pr->handler);
        if (!realm)
            realm = rjs_realm_current(rt);

        rjs_job_enqueue(rt, promise_reaction_job, realm,
                promise_reaction_params_scan, promise_reaction_params_free, prp);
    }

    return RJS_OK;
}

/*Fulfill the promise.*/
static RJS_Result
fulfill_promise (RJS_Runtime *rt, RJS_Value *promise, RJS_Value *v)
{
    RJS_Promise *p = (RJS_Promise*)rjs_value_get_object(rt, promise);
    RJS_List     list;

    assert(p->state == RJS_PROMISE_STATE_PENDING);

    rjs_value_copy(rt, &p->result, v);

    p->state = RJS_PROMISE_STATE_FULFILLED;

    rjs_list_init(&list);
    rjs_list_join(&list, &p->fulfill_reactions);
    rjs_list_init(&p->fulfill_reactions);

    promise_reaction_list_clear(rt, &p->reject_reactions);

    trigger_promise_reactions(rt, &list, v);

    promise_reaction_list_clear(rt, &list);

    return RJS_OK;
}

/*Reject the promise.*/
static RJS_Result
reject_promise (RJS_Runtime *rt, RJS_Value *promise, RJS_Value *reason)
{
    RJS_Promise *p = (RJS_Promise*)rjs_value_get_object(rt, promise);
    RJS_List     list;

    assert(p->state == RJS_PROMISE_STATE_PENDING);

    rjs_value_copy(rt, &p->result, reason);

    p->state = RJS_PROMISE_STATE_REJECTED;

    rjs_list_init(&list);
    rjs_list_join(&list, &p->reject_reactions);
    rjs_list_init(&p->reject_reactions);

    promise_reaction_list_clear(rt, &p->fulfill_reactions);

    if (!rjs_list_is_empty(&list))
        rt->error_flag = RJS_FALSE;

    trigger_promise_reactions(rt, &list, reason);

    promise_reaction_list_clear(rt, &list);

    return RJS_OK;
}

/*Scan the promise then parameters.*/
static void
promise_then_params_scan (RJS_Runtime *rt, void *data)
{
    RJS_PromiseThenParams *ptp = data;

    rjs_gc_scan_value(rt, &ptp->promise);
    rjs_gc_scan_value(rt, &ptp->thenable);
    rjs_gc_scan_value(rt, &ptp->then);
}

/*Free the promise then parameters.*/
static void
promise_then_params_free (RJS_Runtime *rt, void *data)
{
    RJS_PromiseThenParams *ptp = data;

    RJS_DEL(rt, ptp);
}

/*Resolve.*/
static RJS_Result
promise_resolve_nf (RJS_Runtime *rt, RJS_Value *f, RJS_Value *thiz, RJS_Value *args, size_t argc, RJS_Value *nt, RJS_Value *rv)
{
    RJS_Value              *res  = rjs_argument_get(rt, args, argc, 0);
    size_t                  top  = rjs_value_stack_save(rt);
    RJS_Value              *err  = rjs_value_stack_push(rt);
    RJS_Value              *then = rjs_value_stack_push(rt);
    RJS_PromiseStatus      *ps   = rjs_native_object_get_data(rt, f);
    RJS_Realm              *realm;
    RJS_PromiseThenParams  *ptp;
    RJS_Result              r;

    rjs_value_set_undefined(rt, rv);

    if (ps->resolved)
        goto end;

    ps->resolved = RJS_TRUE;

    if (rjs_same_value(rt, res, &ps->promise)) {
        rjs_type_error_new(rt, err, _("promise value mismatch"));
        reject_promise(rt, &ps->promise, err);
        goto end;
    }

    if (!rjs_value_is_object(rt, res)) {
        fulfill_promise(rt, &ps->promise, res);
        goto end;
    }

    if ((r = rjs_get(rt, res, rjs_pn_then(rt), then)) == RJS_ERR) {
        rjs_catch(rt, err);
        reject_promise(rt, &ps->promise, err);
        goto end;
    }

    if (!rjs_is_callable(rt, then)) {
        fulfill_promise(rt, &ps->promise, res);
        goto end;
    }

    /*Enqueue then job.*/
    RJS_NEW(rt, ptp);
    rjs_value_copy(rt, &ptp->promise, &ps->promise);
    rjs_value_copy(rt, &ptp->thenable, res);
    rjs_value_copy(rt, &ptp->then, then);

    realm = rjs_get_function_realm(rt, then);
    if (!realm)
        realm = rjs_realm_current(rt);

    rjs_job_enqueue(rt, promise_then_job, realm, promise_then_params_scan, promise_then_params_free, ptp);
end:
    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}

/*Reject.*/
static RJS_Result
promise_reject_nf (RJS_Runtime *rt, RJS_Value *f, RJS_Value *thiz, RJS_Value *args, size_t argc, RJS_Value *nt, RJS_Value *rv)
{
    RJS_Value         *reason = rjs_argument_get(rt, args, argc, 0);
    RJS_PromiseStatus *ps     = rjs_native_object_get_data(rt, f);

    if (!ps->resolved) {
        ps->resolved = RJS_TRUE;

        reject_promise(rt, &ps->promise, reason);
    }

    rjs_value_set_undefined(rt, rv);
    return RJS_OK;
}

/**
 * Create a new promise.
 * \param rt The current runtime.
 * \param[out] v Return the new promise.
 * \param exec The executor.
 * \param new_target The new target.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_promise_new (RJS_Runtime *rt, RJS_Value *v, RJS_Value *exec, RJS_Value *new_target)
{
    size_t             top     = rjs_value_stack_save(rt);
    RJS_Value         *resolve = rjs_value_stack_push(rt);
    RJS_Value         *reject  = rjs_value_stack_push(rt);
    RJS_Value         *rv      = rjs_value_stack_push(rt);
    RJS_Value         *err     = rjs_value_stack_push(rt);
    RJS_Promise       *p       = NULL;
    RJS_PromiseStatus *ps      = NULL;
    RJS_Result         r;

    if (!new_target || rjs_value_is_undefined(rt, new_target)) {
        r = rjs_throw_type_error(rt, _("\"Promise\" cannot be used as a constructor"));
        goto end;
    }

    if (!rjs_is_callable(rt, exec)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    RJS_NEW(rt, p);

    p->state      = RJS_PROMISE_STATE_PENDING;
    p->is_handled = RJS_FALSE;

    rjs_list_init(&p->fulfill_reactions);
    rjs_list_init(&p->reject_reactions);
    rjs_value_set_undefined(rt, &p->result);

    if ((r = rjs_ordinary_init_from_constructor(rt, &p->object, new_target,
            RJS_O_Promise_prototype, &promise_ops, v)) == RJS_ERR)
        goto end;
    p = NULL;

    ps = promise_status_new(rt, v);

    create_resolving_function(rt, resolve, promise_resolve_nf, ps);
    create_resolving_function(rt, reject, promise_reject_nf, ps);

    if ((r = rjs_call(rt, exec, rjs_v_undefined(rt), resolve, 2, rv)) == RJS_ERR) {
        rjs_catch(rt, err);

        if ((r = rjs_call(rt, reject, rjs_v_undefined(rt), err, 1, NULL)) == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    if (ps)
        promise_status_free(rt, ps);

    if (r == RJS_ERR) {
        if (p)
            RJS_DEL(rt, p);
    }

    rjs_value_stack_restore(rt, top);
    return r;
}

/*Free the promise capability data.*/
static void
promise_capability_data_free (RJS_Runtime *rt, void *ptr)
{
    RJS_PromiseCapabilityData *pcd = ptr;

    rjs_promise_capability_deinit(rt, &pcd->pc);
    RJS_DEL(rt, pcd);
}

/*Create new promise capability.*/
static RJS_Result
promise_capalibity_new_nf (RJS_Runtime *rt, RJS_Value *f, RJS_Value *thiz, RJS_Value *args, size_t argc, RJS_Value *nt, RJS_Value *rv)
{
    RJS_PromiseCapabilityData *pcd     = rjs_native_object_get_data(rt, f);
    RJS_Value                 *resolve = rjs_argument_get(rt, args, argc, 0);
    RJS_Value                 *reject  = rjs_argument_get(rt, args, argc, 1);

    if (!rjs_value_is_undefined(rt, pcd->pc.resolve)
            || !rjs_value_is_undefined(rt, pcd->pc.reject))
        return rjs_throw_type_error(rt, _("\"resolve\" or \"reject\" is undefined"));

    rjs_value_copy(rt, pcd->pc.resolve, resolve);
    rjs_value_copy(rt, pcd->pc.reject, reject);

    return RJS_OK;
}

/**
 * Create a new promise capability.
 * \param rt The current runtime.
 * \param constr The promise constructor.
 * \param[out] pc Return the new promise capability.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_new_promise_capability (RJS_Runtime *rt, RJS_Value *constr, RJS_PromiseCapability *pc)
{
    RJS_Result                 r;
    RJS_PromiseCapabilityData *pcd;
    RJS_Realm                 *realm   = rjs_realm_current(rt);
    size_t                     top     = rjs_value_stack_save(rt);
    RJS_Value                 *exec    = rjs_value_stack_push(rt);
    RJS_Value                 *promise = rjs_value_stack_push(rt);

    if (!rjs_is_constructor(rt, constr)) {
        r = rjs_throw_type_error(rt, _("the value is not a constructor"));
        goto end;
    }

    rjs_value_set_undefined(rt, pc->promise);
    rjs_value_set_undefined(rt, pc->resolve);
    rjs_value_set_undefined(rt, pc->reject);

    r = rjs_create_native_function(rt, NULL, promise_capalibity_new_nf, 2, rjs_s_empty(rt),
            realm, NULL, NULL, exec);
    if (r == RJS_ERR)
        goto end;

    RJS_NEW(rt, pcd);
    rjs_promise_capability_init_vp(rt, &pcd->pc, pc->promise, pc->resolve, pc->reject);
    rjs_native_object_set_data(rt, exec, NULL, pcd, NULL, promise_capability_data_free);

    if ((r = rjs_construct(rt, constr, exec, 1, NULL, promise)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, pc->resolve) || !rjs_is_callable(rt, pc->reject)) {
        r = rjs_throw_type_error(rt, _("\"resolve\" or \"reject\" is not a function"));
        goto end;
    }

    rjs_value_copy(rt, pc->promise, promise);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Resolve the promise.
 * \param rt The current runtime.
 * \param constr The constructor.
 * \param[out] promise Return the promise.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_promise_resolve (RJS_Runtime *rt, RJS_Value *constr, RJS_Value *v, RJS_Value *promise)
{
    size_t                 top     = rjs_value_stack_save(rt);
    RJS_Value             *xconstr = rjs_value_stack_push(rt);
    RJS_PromiseCapability  pc;
    RJS_Result             r;

    rjs_promise_capability_init(rt, &pc);

    if (rjs_value_is_promise(rt, v)) {
        if ((r = rjs_get(rt, v, rjs_pn_constructor(rt), xconstr)) == RJS_ERR) {
            goto end;
        }

        if (rjs_same_value(rt, xconstr, constr)) {
            rjs_value_copy(rt, promise, v);
            r = RJS_OK;
            goto end;
        }
    }

    if ((r = rjs_new_promise_capability(rt, constr, &pc)) == RJS_ERR)
        goto end;

    if ((r = rjs_call(rt, pc.resolve, rjs_v_undefined(rt), v, 1, NULL)) == RJS_ERR)
        goto end;

    rjs_value_copy(rt, promise, pc.promise);

    r = RJS_OK;
end:
    rjs_promise_capability_deinit(rt, &pc);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Initialize promise reaction.*/
static void
promise_reaction_init (RJS_Runtime *rt, RJS_PromiseReaction *pr, RJS_PromiseCapability *pc,
        RJS_PromiseRectionType type, RJS_Value *cb)
{
    pr->type = type;

    rjs_value_set_undefined(rt, &pr->promise);
    rjs_value_set_undefined(rt, &pr->resolve);
    rjs_value_set_undefined(rt, &pr->reject);

    rjs_promise_capability_init_vp(rt, &pr->capability, &pr->promise, &pr->resolve, &pr->reject);

    if (pc)
        rjs_promise_capability_copy(rt, &pr->capability, pc);

    if (cb)
        rjs_value_copy(rt, &pr->handler, cb);
    else
        rjs_value_set_undefined(rt, &pr->handler);
}

/*Create a new promise reaction.*/
static RJS_PromiseReaction*
promise_reaction_new (RJS_Runtime *rt, RJS_PromiseCapability *pc, RJS_PromiseRectionType type, RJS_Value *cb)
{
    RJS_PromiseReaction *pr;

    RJS_NEW(rt, pr);

    promise_reaction_init(rt, pr, pc, type, cb);

    return pr;
}

/**
 * Perform promise then operation.
 * \param rt The current runtime.
 * \param promisev The promise.
 * \param fulfill On fulfill callback function.
 * \param reject On reject callback function.
 * \param result_pc Result promise capability.
 * \param[out] rpromisev The result promise.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_perform_proimise_then (RJS_Runtime *rt, RJS_Value *promisev, RJS_Value *fulfill, RJS_Value *reject,
        RJS_PromiseCapability *result_pc, RJS_Value *rpromisev)
{
    RJS_Promise *promise;
    RJS_Realm   *realm;

    assert(rjs_value_is_promise(rt, promisev));

    if (!rjs_is_callable(rt, fulfill))
        fulfill = NULL;
    if (!rjs_is_callable(rt, reject))
        reject = NULL;

    promise = (RJS_Promise*)rjs_value_get_object(rt, promisev);

    if (promise->state == RJS_PROMISE_STATE_PENDING) {
        RJS_PromiseReaction *pr;

        pr = promise_reaction_new(rt, result_pc, RJS_PROMISE_REACTION_FULFILL, fulfill);
        rjs_list_append(&promise->fulfill_reactions, &pr->ln);

        pr = promise_reaction_new(rt, result_pc, RJS_PROMISE_REACTION_REJECT, reject);
        rjs_list_append(&promise->reject_reactions, &pr->ln);
    } else if (promise->state == RJS_PROMISE_STATE_FULFILLED) {
        RJS_PromiseReactionParams *prp;

        RJS_NEW(rt, prp);

        promise_reaction_init(rt, &prp->reaction, result_pc, RJS_PROMISE_REACTION_FULFILL, fulfill);
        rjs_value_copy(rt, &prp->arg, &promise->result);

        realm = NULL;

        if (fulfill)
            realm = rjs_get_function_realm(rt, fulfill);
        if (!realm)
            realm = rjs_realm_current(rt);

        rjs_job_enqueue(rt, promise_reaction_job, realm, promise_reaction_params_scan, promise_reaction_params_free, prp);
    } else {
        RJS_PromiseReactionParams *prp;

        RJS_NEW(rt, prp);

        promise_reaction_init(rt, &prp->reaction, result_pc, RJS_PROMISE_REACTION_REJECT, reject);
        rjs_value_copy(rt, &prp->arg, &promise->result);

        realm = NULL;

        if (reject)
            realm = rjs_get_function_realm(rt, reject);
        if (!realm)
            realm = rjs_realm_current(rt);

        rjs_job_enqueue(rt, promise_reaction_job, realm, promise_reaction_params_scan, promise_reaction_params_free, prp);
    }

    promise->is_handled = RJS_TRUE;

    if (rpromisev) {
        if (result_pc)
            rjs_value_copy(rt, rpromisev, result_pc->promise);
        else
            rjs_value_set_undefined(rt, rpromisev);
    }

    return RJS_OK;
}

/**
 * Perform "then" operation of the promise.
 * \param rt The current runtime.
 * \param promise The promise.
 * \param fulfill The fulfill callback.
 * \param reject The reject callback.
 * \param rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_promise_then (RJS_Runtime *rt, RJS_Value *promise, RJS_Value *fulfill, RJS_Value *reject, RJS_Value *rv)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *fcb = rjs_value_stack_push(rt);
    RJS_Value *rcb = rjs_value_stack_push(rt);
    RJS_Result r;

    if (fulfill)
        rjs_value_copy(rt, fcb, fulfill);

    if (reject)
        rjs_value_copy(rt, rcb, reject);

    r = rjs_invoke(rt, promise, rjs_pn_then(rt), fcb, 2, rv);

    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Perform "then" operation of the promise with native functions.
 * \param rt The current runtime.
 * \param promise The promise.
 * \param fulfill The fulfill callback.
 * \param reject The reject callback.
 * \param rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_promise_then_native (RJS_Runtime *rt, RJS_Value *promise, RJS_NativeFunc fulfill, RJS_NativeFunc reject, RJS_Value *rv)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *fcb = rjs_value_stack_push(rt);
    RJS_Value *rcb = rjs_value_stack_push(rt);
    RJS_Result r;

    if (fulfill) {
        if ((r = rjs_builtin_func_object_new(rt, fcb, NULL, NULL, NULL, fulfill, 0)) == RJS_ERR)
            goto end;
    }

    if (reject) {
        if ((r = rjs_builtin_func_object_new(rt, rcb, NULL, NULL, NULL, reject, 0)) == RJS_ERR)
            goto end;
    }

    r = rjs_invoke(rt, promise, rjs_pn_then(rt), fcb, 2, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}
