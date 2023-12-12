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
 * Typed array.
 */

#ifndef _RJS_TYPED_ARRAY_H_
#define _RJS_TYPED_ARRAY_H_

#ifdef __cplusplus
extern "C" {
#endif

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
extern RJS_Result
rjs_create_typed_array (RJS_Runtime *rt, RJS_ArrayElementType type,
        RJS_Value *args, size_t argc, RJS_Value *ta);

/**
 * Get the element type of the array type.
 * \param rt The current runtime.
 * \param ta The typed array.
 * \return The element type of the array type.
 * \retval -1 The value is not a typed array.
 */
extern RJS_ArrayElementType
rjs_typed_array_get_type (RJS_Runtime *rt, RJS_Value *ta);

/**
 * Get the typed array's buffer pointer.
 * \param rt The current runtime.
 * \param ta The typed array.
 * \return The buffer's pointer.
 */
extern void*
rjs_typed_array_get_buffer (RJS_Runtime *rt, RJS_Value *ta);

/**
 * Get the typed array's array length
 * \param rt The current runtime.
 * \param ta The typed array.
 * \return The length of the typed array.
 */
extern size_t
rjs_typed_array_get_length (RJS_Runtime *rt, RJS_Value *ta);

#ifdef __cplusplus
}
#endif

#endif

