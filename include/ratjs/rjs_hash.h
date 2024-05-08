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
 * Hash table.
 */

#ifndef _RJS_HASH_H_
#define _RJS_HASH_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup hash Hash table
 * Hash table
 * @{
 */

/**
 * Get the entries number in the hash table.
 * \param hash The hash table.
 * \return The entries number.
 */
static inline size_t
rjs_hash_get_size (RJS_Hash *hash)
{
    return hash->entry_num;
}

/**
 * Initialize the hash table.
 * \param hash The hash table to be initialized.
 */
static inline void
rjs_hash_init (RJS_Hash *hash)
{
    hash->e.list    = NULL;
    hash->entry_num = 0;
    hash->list_num  = 1;
}

/**
 * Release the resource in the hash table.
 * \param hash The hash table to be released.
 * \param ops The operation functions.
 * \param data User defined data used in ops.
 */
extern void
rjs_hash_deinit (RJS_Hash *hash, const RJS_HashOps *ops, void *data);

/**
 * Lookup an entry in the hash table by its key.
 * \param hash The hash table.
 * \param key The key of the hahs table.
 * \param[out] e Return the entry if the entry found.
 * \param[out] pe Return the position where the entry's pointer should be stored.
 * \param ops The hash table's operation functions.
 * \param data User defined data used in ops.
 * \retval RJS_TRUE Find the entry in the hash table.
 * \retval RJS_FALSE Cannot find the entry in the hash table.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_hash_lookup (RJS_Hash *hash, void *key, RJS_HashEntry **e,
        RJS_HashEntry ***pe, const RJS_HashOps *ops, void *data);

/**
 * Insert an entry to the hash table.
 * \param hash The hash table.
 * \param key The key of the entry.
 * \param e The new entry to be added.
 * \param pe The position where the entry's pointer should be stored.
 * \param ops The hash table's operation functions.
 * \param data User defined data used in ops.
 */
extern void
rjs_hash_insert (RJS_Hash *hash, void *key, RJS_HashEntry *e,
        RJS_HashEntry **pe, const RJS_HashOps *ops, void *data);

/**
 * Remove an entry from the hash table.
 * \param hash The hash table.
 * \param pe The position where the entry's pointer stored.
 * \param data User defined data used in ops.
 */
extern void
rjs_hash_remove (RJS_Hash *hash, RJS_HashEntry **pe, void *data);

/**
 * Realloc function used in hash table operation functions.
 * \param data The current runtime.
 * \param optr The old buffer's pointer.
 * \param osize The old buffer's size.
 * \param nsize The new buffer's size.
 * \return The new buffer's size.
 */
extern void*
rjs_hash_op_realloc (void *data, void *optr, size_t osize, size_t nsize);

/**
 * Integer type hash key function.
 * \param data Not used.
 * \param key The integer type key.
 * \return The key value.
 */
extern size_t
rjs_hash_op_size_key (void *data, void *key);

/**
 * Integer type hash key equal check function.
 * \param data Not used.
 * \param k1 Key 1.
 * \param k2 Key 2.
 * \retval RJS_TRUE k1 == k2.
 * \retval RJS_FALSE k1 != k2.
 */
extern RJS_Bool
rjs_hash_op_size_equal (void *data, void *k1, void *k2);

/**
 * Integer type key hash table operation functions.
 */
extern const RJS_HashOps
rjs_hash_size_ops;

/**
 * 0 terminated characters buffer key hash table operation functions.
 */
extern const RJS_HashOps
rjs_hash_char_star_ops;

/**
 * Value key hash operation functions.
 */
extern const RJS_HashOps
rjs_hash_value_ops;

/**
 * Value key hash operation functions (+0 == -0).
 */
extern const RJS_HashOps
rjs_hash_value_ops_0;

/**String key type hash table operation functions.*/
extern const RJS_HashOps
rjs_hash_string_ops;

/**Traverse the entries of the hash table.*/
#define rjs_hash_foreach(_h, _i, _e)\
    for ((_i) = 0; (_i) < (_h)->list_num; (_i) ++)\
        for ((_e) = ((_h)->list_num == 1) ? (_h)->e.list : (_h)->e.lists[_i];\
                (_e);\
                (_e) = (_e)->next)

/**Traverse the entries' container pointers of the hash table.*/
#define rjs_hash_foreach_c(_h, _i, _e, _s, _m)\
    for ((_i) = 0; (_i) < (_h)->list_num; (_i) ++)\
        for ((_e) = ((_h)->list_num == 1)\
                ? ((_h)->e.list ? RJS_CONTAINER_OF((_h)->e.list, _s, _m) : NULL)\
                : ((_h)->e.lists[_i] ? RJS_CONTAINER_OF((_h)->e.lists[_i], _s, _m) : NULL);\
                (_e);\
                (_e) = (_e)->_m.next ? RJS_CONTAINER_OF((_e)->_m.next, _s, _m) : NULL)

/**Traverse the entries of the hash table safely.*/
#define rjs_hash_foreach_safe(_h, _i, _e, _t)\
    for ((_i) = 0; (_i) < (_h)->list_num; (_i) ++)\
        for ((_e) = ((_h)->list_num == 1) ? (_h)->e.list : (_h)->e.lists[_i];\
                (_e) ? (((_t) = (_e)->next) || 1) : 0;\
                (_e) = (_t))

/**Traverse the entries' container pointers of the hash table safely.*/
#define rjs_hash_foreach_safe_c(_h, _i, _e, _t, _s, _m)\
    for ((_i) = 0; (_i) < (_h)->list_num; (_i) ++)\
        for ((_e) = ((_h)->list_num == 1)\
                ? ((_h)->e.list ? RJS_CONTAINER_OF((_h)->e.list, _s, _m) : NULL)\
                : ((_h)->e.lists[_i] ? RJS_CONTAINER_OF((_h)->e.lists[_i], _s, _m) : NULL);\
                (_e) ? (((_t) = (_e)->_m.next ? RJS_CONTAINER_OF((_e)->_m.next, _s, _m) : NULL) || 1) : 0;\
                (_e) = (_t))

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

