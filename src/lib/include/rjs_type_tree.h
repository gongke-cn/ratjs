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
 * Object type tree.
 */

#ifndef _RJS_TYPE_TREE_H_
#define _RJS_TYPE_TREE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Type class.*/
struct RJS_TypeClass_s {
    RJS_HashEntry he;        /**< Hash table entry.*/
    int           proto_num; /**< Prototypes' number.*/
    RJS_List      back_refs; /**< Back references.*/
};

/**Type tree node.*/
typedef struct RJS_TypeTreeNode_s RJS_TypeTreeNode;

/**Type tree node.*/
struct RJS_TypeTreeNode_s {
    RJS_TypeTreeNode *next;      /**< The next node in the list.*/
    RJS_HashEntry he;            /**< Hash table entry.*/
    RJS_Hash      type_hash;     /**< Type hash table.*/
    RJS_Hash      add_node_hash; /**< Nodes hash table of add properties.*/
    RJS_Hash      del_node_hash; /**< Nodes hash table of delete properties.*/
};

/**
 * Initialize the type tree.
 * \param rt The current runtime.
 */
RJS_INTERNAL void
rjs_type_tree_init (RJS_Runtime *rt);

/**
 * Release the type tree.
 * \param rt The current runtime.
 */
RJS_INTERNAL void
rjs_type_tree_deinit (RJS_Runtime *rt);

/**
 * Scan the referenced things in the type tree.
 * \param rt The current runtime.
 */
RJS_INTERNAL void
rjs_gc_scan_type_tree (RJS_Runtime *rt);

/**
 * Reset the object's type information when reset prototype.
 * \param rt The current runtime.
 * \param o The object.
 */
RJS_INTERNAL void
rjs_type_tree_reset_proto (RJS_Runtime *rt, RJS_Object *o);

/**
 * Add an add property node to the type tree.
 * \param rt The current runtime.
 * \param o The object.
 * \param key The key of the property.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_type_tree_add (RJS_Runtime *rt, RJS_Object *o, void *key);

/**
 * Add a delete property node to the type tree.
 * \param rt The current runtime.
 * \param o The object.
 * \param key The key of the property.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_type_tree_del (RJS_Runtime *rt, RJS_Object *o, void *key);

/**
 * Add property cache.
 * \param rt The current runtime.
 * \param pn The property name.
 * \param b The base object.
 * \param p The prototype object.
 * \param prop_idx The property's index.
 */
RJS_INTERNAL void
rjs_type_tree_cache (RJS_Runtime *rt, RJS_PropertyName *pn, RJS_Object *b, RJS_Object *p, int prop_idx);

#ifdef __cplusplus
}
#endif

#endif

