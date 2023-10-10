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

/*Promise.*/
static RJS_NF(Promise_constructor)
{
    RJS_Value *exec = rjs_argument_get(rt, args, argc, 0);

    return rjs_promise_new(rt, rv, exec, nt);
}

static const RJS_BuiltinFuncDesc
promise_constructor_desc = {
    "Promise",
    1,
    Promise_constructor
};

/*Get the resolve method.*/
static RJS_Result
get_promise_resolve (RJS_Runtime *rt, RJS_Value *c, RJS_Value *resolve)
{
    RJS_Result r;

    if ((r = rjs_get(rt, c, rjs_pn_resolve(rt), resolve)) == RJS_ERR)
        return r;

    if (!rjs_is_callable(rt, resolve))
        return rjs_throw_type_error(rt, _("the value is not a function"));

    return RJS_OK;
}

/**
 * Invoke reject if arupt.
 * \param rt The current runtime.
 * \param r The result.
 * \param pc The promise capability.
 * \param rv The return value.
 * \return Return r.
 */
RJS_Result
if_abrupt_reject_promise (RJS_Runtime *rt, RJS_Result r, RJS_PromiseCapability *pc, RJS_Value *rv)
{
    if (r == RJS_ERR) {
        rjs_call(rt, pc->reject, rjs_v_undefined(rt), &rt->error, 1, NULL);
        rjs_value_copy(rt, rv, pc->promise);
    }

    return r;
}

/**Promise all remaining elements count.*/
typedef struct {
    RJS_GcThing gc_thing; /**< Base GC thing data.*/
    size_t      left;     /**< Left promise number.*/
} RJS_PromiseAllRemaining;

/**Promise all resolve function.*/
typedef struct {
    RJS_BuiltinFuncObject bfo;        /**< Base built-in function object.*/
    RJS_PromiseCapability capability; /**< The promise capability.*/
    RJS_Value             promise;    /**< Promise value buffer.*/
    RJS_Value             resolve;    /**< Resolve value buffer.*/
    RJS_Value             reject;     /**< Reject value buffer.*/
    RJS_Value             values;     /**< Result values array.*/
    RJS_Value             remaining;  /**< Remaining elements count.*/
    size_t                left;       /**< Left promise number.*/
    size_t                index;      /**< The current promise index.*/
    RJS_Bool              called;     /**< The function is already called.*/
} RJS_PromiseAllFunc;

/*Scan reference things in promise all remaining data.*/
static void
promise_all_remaining_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
}

/*Free the promise all remaining data.*/
static void
promise_all_remaining_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_PromiseAllRemaining *par = ptr;

    RJS_DEL(rt, par);
}

/*Promise all remaining element count operation functions.*/
static const RJS_GcThingOps
promise_all_remaining_ops = {
    RJS_GC_THING_INT,
    promise_all_remaining_op_gc_scan,
    promise_all_remaining_op_gc_free
};

/*Scan reference thing in the promise all built-in function.*/
static void
promise_all_func_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_PromiseAllFunc *func = ptr;

    rjs_builtin_func_object_op_gc_scan(rt, &func->bfo);

    rjs_gc_scan_value(rt, &func->promise);
    rjs_gc_scan_value(rt, &func->resolve);
    rjs_gc_scan_value(rt, &func->reject);
    rjs_gc_scan_value(rt, &func->values);
    rjs_gc_scan_value(rt, &func->remaining);
}

/*Free the promise all built-in function.*/
static void
promise_all_func_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_PromiseAllFunc *func = ptr;

    rjs_promise_capability_deinit(rt, &func->capability);
    rjs_builtin_func_object_deinit(rt, &func->bfo);

    RJS_DEL(rt, func);
}

/*Promise all built-in function operation functions.*/
static const RJS_ObjectOps
promise_all_func_ops = {
    {
        RJS_GC_THING_BUILTIN_FUNC,
        promise_all_func_op_gc_scan,
        promise_all_func_op_gc_free
    },
    RJS_BUILTIN_FUNCTION_OBJECT_OPS
};

/*Create a new promise all remaining data.*/
static RJS_PromiseAllRemaining*
promise_all_remaining_new (RJS_Runtime *rt, RJS_Value *v, size_t num)
{
    RJS_PromiseAllRemaining *par;

    RJS_NEW(rt, par);

    par->left = num;

    rjs_value_set_gc_thing(rt, v, par);
    rjs_gc_add(rt, par, &promise_all_remaining_ops);

    return par;
}

/*Promise all resolve element function.*/
static RJS_NF(promise_all_resolve)
{
    RJS_Value               *x      = rjs_argument_get(rt, args, argc, 0);
    RJS_PromiseAllFunc      *paf    = (RJS_PromiseAllFunc*)rjs_value_get_object(rt, f);
    RJS_PromiseAllRemaining *remain = rjs_value_get_gc_thing(rt, &paf->remaining);
    RJS_Result               r;

    if (paf->called) {
        rjs_value_set_undefined(rt, rv);
        return RJS_OK;
    }

    paf->called = RJS_TRUE;
    remain->left --;

    rjs_set_index(rt, &paf->values, paf->index, x, RJS_TRUE);

    if (remain->left == 0) {
        r = rjs_call(rt, paf->capability.resolve, rjs_v_undefined(rt), &paf->values, 1, rv);
    } else {
        rjs_value_set_undefined(rt, rv);
        r = RJS_OK;
    }

    return r;
}

/*Create a new promise all built-in function.*/
static RJS_Result
promise_all_func_new (RJS_Runtime *rt, RJS_Value *v, RJS_NativeFunc nf, size_t index,
        RJS_Value *values, RJS_PromiseCapability *pc, RJS_Value *remaining)
{
    RJS_Realm          *realm = rjs_realm_current(rt);
    RJS_PromiseAllFunc *func;
    RJS_Result          r;

    RJS_NEW(rt, func);

    func->called = RJS_FALSE;
    func->index  = index;

    rjs_value_set_undefined(rt, &func->promise);
    rjs_value_set_undefined(rt, &func->resolve);
    rjs_value_set_undefined(rt, &func->reject);

    rjs_promise_capability_init_vp(rt, &func->capability, &func->promise, &func->resolve, &func->reject);
    rjs_promise_capability_copy(rt, &func->capability, pc);

    rjs_value_copy(rt, &func->values, values);
    rjs_value_copy(rt, &func->remaining, remaining);

    r = rjs_init_builtin_function(rt, &func->bfo, nf, 0, &promise_all_func_ops,
            1, rjs_s_empty(rt), realm, NULL, NULL, NULL, v);
    if (r == RJS_ERR) {
        rjs_promise_capability_deinit(rt, &func->capability);
        RJS_DEL(rt, func);
    }

    return r;
}

/*Perform promise all operation.*/
static RJS_Result
perform_promise_all (RJS_Runtime *rt, RJS_Iterator *iter, RJS_Value *constr,
        RJS_PromiseCapability *pc, RJS_Value *resolve, RJS_Value *rv)
{
    size_t                   top     = rjs_value_stack_save(rt);
    RJS_Value               *next    = rjs_value_stack_push(rt);
    RJS_Value               *nextv   = rjs_value_stack_push(rt);
    RJS_Value               *nextp   = rjs_value_stack_push(rt);
    RJS_Value               *values  = rjs_value_stack_push(rt);
    RJS_Value               *fulfill = rjs_value_stack_push(rt);
    RJS_Value               *reject  = rjs_value_stack_push(rt);
    RJS_Value               *remain  = rjs_value_stack_push(rt);
    size_t                   index   = 0;
    RJS_PromiseAllRemaining *par;
    RJS_Result               r;

    par = promise_all_remaining_new(rt, remain, 1);

    rjs_value_copy(rt, reject, pc->reject);

    rjs_array_new(rt, values, 0, NULL);

    while (1) {
        r = rjs_iterator_step(rt, iter, next);
        if (r == RJS_ERR) {
            iter->done = RJS_TRUE;
            goto end;
        }

        if (!r) {
            iter->done = RJS_TRUE;

            par->left --;

            if (par->left == 0) {
                if ((r = rjs_call(rt, pc->resolve, rjs_v_undefined(rt), values, 1, NULL)) == RJS_ERR)
                    goto end;
            }
            break;
        }

        r = rjs_iterator_value(rt, next, nextv);
        if (r == RJS_ERR) {
            iter->done = RJS_TRUE;
            goto end;
        }

        rjs_set_index(rt, values, index, rjs_v_undefined(rt), RJS_TRUE);

        if ((r = rjs_call(rt, resolve, constr, nextv, 1, nextp)) == RJS_ERR)
            goto end;

        if ((r = promise_all_func_new(rt, fulfill, promise_all_resolve, index, values, pc, remain)) == RJS_ERR)
            goto end;

        par->left ++;

        if ((r = rjs_invoke(rt, nextp, rjs_pn_then(rt), fulfill, 2, NULL)) == RJS_ERR)
            goto end;

        index ++;
    }

    rjs_value_copy(rt, rv, pc->promise);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Promise.all*/
static RJS_NF(Promise_all)
{
    RJS_Value             *iterable = rjs_argument_get(rt, args, argc, 0);
    size_t                 top      = rjs_value_stack_save(rt);
    RJS_Value             *resolve  = rjs_value_stack_push(rt);
    RJS_PromiseCapability  pc;
    RJS_Iterator           iterator_rec;
    RJS_Result             r;

    rjs_iterator_init(rt, &iterator_rec);
    rjs_promise_capability_init(rt, &pc);

    if ((r = rjs_new_promise_capability(rt, thiz, &pc)) == RJS_ERR)
        goto end;

    r = get_promise_resolve(rt, thiz, resolve);
    if (if_abrupt_reject_promise(rt, r, &pc, rv) == RJS_ERR) {
        r = RJS_OK;
        goto end;
    }

    r = rjs_get_iterator(rt, iterable, RJS_ITERATOR_SYNC, NULL, &iterator_rec);
    if (if_abrupt_reject_promise(rt, r, &pc, rv) == RJS_ERR) {
        r = RJS_OK;
        goto end;
    }

    r = perform_promise_all(rt, &iterator_rec, thiz, &pc, resolve, rv);
    if (r == RJS_ERR) {
        if (!iterator_rec.done)
            rjs_iterator_close(rt, &iterator_rec);

        if (if_abrupt_reject_promise(rt, r, &pc, rv) == RJS_ERR) {
            r = RJS_OK;
            goto end;
        }
    }

    r = RJS_OK;
end:
    rjs_promise_capability_deinit(rt, &pc);
    rjs_iterator_deinit(rt, &iterator_rec);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Promise all settled resolve function.*/
static RJS_NF(promise_all_settled_resolve)
{
    RJS_Value               *x      = rjs_argument_get(rt, args, argc, 0);
    RJS_PromiseAllFunc      *paf    = (RJS_PromiseAllFunc*)rjs_value_get_object(rt, f);
    RJS_PromiseAllRemaining *remain = rjs_value_get_gc_thing(rt, &paf->remaining);
    size_t                   top    = rjs_value_stack_save(rt);
    RJS_Value               *obj    = rjs_value_stack_push(rt);
    RJS_Result               r;

    if (paf->called) {
        rjs_value_set_undefined(rt, rv);
        r = RJS_OK;
        goto end;
    }

    paf->called = RJS_TRUE;

    rjs_ordinary_object_create(rt, NULL, obj);
    rjs_create_data_property_or_throw(rt, obj, rjs_pn_status(rt), rjs_s_fulfilled(rt));
    rjs_create_data_property_or_throw(rt, obj, rjs_pn_value(rt), x);

    rjs_set_index(rt, &paf->values, paf->index, obj, RJS_TRUE);

    remain->left --;

    if (remain->left == 0) {
        r = rjs_call(rt, paf->capability.resolve, rjs_v_undefined(rt), &paf->values, 1, rv);
    } else {
        rjs_value_set_undefined(rt, rv);
        r = RJS_OK;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Promise all settled reject function.*/
static RJS_NF(promise_all_settled_reject)
{
    RJS_Value               *x      = rjs_argument_get(rt, args, argc, 0);
    RJS_PromiseAllFunc      *paf    = (RJS_PromiseAllFunc*)rjs_value_get_object(rt, f);
    RJS_PromiseAllRemaining *remain = rjs_value_get_gc_thing(rt, &paf->remaining);
    size_t                   top    = rjs_value_stack_save(rt);
    RJS_Value               *obj    = rjs_value_stack_push(rt);
    RJS_Result               r;

    if (paf->called) {
        rjs_value_set_undefined(rt, rv);
        r = RJS_OK;
        goto end;
    }

    paf->called = RJS_TRUE;

    rjs_ordinary_object_create(rt, NULL, obj);
    rjs_create_data_property_or_throw(rt, obj, rjs_pn_status(rt), rjs_s_rejected(rt));
    rjs_create_data_property_or_throw(rt, obj, rjs_pn_reason(rt), x);

    rjs_set_index(rt, &paf->values, paf->index, obj, RJS_TRUE);

    remain->left --;
    
    if (remain->left == 0) {
        r = rjs_call(rt, paf->capability.resolve, rjs_v_undefined(rt), &paf->values, 1, rv);
    } else {
        rjs_value_set_undefined(rt, rv);
        r = RJS_OK;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Perform promise all settled operation.*/
static RJS_Result
perform_promise_all_settled (RJS_Runtime *rt, RJS_Iterator *iter, RJS_Value *constr,
        RJS_PromiseCapability *pc, RJS_Value *resolve, RJS_Value *rv)
{
    size_t         top     = rjs_value_stack_save(rt);
    RJS_Value     *next    = rjs_value_stack_push(rt);
    RJS_Value     *nextv   = rjs_value_stack_push(rt);
    RJS_Value     *nextp   = rjs_value_stack_push(rt);
    RJS_Value     *values  = rjs_value_stack_push(rt);
    RJS_Value     *fulfill = rjs_value_stack_push(rt);
    RJS_Value     *reject  = rjs_value_stack_push(rt);
    RJS_Value     *remain  = rjs_value_stack_push(rt);
    size_t         index   = 0;
    RJS_PromiseAllRemaining *par;
    RJS_Result     r;

    par = promise_all_remaining_new(rt, remain, 1);

    rjs_array_new(rt, values, 0, NULL);

    while (1) {
        r = rjs_iterator_step(rt, iter, next);
        if (r == RJS_ERR) {
            iter->done = RJS_TRUE;
            goto end;
        }

        if (!r) {
            iter->done = RJS_TRUE;

            par->left --;

            if (par->left == 0) {
                if ((r = rjs_call(rt, pc->resolve, rjs_v_undefined(rt), values, 1, NULL)) == RJS_ERR)
                    goto end;
            }
            break;
        }

        r = rjs_iterator_value(rt, next, nextv);
        if (r == RJS_ERR) {
            iter->done = RJS_TRUE;
            goto end;
        }

        rjs_set_index(rt, values, index, rjs_v_undefined(rt), RJS_TRUE);

        if ((r = rjs_call(rt, resolve, constr, nextv, 1, nextp)) == RJS_ERR)
            goto end;

        if ((r = promise_all_func_new(rt, fulfill, promise_all_settled_resolve, index, values, pc, remain)) == RJS_ERR)
            goto end;

        if ((r = promise_all_func_new(rt, reject, promise_all_settled_reject, index, values, pc, remain)) == RJS_ERR)
            goto end;

        par->left ++;

        if ((r = rjs_invoke(rt, nextp, rjs_pn_then(rt), fulfill, 2, NULL)) == RJS_ERR)
            goto end;

        index ++;
    }

    rjs_value_copy(rt, rv, pc->promise);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Promise.allSettled*/
static RJS_NF(Promise_allSettled)
{
    RJS_Value             *iterable = rjs_argument_get(rt, args, argc, 0);
    size_t                 top      = rjs_value_stack_save(rt);
    RJS_Value             *resolve  = rjs_value_stack_push(rt);
    RJS_PromiseCapability  pc;
    RJS_Iterator           iterator_rec;
    RJS_Result             r;

    rjs_iterator_init(rt, &iterator_rec);
    rjs_promise_capability_init(rt, &pc);

    if ((r = rjs_new_promise_capability(rt, thiz, &pc)) == RJS_ERR)
        goto end;

    r = get_promise_resolve(rt, thiz, resolve);
    if (if_abrupt_reject_promise(rt, r, &pc, rv) == RJS_ERR) {
        r = RJS_OK;
        goto end;
    }

    r = rjs_get_iterator(rt, iterable, RJS_ITERATOR_SYNC, NULL, &iterator_rec);
    if (if_abrupt_reject_promise(rt, r, &pc, rv) == RJS_ERR) {
        r = RJS_OK;
        goto end;
    }

    r = perform_promise_all_settled(rt, &iterator_rec, thiz, &pc, resolve, rv);
    if (r == RJS_ERR) {
        if (!iterator_rec.done)
            rjs_iterator_close(rt, &iterator_rec);

        if (if_abrupt_reject_promise(rt, r, &pc, rv) == RJS_ERR) {
            r = RJS_OK;
            goto end;
        }
    }

    r = RJS_OK;
end:
    rjs_promise_capability_deinit(rt, &pc);
    rjs_iterator_deinit(rt, &iterator_rec);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Promise any reject built-in function.*/
static RJS_NF(promise_any_reject)
{
    RJS_Value               *x      = rjs_argument_get(rt, args, argc, 0);
    RJS_PromiseAllFunc      *paf    = (RJS_PromiseAllFunc*)rjs_value_get_object(rt, f);
    RJS_PromiseAllRemaining *remain = rjs_value_get_gc_thing(rt, &paf->remaining);
    RJS_Realm               *realm  = rjs_realm_current(rt);
    size_t                   top    = rjs_value_stack_save(rt);
    RJS_Value               *error  = rjs_value_stack_push(rt);
    RJS_Result               r;

    if (paf->called) {
        rjs_value_set_undefined(rt, rv);
        r = RJS_OK;
        goto end;
    }

    paf->called = RJS_TRUE;

    rjs_set_index(rt, &paf->values, paf->index, x, RJS_TRUE);

    remain->left --;

    if (remain->left == 0) {
        if ((r = rjs_call(rt, rjs_o_AggregateError(realm), rjs_v_undefined(rt), &paf->values, 1, error)) == RJS_ERR)
            goto end;

        r = rjs_call(rt, paf->capability.reject, rjs_v_undefined(rt), error, 1, rv);
    } else {
        rjs_value_set_undefined(rt, rv);
        r = RJS_OK;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Perform promise any operation.*/
static RJS_Result
perform_promise_any (RJS_Runtime *rt, RJS_Iterator *iter, RJS_Value *constr,
        RJS_PromiseCapability *pc, RJS_Value *resolve, RJS_Value *rv)
{
    RJS_Realm     *realm   = rjs_realm_current(rt);
    size_t         top     = rjs_value_stack_save(rt);
    RJS_Value     *next    = rjs_value_stack_push(rt);
    RJS_Value     *nextv   = rjs_value_stack_push(rt);
    RJS_Value     *nextp   = rjs_value_stack_push(rt);
    RJS_Value     *errors  = rjs_value_stack_push(rt);
    RJS_Value     *fulfill = rjs_value_stack_push(rt);
    RJS_Value     *reject  = rjs_value_stack_push(rt);
    RJS_Value     *remain  = rjs_value_stack_push(rt);
    RJS_Value     *error   = rjs_value_stack_push(rt);
    size_t         index   = 0;
    RJS_PromiseAllRemaining *par;
    RJS_Result     r;

    par = promise_all_remaining_new(rt, remain, 1);

    rjs_array_new(rt, errors, 0, NULL);
    rjs_value_copy(rt, fulfill, pc->resolve);

    while (1) {
        r = rjs_iterator_step(rt, iter, next);
        if (r == RJS_ERR) {
            iter->done = RJS_TRUE;
            goto end;
        }

        if (!r) {
            iter->done = RJS_TRUE;

            par->left --;

            if (par->left == 0) {
                if ((r = rjs_call(rt, rjs_o_AggregateError(realm), rjs_v_undefined(rt), errors, 1, error)) == RJS_ERR)
                    goto end;

                r = rjs_throw(rt, error);
                goto end;
            }
            break;
        }

        r = rjs_iterator_value(rt, next, nextv);
        if (r == RJS_ERR) {
            iter->done = RJS_TRUE;
            goto end;
        }

        rjs_set_index(rt, errors, index, rjs_v_undefined(rt), RJS_TRUE);

        if ((r = rjs_call(rt, resolve, constr, nextv, 1, nextp)) == RJS_ERR)
            goto end;

        if ((r = promise_all_func_new(rt, reject, promise_any_reject, index, errors, pc, remain)) == RJS_ERR)
            goto end;

        par->left ++;

        if ((r = rjs_invoke(rt, nextp, rjs_pn_then(rt), fulfill, 2, NULL)) == RJS_ERR)
            goto end;

        index ++;
    }

    rjs_value_copy(rt, rv, pc->promise);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Promise.any*/
static RJS_NF(Promise_any)
{
    RJS_Value             *iterable = rjs_argument_get(rt, args, argc, 0);
    size_t                 top      = rjs_value_stack_save(rt);
    RJS_Value             *resolve  = rjs_value_stack_push(rt);
    RJS_PromiseCapability  pc;
    RJS_Iterator           iterator_rec;
    RJS_Result             r;

    rjs_iterator_init(rt, &iterator_rec);
    rjs_promise_capability_init(rt, &pc);

    if ((r = rjs_new_promise_capability(rt, thiz, &pc)) == RJS_ERR)
        goto end;

    r = get_promise_resolve(rt, thiz, resolve);
    if (if_abrupt_reject_promise(rt, r, &pc, rv) == RJS_ERR) {
        r = RJS_OK;
        goto end;
    }

    r = rjs_get_iterator(rt, iterable, RJS_ITERATOR_SYNC, NULL, &iterator_rec);
    if (if_abrupt_reject_promise(rt, r, &pc, rv) == RJS_ERR) {
        r = RJS_OK;
        goto end;
    }

    r = perform_promise_any(rt, &iterator_rec, thiz, &pc, resolve, rv);
    if (r == RJS_ERR) {
        if (!iterator_rec.done)
            rjs_iterator_close(rt, &iterator_rec);

        if (if_abrupt_reject_promise(rt, r, &pc, rv) == RJS_ERR) {
            r = RJS_OK;
            goto end;
        }
    }

    r = RJS_OK;
end:
    rjs_promise_capability_deinit(rt, &pc);
    rjs_iterator_deinit(rt, &iterator_rec);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Perform promise race operation.*/
static RJS_Result
perform_promise_race (RJS_Runtime *rt, RJS_Iterator *iter, RJS_Value *constr,
        RJS_PromiseCapability *pc, RJS_Value *resolve, RJS_Value *rv)
{
    size_t                 top     = rjs_value_stack_save(rt);
    RJS_Value             *next    = rjs_value_stack_push(rt);
    RJS_Value             *nextv   = rjs_value_stack_push(rt);
    RJS_Value             *nextp   = rjs_value_stack_push(rt);
    RJS_Value             *fulfill = rjs_value_stack_push(rt);
    RJS_Value             *reject  = rjs_value_stack_push(rt);
    RJS_Result             r;

    rjs_value_copy(rt, fulfill, pc->resolve);
    rjs_value_copy(rt, reject, pc->reject);

    while (1) {
        r = rjs_iterator_step(rt, iter, next);
        if (r == RJS_ERR) {
            iter->done = RJS_TRUE;
            goto end;
        }

        if (!r) {
            iter->done = RJS_TRUE;
            break;
        }

        r = rjs_iterator_value(rt, next, nextv);
        if (r == RJS_ERR) {
            iter->done = RJS_TRUE;
            goto end;
        }

        if ((r = rjs_call(rt, resolve, constr, nextv, 1, nextp)) == RJS_ERR)
            goto end;

        if ((r = rjs_invoke(rt, nextp, rjs_pn_then(rt), fulfill, 2, NULL)) == RJS_ERR)
            goto end;
    }

    rjs_value_copy(rt, rv, pc->promise);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Promise.race*/
static RJS_NF(Promise_race)
{
    RJS_Value             *iterable = rjs_argument_get(rt, args, argc, 0);
    size_t                 top      = rjs_value_stack_save(rt);
    RJS_Value             *resolve  = rjs_value_stack_push(rt);
    RJS_PromiseCapability  pc;
    RJS_Iterator           iterator_rec;
    RJS_Result             r;

    rjs_iterator_init(rt, &iterator_rec);
    rjs_promise_capability_init(rt, &pc);

    if ((r = rjs_new_promise_capability(rt, thiz, &pc)) == RJS_ERR)
        goto end;

    r = get_promise_resolve(rt, thiz, resolve);
    if (if_abrupt_reject_promise(rt, r, &pc, rv) == RJS_ERR) {
        r = RJS_OK;
        goto end;
    }

    r = rjs_get_iterator(rt, iterable, RJS_ITERATOR_SYNC, NULL, &iterator_rec);
    if (if_abrupt_reject_promise(rt, r, &pc, rv) == RJS_ERR) {
        r = RJS_OK;
        goto end;
    }

    r = perform_promise_race(rt, &iterator_rec, thiz, &pc, resolve, rv);
    if (r == RJS_ERR) {
        if (!iterator_rec.done)
            rjs_iterator_close(rt, &iterator_rec);

        if (if_abrupt_reject_promise(rt, r, &pc, rv) == RJS_ERR) {
            r = RJS_OK;
            goto end;
        }
    }

    r = RJS_OK;
end:
    rjs_promise_capability_deinit(rt, &pc);
    rjs_iterator_deinit(rt, &iterator_rec);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Promise.reject*/
static RJS_NF(Promise_reject)
{
    RJS_Value             *v   = rjs_argument_get(rt, args, argc, 0);
    size_t                 top = rjs_value_stack_save(rt);
    RJS_Result             r;
    RJS_PromiseCapability  pc;

    rjs_promise_capability_init(rt, &pc);

    if ((r = rjs_new_promise_capability(rt, thiz, &pc)) == RJS_ERR)
        goto end;

    if ((r = rjs_call(rt, pc.reject, rjs_v_undefined(rt), v, 1, NULL)) == RJS_ERR)
        goto end;

    rjs_value_copy(rt, rv, pc.promise);
    r = RJS_OK;
end:
    rjs_promise_capability_deinit(rt, &pc);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Promise.resolve*/
static RJS_NF(Promise_resolve)
{
    RJS_Value *v = rjs_argument_get(rt, args, argc, 0);

    if (!rjs_value_is_object(rt, thiz))
        return rjs_throw_type_error(rt, _("the value is not an object"));

    return rjs_promise_resolve(rt, thiz, v, rv);
}

static const RJS_BuiltinFuncDesc
promise_function_descs[] = {
    {
        "all",
        1,
        Promise_all
    },
    {
        "allSettled",
        1,
        Promise_allSettled
    },
    {
        "any",
        1,
        Promise_any
    },
    {
        "race",
        1,
        Promise_race
    },
    {
        "reject",
        1,
        Promise_reject
    },
    {
        "resolve",
        1,
        Promise_resolve
    },
    {NULL}
};

static const RJS_BuiltinAccessorDesc
promise_accessor_descs[] = {
    {
        "@@species",
        rjs_return_this
    },
    {NULL}
};

static const RJS_BuiltinFieldDesc
promise_prototype_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "Promise",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

/*Promise.prototype.catch*/
static RJS_NF(Promise_prototype_catch)
{
    RJS_Value *on_rejected = rjs_argument_get(rt, args, argc, 0);
    size_t     top         = rjs_value_stack_save(rt);
    RJS_Value *fulfill     = rjs_value_stack_push(rt);
    RJS_Value *reject      = rjs_value_stack_push(rt);
    RJS_Result r;

    rjs_value_set_undefined(rt, fulfill);
    rjs_value_copy(rt, reject, on_rejected);

    r = rjs_invoke(rt, thiz, rjs_pn_then(rt), fulfill, 2, rv);

    rjs_value_stack_restore(rt, top);
    return r;
}

/**Built-in promise finally function.*/
typedef struct {
    RJS_BuiltinFuncObject bfo;        /**< Base built-in function object data.*/
    RJS_Value             c;          /**< The constructor.*/
    RJS_Value             on_finally; /**< On finally function.*/
} RJS_PromiseFinallyFunc;

/*Scan reference things in the promise finally function.*/
static void
promise_finally_func_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_PromiseFinallyFunc *pff = ptr;

    rjs_builtin_func_object_op_gc_scan(rt, &pff->bfo);

    rjs_gc_scan_value(rt, &pff->c);
    rjs_gc_scan_value(rt, &pff->on_finally);
}

/*Free the promise finally function.*/
static void
promise_finally_func_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_PromiseFinallyFunc *pff = ptr;

    rjs_builtin_func_object_deinit(rt, &pff->bfo);

    RJS_DEL(rt, pff);
}

/*Promise finally function operation functions.*/
static const RJS_ObjectOps
promise_finally_func_ops = {
    {
        RJS_GC_THING_BUILTIN_FUNC,
        promise_finally_func_op_gc_scan,
        promise_finally_func_op_gc_free
    },
    RJS_BUILTIN_FUNCTION_OBJECT_OPS
};

/*Create a new promise finally function.*/
static RJS_Result
promise_finally_func_new (RJS_Runtime *rt, RJS_Value *v, RJS_NativeFunc nf, RJS_Value *c, RJS_Value *func)
{
    RJS_Realm              *realm = rjs_realm_current(rt);
    RJS_PromiseFinallyFunc *pff;
    RJS_Result              r;

    RJS_NEW(rt, pff);

    rjs_value_copy(rt, &pff->c, c);
    rjs_value_copy(rt, &pff->on_finally, func);

    r = rjs_init_builtin_function(rt, &pff->bfo, nf, 0, &promise_finally_func_ops, 1, rjs_s_empty(rt),
            realm, NULL, NULL, NULL, v);
    if (r == RJS_ERR)
        RJS_DEL(rt, pff);

    return r;
}

/**Promise value function.*/
typedef struct {
    RJS_BuiltinFuncObject bfo; /**< Base built-in function object data.*/
    RJS_Value             v;   /**< The value.*/
} RJS_PromiseValueFunc;

/*Scan reference things in the promise value function.*/
static void
promise_value_func_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_PromiseValueFunc *pvf = ptr;

    rjs_builtin_func_object_op_gc_scan(rt, &pvf->bfo);

    rjs_gc_scan_value(rt, &pvf->v);
}

/*Free the promise value function.*/
static void
promise_value_func_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_PromiseValueFunc *pvf = ptr;

    rjs_builtin_func_object_deinit(rt, &pvf->bfo);

    RJS_DEL(rt, pvf);
}

/*Promise value function operation functions.*/
static const RJS_ObjectOps
promise_value_func_ops = {
    {
        RJS_GC_THING_BUILTIN_FUNC,
        promise_value_func_op_gc_scan,
        promise_value_func_op_gc_free
    },
    RJS_BUILTIN_FUNCTION_OBJECT_OPS
};

/*Create a new promise value function.*/
static RJS_Result
promise_value_func_new (RJS_Runtime *rt, RJS_Value *v, RJS_NativeFunc nf, RJS_Value *pv, RJS_Value *func)
{
    RJS_Realm            *realm = rjs_realm_current(rt);
    RJS_PromiseValueFunc *pvf;
    RJS_Result            r;

    RJS_NEW(rt, pvf);

    rjs_value_copy(rt, &pvf->v, pv);

    r = rjs_init_builtin_function(rt, &pvf->bfo, nf, 0, &promise_value_func_ops, 0, rjs_s_empty(rt),
            realm, NULL, NULL, NULL, v);
    if (r == RJS_ERR)
        RJS_DEL(rt, pvf);

    return r;
}

/*Return value.*/
static RJS_NF(return_value_func)
{
    RJS_PromiseValueFunc *pvf = (RJS_PromiseValueFunc*)rjs_value_get_object(rt, f);

    rjs_value_copy(rt, rv, &pvf->v);
    return RJS_OK;
}

/*Finally then function.*/
static RJS_NF(then_finally_func)
{
    RJS_Value              *v    = rjs_argument_get(rt, args, argc, 0);
    RJS_PromiseFinallyFunc *pff  = (RJS_PromiseFinallyFunc*)rjs_value_get_object(rt, f);
    size_t                  top  = rjs_value_stack_save(rt);
    RJS_Value              *res  = rjs_value_stack_push(rt);
    RJS_Value              *p    = rjs_value_stack_push(rt);
    RJS_Value              *func = rjs_value_stack_push(rt);
    RJS_Result              r;

    if ((r = rjs_call(rt, &pff->on_finally, rjs_v_undefined(rt), NULL, 0, res)) == RJS_ERR)
        goto end;

    if ((r = rjs_promise_resolve(rt, &pff->c, res, p)) == RJS_ERR)
        goto end;

    promise_value_func_new(rt, func, return_value_func, v, func);

    r = rjs_invoke(rt, p, rjs_pn_then(rt), func, 1, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Throw reason function.*/
static RJS_NF(throw_reason_func)
{
    RJS_PromiseValueFunc *pvf = (RJS_PromiseValueFunc*)rjs_value_get_object(rt, f);

    return rjs_throw(rt, &pvf->v);
}

/*Finally catch function.*/
static RJS_NF(catch_finally_func)
{
    RJS_Value              *v    = rjs_argument_get(rt, args, argc, 0);
    RJS_PromiseFinallyFunc *pff  = (RJS_PromiseFinallyFunc*)rjs_value_get_object(rt, f);
    size_t                  top  = rjs_value_stack_save(rt);
    RJS_Value              *res  = rjs_value_stack_push(rt);
    RJS_Value              *p    = rjs_value_stack_push(rt);
    RJS_Value              *func = rjs_value_stack_push(rt);
    RJS_Result              r;

    if ((r = rjs_call(rt, &pff->on_finally, rjs_v_undefined(rt), NULL, 0, res)) == RJS_ERR)
        goto end;

    if ((r = rjs_promise_resolve(rt, &pff->c, res, p)) == RJS_ERR)
        goto end;

    promise_value_func_new(rt, func, throw_reason_func, v, func);

    r = rjs_invoke(rt, p, rjs_pn_then(rt), func, 1, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Promise.prototype.finally*/
static RJS_NF(Promise_prototype_finally)
{
    RJS_Value *on_finally = rjs_argument_get(rt, args, argc, 0);
    RJS_Realm *realm      = rjs_realm_current(rt);
    size_t     top        = rjs_value_stack_save(rt);
    RJS_Value *c          = rjs_value_stack_push(rt);
    RJS_Value *fulfill    = rjs_value_stack_push(rt);
    RJS_Value *reject     = rjs_value_stack_push(rt);
    RJS_Result r;

    if (!rjs_value_is_object(rt, thiz)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    if ((r = rjs_species_constructor(rt, thiz, rjs_o_Promise(realm), c)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, on_finally)) {
        rjs_value_copy(rt, fulfill, on_finally);
        rjs_value_copy(rt, reject, on_finally);
    } else {
        promise_finally_func_new(rt, fulfill, then_finally_func, c, on_finally);
        promise_finally_func_new(rt, reject, catch_finally_func, c, on_finally);
    }

    r = rjs_invoke(rt, thiz, rjs_pn_then(rt), fulfill, 2, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Promise.prototype.then*/
static RJS_NF(Promise_prototype_then)
{
    RJS_Value             *on_fulfilled = rjs_argument_get(rt, args, argc, 0);
    RJS_Value             *on_rejected  = rjs_argument_get(rt, args, argc, 1);
    RJS_Realm             *realm        = rjs_realm_current(rt);
    size_t                 top          = rjs_value_stack_save(rt);
    RJS_Value             *c            = rjs_value_stack_push(rt);
    RJS_PromiseCapability  pc;
    RJS_Result             r;

    rjs_promise_capability_init(rt, &pc);

    if (!rjs_value_is_promise(rt, thiz)) {
        r = rjs_throw_type_error(rt, _("the value is not a promise object"));
        goto end;
    }

    if ((r = rjs_species_constructor(rt, thiz, rjs_o_Promise(realm), c)) == RJS_ERR)
        goto end;

    if ((r = rjs_new_promise_capability(rt, c, &pc)) == RJS_ERR)
        goto end;

    r = rjs_perform_proimise_then(rt, thiz, on_fulfilled, on_rejected, &pc, rv);
end:
    rjs_promise_capability_deinit(rt, &pc);
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
promise_prototype_function_descs[] = {
    {
        "catch",
        1,
        Promise_prototype_catch
    },
    {
        "finally",
        1,
        Promise_prototype_finally
    },
    {
        "then",
        2,
        Promise_prototype_then
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
promise_prototype_desc = {
    "Promise",
    NULL,
    NULL,
    NULL,
    promise_prototype_field_descs,
    promise_prototype_function_descs,
    NULL,
    NULL,
    "Promise_prototype"
};
