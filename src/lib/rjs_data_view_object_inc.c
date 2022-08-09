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

/**Data view.*/
typedef struct {
    RJS_Object objcet;      /**< Base object data.*/
    RJS_Value  buffer;      /**< Viewd buffer.*/
    size_t     byte_offset; /**< Byte offset.*/
    size_t     byte_length; /**< Byte length.*/
} RJS_DataView;

/*Scan referenced things in the data view.*/
static void
data_view_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_DataView *dv = ptr;

    rjs_object_op_gc_scan(rt, &dv->objcet);
    rjs_gc_scan_value(rt, &dv->buffer);
}

/*Free the data view.*/
static void
data_view_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_DataView *dv = ptr;

    RJS_DEL(rt, dv);
}

/*Data view operation functions.*/
static const RJS_ObjectOps
data_view_ops = {
    {
        RJS_GC_THING_DATA_VIEW,
        data_view_op_gc_scan,
        data_view_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS
};

/*DataView*/
static RJS_NF(DataView_constructor)
{
    RJS_Value       *buffer   = rjs_argument_get(rt, args, argc, 0);
    RJS_Value       *byte_off = rjs_argument_get(rt, args, argc, 1);
    RJS_Value       *byte_len = rjs_argument_get(rt, args, argc, 2);
    RJS_ArrayBuffer *ab;
    int64_t          offset, view_byte_len;
    size_t           buf_byte_len;
    RJS_DataView    *dv;
    RJS_Result       r;

    if (!nt) {
        r = rjs_throw_type_error(rt, _("\"DataView\" must be used as a constructor"));
        goto end;
    }

    if (rjs_value_get_gc_thing_type(rt, buffer) != RJS_GC_THING_ARRAY_BUFFER) {
        r = rjs_throw_type_error(rt, _("the value is not an array buffer"));
        goto end;
    }

    if ((r = rjs_to_index(rt, byte_off, &offset)) == RJS_ERR)
        goto end;

    if (rjs_is_detached_buffer(rt, buffer)) {
        r = rjs_throw_type_error(rt, _("the array buffer is detached"));
        goto end;
    }

    ab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, buffer);

    buf_byte_len = ab->byte_length;

    if (offset > buf_byte_len) {
        r = rjs_throw_range_error(rt, _("offset must <= the array buffer's length"));
        goto end;
    }

    if (rjs_value_is_undefined(rt, byte_len))
        view_byte_len = buf_byte_len - offset;
    else {
        if ((r = rjs_to_index(rt, byte_len, &view_byte_len)) == RJS_ERR)
            goto end;

        if (offset + view_byte_len > buf_byte_len) {
            r = rjs_throw_range_error(rt, _("data view length must <= the array buffer's length"));
            goto end;
        }
    }

    RJS_NEW(rt, dv);

    rjs_value_copy(rt, &dv->buffer, buffer);
    dv->byte_offset = offset;
    dv->byte_length = view_byte_len;

    r = rjs_ordinary_init_from_constructor(rt, &dv->objcet, nt, RJS_O_DataView_prototype, &data_view_ops, rv);
    if (r == RJS_ERR) {
        RJS_DEL(rt, dv);
        goto end;
    }

    if (rjs_is_detached_buffer(rt, buffer)) {
        r = rjs_throw_type_error(rt, _("the array buffer is detached"));
        goto end;
    }

    r = RJS_OK;
end:
    return r;
}

static const RJS_BuiltinFuncDesc
data_view_constructor_desc = {
    "DataView",
    1,
    DataView_constructor
};

static const RJS_BuiltinFieldDesc
data_view_prototype_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "DataView",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

/*Get the value from the data view.*/
static RJS_Result
get_view_value (RJS_Runtime *rt, RJS_Value *view, RJS_Value *req_idx, RJS_Value *is_little_v,
        RJS_ArrayElementType type, RJS_Value *rv)
{
    RJS_DataView *dv;
    int64_t       get_idx;
    RJS_Bool      is_little;
    size_t        esize;
    RJS_Result    r;

    if (rjs_value_get_gc_thing_type(rt, view) != RJS_GC_THING_DATA_VIEW) {
        r = rjs_throw_type_error(rt, _("the value is not a data view"));
        goto end;
    }

    if ((r = rjs_to_index(rt, req_idx, &get_idx)) == RJS_ERR)
        goto end;

    is_little = rjs_to_boolean(rt, is_little_v);

    dv = (RJS_DataView*)rjs_value_get_object(rt, view);

    if (rjs_is_detached_buffer(rt, &dv->buffer)) {
        r = rjs_throw_type_error(rt, _("the array buffer is detached"));
        goto end;
    }

    esize = rjs_typed_array_element_size(type);

    if (get_idx + esize > dv->byte_length) {
        r = rjs_throw_range_error(rt, _("the request offset overflow"));
        goto end;
    }

    r = rjs_get_value_from_buffer(rt, &dv->buffer, get_idx + dv->byte_offset, type, is_little, rv);
end:
    return r;
}

/*Set the value in the data view.*/
static RJS_Result
set_view_value (RJS_Runtime *rt, RJS_Value *view, RJS_Value *req_idx, RJS_Value *is_little_v,
        RJS_ArrayElementType type, RJS_Value *v)
{
    size_t        top  = rjs_value_stack_save(rt);
    RJS_Value    *setv = rjs_value_stack_push(rt);
    int64_t       get_idx;
    RJS_Bool      is_little;
    RJS_DataView *dv;
    size_t        esize;
    RJS_Result    r;

    if (rjs_value_get_gc_thing_type(rt, view) != RJS_GC_THING_DATA_VIEW) {
        r = rjs_throw_type_error(rt, _("the value is not a data view"));
        goto end;
    }

    if ((r = rjs_to_index(rt, req_idx, &get_idx)) == RJS_ERR)
        goto end;

#if ENABLE_BIG_INT
    if ((type == RJS_ARRAY_ELEMENT_BIGINT64) || (type == RJS_ARRAY_ELEMENT_BIGUINT64)) {
        if ((r = rjs_to_big_int(rt, v, setv)) == RJS_ERR)
            goto end;
    } else
#endif /*ENABLE_BIG_INT*/
    {
        RJS_Number n;

        if ((r = rjs_to_number(rt, v, &n)) == RJS_ERR)
            goto end;

        rjs_value_set_number(rt, setv, n);
    }

    is_little = rjs_to_boolean(rt, is_little_v);

    dv = (RJS_DataView*)rjs_value_get_object(rt, view);

    if (rjs_is_detached_buffer(rt, &dv->buffer)) {
        r = rjs_throw_type_error(rt, _("the array buffer is detached"));
        goto end;
    }

    esize = rjs_typed_array_element_size(type);

    if (get_idx + esize > dv->byte_length) {
        r = rjs_throw_range_error(rt, _("the request offset overflow"));
        goto end;
    }

    r = rjs_set_value_in_buffer(rt, &dv->buffer, get_idx + dv->byte_offset, type, setv, is_little);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

#if ENABLE_BIG_INT
/*DataView.prototype.getBigInt64*/
static RJS_NF(DataView_prototype_getBigInt64)
{
    RJS_Value *byte_off  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *is_little = rjs_argument_get(rt, args, argc, 1);

    return get_view_value(rt, thiz, byte_off, is_little, RJS_ARRAY_ELEMENT_BIGINT64, rv);
}

/*DataView.prototype.getBigUint64*/
static RJS_NF(DataView_prototype_getBigUint64)
{
    RJS_Value *byte_off  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *is_little = rjs_argument_get(rt, args, argc, 1);

    return get_view_value(rt, thiz, byte_off, is_little, RJS_ARRAY_ELEMENT_BIGUINT64, rv);
}

/*DataView.prototype.setBigInt64*/
static RJS_NF(DataView_prototype_setBigInt64)
{
    RJS_Value *byte_off  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *setv      = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *is_little = rjs_argument_get(rt, args, argc, 2);
    RJS_Result r;

    r = set_view_value(rt, thiz, byte_off, is_little, RJS_ARRAY_ELEMENT_BIGINT64, setv);
    if (r == RJS_OK)
        rjs_value_set_undefined(rt, rv);

    return r;
}

/*DataView.prototype.setBigUint64*/
static RJS_NF(DataView_prototype_setBigUint64)
{
    RJS_Value *byte_off  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *setv      = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *is_little = rjs_argument_get(rt, args, argc, 2);
    RJS_Result r;

    r = set_view_value(rt, thiz, byte_off, is_little, RJS_ARRAY_ELEMENT_BIGUINT64, setv);
    if (r == RJS_OK)
        rjs_value_set_undefined(rt, rv);

    return r;
}
#endif /*ENABLE_BIG_INT*/

/*DataView.prototype.getFloat32*/
static RJS_NF(DataView_prototype_getFloat32)
{
    RJS_Value *byte_off  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *is_little = rjs_argument_get(rt, args, argc, 1);

    return get_view_value(rt, thiz, byte_off, is_little, RJS_ARRAY_ELEMENT_FLOAT32, rv);
}

/*DataView.prototype.getFloat64*/
static RJS_NF(DataView_prototype_getFloat64)
{
    RJS_Value *byte_off  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *is_little = rjs_argument_get(rt, args, argc, 1);

    return get_view_value(rt, thiz, byte_off, is_little, RJS_ARRAY_ELEMENT_FLOAT64, rv);
}

/*DataView.prototype.getInt8*/
static RJS_NF(DataView_prototype_getInt8)
{
    RJS_Value *byte_off  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *is_little = rjs_argument_get(rt, args, argc, 1);

    return get_view_value(rt, thiz, byte_off, is_little, RJS_ARRAY_ELEMENT_INT8, rv);
}

/*DataView.prototype.getInt16*/
static RJS_NF(DataView_prototype_getInt16)
{
    RJS_Value *byte_off  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *is_little = rjs_argument_get(rt, args, argc, 1);

    return get_view_value(rt, thiz, byte_off, is_little, RJS_ARRAY_ELEMENT_INT16, rv);
}

/*DataView.prototype.getInt32*/
static RJS_NF(DataView_prototype_getInt32)
{
    RJS_Value *byte_off  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *is_little = rjs_argument_get(rt, args, argc, 1);

    return get_view_value(rt, thiz, byte_off, is_little, RJS_ARRAY_ELEMENT_INT32, rv);
}

/*DataView.prototype.getUint8*/
static RJS_NF(DataView_prototype_getUint8)
{
    RJS_Value *byte_off  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *is_little = rjs_argument_get(rt, args, argc, 1);

    return get_view_value(rt, thiz, byte_off, is_little, RJS_ARRAY_ELEMENT_UINT8, rv);
}

/*DataView.prototype.getUint16*/
static RJS_NF(DataView_prototype_getUint16)
{
    RJS_Value *byte_off  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *is_little = rjs_argument_get(rt, args, argc, 1);

    return get_view_value(rt, thiz, byte_off, is_little, RJS_ARRAY_ELEMENT_UINT16, rv);
}

/*DataView.prototype.getUint32*/
static RJS_NF(DataView_prototype_getUint32)
{
    RJS_Value *byte_off  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *is_little = rjs_argument_get(rt, args, argc, 1);

    return get_view_value(rt, thiz, byte_off, is_little, RJS_ARRAY_ELEMENT_UINT32, rv);
}

/*DataView.prototype.setFloat32*/
static RJS_NF(DataView_prototype_setFloat32)
{
    RJS_Value *byte_off  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *setv      = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *is_little = rjs_argument_get(rt, args, argc, 2);
    RJS_Result r;

    r = set_view_value(rt, thiz, byte_off, is_little, RJS_ARRAY_ELEMENT_FLOAT32, setv);
    if (r == RJS_OK)
        rjs_value_set_undefined(rt, rv);

    return r;
}

/*DataView.prototype.setFloat64*/
static RJS_NF(DataView_prototype_setFloat64)
{
    RJS_Value *byte_off  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *setv      = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *is_little = rjs_argument_get(rt, args, argc, 2);
    RJS_Result r;

    r = set_view_value(rt, thiz, byte_off, is_little, RJS_ARRAY_ELEMENT_FLOAT64, setv);
    if (r == RJS_OK)
        rjs_value_set_undefined(rt, rv);

    return r;
}

/*DataView.prototype.setInt8*/
static RJS_NF(DataView_prototype_setInt8)
{
    RJS_Value *byte_off  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *setv      = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *is_little = rjs_argument_get(rt, args, argc, 2);
    RJS_Result r;

    r = set_view_value(rt, thiz, byte_off, is_little, RJS_ARRAY_ELEMENT_INT8, setv);
    if (r == RJS_OK)
        rjs_value_set_undefined(rt, rv);

    return r;
}

/*DataView.prototype.setInt16*/
static RJS_NF(DataView_prototype_setInt16)
{
    RJS_Value *byte_off  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *setv      = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *is_little = rjs_argument_get(rt, args, argc, 2);
    RJS_Result r;

    r = set_view_value(rt, thiz, byte_off, is_little, RJS_ARRAY_ELEMENT_INT16, setv);
    if (r == RJS_OK)
        rjs_value_set_undefined(rt, rv);

    return r;
}

/*DataView.prototype.setInt32*/
static RJS_NF(DataView_prototype_setInt32)
{
    RJS_Value *byte_off  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *setv      = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *is_little = rjs_argument_get(rt, args, argc, 2);
    RJS_Result r;

    r = set_view_value(rt, thiz, byte_off, is_little, RJS_ARRAY_ELEMENT_INT32, setv);
    if (r == RJS_OK)
        rjs_value_set_undefined(rt, rv);

    return r;
}

/*DataView.prototype.setUint8*/
static RJS_NF(DataView_prototype_setUint8)
{
    RJS_Value *byte_off  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *setv      = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *is_little = rjs_argument_get(rt, args, argc, 2);
    RJS_Result r;

    r = set_view_value(rt, thiz, byte_off, is_little, RJS_ARRAY_ELEMENT_UINT8, setv);
    if (r == RJS_OK)
        rjs_value_set_undefined(rt, rv);

    return r;
}

/*DataView.prototype.setUint16*/
static RJS_NF(DataView_prototype_setUint16)
{
    RJS_Value *byte_off  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *setv      = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *is_little = rjs_argument_get(rt, args, argc, 2);
    RJS_Result r;

    r = set_view_value(rt, thiz, byte_off, is_little, RJS_ARRAY_ELEMENT_UINT16, setv);
    if (r == RJS_OK)
        rjs_value_set_undefined(rt, rv);

    return r;
}

/*DataView.prototype.setUint32*/
static RJS_NF(DataView_prototype_setUint32)
{
    RJS_Value *byte_off  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *setv      = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *is_little = rjs_argument_get(rt, args, argc, 2);
    RJS_Result r;

    r = set_view_value(rt, thiz, byte_off, is_little, RJS_ARRAY_ELEMENT_UINT32, setv);
    if (r == RJS_OK)
        rjs_value_set_undefined(rt, rv);

    return r;
}

static const RJS_BuiltinFuncDesc
data_view_prototype_function_descs[] = {
#if ENABLE_BIG_INT
    {
        "getBigInt64",
        1,
        DataView_prototype_getBigInt64
    },
    {
        "getBigUint64",
        1,
        DataView_prototype_getBigUint64
    },
    {
        "setBigInt64",
        2,
        DataView_prototype_setBigInt64
    },
    {
        "setBigUint64",
        2,
        DataView_prototype_setBigUint64
    },
#endif /*ENABLE_BIG_INT*/
    {
        "getFloat32",
        1,
        DataView_prototype_getFloat32
    },
    {
        "getFloat64",
        1,
        DataView_prototype_getFloat64
    },
    {
        "getInt8",
        1,
        DataView_prototype_getInt8
    },
    {
        "getInt16",
        1,
        DataView_prototype_getInt16
    },
    {
        "getInt32",
        1,
        DataView_prototype_getInt32
    },
    {
        "getUint8",
        1,
        DataView_prototype_getUint8
    },
    {
        "getUint16",
        1,
        DataView_prototype_getUint16
    },
    {
        "getUint32",
        1,
        DataView_prototype_getUint32
    },
    {
        "setFloat32",
        2,
        DataView_prototype_setFloat32
    },
    {
        "setFloat64",
        2,
        DataView_prototype_setFloat64
    },
    {
        "setInt8",
        2,
        DataView_prototype_setInt8
    },
    {
        "setInt16",
        2,
        DataView_prototype_setInt16
    },
    {
        "setInt32",
        2,
        DataView_prototype_setInt32
    },
    {
        "setUint8",
        2,
        DataView_prototype_setUint8
    },
    {
        "setUint16",
        2,
        DataView_prototype_setUint16
    },
    {
        "setUint32",
        2,
        DataView_prototype_setUint32
    },
    {NULL}
};

/*get DataView.prototype.buffer*/
static RJS_NF(DataView_prototype_buffer_get)
{
    RJS_DataView *dv;
    RJS_Result    r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_DATA_VIEW) {
        r = rjs_throw_type_error(rt, _("the value is not a data view"));
        goto end;
    }

    dv = (RJS_DataView*)rjs_value_get_object(rt, thiz);

    rjs_value_copy(rt, rv, &dv->buffer);

    r = RJS_OK;
end:
    return r;
}

/*get DataView.prototype.byteLength*/
static RJS_NF(DataView_prototype_byteLength_get)
{
    RJS_DataView *dv;
    RJS_Result    r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_DATA_VIEW) {
        r = rjs_throw_type_error(rt, _("the value is not a data view"));
        goto end;
    }

    dv = (RJS_DataView*)rjs_value_get_object(rt, thiz);

    if (rjs_is_detached_buffer(rt, &dv->buffer)) {
        r = rjs_throw_type_error(rt, _("the array buffer is detached"));
        goto end;
    }

    rjs_value_set_number(rt, rv, dv->byte_length);

    r = RJS_OK;
end:
    return r;
}

/*get DataView.prototype.byteOffset*/
static RJS_NF(DataView_prototype_byteOffset_get)
{
    RJS_DataView *dv;
    RJS_Result    r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_DATA_VIEW) {
        r = rjs_throw_type_error(rt, _("the value is not a data view"));
        goto end;
    }

    dv = (RJS_DataView*)rjs_value_get_object(rt, thiz);

    if (rjs_is_detached_buffer(rt, &dv->buffer)) {
        r = rjs_throw_type_error(rt, _("the array buffer is detached"));
        goto end;
    }

    rjs_value_set_number(rt, rv, dv->byte_offset);

    r = RJS_OK;
end:
    return r;
}

static const RJS_BuiltinAccessorDesc
data_view_prototype_accessor_descs[] = {
    {
        "buffer",
        DataView_prototype_buffer_get
    },
    {
        "byteLength",
        DataView_prototype_byteLength_get
    },
    {
        "byteOffset",
        DataView_prototype_byteOffset_get
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
data_view_prototype_desc = {
    "DataView",
    NULL,
    NULL,
    NULL,
    data_view_prototype_field_descs,
    data_view_prototype_function_descs,
    data_view_prototype_accessor_descs,
    NULL,
    "DataView_prototype"
};