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

/**Red color.*/
#define RBT_COLOR_RED   0
/**Black color.*/
#define RBT_COLOR_BLACK 1

/*Get the color from the parent color field.*/
static inline int
rbt_pc_get_color (size_t pc)
{
    return pc & 1;
}

/*Get parent node from the parent color field.*/
static inline RJS_Rbt*
rbt_pc_get_parent (size_t pc)
{
    return RJS_SIZE2PTR(pc & ~3);
}

/*Check if the parent color field is red.*/
static inline RJS_Bool
rbt_pc_is_red (size_t pc)
{
    return rbt_pc_get_color(pc) == RBT_COLOR_RED;
}

/*Check if the parent color field is black.*/
static inline RJS_Bool
rbt_pc_is_black (size_t pc)
{
    return rbt_pc_get_color(pc) == RBT_COLOR_BLACK;
}

/*Check if the node is red.*/
static inline RJS_Bool
rbt_is_red (RJS_Rbt *n)
{
    return rbt_pc_is_red(n->parent_color);
}

/*Check if the node is black.*/
static inline RJS_Bool
rbt_is_black (RJS_Rbt *n)
{
    return rbt_pc_is_black(n->parent_color);
}

/*Get parent node from the red node's parent color field.*/
static inline RJS_Rbt*
rbt_red_pc_get_parent (size_t pc)
{
    return RJS_SIZE2PTR(pc);
}

/*Set the node as black.*/
static inline void
rbt_set_black (RJS_Rbt *n)
{
    n->parent_color |= RBT_COLOR_BLACK;
}

/*Set the node from the parent.*/
static inline void
rbt_set_parent (RJS_Rbt *n, RJS_Rbt *p)
{
    n->parent_color = rbt_pc_get_color(n->parent_color) | RJS_PTR2SIZE(p);
}

/*Set the node's parent and color.*/
static inline void
rbt_set_parent_color (RJS_Rbt *n, RJS_Rbt *p, int col)
{
    n->parent_color = RJS_PTR2SIZE(p) | col;
}

/*Change the child of the node.*/
static inline void
rbt_change_child (RJS_Rbt *old, RJS_Rbt *new, RJS_Rbt *p, RJS_Rbt **root)
{
    if (p) {
        if (p->left == old)
            p->left = new;
        else
            p->right = new;
    } else {
        *root = new;
    }
}

/*Rotation change parents.*/
static inline void
rbt_rotate_set_parents (RJS_Rbt *old, RJS_Rbt *new, RJS_Rbt **root, int col)
{
    size_t   pc;
    RJS_Rbt *p;
    
    pc = old->parent_color;
    p  = rbt_pc_get_parent(pc);

    new->parent_color = pc;
    rbt_set_parent_color(old, new, col);
    rbt_change_child(old, new, p, root);
}

/**
 * Get the first node in the red black tree.
 * \param root The root of the tree.
 * \return The first node in the tree.
 */
RJS_Rbt*
rjs_rbt_first (RJS_Rbt **root)
{
    RJS_Rbt *n = *root;

    if (!n)
        return NULL;

    while (n->left)
        n = n->left;

    return n;
}

/**
 * Get the next node in the red black tree.
 * \param root The root of the tree.
 * \return The last node in the tree.
 */
RJS_Rbt*
rjs_rbt_last (RJS_Rbt **root)
{
    RJS_Rbt *n = *root;

    if (!n)
        return NULL;

    while (n->right)
        n = n->right;

    return n;
}

/**
 * Get the previous node in the red black tree.
 * \param n The current node.
 * \return The previous node.
 */
RJS_Rbt*
rjs_rbt_prev (RJS_Rbt *n)
{
    RJS_Rbt *p;

    if (!n)
        return NULL;

    if (n->left) {
        n = n->left;

        while (n->right)
            n = n->right;

        return n;
    }

    while ((p = rbt_pc_get_parent(n->parent_color)) && (p->left == n))
        n = p;

    return p;
}

/**
 * Get the next node in the red black tree.
 * \param n The current node.
 * \return The next node.
 */
RJS_Rbt*
rjs_rbt_next (RJS_Rbt *n)
{
    RJS_Rbt *p;

    if (!n)
        return NULL;

    if (n->right) {
        n = n->right;

        while (n->left)
            n = n->left;

        return n;
    }

    while ((p = rbt_pc_get_parent(n->parent_color)) && (p->right == n))
        n = p;

    return p;
}

/**
 * Insert a new node to the red black tree.
 * \param root The root of the tree.
 * \param n The node to be added.
 */
void
rjs_rbt_insert (RJS_Rbt **root, RJS_Rbt *n)
{
    RJS_Rbt *p, *gp, *t;

    p = rbt_red_pc_get_parent(n->parent_color);

    while (1) {
        if (!p) {
            rbt_set_parent_color(n, NULL, RBT_COLOR_BLACK);
            break;
        }

        if (rbt_pc_is_black(p->parent_color))
            break;

        gp = rbt_red_pc_get_parent(p->parent_color);
        t  = gp->right;

        if (p != t) {
            if (t && rbt_is_red(t)) {
                rbt_set_parent_color(t, gp, RBT_COLOR_BLACK);
                rbt_set_parent_color(p, gp, RBT_COLOR_BLACK);

                n = gp;
                p = rbt_pc_get_parent(n->parent_color);
                rbt_set_parent_color(n, p, RBT_COLOR_RED);
                continue;
            }

            t = p->right;
            if (n == t) {
                t = n->left;
                p->right = t;
                n->left  = p;

                if (t)
                    rbt_set_parent_color(t, p, RBT_COLOR_BLACK);

                rbt_set_parent_color(p, n, RBT_COLOR_RED);

                p = n;
                t = n->right;
            }

            gp->left = t;
            p->right = gp;

            if (t)
                rbt_set_parent_color(t, gp, RBT_COLOR_BLACK);
            rbt_rotate_set_parents(gp, p, root, RBT_COLOR_RED);
            break;
        } else {
            t = gp->left;

            if (t && rbt_is_red(t)) {
                rbt_set_parent_color(t, gp, RBT_COLOR_BLACK);
                rbt_set_parent_color(p, gp, RBT_COLOR_BLACK);

                n = gp;
                p = rbt_pc_get_parent(n->parent_color);
                rbt_set_parent_color(n, p, RBT_COLOR_RED);
                continue;
            }

            t = p->left;
            if (n == t) {
                t = n->right;
                p->left = t;
                n->right = p;

                if (t)
                    rbt_set_parent_color(t, p, RBT_COLOR_BLACK);

                rbt_set_parent_color(p, n, RBT_COLOR_RED);

                p = n;
                t = n->left;
            }

            gp->right = t;
            p->left = gp;

            if (t)
                rbt_set_parent_color(t, gp, RBT_COLOR_BLACK);
            rbt_rotate_set_parents(gp, p, root, RBT_COLOR_RED);
            break;
        }
    }
}

/*Fix up the red black tree after remove.*/
static void
rbt_fixup (RJS_Rbt **root, RJS_Rbt *p)
{
    RJS_Rbt *n = NULL, *s, *t1, *t2;

    while (1) {
        s = p->right;
        if (n != s) {
            if (rbt_is_red(s)) {
                t1 = s->left;
                p->right = t1;
                s->left  = p;
                rbt_set_parent_color(t1, p, RBT_COLOR_BLACK);
                rbt_rotate_set_parents(p, s, root, RBT_COLOR_RED);
                s = t1;
            }
            t1 = s->right;
            if (!t1 || rbt_is_black(t1)) {
                t2 = s->left;
                if (!t2 || rbt_is_black(t2)) {
                    rbt_set_parent_color(s, p, RBT_COLOR_RED);
                    if (rbt_is_red(p)) {
                        rbt_set_black(p);
                    } else {
                        n = p;
                        p = rbt_pc_get_parent(n->parent_color);
                        if (p)
                            continue;
                    }
                    break;
                }

                t1 = t2->right;
                s->left   = t1;
                t2->right = s;
                p->right  = t2;
                if (t1)
                    rbt_set_parent_color(t1, s, RBT_COLOR_BLACK);
                t1 = s;
                s  = t2;
            }
            t2 = s->left;
            p->right = t2;
            s->left  = p;
            rbt_set_parent_color(t1, s, RBT_COLOR_BLACK);
            if (t2)
                rbt_set_parent(t2, p);
            rbt_rotate_set_parents(p, s, root, RBT_COLOR_BLACK);
            break;
        } else {
            s = p->left;
            if (rbt_is_red(s)) {
                t1 = s->right;
                p->left  = t1;
                s->right = p;
                rbt_set_parent_color(t1, p, RBT_COLOR_BLACK);
                rbt_rotate_set_parents(p, s, root, RBT_COLOR_RED);
                s = t1;
            }
            t1 = s->left;
            if (!t1 || rbt_is_black(t1)) {
                t2 = s->right;
                if (!t2 || rbt_is_black(t2)) {
                    rbt_set_parent_color(s, p, RBT_COLOR_RED);
                    if (rbt_is_red(p)) {
                        rbt_set_black(p);
                    } else {
                        n = p;
                        p = rbt_pc_get_parent(n->parent_color);
                        if (p)
                            continue;
                    }
                    break;
                }
                t1 = t2->left;
                s->right = t1;
                t2->left = s;
                p->left  = t2;
                if (t1)
                    rbt_set_parent_color(t1, s, RBT_COLOR_BLACK);
                t1 = s;
                s  = t2;
            }
            t2 = s->right;
            p->left  = t2;
            s->right = p;
            rbt_set_parent_color(t1, s, RBT_COLOR_BLACK);
            if (t2)
                rbt_set_parent(t2, p);
            rbt_rotate_set_parents(p, s, root, RBT_COLOR_BLACK);
            break;
        }
    } 
}

/**
 * Remove a node from the red black tree.
 * \param root The root of the tree.
 * \param n The node to be removed.
 */
void
rjs_rbt_remove (RJS_Rbt **root, RJS_Rbt *n)
{
    RJS_Rbt *c = n->right;
    RJS_Rbt *t = n->left;
    RJS_Rbt *p, *b;
    size_t   pc;

    if (!t) {
        pc = n->parent_color;
        p  = rbt_pc_get_parent(pc);
        rbt_change_child(n, c, p, root);
        if (c) {
            c->parent_color = pc;
            b = NULL;
        } else {
            b = rbt_pc_is_black(pc) ? p : NULL;
        }
    } else if (!c) {
        pc = n->parent_color;
        t->parent_color = pc;
        p = rbt_pc_get_parent(pc);

        rbt_change_child(n, t, p, root);
        b = NULL;
    } else {
        RJS_Rbt *s = c, *c2;

        t = c->left;
        if (!t) {
            p  = s;
            c2 = s->right;
        } else {
            do {
                p = s;
                s = t;
                t = t->left;
            } while (t);
            
            c2 = s->right;
            p->left  = c2;
            s->right = c;

            rbt_set_parent(c, s);
        }

        t = n->left;
        s->left = t;
        rbt_set_parent(t, s);

        pc = n->parent_color;
        t = rbt_pc_get_parent(pc);
        rbt_change_child(n, s, t, root);

        if (c2) {
            s->parent_color = pc;
            rbt_set_parent_color(c2, p, RBT_COLOR_BLACK);
            b = NULL;
        } else {
            size_t pc2 = s->parent_color;

            s->parent_color = pc;
            b = rbt_pc_is_black(pc2) ? p : NULL;
        }
    }

    if (b)
        rbt_fixup(root, b);
}
