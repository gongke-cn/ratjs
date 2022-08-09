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

/*Function*/
static RJS_NF(Function_constructor)
{
    return rjs_create_dynamic_function(rt, f, nt, 0, args, argc, rv);
}

static const RJS_BuiltinFuncDesc
function_constructor_desc = {
    "Function",
    1,
    Function_constructor
};

/*Function.prototype.apply*/
static RJS_NF(Function_prototype_apply)
{
    RJS_Value *this_arg  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *arg_array = rjs_argument_get(rt, args, argc, 1);
    size_t     top       = rjs_value_stack_save(rt);
    RJS_Value *item      = rjs_value_stack_push(rt);
    RJS_Value *arg_buf;
    int64_t    arg_len, i;
    RJS_Result r;

    if (!rjs_is_callable(rt, thiz)) {
        r = rjs_throw_type_error(rt, _("this is not a function"));
        goto end;
    }

    if (rjs_value_is_undefined(rt, arg_array) || rjs_value_is_null(rt, arg_array)) {
        arg_buf = NULL;
        arg_len = 0;
    } else {
        if (!rjs_value_is_object(rt, arg_array)) {
            r = rjs_throw_type_error(rt, _("the value is not an object"));
            goto end;
        }

        if ((r = rjs_length_of_array_like(rt, arg_array, &arg_len)) == RJS_ERR)
            goto end;

        if (arg_len) {
            arg_buf = rjs_value_stack_push_n(rt, arg_len);

            for (i = 0; i < arg_len; i ++) {
                RJS_Value *arg = rjs_value_buffer_item(rt, arg_buf, i);

                if ((r = rjs_get_index(rt, arg_array, i, item)) == RJS_ERR)
                    goto end;

                rjs_value_copy(rt, arg, item);
            }
        } else {
            arg_buf = NULL;
            arg_len = 0;
        }
    }

    r = rjs_call(rt, thiz, this_arg, arg_buf, arg_len, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

#if ENABLE_BOUND_FUNC
/*Function.prototype.bind*/
static RJS_NF(Function_prototype_bind)
{
    RJS_Value       *this_arg  = rjs_argument_get(rt, args, argc, 0);
    size_t           top       = rjs_value_stack_save(rt);
    RJS_Value       *lenv      = rjs_value_stack_push(rt);
    RJS_Value       *name      = rjs_value_stack_push(rt);
    RJS_Value       *bind_args;
    size_t           bind_argc;
    RJS_Number       len;
    RJS_Result       r;

    if (!rjs_is_callable(rt, thiz)) {
        r = rjs_throw_type_error(rt, _("this argument is not a function"));
        goto end;
    }

    /*Create the bound function.*/
    if (argc > 1) {
        bind_args = rjs_value_buffer_item(rt, args, 1);
        bind_argc = argc - 1;
    } else {
        bind_args = NULL;
        bind_argc = 0;
    }

    if ((r = rjs_bound_func_object_new(rt, rv, thiz, this_arg, bind_args, bind_argc)) == RJS_ERR)
        goto end;

    /*Set length.*/
    if ((r = rjs_has_own_property(rt, thiz, rjs_pn_length(rt))) == RJS_ERR)
        goto end;

    len = 0;

    if (r) {
        if ((r = rjs_get(rt, thiz, rjs_pn_length(rt), lenv)) == RJS_ERR)
            goto end;

        if (rjs_value_is_number(rt, lenv)) {
            len = rjs_value_get_number(rt, lenv);

            if (isinf(len)) {
                if (signbit(len))
                    len = 0;
            } else {
                rjs_to_integer_or_infinity(rt, lenv, &len);

                len = RJS_MAX(len - bind_argc, 0);
            }
        }
    }

    rjs_set_function_length(rt, rv, len);

    /*Set name.*/
    if ((r = rjs_get(rt, thiz, rjs_pn_name(rt), name)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_string(rt, name))
        rjs_value_copy(rt, name, rjs_s_empty(rt));

    rjs_set_function_name(rt, rv, name, rjs_s_bound(rt));

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}
#endif /*ENABLE_BOUND_FUNC*/

/*Function.prototype.call*/
static RJS_NF(Function_prototype_call)
{
    RJS_Value *this_arg = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *call_args;
    size_t     call_argc;

    if (!rjs_is_callable(rt, thiz))
        return rjs_throw_type_error(rt, _("this is not a function"));

    if (argc > 1) {
        call_args = rjs_value_buffer_item(rt, args, 1);
        call_argc = argc - 1;
    } else {
        call_args = NULL;
        call_argc = 0;
    }

    return rjs_call(rt, thiz, this_arg, call_args, call_argc, rv);
}

/*Function.prototype.toString*/
static RJS_NF(Function_prototype_toString)
{
    RJS_Value      *name  = NULL;
    int             flags = 0;
    RJS_GcThingType gtt;
    int64_t         len;
    int             i;
    RJS_UCharBuffer ucb;
    RJS_Result      r;

    rjs_uchar_buffer_init(rt, &ucb);
    
    if (!rjs_is_callable(rt, thiz)) {
        r = rjs_throw_type_error(rt, _("this is not a function"));
        goto end;
    }

    rjs_uchar_buffer_append_chars(rt, &ucb, "function ", -1);

    gtt = rjs_value_get_gc_thing_type(rt, thiz);
    if (gtt == RJS_GC_THING_SCRIPT_FUNC) {
        RJS_ScriptFuncObject *sfo = (RJS_ScriptFuncObject*)rjs_value_get_object(rt, thiz);

#if ENABLE_FUNC_SOURCE
        if (!rjs_value_is_undefined(rt, &sfo->source)) {
            rjs_value_copy(rt, rv, &sfo->source);
            r = RJS_OK;
            goto end;
        }
#endif /*ENABLE_FUNC_SOURCE*/

        flags = sfo->script_func->flags;

        if (sfo->script_func->name_idx != RJS_INVALID_VALUE_INDEX) {
            name = &sfo->bfo.script->value_table[sfo->script_func->name_idx];
        }
    } else if (gtt == RJS_GC_THING_BUILTIN_FUNC) {
        RJS_BuiltinFuncObject *bfo = (RJS_BuiltinFuncObject*)rjs_value_get_object(rt, thiz);

        flags = bfo->flags;

#if ENABLE_FUNC_SOURCE
        if (!rjs_value_is_undefined(rt, &bfo->init_name))
            name = &bfo->init_name;
#endif /*ENABLE_FUNC_SOURCE*/
    }

    if (flags & RJS_FUNC_FL_GET) {
        rjs_uchar_buffer_append_chars(rt, &ucb, "get ", -1);
    } else if (flags & RJS_FUNC_FL_SET) {
        rjs_uchar_buffer_append_chars(rt, &ucb, "set ", -1);
    }

    if (name)
        rjs_uchar_buffer_append_string(rt, &ucb, name);

    rjs_uchar_buffer_append_uchar(rt, &ucb, '(');

    if ((r = rjs_length_of_array_like(rt, thiz, &len)) == RJS_ERR)
        goto end;

    for (i = 0; i < (int)len; i ++) {
        char buf[32];

        if (i)
            rjs_uchar_buffer_append_uchar(rt, &ucb, ',');

        snprintf(buf, sizeof(buf), "p%d", i);

        rjs_uchar_buffer_append_chars(rt, &ucb, buf, -1);
    }

    rjs_uchar_buffer_append_chars(rt, &ucb, "){[native code]}", -1);

    rjs_string_from_uchars(rt, rv, ucb.items, ucb.item_num);

    r = RJS_OK;
end:
    rjs_uchar_buffer_deinit(rt, &ucb);
    return r;
}

/*Function.prototype[@@hasInstance]*/
static RJS_NF(Function_prototype_hasInstance)
{
    RJS_Value *v = rjs_argument_get(rt, args, argc, 0);
    RJS_Result r;

    if ((r = rjs_ordinary_has_instance(rt, thiz, v)) == RJS_ERR)
        return r;

    rjs_value_set_boolean(rt, rv, r);
    return RJS_OK;
}

static const RJS_BuiltinFuncDesc
function_prototype_function_descs[] = {
    {
        "apply",
        2,
        Function_prototype_apply
    },
#if ENABLE_BOUND_FUNC
    {
        "bind",
        1,
        Function_prototype_bind
    },
#endif /*ENABLE_BOUND_FUNC*/
    {
        "call",
        1,
        Function_prototype_call
    },
    {
        "toString",
        0,
        Function_prototype_toString
    },
    {
        "@@hasInstance",
        1,
        Function_prototype_hasInstance
    },
    {NULL}
};

#if ENABLE_CALLER
/*get Function.prototype.caller*/
static RJS_NF(Function_prototype_caller_get)
{
    RJS_ScriptFuncObject *sfo;
    RJS_Context          *ctxt;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_SCRIPT_FUNC)
        return rjs_throw_type_error(rt, _("ths value is not a function"));

    sfo = (RJS_ScriptFuncObject*)rjs_value_get_object(rt, thiz);
    if (sfo->script_func->flags & RJS_FUNC_FL_STRICT)
        return rjs_throw_type_error(rt, _("\"caller\" cannot be used in strict mode"));

#if ENABLE_GENERATOR
    if (sfo->script_func->flags & RJS_FUNC_FL_GENERATOR)
        return rjs_throw_type_error(rt, _("\"caller\" cannot be used in generator"));
#endif /*ENABLE_GENERATOR*/

    ctxt = rjs_context_running(rt);
    ctxt = ctxt->bot;

    if (!rjs_same_value(rt, thiz, &ctxt->function))
        return rjs_throw_type_error(rt, _("\"caller\" cannot be used here"));

    if (!ctxt->bot) {
        rjs_value_set_undefined(rt, rv);
        return RJS_OK;
    }

    ctxt = ctxt->bot;

    if (rjs_value_get_gc_thing_type(rt, &ctxt->function) == RJS_GC_THING_SCRIPT_FUNC) {
        RJS_ScriptFuncObject *sfo = (RJS_ScriptFuncObject*)rjs_value_get_object(rt, &ctxt->function);

        if (sfo->script_func->flags & RJS_FUNC_FL_STRICT)
            return rjs_throw_type_error(rt, _("cannot access the strict mode \"caller\""));
    }

    rjs_value_copy(rt, rv, &ctxt->function);
    return RJS_OK;
}
#endif /*ENABLE_CALLER*/

static const RJS_BuiltinAccessorDesc
function_prototype_accessor_descs[] = {
    {
        "caller",
#if ENABLE_CALLER
        Function_prototype_caller_get,
#else /*!ENABLE_CALLER*/
        NULL,
#endif /*ENABLE_CALLER*/
        NULL,
#if ENABLE_CALLER
        NULL,
#else
        "ThrowTypeError",
#endif
        "ThrowTypeError"
    },
    {
        "arguments",
        NULL,
        NULL,
        "ThrowTypeError",
        "ThrowTypeError"
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
function_prototype_desc = {
    "Function",
    NULL,
    NULL,
    NULL,
    NULL,
    function_prototype_function_descs,
    function_prototype_accessor_descs,
    NULL,
    "Function_prototype"
};
