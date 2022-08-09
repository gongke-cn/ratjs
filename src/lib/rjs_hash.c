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

/*Expand the list buffer size in the hash table.*/
static void
hash_expand (RJS_Hash *hash, const RJS_HashOps *ops, void *data)
{
    RJS_HashEntry **nb;
    size_t          i;
    size_t          ns = RJS_MAX(hash->entry_num, 8);

    nb = ops->realloc(data, NULL, 0, sizeof(RJS_HashEntry*) * ns);
    assert(nb);

    memset(nb, 0, sizeof(RJS_HashEntry*) * ns);

    for (i = 0; i < hash->list_num; i ++) {
        RJS_HashEntry *e, *ne;
        size_t         p;

        e = hash->lists[i];

        while (e) {
            ne = e->next;

            p = ops->key(data, e->key) % ns;

            e->next = nb[p];
            nb[p]   = e;

            e = ne;
        }
    }

    if (hash->lists)
        ops->realloc(data, hash->lists, sizeof(RJS_HashEntry*) * hash->list_num, 0);

    hash->lists    = nb;
    hash->list_num = ns;
}

/**
 * Release the resource in the hash table.
 * \param hash The hash table to be released.
 * \param ops The operation functions.
 * \param data User defined data used in ops.
 */
void
rjs_hash_deinit (RJS_Hash *hash, const RJS_HashOps *ops, void *data)
{
    if (hash->lists) {
        ops->realloc(data, hash->lists, sizeof(RJS_HashEntry*) * hash->list_num, 0);
    }
}

/**
 * Lookup an entry in the hash table by its key.
 * \param hash The hash table.
 * \param key The key of the hahs table.
 * \param[out] re Return the entry if the entry found.
 * \param[out] rpe Return the position where the entry's pointer should be stored.
 * \param ops The hash table's operation functions.
 * \param data User defined data used in ops.
 * \retval RJS_TRUE Find the entry in the hash table.
 * \retval RJS_FALSE Cannot find the entry in the hash table.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_hash_lookup (RJS_Hash *hash, void *key, RJS_HashEntry **re,
        RJS_HashEntry ***rpe, const RJS_HashOps *ops, void *data)
{
    RJS_HashEntry *e, **pe;
    RJS_Result     r = RJS_FALSE;

    if (!hash->entry_num) {
        e  = NULL;
        pe = NULL;
    } else {
        size_t kv = ops->key(data, key);

        pe = &hash->lists[kv % hash->list_num];
        while ((e = *pe)) {
            if (ops->equal(data, e->key, key)) {
                r = RJS_TRUE;
                break;
            }

            pe = &e->next;
        }
    }

    if (re)
        *re = e;
    if (rpe)
        *rpe = pe;

    return r;
}

/**
 * Insert an entry to the hash table.
 * \param hash The hash table.
 * \param key The key of the entry.
 * \param e The new entry to be added.
 * \param pe The position where the entry's pointer should be stored.
 * \param ops The hash table's operation functions.
 * \param data User defined data used in ops.
 */
void
rjs_hash_insert (RJS_Hash *hash, void *key, RJS_HashEntry *e,
        RJS_HashEntry **pe, const RJS_HashOps *ops, void *data)
{
    if (hash->entry_num >= hash->list_num * 3) {
        hash_expand(hash, ops, data);
        pe = NULL;
    }

    if (!pe) {
        size_t p = ops->key(data, key) % hash->list_num;

        pe = &hash->lists[p];
    }

    e->key  = key;
    e->next = *pe;
    *pe = e;

    hash->entry_num ++;
}

/**
 * Remove an entry from the hash table.
 * \param hash The hash table.
 * \param pe The position where the entry's pointer stored.
 * \param data User defined data used in ops.
 */
void
rjs_hash_remove (RJS_Hash *hash, RJS_HashEntry **pe, void *data)
{
    RJS_HashEntry *e;

    assert(pe);

    e = *pe;

    *pe = e->next;

    hash->entry_num --;
}

/**
 * Realloc function used in hash table operation functions.
 * \param data The current runtime.
 * \param oper The old buffer's pointer.
 * \param osize The old buffer's size.
 * \param nsize The new buffer's size.
 * \return The new buffer's size.
 */
void*
rjs_hash_op_realloc (void *data, void *optr, size_t osize, size_t nsize)
{
    RJS_Runtime *rt = data;

    return rjs_realloc_assert(rt, optr, osize, nsize);
}

/**
 * Integer type hash key function.
 * \param data Not used.
 * \param key The integer type key.
 * \return The key value.
 */
size_t
rjs_hash_op_size_key (void *data, void *key)
{
    return RJS_PTR2SIZE(key);
}

/**
 * Integer type hash key equal check function.
 * \param data Not used.
 * \param k1 Key 1.
 * \param k2 Key 2.
 * \retval RJS_TRUE k1 == k2.
 * \retval RJS_FALSE k1 != k2.
 */
RJS_Bool
rjs_hash_op_size_equal (void *data, void *k1, void *k2)
{
    return (k1 == k2);
}

/**
 * Integer type key hash table operation functions.
 */
const RJS_HashOps
rjs_hash_size_ops = {
    rjs_hash_op_realloc,
    rjs_hash_op_size_key,
    rjs_hash_op_size_equal
};

/*Get the key value of the 0 terminated characters.*/
static size_t
char_star_op_key (void *data, void *key)
{
    char  *c = key;
    size_t v;

    if (!c)
        return 0;

    v = 0x19820810;
    while (*c) {
        v = (v << 5) | *c;
        c ++;
    }

    return v;
}

/*Check if 2 0 terminated characters buffer are equal.*/
static RJS_Bool
char_star_op_equal (void *data, void *k1, void *k2)
{
    return strcmp(k1, k2) == 0;
}

/**
 * 0 terminated characters buffer key hash table operation functions.
 */
const RJS_HashOps
rjs_hash_char_star_ops = {
    rjs_hash_op_realloc,
    char_star_op_key,
    char_star_op_equal
};

/*Calculate the key of the value.*/
static size_t
hash_op_value_key (void *data, void *key)
{
    RJS_Runtime *rt = data;
    RJS_Value *v     = key;
    size_t     r;

    switch (rjs_value_get_type(rt, v)) {
    case RJS_VALUE_NULL:
    case RJS_VALUE_UNDEFINED:
        r = 0;
        break;
    case RJS_VALUE_BOOLEAN:
        r = rjs_value_get_boolean(rt, v);
        break;
    case RJS_VALUE_NUMBER:
        r = rjs_value_get_number(rt, v);
        break;
    case RJS_VALUE_STRING:
        r = rjs_string_hash_key(rt, v);
        break;
    default:
        r = RJS_PTR2SIZE(rjs_value_get_gc_thing(rt, v));
        break;
    }

    return r;
}

/*Check if 2 values are equal.*/
static RJS_Bool
hash_op_value_equal (void *data, void *k1, void *k2)
{
    RJS_Runtime *rt = data;
    RJS_Value   *v1 = k1;
    RJS_Value   *v2 = k2;

    return rjs_same_value(rt, v1, v2);
}

/*Check if 2 values are equal (+0 == -0).*/
static RJS_Bool
hash_op_value_equal_0 (void *data, void *k1, void *k2)
{
    RJS_Runtime *rt = data;
    RJS_Value   *v1 = k1;
    RJS_Value   *v2 = k2;

    return rjs_same_value_0(rt, v1, v2);
}

/**Value key hash operation functions.*/
const RJS_HashOps
rjs_hash_value_ops = {
    rjs_hash_op_realloc,
    hash_op_value_key,
    hash_op_value_equal
};

/**Value key hash operation functions (+0 == -0).*/
const RJS_HashOps
rjs_hash_value_ops_0 = {
    rjs_hash_op_realloc,
    hash_op_value_key,
    hash_op_value_equal_0
};
