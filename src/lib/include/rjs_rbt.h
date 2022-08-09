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
 * Red black tree.
 */

#ifndef _RJS_RBT_H_
#define _RJS_RBT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Red black tree.*/
typedef struct RJS_Rbt_s RJS_Rbt;

/**Red black tree.*/
struct RJS_Rbt_s {
    size_t   parent_color; /**< The parent node and this node's color.*/
    RJS_Rbt *left;         /**< The left child node.*/
    RJS_Rbt *right;        /**< The right child node.*/
};

/**
 * Initialize the red black tree's root node.
 * \param root The root node to be initialized.
 */
static inline void
rjs_rbt_init (RJS_Rbt **root)
{
    *root = NULL;
}

/**
 * Get the parent node in the red black tree.
 * \param n The node.
 * \return Return the parent node.
 */
static inline RJS_Rbt*
rjs_rbt_get_parent (RJS_Rbt *n)
{
    return RJS_SIZE2PTR(n->parent_color & ~3);
}

/**
 * Link a new node to the red black tree.
 * \param n The new node to be added.
 * \param p The parent node.
 * \param pos The position where store the node's pointer.
 */
static inline void
rjs_rbt_link (RJS_Rbt *n, RJS_Rbt *p, RJS_Rbt **pos)
{
    n->parent_color = RJS_PTR2SIZE(p);
    n->left  = NULL;
    n->right = NULL;
    *pos = n;
}

/**
 * Get the first node in the red black tree.
 * \param root The root of the tree.
 * \return The first node in the tree.
 */
extern RJS_Rbt*
rjs_rbt_first (RJS_Rbt **root);

/**
 * Get the next node in the red black tree.
 * \param root The root of the tree.
 * \return The last node in the tree.
 */
extern RJS_Rbt*
rjs_rbt_last (RJS_Rbt **root);

/**
 * Get the previous node in the red black tree.
 * \param n The current node.
 * \return The previous node.
 */
extern RJS_Rbt*
rjs_rbt_prev (RJS_Rbt *n);

/**
 * Get the next node in the red black tree.
 * \param n The current node.
 * \return The next node.
 */
extern RJS_Rbt*
rjs_rbt_next (RJS_Rbt *n);

/**
 * Insert a new node to the red black tree.
 * \param root The root of the tree.
 * \param n The node to be added.
 */
extern void
rjs_rbt_insert (RJS_Rbt **root, RJS_Rbt *n);

/**
 * Remove a node from the red black tree.
 * \param root The root of the tree.
 * \param n The node to be removed.
 */
extern void
rjs_rbt_remove (RJS_Rbt **root, RJS_Rbt *n);

#ifdef __cplusplus
}
#endif

#endif

