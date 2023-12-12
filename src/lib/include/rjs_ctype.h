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
 * C type internal header.
 */

#ifndef _RJS_CTYPE_INTERNAL_H_
#define _RJS_CTYPE_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**C pointer information.*/
typedef struct {
    RJS_CType   *ctype; /**< The C type of this pointer.*/
    RJS_CPtrType ptype; /**< The pointer type.*/
    size_t       nitem; /**< Number of items in this buffer.*/
    void        *ptr;   /**< The pointer.*/
} RJS_CPtrInfo;

/**
 * Remove the C pointer from the hash table.
 * \param rt The current runtime.
 * \param info The C pointer information.
 */
extern void
rjs_cptr_remove (RJS_Runtime *rt, RJS_CPtrInfo *info);

/**
 * Initialize the C type data in the runtime.
 * \param rt The current runtime.
 */
extern void
rjs_runtime_ctype_init (RJS_Runtime *rt);

/**
 * Release the C type data in the runtime.
 * \param rt The current runtime.
 */
extern void
rjs_runtime_ctype_deinit (RJS_Runtime *rt);

/**
 * Scan the C types in the runtime.
 * \param rt The current runtime.
 */
extern void
rjs_gc_scan_ctype (RJS_Runtime *rt);

#ifdef __cplusplus
}
#endif

#endif

