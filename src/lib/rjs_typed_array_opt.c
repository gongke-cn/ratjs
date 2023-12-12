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

/**
 * Create a typed array.
 * \param rt The runtime.
 * \param type The element type of the typed array.
 * \param args The arguments.
 * \param argc The arguments' count.
 * \param[out] ta Return the typed array.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_create_typed_array (RJS_Runtime *rt, RJS_ArrayElementType type,
        RJS_Value *args, size_t argc, RJS_Value *ta)
{
    RJS_Realm *realm = rjs_realm_current(rt);
    RJS_Value *c;

    switch (type) {
    case RJS_ARRAY_ELEMENT_UINT8:
        c = rjs_o_Uint8Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_INT8:
        c = rjs_o_Int8Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_UINT8C:
        c = rjs_o_Uint8ClampedArray(realm);
        break;
    case RJS_ARRAY_ELEMENT_UINT16:
        c = rjs_o_Uint16Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_INT16:
        c = rjs_o_Int16Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_UINT32:
        c = rjs_o_Uint32Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_INT32:
        c = rjs_o_Int32Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_FLOAT32:
        c = rjs_o_Float32Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_FLOAT64:
        c = rjs_o_Float64Array(realm);
        break;
#if ENABLE_BIG_INT
    case RJS_ARRAY_ELEMENT_BIGUINT64:
        c = rjs_o_BigUint64Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_BIGINT64:
        c = rjs_o_BigInt64Array(realm);
        break;
#endif /*ENABLE_BIG_INT*/
    default:
        assert(0);
    }

    return rjs_construct(rt, c, args, argc, c, ta);
}

/**
 * Get the element type of the array type.
 * \param rt The current runtime.
 * \param ta The typed array.
 * \return The element type of the array type.
 * \retval -1 The value is not a typed array.
 */
RJS_ArrayElementType
rjs_typed_array_get_type (RJS_Runtime *rt, RJS_Value *ta)
{
    RJS_IntIndexedObject *iio;

    if (rjs_value_get_gc_thing_type(rt, ta) != RJS_GC_THING_INT_INDEXED_OBJECT)
        return -1;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, ta);

    return iio->type;
}

/**
 * Get the typed array's buffer pointer.
 * \param rt The current runtime.
 * \param ta The typed array.
 * \return The buffer's pointer.
 */
void*
rjs_typed_array_get_buffer (RJS_Runtime *rt, RJS_Value *ta)
{
    RJS_IntIndexedObject *iio;
    RJS_DataBlock        *db;

    if (rjs_value_get_gc_thing_type(rt, ta) != RJS_GC_THING_INT_INDEXED_OBJECT) {
        rjs_throw_type_error(rt, _("the value is not a typed array"));
        return NULL;
    }

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, ta);

    if (rjs_is_detached_buffer(rt, &iio->buffer)) {
        rjs_throw_type_error(rt, _("the array buffer is detached"));
        return NULL;
    }

    db = rjs_array_buffer_get_data_block(rt, &iio->buffer);

    return db->data;
}

/**
 * Get the typed array's array length
 * \param rt The current runtime.
 * \param ta The typed array.
 * \return The length of the typed array.
 */
size_t
rjs_typed_array_get_length (RJS_Runtime *rt, RJS_Value *ta)
{
    RJS_IntIndexedObject *iio;

    if (rjs_value_get_gc_thing_type(rt, ta) != RJS_GC_THING_INT_INDEXED_OBJECT) {
        rjs_throw_type_error(rt, _("the value is not a typed array"));
        return (size_t)-1;
    }

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, ta);

    return iio->array_length;
}
