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

/*Create a new type tree node.*/
static RJS_TypeTreeNode*
type_tree_node_new (RJS_Runtime *rt)
{
    RJS_TypeTreeNode *ttn;

    RJS_NEW(rt, ttn);

    ttn->next = rt->type_tree_nodes;
    rt->type_tree_nodes = ttn;

    rjs_hash_init(&ttn->type_hash);
    rjs_hash_init(&ttn->add_node_hash);
    rjs_hash_init(&ttn->del_node_hash);

    return ttn;
}

/*Free the type tree node.*/
static void
type_tree_node_free (RJS_Runtime *rt, RJS_TypeTreeNode *ttn)
{
    size_t         i;
    RJS_TypeClass *c, *nc;

    rjs_hash_foreach_safe_c(&ttn->type_hash, i, c, nc, RJS_TypeClass, he) {
        RJS_DEL(rt, c);
    }

    rjs_hash_deinit(&ttn->type_hash, &rjs_hash_size_ops, rt);
    rjs_hash_deinit(&ttn->add_node_hash, &rjs_hash_size_ops, rt);
    rjs_hash_deinit(&ttn->del_node_hash, &rjs_hash_size_ops, rt);

    RJS_DEL(rt, ttn);
}

/*Build the object's prototype array.*/
static void
proto_array_build (RJS_Runtime *rt, RJS_Object *o)
{
    if (o->type_class->proto_num) {
        RJS_PrototypeRef *pr, *pr_end;
        RJS_Value        *proto;

        RJS_NEW_N(rt, o->prototypes, o->type_class->proto_num);

        pr     = o->prototypes;
        pr_end = pr + o->type_class->proto_num;
        proto  = &o->prototype;

        while (pr < pr_end) {
            RJS_Object *po = rjs_value_get_object(rt, proto);

            pr->o = o;
            pr->p = po;

            rjs_list_append(&po->type_class->back_refs, &pr->ln);

            proto = &po->prototype;
            pr ++;
        }
    }
}

/*Clear the object's prototype array.*/
static void
proto_array_clear (RJS_Runtime *rt, RJS_Object *o)
{
    if (o->prototypes) {
        RJS_PrototypeRef *pr, *pr_end;

        pr = o->prototypes;
        pr_end = pr + o->type_class->proto_num;
        while (pr < pr_end) {
            rjs_list_remove(&pr->ln);
            pr ++;
        }

        RJS_DEL_N(rt, o->prototypes, o->type_class->proto_num);
        o->prototypes = NULL;
    }
}

/*Get the type class of the object.*/
static RJS_TypeClass*
type_class_get (RJS_Runtime *rt, RJS_Object *o)
{
    RJS_TypeClass *tc  = o->type_class;
    RJS_HashEntry *he, **phe;
    RJS_Result     r;

    /*Get the object's type class.*/
    if (!tc) {
        RJS_TypeTreeNode *ttn = o->type_node;
        RJS_TypeClass    *pc;

        //RJS_LOGD("build type class");

        if (rjs_value_is_null(rt, &o->prototype)) {
            pc = NULL;
        } else {
            RJS_Object *pp = rjs_value_get_object(rt, &o->prototype);

            pc = pp->type_class ? pp->type_class : type_class_get(rt, pp);
        }

        r = rjs_hash_lookup(&ttn->type_hash, pc, &he, &phe, &rjs_hash_size_ops, rt);
        if (r) {
            tc = RJS_CONTAINER_OF(he, RJS_TypeClass, he);
        } else {
            RJS_NEW(rt, tc);

            rjs_list_init(&tc->back_refs);

            tc->proto_num = pc ? pc->proto_num + 1 : 0;

            /*Add the new type class.*/
            rjs_hash_insert(&ttn->type_hash, pc, &tc->he, phe, &rjs_hash_size_ops, rt);
        }

        o->type_class = tc;

        proto_array_build(rt, o);
    } else {
        //RJS_LOGD("cache type class");
    }

    return tc;
}

/*Reset the type class.*/
static void
type_class_reset (RJS_Runtime *rt, RJS_TypeClass *c)
{
    RJS_PrototypeRef *pr, *npr;

    /*Clear all the object's type inforation referenced to this class.*/
    rjs_list_foreach_safe_c(&c->back_refs, pr, npr, RJS_PrototypeRef, ln) {
        RJS_Object *o = pr->o;
        
        proto_array_clear(rt, o);

        o->type_class = NULL;
    }

    rjs_list_init(&c->back_refs);
}

/*Scan the hash table keys.*/
static void
gc_scan_hash_keys (RJS_Runtime *rt, RJS_Hash *hash)
{
    size_t         i;
    RJS_HashEntry *e;

    rjs_hash_foreach(hash, i, e) {
        rjs_gc_mark(rt, e->key);
    }
}

/**
 * Initialize the type tree.
 * \param rt The current runtime.
 */
void
rjs_type_tree_init (RJS_Runtime *rt)
{
    rt->type_tree_nodes = NULL;

    rt->type_tree_root = type_tree_node_new(rt);

    rjs_list_init(&rt->rb.prop_cache_list);
}

/**
 * Release the type tree.
 * \param rt The current runtime.
 */
void
rjs_type_tree_deinit (RJS_Runtime *rt)
{
    RJS_TypeTreeNode  *n, *nn;
    RJS_PropertyCache *c, *nc;

    for (n = rt->type_tree_root; n; n = nn) {
        nn = n->next;
        type_tree_node_free(rt, n);
    }

    rjs_list_foreach_safe_c(&rt->rb.prop_cache_list, c, nc, RJS_PropertyCache, ln) {
        RJS_DEL(rt, c);
    }
}

/**
 * Scan the referenced things in the type tree.
 * \param rt The current runtime.
 */
void
rjs_gc_scan_type_tree (RJS_Runtime *rt)
{
    RJS_TypeTreeNode *n;

    n = rt->type_tree_nodes;
    while (n) {
        gc_scan_hash_keys(rt, &n->add_node_hash);
        gc_scan_hash_keys(rt, &n->del_node_hash);

        n = n->next;
    }
}

/**
 * Reset the object's type information when reset prototype.
 * \param rt The current runtime.
 * \param o The object.
 */
void
rjs_type_tree_reset_proto (RJS_Runtime *rt, RJS_Object *o)
{
    if (o->type_class) {
        type_class_reset(rt, o->type_class);
        o->type_class = NULL;
    }
}

/**
 * Add an add property node to the type tree.
 * \param rt The current runtime.
 * \param o The object.
 * \param key The key of the property.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_type_tree_add (RJS_Runtime *rt, RJS_Object *o, void *key)
{
    RJS_HashEntry    *he, **phe;
    RJS_TypeTreeNode *ttn;
    RJS_Result        r;

    if (o->type_class) {
        type_class_reset(rt, o->type_class);
        o->type_class = NULL;
    }

    r = rjs_hash_lookup(&o->type_node->add_node_hash, key, &he, &phe, &rjs_hash_size_ops, rt);
    if (r) {
        ttn = RJS_CONTAINER_OF(he, RJS_TypeTreeNode, he);
    } else {
        ttn = type_tree_node_new(rt);

        rjs_hash_insert(&o->type_node->add_node_hash, key, &ttn->he, phe, &rjs_hash_size_ops, rt);
    }

    o->type_node = ttn;
    return RJS_OK;
}

/**
 * Add a delete property node to the type tree.
 * \param rt The current runtime.
 * \param o The object.
 * \param key The key of the property.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_type_tree_del (RJS_Runtime *rt, RJS_Object *o, void *key)
{
    RJS_HashEntry    *he, **phe;
    RJS_TypeTreeNode *ttn;
    RJS_Result        r;

    if (o->type_class) {
        type_class_reset(rt, o->type_class);
        o->type_class = NULL;
    }

    r = rjs_hash_lookup(&o->type_node->del_node_hash, key, &he, &phe, &rjs_hash_size_ops, rt);
    if (r) {
        ttn = RJS_CONTAINER_OF(he, RJS_TypeTreeNode, he);
    } else {
        ttn = type_tree_node_new(rt);

        rjs_hash_insert(&o->type_node->del_node_hash, key, &ttn->he, phe, &rjs_hash_size_ops, rt);
    }

    o->type_node = ttn;
    return RJS_OK;
}

/**
 * Add property cache.
 * \param rt The current runtime.
 * \param pn The property name.
 * \param b The base object.
 * \param p The prototype object.
 * \param prop_idx The property's index.
 */
void
rjs_type_tree_cache (RJS_Runtime *rt, RJS_PropertyName *pn, RJS_Object *b, RJS_Object *p, int prop_idx)
{
    RJS_TypeClass     *bc, *pc;
    RJS_PropertyCache *cache;

    bc = type_class_get(rt, b);
    pc = type_class_get(rt, p);

    if (rjs_list_is_empty(&rt->rb.prop_cache_list)) {
        RJS_NEW(rt, cache);
    } else {
        cache = RJS_CONTAINER_OF(rt->rb.prop_cache_list.next, RJS_PropertyCache, ln);

        rjs_list_remove(&cache->ln);
    }

    cache->clazz     = bc;
    cache->proto_idx = bc->proto_num - pc->proto_num;
    cache->prop_idx  = prop_idx;

    rjs_list_prepend(&pn->n.p.cache_list, &cache->ln);
}
