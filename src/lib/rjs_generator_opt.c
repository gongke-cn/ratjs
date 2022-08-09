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

/*Call the generator function.*/
static RJS_Result
generator_function_op_call (RJS_Runtime *rt, RJS_Value *o, RJS_Value *thiz, RJS_Value *args, size_t argc, RJS_Value *rv)
{
    RJS_Result  r;
    RJS_Value  *fp    = rjs_value_get_pointer(rt, o);
    RJS_Value  *argsp = argc ? rjs_value_get_pointer(rt, args) : NULL;
    RJS_Value  *thisp = rjs_value_get_pointer(rt, thiz);
    RJS_Value  *rvp   = rjs_value_get_pointer(rt, rv);

    rjs_prepare_for_ordinary_call(rt, fp, rjs_v_undefined(rt), argsp, argc, NULL);

    rjs_ordinary_call_bind_this(rt, fp, thisp);

    r = rjs_script_func_call(rt, RJS_SCRIPT_CALL_SYNC_START, NULL, rvp);

    rjs_context_pop(rt);

    return r;
}

/*Generator function's operation functions.*/
static const RJS_ObjectOps
generator_function_ops = {
    {
        RJS_GC_THING_SCRIPT_FUNC,
        rjs_script_func_object_op_gc_scan,
        rjs_script_func_object_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS,
    generator_function_op_call
};

/*Scan the referenced things in the generator.*/
static void
generator_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_Generator *g = ptr;

    rjs_script_func_object_op_gc_scan(rt, ptr);

    if (g->context)
        rjs_gc_mark(rt, g->context);

    rjs_gc_scan_value(rt, &g->brand);
    rjs_gc_scan_value(rt, &g->iteratorv);
    rjs_gc_scan_value(rt, &g->nextv);
    rjs_gc_scan_value(rt, &g->receivedv);
}

/*Release the generator.*/
static void
generator_deinit (RJS_Runtime *rt, RJS_Generator *g)
{
    rjs_iterator_deinit(rt, &g->iterator);
    rjs_script_func_object_deinit(rt, &g->sfo);
}

/*Free the generator.*/
static void
generator_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_Generator *g = ptr;

    generator_deinit(rt, g);

    RJS_DEL(rt, g);
}

/*Generator's operation functions.*/
static const RJS_ObjectOps
generator_ops = {
    {
        RJS_GC_THING_GENERATOR,
        generator_op_gc_scan,
        generator_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS
};

/*Get the current generator.*/
static RJS_Generator*
generator_get (RJS_Runtime *rt)
{
    RJS_Context   *ctxt = rjs_context_running(rt);
    RJS_Value     *gv   = &ctxt->function;
    
    return (RJS_Generator*)rjs_value_get_object(rt, gv);
}

#if ENABLE_ASYNC

/*Scan the referenced things in the async generator.*/
static void
async_generator_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_AsyncGenerator        *ag = ptr;
    RJS_AsyncGeneratorRequest *agr;

    generator_op_gc_scan(rt, &ag->generator);

    rjs_list_foreach_c(&ag->queue, agr, RJS_AsyncGeneratorRequest, ln) {
        rjs_gc_scan_value(rt, &agr->value);
        rjs_gc_scan_value(rt, &agr->promise);
        rjs_gc_scan_value(rt, &agr->resolve);
        rjs_gc_scan_value(rt, &agr->reject);
    }
}

/*Free the async generator.*/
static void
async_generator_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_AsyncGenerator        *ag = ptr;
    RJS_AsyncGeneratorRequest *agr, *nagr;

    generator_deinit(rt, &ag->generator);

    rjs_list_foreach_safe_c(&ag->queue, agr, nagr, RJS_AsyncGeneratorRequest, ln) {
        rjs_promise_capability_deinit(rt, &agr->capability);
        RJS_DEL(rt, agr);
    }

    RJS_DEL(rt, ag);
}

/*Async generator's operation functions.*/
static const RJS_ObjectOps
async_generator_ops = {
    {
        RJS_GC_THING_ASYNC_GENERATOR,
        async_generator_op_gc_scan,
        async_generator_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS
};

/*Enqueue an async generator request.*/
static void
async_generator_enqueue (RJS_Runtime *rt, RJS_Value *gv, RJS_GeneratorRequestType type,
        RJS_Value *v, RJS_PromiseCapability *pc)
{
    RJS_AsyncGenerator        *ag  = (RJS_AsyncGenerator*)rjs_value_get_object(rt, gv);
    RJS_AsyncGeneratorRequest *agr;

    RJS_NEW(rt, agr);

    agr->type = type;

    rjs_value_set_undefined(rt, &agr->promise);
    rjs_value_set_undefined(rt, &agr->resolve);
    rjs_value_set_undefined(rt, &agr->reject);

    rjs_promise_capability_init_vp(rt, &agr->capability, &agr->promise, &agr->resolve, &agr->reject);
    rjs_promise_capability_copy(rt, &agr->capability, pc);

    rjs_value_copy(rt, &agr->value, v);

    rjs_list_append(&ag->queue, &agr->ln);
}

/**
 * Run an async generator complete step.
 * \param rt The current runtime.
 * \param gv The generator value.
 * \param type Request type.
 * \param rv The return value.
 * \param done The generator is done.
 * \param realm The realm.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_async_generator_complete_step (RJS_Runtime *rt, RJS_Value *gv, RJS_GeneratorRequestType type,
        RJS_Value *rv, RJS_Bool done, RJS_Realm *realm)
{
    RJS_AsyncGenerator        *ag  = (RJS_AsyncGenerator*)rjs_value_get_object(rt, gv);
    size_t                     top = rjs_value_stack_save(rt);
    RJS_Value                 *ro  = rjs_value_stack_push(rt);
    RJS_AsyncGeneratorRequest *agr;

    assert(!rjs_list_is_empty(&ag->queue));

    agr = RJS_CONTAINER_OF(ag->queue.next, RJS_AsyncGeneratorRequest, ln);

    if (type == RJS_GENERATOR_REQUEST_THROW) {
        rjs_call(rt, agr->capability.reject, rjs_v_undefined(rt), rv, 1, NULL);
    } else {
        assert(type == RJS_GENERATOR_REQUEST_NEXT);

        if (realm) {
            RJS_Realm *old_realm = rjs_realm_current(rt);

            rt->rb.bot_realm = realm;
            rjs_create_iter_result_object(rt, rv, done, ro);
            rt->rb.bot_realm = old_realm;
        } else {
            rjs_create_iter_result_object(rt, rv, done, ro);
        }

        rjs_call(rt, agr->capability.resolve, rjs_v_undefined(rt), ro, 1, NULL);
    }

    rjs_list_remove(&agr->ln);

    RJS_DEL(rt, agr);
    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}

/**Async generator native function.*/
typedef struct {
    RJS_BuiltinFuncObject bfo;   /**< Base built-in function object.*/
    RJS_Value             value; /**< The async generator value.*/
} RJS_AsyncGeneratorFunc;

/*Scan referenced things in the async generator function.*/
static void
async_generator_func_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_AsyncGeneratorFunc *agf = ptr;

    rjs_builtin_func_object_op_gc_scan(rt, ptr);

    rjs_gc_scan_value(rt, &agf->value);
}

/*Free the async generator function.*/
static void
async_generator_func_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_AsyncGeneratorFunc *agf = ptr;

    rjs_builtin_func_object_deinit(rt, &agf->bfo);

    RJS_DEL(rt, agf);
}

/*Async generator function operation functions.*/
static const RJS_ObjectOps
async_generator_func_ops = {
    {
        RJS_GC_THING_BUILTIN_FUNC,
        async_generator_func_op_gc_scan,
        async_generator_func_op_gc_free
    },
    RJS_BUILTIN_FUNCTION_OBJECT_OPS
};

/*Start the async generator.*/
static RJS_Result
async_generator_start (RJS_Runtime *rt, RJS_Value *rv)
{
    RJS_Environment       *env      = rjs_lex_env_running(rt);
    RJS_GeneratorContext  *gc       = (RJS_GeneratorContext*)rjs_context_running(rt);
    RJS_ScriptContext     *sc       = &gc->scontext;
    RJS_PrivateEnv        *priv_env = NULL;
    size_t                 top      = rjs_value_stack_save(rt);
    RJS_Value             *proto    = rjs_value_stack_push(rt);
    RJS_Result             r;
    RJS_AsyncGenerator    *ag;
    const RJS_ObjectOps   *ops;

    if ((r = rjs_get_prototype_from_constructor(rt, &sc->scb.context.function,
            RJS_O_AsyncGenerator_prototype, proto)) == RJS_ERR)
        goto end;
        
    RJS_NEW(rt, ag);

    ops = &async_generator_ops;

    ag->generator.state         = RJS_GENERATOR_STATE_UNDEFINED;
    ag->generator.received_type = RJS_GENERATOR_REQUEST_NEXT;
    ag->generator.context       = NULL;

    rjs_list_init(&ag->queue);

    rjs_value_copy(rt, &ag->generator.brand, rjs_s_empty(rt));
    rjs_value_set_undefined(rt, &ag->generator.iteratorv);
    rjs_value_set_undefined(rt, &ag->generator.nextv);
    rjs_value_set_undefined(rt, &ag->generator.receivedv);

    rjs_iterator_init_vp(rt, &ag->generator.iterator, &ag->generator.iteratorv, &ag->generator.nextv);

#if ENABLE_PRIV_NAME
    priv_env = sc->scb.priv_env;
#endif /*ENABLE_PRIV_NAME*/

    rjs_script_func_object_init(rt, rv, &ag->generator.sfo, proto, sc->script, sc->script_func, env,
            priv_env, ops);

    /*Start the generator.*/
    ag->generator.state   = RJS_GENERATOR_STATE_SUSPENDED_START;
    ag->generator.context = &sc->scb.context;

    /*Store the generator.*/
    rjs_value_copy(rt, &sc->scb.context.function, rv);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Async generator await return on fulfill native function.*/
static RJS_Result
async_generator_await_return_fulfill (RJS_Runtime *rt, RJS_Value *f, RJS_Value *thiz, RJS_Value *args,
        size_t argc, RJS_Value *nt, RJS_Value *rv)
{
    RJS_AsyncGeneratorFunc *agf = (RJS_AsyncGeneratorFunc*)rjs_value_get_object(rt, f);
    RJS_AsyncGenerator     *ag  = (RJS_AsyncGenerator*)rjs_value_get_object(rt, &agf->value);
    RJS_Value              *v   = rjs_argument_get(rt, args, argc, 0);

    ag->generator.state = RJS_GENERATOR_STATE_COMPLETED;

    rjs_async_generator_complete_step(rt, &agf->value, RJS_GENERATOR_REQUEST_NEXT, v, RJS_TRUE, NULL);
    rjs_async_generator_drain_queue(rt, &agf->value);

    rjs_value_set_undefined(rt, rv);
    return RJS_OK;
}

/*Async generator await return on reject native function.*/
static RJS_Result
async_generator_await_return_reject (RJS_Runtime *rt, RJS_Value *f, RJS_Value *thiz, RJS_Value *args,
        size_t argc, RJS_Value *nt, RJS_Value *rv)
{
    RJS_AsyncGeneratorFunc *agf = (RJS_AsyncGeneratorFunc*)rjs_value_get_object(rt, f);
    RJS_AsyncGenerator     *ag  = (RJS_AsyncGenerator*)rjs_value_get_object(rt, &agf->value);
    RJS_Value              *v   = rjs_argument_get(rt, args, argc, 0);

    ag->generator.state = RJS_GENERATOR_STATE_COMPLETED;

    rjs_async_generator_complete_step(rt, &agf->value, RJS_GENERATOR_REQUEST_THROW, v, RJS_TRUE, NULL);
    rjs_async_generator_drain_queue(rt, &agf->value);

    rjs_value_set_undefined(rt, rv);
    return RJS_OK;
}

/*Async wait return.*/
static RJS_Result
async_generator_await_return (RJS_Runtime *rt, RJS_Value *gv)
{
    RJS_AsyncGenerator        *ag      = (RJS_AsyncGenerator*)rjs_value_get_object(rt, gv);
    RJS_Realm                 *realm   = rjs_realm_current(rt);
    size_t                     top     = rjs_value_stack_save(rt);
    RJS_Value                 *promise = rjs_value_stack_push(rt);
    RJS_Value                 *fulfill = rjs_value_stack_push(rt);
    RJS_Value                 *reject  = rjs_value_stack_push(rt);
    RJS_Value                 *rv      = rjs_value_stack_push(rt);
    RJS_Value                 *errv    = rjs_value_stack_push(rt);
    RJS_AsyncGeneratorRequest *agr;
    RJS_AsyncGeneratorFunc    *agf;
    RJS_Result                 r;

    agr = RJS_CONTAINER_OF(ag->queue.next, RJS_AsyncGeneratorRequest, ln);

    assert(agr->type == RJS_GENERATOR_REQUEST_RETURN);

    if ((r = rjs_promise_resolve(rt, rjs_o_Promise(realm), &agr->value, promise)) == RJS_ERR) {
        ag->generator.state = RJS_GENERATOR_STATE_COMPLETED;

        rjs_catch(rt, errv);
        rjs_async_generator_complete_step(rt, gv, RJS_GENERATOR_REQUEST_THROW, errv, RJS_TRUE, NULL);
        rjs_async_generator_drain_queue(rt, gv);
        r = RJS_OK;
        goto end;
    }

    RJS_NEW(rt, agf);
    rjs_value_copy(rt, &agf->value, gv);
    rjs_init_builtin_function(rt, &agf->bfo, async_generator_await_return_fulfill, 0,
            &async_generator_func_ops, 1, rjs_s_empty(rt), realm, NULL, NULL, NULL, fulfill);

    RJS_NEW(rt, agf);
    rjs_value_copy(rt, &agf->value, gv);
    rjs_init_builtin_function(rt, &agf->bfo, async_generator_await_return_reject, 0,
            &async_generator_func_ops, 1, rjs_s_empty(rt), realm, NULL, NULL, NULL, reject);

    r = rjs_perform_proimise_then(rt, promise, fulfill, reject, NULL, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Drain the async generator's request queue.
 * \param rt The current runtime.
 * \param gv The generator value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_async_generator_drain_queue (RJS_Runtime *rt, RJS_Value *gv)
{
    RJS_AsyncGenerator        *ag  = (RJS_AsyncGenerator*)rjs_value_get_object(rt, gv);
    RJS_AsyncGeneratorRequest *agr, *nagr;

    assert(ag->generator.state == RJS_GENERATOR_STATE_COMPLETED);

    rjs_list_foreach_safe_c(&ag->queue, agr, nagr, RJS_AsyncGeneratorRequest, ln) {
        if (agr->type == RJS_GENERATOR_REQUEST_RETURN) {
            ag->generator.state = RJS_GENERATOR_STATE_AWAIT_RETURN;
            async_generator_await_return(rt, gv);
            break;
        } else {
            rjs_async_generator_complete_step(rt, gv, agr->type, &agr->value, RJS_TRUE, NULL);
        }
    }

    return RJS_OK;
}

/*Check if the generator is async generator.*/
static RJS_Bool
generator_is_async (RJS_Runtime *rt, RJS_Generator *g)
{
    RJS_ScriptContext *sc = (RJS_ScriptContext*)g->context;

    return (sc->script_func->flags & RJS_FUNC_FL_ASYNC) ? RJS_TRUE : RJS_FALSE;
}

/*Async generator unwrap yield.*/
static RJS_Result
async_generator_unwrap_yield_resumption (RJS_Runtime *rt, RJS_AsyncGeneratorRequest *agr,
        RJS_AsyncOpFunc op, size_t ip, RJS_Value *vp)
{
    RJS_Context        *ctxt = rjs_context_running(rt);
    RJS_Value          *gv   = &ctxt->function;
    RJS_AsyncGenerator *ag   = (RJS_AsyncGenerator*)rjs_value_get_object(rt, gv);
    RJS_Result          r;

    if (agr->type == RJS_GENERATOR_REQUEST_NEXT) {
        rjs_value_copy(rt, &ag->generator.receivedv, &agr->value);
        ag->generator.received_type = RJS_GENERATOR_REQUEST_NEXT;
        r = RJS_OK;
    } else if (agr->type == RJS_GENERATOR_REQUEST_THROW) {
        rjs_value_copy(rt, &ag->generator.receivedv, &agr->value);
        ag->generator.received_type = RJS_GENERATOR_REQUEST_THROW;
        r = RJS_OK;
    } else {
        r = rjs_await(rt, &agr->value, op, ip, vp);
    }

    return r;
}

/*Await yield return operation.*/
static RJS_Result
await_async_generator_yield_return (RJS_Runtime *rt, RJS_ScriptCallType type, RJS_Value *iv, RJS_Value *rv)
{
    ASYNC_OP_DEBUG();

    if (type == RJS_SCRIPT_CALL_ASYNC_REJECT)
        return rjs_throw(rt, iv);

    rjs_value_copy(rt, rv, iv);
    return RJS_RETURN;
}

/*Await async generator yield operation.*/
static RJS_Result
await_async_generator_yield (RJS_Runtime *rt, RJS_ScriptCallType type, RJS_Value *iv, RJS_Value *rv)
{
    RJS_Context             *ctxt = rjs_context_running(rt);
    RJS_Value               *gv   = &ctxt->function;
    RJS_AsyncGenerator      *ag   = (RJS_AsyncGenerator*)rjs_value_get_object(rt, gv);
    RJS_Context             *prev_ctxt;
    RJS_Realm               *prev_realm;
    RJS_GeneratorRequestType rtype;
    RJS_Result               r;

    ASYNC_OP_DEBUG();

    if (type == RJS_SCRIPT_CALL_ASYNC_REJECT) {
        rjs_throw(rt, iv);
        return RJS_ERR;
    }

    rtype      = RJS_GENERATOR_REQUEST_NEXT;
    prev_ctxt  = ctxt->bot;
    prev_realm = prev_ctxt->realm;

    rjs_async_generator_complete_step(rt, gv, rtype, iv, RJS_FALSE, prev_realm);

    if (!rjs_list_is_empty(&ag->queue)) {
        r = RJS_OK;
    } else {
        ag->generator.state = RJS_GENERATOR_STATE_SUSPENDED_YIELD;
        r = RJS_SUSPEND;
    }

    return r;
}

/*Await and return for the async generator iterator.*/
static RJS_Result
await_async_generator_iterator_return (RJS_Runtime *rt, RJS_ScriptCallType type, RJS_Value *iv, RJS_Value *rv)
{
    RJS_Context        *ctxt = rjs_context_running(rt);
    RJS_Value          *gv   = &ctxt->function;
    RJS_AsyncGenerator *ag   = (RJS_AsyncGenerator*)rjs_value_get_object(rt, gv);

    if (type == RJS_SCRIPT_CALL_ASYNC_REJECT) {
        rjs_value_copy(rt, &ag->generator.receivedv, iv);
        ag->generator.received_type = RJS_GENERATOR_REQUEST_THROW;
    } else {
        rjs_value_copy(rt, &ag->generator.receivedv, iv);
        ag->generator.received_type = RJS_GENERATOR_REQUEST_RETURN;
    }

    return RJS_OK;
}

/*Get the next async generator request for the iterator.*/
static RJS_Result
async_generator_iterator_next (RJS_Runtime *rt, RJS_AsyncGenerator *ag)
{
    RJS_AsyncGeneratorRequest *agr;
    RJS_Result                 r;

    assert(!rjs_list_is_empty(&ag->queue));

    agr = RJS_CONTAINER_OF(ag->queue.next, RJS_AsyncGeneratorRequest, ln);

    if (agr->type == RJS_GENERATOR_REQUEST_NEXT) {
        rjs_value_copy(rt, &ag->generator.receivedv, &agr->value);
        ag->generator.received_type = RJS_GENERATOR_REQUEST_NEXT;
        r = RJS_OK;
    } else if (agr->type == RJS_GENERATOR_REQUEST_THROW) {
        rjs_value_copy(rt, &ag->generator.receivedv, &agr->value);
        ag->generator.received_type = RJS_GENERATOR_REQUEST_THROW;
        r = RJS_OK;
    } else {
        r = rjs_await(rt, &agr->value, await_async_generator_iterator_return, 0, NULL);
    }

    return r;
}

/*Await and get the next async generator request for the iterator.*/
static RJS_Result
await_async_generator_iterator_next (RJS_Runtime *rt, RJS_ScriptCallType type, RJS_Value *iv, RJS_Value *rv)
{
    RJS_Context        *ctxt = rjs_context_running(rt);
    RJS_Value          *gv   = &ctxt->function;
    RJS_AsyncGenerator *ag   = (RJS_AsyncGenerator*)rjs_value_get_object(rt, gv);

    if (type == RJS_SCRIPT_CALL_ASYNC_REJECT)
        return rjs_throw(rt, iv);

    return async_generator_iterator_next(rt, ag);
}

/*Await async generator iterator yield next operation.*/
static RJS_Result
await_async_generator_iterator_yield_next (RJS_Runtime *rt, RJS_ScriptCallType type, RJS_Value *iv, RJS_Value *rv)
{
    RJS_Result          r;
    RJS_Bool            done;
    RJS_Context        *ctxt = rjs_context_running(rt);
    RJS_Value          *gv   = &ctxt->function;
    RJS_AsyncGenerator *ag   = (RJS_AsyncGenerator*)rjs_value_get_object(rt, gv);
    size_t              top  = rjs_value_stack_save(rt);
    RJS_Value          *v    = rjs_value_stack_push(rt);

    ASYNC_OP_DEBUG();

    if (type == RJS_SCRIPT_CALL_ASYNC_REJECT) {
        r = rjs_throw(rt, iv);
        goto end;
    }

    if (!rjs_value_is_object(rt, iv)) {
        r = rjs_throw_type_error(rt, _("the result is not an object"));
        goto end;
    }

    if ((r = rjs_iterator_complete(rt, iv)) == RJS_ERR)
        goto end;
    done = r;

    if ((r = rjs_iterator_value(rt, iv, v)) == RJS_ERR)
        goto end;

    if (done) {
        if (ag->generator.received_type == RJS_GENERATOR_REQUEST_RETURN) {
            rjs_value_copy(rt, rv, v);
            r = RJS_RETURN;
        } else {
            ag->generator.received_type = RJS_GENERATOR_REQUEST_END;
            rjs_value_copy(rt, &ag->generator.receivedv, v);
            r = RJS_OK;
        }
    } else {
        if ((r = await_async_generator_yield(rt, type, v, rv)) == RJS_ERR)
            goto end;

        if (r == RJS_OK) {
            r = async_generator_iterator_next(rt, ag);
        } else {
            RJS_AsyncContext *ac = (RJS_AsyncContext*)ctxt;

            ac->op = await_async_generator_iterator_next;
            r = RJS_SUSPEND;
        }
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Await throw type error.*/
static RJS_Result
await_throw_type_error (RJS_Runtime *rt, RJS_ScriptCallType type, RJS_Value *iv, RJS_Value *rv)
{
    ASYNC_OP_DEBUG();

    if (type == RJS_SCRIPT_CALL_ASYNC_REJECT)
        return rjs_throw(rt, iv);

    return rjs_throw_type_error(rt, _("iterator has not \"throw\" method"));
}

/*Await async generator iterator yield return when "return" is undefined.*/
static RJS_Result
await_async_generator_iterator_yield_return_undefined (RJS_Runtime *rt, RJS_ScriptCallType type, RJS_Value *iv, RJS_Value *rv)
{
    ASYNC_OP_DEBUG();

    if (type == RJS_SCRIPT_CALL_ASYNC_REJECT)
        return rjs_throw(rt, iv);

    rjs_value_copy(rt, rv, iv);
    return RJS_RETURN;
}

/*Async generator's next iterator yield operation.*/
static RJS_Result
async_generator_iterator_yield_next (RJS_Runtime *rt, RJS_Value *rv)
{
    RJS_Generator *g   = generator_get(rt);
    size_t         top = rjs_value_stack_save(rt);
    RJS_Value     *ir  = rjs_value_stack_push(rt);
    RJS_Value     *fn  = rjs_value_stack_push(rt);
    RJS_Result     r;

    switch (g->received_type) {
    case RJS_GENERATOR_REQUEST_NEXT:
        if ((r = rjs_call(rt, g->iterator.next_method, g->iterator.iterator, &g->receivedv, 1, ir)) == RJS_ERR)
            goto end;

        r = rjs_await(rt, ir, await_async_generator_iterator_yield_next, 0, NULL);
        goto end;
    case RJS_GENERATOR_REQUEST_THROW:
        if ((r = rjs_get_method(rt, g->iterator.iterator, rjs_pn_throw(rt), fn)) == RJS_ERR)
            goto end;

        if (!rjs_value_is_undefined(rt, fn)) {
            if ((r = rjs_call(rt, fn, g->iterator.iterator, &g->receivedv, 1, ir)) == RJS_ERR)
                goto end;

            r = rjs_await(rt, ir, await_async_generator_iterator_yield_next, 0, NULL);
            goto end;
        } else {
            if ((r = rjs_async_iterator_close(rt, &g->iterator, await_throw_type_error, 0, NULL)) == RJS_ERR)
                goto end;

            if (!r)
                goto end;

            r = rjs_throw_type_error(rt, _("iterator has not \"throw\" method"));
        }
        break;
    case RJS_GENERATOR_REQUEST_RETURN:
        if ((r = rjs_get_method(rt, g->iterator.iterator, rjs_pn_return(rt), fn)) == RJS_ERR)
            goto end;

        if (rjs_value_is_undefined(rt, fn)) {
            r = rjs_await(rt, &g->receivedv, await_async_generator_iterator_yield_return_undefined, 0, NULL);
            goto end;
        }

        if ((r = rjs_call(rt, fn, g->iterator.iterator, &g->receivedv, 1, ir)) == RJS_ERR)
            goto end;

        r = rjs_await(rt, ir, await_async_generator_iterator_yield_next, 0, NULL);
        break;
    case RJS_GENERATOR_REQUEST_END:
        rjs_value_copy(rt, rv, &g->receivedv);
        r = RJS_OK;
        break;
    default:
        assert(0);
        r = RJS_ERR;
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Validate the async generator.*/
static RJS_Result
async_generator_validate (RJS_Runtime *rt, RJS_Value *gv, RJS_Value *brand)
{
    RJS_AsyncGenerator *ag;

    if (rjs_value_get_gc_thing_type(rt, gv) != RJS_GC_THING_ASYNC_GENERATOR)
        return rjs_throw_type_error(rt, _("the value is not an async generator"));

    ag = (RJS_AsyncGenerator*)rjs_value_get_object(rt, gv);

    if (!rjs_same_value(rt, &ag->generator.brand, brand))
        return rjs_throw_type_error(rt, _("async generator's brand mismatch"));

    return RJS_OK;
}

/*Invoke reject if arupt.*/
static RJS_Result
if_abrupt_reject_promise (RJS_Runtime *rt, RJS_Result r, RJS_PromiseCapability *pc, RJS_Value *rv)
{
    if (r == RJS_ERR) {
        rjs_call(rt, pc->reject, rjs_v_undefined(rt), &rt->error, 1, NULL);
        rjs_value_copy(rt, rv, pc->promise);
    }

    return r;
}

/*Resume the generator.*/
static RJS_Result
async_generator_resume (RJS_Runtime *rt, RJS_Value *gv, RJS_Value *v)
{
    RJS_AsyncGenerator *ag  = (RJS_AsyncGenerator*)rjs_value_get_object(rt, gv);
    size_t              top = rjs_value_stack_save(rt);
    RJS_Value          *rv  = rjs_value_stack_push(rt);
    RJS_Value          *vp  = rjs_value_get_pointer(rt, v);
    RJS_Value          *rvp = rjs_value_get_pointer(rt, rv);
    RJS_Result          r;

    ag->generator.state = RJS_GENERATOR_STATE_EXECUTING;

    rjs_context_restore(rt, ag->generator.context);

    r = rjs_script_func_call(rt, RJS_SCRIPT_CALL_ASYNC_FULFILL, vp, rvp);

    rjs_context_pop(rt);

    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * AsyncGenerator.prototype.next operation.
 * \param rt The current runtime.
 * \param agv The async generator.
 * \param v The input value.
 * \param[out] rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_async_generator_next (RJS_Runtime *rt, RJS_Value *agv, RJS_Value *v, RJS_Value *rv)
{
    RJS_Realm             *realm = rjs_realm_current(rt);
    size_t                 top   = rjs_value_stack_save(rt);
    RJS_Value             *ir    = rjs_value_stack_push(rt);
    RJS_AsyncGenerator    *ag;
    RJS_PromiseCapability  pc;
    RJS_Result             r;

    rjs_promise_capability_init(rt, &pc);

    rjs_new_promise_capability(rt, rjs_o_Promise(realm), &pc);

    r = async_generator_validate(rt, agv, rjs_s_empty(rt));
    if (if_abrupt_reject_promise(rt, r, &pc, rv) == RJS_ERR)
        goto end;

    ag = (RJS_AsyncGenerator*)rjs_value_get_object(rt, agv);

    if (ag->generator.state == RJS_GENERATOR_STATE_COMPLETED) {
        rjs_create_iter_result_object(rt, rjs_v_undefined(rt), RJS_TRUE, ir);
        rjs_call(rt, pc.resolve, rjs_v_undefined(rt), ir, 1, NULL);

        rjs_value_copy(rt, rv, pc.promise);
    } else {
        async_generator_enqueue(rt, agv, RJS_GENERATOR_REQUEST_NEXT, v, &pc);
    }

    if ((ag->generator.state == RJS_GENERATOR_STATE_SUSPENDED_YIELD)
            || (ag->generator.state == RJS_GENERATOR_STATE_SUSPENDED_START)) {
        async_generator_resume(rt, agv, v);
    }

    rjs_value_copy(rt, rv, pc.promise);
end:
    rjs_promise_capability_deinit(rt, &pc);
    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}

/**
 * AsyncGenerator.prototype.return operation.
 * \param rt The current runtime.
 * \param agv The async generator.
 * \param v The input value.
 * \param[out] rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_async_generator_return (RJS_Runtime *rt, RJS_Value *agv, RJS_Value *v, RJS_Value *rv)
{
    RJS_Realm             *realm = rjs_realm_current(rt);
    size_t                 top   = rjs_value_stack_save(rt);
    RJS_AsyncGenerator    *ag;
    RJS_PromiseCapability  pc;
    RJS_Result             r;

    rjs_promise_capability_init(rt, &pc);

    rjs_new_promise_capability(rt, rjs_o_Promise(realm), &pc);

    r = async_generator_validate(rt, agv, rjs_s_empty(rt));
    if (if_abrupt_reject_promise(rt, r, &pc, rv) == RJS_ERR)
        goto end;

    ag = (RJS_AsyncGenerator*)rjs_value_get_object(rt, agv);

    async_generator_enqueue(rt, agv, RJS_GENERATOR_REQUEST_RETURN, v, &pc);

    if ((ag->generator.state == RJS_GENERATOR_STATE_COMPLETED)
            || (ag->generator.state == RJS_GENERATOR_STATE_SUSPENDED_START)) {
        ag->generator.state = RJS_GENERATOR_STATE_AWAIT_RETURN;

        async_generator_await_return(rt, agv);
    } else if (ag->generator.state == RJS_GENERATOR_STATE_SUSPENDED_YIELD) {
        async_generator_resume(rt, agv, v);
    }

    rjs_value_copy(rt, rv, pc.promise);
end:
    rjs_promise_capability_deinit(rt, &pc);
    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}

/**
 * AsyncGenerator.prototype.throw operation.
 * \param rt The current runtime.
 * \param agv The async generator.
 * \param v The input value.
 * \param[out] rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_async_generator_throw (RJS_Runtime *rt, RJS_Value *agv, RJS_Value *v, RJS_Value *rv)
{
    RJS_Realm             *realm = rjs_realm_current(rt);
    size_t                 top   = rjs_value_stack_save(rt);
    RJS_AsyncGenerator    *ag;
    RJS_PromiseCapability  pc;
    RJS_Result             r;

    rjs_promise_capability_init(rt, &pc);

    rjs_new_promise_capability(rt, rjs_o_Promise(realm), &pc);

    r = async_generator_validate(rt, agv, rjs_s_empty(rt));
    if (if_abrupt_reject_promise(rt, r, &pc, rv) == RJS_ERR)
        goto end;

    ag = (RJS_AsyncGenerator*)rjs_value_get_object(rt, agv);

    if (ag->generator.state == RJS_GENERATOR_STATE_SUSPENDED_START) {
        ag->generator.state = RJS_GENERATOR_STATE_COMPLETED;
    }
    
    if (ag->generator.state == RJS_GENERATOR_STATE_COMPLETED) {
        rjs_call(rt, pc.reject, rjs_v_undefined(rt), v, 1, NULL);
    } else {
        async_generator_enqueue(rt, agv, RJS_GENERATOR_REQUEST_THROW, v, &pc);

        if (ag->generator.state == RJS_GENERATOR_STATE_SUSPENDED_YIELD)
            async_generator_resume(rt, agv, v);
    }

    rjs_value_copy(rt, rv, pc.promise);
end:
    rjs_promise_capability_deinit(rt, &pc);
    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}

#endif /*ENABLE_ASYNC*/

/*Start the generator.*/
static RJS_Result
generator_start (RJS_Runtime *rt, RJS_Value *rv)
{
    RJS_Environment       *env      = rjs_lex_env_running(rt);
    RJS_GeneratorContext  *gc       = (RJS_GeneratorContext*)rjs_context_running(rt);
    RJS_ScriptContext     *sc       = &gc->scontext;
    RJS_PrivateEnv        *priv_env = NULL;
    size_t                 top      = rjs_value_stack_save(rt);
    RJS_Value             *proto    = rjs_value_stack_push(rt);
    RJS_Result             r;
    RJS_Generator         *g;
    const RJS_ObjectOps   *ops;

    if ((r = rjs_get_prototype_from_constructor(rt, &sc->scb.context.function,
            RJS_O_Generator_prototype, proto)) == RJS_ERR)
        goto end;
        
    RJS_NEW(rt, g);

    ops = &generator_ops;

    g->state         = RJS_GENERATOR_STATE_UNDEFINED;
    g->received_type = RJS_GENERATOR_REQUEST_NEXT;
    g->context       = NULL;

    rjs_value_copy(rt, &g->brand, rjs_s_empty(rt));
    rjs_value_set_undefined(rt, &g->iteratorv);
    rjs_value_set_undefined(rt, &g->nextv);
    rjs_value_set_undefined(rt, &g->receivedv);

    rjs_iterator_init_vp(rt, &g->iterator, &g->iteratorv, &g->nextv);

#if ENABLE_PRIV_NAME
    priv_env = sc->scb.priv_env;
#endif /*ENABLE_PRIV_NAME*/

    rjs_script_func_object_init(rt, rv, &g->sfo, proto, sc->script, sc->script_func, env,
            priv_env, ops);

    /*Start the generator.*/
    g->state   = RJS_GENERATOR_STATE_SUSPENDED_START;
    g->context = &sc->scb.context;

    /*Store the generator.*/
    rjs_value_copy(rt, &sc->scb.context.function, rv);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Validate the generator.*/
static RJS_Result
generator_validate (RJS_Runtime *rt, RJS_Value *gv, RJS_Value *brand)
{
    RJS_Generator *g;

    if (rjs_value_get_gc_thing_type(rt, gv) != RJS_GC_THING_GENERATOR)
        return rjs_throw_type_error(rt, _("the value is not a generator"));

    g = (RJS_Generator*)rjs_value_get_object(rt, gv);

    if (!rjs_same_value(rt, &g->brand, brand))
        return rjs_throw_type_error(rt, _("generator's brand mismatch"));

    if (g->state == RJS_GENERATOR_STATE_EXECUTING)
        return rjs_throw_type_error(rt, _("the generator is executing"));

    return g->state;
}

/*Resume the generator.*/
static RJS_Result
generator_resume (RJS_Runtime *rt, RJS_Value *gv, RJS_Value *v, RJS_Value *brand, RJS_Value *rv)
{
    RJS_Generator *g   = (RJS_Generator*)rjs_value_get_object(rt, gv);
    RJS_Value     *vp  = rjs_value_get_pointer(rt, v);
    RJS_Value     *rvp = rjs_value_get_pointer(rt, rv);
    RJS_Result     r;

    if ((r = generator_validate(rt, gv, brand)) == RJS_ERR)
        return r;

    if (g->state == RJS_GENERATOR_STATE_COMPLETED) {
        rjs_create_iter_result_object(rt, rjs_v_undefined(rt), RJS_TRUE, rv);
        return RJS_OK;
    }

    assert((r == RJS_GENERATOR_STATE_SUSPENDED_START) || (r == RJS_GENERATOR_STATE_SUSPENDED_YIELD));

    g->state = RJS_GENERATOR_STATE_EXECUTING;

    rjs_context_restore(rt, g->context);

    r = rjs_script_func_call(rt, RJS_SCRIPT_CALL_GENERATOR_RESUME, vp, rvp);

    rjs_context_pop(rt);

    return r;
}

/*Resume the generator with abrupt.*/
static RJS_Result
generator_resume_abrupt (RJS_Runtime *rt, RJS_Value *gv, RJS_GeneratorAbruptType type,
        RJS_Value *v, RJS_Value *brand, RJS_Value *rv)
{
    RJS_Generator     *g   = (RJS_Generator*)rjs_value_get_object(rt, gv);
    RJS_Value         *vp  = rjs_value_get_pointer(rt, v);
    RJS_Value         *rvp = rjs_value_get_pointer(rt, rv);
    RJS_Result         r;
    RJS_ScriptCallType ct;

    if ((r = generator_validate(rt, gv, brand)) == RJS_ERR)
        return r;

    if (r == RJS_GENERATOR_STATE_SUSPENDED_START)
        g->state = RJS_GENERATOR_STATE_COMPLETED;

    if (g->state == RJS_GENERATOR_STATE_COMPLETED) {
        if (type == RJS_GENERATOR_ABRUPT_RETURN) {
            rjs_create_iter_result_object(rt, v, RJS_TRUE, rv);
            return RJS_OK;
        }

        return rjs_throw(rt, v);
    }

    assert(g->state == RJS_GENERATOR_STATE_SUSPENDED_YIELD);

    g->state = RJS_GENERATOR_STATE_EXECUTING;

    rjs_context_restore(rt, g->context);

    ct = (type == RJS_GENERATOR_ABRUPT_RETURN)
            ? RJS_SCRIPT_CALL_GENERATOR_RETURN
            : RJS_SCRIPT_CALL_GENERATOR_THROW;

    r = rjs_script_func_call(rt, ct, vp, rvp);

    rjs_context_pop(rt);

    return r;
}

/**
 * Create and start a generator.
 * \param rt The current runtime.
 * \param[out] rv Return the generator.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_generator_start (RJS_Runtime *rt, RJS_Value *rv)
{
    RJS_Result r;

#if ENABLE_ASYNC
    RJS_Context *ctxt = rjs_context_running(rt);

    if (ctxt->gc_thing.ops->type == RJS_GC_THING_ASYNC_CONTEXT) {
        r = async_generator_start(rt, rv);
    } else
#endif /*ENABLE_ASYNC*/
    {
        r = generator_start(rt, rv);
    }

    return r;
}

/**
 * Create a new generator function object.
 * \param rt The current runtime.
 * \param[out] f Return the new generator function object.
 * \param def_proto The default prototype.
 * \param script The script.
 * \param sf The script function.
 * \param env The environment.
 * \param priv_env The private environment.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_generator_function_new (RJS_Runtime *rt, RJS_Value *f, RJS_Value *def_proto,
        RJS_Script *script, RJS_ScriptFunc *sf, RJS_Environment *env,
        RJS_PrivateEnv *priv_env)
{
    RJS_Realm            *realm = rjs_realm_current(rt);
    size_t                top   = rjs_value_stack_save(rt);
    RJS_Value            *proto = rjs_value_stack_push(rt);
    RJS_ScriptFuncObject *sfo;
    RJS_PropertyDesc      pd;
    RJS_Value            *constr_proto;
    RJS_Value            *proto_proto;

    /*Get the prototypes.*/
#if ENABLE_ASYNC
    if (sf->flags & RJS_FUNC_FL_ASYNC) {
        constr_proto = rjs_o_AsyncGeneratorFunction_prototype(realm);
        proto_proto  = rjs_o_AsyncGenerator_prototype(realm);
    } else
#endif /*ENABLE_ASYNC*/
    {
        constr_proto = rjs_o_GeneratorFunction_prototype(realm);
        proto_proto  = rjs_o_Generator_prototype(realm);
    }

    if (!def_proto)
        def_proto = constr_proto;

    /*Create the script function.*/
    RJS_NEW(rt, sfo);

    rjs_script_func_object_init(rt, f, sfo, def_proto, script, sf, env, priv_env,
            &generator_function_ops);

    /*Set the function's name.*/
    if (sf->name_idx != RJS_INVALID_VALUE_INDEX) {
        RJS_Value *name = &script->value_table[sf->name_idx];

        rjs_set_function_name(rt, f, name, NULL);
    }

    /*Set the length.*/
    rjs_set_function_length(rt, f, sf->param_len);

    /*Set "prototype".*/
    rjs_ordinary_object_create(rt, proto_proto, proto);

    rjs_property_desc_init(rt, &pd);

    pd.flags = RJS_PROP_FL_DATA|RJS_PROP_FL_WRITABLE;
    rjs_value_copy(rt, pd.value, proto);

    rjs_define_property_or_throw(rt, f, rjs_pn_prototype(rt), &pd);

    rjs_property_desc_deinit(rt, &pd);

    rjs_value_stack_restore(rt, top);

    return RJS_OK;
}

/**
 * Yield the generator.
 * \param rt The current runtime.
 * \param v The input value.
 * \param[out] rv The return value.
 * \retval RJS_SUSPEND Function suspended.
 */
RJS_Result
rjs_yield (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv)
{
    RJS_Generator *g = generator_get(rt);
    RJS_Result     r;

#if ENABLE_ASYNC
    if (generator_is_async(rt, g)) {
        r = rjs_await(rt, v, await_async_generator_yield, 0, NULL);
    } else
#endif /*ENABLE_ASYNC*/
    {
        g->state = RJS_GENERATOR_STATE_SUSPENDED_YIELD;

        rjs_create_iter_result_object(rt, v, RJS_FALSE, rv);

        r = RJS_SUSPEND;
    }

    return r;
}

/**
 * Resume the yield expression.
 * \param rt The current runtime.
 * \param[out] result The result value.
 * \param[out] rv The return value.
 * \retval RJS_NEXT Resumed.
 * \retval RJS_THROW An error throwed.
 * \retval RJS_RETURN Return from the generator.
 */
RJS_Result
rjs_yield_resume (RJS_Runtime *rt, RJS_Value *result, RJS_Value *rv)
{
    RJS_Generator *g = generator_get(rt);
    RJS_Result     r = RJS_NEXT;

#if ENABLE_ASYNC
    if (generator_is_async(rt, g)) {
        RJS_AsyncGenerator        *ag = (RJS_AsyncGenerator*)g;
        RJS_AsyncGeneratorRequest *agr;

        assert(!rjs_list_is_empty(&ag->queue));

        agr = RJS_CONTAINER_OF(ag->queue.next, RJS_AsyncGeneratorRequest, ln);

        if ((r = async_generator_unwrap_yield_resumption(rt, agr, await_async_generator_yield_return, 0, NULL)) != RJS_OK)
            return r;
    }
#endif /*ENABLE_ASYNC*/

    {
        switch (g->received_type) {
        case RJS_GENERATOR_REQUEST_NEXT:
            rjs_value_copy(rt, result, &g->receivedv);
            r = RJS_NEXT;
            break;
        case RJS_GENERATOR_REQUEST_THROW:
            rjs_throw(rt, &g->receivedv);
            r = RJS_THROW;
            break;
        case RJS_GENERATOR_REQUEST_RETURN:
            rjs_value_copy(rt, rv, &g->receivedv);
            r = RJS_RETURN;
            break;
        default:
            assert(0);
            r = RJS_ERR;
        }
    }

    return r;
}

/**
 * Start iterator yield operation.
 * \param rt The current runtime.
 * \param v The input value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_iterator_yield_start (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_Generator    *g = generator_get(rt);
    RJS_IteratorType  type;
    RJS_Result        r;

#if ENABLE_ASYNC
    type = generator_is_async(rt, g) ? RJS_ITERATOR_ASYNC : RJS_ITERATOR_SYNC;
#else
    type = RJS_ITERATOR_SYNC;
#endif

    g->iterator.done = RJS_FALSE;
    g->received_type = RJS_GENERATOR_REQUEST_NEXT;

    rjs_value_set_undefined(rt, &g->receivedv);

    if ((r = rjs_get_iterator(rt, v, type, NULL, &g->iterator)) == RJS_ERR)
        return r;

    return RJS_OK;
}

/*Generator's next iterator yield operation.*/
static RJS_Result
generator_iterator_yield_next (RJS_Runtime *rt, RJS_Value *rv)
{
    RJS_Generator *g   = generator_get(rt);
    size_t         top = rjs_value_stack_save(rt);
    RJS_Value     *ir  = rjs_value_stack_push(rt);
    RJS_Value     *fn  = rjs_value_stack_push(rt);
    RJS_Result     r;
    RJS_Bool       done;

    switch (g->received_type) {
    case RJS_GENERATOR_REQUEST_NEXT:
        if ((r = rjs_call(rt, g->iterator.next_method, g->iterator.iterator, &g->receivedv, 1, ir)) == RJS_ERR)
            goto end;

        if (!rjs_value_is_object(rt, ir)) {
            r = rjs_throw_type_error(rt, _("the iterator result is not an object"));
            goto end;
        }

        if ((r = rjs_iterator_complete(rt, ir)) == RJS_ERR)
            goto end;

        done = r;

        if (done) {
            if ((r = rjs_iterator_value(rt, ir, rv)) == RJS_ERR)
                goto end;

            r = RJS_OK;
            goto end;
        }

        g->state = RJS_GENERATOR_STATE_SUSPENDED_YIELD;
        rjs_value_copy(rt, rv, ir);
        r = RJS_SUSPEND;
        break;
    case RJS_GENERATOR_REQUEST_THROW:
        if ((r = rjs_get_method(rt, g->iterator.iterator, rjs_pn_throw(rt), fn)) == RJS_ERR)
            goto end;

        if (!rjs_value_is_undefined(rt, fn)) {
            if ((r = rjs_call(rt, fn, g->iterator.iterator, &g->receivedv, 1, ir)) == RJS_ERR)
                goto end;

            if (!rjs_value_is_object(rt, ir)) {
                r = rjs_throw_type_error(rt, _("the iterator result is not an object"));
                goto end;
            }

            if ((r = rjs_iterator_complete(rt, ir)) == RJS_ERR)
                goto end;

            done = r;

            if (done) {
                if ((r = rjs_iterator_value(rt, ir, rv)) == RJS_ERR)
                    goto end;

                r = RJS_OK;
                goto end;
            }

            g->state = RJS_GENERATOR_STATE_SUSPENDED_YIELD;
            rjs_value_copy(rt, rv, ir);
            r = RJS_SUSPEND;
        } else {
            if ((r = rjs_iterator_close(rt, &g->iterator)) == RJS_ERR)
                goto end;

            r = rjs_throw_type_error(rt, _("\"throw\" is not a function"));
        }
        break;
    case RJS_GENERATOR_REQUEST_RETURN:
        if ((r = rjs_get_method(rt, g->iterator.iterator, rjs_pn_return(rt), fn)) == RJS_ERR)
            goto end;

        if (rjs_value_is_undefined(rt, fn)) {
            rjs_value_copy(rt, rv, &g->receivedv);
            r = RJS_RETURN;
            goto end;
        }

        if ((r = rjs_call(rt, fn, g->iterator.iterator, &g->receivedv, 1, ir)) == RJS_ERR)
            goto end;

        if (!rjs_value_is_object(rt, ir)) {
            r = rjs_throw_type_error(rt, _("the iterator result is not an object"));
            goto end;
        }

        if ((r = rjs_iterator_complete(rt, ir)) == RJS_ERR)
            goto end;

        done = r;

        if (done) {
            if ((r = rjs_iterator_value(rt, ir, rv)) == RJS_ERR)
                goto end;

            r = RJS_RETURN;
            goto end;
        }

        g->state = RJS_GENERATOR_STATE_SUSPENDED_YIELD;
        rjs_value_copy(rt, rv, ir);
        r = RJS_SUSPEND;
        break;
    default:
        assert(0);
        r = RJS_ERR;
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Next iterator yield operation.
 * \param rt The current runtime.
 * \param[out] rv The return value.
 * \retval RJS_NEXT The iterator is done.
 * \retval RJS_SUSPEND The iterator is not done.
 * \retval RJS_THROW Throw an error.
 * \retval RJS_RETURN Return from the generator.
 */
RJS_Result
rjs_iterator_yield_next (RJS_Runtime *rt, RJS_Value *rv)
{
    RJS_Generator *g = generator_get(rt);
    RJS_Result     r;

#if ENABLE_ASYNC
    if (generator_is_async(rt, g))
        r = async_generator_iterator_yield_next(rt, rv);
    else
#endif /*ENABLE_ASYNC*/
        r = generator_iterator_yield_next(rt, rv);

    return r;
}

/**
 * Resume the generator.
 * \param rt The current runtime.
 * \param gv The generator value.
 * \param v The parameter value.
 * \param brand The brand.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_generator_resume (RJS_Runtime *rt, RJS_Value *gv, RJS_Value *v, RJS_Value *brand, RJS_Value *rv)
{
    return generator_resume(rt, gv, v, brand, rv);
}

/**
 * Resume the generator with abrupt.
 * \param rt The current runtime.
 * \param gv The generator value.
 * \param type The abrupt type.
 * \param v The parameter value.
 * \param brand The brand.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_generator_resume_abrupt (RJS_Runtime *rt, RJS_Value *gv, RJS_GeneratorAbruptType type,
        RJS_Value *v, RJS_Value *brand, RJS_Value *rv)
{
    return generator_resume_abrupt(rt, gv, type, v, brand, rv);
}
