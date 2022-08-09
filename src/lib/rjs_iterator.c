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

#if ENABLE_ASYNC

/*Scan the reference things in the async from sync iterator object.*/
static void
async_from_sync_iter_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_AsyncFromSyncIterObject *afs = ptr;

    rjs_object_op_gc_scan(rt, &afs->object);
    rjs_gc_scan_value(rt, &afs->sync_object);
    rjs_gc_scan_value(rt, &afs->sync_method);
}

/*Free the async from sync iterator object.*/
static void
async_from_sync_iter_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_AsyncFromSyncIterObject *afs = ptr;

    rjs_iterator_deinit(rt, &afs->sync_iter);
    rjs_object_deinit(rt, &afs->object);

    RJS_DEL(rt, afs);
}

/*Async from sync iterator object operation functions.*/
static const RJS_ObjectOps
async_from_sync_iter_ops = {
    {
        RJS_GC_THING_ASYNC_FROM_SYNC_ITER,
        async_from_sync_iter_op_gc_scan,
        async_from_sync_iter_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS
};

/*Create async iterator from a sync iterator.*/
static RJS_Result
create_async_from_sync_iterator (RJS_Runtime *rt, RJS_Iterator *sync, RJS_Iterator *async)
{
    RJS_Realm                   *realm = rjs_realm_current(rt);
    RJS_AsyncFromSyncIterObject *afs;
    RJS_Result                   r;

    RJS_NEW(rt, afs);

    rjs_value_copy(rt, &afs->sync_object, sync->iterator);
    rjs_value_copy(rt, &afs->sync_method, sync->next_method);

    rjs_iterator_init_vp(rt, &afs->sync_iter, &afs->sync_object, &afs->sync_method);

    rjs_object_init(rt, async->iterator, &afs->object, rjs_o_AsyncFromSyncIteratorPrototype(realm),
            &async_from_sync_iter_ops);

    r = rjs_get_v(rt, async->iterator, rjs_pn_next(rt), async->next_method);
    assert(r == RJS_OK);

    async->type = RJS_ITERATOR_ASYNC;
    async->done = RJS_FALSE;

    return RJS_OK;
}

#endif /*ENABLE_ASYNC*/

/**
 * Get the iterator of the object.
 * \param rt The current runtime.
 * \param obj The object.
 * \param hint Sync or async.
 * \param method The method to get the iterator.
 * \param[out] iter Return the iterator record.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_get_iterator (RJS_Runtime *rt, RJS_Value *obj, RJS_IteratorType hint,
        RJS_Value *method, RJS_Iterator *iter)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *tmp = rjs_value_stack_push(rt);
    RJS_Result r;

    if (!method) {
        method = tmp;

#if ENABLE_ASYNC
        if (hint == RJS_ITERATOR_ASYNC) {
            RJS_Iterator sync_iter;

            if ((r = rjs_get_method(rt, obj, rjs_pn_s_asyncIterator(rt), tmp)) == RJS_ERR)
                goto end;

            if (rjs_value_is_undefined(rt, tmp)) {
                if ((r = rjs_get_method(rt, obj, rjs_pn_s_iterator(rt), tmp)) == RJS_ERR)
                    goto end;

                rjs_iterator_init(rt, &sync_iter);

                if ((r = rjs_get_iterator(rt, obj, RJS_ITERATOR_SYNC, tmp, &sync_iter)) == RJS_ERR)
                    goto end1;

                if ((r = create_async_from_sync_iterator(rt, &sync_iter, iter)) == RJS_ERR)
                    goto end1;
end1:
                rjs_iterator_deinit(rt, &sync_iter);
                goto end;
            }
        } else
#endif /*ENABLE_ASYNC*/
        {
            if ((r = rjs_get_method(rt, obj, rjs_pn_s_iterator(rt), tmp)) == RJS_ERR)
                goto end;
        }
    }

    if ((r = rjs_call(rt, method, obj, NULL, 0, iter->iterator)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_object(rt, iter->iterator)) {
        r = rjs_throw_type_error(rt, _("iterator is not an object"));
        goto end;
    }

    if ((r = rjs_get_v(rt, iter->iterator, rjs_pn_next(rt), iter->next_method)) == RJS_ERR)
        goto end;

    iter->type = hint;
    iter->done = RJS_FALSE;

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Move the iterator to the next position.
 * \param rt The current runtime.
 * \param iter The iterator record.
 * \param v Argument of the next method.
 * \param[out] rv The result object value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_iterator_next (RJS_Runtime *rt, RJS_Iterator *iter, RJS_Value *v, RJS_Value *rv)
{
    RJS_Result r;

    if (v) {
        r = rjs_call(rt, iter->next_method, iter->iterator, v, 1, rv);
    } else {
        r = rjs_call(rt, iter->next_method, iter->iterator, NULL, 0, rv);
    }

    if (r == RJS_ERR)
        return r;

    if (!rjs_value_is_object(rt, rv))
        return rjs_throw_type_error(rt, _("iterator result is not an object"));

    return RJS_OK;
}

/**
 * Check if the iterator is completed.
 * \param rt The current runtime.
 * \param ir The iterator result.
 * \retval RJS_TRUE The iterator is completed.
 * \retval RJS_FALSE The iterator is not completed.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_iterator_complete (RJS_Runtime *rt, RJS_Value *ir)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *tmp = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_get(rt, ir, rjs_pn_done(rt), tmp)) == RJS_ERR)
        goto end;

    r = rjs_to_boolean(rt, tmp);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Get the current value of the iterator.
 * \param rt The current runtime.
 * \param ir The iterator result.
 * \param[out] v Return the current value.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE The iterator is completed.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_iterator_value (RJS_Runtime *rt, RJS_Value *ir, RJS_Value *v)
{
    return rjs_get(rt, ir, rjs_pn_value(rt), v);
}

/**
 * Move the iterator to the next position and check if it is completed.
 * \param rt The current runtime.
 * \param iter The iterator record.
 * \param[out] rv The result value.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE The iterator is completed.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_iterator_step (RJS_Runtime *rt, RJS_Iterator *iter, RJS_Value *rv)
{
    RJS_Result r;

    if ((r = rjs_iterator_next(rt, iter, NULL, rv)) == RJS_ERR)
        return r;

    if ((r = rjs_iterator_complete(rt, rv)) == RJS_ERR)
        return r;

    if (r)
        return RJS_FALSE;

    return RJS_TRUE;
}

/**
 * Close the iterator.
 * \param rt The current runtime.
 * \param iter The iterator record.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_iterator_close (RJS_Runtime *rt, RJS_Iterator *iter)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *ret = rjs_value_stack_push(rt);
    RJS_Value *res = rjs_value_stack_push(rt);
    RJS_Value *err = rjs_value_stack_push(rt);
    RJS_Bool   is_err;
    RJS_Result r;

    is_err = rt->error_flag;
    if (is_err)
        rjs_value_copy(rt, err, &rt->error);

    if ((r = rjs_get_method(rt, iter->iterator, rjs_pn_return(rt), ret)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, ret)) {
        r = RJS_OK;
        goto end;
    }

    if ((r = rjs_call(rt, ret, iter->iterator, NULL, 0, res)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_object(rt, res)) {
        r = rjs_throw_type_error(rt, _("close result is not an object"));
        goto end;
    }

    r = RJS_OK;
end:
    if (is_err)
        rjs_value_copy(rt, &rt->error, err);

    rjs_value_stack_restore(rt, top);
    return r;
}

#if ENABLE_ASYNC

/**
 * Close the async iterator.
 * \param rt The current runtime.
 * \param iter The iterator record.
 * \param op The await operation function.
 * \param ip The integer parameter of the function.
 * \param vp The value parameter of the function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 * \retval RJS_SUSPEND Async wait a promise.
 */
RJS_Result
rjs_async_iterator_close (RJS_Runtime *rt, RJS_Iterator *iter, RJS_AsyncOpFunc op, size_t ip, RJS_Value *vp)
{
    size_t      top = rjs_value_stack_save(rt);
    RJS_Value  *ret = rjs_value_stack_push(rt);
    RJS_Value  *res = rjs_value_stack_push(rt);
    RJS_Value  *err = rjs_value_stack_push(rt);
    RJS_Bool    is_err;
    RJS_Result  r;

    is_err = rt->error_flag;
    if (is_err)
        rjs_value_copy(rt, err, &rt->error);

    if ((r = rjs_get_method(rt, iter->iterator, rjs_pn_return(rt), ret)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, ret)) {
        r = RJS_OK;
        goto end;
    }

    if ((r = rjs_call(rt, ret, iter->iterator, NULL, 0, res)) == RJS_ERR)
        goto end;

    if (op)
        r = rjs_await(rt, res, op, ip, vp);
end:
    if (is_err)
        rjs_value_copy(rt, &rt->error, err);

    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Await the async iterator close operation.
 * \param rt The current runtime.
 * \param type The calling type.
 * \param iv The input value.
 * \param rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_await_async_iterator_close (RJS_Runtime *rt, RJS_ScriptCallType type, RJS_Value *iv, RJS_Value *rv)
{
    if (type == RJS_SCRIPT_CALL_ASYNC_REJECT) {
        rjs_throw(rt, iv);
        return RJS_ERR;
    }

    if (!rjs_value_is_object(rt, iv)) {
        return rjs_throw_type_error(rt, _("close result is not an object"));
    }

    return RJS_OK;
}

#endif /*ENABLE_ASYNC*/

/**
 * Create an iterator result object.
 * \param rt The current runtime.
 * \param v The current value.
 * \param done The iterator is done or not.
 * \param[out] rv Return the iterator result.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_create_iter_result_object (RJS_Runtime *rt, RJS_Value *v, RJS_Bool done, RJS_Value *rv)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *tmp = rjs_value_stack_push(rt);

    rjs_ordinary_object_create(rt, NULL, rv);
    rjs_create_data_property_or_throw(rt, rv, rjs_pn_value(rt), v);

    rjs_value_set_boolean(rt, tmp, done);
    rjs_create_data_property_or_throw(rt, rv, rjs_pn_done(rt), tmp);

    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}
