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
 * Array buffer internal header.
 */

#ifndef _RJS_ARRAY_BUFFER_INTERNAL_H_
#define _RJS_ARRAY_BUFFER_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Array buffer.*/
typedef struct {
    RJS_Object      object;      /**< Base object data.*/
    RJS_DataBlock  *data_block;  /**< Data block.*/
    size_t          byte_length; /**< Byte length of the buffer.*/
} RJS_ArrayBuffer;

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
extern RJS_Result
rjs_get_value_from_raw (RJS_Runtime *rt, const uint8_t *b,
        RJS_ArrayElementType type, RJS_Bool is_little_endian, RJS_Value *v);

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
extern RJS_Result
rjs_set_value_in_raw (RJS_Runtime *rt, uint8_t *b,
        RJS_ArrayElementType type, RJS_Value *v, RJS_Bool is_little_endian);

#ifdef __cplusplus
}
#endif

#endif

