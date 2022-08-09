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

/*Concatenate function.*/
static RJS_NF(internal_concat)
{
    RJS_Value       *templ = rjs_argument_get(rt, args, argc, 0);
    size_t           top   = rjs_value_stack_save(rt);
    RJS_Value       *str   = rjs_value_stack_push(rt);
    size_t           i;
    RJS_UCharBuffer  ucb;
    RJS_Result       r;

    rjs_uchar_buffer_init(rt, &ucb);

    if ((r = rjs_get_index_v(rt, templ, 0, str)) == RJS_ERR)
        goto end;

    rjs_uchar_buffer_append_string(rt, &ucb, str);

    for (i = 1; i < argc; i ++) {
        RJS_Value *arg;

        arg = rjs_argument_get(rt, args, argc, i);

        if ((r = rjs_to_string(rt, arg, str)) == RJS_ERR)
            goto end;

        rjs_uchar_buffer_append_string(rt, &ucb, str);

        if ((r = rjs_get_index_v(rt, templ, i, str)) == RJS_ERR)
            goto end;

        rjs_uchar_buffer_append_string(rt, &ucb, str);
    }

    rjs_string_from_uchars(rt, rv, ucb.items, ucb.item_num);

    r = RJS_OK;
end:
    rjs_uchar_buffer_deinit(rt, &ucb);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Throw type error function.*/
static RJS_NF(internal_throw_type_error)
{
    return rjs_throw_type_error(rt, _("type error"));
}

#if ENABLE_EVAL
/*Eval.*/
static RJS_NF(global_eval)
{
    RJS_Value *arg    = rjs_argument_get(rt, args, argc, 0);
    size_t     top    = rjs_value_stack_save(rt);
    RJS_Value *script = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_eval_from_string(rt, script, arg, NULL, RJS_FALSE, RJS_FALSE)) == RJS_OK) {
        r = rjs_eval_evaluation(rt, script, RJS_FALSE, rv);
    } else if (r == RJS_FALSE) {
        r = RJS_OK;
        rjs_value_copy(rt, rv, arg);
    }

    rjs_value_stack_restore(rt, top);
    return r;
}
#endif /*ENABLE_EVAL*/

/*isFinite*/
static RJS_NF(global_isFinite)
{
    RJS_Value  *arg = rjs_argument_get(rt, args, argc, 0);
    RJS_Number  n;
    RJS_Result  r;
    RJS_Bool    b;

    if ((r = rjs_to_number(rt, arg, &n)) == RJS_ERR)
        return r;

    b = !(isinf(n) || isnan(n));

    rjs_value_set_boolean(rt, rv, b);
    return RJS_OK;
}

/*isNaN*/
static RJS_NF(global_isNaN)
{
    RJS_Value  *arg = rjs_argument_get(rt, args, argc, 0);
    RJS_Number  n;
    RJS_Result  r;
    RJS_Bool    b;

    if ((r = rjs_to_number(rt, arg, &n)) == RJS_ERR)
        return r;

    b = isnan(n);

    rjs_value_set_boolean(rt, rv, b);
    return RJS_OK;
}

/*parseFloat*/
static RJS_NF(global_parseFloat)
{
    RJS_Value     *arg  = rjs_argument_get(rt, args, argc, 0);
    size_t         top  = rjs_value_stack_save(rt);
    RJS_Value     *str  = rjs_value_stack_push(rt);
    RJS_Value     *trim = rjs_value_stack_push(rt);
    RJS_CharBuffer cb;
    const char    *cstr;
    char          *end;
    size_t         len;
    double         d;
    RJS_Result     r;

    rjs_char_buffer_init(rt, &cb);

    if ((r = rjs_to_string(rt, arg, str)) == RJS_ERR)
        goto end;

    if ((r = rjs_string_trim(rt, str, RJS_STRING_TRIM_START|RJS_STRING_TRIM_END, trim)) == RJS_ERR)
        goto end;

    cstr = rjs_string_to_enc_chars(rt, trim, &cb, "UTF-8");
    len  = strlen(cstr);

    if (len == 0) {
        d = NAN;
    } else {
        d = rjs_strtod(cstr, &end);
        if (end == cstr) {
            d = NAN;
        } else if ((errno == ERANGE) && ((d == HUGE_VAL) || (d == -HUGE_VAL))) {
            d = NAN;
        }
    }

    rjs_value_set_number(rt, rv, d);

    r = RJS_OK;
end:
    rjs_char_buffer_deinit(rt, &cb);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*parseInt*/
static RJS_NF(global_parseInt)
{
    RJS_Value     *arg    = rjs_argument_get(rt, args, argc, 0);
    RJS_Value     *radixv = rjs_argument_get(rt, args, argc, 1);
    size_t         top    = rjs_value_stack_save(rt);
    RJS_Value     *str    = rjs_value_stack_push(rt);
    RJS_Value     *trim   = rjs_value_stack_push(rt);
    RJS_CharBuffer cb;
    const char    *cstr;
    char          *end;
    size_t         len;
    int32_t        base;
    double         d;
    RJS_Result     r;

    rjs_char_buffer_init(rt, &cb);

    if ((r = rjs_to_string(rt, arg, str)) == RJS_ERR)
        goto end;

    if ((r = rjs_string_trim(rt, str, RJS_STRING_TRIM_START|RJS_STRING_TRIM_END, trim)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_int32(rt, radixv, &base)) == RJS_ERR)
        goto end;

    r = RJS_OK;

    if (base != 0) {
        if ((base < 2) || (base > 36)) {
            rjs_value_set_number(rt, rv, NAN);
            goto end;
        }
    }

    cstr = rjs_string_to_enc_chars(rt, trim, &cb, "UTF-8");
    if (!cstr) {
        rjs_value_set_number(rt, rv, NAN);
        goto end;
    }

    len = strlen(cstr);
    if (len == 0) {
        rjs_value_set_number(rt, rv, NAN);
        goto end;
    }

    d = rjs_strtoi(cstr, &end, base);

    if (end == cstr) {
        rjs_value_set_number(rt, rv, NAN);
        goto end;
    }

    if ((d == INFINITY) || (d == -INFINITY))
        d = NAN;

    rjs_value_set_number(rt, rv, d);
    r = RJS_OK;
end:
    rjs_char_buffer_deinit(rt, &cb);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Global fields.*/
static const RJS_BuiltinFieldDesc
global_field_descs[] = {
    {
        "Infinity",
        RJS_VALUE_NUMBER,
        INFINITY
    },
    {
        "NaN",
        RJS_VALUE_NUMBER,
        NAN
    },
    {
        "undefined",
        RJS_VALUE_UNDEFINED
    },
    {NULL}
};

#if ENABLE_URI
#include "rjs_uri_functions_inc.c"
#endif

/*Global functions description.*/
static const RJS_BuiltinFuncDesc
global_function_descs[] = {
    {
        "%Concat",
        1,
        internal_concat,
        "Concat"
    },
    {
        "%ThrowTypeError",
        0,
        internal_throw_type_error,
        "ThrowTypeError"
    },
    {
        "isFinite",
        1,
        global_isFinite
    },
    {
        "isNaN",
        1,
        global_isNaN
    },
    {
        "parseFloat",
        1,
        global_parseFloat,
        "parseFloat"
    },
    {
        "parseInt",
        2,
        global_parseInt,
        "parseInt"
    },
#if ENABLE_EVAL
    {
        "eval",
        1,
        global_eval,
        "eval"
    },
#endif /*ENABLE_EVAL*/
#if ENABLE_URI
    {
        "decodeURI",
        1,
        global_decodeURI
    },
    {
        "decodeURIComponent",
        1,
        global_decodeURIComponent
    },
    {
        "encodeURI",
        1,
        global_encodeURI
    },
    {
        "encodeURIComponent",
        1,
        global_encodeURIComponent
    },
#endif /*ENABLE_URI*/
    {NULL}
};

#include "rjs_object_object_inc.c"
#include "rjs_function_object_inc.c"
#include "rjs_boolean_object_inc.c"
#include "rjs_symbol_object_inc.c"
#include "rjs_number_object_inc.c"
#include "rjs_string_object_inc.c"
#include "rjs_regexp_object_inc.c"
#include "rjs_array_object_inc.c"
#include "rjs_iterator_prototype_inc.c"
#include "rjs_promise_object_inc.c"

#if ENABLE_MATH
#include "rjs_math_object_inc.c"
#endif

#if ENABLE_DATE
#include "rjs_date_object_inc.c"
#endif

#if ENABLE_BIG_INT
#include "rjs_big_int_object_inc.c"
#endif

#if ENABLE_ARRAY_BUFFER
#include "rjs_array_buffer_object_inc.c"
#endif

#if ENABLE_SHARED_ARRAY_BUFFER
#include "rjs_shared_array_buffer_object_inc.c"
#endif

#if ENABLE_INT_INDEXED_OBJECT
#include "rjs_typed_array_object_inc.c"
#endif

#if ENABLE_DATA_VIEW
#include "rjs_data_view_object_inc.c"
#endif

#if ENABLE_ATOMICS
#include "rjs_atomics_object_inc.c"
#endif

#if ENABLE_MAP || ENABLE_SET || ENABLE_WEAK_MAP || ENABLE_WEAK_SET
#include "rjs_hash_object_inc.c"
#endif

#if ENABLE_MAP
#include "rjs_map_object_inc.c"
#endif

#if ENABLE_SET
#include "rjs_set_object_inc.c"
#endif

#if ENABLE_WEAK_MAP
#include "rjs_weak_map_object_inc.c"
#endif

#if ENABLE_WEAK_SET
#include "rjs_weak_set_object_inc.c"
#endif

#if ENABLE_FINALIZATION_REGISTRY
#include "rjs_finalization_registry_object_inc.c"
#endif

#if ENABLE_WEAK_REF
#include "rjs_weak_ref_object_inc.c"
#endif

#if ENABLE_JSON
#include "rjs_json_object_inc.c"
#endif

#if ENABLE_GENERATOR
#include "rjs_generator_object_inc.c"
#endif

#if ENABLE_ASYNC
#include "rjs_async_function_object_inc.c"
#endif

#if ENABLE_GENERATOR && ENABLE_ASYNC
#include "rjs_async_generator_object_inc.c"
#endif

#if ENABLE_REFLECT
#include "rjs_reflect_object_inc.c"
#endif

#if ENABLE_PROXY
#include "rjs_proxy_object_inc.c"
#endif

/*Global object descriptiosn.*/
static const RJS_BuiltinObjectDesc
global_object_descs[] = {
    {
        "Object",
        NULL,
        &object_constructor_desc,
        &object_prototype_desc,
        NULL,
        object_function_descs,
        NULL,
        NULL
    },
    {
        "Function",
        NULL,
        &function_constructor_desc,
        &function_prototype_desc,
        NULL,
        NULL,
        NULL,
        NULL,
        "Function"
    },
    {
        "Boolean",
        NULL,
        &boolean_constructor_desc,
        &boolean_prototype_desc,
        NULL,
        NULL,
        NULL,
        NULL,
    },
    {
        "Symbol",
        NULL,
        &symbol_constructor_desc,
        &symbol_prototype_desc,
        symbol_field_descs,
        symbol_function_descs,
        NULL,
        NULL,
    },
    {
        "Number",
        NULL,
        &number_constructor_desc,
        &number_prototype_desc,
        number_field_descs,
        number_function_descs,
        NULL,
        NULL,
    },
    {
        "String",
        NULL,
        &string_constructor_desc,
        &string_prototype_desc,
        NULL,
        string_function_descs,
        NULL,
        NULL,
    },
    {
        "RegExp",
        NULL,
        &regexp_constructor_desc,
        &regexp_prototype_desc,
        NULL,
        NULL,
        regexp_accessor_descs,
        NULL,
        "RegExp"
    },
    {
        "Array",
        NULL,
        &array_constructor_desc,
        &array_prototype_desc,
        NULL,
        array_function_descs,
        array_accessor_descs,
        NULL,
        "Array"
    },
    {
        "%IteratorPrototype%",
        NULL,
        NULL,
        NULL,
        NULL,
        iterator_prototype_function_descs,
        NULL,
        NULL,
        "IteratorPrototype"
    },
    {
        "%StringIteratorPrototype%",
        "IteratorPrototype",
        NULL,
        NULL,
        string_iterator_prototype_field_descs,
        string_iterator_prototype_function_descs,
        NULL,
        NULL,
        "StringIteratorPrototype"
    },
    {
        "%RegExpStringIteratorPrototype%",
        "IteratorPrototype",
        NULL,
        NULL,
        regexp_str_iter_prototype_field_descs,
        regexp_str_iter_prototype_function_descs,
        NULL,
        NULL,
        "RegExpStringIteratorPrototype"
    },
    {
        "%ArrayIteratorPrototype%",
        "IteratorPrototype",
        NULL,
        NULL,
        array_iterator_prototype_field_descs,
        array_iterator_prototype_function_descs,
        NULL,
        NULL,
        "ArrayIteratorPrototype"
    },
    {
        "Promise",
        NULL,
        &promise_constructor_desc,
        &promise_prototype_desc,
        NULL,
        promise_function_descs,
        promise_accessor_descs,
        NULL,
        "Promise"
    },
#if ENABLE_MATH
    {
        "Math",
        NULL,
        NULL,
        NULL,
        math_field_descs,
        math_function_descs
    },
#endif /*ENABLE_MATH*/
#if ENABLE_DATE
    {
        "Date",
        NULL,
        &date_constructor_desc,
        &date_prototype_desc,
        NULL,
        date_function_descs,
        NULL,
        NULL,
        "Date"
    },
#endif /*ENABLE_DATE*/
#if ENABLE_BIG_INT
    {
        "BigInt",
        NULL,
        &big_int_constructor_desc,
        &big_int_prototype_desc,
        NULL,
        big_int_function_descs,
        NULL,
        NULL,
    },
#endif /*ENABLE_BIG_INT*/
#if ENABLE_ARRAY_BUFFER
    {
        "ArrayBuffer",
        NULL,
        &array_buffer_constructor_desc,
        &array_buffer_prototype_desc,
        NULL,
        array_buffer_function_descs,
        array_buffer_accessor_descs,
        NULL,
        "ArrayBuffer"
    },
#endif /*ENABLE_ARRAY_BUFFER*/
#if ENABLE_SHARED_ARRAY_BUFFER
    {
        "SharedArrayBuffer",
        NULL,
        &shared_array_buffer_constructor_desc,
        &shared_array_buffer_prototype_desc,
        NULL,
        NULL,
        shared_array_buffer_accessor_descs,
        NULL,
        "SharedArrayBuffer"
    },
#endif /*ENABLE_SHARED_ARRAY_BUFFER*/
#if ENABLE_INT_INDEXED_OBJECT
    {
        "%TypedArray",
        NULL,
        &typed_array_constructor_desc,
        &typed_array_prototype_desc,
        NULL,
        typed_array_function_descs,
        typed_array_accessor_descs,
        NULL,
        "TypedArray"
    },
#define TYPED_ARRAY_OBJECT_DECL(n)\
    {\
        #n,\
        "TypedArray",\
        &n##_constructor_desc,\
        &n##_prototype_desc,\
        n##_field_descs,\
        NULL,\
        NULL,\
        NULL,\
        #n\
    },
    TYPED_ARRAY_OBJECT_DECL(Int8Array)
    TYPED_ARRAY_OBJECT_DECL(Uint8Array)
    TYPED_ARRAY_OBJECT_DECL(Uint8ClampedArray)
    TYPED_ARRAY_OBJECT_DECL(Int16Array)
    TYPED_ARRAY_OBJECT_DECL(Uint16Array)
    TYPED_ARRAY_OBJECT_DECL(Int32Array)
    TYPED_ARRAY_OBJECT_DECL(Uint32Array)
    TYPED_ARRAY_OBJECT_DECL(Float32Array)
    TYPED_ARRAY_OBJECT_DECL(Float64Array)
#if ENABLE_BIG_INT
    TYPED_ARRAY_OBJECT_DECL(BigInt64Array)
    TYPED_ARRAY_OBJECT_DECL(BigUint64Array)
#endif /*ENABLE_BIG_INT*/
#endif /*ENABLE_INT_INDEXED_OBJECT*/
#if ENABLE_DATA_VIEW
    {
        "DataView",
        NULL,
        &data_view_constructor_desc,
        &data_view_prototype_desc,
        NULL,
        NULL,
        NULL,
        NULL,
    },
#endif /*ENABLE_DATA_VIEW*/
#if ENABLE_ATOMICS
    {
        "Atomics",
        NULL,
        NULL,
        NULL,
        atomics_field_descs,
        atomics_function_descs,
        NULL,
        NULL,
    },
#endif /*ENABLE_ATOMICS*/
#if ENABLE_MAP
    {
        "Map",
        NULL,
        &map_constructor_desc,
        &map_prototype_desc,
        NULL,
        NULL,
        map_accessor_descs,
        NULL,
    },
    {
        "%MapIteratorPrototype%",
        "IteratorPrototype",
        NULL,
        NULL,
        map_iterator_prototype_field_descs,
        map_iterator_prototype_function_descs,
        NULL,
        NULL,
        "MapIteratorPrototype"
    },
#endif /*ENABLE_MAP*/
#if ENABLE_SET
    {
        "Set",
        NULL,
        &set_constructor_desc,
        &set_prototype_desc,
        NULL,
        NULL,
        set_accessor_descs,
        NULL,
    },
    {
        "%SetIteratorPrototype%",
        "IteratorPrototype",
        NULL,
        NULL,
        set_iterator_prototype_field_descs,
        set_iterator_prototype_function_descs,
        NULL,
        NULL,
        "SetIteratorPrototype"
    },
#endif /*ENABLE_SET*/
#if ENABLE_WEAK_MAP
    {
        "WeakMap",
        NULL,
        &weak_map_constructor_desc,
        &weak_map_prototype_desc,
        NULL,
        NULL,
        NULL,
        NULL,
    },
#endif /*ENABLE_WEAK_MAP*/
#if ENABLE_WEAK_SET
    {
        "WeakSet",
        NULL,
        &weak_set_constructor_desc,
        &weak_set_prototype_desc,
        NULL,
        NULL,
        NULL,
        NULL,
    },
#endif /*ENABLE_WEAK_SET*/
#if ENABLE_WEAK_REF
    {
        "WeakRef",
        NULL,
        &weak_ref_constructor_desc,
        &weak_ref_prototype_desc,
        NULL,
        NULL,
        NULL,
        NULL,
    },
#endif /*ENABLE_WEAK_REF*/
#if ENABLE_FINALIZATION_REGISTRY
    {
        "FinalizationRegistry",
        NULL,
        &finalization_registry_constructor_desc,
        &finalization_registry_prototype_desc,
        NULL,
        NULL,
        NULL,
        NULL,
    },
#endif /*ENABLE_FINALIZATION_REGISTRY*/
#if ENABLE_JSON
    {
        "JSON",
        NULL,
        NULL,
        NULL,
        json_field_descs,
        json_function_descs,
        NULL,
        NULL,
    },
#endif /*ENABLE_JSON*/
#if ENABLE_GENERATOR
    {
        "%GeneratorFunction%",
        NULL,
        &generator_function_constructor_desc,
        &generator_function_prototype_desc,
        NULL,
        NULL,
        NULL,
        NULL,
        "GeneratorFunction"
    },
#endif /*ENABLE_GENERATOR*/
#if ENABLE_ASYNC
    {
        "%AsyncFunction%",
        "Function",
        &async_function_constructor_desc,
        &async_function_prototype_desc,
        NULL,
        NULL,
        NULL,
        NULL,
        "AsyncFunction"
    },
    {
        "%AsyncIteratorPrototype%",
        NULL,
        NULL,
        NULL,
        NULL,
        async_iterator_prototype_function_descs,
        NULL,
        NULL,
        "AsyncIteratorPrototype"
    },
    {
        "%AsyncFromSyncIteratorPrototype%",
        "AsyncIteratorPrototype",
        NULL,
        NULL,
        NULL,
        async_from_sync_iterator_prototype_function_descs,
        NULL,
        NULL,
        "AsyncFromSyncIteratorPrototype"
    },
#endif /*ENABLE_ASYNC*/
#if ENABLE_GENERATOR && ENABLE_ASYNC
    {
        "%AsyncGeneratorFunction%",
        NULL,
        &async_generator_function_constructor_desc,
        &async_generator_function_prototype_desc,
        NULL,
        NULL,
        NULL,
        NULL,
        "AsyncGeneratorFunction"
    },
#endif /*ENABLE_GENERATOR && ENABLE_ASYNC*/
#if ENABLE_REFLECT
    {
        "Reflect",
        NULL,
        NULL,
        NULL,
        reflect_field_descs,
        reflect_function_descs,
        NULL,
        NULL
    },
#endif /*ENABLE_REFLECT*/
#if ENABLE_PROXY
    {
        "Proxy",
        NULL,
        &proxy_constructor_desc,
        NULL,
        NULL,
        proxy_function_descs,
        NULL,
        NULL,
        NULL
    },
#endif /*ENABLE_PROXY*/
    {NULL}
};

/*Global description.*/
static const RJS_BuiltinDesc
global_desc = {
    global_field_descs,
    global_function_descs,
    global_object_descs
};

/**
 * Initialize the global object in the realm.
 * \param rt The current runtime.
 * \param realm The realm to be initialized.
 */
void
rjs_realm_global_object_init (RJS_Runtime *rt, RJS_Realm *realm)
{
    RJS_PropertyName pn;
    RJS_GlobalEnv   *ge  = (RJS_GlobalEnv*)rjs_global_env(realm);
    RJS_Value       *go  = rjs_global_object(realm);
    size_t           top = rjs_value_stack_save(rt);
    RJS_Value       *n   = rjs_value_stack_push(rt);

    /*Add "globalThis".*/
    rjs_string_from_chars(rt, n, "globalThis", -1);
    rjs_string_to_property_key(rt, n);
    rjs_property_name_init(rt, &pn, n);

    rjs_create_data_property_attrs_or_throw(rt, go, &pn, &ge->global_this,
                RJS_PROP_FL_DATA|RJS_PROP_FL_WRITABLE|RJS_PROP_FL_CONFIGURABLE);

    rjs_property_name_deinit(rt, &pn);

    /*Load definitions.*/
    rjs_load_builtin_desc(rt, realm, &global_desc);

    /*Create Array.prototype[@@unscopables]*/
    add_Array_prototype_unscopables(rt, realm);

    rjs_value_stack_restore(rt, top);
}
