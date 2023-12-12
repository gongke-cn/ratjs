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

/*Scan the reference things in the array buffer.*/
static void
array_buffer_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_ArrayBuffer *ab = ptr;

    rjs_object_op_gc_scan(rt, &ab->object);
}

/*Free the array buffer.*/
static void
array_buffer_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_ArrayBuffer *ab = ptr;

    rjs_object_deinit(rt, &ab->object);

    if (ab->data_block)
        rjs_data_block_unref(ab->data_block);

    RJS_DEL(rt, ab);
}

/*Array buffer operation functions.*/
static const RJS_ObjectOps
array_buffer_ops = {
    {
        RJS_GC_THING_ARRAY_BUFFER,
        array_buffer_op_gc_scan,
        array_buffer_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS
};

/**
 * Check if the array buffer is detached.
 * \param rt The current runtime.
 * \param v The array buffer.
 * \retval RJS_TRUE The buffer is detached.
 * \retval RJS_FALSE The buffer is not detached.
 */
RJS_Bool
rjs_is_detached_buffer (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_ArrayBuffer *ab;

    if (rjs_value_is_undefined(rt, v))
        return RJS_FALSE;

    assert(rjs_is_array_buffer(rt, v));

    ab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, v);

    return ab->data_block ? RJS_FALSE : RJS_TRUE;
}

/**
 * Check if the array buffer is shared.
 * \param rt The current runtime.
 * \param v The array buffer value.
 * \retval RJS_TRUE The array buffer is shared.
 * \retval RJS_FALSE The array buffer is not shared.
 */
RJS_Bool
rjs_is_shared_array_buffer (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_ArrayBuffer *ab;

    assert(rjs_is_array_buffer(rt, v));

    ab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, v);

    if (!ab->data_block)
        return RJS_FALSE;

#if ENABLE_SHARED_ARRAY_BUFFER
    return rjs_data_block_is_shared(ab->data_block);
#else /*ENABLE_SHARED_ARRAY_BUFFER*/
    return RJS_FALSE;
#endif /*ENABLE_SHARED_ARRAY_BUFFER*/
}

/**
 * Get the data block of the array buffer.
 * \param rt The current runtime.
 * \param v The array buffer value.
 * \return The data block.
 */
RJS_DataBlock*
rjs_array_buffer_get_data_block (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_ArrayBuffer *ab;

    if (rjs_value_is_undefined(rt, v))
        return NULL;

    assert(rjs_is_array_buffer(rt, v));

    ab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, v);

    return ab->data_block;
}

/**
 * Get the size of the array buffer.
 * \param rt The current runtime.
 * \param v The array buffer value.
 * \return The size of the array buffer.
 */
size_t
rjs_array_buffer_get_size (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_ArrayBuffer *ab;

    if (rjs_value_is_undefined(rt, v))
        return 0;

    assert(rjs_is_array_buffer(rt, v));

    ab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, v);

    return ab->byte_length;
}

/**
 * Create a new array buffer.
 * \param rt The current runtime.
 * \param c The constructor.
 * \param len The byte length.
 * \param[out] v Return the array buffer value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_allocate_array_buffer (RJS_Runtime *rt, RJS_Value *c, int64_t len, RJS_Value *v)
{
    RJS_ArrayBuffer *ab;
    RJS_Result       r;

    RJS_NEW(rt, ab);

    ab->data_block  = NULL;
    ab->byte_length = len;

    r = rjs_ordinary_init_from_constructor(rt, &ab->object, c, RJS_O_ArrayBuffer_prototype, &array_buffer_ops, v);
    if (r == RJS_ERR) {
        RJS_DEL(rt, ab);
        return r;
    }

    ab->data_block = rjs_data_block_new(len, 0);
    if (!ab->data_block)
        return rjs_throw_range_error(rt, _("cannot allocate %"PRIdPTR"B array buffer"), len);

    return RJS_OK;
}

/**
 * Detach the array buffer.
 * \param rt The current runtime.
 * \param abv The array buffer value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_detach_array_buffer (RJS_Runtime *rt, RJS_Value *abv)
{
    RJS_ArrayBuffer *ab;

    assert(rjs_is_array_buffer(rt, abv));

    ab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, abv);

    if (ab->data_block) {
        rjs_data_block_unref(ab->data_block);

        ab->data_block  = NULL;
        ab->byte_length = 0;
    }

    return RJS_OK;
}

#if ENABLE_SHARED_ARRAY_BUFFER

/**
 * Create a new shared array buffer.
 * \param rt The current runtime.
 * \param c The constructor.
 * \param len The byte length.
 * \param db The data block.
 * If db == NULL, a new data block will be allocated.
 * \param[out] v Return the array buffer value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_allocate_shared_array_buffer (RJS_Runtime *rt, RJS_Value *c, int64_t len, RJS_DataBlock *db, RJS_Value *v)
{
    RJS_ArrayBuffer *ab;
    RJS_Result       r;

    RJS_NEW(rt, ab);

    ab->data_block  = NULL;
    ab->byte_length = len;

    r = rjs_ordinary_init_from_constructor(rt, &ab->object, c, RJS_O_SharedArrayBuffer_prototype, &array_buffer_ops, v);
    if (r == RJS_ERR) {
        RJS_DEL(rt, ab);
        return r;
    }

    if (db) {
        ab->data_block = rjs_data_block_ref(db);
    } else {
        ab->data_block = rjs_data_block_new(len, RJS_DATA_BLOCK_FL_SHARED);
        if (!ab->data_block)
            return rjs_throw_range_error(rt, _("cannot allocate %"PRIdPTR"B array buffer"), len);
    }

    return RJS_OK;
}

/**
 * Lock the data in the shared array buffer.
 * \param rt The current runtime.
 * \param v The array buffer value.
 */
void
rjs_array_buffer_lock (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_ArrayBuffer *ab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, v);
    RJS_DataBlock   *db = ab->data_block;

    if (db)
        rjs_data_block_lock(db);
}

/**
 * Unlock the data in the shared array buffer.
 * \param rt The current runtime.
 * \param v The array buffer value.
 */
void
rjs_array_buffer_unlock (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_ArrayBuffer *ab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, v);
    RJS_DataBlock   *db = ab->data_block;

    if (db)
        rjs_data_block_unlock(db);
}

#endif /*ENABLE_SHARED_ARRAY_BUFFER*/

/**
 * Get value from a raw buffer.
 * \param rt The current runtime.
 * \param b The raw buffer.
 * \param type The element type.
 * \param is_little_endian Is little endian.
 * \param[out] v Return the value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_get_value_from_raw (RJS_Runtime *rt, const uint8_t *b,
        RJS_ArrayElementType type, RJS_Bool is_little_endian, RJS_Value *v)
{
    RJS_Number n;

    switch (type) {
    case RJS_ARRAY_ELEMENT_UINT8:
    case RJS_ARRAY_ELEMENT_UINT8C:
        n = *b;
        rjs_value_set_number(rt, v, n);
        break;
    case RJS_ARRAY_ELEMENT_INT8:
        n = *(int8_t*)b;
        rjs_value_set_number(rt, v, n);
        break;
    case RJS_ARRAY_ELEMENT_UINT16: {
        uint16_t u16;

        if (is_little_endian)
            u16 = b[0] | (b[1] << 8);
        else
            u16 = (b[0] << 8) | b[1];

        n = u16;
        rjs_value_set_number(rt, v, n);
        break;
    }
    case RJS_ARRAY_ELEMENT_INT16: {
        int16_t s16;

        if (is_little_endian)
            s16 = b[0] | (b[1] << 8);
        else
            s16 = (b[0] << 8) | b[1];

        n = s16;
        rjs_value_set_number(rt, v, n);
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT32: {
        uint32_t u32;

        if (is_little_endian)
            u32 = b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24);
        else
            u32 = (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];

        n = u32;
        rjs_value_set_number(rt, v, n);
        break;
    }
    case RJS_ARRAY_ELEMENT_INT32: {
        int32_t s32;

        if (is_little_endian)
            s32 = b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24);
        else
            s32 = (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];

        n = s32;
        rjs_value_set_number(rt, v, n);
        break;
    }
#if ENABLE_BIG_INT
    case RJS_ARRAY_ELEMENT_BIGUINT64: {
        uint64_t u64;

        if (is_little_endian)
            u64 = ((uint64_t)b[0]) | ((uint64_t)b[1] << 8)
                    | ((uint64_t)b[2] << 16) | ((uint64_t)b[3] << 24)
                    | ((uint64_t)b[4] << 32) | ((uint64_t)b[5] << 40)
                    | ((uint64_t)b[6] << 48) | ((uint64_t)b[7] << 56);
        else
            u64 = ((uint64_t)b[0] << 56) | ((uint64_t)b[1] << 48)
                    | ((uint64_t)b[2] << 40) | ((uint64_t)b[3] << 32)
                    | ((uint64_t)b[4] << 24) | ((uint64_t)b[5] << 16)
                    | ((uint64_t)b[6] << 8) | ((uint64_t)b[7]);

        rjs_big_int_from_uint64(rt, v, u64);
        break;
    }
    case RJS_ARRAY_ELEMENT_BIGINT64: {
        int64_t s64;

        if (is_little_endian)
            s64 = ((uint64_t)b[0]) | ((uint64_t)b[1] << 8)
                    | ((uint64_t)b[2] << 16) | ((uint64_t)b[3] << 24)
                    | ((uint64_t)b[4] << 32) | ((uint64_t)b[5] << 40)
                    | ((uint64_t)b[6] << 48) | ((uint64_t)b[7] << 56);
        else
            s64 = ((uint64_t)b[0] << 56) | ((uint64_t)b[1] << 48)
                    | ((uint64_t)b[2] << 40) | ((uint64_t)b[3] << 32)
                    | ((uint64_t)b[4] << 24) | ((uint64_t)b[5] << 16)
                    | ((uint64_t)b[6] << 8) | ((uint64_t)b[7]);

        rjs_big_int_from_int64(rt, v, s64);

        int64_t i;

        rjs_big_int_to_int64(rt, v, &i);
        break;
    }
#endif /*ENABLE_BIG_INT*/
    case RJS_ARRAY_ELEMENT_FLOAT32: {
        union {
            float    f;
            uint32_t i;
        } u;

        if (is_little_endian)
            u.i = b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24);
        else
            u.i = (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];

        if (isnan(u.f))
            rjs_value_set_number(rt, v, NAN);
        else
            rjs_value_set_number(rt, v, u.f);
        break;
    }
    case RJS_ARRAY_ELEMENT_FLOAT64: {
        union {
            double   d;
            uint64_t i;
        } u;

        if (is_little_endian)
            u.i = ((uint64_t)b[0]) | ((uint64_t)b[1] << 8)
                    | ((uint64_t)b[2] << 16) | ((uint64_t)b[3] << 24)
                    | ((uint64_t)b[4] << 32) | ((uint64_t)b[5] << 40)
                    | ((uint64_t)b[6] << 48) | ((uint64_t)b[7] << 56);
        else
            u.i = ((uint64_t)b[0] << 56) | ((uint64_t)b[1] << 48)
                    | ((uint64_t)b[2] << 40) | ((uint64_t)b[3] << 32)
                    | ((uint64_t)b[4] << 24) | ((uint64_t)b[5] << 16)
                    | ((uint64_t)b[6] << 8) | ((uint64_t)b[7]);

        if (isnan(u.d))
            rjs_value_set_number(rt, v, NAN);
        else
            rjs_value_set_number(rt, v, u.d);
        break;
    }
    default:
        assert(0);
    }

    return RJS_OK;
}

/**
 * Get value from an array buffer.
 * \param rt The current runtime.
 * \param b The raw buffer.
 * \param type The element type.
 * \param v The value.
 * \param is_little_endian Is little endian.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_set_value_in_raw (RJS_Runtime *rt, uint8_t *b,
        RJS_ArrayElementType type, RJS_Value *v, RJS_Bool is_little_endian)
{
    assert(rjs_value_is_number(rt, v)
#if ENABLE_BIG_INT
            || rjs_value_is_big_int(rt, v)
#endif /*ENABLE_BIG_INT*/
            );
    
    switch (type) {
    case RJS_ARRAY_ELEMENT_UINT8: {
        uint8_t u8 = 0;

        rjs_to_uint8(rt, v, &u8);
        *b = u8;
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT8C: {
        uint8_t u8 = 0;

        rjs_to_uint8_clamp(rt, v, &u8);
        *b = u8;
        break;
    }
    case RJS_ARRAY_ELEMENT_INT8: {
        int8_t s8 = 0;

        rjs_to_int8(rt, v, &s8);
        *b = s8;
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT16: {
        uint16_t u16 = 0;

        rjs_to_uint16(rt, v, &u16);
        if (is_little_endian) {
            b[0] = u16;
            b[1] = u16 >> 8;
        } else {
            b[0] = u16 >> 8;
            b[1] = u16;
        }
        break;
    }
    case RJS_ARRAY_ELEMENT_INT16: {
        int16_t s16 = 0;

        rjs_to_int16(rt, v, &s16);
        if (is_little_endian) {
            b[0] = s16;
            b[1] = s16 >> 8;
        } else {
            b[0] = s16 >> 8;
            b[1] = s16;
        }
        break;
    }
    case RJS_ARRAY_ELEMENT_UINT32: {
        uint32_t u32 = 0;

        rjs_to_uint32(rt, v, &u32);
        if (is_little_endian) {
            b[0] = u32;
            b[1] = u32 >> 8;
            b[2] = u32 >> 16;
            b[3] = u32 >> 24;
        } else {
            b[0] = u32 >> 24;
            b[1] = u32 >> 16;
            b[2] = u32 >> 8;
            b[3] = u32;
        }
        break;
    }
    case RJS_ARRAY_ELEMENT_INT32: {
        int32_t s32 = 0;

        rjs_to_int32(rt, v, &s32);
        if (is_little_endian) {
            b[0] = s32;
            b[1] = s32 >> 8;
            b[2] = s32 >> 16;
            b[3] = s32 >> 24;
        } else {
            b[0] = s32 >> 24;
            b[1] = s32 >> 16;
            b[2] = s32 >> 8;
            b[3] = s32;
        }
        break;
    }
#if ENABLE_BIG_INT
    case RJS_ARRAY_ELEMENT_BIGUINT64: {
        uint64_t u64;

        rjs_big_int_to_uint64(rt, v, &u64);

        if (is_little_endian) {
            b[0] = u64;
            b[1] = u64 >> 8;
            b[2] = u64 >> 16;
            b[3] = u64 >> 24;
            b[4] = u64 >> 32;
            b[5] = u64 >> 40;
            b[6] = u64 >> 48;
            b[7] = u64 >> 56;
        } else {
            b[0] = u64 >> 56;
            b[1] = u64 >> 48;
            b[2] = u64 >> 40;
            b[3] = u64 >> 32;
            b[4] = u64 >> 24;
            b[5] = u64 >> 16;
            b[6] = u64 >> 8;
            b[7] = u64;
        }
        break;
    }
    case RJS_ARRAY_ELEMENT_BIGINT64: {
        int64_t s64;

        rjs_big_int_to_int64(rt, v, &s64);

        if (is_little_endian) {
            b[0] = s64;
            b[1] = s64 >> 8;
            b[2] = s64 >> 16;
            b[3] = s64 >> 24;
            b[4] = s64 >> 32;
            b[5] = s64 >> 40;
            b[6] = s64 >> 48;
            b[7] = s64 >> 56;
        } else {
            b[0] = s64 >> 56;
            b[1] = s64 >> 48;
            b[2] = s64 >> 40;
            b[3] = s64 >> 32;
            b[4] = s64 >> 24;
            b[5] = s64 >> 16;
            b[6] = s64 >> 8;
            b[7] = s64;
        }
        break;
    }
#endif /*ENABLE_BIG_INT*/
    case RJS_ARRAY_ELEMENT_FLOAT32: {
        union {
            float    f;
            uint32_t i;
        } u;

        u.f = rjs_value_get_number(rt, v);

        if (is_little_endian) {
            b[0] = u.i;
            b[1] = u.i >> 8;
            b[2] = u.i >> 16;
            b[3] = u.i >> 24;
        } else {
            b[0] = u.i >> 24;
            b[1] = u.i >> 16;
            b[2] = u.i >> 8;
            b[3] = u.i;
        }
        break;
    }
    case RJS_ARRAY_ELEMENT_FLOAT64: {
        union {
            double   d;
            uint64_t i;
        } u;

        u.d = rjs_value_get_number(rt, v);

        if (is_little_endian) {
            b[0] = u.i;
            b[1] = u.i >> 8;
            b[2] = u.i >> 16;
            b[3] = u.i >> 24;
            b[4] = u.i >> 32;
            b[5] = u.i >> 40;
            b[6] = u.i >> 48;
            b[7] = u.i >> 56;
        } else {
            b[0] = u.i >> 56;
            b[1] = u.i >> 48;
            b[2] = u.i >> 40;
            b[3] = u.i >> 32;
            b[4] = u.i >> 24;
            b[5] = u.i >> 16;
            b[6] = u.i >> 8;
            b[7] = u.i;
        }
        break;
    }
    default:
        assert(0);
    }

    return RJS_OK;
}

/**
 * Get value from an array buffer.
 * \param rt The current runtime.
 * \param abv The array buffer value.
 * \param byte_idx The byte index.
 * \param type The element type.
 * \param is_little_endian Is little endian.
 * \param[out] v Return the value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_get_value_from_buffer (RJS_Runtime *rt, RJS_Value *abv, size_t byte_idx,
        RJS_ArrayElementType type, RJS_Bool is_little_endian, RJS_Value *v)
{
    RJS_ArrayBuffer *ab;
    uint8_t         *b;
    RJS_Result       r;

    assert(!rjs_is_detached_buffer(rt, abv));

    ab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, abv);
    b  = rjs_data_block_get_buffer(ab->data_block) + byte_idx;

    rjs_array_buffer_lock(rt, abv);

    r = rjs_get_value_from_raw(rt, b, type, is_little_endian, v);

    rjs_array_buffer_unlock(rt, abv);

    return r;
}

/**
 * Get value from an array buffer.
 * \param rt The current runtime.
 * \param abv The array buffer value.
 * \param byte_idx The byte index.
 * \param type The element type.
 * \param v The value.
 * \param is_little_endian Is little endian.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_set_value_in_buffer (RJS_Runtime *rt, RJS_Value *abv, size_t byte_idx,
        RJS_ArrayElementType type, RJS_Value *v, RJS_Bool is_little_endian)
{
    RJS_ArrayBuffer *ab;
    uint8_t         *b;
    RJS_Result       r;

    assert(!rjs_is_detached_buffer(rt, abv));

    ab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, abv);
    b  = rjs_data_block_get_buffer(ab->data_block) + byte_idx;

    rjs_array_buffer_lock(rt, abv);

    r = rjs_set_value_in_raw(rt, b, type, v, is_little_endian);

    rjs_array_buffer_unlock(rt, abv);

    return r;
}

/**
 * Clone an array buffer.
 * \param rt The current runtime.
 * \param src The source array buffer.
 * \param byte_off Byte offset in the source buffer.
 * \param len Byte length of the source buffer.
 * \param[out] rv Return the new array buffer.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_clone_array_buffer (RJS_Runtime *rt, RJS_Value *src, size_t byte_off, size_t len, RJS_Value *rv)
{
    RJS_Realm       *realm = rjs_realm_current(rt);
    RJS_ArrayBuffer *sab, *tab;
    uint8_t         *sp, *tp;
    RJS_Result       r;

    if ((r = rjs_allocate_array_buffer(rt, rjs_o_ArrayBuffer(realm), len, rv)) == RJS_ERR)
        return r;

    if (rjs_is_detached_buffer(rt, src))
        return rjs_throw_type_error(rt, _("the array buffer is detached"));

    sab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, src);
    tab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, rv);

    rjs_array_buffer_lock(rt, src);

    sp = rjs_data_block_get_buffer(sab->data_block);
    tp = rjs_data_block_get_buffer(tab->data_block);

    memcpy(tp, sp + byte_off, len);

    rjs_array_buffer_unlock(rt, src);

    return RJS_OK;
}
