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

/*Scan the referenced things in the bound function object.*/
static void
bound_func_object_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_BoundFuncObject *bfo = (RJS_BoundFuncObject*)ptr;

    rjs_object_op_gc_scan(rt, ptr);

    rjs_gc_scan_value(rt, &bfo->target_func);
    rjs_gc_scan_value(rt, &bfo->thiz);
    rjs_gc_scan_value_buffer(rt, bfo->args, bfo->argc);
}

/*Free the bound function object.*/
static void
bound_func_object_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_BoundFuncObject *bfo = (RJS_BoundFuncObject*)ptr;

    rjs_object_deinit(rt, &bfo->object);

    if (bfo->args)
        RJS_DEL_N(rt, bfo->args, bfo->argc);

    RJS_DEL(rt, bfo);
}

/*Call the bound function object.*/
static RJS_Result
bound_func_object_call (RJS_Runtime *rt, RJS_Value *o, RJS_Value *thiz, RJS_Value *args, size_t argc, RJS_Value *rv)
{
    RJS_BoundFuncObject *bfo = (RJS_BoundFuncObject*)rjs_value_get_object(rt, o);
    size_t               top = rjs_value_stack_save(rt);
    RJS_Value           *bargs;
    size_t               bargc;
    RJS_Result           r;

    if (!argc) {
        bargs = bfo->args;
        bargc = bfo->argc;
    } else if (!bfo->args) {
        bargs = args;
        bargc = argc;
    } else {
        bargc = argc + bfo->argc;
        bargs = rjs_value_stack_push_n(rt, bargc);

        rjs_value_buffer_copy(rt, bargs, bfo->args, bfo->argc);
        rjs_value_buffer_copy(rt, rjs_value_buffer_item(rt, bargs, bfo->argc), args, argc);
    }

    r = rjs_call(rt, &bfo->target_func, &bfo->thiz, bargs, bargc, rv);

    rjs_value_stack_restore(rt, top);

    return r;
}

/*Construct a new object.*/
static RJS_Result
bound_func_object_construct (RJS_Runtime *rt, RJS_Value *o, RJS_Value *args, size_t argc, RJS_Value *target, RJS_Value *rv)
{
    RJS_BoundFuncObject *bfo = (RJS_BoundFuncObject*)rjs_value_get_object(rt, o);
    size_t               top = rjs_value_stack_save(rt);
    RJS_Value           *bargs;
    size_t               bargc;
    RJS_Result           r;

    if (!argc) {
        bargs = bfo->args;
        bargc = bfo->argc;
    } else if (!bfo->args) {
        bargs = args;
        bargc = argc;
    } else {
        bargc = argc + bfo->argc;
        bargs = rjs_value_stack_push_n(rt, bargc);

        rjs_value_buffer_copy(rt, bargs, bfo->args, bfo->argc);
        rjs_value_buffer_copy(rt, rjs_value_buffer_item(rt, bargs, bfo->argc), args, argc);
    }

    if (rjs_same_value(rt, o, target))
        target = &bfo->target_func;

    r = rjs_object_construct(rt, &bfo->target_func, bargs, bargc, target, rv);

    rjs_value_stack_restore(rt, top);

    return r;
}

/*Bound function operation functions.*/
static const RJS_ObjectOps
bound_func_object_ops = {
    {
        RJS_GC_THING_BOUND_FUNC,
        bound_func_object_op_gc_scan,
        bound_func_object_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS,
    bound_func_object_call
};

/*Bound constructor operation functions.*/
static const RJS_ObjectOps
bound_constructor_object_ops = {
    {
        RJS_GC_THING_BOUND_FUNC,
        bound_func_object_op_gc_scan,
        bound_func_object_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS,
    bound_func_object_call,
    bound_func_object_construct
};

/**
 * Create a new bound function object.
 * \param rt The current runtime.
 * \param[out] v Return the new function.
 * \param func The bound target function.
 * \param thiz The bound this argument.
 * \param args The bound arguments.
 * \param argc Arguments' count.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_bound_func_object_new (RJS_Runtime *rt, RJS_Value *v, RJS_Value *func,
        RJS_Value *thiz, RJS_Value *args, size_t argc)
{
    RJS_BoundFuncObject *bfo;
    RJS_Result           r;
    const RJS_ObjectOps *ops;
    size_t               top   = rjs_value_stack_save(rt);
    RJS_Value           *proto = rjs_value_stack_push(rt);

    if ((r = rjs_object_get_prototype_of(rt, func, proto)) == RJS_ERR)
        goto end;

    RJS_NEW(rt, bfo);

    rjs_value_copy(rt, &bfo->object.prototype, proto);
    rjs_value_copy(rt, &bfo->target_func, func);
    rjs_value_copy(rt, &bfo->thiz, thiz);

    bfo->argc = argc;

    if (argc) {
        RJS_NEW_N(rt, bfo->args, bfo->argc);

        rjs_value_buffer_copy(rt, bfo->args, args, argc);
    } else {
        bfo->args = NULL;
    }

    if (rjs_is_constructor(rt, func))
        ops = &bound_constructor_object_ops;
    else
        ops = &bound_func_object_ops;

    rjs_object_init(rt, v, &bfo->object, proto, ops);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}
