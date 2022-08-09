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
 * Integer indexed object internal header.
 */

#ifndef _RJS_INT_INDEXED_OBJECT_INTERNAL_H_
#define _RJS_INT_INDEXED_OBJECT_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Integer-indexed object.*/
typedef struct {
    RJS_Object           object;       /**< Base object data.*/
    RJS_ArrayElementType type;         /**< Element type.*/
    RJS_Value            buffer;       /**< The array buffer.*/
    size_t               byte_offset;  /**< Start offset in bytes.*/
    size_t               byte_length;  /**< Byte length.*/
    size_t               array_length; /**< The array length.*/
} RJS_IntIndexedObject;

#ifdef __cplusplus
}
#endif

#endif

