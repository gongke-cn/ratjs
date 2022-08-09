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
 * Integer indexed object.
 */

#ifndef _RJS_INT_INDEXED_OBJECT_H_
#define _RJS_INT_INDEXED_OBJECT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup iio Integer indexed object
 * The object managed elements by integer index
 * @{
 */

/**
 * Check if the number is a valid integer index.
 * \param rt The current runtime.
 * \param o The integer index object.
 * \param n The number value.
 * \param[out] pidx Return the index value.
 * \retval RJS_TRUE The value is a valid index.
 * \retval RJS_FALSE The value is not a valid index.
 */
extern RJS_Bool
rjs_is_valid_int_index (RJS_Runtime *rt, RJS_Value *o, RJS_Number n, size_t *pidx);

/**
 * Create a new integer indexed object.
 * \param rt The current runtime.
 * \param proto The prototype.
 * \param[out] v Return the new object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_int_indexed_object_create (RJS_Runtime *rt, RJS_Value *proto, RJS_Value *v);

/**
 * Get the integer indexed element.
 * \param rt The current runtime.
 * \param o The integer indexed object.
 * \param n The index number.
 * \param[out] v Return the element value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_int_indexed_element_get (RJS_Runtime *rt, RJS_Value *o, RJS_Number n, RJS_Value *v);

/**
 * Set the integer indexed element.
 * \param rt The current runtime.
 * \param o The integer indexed object.
 * \param n The index number.
 * \param v The new element value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_int_indexed_element_set (RJS_Runtime *rt, RJS_Value *o, RJS_Number n, RJS_Value *v);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

