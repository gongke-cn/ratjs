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

/*AsyncFunction.*/
static RJS_NF(AsyncFunction_constructor)
{
    return rjs_create_dynamic_function(rt, f, nt, RJS_FUNC_FL_ASYNC, args, argc, rv);
}

static const RJS_BuiltinFuncDesc
async_function_constructor_desc = {
    "AsyncFunction",
    1,
    AsyncFunction_constructor
};

static const RJS_BuiltinFieldDesc
async_function_prototype_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "AsyncFunction",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
async_function_prototype_desc = {
    "AsyncFunction",
    "Function_prototype",
    NULL,
    NULL,
    async_function_prototype_field_descs,
    NULL,
    NULL,
    NULL,
    "AsyncFunction_prototype"
};

static const RJS_BuiltinFuncDesc
async_iterator_prototype_function_descs[] = {
    {
        "@@asyncIterator",
        0,
        rjs_return_this
    },
    {NULL}
};

/**Async from sync iterator function object.*/
typedef struct {
    RJS_BuiltinFuncObject bfo;  /**< Basic built-in function object data.*/
    RJS_Bool              done; /**< Iterator's done flag.*/
} RJS_AsyncFromSyncIterFunc;

/*Scan the reference things in the async from sync iterator function.*/
static void
async_from_sync_iter_func_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_AsyncFromSyncIterFunc *afsf = ptr;

    rjs_builtin_func_object_op_gc_scan(rt, &afsf->bfo);
}

static void
async_from_sync_iter_func_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_AsyncFromSyncIterFunc *afsf = ptr;

    rjs_builtin_func_object_deinit(rt, &afsf->bfo);

    RJS_DEL(rt, afsf);
}

/*Async from sync iterator function object's operations.*/
static const RJS_ObjectOps
async_from_sync_iter_func_ops = {
    {
        RJS_GC_THING_BUILTIN_FUNC,
        async_from_sync_iter_func_op_gc_scan,
        async_from_sync_iter_func_op_gc_free
    },
    RJS_BUILTIN_CONSTRUCTOR_OBJECT_OPS
};

/*Async from sync iterator continuation function.*/
static RJS_NF(async_from_sync_iter_cont_func)
{
    RJS_Value                 *v = rjs_argument_get(rt, args, argc, 0);
    RJS_AsyncFromSyncIterFunc *afsf;

    afsf = (RJS_AsyncFromSyncIterFunc*)rjs_value_get_object(rt, f);

    return rjs_create_iter_result_object(rt, v, afsf->done, rv);
}

/*Continuation of async from sync iterator.*/
static RJS_Result
async_from_sync_iterator_continuation (RJS_Runtime *rt, RJS_Value *result,
        RJS_PromiseCapability *pc, RJS_Value *rv)
{
    RJS_Realm                 *realm   = rjs_realm_current(rt);
    size_t                     top     = rjs_value_stack_save(rt);
    RJS_Value                 *value   = rjs_value_stack_push(rt);
    RJS_Value                 *wrapper = rjs_value_stack_push(rt);
    RJS_Value                 *fulfill = rjs_value_stack_push(rt);
    RJS_Result                 r;
    RJS_Bool                   done;
    RJS_AsyncFromSyncIterFunc *afsf;

    r = rjs_iterator_complete(rt, result);
    if (if_abrupt_reject_promise(rt, r, pc, rv) == RJS_ERR)
        goto end;

    done = r;

    r = rjs_iterator_value(rt, result, value);
    if (if_abrupt_reject_promise(rt, r, pc, rv) == RJS_ERR)
        goto end;

    r = rjs_promise_resolve(rt, rjs_o_Promise(realm), value, wrapper);
    if (if_abrupt_reject_promise(rt, r, pc, rv) == RJS_ERR)
        goto end;

    RJS_NEW(rt, afsf);

    afsf->done = done;

    rjs_init_builtin_function(rt, &afsf->bfo, async_from_sync_iter_cont_func, 0,
            &async_from_sync_iter_func_ops, 1, rjs_s_empty(rt), realm,
            NULL, NULL, NULL, fulfill);

    rjs_perform_proimise_then(rt, wrapper, fulfill, rjs_v_undefined(rt), pc, NULL);

    rjs_value_copy(rt, rv, pc->promise);

end:
    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}

/*%AsyncFromSyncIteratorPrototype%.next*/
static RJS_NF(AsyncFromSyncIteratorPrototype_next)
{
    RJS_Value                   *v     = rjs_argument_get(rt, args, argc, 0);
    RJS_Realm                   *realm = rjs_realm_current(rt);
    size_t                       top   = rjs_value_stack_save(rt);
    RJS_Value                   *res   = rjs_value_stack_push(rt);
    RJS_PromiseCapability        pc;
    RJS_AsyncFromSyncIterObject *afs;
    RJS_Result                   r;

    assert(rjs_value_get_gc_thing_type(rt, thiz) == RJS_GC_THING_ASYNC_FROM_SYNC_ITER);

    rjs_promise_capability_init(rt, &pc);

    afs = (RJS_AsyncFromSyncIterObject*)rjs_value_get_object(rt, thiz);

    rjs_new_promise_capability(rt, rjs_o_Promise(realm), &pc);

    if (argc > 0)
        r = rjs_iterator_next(rt, &afs->sync_iter, v, res);
    else
        r = rjs_iterator_next(rt, &afs->sync_iter, NULL, res);
        
    if (if_abrupt_reject_promise(rt, r, &pc, rv) == RJS_ERR) {
        r = RJS_OK;
        goto end;
    }

    r = async_from_sync_iterator_continuation(rt, res, &pc, rv);
end:
    rjs_promise_capability_deinit(rt, &pc);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*%AsyncFromSyncIteratorPrototype%.return*/
static RJS_NF(AsyncFromSyncIteratorPrototype_return)
{
    RJS_Value                   *v     = rjs_argument_get(rt, args, argc, 0);
    RJS_Realm                   *realm = rjs_realm_current(rt);
    size_t                       top   = rjs_value_stack_save(rt);
    RJS_Value                   *fn    = rjs_value_stack_push(rt);
    RJS_Value                   *res   = rjs_value_stack_push(rt);
    RJS_PromiseCapability        pc;
    RJS_AsyncFromSyncIterObject *afs;
    RJS_Result                   r;

    assert(rjs_value_get_gc_thing_type(rt, thiz) == RJS_GC_THING_ASYNC_FROM_SYNC_ITER);

    rjs_promise_capability_init(rt, &pc);

    afs = (RJS_AsyncFromSyncIterObject*)rjs_value_get_object(rt, thiz);

    rjs_new_promise_capability(rt, rjs_o_Promise(realm), &pc);

    r = rjs_get_method(rt, &afs->sync_object, rjs_pn_return(rt), fn);
    if (if_abrupt_reject_promise(rt, r, &pc, rv) == RJS_ERR) {
        r = RJS_OK;
        goto end;
    }

    if (rjs_value_is_undefined(rt, fn)) {
        rjs_create_iter_result_object(rt, v, RJS_TRUE, res);
        rjs_call(rt, pc.resolve, rjs_v_undefined(rt), res, 1, NULL);
        rjs_value_copy(rt, rv, pc.promise);
        r = RJS_OK;
        goto end;
    }

    if (argc > 0)
        r = rjs_call(rt, fn, &afs->sync_object, v, 1, res);
    else
        r = rjs_call(rt, fn, &afs->sync_object, NULL, 0, res);

    if (if_abrupt_reject_promise(rt, r, &pc, rv) == RJS_ERR) {
        r = RJS_OK;
        goto end;
    }

    if (!rjs_value_is_object(rt, res)) {
        rjs_type_error_new(rt, res, _("the value is not an object"));
        rjs_call(rt, pc.reject, rjs_v_undefined(rt), res, 1, NULL);
        rjs_value_copy(rt, rv, pc.promise);
        r = RJS_OK;
        goto end;
    }

    r = async_from_sync_iterator_continuation(rt, res, &pc, rv);
end:
    rjs_promise_capability_deinit(rt, &pc);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*%AsyncFromSyncIteratorPrototype%.throw*/
static RJS_NF(AsyncFromSyncIteratorPrototype_throw)
{
    RJS_Value                   *v     = rjs_argument_get(rt, args, argc, 0);
    RJS_Realm                   *realm = rjs_realm_current(rt);
    size_t                       top   = rjs_value_stack_save(rt);
    RJS_Value                   *fn    = rjs_value_stack_push(rt);
    RJS_Value                   *res   = rjs_value_stack_push(rt);
    RJS_PromiseCapability        pc;
    RJS_AsyncFromSyncIterObject *afs;
    RJS_Result                   r;

    assert(rjs_value_get_gc_thing_type(rt, thiz) == RJS_GC_THING_ASYNC_FROM_SYNC_ITER);

    rjs_promise_capability_init(rt, &pc);

    afs = (RJS_AsyncFromSyncIterObject*)rjs_value_get_object(rt, thiz);

    rjs_new_promise_capability(rt, rjs_o_Promise(realm), &pc);

    r = rjs_get_method(rt, &afs->sync_object, rjs_pn_throw(rt), fn);
    if (if_abrupt_reject_promise(rt, r, &pc, rv) == RJS_ERR) {
        r = RJS_OK;
        goto end;
    }

    if (rjs_value_is_undefined(rt, fn)) {
        rjs_call(rt, pc.reject, rjs_v_undefined(rt), v, 1, NULL);
        rjs_value_copy(rt, rv, pc.promise);
        r = RJS_OK;
        goto end;
    }

    r = rjs_call(rt, fn, &afs->sync_object, v, 1, res);
    if (if_abrupt_reject_promise(rt, r, &pc, rv) == RJS_ERR) {
        r = RJS_OK;
        goto end;
    }

    if (!rjs_value_is_object(rt, res)) {
        rjs_type_error_new(rt, res, _("the value is not an object"));
        rjs_call(rt, pc.reject, rjs_v_undefined(rt), res, 1, NULL);
        rjs_value_copy(rt, rv, pc.promise);
        r = RJS_OK;
        goto end;
    }

    r = async_from_sync_iterator_continuation(rt, res, &pc, rv);
end:
    rjs_promise_capability_deinit(rt, &pc);
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
async_from_sync_iterator_prototype_function_descs[] = {
    {
        "next",
        0,
        AsyncFromSyncIteratorPrototype_next
    },
    {
        "return",
        0,
        AsyncFromSyncIteratorPrototype_return
    },
    {
        "throw",
        0,
        AsyncFromSyncIteratorPrototype_throw
    },
    {NULL}
};
