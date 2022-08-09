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

/*Error object operation functions.*/
static const RJS_ObjectOps
error_ops = {
    {
        RJS_GC_THING_ERROR,
        rjs_object_op_gc_scan,
        rjs_object_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS
};

/*Create a new error.*/
static RJS_Result
error_new (RJS_Runtime *rt, RJS_Value *err, RJS_Value *constr, const char *msg, va_list ap)
{
    size_t     top  = rjs_value_stack_save(rt);
    RJS_Value *mstr = rjs_value_stack_push(rt);
    char       buf[256];
    RJS_Result r;

    vsnprintf(buf, sizeof(buf), msg, ap);

    if ((r = rjs_string_from_enc_chars(rt, mstr, buf, -1, NULL)) == RJS_ERR)
        goto end;

    r = rjs_call(rt, constr, rjs_v_undefined(rt), mstr, 1, err);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Create a new error and throw it.*/
static RJS_Result
error_throw (RJS_Runtime *rt, RJS_Value *constr, const char *msg, va_list ap)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *err = rjs_value_stack_push(rt);
    RJS_Result r;

    r = error_new(rt, err, constr, msg, ap);
    if (r == RJS_OK)
        rjs_throw(rt, err);
        
    rjs_value_stack_restore(rt, top);
    return RJS_ERR;
}

/**
 * Create a new type error.
 * \param rt The current runtime.
 * \param[out] err Return the type error object.
 * \param msg The error message.
 * \param ... Arguments.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_type_error_new (RJS_Runtime *rt, RJS_Value *err, const char *msg, ...)
{
    RJS_Realm *realm = rjs_realm_current(rt);
    va_list    ap;
    RJS_Result r;

    va_start(ap, msg);
    r = error_new(rt, err, rjs_o_TypeError(realm), msg, ap);
    va_end(ap);

    return r;
}

/**
 * Create a new range error.
 * \param rt The current runtime.
 * \param[out] err Return the range object.
 * \param msg The error message.
 * \param ... Arguments.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_range_error_new (RJS_Runtime *rt, RJS_Value *err, const char *msg, ...)
{
    RJS_Realm *realm = rjs_realm_current(rt);
    va_list    ap;
    RJS_Result r;

    va_start(ap, msg);
    r = error_new(rt, err, rjs_o_RangeError(realm), msg, ap);
    va_end(ap);

    return r;
}

/**
 * Create a new reference error.
 * \param rt The current runtime.
 * \param[out] err Return the reference error object.
 * \param msg The error message.
 * \param ... Arguments.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_reference_error_new (RJS_Runtime *rt, RJS_Value *err, const char *msg, ...)
{
    RJS_Realm *realm = rjs_realm_current(rt);
    va_list    ap;
    RJS_Result r;

    va_start(ap, msg);
    r = error_new(rt, err, rjs_o_ReferenceError(realm), msg, ap);
    va_end(ap);

    return r;
}

/**
 * Create a new syntax error.
 * \param rt The current runtime.
 * \param[out] err Return the syntax error object.
 * \param msg The error message.
 * \param ... Arguments.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_syntax_error_new (RJS_Runtime *rt, RJS_Value *err, const char *msg, ...)
{
    RJS_Realm *realm = rjs_realm_current(rt);
    va_list    ap;
    RJS_Result r;

    va_start(ap, msg);
    r = error_new(rt, err, rjs_o_SyntaxError(realm), msg, ap);
    va_end(ap);

    return r;
}

/**
 * Throw a type error.
 * \param rt The current runtime.
 * \param msg The error message.
 * \param ... Arguments.
 * \return RJS_ERR;
 */
RJS_Result
rjs_throw_type_error (RJS_Runtime *rt, const char *msg, ...)
{
    RJS_Realm *realm = rjs_realm_current(rt);
    va_list    ap;
    RJS_Result r;

    va_start(ap, msg);
    r = error_throw(rt, rjs_o_TypeError(realm), msg, ap);
    va_end(ap);

    return r;
}

/**
 * Throw a range error.
 * \param rt The current runtime.
 * \param msg The error message.
 * \param ... Arguments.
 * \return RJS_ERR;
 */
RJS_Result
rjs_throw_range_error (RJS_Runtime *rt, const char *msg, ...)
{
    RJS_Realm *realm = rjs_realm_current(rt);
    va_list    ap;
    RJS_Result r;

    va_start(ap, msg);
    r = error_throw(rt, rjs_o_RangeError(realm), msg, ap);
    va_end(ap);

    return r;
}

/**
 * Throw a reference error.
 * \param rt The current runtime.
 * \param msg The error message.
 * \param ... Arguments.
 * \return RJS_ERR;
 */
RJS_Result
rjs_throw_reference_error (RJS_Runtime *rt, const char *msg, ...)
{
    RJS_Realm *realm = rjs_realm_current(rt);
    va_list    ap;
    RJS_Result r;

    va_start(ap, msg);
    r = error_throw(rt, rjs_o_ReferenceError(realm), msg, ap);
    va_end(ap);

    return r;
}

/**
 * Throw a syntax error.
 * \param rt The current runtime.
 * \param msg The error message.
 * \param ... Arguments.
 * \return RJS_ERR;
 */
RJS_Result
rjs_throw_syntax_error (RJS_Runtime *rt, const char *msg, ...)
{
    RJS_Realm *realm = rjs_realm_current(rt);
    va_list    ap;
    RJS_Result r;

    va_start(ap, msg);
    r = error_throw(rt, rjs_o_SyntaxError(realm), msg, ap);
    va_end(ap);

    return r;
}

/**
 * Throw an error.
 * \param rt The current runtime.
 * \param err The error value.
 * \retval RJS_ERR.
 */
RJS_Result
rjs_throw (RJS_Runtime *rt, RJS_Value *err)
{
    RJS_Context *ctxt = rjs_context_running(rt);

    if (ctxt) {
        rt->error_context = ctxt;

        if (ctxt->gc_thing.ops->type != RJS_GC_THING_CONTEXT) {
            RJS_ScriptContext *sc = (RJS_ScriptContext*)ctxt;

            rt->error_ip = sc->ip;
        }
    }

    rt->error_flag = RJS_TRUE;

    rjs_value_copy(rt, &rt->error, err);

#if 0
    {
        const char *cstr;

        cstr = rjs_to_desc_chars(rt, err, NULL, NULL);
        RJS_LOGD("throw: %s", cstr);
    }
#endif

    return RJS_ERR;
}

/**
 * Dump the error stack.
 * \param rt The current runtime.
 * \param fp The output file.
 */
void
rjs_dump_error_stack (RJS_Runtime *rt, FILE *fp)
{
    RJS_Context *ctxt  = rt->error_context;
    int          depth = 0;
    size_t       top   = rjs_value_stack_save(rt);
    RJS_Value   *name  = rjs_value_stack_push(rt);
    RJS_Result   r;

    if (!ctxt)
        return;

    fprintf(fp, "stack:\n");

    while (ctxt) {
        fprintf(fp, "  %d: ", depth);

        if (!rjs_value_is_undefined(rt, &ctxt->function)) {
            r = rjs_get_v(rt, &ctxt->function, rjs_pn_name(rt), name);
            if ((r == RJS_OK) && rjs_value_is_string(rt, name)) {
                fprintf(fp, "%s ", rjs_string_to_enc_chars(rt, name, NULL, NULL));
            }
        }

        if (ctxt->gc_thing.ops->type != RJS_GC_THING_CONTEXT) {
            RJS_ScriptContext *sc = (RJS_ScriptContext*)ctxt;

            if (sc->script && sc->script->path)
                fprintf(fp, "%s ", sc->script->path);

            if (sc->script_func) {
                int line;

                line = rjs_function_get_line(rt, sc->script, sc->script_func, sc->ip);

                fprintf(fp, "line %d", line);
            }
        }

        fprintf(fp, "\n");

        depth ++;
        ctxt = ctxt->bot;
    }

    rjs_value_stack_restore(rt, top);
}

/**
 * Catch the current error.
 * \param rt The current runtime.
 * \param[out] err Return the error value.
 * \retval RJS_TRUE Catch the error successfully.
 * \retval RJS_FALSE There is no error.
 */
RJS_Result
rjs_catch (RJS_Runtime *rt, RJS_Value *err)
{
    if (!rt->error_flag)
        return RJS_FALSE;

    if (err)
        rjs_value_copy(rt, err, &rt->error);

    rt->error_flag = RJS_FALSE;
    rjs_value_set_undefined(rt, &rt->error);

    return RJS_TRUE;
}

/*Generic error constructor.*/
static RJS_Result
generic_error_constructor (RJS_Runtime *rt, RJS_Value *f, RJS_Value *args, size_t argc, RJS_Value *nt, int proto_idx, RJS_Value *rv)
{
    RJS_Value  *msg   = rjs_argument_get(rt, args, argc, 0);
    RJS_Value  *opt   = rjs_argument_get(rt, args, argc, 1);
    size_t      top   = rjs_value_stack_save(rt);
    RJS_Value  *str   = rjs_value_stack_push(rt);
    RJS_Value  *cause = rjs_value_stack_push(rt);
    RJS_Object *o;
    RJS_Result  r;

    if (!nt)
        nt = f;

    RJS_NEW(rt, o);

    if ((r = rjs_ordinary_init_from_constructor(rt, o, nt, proto_idx, &error_ops, rv)) == RJS_ERR) {
        RJS_DEL(rt, o);
        goto end;
    }

    if (!rjs_value_is_undefined(rt, msg)) {
        if ((r = rjs_to_string(rt, msg, str)) == RJS_ERR)
            goto end;

        rjs_create_data_property_attrs_or_throw(rt, rv, rjs_pn_message(rt), str,
                RJS_PROP_ATTR_WRITABLE|RJS_PROP_ATTR_CONFIGURABLE);
    }

    if (rjs_value_is_object(rt, opt)) {
        if ((r = rjs_has_property(rt, opt, rjs_pn_cause(rt)->name)) == RJS_ERR)
            goto end;

        if (r) {
            if ((r = rjs_get(rt, opt, rjs_pn_cause(rt), cause)) == RJS_ERR)
                goto end;

            rjs_create_data_property_attrs_or_throw(rt, rv, rjs_pn_cause(rt), cause,
                    RJS_PROP_ATTR_WRITABLE|RJS_PROP_ATTR_CONFIGURABLE);
        }
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Convert the error to string.*/
static RJS_NF(error_toString)
{
    size_t          top  = rjs_value_stack_save(rt);
    RJS_Value      *name = rjs_value_stack_push(rt);
    RJS_Value      *msg  = rjs_value_stack_push(rt);
    RJS_Value      *nstr = rjs_value_stack_push(rt);
    RJS_Value      *mstr = rjs_value_stack_push(rt);
    RJS_UCharBuffer ucb;
    RJS_Result      r;

    if (!rjs_value_is_object(rt, thiz)) {
        r = rjs_throw_type_error(rt, _("this is not an object"));
        goto end;
    }

    if ((r = rjs_get(rt, thiz, rjs_pn_name(rt), name)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, name)) {
        rjs_value_copy(rt, nstr, rjs_s_Error(rt));
    } else {
        if ((r = rjs_to_string(rt, name, nstr)) == RJS_ERR)
            goto end;
    }

    if ((r = rjs_get(rt, thiz, rjs_pn_message(rt), msg)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, msg)) {
        rjs_value_copy(rt, mstr, rjs_s_empty(rt));
    } else {
        if ((r = rjs_to_string(rt, msg, mstr)) == RJS_ERR)
            goto end;
    }

    if (rjs_string_get_length(rt, nstr) == 0) {
        rjs_value_copy(rt, rv, mstr);

        r = RJS_OK;
    } else if (rjs_string_get_length(rt, mstr) == 0) {
        rjs_value_copy(rt, rv, nstr);

        r = RJS_OK;
    } else {
        rjs_uchar_buffer_init(rt, &ucb);

        rjs_uchar_buffer_append_string(rt, &ucb, nstr);
        rjs_uchar_buffer_append_uc(rt, &ucb, ':');
        rjs_uchar_buffer_append_uc(rt, &ucb, ' ');
        rjs_uchar_buffer_append_string(rt, &ucb, mstr);

        r = rjs_string_from_uchars(rt, rv, ucb.items, ucb.item_num);

        rjs_uchar_buffer_deinit(rt, &ucb);
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

#define ERROR_DESCS(name, parent)\
static RJS_NF(name##_constructor)\
{\
    return generic_error_constructor(rt, f, args, argc, nt, RJS_O_##name##_prototype, rv);\
}\
static const RJS_BuiltinFuncDesc \
name##_func_desc = { \
    #name,\
    1,\
    name##_constructor\
};\
static const RJS_BuiltinFieldDesc \
name##_proto_field_descs[] = {\
    {"name", RJS_VALUE_STRING, 0, #name, RJS_PROP_ATTR_WRITABLE|RJS_PROP_ATTR_CONFIGURABLE},\
    {"message", RJS_VALUE_STRING, 0, "", RJS_PROP_ATTR_WRITABLE|RJS_PROP_ATTR_CONFIGURABLE},\
    {NULL}\
};\
static const RJS_BuiltinFuncDesc \
name##_proto_func_descs[] = {\
    {\
        "toString",\
        0,\
        error_toString\
    },\
    {NULL}\
};\
static const RJS_BuiltinObjectDesc \
name##_proto_desc = {\
    #name "_prototype",\
    parent,\
    NULL,\
    NULL,\
    name##_proto_field_descs,\
    name##_proto_func_descs,\
    NULL,\
    NULL,\
    #name "_prototype"\
};

/*AggregateError constructor.*/
static RJS_NF(AggregateError_constructor)
{
    RJS_Value       *errs  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value       *msg   = rjs_argument_get(rt, args, argc, 1);
    RJS_Value       *opt   = rjs_argument_get(rt, args, argc, 2);
    size_t           top   = rjs_value_stack_save(rt);
    RJS_Value       *str   = rjs_value_stack_push(rt);
    RJS_Value       *cause = rjs_value_stack_push(rt);
    RJS_PropertyDesc pd;
    RJS_Result       r;

    rjs_property_desc_init(rt, &pd);

    if (!nt)
        nt = f;

    if ((r = rjs_ordinary_create_from_constructor(rt, nt, RJS_O_AggregateError_prototype, rv)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_undefined(rt, msg)) {
        if ((r = rjs_to_string(rt, msg, str)) == RJS_ERR)
            goto end;

        rjs_create_data_property_attrs_or_throw(rt, rv, rjs_pn_message(rt), str,
                RJS_PROP_ATTR_WRITABLE|RJS_PROP_ATTR_CONFIGURABLE);
    }

    if (rjs_value_is_object(rt, opt)) {
        if ((r = rjs_has_property(rt, opt, rjs_pn_cause(rt)->name)) == RJS_ERR)
            goto end;

        if (r) {
            if ((r = rjs_get(rt, opt, rjs_pn_cause(rt), cause)) == RJS_ERR)
                goto end;

            rjs_create_data_property_attrs_or_throw(rt, rv, rjs_pn_cause(rt), cause,
                    RJS_PROP_ATTR_WRITABLE|RJS_PROP_ATTR_CONFIGURABLE);
        }
    }

    if ((r = rjs_create_array_from_iterable(rt, errs, pd.value)) == RJS_ERR)
        goto end;

    pd.flags = RJS_PROP_FL_DATA|RJS_PROP_FL_WRITABLE|RJS_PROP_FL_CONFIGURABLE;

    rjs_define_property_or_throw(rt, rv, rjs_pn_errors(rt), &pd);

    r = RJS_OK;
end:
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*AggregateError constructor's description.*/
static const RJS_BuiltinFuncDesc
AggregateError_func_desc = {
    "AggregateError",
    2,
    AggregateError_constructor
};

/*AggregateError prototype's fields.*/
static const RJS_BuiltinFieldDesc
AggregateError_proto_field_descs[] = {
    {"name", RJS_VALUE_STRING, 0, "AggregateError", RJS_PROP_ATTR_WRITABLE|RJS_PROP_ATTR_CONFIGURABLE},
    {"message", RJS_VALUE_STRING, 0, "", RJS_PROP_ATTR_WRITABLE|RJS_PROP_ATTR_CONFIGURABLE},
    {NULL}
};

/*AggregateError prototype's methods.*/
static const RJS_BuiltinFuncDesc
AggregateError_proto_func_descs[] = {
    {
        "toString",
        0,
        error_toString
    },
    {NULL}
};

/*AggregateError prototype's description.*/
static const RJS_BuiltinObjectDesc
AggregateError_proto_desc = {
    "AggregateError_prototype",
    "Error_prototype",
    NULL,
    NULL,
    AggregateError_proto_field_descs,
    AggregateError_proto_func_descs,
    NULL,
    NULL,
    "AggregateError_prototype"
};

ERROR_DESCS(Error, NULL)
ERROR_DESCS(EvalError, "Error_prototype")
ERROR_DESCS(RangeError, "Error_prototype")
ERROR_DESCS(ReferenceError, "Error_prototype")
ERROR_DESCS(SyntaxError, "Error_prototype")
ERROR_DESCS(TypeError, "Error_prototype")

#if ENABLE_URI
ERROR_DESCS(URIError, "Error_prototype")
#endif /*ENABLE_URI*/

#define ERROR_OBJECT(name, parent)\
    {\
        #name,\
        parent,\
        &name##_func_desc,\
        &name##_proto_desc,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        #name\
    }

/*Error object descriptions.*/
static const RJS_BuiltinObjectDesc
error_object_descs[] = {
    ERROR_OBJECT(Error, NULL),
    ERROR_OBJECT(EvalError, "Error"),
    ERROR_OBJECT(RangeError, "Error"),
    ERROR_OBJECT(ReferenceError, "Error"),
    ERROR_OBJECT(SyntaxError, "Error"),
    ERROR_OBJECT(TypeError, "Error"),
#if ENABLE_URI
    ERROR_OBJECT(URIError, "Error"),
#endif /*ENABLE_URI*/
    ERROR_OBJECT(AggregateError, "Error"),
    {NULL}
};

/*Error description.*/
static const RJS_BuiltinDesc
error_desc = {
    NULL,
    NULL,
    error_object_descs
};

/**
 * Initialize the error objects in the realm.
 * \param rt The current runtime.
 * \param realm The realm to be initialized.
 */
void
rjs_realm_error_init (RJS_Runtime *rt, RJS_Realm *realm)
{
    rjs_load_builtin_desc(rt, realm, &error_desc);
}
