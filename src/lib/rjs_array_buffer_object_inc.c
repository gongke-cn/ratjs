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

/*ArrayBuffer.*/
static RJS_NF(ArrayBuffer_constructor)
{
    RJS_Value *length = rjs_argument_get(rt, args, argc, 0);
    int64_t    byte_len;
    RJS_Result r;

    if (!nt) {
        r = rjs_throw_type_error(rt, _("\"ArrayBuffer\" must be used as a constructor"));
        goto end;
    }

    if ((r = rjs_to_index(rt, length, &byte_len)) == RJS_ERR)
        goto end;

    r = rjs_allocate_array_buffer(rt, nt, byte_len, rv);
end:
    return r;
}

static const RJS_BuiltinFuncDesc
array_buffer_constructor_desc = {
    "ArrayBuffer",
    1,
    ArrayBuffer_constructor
};

/*ArrayBuffer.isView*/
static RJS_NF(ArrayBuffer_isView)
{
    RJS_Value *arg = rjs_argument_get(rt, args, argc, 0);
    RJS_Bool   b;

    if (!rjs_value_is_object(rt, arg)) {
        b = RJS_FALSE;
    }
#if ENABLE_INT_INDEXED_OBJECT
    else if (rjs_value_get_gc_thing_type(rt, arg) == RJS_GC_THING_INT_INDEXED_OBJECT) {
        b = RJS_TRUE;
    }
#endif /*ENABLE_INT_INDEXED_OBJECT*/
#if ENABLE_DATA_VIEW
    else if (rjs_value_get_gc_thing_type(rt, arg) == RJS_GC_THING_DATA_VIEW) {
        b = RJS_TRUE;
    }
#endif /*ENABLE_DATA_VIEW*/
    else {
        b = RJS_FALSE;
    }

    rjs_value_set_boolean(rt, rv, b);
    return RJS_OK;
}

static const RJS_BuiltinFuncDesc
array_buffer_function_descs[] = {
    {
        "isView",
        1,
        ArrayBuffer_isView
    },
    {NULL}
};

static const RJS_BuiltinAccessorDesc
array_buffer_accessor_descs[] = {
    {
        "@@species",
        rjs_return_this
    },
    {NULL}
};

static const RJS_BuiltinFieldDesc
array_buffer_prototype_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "ArrayBuffer",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

/*ArrayBuffer.prototype.slice*/
static RJS_NF(ArrayBuffer_prototype_slice)
{
    RJS_Value       *start  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value       *end    = rjs_argument_get(rt, args, argc, 1);
    RJS_Realm       *realm  = rjs_realm_current(rt);
    size_t           top    = rjs_value_stack_save(rt);
    RJS_Value       *constr = rjs_value_stack_push(rt);
    RJS_Value       *lenv   = rjs_value_stack_push(rt);
    RJS_ArrayBuffer *ab, *nab;
    ssize_t          len, first, final, new_len;
    RJS_Number       rel_start, rel_end;
    RJS_Result       r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_ARRAY_BUFFER) {
        r = rjs_throw_type_error(rt, _("the value is not an array buffer"));
        goto end;
    }

    if (rjs_is_shared_array_buffer(rt, thiz)) {
        r = rjs_throw_type_error(rt, _("the array buffer is shared"));
        goto end;
    }

    if (rjs_is_detached_buffer(rt, thiz)) {
        r = rjs_throw_type_error(rt, _("the array buffer is detached"));
        goto end;
    }

    ab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, thiz);

    len = ab->byte_length;

    if ((r = rjs_to_integer_or_infinity(rt, start, &rel_start)) == RJS_ERR)
        goto end;

    if (rel_start == -INFINITY)
        first = 0;
    else if (rel_start < 0)
        first = RJS_MAX(rel_start + len, 0);
    else
        first = RJS_MIN(rel_start, len);

    if (rjs_value_is_undefined(rt, end))
        rel_end = len;
    else if ((r = rjs_to_integer_or_infinity(rt, end, &rel_end)) == RJS_ERR)
        goto end;

    if (rel_end == -INFINITY)
        final = 0;
    else if (rel_end < 0)
        final = RJS_MAX(rel_end + len, 0);
    else
        final = RJS_MIN(rel_end, len);

    new_len = RJS_MAX(final - first, 0);

    if ((r = rjs_species_constructor(rt, thiz, rjs_o_ArrayBuffer(realm), constr)) == RJS_ERR)
        goto end;

    rjs_value_set_number(rt, lenv, new_len);

    if ((r = rjs_construct(rt, constr, lenv, 1, NULL, rv)) == RJS_ERR)
        goto end;

    if (rjs_value_get_gc_thing_type(rt, rv) != RJS_GC_THING_ARRAY_BUFFER) {
        r = rjs_throw_type_error(rt, _("the value is not an array buffer"));
        goto end;
    }

    if (rjs_is_shared_array_buffer(rt, rv)) {
        r = rjs_throw_type_error(rt, _("the array buffer is shared"));
        goto end;
    }

    if (rjs_is_detached_buffer(rt, rv)) {
        r = rjs_throw_type_error(rt, _("the array buffer is detached"));
        goto end;
    }

    if (rjs_same_value(rt, rv, thiz)) {
        r = rjs_throw_type_error(rt, _("new array buffer is same as the source one"));
        goto end;
    }

    nab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, rv);

    if (nab->byte_length < new_len) {
        r = rjs_throw_type_error(rt, _("the length of the array buffer is less than the expect value"));
        goto end;
    }

    if (rjs_is_detached_buffer(rt, thiz)) {
        r = rjs_throw_type_error(rt, _("the array buffer is detached"));
        goto end;
    }

    memcpy(rjs_data_block_get_buffer(nab->data_block),
            rjs_data_block_get_buffer(ab->data_block) + first, new_len);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
array_buffer_prototype_function_descs[] = {
    {
        "slice",
        2,
        ArrayBuffer_prototype_slice
    },
    {NULL}
};

/*get ArrayBuffer.prototype.byteLength*/
static RJS_NF(ArrayBuffer_prototype_byteLength_get)
{
    RJS_ArrayBuffer *ab;
    RJS_Result       r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_ARRAY_BUFFER) {
        r = rjs_throw_type_error(rt, _("the value is not an array buffer"));
        goto end;
    }

    if (rjs_is_shared_array_buffer(rt, thiz)) {
        r = rjs_throw_type_error(rt, _("the array buffer is shared"));
        goto end;
    }

    if (rjs_is_detached_buffer(rt, thiz)) {
        r = rjs_throw_type_error(rt, _("the array buffer is detached"));
        goto end;
    }

    ab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, thiz);

    rjs_value_set_number(rt, rv, ab->byte_length);

    r = RJS_OK;
end:
    return r;
}

static const RJS_BuiltinAccessorDesc
array_buffer_prototype_accessor_descs[] = {
    {
        "byteLength",
        ArrayBuffer_prototype_byteLength_get
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
array_buffer_prototype_desc = {
    "ArrayBuffer",
    NULL,
    NULL,
    NULL,
    array_buffer_prototype_field_descs,
    array_buffer_prototype_function_descs,
    array_buffer_prototype_accessor_descs,
    NULL,
    "ArrayBuffer_prototype"
};
