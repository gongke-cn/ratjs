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
 * For in iterator internal header.
 */

#ifndef _RJS_FOR_IN_ITERATOR_INTERNAL_H_
#define _RJS_FOR_IN_ITERATOR_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**For in iterator.*/
typedef struct {
    RJS_Object object;   /**< Base object data.*/
    RJS_Value  v;        /**< Current object value.*/
    RJS_Value  keys;     /**< The keys.*/
    size_t     id;       /**< Current key's index.*/
    RJS_Bool   visited;  /**< The object is visited.*/
    RJS_Hash   key_hash; /**< Visited keys' hash table.*/
} RJS_ForInIterator;

/**
 * Create a new for in iterator.
 * \param rt The current runtime.
 * \param[out] iterv Return the iterator value.
 * \param v The object value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_for_in_iterator_new (RJS_Runtime *rt, RJS_Value *iterv, RJS_Value *v);

/**
 * Initialize the for in iterator data in the realm.
 * \param rt The current runtime.
 * \param realm The realm to be initialized.
 */
extern void
rjs_realm_for_in_iterator_init (RJS_Runtime *rt, RJS_Realm *realm);

#ifdef __cplusplus
}
#endif

#endif

