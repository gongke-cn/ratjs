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

/**
 * \file
 * Array buffer.
 */

#ifndef _RJS_ARRAY_BUFFER_H_
#define _RJS_ARRAY_BUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup array_buffer Array buffer
 * Array buffer store the basic data elements
 * @{
 */

/**Array element type.*/
typedef enum {
    RJS_ARRAY_ELEMENT_UINT8,     /**< 8 bits unsigned integer.*/
    RJS_ARRAY_ELEMENT_INT8,      /**< 8 bits singed integer.*/
    RJS_ARRAY_ELEMENT_UINT8C,    /**< 8 bits unsigned integer (clamped conversion).*/
    RJS_ARRAY_ELEMENT_UINT16,    /**< 16 bits unsigned integer.*/
    RJS_ARRAY_ELEMENT_INT16,     /**< 16 bits signed integer.*/
    RJS_ARRAY_ELEMENT_UINT32,    /**< 32 bits unsigned integer.*/
    RJS_ARRAY_ELEMENT_INT32,     /**< 32 bits signed integer.*/
    RJS_ARRAY_ELEMENT_FLOAT32,   /**< 32 bits float point number.*/
    RJS_ARRAY_ELEMENT_FLOAT64,   /**< 64 bits float point number.*/
    RJS_ARRAY_ELEMENT_BIGUINT64, /**< 64 bits unsigned integer.*/
    RJS_ARRAY_ELEMENT_BIGINT64,  /**< 64 bits signed integer.*/
    RJS_ARRAY_ELEMENT_MAX
} RJS_ArrayElementType;

/**
 * Get the element size of an array buffer.
 * \param type The element data type.
 * \return The element's size.  
 */
static inline int
rjs_typed_array_element_size (RJS_ArrayElementType type)
{
    int s;

    switch (type) {
    case RJS_ARRAY_ELEMENT_UINT8:
    case RJS_ARRAY_ELEMENT_INT8:
    case RJS_ARRAY_ELEMENT_UINT8C:
        s = 1;
        break;
    case RJS_ARRAY_ELEMENT_UINT16:
    case RJS_ARRAY_ELEMENT_INT16:
        s = 2;
        break;
    case RJS_ARRAY_ELEMENT_UINT32:
    case RJS_ARRAY_ELEMENT_INT32:
    case RJS_ARRAY_ELEMENT_FLOAT32:
        s = 4;
        break;
    case RJS_ARRAY_ELEMENT_BIGUINT64:
    case RJS_ARRAY_ELEMENT_BIGINT64:
    case RJS_ARRAY_ELEMENT_FLOAT64:
        s = 8;
        break;
    default:
        assert(0);
        s = -1;
    }

    return s;
}

/**
 * Chekc if the value is an array buffer.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE The value is an array buffer.
 * \retval RJS_FALSE The value is not an array buffer.
 */
static inline RJS_Bool
rjs_is_array_buffer (RJS_Runtime *rt, RJS_Value *v)
{
    return rjs_value_get_gc_thing_type(rt, v) == RJS_GC_THING_ARRAY_BUFFER;
}

/**
 * Check if the array buffer is detached.
 * \param rt The current runtime.
 * \param v The array buffer.
 * \retval RJS_TRUE The buffer is detached.
 * \retval RJS_FALSE The buffer is not detached.
 */
extern RJS_Bool
rjs_is_detached_buffer (RJS_Runtime *rt, RJS_Value *v);

/**
 * Check if the array buffer is shared.
 * \param rt The current runtime.
 * \param v The array buffer value.
 * \retval RJS_TRUE The array buffer is shared.
 * \retval RJS_FALSE The array buffer is not shared.
 */
extern RJS_Bool
rjs_is_shared_array_buffer (RJS_Runtime *rt, RJS_Value *v);

/**
 * Get the data block of the array buffer.
 * \param rt The current runtime.
 * \param v The array buffer value.
 * \return The data block.
 */
extern RJS_DataBlock*
rjs_array_buffer_get_data_block (RJS_Runtime *rt, RJS_Value *v);

/**
 * Get the size of the array buffer.
 * \param rt The current runtime.
 * \param v The array buffer value.
 * \return The size of the array buffer.
 */
extern size_t
rjs_array_buffer_get_size (RJS_Runtime *rt, RJS_Value *v);

/**
 * Create a new array buffer.
 * \param rt The current runtime.
 * \param c The constructor.
 * \param len The byte length.
 * \param[out] v Return the array buffer value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_allocate_array_buffer (RJS_Runtime *rt, RJS_Value *c, int64_t len, RJS_Value *v);

/**
 * Detach the array buffer.
 * \param rt The current runtime.
 * \param abv The array buffer value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_detach_array_buffer (RJS_Runtime *rt, RJS_Value *abv);

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
extern RJS_Result
rjs_allocate_shared_array_buffer (RJS_Runtime *rt, RJS_Value *c, int64_t len, RJS_DataBlock *db, RJS_Value *v);

/**
 * Lock the data in the shared array buffer.
 * \param rt The current runtime.
 * \param v The array buffer value.
 */
extern void
rjs_array_buffer_lock (RJS_Runtime *rt, RJS_Value *v);

/**
 * Unlock the data in the shared array buffer.
 * \param rt The current runtime.
 * \param v The array buffer value.
 */
extern void
rjs_array_buffer_unlock (RJS_Runtime *rt, RJS_Value *v);

#else /*!ENABLE_SHARED_ARRAY_BUFFER*/

/**
 * Lock the data in the shared array buffer.
 * \param rt The current runtime.
 * \param v The array buffer value.
 */
static inline void
rjs_array_buffer_lock (RJS_Runtime *rt, RJS_Value *v)
{
}

/**
 * Unlock the data in the shared array buffer.
 * \param rt The current runtime.
 * \param v The array buffer value.
 */
static inline void
rjs_array_buffer_unlock (RJS_Runtime *rt, RJS_Value *v)
{
}

#endif /*ENABLE_SHARED_ARRAY_BUFFER*/

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
extern RJS_Result
rjs_get_value_from_buffer (RJS_Runtime *rt, RJS_Value *abv, size_t byte_idx,
        RJS_ArrayElementType type, RJS_Bool is_little_endian, RJS_Value *v);

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
extern RJS_Result
rjs_set_value_in_buffer (RJS_Runtime *rt, RJS_Value *abv, size_t byte_idx,
        RJS_ArrayElementType type, RJS_Value *v, RJS_Bool is_little_endian);

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
extern RJS_Result
rjs_clone_array_buffer (RJS_Runtime *rt, RJS_Value *src, size_t byte_off, size_t len, RJS_Value *rv);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

