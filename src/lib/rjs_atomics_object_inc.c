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
#include <stdatomic.h>

static const RJS_BuiltinFieldDesc
atomics_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "Atomics",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

/*Check if the object is a valid integer typed array.*/
static RJS_Result
validate_integer_typed_array (RJS_Runtime *rt, RJS_Value *o, RJS_Bool waitable)
{
    RJS_IntIndexedObject *iio;

    if (rjs_value_get_gc_thing_type(rt, o) != RJS_GC_THING_INT_INDEXED_OBJECT)
        return rjs_throw_type_error(rt, _("the value is not a typed array"));

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, o);

    if (rjs_is_detached_buffer(rt, &iio->buffer))
        return rjs_throw_type_error(rt, _("the array buffer is detached"));

    if (waitable) {
        if ((iio->type != RJS_ARRAY_ELEMENT_INT32)
#if ENABLE_BIG_INT
                && (iio->type != RJS_ARRAY_ELEMENT_BIGINT64)
#endif /*ENABLE_BIG_INT*/
                )
            return rjs_throw_type_error(rt, _("the element type must be Int32 or BigInt64"));
    } else {
        switch (iio->type) {
        case RJS_ARRAY_ELEMENT_INT8:
        case RJS_ARRAY_ELEMENT_INT16:
        case RJS_ARRAY_ELEMENT_INT32:
        case RJS_ARRAY_ELEMENT_UINT8:
        case RJS_ARRAY_ELEMENT_UINT16:
        case RJS_ARRAY_ELEMENT_UINT32:
#if ENABLE_BIG_INT
        case RJS_ARRAY_ELEMENT_BIGINT64:
        case RJS_ARRAY_ELEMENT_BIGUINT64:
#endif /*ENABLE_BIG_INT*/
            break;
        default:
            return rjs_throw_type_error(rt, _("the element type must be integer"));
        }
    }

    return RJS_TRUE;
}

/*Check the request index.*/
static RJS_Result
validate_atomic_access (RJS_Runtime *rt, RJS_Value *ta, RJS_Value *req_idx, size_t *poff)
{
    RJS_IntIndexedObject *iio;
    size_t                len, esize;
    int64_t               acc_idx;
    RJS_Result            r;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, ta);
    len = iio->array_length;

    if ((r = rjs_to_index(rt, req_idx, &acc_idx)) == RJS_ERR)
        return r;

    if (acc_idx >= len)
        return rjs_throw_range_error(rt, _("request index overflow"));

    esize = rjs_typed_array_element_size(iio->type);

    *poff = iio->byte_offset + esize * acc_idx;
    return RJS_OK;
}

/*Atomics.add*/
static RJS_NF(Atomics_add)
{
    RJS_Value            *ta      = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *index   = rjs_argument_get(rt, args, argc, 1);
    RJS_Value            *value   = rjs_argument_get(rt, args, argc, 2);
    size_t                idx_pos = 0;
    size_t                top     = rjs_value_stack_save(rt);
#if ENABLE_BIG_INT
    RJS_Value            *bi      = rjs_value_stack_push(rt);
#endif /*ENABLE_BIG_INT*/
    RJS_IntIndexedObject *iio;
    RJS_ArrayBuffer      *ab;
    uint8_t              *ptr;
    RJS_Result            r;

    if ((r = validate_integer_typed_array(rt, ta, RJS_FALSE)) == RJS_ERR)
        goto end;

    if ((r = validate_atomic_access(rt, ta, index, &idx_pos)) == RJS_ERR)
        goto end;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, ta);
    ab  = (RJS_ArrayBuffer*)rjs_value_get_object(rt, &iio->buffer);
    ptr = rjs_data_block_get_buffer(ab->data_block) + idx_pos;

    switch (iio->type) {
    case RJS_ARRAY_ELEMENT_INT8: {
        int8_t       i;
        atomic_char *p;

        if ((r = rjs_to_int8(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_char*)ptr;

        i = atomic_fetch_add(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_INT16: {
        int16_t       i;
        atomic_short *p;

        if ((r = rjs_to_int16(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_short*)ptr;

        i = atomic_fetch_add(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_INT32: {
        int32_t     i;
        atomic_int *p;

        if ((r = rjs_to_int32(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_int*)ptr;

        i = atomic_fetch_add(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT8: {
        uint8_t       i;
        atomic_uchar *p;

        if ((r = rjs_to_uint8(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_uchar*)ptr;

        i = atomic_fetch_add(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT16: {
        uint16_t       i;
        atomic_ushort *p;

        if ((r = rjs_to_uint16(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_ushort*)ptr;

        i = atomic_fetch_add(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT32: {
        uint32_t     i;
        atomic_uint *p;

        if ((r = rjs_to_uint32(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_uint*)ptr;

        i = atomic_fetch_add(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
#if ENABLE_BIG_INT
    case RJS_ARRAY_ELEMENT_BIGINT64: {
        int64_t       i;
        atomic_llong *p;

        if ((r = rjs_to_big_int(rt, value, bi)) == RJS_ERR)
            goto end;

        if ((r = rjs_big_int_to_int64(rt, bi, &i)) == RJS_ERR)
            goto end;

        p = (atomic_llong*)ptr;

        i = atomic_fetch_add(p, i);

        rjs_big_int_from_int64(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_BIGUINT64: {
        uint64_t       i;
        atomic_ullong *p;

        if ((r = rjs_to_big_int(rt, value, bi)) == RJS_ERR)
            goto end;

        if ((r = rjs_big_int_to_uint64(rt, bi, &i)) == RJS_ERR)
            goto end;

        p = (atomic_ullong*)ptr;

        i = atomic_fetch_add(p, i);

        rjs_big_int_from_uint64(rt, rv, i);
        break;
    }
#endif /*ENABLE_BIG_INT*/
    default:
        assert(0);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Atomics.and*/
static RJS_NF(Atomics_and)
{
    RJS_Value            *ta      = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *index   = rjs_argument_get(rt, args, argc, 1);
    RJS_Value            *value   = rjs_argument_get(rt, args, argc, 2);
    size_t                idx_pos = 0;
    size_t                top     = rjs_value_stack_save(rt);
#if ENABLE_BIG_INT
    RJS_Value            *bi      = rjs_value_stack_push(rt);
#endif /*ENABLE_BIG_INT*/
    RJS_IntIndexedObject *iio;
    RJS_ArrayBuffer      *ab;
    uint8_t              *ptr;
    RJS_Result            r;

    if ((r = validate_integer_typed_array(rt, ta, RJS_FALSE)) == RJS_ERR)
        goto end;

    if ((r = validate_atomic_access(rt, ta, index, &idx_pos)) == RJS_ERR)
        goto end;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, ta);
    ab  = (RJS_ArrayBuffer*)rjs_value_get_object(rt, &iio->buffer);
    ptr = rjs_data_block_get_buffer(ab->data_block) + idx_pos;

    switch (iio->type) {
    case RJS_ARRAY_ELEMENT_INT8: {
        int8_t       i;
        atomic_char *p;

        if ((r = rjs_to_int8(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_char*)ptr;

        i = atomic_fetch_and(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_INT16: {
        int16_t       i;
        atomic_short *p;

        if ((r = rjs_to_int16(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_short*)ptr;

        i = atomic_fetch_and(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_INT32: {
        int32_t     i;
        atomic_int *p;

        if ((r = rjs_to_int32(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_int*)ptr;

        i = atomic_fetch_and(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT8: {
        uint8_t       i;
        atomic_uchar *p;

        if ((r = rjs_to_uint8(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_uchar*)ptr;

        i = atomic_fetch_and(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT16: {
        uint16_t       i;
        atomic_ushort *p;

        if ((r = rjs_to_uint16(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_ushort*)ptr;

        i = atomic_fetch_and(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT32: {
        uint32_t     i;
        atomic_uint *p;

        if ((r = rjs_to_uint32(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_uint*)ptr;

        i = atomic_fetch_and(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
#if ENABLE_BIG_INT
    case RJS_ARRAY_ELEMENT_BIGINT64: {
        int64_t       i;
        atomic_llong *p;

        if ((r = rjs_to_big_int(rt, value, bi)) == RJS_ERR)
            goto end;

        if ((r = rjs_big_int_to_int64(rt, bi, &i)) == RJS_ERR)
            goto end;

        p = (atomic_llong*)ptr;

        i = atomic_fetch_and(p, i);

        rjs_big_int_from_int64(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_BIGUINT64: {
        uint64_t       i;
        atomic_ullong *p;

        if ((r = rjs_to_big_int(rt, value, bi)) == RJS_ERR)
            goto end;

        if ((r = rjs_big_int_to_uint64(rt, bi, &i)) == RJS_ERR)
            goto end;

        p = (atomic_ullong*)ptr;

        i = atomic_fetch_and(p, i);

        rjs_big_int_from_uint64(rt, rv, i);
        break;
    }
#endif /*ENABLE_BIG_INT*/
    default:
        assert(0);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Atomics.compareExchange*/
static RJS_NF(Atomics_compareExchange)
{
    RJS_Value            *ta      = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *index   = rjs_argument_get(rt, args, argc, 1);
    RJS_Value            *value   = rjs_argument_get(rt, args, argc, 2);
    RJS_Value            *repv    = rjs_argument_get(rt, args, argc, 3);
    size_t                idx_pos = 0;
    size_t                top     = rjs_value_stack_save(rt);
#if ENABLE_BIG_INT
    RJS_Value            *bi      = rjs_value_stack_push(rt);
#endif /*ENABLE_BIG_INT*/
    RJS_IntIndexedObject *iio;
    RJS_ArrayBuffer      *ab;
    uint8_t              *ptr;
    RJS_Result            r;

    if ((r = validate_integer_typed_array(rt, ta, RJS_FALSE)) == RJS_ERR)
        goto end;

    if ((r = validate_atomic_access(rt, ta, index, &idx_pos)) == RJS_ERR)
        goto end;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, ta);
    ab  = (RJS_ArrayBuffer*)rjs_value_get_object(rt, &iio->buffer);
    ptr = rjs_data_block_get_buffer(ab->data_block) + idx_pos;

    switch (iio->type) {
    case RJS_ARRAY_ELEMENT_INT8: {
        int8_t       i, ri;
        atomic_char *p;

        if ((r = rjs_to_int8(rt, value, &i)) == RJS_ERR)
            goto end;
        if ((r = rjs_to_int8(rt, repv, &ri)) == RJS_ERR)
            goto end;

        p = (atomic_char*)ptr;

        atomic_compare_exchange_strong(p, (char*)&i, ri);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_INT16: {
        int16_t       i, ri;
        atomic_short *p;

        if ((r = rjs_to_int16(rt, value, &i)) == RJS_ERR)
            goto end;
        if ((r = rjs_to_int16(rt, repv, &ri)) == RJS_ERR)
            goto end;

        p = (atomic_short*)ptr;

        atomic_compare_exchange_strong(p, &i, ri);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_INT32: {
        int32_t     i, ri;
        atomic_int *p;

        if ((r = rjs_to_int32(rt, value, &i)) == RJS_ERR)
            goto end;
        if ((r = rjs_to_int32(rt, repv, &ri)) == RJS_ERR)
            goto end;

        p = (atomic_int*)ptr;

        atomic_compare_exchange_strong(p, &i, ri);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT8: {
        uint8_t       i, ri;
        atomic_uchar *p;

        if ((r = rjs_to_uint8(rt, value, &i)) == RJS_ERR)
            goto end;
        if ((r = rjs_to_uint8(rt, repv, &ri)) == RJS_ERR)
            goto end;

        p = (atomic_uchar*)ptr;

        atomic_compare_exchange_strong(p, &i, ri);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT16: {
        uint16_t       i, ri;
        atomic_ushort *p;

        if ((r = rjs_to_uint16(rt, value, &i)) == RJS_ERR)
            goto end;
        if ((r = rjs_to_uint16(rt, repv, &ri)) == RJS_ERR)
            goto end;

        p = (atomic_ushort*)ptr;

        atomic_compare_exchange_strong(p, &i, ri);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT32: {
        uint32_t     i, ri;
        atomic_uint *p;

        if ((r = rjs_to_uint32(rt, value, &i)) == RJS_ERR)
            goto end;
        if ((r = rjs_to_uint32(rt, repv, &ri)) == RJS_ERR)
            goto end;

        p = (atomic_uint*)ptr;

        atomic_compare_exchange_strong(p, &i, ri);

        rjs_value_set_number(rt, rv, i);
        break;
    }
#if ENABLE_BIG_INT
    case RJS_ARRAY_ELEMENT_BIGINT64: {
        int64_t       i, ri;
        atomic_llong *p;

        if ((r = rjs_to_big_int(rt, value, bi)) == RJS_ERR)
            goto end;
        if ((r = rjs_big_int_to_int64(rt, bi, &i)) == RJS_ERR)
            goto end;
        if ((r = rjs_to_big_int(rt, repv, bi)) == RJS_ERR)
            goto end;
        if ((r = rjs_big_int_to_int64(rt, bi, &ri)) == RJS_ERR)
            goto end;

        p = (atomic_llong*)ptr;

        atomic_compare_exchange_strong(p, (long long*)&i, ri);

        rjs_big_int_from_int64(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_BIGUINT64: {
        uint64_t       i, ri;
        atomic_ullong *p;

        if ((r = rjs_to_big_int(rt, value, bi)) == RJS_ERR)
            goto end;
        if ((r = rjs_big_int_to_uint64(rt, bi, &i)) == RJS_ERR)
            goto end;
        if ((r = rjs_to_big_int(rt, repv, bi)) == RJS_ERR)
            goto end;
        if ((r = rjs_big_int_to_uint64(rt, bi, &ri)) == RJS_ERR)
            goto end;

        p = (atomic_ullong*)ptr;

        atomic_compare_exchange_strong(p, (unsigned long long*)&i, ri);

        rjs_big_int_from_uint64(rt, rv, i);
        break;
    }
#endif /*ENABLE_BIG_INT*/
    default:
        assert(0);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Atomics.exchange*/
static RJS_NF(Atomics_exchange)
{
    RJS_Value            *ta      = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *index   = rjs_argument_get(rt, args, argc, 1);
    RJS_Value            *value   = rjs_argument_get(rt, args, argc, 2);
    size_t                idx_pos = 0;
    size_t                top     = rjs_value_stack_save(rt);
#if ENABLE_BIG_INT
    RJS_Value            *bi      = rjs_value_stack_push(rt);
#endif /*ENABLE_BIG_INT*/
    RJS_IntIndexedObject *iio;
    RJS_ArrayBuffer      *ab;
    uint8_t              *ptr;
    RJS_Result            r;

    if ((r = validate_integer_typed_array(rt, ta, RJS_FALSE)) == RJS_ERR)
        goto end;

    if ((r = validate_atomic_access(rt, ta, index, &idx_pos)) == RJS_ERR)
        goto end;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, ta);
    ab  = (RJS_ArrayBuffer*)rjs_value_get_object(rt, &iio->buffer);
    ptr = rjs_data_block_get_buffer(ab->data_block) + idx_pos;

    switch (iio->type) {
    case RJS_ARRAY_ELEMENT_INT8: {
        int8_t       i;
        atomic_char *p;

        if ((r = rjs_to_int8(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_char*)ptr;

        i = atomic_exchange(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_INT16: {
        int16_t       i;
        atomic_short *p;

        if ((r = rjs_to_int16(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_short*)ptr;

        i = atomic_exchange(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_INT32: {
        int32_t     i;
        atomic_int *p;

        if ((r = rjs_to_int32(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_int*)ptr;

        i = atomic_exchange(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT8: {
        uint8_t       i;
        atomic_uchar *p;

        if ((r = rjs_to_uint8(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_uchar*)ptr;

        i = atomic_exchange(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT16: {
        uint16_t       i;
        atomic_ushort *p;

        if ((r = rjs_to_uint16(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_ushort*)ptr;

        i = atomic_exchange(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT32: {
        uint32_t     i;
        atomic_uint *p;

        if ((r = rjs_to_uint32(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_uint*)ptr;

        i = atomic_exchange(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
#if ENABLE_BIG_INT
    case RJS_ARRAY_ELEMENT_BIGINT64: {
        int64_t       i;
        atomic_llong *p;

        if ((r = rjs_to_big_int(rt, value, bi)) == RJS_ERR)
            goto end;

        if ((r = rjs_big_int_to_int64(rt, bi, &i)) == RJS_ERR)
            goto end;

        p = (atomic_llong*)ptr;

        i = atomic_exchange(p, i);

        rjs_big_int_from_int64(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_BIGUINT64: {
        uint64_t       i;
        atomic_ullong *p;

        if ((r = rjs_to_big_int(rt, value, bi)) == RJS_ERR)
            goto end;

        if ((r = rjs_big_int_to_uint64(rt, bi, &i)) == RJS_ERR)
            goto end;

        p = (atomic_ullong*)ptr;

        i = atomic_exchange(p, i);

        rjs_big_int_from_uint64(rt, rv, i);
        break;
    }
#endif /*ENABLE_BIG_INT*/
    default:
        assert(0);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Atomics.isLockFree*/
static RJS_NF(Atomics_isLockFree)
{
    RJS_Value *size = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Bool   b;
    RJS_Result r;

    if ((r = rjs_to_integer_or_infinity(rt, size, &n)) == RJS_ERR)
        goto end;

    if ((n == 1) || (n == 2) || (n == 4) || (n == 8))
        b = RJS_TRUE;
    else
        b = RJS_FALSE;

    rjs_value_set_boolean(rt, rv, b);
    r = RJS_OK;
end:
    return r;
}

/*Atomics.load*/
static RJS_NF(Atomics_load)
{
    RJS_Value            *ta      = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *index   = rjs_argument_get(rt, args, argc, 1);
    size_t                idx_pos = 0;
    RJS_IntIndexedObject *iio;
    RJS_Result            r;

    if ((r = validate_integer_typed_array(rt, ta, RJS_FALSE)) == RJS_ERR)
        goto end;

    if ((r = validate_atomic_access(rt, ta, index, &idx_pos)) == RJS_ERR)
        goto end;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, ta);

    if (rjs_is_detached_buffer(rt, &iio->buffer)) {
        r = rjs_throw_type_error(rt, _("the array buffer is detached"));
        goto end;
    }

    r = rjs_get_value_from_buffer(rt, &iio->buffer, idx_pos, iio->type, rjs_is_little_endian(), rv);
end:
    return r;
}

/*Atomics.or*/
static RJS_NF(Atomics_or)
{
    RJS_Value            *ta      = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *index   = rjs_argument_get(rt, args, argc, 1);
    RJS_Value            *value   = rjs_argument_get(rt, args, argc, 2);
    size_t                idx_pos = 0;
    size_t                top     = rjs_value_stack_save(rt);
#if ENABLE_BIG_INT
    RJS_Value            *bi      = rjs_value_stack_push(rt);
#endif /*ENABLE_BIG_INT*/
    RJS_IntIndexedObject *iio;
    RJS_ArrayBuffer      *ab;
    uint8_t              *ptr;
    RJS_Result            r;

    if ((r = validate_integer_typed_array(rt, ta, RJS_FALSE)) == RJS_ERR)
        goto end;

    if ((r = validate_atomic_access(rt, ta, index, &idx_pos)) == RJS_ERR)
        goto end;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, ta);
    ab  = (RJS_ArrayBuffer*)rjs_value_get_object(rt, &iio->buffer);
    ptr = rjs_data_block_get_buffer(ab->data_block) + idx_pos;

    switch (iio->type) {
    case RJS_ARRAY_ELEMENT_INT8: {
        int8_t       i;
        atomic_char *p;

        if ((r = rjs_to_int8(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_char*)ptr;

        i = atomic_fetch_or(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_INT16: {
        int16_t       i;
        atomic_short *p;

        if ((r = rjs_to_int16(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_short*)ptr;

        i = atomic_fetch_or(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_INT32: {
        int32_t     i;
        atomic_int *p;

        if ((r = rjs_to_int32(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_int*)ptr;

        i = atomic_fetch_or(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT8: {
        uint8_t       i;
        atomic_uchar *p;

        if ((r = rjs_to_uint8(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_uchar*)ptr;

        i = atomic_fetch_or(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT16: {
        uint16_t       i;
        atomic_ushort *p;

        if ((r = rjs_to_uint16(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_ushort*)ptr;

        i = atomic_fetch_or(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT32: {
        uint32_t     i;
        atomic_uint *p;

        if ((r = rjs_to_uint32(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_uint*)ptr;

        i = atomic_fetch_or(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
#if ENABLE_BIG_INT
    case RJS_ARRAY_ELEMENT_BIGINT64: {
        int64_t       i;
        atomic_llong *p;

        if ((r = rjs_to_big_int(rt, value, bi)) == RJS_ERR)
            goto end;

        if ((r = rjs_big_int_to_int64(rt, bi, &i)) == RJS_ERR)
            goto end;

        p = (atomic_llong*)ptr;

        i = atomic_fetch_or(p, i);

        rjs_big_int_from_int64(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_BIGUINT64: {
        uint64_t       i;
        atomic_ullong *p;

        if ((r = rjs_to_big_int(rt, value, bi)) == RJS_ERR)
            goto end;

        if ((r = rjs_big_int_to_uint64(rt, bi, &i)) == RJS_ERR)
            goto end;

        p = (atomic_ullong*)ptr;

        i = atomic_fetch_or(p, i);

        rjs_big_int_from_uint64(rt, rv, i);
        break;
    }
#endif /*ENABLE_BIG_INT*/
    default:
        assert(0);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Atomics.store*/
static RJS_NF(Atomics_store)
{
    RJS_Value            *ta      = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *index   = rjs_argument_get(rt, args, argc, 1);
    RJS_Value            *value   = rjs_argument_get(rt, args, argc, 2);
    size_t                idx_pos = 0;
    RJS_IntIndexedObject *iio;
    RJS_Result            r;

    if ((r = validate_integer_typed_array(rt, ta, RJS_FALSE)) == RJS_ERR)
        goto end;

    if ((r = validate_atomic_access(rt, ta, index, &idx_pos)) == RJS_ERR)
        goto end;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, ta);

#if ENABLE_BIG_INT
    if ((iio->type == RJS_ARRAY_ELEMENT_BIGINT64) || (iio->type == RJS_ARRAY_ELEMENT_BIGUINT64)) {
        if ((r = rjs_to_big_int(rt, value, rv)) == RJS_ERR)
            goto end;
    } else
#endif /*ENABLE_BIG_INT*/
    {
        RJS_Number n;

        if ((r = rjs_to_integer_or_infinity(rt, value, &n)) == RJS_ERR)
            goto end;

        rjs_value_set_number(rt, rv, n);
    }

    if (rjs_is_detached_buffer(rt, &iio->buffer)) {
        r = rjs_throw_type_error(rt, _("the array buffer is detached"));
        goto end;
    }

    r = rjs_set_value_in_buffer(rt, &iio->buffer, idx_pos, iio->type, rv, rjs_is_little_endian());
end:
    return r;
}

/*Atomics.sub*/
static RJS_NF(Atomics_sub)
{
    RJS_Value            *ta      = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *index   = rjs_argument_get(rt, args, argc, 1);
    RJS_Value            *value   = rjs_argument_get(rt, args, argc, 2);
    size_t                idx_pos = 0;
    size_t                top     = rjs_value_stack_save(rt);
#if ENABLE_BIG_INT
    RJS_Value            *bi      = rjs_value_stack_push(rt);
#endif /*ENABLE_BIG_INT*/
    RJS_IntIndexedObject *iio;
    RJS_ArrayBuffer      *ab;
    uint8_t              *ptr;
    RJS_Result            r;

    if ((r = validate_integer_typed_array(rt, ta, RJS_FALSE)) == RJS_ERR)
        goto end;

    if ((r = validate_atomic_access(rt, ta, index, &idx_pos)) == RJS_ERR)
        goto end;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, ta);
    ab  = (RJS_ArrayBuffer*)rjs_value_get_object(rt, &iio->buffer);
    ptr = rjs_data_block_get_buffer(ab->data_block) + idx_pos;

    switch (iio->type) {
    case RJS_ARRAY_ELEMENT_INT8: {
        int8_t       i;
        atomic_char *p;

        if ((r = rjs_to_int8(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_char*)ptr;

        i = atomic_fetch_sub(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_INT16: {
        int16_t       i;
        atomic_short *p;

        if ((r = rjs_to_int16(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_short*)ptr;

        i = atomic_fetch_sub(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_INT32: {
        int32_t     i;
        atomic_int *p;

        if ((r = rjs_to_int32(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_int*)ptr;

        i = atomic_fetch_sub(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT8: {
        uint8_t       i;
        atomic_uchar *p;

        if ((r = rjs_to_uint8(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_uchar*)ptr;

        i = atomic_fetch_sub(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT16: {
        uint16_t       i;
        atomic_ushort *p;

        if ((r = rjs_to_uint16(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_ushort*)ptr;

        i = atomic_fetch_sub(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT32: {
        uint32_t     i;
        atomic_uint *p;

        if ((r = rjs_to_uint32(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_uint*)ptr;

        i = atomic_fetch_sub(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
#if ENABLE_BIG_INT
    case RJS_ARRAY_ELEMENT_BIGINT64: {
        int64_t       i;
        atomic_llong *p;

        if ((r = rjs_to_big_int(rt, value, bi)) == RJS_ERR)
            goto end;

        if ((r = rjs_big_int_to_int64(rt, bi, &i)) == RJS_ERR)
            goto end;

        p = (atomic_llong*)ptr;

        i = atomic_fetch_sub(p, i);

        rjs_big_int_from_int64(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_BIGUINT64: {
        uint64_t       i;
        atomic_ullong *p;

        if ((r = rjs_to_big_int(rt, value, bi)) == RJS_ERR)
            goto end;

        if ((r = rjs_big_int_to_uint64(rt, bi, &i)) == RJS_ERR)
            goto end;

        p = (atomic_ullong*)ptr;

        i = atomic_fetch_sub(p, i);

        rjs_big_int_from_uint64(rt, rv, i);
        break;
    }
#endif /*ENABLE_BIG_INT*/
    default:
        assert(0);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Atomics.wait*/
static RJS_NF(Atomics_wait)
{
    RJS_Value            *ta      = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *index   = rjs_argument_get(rt, args, argc, 1);
    RJS_Value            *value   = rjs_argument_get(rt, args, argc, 2);
    RJS_Value            *timeout = rjs_argument_get(rt, args, argc, 3);
    size_t                top     = rjs_value_stack_save(rt);
    RJS_Value            *v       = rjs_value_stack_push(rt);
    RJS_Value            *w       = rjs_value_stack_push(rt);
    RJS_Value            *buf;
    RJS_IntIndexedObject *iio;
    RJS_ArrayBuffer      *ab;
    uint8_t              *ptr;
    size_t                idx_pos;
    RJS_WaiterList       *wl;
    RJS_Number            t;
    RJS_Result            r;

    if ((r = validate_integer_typed_array(rt, ta, RJS_TRUE)) == RJS_ERR)
        goto end;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, ta);
    buf = &iio->buffer;

    if (!rjs_is_shared_array_buffer(rt, buf)) {
        r = rjs_throw_type_error(rt, _("the array buffer is not shared"));
        goto end;
    }

    if ((r = validate_atomic_access(rt, ta, index, &idx_pos)) == RJS_ERR)
        goto end;

#if ENABLE_BIG_INT
    if (iio->type == RJS_ARRAY_ELEMENT_BIGINT64) {
        if ((r = rjs_to_big_int(rt, value, v)) == RJS_ERR)
            goto end;
    } else
#endif /*ENABLE_BIG_INT*/
    {
        int32_t n;

        if ((r = rjs_to_int32(rt, value, &n)) == RJS_ERR)
            goto end;

        rjs_value_set_number(rt, v, n);
    }

    if ((r = rjs_to_number(rt, timeout, &t)) == RJS_ERR)
        goto end;

    if (isnan(t) || (t == INFINITY))
        t = INFINITY;
    else if (t == -INFINITY)
        t = 0;
    else
        t = RJS_MAX(t, 0);

    if (!rt->agent_can_block) {
        r = rjs_throw_type_error(rt, _("the agent cannot be blocked"));
        goto end;
    }

    ab  = (RJS_ArrayBuffer*)rjs_value_get_object(rt, buf);
    ptr = rjs_data_block_get_buffer(ab->data_block) + idx_pos;

    rjs_array_buffer_lock(rt, buf);

    rjs_get_value_from_raw(rt, ptr, iio->type, rjs_is_little_endian(), w);

    if (rjs_same_value(rt, v, w)) {
        wl = rjs_get_waiter_list(rt, ab->data_block, idx_pos);

        r = rjs_add_waiter(rt, ab->data_block, wl, t);
    } else {
        r = RJS_ERR;
    }

    rjs_array_buffer_unlock(rt, buf);

    if (r == RJS_ERR)
        rjs_string_from_chars(rt, rv, "not-equal", -1);
    else if (r)
        rjs_string_from_chars(rt, rv, "ok", -1);
    else
        rjs_string_from_chars(rt, rv, "timed-out", -1);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Atomics.notify*/
static RJS_NF(Atomics_notify)
{
    RJS_Value            *ta      = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *index   = rjs_argument_get(rt, args, argc, 1);
    RJS_Value            *count   = rjs_argument_get(rt, args, argc, 2);
    RJS_Value            *buf;
    RJS_IntIndexedObject *iio;
    size_t                idx_pos;
    RJS_WaiterList       *wl;
    RJS_Number            c;
    size_t                n;
    RJS_Result            r;

    if ((r = validate_integer_typed_array(rt, ta, RJS_TRUE)) == RJS_ERR)
        goto end;

    if ((r = validate_atomic_access(rt, ta, index, &idx_pos)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, count)) {
        c = INFINITY;
    } else {
        if ((r = rjs_to_integer_or_infinity(rt, count, &c)) == RJS_ERR)
            goto end;

        c = RJS_MAX(0, c);
    }

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, ta);
    buf = &iio->buffer;

    if (!rjs_is_shared_array_buffer(rt, buf)) {
        n = 0;
    } else {
        RJS_ArrayBuffer *ab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, buf);

        rjs_array_buffer_lock(rt, buf);

        wl = rjs_get_waiter_list(rt, ab->data_block, idx_pos);

        n = 0;

        while ((c == INFINITY) || (n < c)) {
            RJS_Waiter *w;

            if (rjs_list_is_empty(&wl->waiters))
                break;

            w = RJS_CONTAINER_OF(wl->waiters.next, RJS_Waiter, ln);

            rjs_list_remove(&w->ln);
            rjs_list_init(&w->ln);

            rjs_notify_waiter(rt, w);

            n ++;
        }

        rjs_array_buffer_unlock(rt, buf);
    }

    rjs_value_set_number(rt, rv, n);
    r = RJS_OK;
end:
    return r;
}

/*Atomics.xor*/
static RJS_NF(Atomics_xor)
{
    RJS_Value            *ta      = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *index   = rjs_argument_get(rt, args, argc, 1);
    RJS_Value            *value   = rjs_argument_get(rt, args, argc, 2);
    size_t                idx_pos = 0;
    size_t                top     = rjs_value_stack_save(rt);
#if ENABLE_BIG_INT
    RJS_Value            *bi      = rjs_value_stack_push(rt);
#endif /*ENABLE_BIG_INT*/
    RJS_IntIndexedObject *iio;
    RJS_ArrayBuffer      *ab;
    uint8_t              *ptr;
    RJS_Result            r;

    if ((r = validate_integer_typed_array(rt, ta, RJS_FALSE)) == RJS_ERR)
        goto end;

    if ((r = validate_atomic_access(rt, ta, index, &idx_pos)) == RJS_ERR)
        goto end;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, ta);
    ab  = (RJS_ArrayBuffer*)rjs_value_get_object(rt, &iio->buffer);
    ptr = rjs_data_block_get_buffer(ab->data_block) + idx_pos;

    switch (iio->type) {
    case RJS_ARRAY_ELEMENT_INT8: {
        int8_t       i;
        atomic_char *p;

        if ((r = rjs_to_int8(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_char*)ptr;

        i = atomic_fetch_xor(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_INT16: {
        int16_t       i;
        atomic_short *p;

        if ((r = rjs_to_int16(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_short*)ptr;

        i = atomic_fetch_xor(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_INT32: {
        int32_t     i;
        atomic_int *p;

        if ((r = rjs_to_int32(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_int*)ptr;

        i = atomic_fetch_xor(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT8: {
        uint8_t       i;
        atomic_uchar *p;

        if ((r = rjs_to_uint8(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_uchar*)ptr;

        i = atomic_fetch_xor(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT16: {
        uint16_t       i;
        atomic_ushort *p;

        if ((r = rjs_to_uint16(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_ushort*)ptr;

        i = atomic_fetch_xor(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT32: {
        uint32_t     i;
        atomic_uint *p;

        if ((r = rjs_to_uint32(rt, value, &i)) == RJS_ERR)
            goto end;

        p = (atomic_uint*)ptr;

        i = atomic_fetch_xor(p, i);

        rjs_value_set_number(rt, rv, i);
        break;
    }
#if ENABLE_BIG_INT
    case RJS_ARRAY_ELEMENT_BIGINT64: {
        int64_t       i;
        atomic_llong *p;

        if ((r = rjs_to_big_int(rt, value, bi)) == RJS_ERR)
            goto end;

        if ((r = rjs_big_int_to_int64(rt, bi, &i)) == RJS_ERR)
            goto end;

        p = (atomic_llong*)ptr;

        i = atomic_fetch_xor(p, i);

        rjs_big_int_from_int64(rt, rv, i);
        break;
    }
    case RJS_ARRAY_ELEMENT_BIGUINT64: {
        uint64_t       i;
        atomic_ullong *p;

        if ((r = rjs_to_big_int(rt, value, bi)) == RJS_ERR)
            goto end;

        if ((r = rjs_big_int_to_uint64(rt, bi, &i)) == RJS_ERR)
            goto end;

        p = (atomic_ullong*)ptr;

        i = atomic_fetch_xor(p, i);

        rjs_big_int_from_uint64(rt, rv, i);
        break;
    }
#endif /*ENABLE_BIG_INT*/
    default:
        assert(0);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
atomics_function_descs[] = {
    {
        "add",
        3,
        Atomics_add
    },
    {
        "and",
        3,
        Atomics_and
    },
    {
        "compareExchange",
        4,
        Atomics_compareExchange
    },
    {
        "exchange",
        3,
        Atomics_exchange
    },
    {
        "isLockFree",
        1,
        Atomics_isLockFree
    },
    {
        "load",
        2,
        Atomics_load
    },
    {
        "or",
        3,
        Atomics_or
    },
    {
        "store",
        3,
        Atomics_store
    },
    {
        "sub",
        3,
        Atomics_sub
    },
    {
        "wait",
        4,
        Atomics_wait
    },
    {
        "notify",
        3,
        Atomics_notify
    },
    {
        "xor",
        3,
        Atomics_xor
    },
    {NULL}
};
