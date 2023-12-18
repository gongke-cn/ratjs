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

/*Scan the reference things in the property key list*/
static void
prop_key_list_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_PropertyKeyList *pkl = ptr;

    rjs_gc_scan_value_buffer(rt, pkl->keys.items, pkl->keys.item_num);
}

/*Free the property key list.*/
static void
prop_key_list_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_PropertyKeyList *pkl = ptr;

    rjs_vector_deinit(&pkl->keys, rt);

    RJS_DEL(rt, pkl);
}

/*Property key list operation functions.*/
static const RJS_GcThingOps
prop_key_list_ops = {
    RJS_GC_THING_PROP_KEY_LIST,
    prop_key_list_op_gc_scan,
    prop_key_list_op_gc_free
};

/**
 * Allocate a new proprty key list.
 * \param rt The current runtime.
 * \param[out] v Return the key list value.
 * \param cap The buffer capacity of the key list.
 * \return The key list.
 */
RJS_PropertyKeyList*
rjs_property_key_list_new (RJS_Runtime *rt, RJS_Value *v, size_t cap)
{
    RJS_PropertyKeyList *pkl;

    RJS_NEW(rt, pkl);

    rjs_vector_init(&pkl->keys);
    rjs_vector_set_capacity(&pkl->keys, cap, rt);

    rjs_value_set_gc_thing(rt, v, pkl);
    rjs_gc_add(rt, pkl, &prop_key_list_ops);

    return pkl;
}

/*Ordinary object operation functions.*/
static const RJS_ObjectOps
ordinary_object_ops = {
    {
        RJS_GC_THING_OBJECT,
        rjs_object_op_gc_scan,
        rjs_object_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS,
    NULL,
    NULL
};

/*Free the property red/black tree.*/
static void
prop_rbt_free_all (RJS_Runtime *rt, RJS_Rbt *rbt)
{
    RJS_PropertyRbt *pr = (RJS_PropertyRbt*)rbt;

    if (rbt->left)
        prop_rbt_free_all(rt, rbt->left);

    if (rbt->right)
        prop_rbt_free_all(rt, rbt->right);

    RJS_DEL(rt, pr);
}

/*Copy property red/black tree to vector.*/
static void
prop_rbt_set_vec (RJS_Runtime *rt, RJS_Rbt *rbt, RJS_Property *vec, size_t len)
{
    RJS_PropertyRbt *pr = (RJS_PropertyRbt*)rbt;

    if (pr->index < len)
        vec[pr->index] = pr->prop;

    if (pr->rbt.left)
        prop_rbt_set_vec(rt, pr->rbt.left, vec, len);

    if (pr->rbt.right)
        prop_rbt_set_vec(rt, pr->rbt.right, vec, len);
}

/*Convert the property red/black tree to vector.*/
static RJS_Property*
prop_rbt_to_vec (RJS_Runtime *rt, RJS_Rbt *rbt, size_t len)
{
    RJS_Property *vec;
    size_t        i;

    if (!rbt)
        return NULL;

    RJS_NEW_N(rt, vec, len);

    for (i = 0; i < len; i ++)
        vec[i].attrs = RJS_PROP_ATTR_DELETED;

    if (rbt)
        prop_rbt_set_vec(rt, rbt, vec, len);

    prop_rbt_free_all(rt, rbt);

    return vec;
}

/*Convert the property vector to red/black tree.*/
static void
prop_vec_to_rbt (RJS_Runtime *rt, RJS_Property *vec, uint32_t max, size_t cap, RJS_Rbt **root)
{
    size_t i;

    if (!vec)
        return;

    for (i = 0; i <= max; i ++) {
        RJS_Property    *prop;
        RJS_PropertyRbt *pr;
        RJS_Rbt        **pos;
        RJS_Rbt         *p;

        prop = &vec[i];

        if (prop->attrs & RJS_PROP_ATTR_DELETED)
            continue;

        pos = root;
        p   = NULL;
        while (1) {
            pr = (RJS_PropertyRbt*)*pos;
            if (!pr)
                break;

            p = (RJS_Rbt*)pr;
            if (i < pr->index)
                pos = &pr->rbt.left;
            else
                pos = &pr->rbt.right;
        }

        RJS_NEW(rt, pr);

        pr->index = i;
        pr->prop  = *prop;

        rjs_rbt_link(&pr->rbt, p, pos);
        rjs_rbt_insert(root, &pr->rbt);
    }

    RJS_DEL_N(rt, vec, cap);
}

/*Get the property key.*/
static void
prop_key_get (RJS_Runtime *rt, RJS_Value *v, RJS_PropertyKey *pk)
{
    switch (rjs_value_get_type(rt, v)) {
    case RJS_VALUE_STRING:
        if (rjs_value_is_index_string(rt, v)) {
            pk->is_index = RJS_TRUE;
            pk->index    = rjs_value_get_index_string(rt, v);
        } else {
            int64_t idx;

            pk->is_index = rjs_string_to_index(rt, v, &idx);

            if (!pk->is_index) {
                rjs_string_to_property_key(rt, v);
                pk->key = rjs_value_get_string(rt, v);
            } else {
                pk->index = idx;
            }
        }
        break;
    case RJS_VALUE_SYMBOL:
        pk->is_index = RJS_FALSE;
        pk->key      = rjs_value_get_symbol(rt, v);
        break;
    default:
#if ENABLE_PRIV_NAME
        if (rjs_value_is_private_name(rt, v)) {
            pk->is_index = RJS_FALSE;
            pk->key      = rjs_value_get_gc_thing(rt, v);
        } else
#endif /*ENABLE_PRIV_NAME*/
        {
            assert(0);
        }
    }
}

/**
 * Convert the value to property key.
 * \param rt The current runtime.
 * \param p The property key value.
 * \param[out] pk Return the property key.
 */
void
rjs_property_key_get (RJS_Runtime *rt, RJS_Value *p, RJS_PropertyKey *pk)
{
    prop_key_get(rt, p, pk);
}

/*Lookup the property.*/
static RJS_Property*
prop_lookup (RJS_Runtime *rt, RJS_Value *v, RJS_PropertyKey *pk)
{
    RJS_Object *o = rjs_value_get_object(rt, v);

    if (pk->is_index) {
        if (o->flags & RJS_OBJECT_FL_RBT) {
            RJS_PropertyRbt **pn = (RJS_PropertyRbt**)&o->prop_array.rbt;
            RJS_PropertyRbt  *n;

            while ((n = *pn)) {
                if (n->index == pk->index)
                    return &n->prop;

                if (pk->index < n->index)
                    pn = (RJS_PropertyRbt**)&n->rbt.left;
                else
                    pn = (RJS_PropertyRbt**)&n->rbt.right;
            }
        } else if (o->array_item_cap) {
            RJS_Property *prop;

            if (pk->index > o->array_item_max)
                return NULL;

            prop = &o->prop_array.vec[pk->index];

            if (prop->attrs & RJS_PROP_ATTR_DELETED)
                return NULL;

            return prop;
        }

        return NULL;
    } else {
        RJS_HashEntry    *he;
        RJS_Result        r;
        RJS_PropertyNode *pn;

        r = rjs_hash_lookup(&o->prop_hash, pk->key, &he, NULL, &rjs_hash_size_ops, rt);
        if (!r)
            return NULL;

        pn = RJS_CONTAINER_OF(he, RJS_PropertyNode, he);

        return &pn->prop;
    }
}

/*Update the array's maximum index.*/
static void
update_array_item_max (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_Object *o   = rjs_value_get_object(rt, v);
    size_t      max = o->array_item_max;

    if (o->flags & RJS_OBJECT_FL_RBT) {
        RJS_Rbt         *rbt;
        RJS_PropertyRbt *pr;

        rbt = rjs_rbt_last(&o->prop_array.rbt);
        if (rbt) {
            pr  = (RJS_PropertyRbt*)rbt;
            max = pr->index;
        } else {
            max = 0;
        }
    } else {
        while (max > 0) {
            if (!(o->prop_array.vec[max].attrs & RJS_PROP_ATTR_DELETED))
                break;

            max --;
        }
    }

    o->array_item_max = max;
}

/*Fixup the array properties.*/
static void
prop_array_fixup (RJS_Runtime *rt, RJS_Value *v, uint32_t num, uint32_t max)
{
    RJS_Object *o = rjs_value_get_object(rt, v);

    if (!(o->flags & RJS_OBJECT_FL_RBT)) {
        /*Convert the vector to RBT if the array is sparse.*/
        if ((max > num * 4) && (max > 16)) {
            RJS_Rbt *root;

            rjs_rbt_init(&root);
            prop_vec_to_rbt(rt, o->prop_array.vec, o->array_item_max, o->array_item_cap, &root);

            o->prop_array.rbt = root;
            o->flags |= RJS_OBJECT_FL_RBT;
        } else if (max >= o->array_item_cap) {
            /*Expand the vector.*/
            size_t olen, nlen, i;

            olen = o->array_item_cap;
            nlen = RJS_MIN(RJS_MAX_3(max + 1, o->array_item_cap * 2, 8), 0xffffffff);

            RJS_RENEW(rt, o->prop_array.vec, olen, nlen);

            o->array_item_cap = nlen;

            for (i = olen; i < nlen; i ++) {
                RJS_Property *prop;

                prop = &o->prop_array.vec[i];

                prop->attrs = RJS_PROP_ATTR_DELETED;
            }
        }
    } else {
        /*Convert the property RBT to vector if the array is not sparse.*/
        if (max < num * 3) {
            o->prop_array.vec = prop_rbt_to_vec(rt, o->prop_array.rbt, max + 1);
            o->array_item_cap = max + 1;
            o->flags &= ~RJS_OBJECT_FL_RBT;
        }
    }

    o->array_item_num = num;
    o->array_item_max = max;
}

/*Add a new property.*/
static RJS_Property*
prop_add (RJS_Runtime *rt, RJS_Value *v, RJS_PropertyKey *pk)
{
    RJS_Object   *o = rjs_value_get_object(rt, v);
    RJS_Property *prop;

    /*Add the new property.*/
    if (pk->is_index) {
        prop_array_fixup(rt, v, o->array_item_num + 1,
                RJS_MAX(pk->index, o->array_item_max));

        if (o->flags & RJS_OBJECT_FL_RBT) {
            RJS_Rbt        **pos = &o->prop_array.rbt;
            RJS_Rbt         *p   = NULL;
            RJS_PropertyRbt *pr;

            while (1) {
                pr = (RJS_PropertyRbt*)*pos;
                if (!pr)
                    break;

                p = (RJS_Rbt*)pr;

                if (pk->index < pr->index)
                    pos = &pr->rbt.left;
                else
                    pos = &pr->rbt.right;
            }

            RJS_NEW(rt, pr);

            pr->index = pk->index;

            rjs_rbt_link(&pr->rbt, p, pos);
            rjs_rbt_insert(&o->prop_array.rbt, &pr->rbt);

            prop = &pr->prop;
        } else {
            prop = &o->prop_array.vec[pk->index];
        }
    } else {
        /*String or symbol key.*/
        RJS_PropertyNode *pn;

        RJS_NEW(rt, pn);

        rjs_hash_insert(&o->prop_hash, pk->key, &pn->he, NULL, &rjs_hash_size_ops, rt);
        rjs_list_append(&o->prop_list, &pn->ln);

        prop = &pn->prop;
    }

    return prop;
}

/*Delete a property.*/
static void
prop_delete (RJS_Runtime *rt, RJS_Value *v, RJS_PropertyKey *pk)
{
    RJS_Object *o = rjs_value_get_object(rt, v);

    /*Delete the property.*/
    if (pk->is_index) {
        if (o->flags & RJS_OBJECT_FL_RBT) {
            RJS_Rbt        **pos = &o->prop_array.rbt;
            RJS_PropertyRbt *pr;

            while (1) {
                pr = (RJS_PropertyRbt*)*pos;

                if (pr->index == pk->index)
                    break;

                if (pk->index < pr->index)
                    pos = &pr->rbt.left;
                else
                    pos = &pr->rbt.right;
            }

            rjs_rbt_remove(&o->prop_array.rbt, &pr->rbt);

            RJS_DEL(rt, pr);
        } else {
            RJS_Property *prop;

            prop = &o->prop_array.vec[pk->index];

            prop->attrs |= RJS_PROP_ATTR_DELETED;
        }

        if (pk->index == o->array_item_max)
            update_array_item_max(rt, v);

        prop_array_fixup(rt, v, o->array_item_num - 1, o->array_item_max);
    } else {
        RJS_PropertyNode *pn;
        RJS_HashEntry    *he, **phe;
        RJS_Result        r;

        r = rjs_hash_lookup(&o->prop_hash, pk->key, &he, &phe, &rjs_hash_size_ops, rt);
        assert(r);

        pn = RJS_CONTAINER_OF(he, RJS_PropertyNode, he);

        rjs_hash_remove(&o->prop_hash, phe, rt);
        rjs_list_remove(&pn->ln);

        RJS_DEL(rt, pn);
    }
}

/*Scan the refernced things in the property.*/
static void
prop_gc_scan (RJS_Runtime *rt, RJS_Property *p)
{
    if (p->attrs & RJS_PROP_ATTR_ACCESSOR) {
        rjs_gc_scan_value(rt, &p->p.a.get);
        rjs_gc_scan_value(rt, &p->p.a.set);
    } else {
        rjs_gc_scan_value(rt, &p->p.value);
    }
}

/*Scan the property red/black tree.*/
static void
prop_rbt_gc_scan (RJS_Runtime *rt, RJS_Rbt *rbt)
{
    RJS_PropertyRbt *pr = (RJS_PropertyRbt*)rbt;

    if (rbt->left)
        prop_rbt_gc_scan(rt, rbt->left);

    if (rbt->right)
        prop_rbt_gc_scan(rt, rbt->right);

    prop_gc_scan(rt, &pr->prop);
}

/**
 * Scan the reference things in the ordinary object.
 * \param rt The current runtime.
 * \param ptr The object pointer.
 */
void
rjs_object_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_Object       *o = ptr;
    RJS_PropertyNode *pn;

    rjs_gc_scan_value(rt, &o->prototype);

    rjs_list_foreach_c(&o->prop_list, pn, RJS_PropertyNode, ln) {
        rjs_gc_mark(rt, pn->he.key);

        prop_gc_scan(rt, &pn->prop);
    }

    if (o->flags & RJS_OBJECT_FL_RBT) {
        if (o->prop_array.rbt)
            prop_rbt_gc_scan(rt, o->prop_array.rbt);
    } else if (o->array_item_cap) {
        size_t i;

        for (i = 0; i <= o->array_item_max; i ++) {
            RJS_Property *prop = &o->prop_array.vec[i];

            if (prop->attrs & RJS_PROP_ATTR_DELETED)
                continue;

            prop_gc_scan(rt, prop);
        }
    }
}

/**
 * Free the ordinary object.
 * \param rt The current runtime.
 * \param ptr The object pointer.
 */
void
rjs_object_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_Object *o = ptr;

    rjs_object_deinit(rt, o);

    RJS_DEL(rt, o);
}

/**
 * Get the prototype of an ordinary object.
 * \param rt The current runtime.
 * \param v The object value.
 * \param[out] proto Return the prototype object or null.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_ordinary_object_op_get_prototype_of (RJS_Runtime *rt, RJS_Value *v, RJS_Value *proto)
{
    RJS_Object *o = rjs_value_get_object(rt, v);

    rjs_value_copy(rt, proto, &o->prototype);

    return RJS_OK;
}

/**
 * Set the prototype of an ordinary object.
 * \param rt The current runtime.
 * \param v The object value.
 * \param proto The prototype object or null.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE Cannot reset the prototype.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_ordinary_object_op_set_prototype_of (RJS_Runtime *rt, RJS_Value *v, RJS_Value *proto)
{
    RJS_Object *o   = rjs_value_get_object(rt, v);
    size_t      top = rjs_value_stack_save(rt);
    RJS_Value  *p   = rjs_value_stack_push(rt);
    RJS_Result  r;

    if (rjs_same_value(rt, &o->prototype, proto)) {
        r = RJS_TRUE;
        goto end;
    }

    if (!(o->flags & RJS_OBJECT_FL_EXTENSIBLE)) {
        r = RJS_FALSE;
        goto end;
    }

    rjs_value_copy(rt, p, proto);

    while (1) {
        RJS_GcThing   *gt;
        RJS_ObjectOps *ops;

        if (rjs_value_is_null(rt, p))
            break;

        if (rjs_same_value(rt, p, v)) {
            r = RJS_FALSE;
            goto end;
        }

        gt  = (RJS_GcThing*)rjs_value_get_object(rt, p);
        ops = (RJS_ObjectOps*)gt->ops;

        if (ops->get_prototype_of != rjs_ordinary_object_op_get_prototype_of)
            break;

        rjs_ordinary_object_op_get_prototype_of(rt, p, p);
    }

    rjs_value_copy(rt, &o->prototype, proto);
    r = RJS_TRUE;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Check if the ordinary object is extensible.
 * \param rt The current runtime.
 * \param v The object value.
 * \retval RJS_TRUE The object is extensible.
 * \retval RJS_FALSE The object is not extensible.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_ordinary_object_op_is_extensible (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_Object *o = rjs_value_get_object(rt, v);

    return o->flags & RJS_OBJECT_FL_EXTENSIBLE ? RJS_TRUE : RJS_FALSE;
}

/**
 * Prevent the extensions of the ordinary object.
 * \param rt The current runtime.
 * \param v The object value.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE Cannot prevent extensions.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_ordinary_object_op_prevent_extensions (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_Object *o = rjs_value_get_object(rt, v);

    o->flags &= ~RJS_OBJECT_FL_EXTENSIBLE;

    return RJS_TRUE;
}

/**
 * Get the own property's descriptor of the ordinary object.
 * \param rt The current runtime.
 * \param v the object value.
 * \param pn The property name.
 * \param[out] pd Return the property descriptor.
 * \retval RJS_OK On success.
 * \retval RJS_FALSE The property is not defined.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_ordinary_object_op_get_own_property (RJS_Runtime *rt, RJS_Value *v, RJS_PropertyName *pn, RJS_PropertyDesc *pd)
{
    RJS_PropertyKey  pk;
    RJS_Property    *prop;

    prop_key_get(rt, pn->name, &pk);

    prop = prop_lookup(rt, v, &pk);
    if (!prop)
        return RJS_FALSE;

    if (prop->attrs & RJS_PROP_ATTR_ACCESSOR) {
        pd->flags = RJS_PROP_FL_ACCESSOR;

        rjs_value_copy(rt, pd->get, &prop->p.a.get);
        rjs_value_copy(rt, pd->set, &prop->p.a.set);
    } else {
        pd->flags = RJS_PROP_FL_DATA;

        if (prop->attrs & RJS_PROP_ATTR_WRITABLE)
            pd->flags |= RJS_PROP_FL_WRITABLE;

        rjs_value_copy(rt, pd->value, &prop->p.value);
    }

    if (prop->attrs & RJS_PROP_ATTR_CONFIGURABLE)
        pd->flags |= RJS_PROP_FL_CONFIGURABLE;
    if (prop->attrs & RJS_PROP_ATTR_ENUMERABLE)
        pd->flags |= RJS_PROP_FL_ENUMERABLE;

    return RJS_TRUE;
}

/**
 * Validate and try to apply the property descriptor.
 */
static RJS_Result
validate_and_apply_property (RJS_Runtime *rt, RJS_Value *v, RJS_PropertyName *pn, RJS_Bool ext,
        RJS_PropertyDesc *desc, RJS_PropertyDesc *curr)
{
    RJS_PropertyKey   pk;
    RJS_Property     *prop;

    if (pn)
        assert(rjs_is_property_key(rt, pn->name));

    if (!curr) {
        if (!ext)
            return RJS_FALSE;

        if (rjs_value_is_undefined(rt, v))
            return RJS_TRUE;

        prop_key_get(rt, pn->name, &pk);

        prop = prop_add(rt, v, &pk);

        prop->attrs = 0;

        if (rjs_is_accessor_descriptor(desc)) {
            prop->attrs |= RJS_PROP_ATTR_ACCESSOR;
            prop->attrs |= desc->flags
                    & (RJS_PROP_FL_CONFIGURABLE|RJS_PROP_FL_ENUMERABLE);

            if (desc->flags & RJS_PROP_FL_HAS_GET)
                rjs_value_copy(rt, &prop->p.a.get, desc->get);
            else
                rjs_value_set_undefined(rt, &prop->p.a.get);

            if (desc->flags & RJS_PROP_FL_HAS_SET)
                rjs_value_copy(rt, &prop->p.a.set, desc->set);
            else
                rjs_value_set_undefined(rt, &prop->p.a.set);
        } else {
            prop->attrs |= desc->flags
                    & (RJS_PROP_FL_CONFIGURABLE|RJS_PROP_FL_ENUMERABLE|RJS_PROP_FL_WRITABLE);

            if (desc->flags & RJS_PROP_FL_HAS_VALUE)
                rjs_value_copy(rt, &prop->p.value, desc->value);
            else
                rjs_value_set_undefined(rt, &prop->p.value);
        }

        return RJS_TRUE;
    }

    if (!desc->flags)
        return RJS_TRUE;

    if (!(curr->flags & RJS_PROP_FL_CONFIGURABLE)) {
        if ((desc->flags & RJS_PROP_FL_HAS_CONFIGURABLE)
                && (desc->flags & RJS_PROP_FL_CONFIGURABLE))
            return RJS_FALSE;
        if ((desc->flags & RJS_PROP_FL_HAS_ENUMERABLE)
                && ((desc->flags & RJS_PROP_FL_ENUMERABLE) != (curr->flags & RJS_PROP_FL_ENUMERABLE)))
            return RJS_FALSE;
        if (!rjs_is_generic_descriptor(desc)
                && (rjs_is_accessor_descriptor(desc) != rjs_is_accessor_descriptor(curr)))
            return RJS_FALSE;
        if (rjs_is_accessor_descriptor(desc)) {
            if ((desc->flags & RJS_PROP_FL_HAS_GET) && !rjs_same_value(rt, desc->get, curr->get))
                return RJS_FALSE;
            if ((desc->flags & RJS_PROP_FL_HAS_SET) && !rjs_same_value(rt, desc->set, curr->set))
                return RJS_FALSE;
        } else if (!(curr->flags & RJS_PROP_FL_WRITABLE)) {
            if ((desc->flags & RJS_PROP_FL_HAS_WRITABLE) && (desc->flags & RJS_PROP_FL_WRITABLE))
                return RJS_FALSE;
            if ((desc->flags & RJS_PROP_FL_HAS_VALUE) && !rjs_same_value(rt, desc->value, curr->value))
                return RJS_FALSE;
        }
    }

    if (!rjs_value_is_undefined(rt, v)) {
        prop_key_get(rt, pn->name, &pk);

        prop = prop_lookup(rt, v, &pk);
        assert(prop);

        if (desc->flags & RJS_PROP_FL_HAS_CONFIGURABLE) {
            if (desc->flags & RJS_PROP_FL_CONFIGURABLE)
                prop->attrs |= RJS_PROP_ATTR_CONFIGURABLE;
            else
                prop->attrs &= ~RJS_PROP_ATTR_CONFIGURABLE;
        }

        if (desc->flags & RJS_PROP_FL_HAS_ENUMERABLE) {
            if (desc->flags & RJS_PROP_FL_ENUMERABLE)
                prop->attrs |= RJS_PROP_ATTR_ENUMERABLE;
            else
                prop->attrs &= ~RJS_PROP_ATTR_ENUMERABLE;
        }

        if (rjs_is_data_descriptor(desc) && rjs_is_accessor_descriptor(curr)) {
            prop->attrs &= ~RJS_PROP_ATTR_ACCESSOR;

            if (desc->flags & RJS_PROP_FL_HAS_VALUE)
                rjs_value_copy(rt, &prop->p.value, desc->value);
            else
                rjs_value_set_undefined(rt, &prop->p.value);

            if (desc->flags & RJS_PROP_FL_HAS_WRITABLE) {
                if (desc->flags & RJS_PROP_FL_WRITABLE)
                    prop->attrs |= RJS_PROP_ATTR_WRITABLE;
            }
        } else if (rjs_is_accessor_descriptor(desc) && rjs_is_data_descriptor(curr)) {
            prop->attrs &= ~RJS_PROP_ATTR_WRITABLE;
            prop->attrs |= RJS_PROP_ATTR_ACCESSOR;

            if (desc->flags & RJS_PROP_FL_HAS_GET)
                rjs_value_copy(rt, &prop->p.a.get, desc->get);
            else
                rjs_value_set_undefined(rt, &prop->p.a.get);

            if (desc->flags & RJS_PROP_FL_HAS_SET)
                rjs_value_copy(rt, &prop->p.a.set, desc->set);
            else
                rjs_value_set_undefined(rt, &prop->p.a.set);
        } else {
            if (prop->attrs & RJS_PROP_ATTR_ACCESSOR) {
                if (desc->flags & RJS_PROP_FL_HAS_GET)
                    rjs_value_copy(rt, &prop->p.a.get, desc->get);

                if (desc->flags & RJS_PROP_FL_HAS_SET)
                    rjs_value_copy(rt, &prop->p.a.set, desc->set);
            } else {
                if (desc->flags & RJS_PROP_FL_HAS_VALUE)
                    rjs_value_copy(rt, &prop->p.value, desc->value);
                if (desc->flags & RJS_PROP_FL_HAS_WRITABLE) {
                    if (desc->flags & RJS_PROP_FL_WRITABLE)
                        prop->attrs |= RJS_PROP_FL_WRITABLE;
                    else
                        prop->attrs &= ~RJS_PROP_FL_WRITABLE;
                }
            }
        }
    }

    return RJS_TRUE;
}

/**
 * Check if the value is a regular expression.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE The value is a regular expression.
 * \retval RJS_FALSE The value is not a regular expression.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_is_regexp (RJS_Runtime *rt, RJS_Value *v)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *match = rjs_value_stack_push(rt);
    RJS_Result r;

    if (!rjs_value_is_object(rt, v)) {
        r = RJS_FALSE;
        goto end;
    }

    if ((r = rjs_get(rt, v, rjs_pn_s_match(rt), match)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_undefined(rt, match)) {
        r = rjs_to_boolean(rt, match);
        goto end;
    }

    r = (rjs_value_get_gc_thing_type(rt, v) == RJS_GC_THING_REGEXP);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Check if 2 property descriptors are compatible.
 * \param rt The current runtime.
 * \param ext The object is extensible.
 * \param desc The new descriptor.
 * \param curr The current descriptor.
 * \retval RJS_TRUE The descriptors are compatible.
 * \retval RJS_FALSE The descriptors are not compatible.
 */
RJS_Result
rjs_is_compatible_property_descriptor (RJS_Runtime *rt, RJS_Bool ext, RJS_PropertyDesc *desc, RJS_PropertyDesc *curr)
{
    return validate_and_apply_property(rt, rjs_v_undefined(rt), NULL, ext, desc, curr);
}

/**
 * Define an own property of an ordinary object.
 * \param rt The current runtime.
 * \param v The object value.
 * \param pn The property name.
 * \param pd The property's descriptor.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE Cannot create or updete the property.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_ordinary_object_op_define_own_property (RJS_Runtime *rt, RJS_Value *v, RJS_PropertyName *pn, RJS_PropertyDesc *pd)
{
    RJS_PropertyDesc  curr_buf;
    RJS_Result        r;
    RJS_Bool          ext;
    RJS_PropertyDesc *curr = &curr_buf;
    size_t            top  = rjs_value_stack_save(rt);

    rjs_property_desc_init(rt, curr);

    if ((r = rjs_object_get_own_property(rt, v, pn, curr)) == RJS_ERR)
        goto end;
    if (!r)
        curr = NULL;

    if ((r = rjs_object_is_extensible(rt, v)) == RJS_ERR)
        goto end;
    ext = r;

    r = validate_and_apply_property(rt, v, pn, ext, pd, curr);
end:
    rjs_property_desc_deinit(rt, curr);
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Check if the ordinary object has the property.
 * \param rt The current runtime.
 * \param v The object value.
 * \param pn The property name.
 * \retval RJS_TRUE The object has the property.
 * \retval RJS_FALSE The object has not the property.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_ordinary_object_op_has_property (RJS_Runtime *rt, RJS_Value *v, RJS_PropertyName *pn)
{
    RJS_PropertyDesc pd;
    RJS_Result       r;
    size_t           top    = rjs_value_stack_save(rt);
    RJS_Value       *parent = rjs_value_stack_push(rt);

    rjs_property_desc_init(rt, &pd);

    if ((r = rjs_object_get_own_property(rt, v, pn, &pd)) == RJS_ERR)
        goto end;

    if (r)
        goto end;

    if ((r = rjs_object_get_prototype_of(rt, v, parent)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_null(rt, parent)) {
        r = rjs_object_has_property(rt, parent, pn);
        goto end;
    }

    r = RJS_FALSE;
end:
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Get the property value of an ordinary object.
 * \param rt The current runtime.
 * \param v The object value.
 * \param pn The property name.
 * \param receiver The receiver object.
 * \param[out] pv Return the property value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_ordinary_object_op_get (RJS_Runtime *rt, RJS_Value *v, RJS_PropertyName *pn, RJS_Value *receiver, RJS_Value *pv)
{
    RJS_PropertyDesc pd;
    RJS_Result       r;
    size_t           top    = rjs_value_stack_save(rt);
    RJS_Value       *parent = rjs_value_stack_push(rt);

    rjs_property_desc_init(rt, &pd);

    if ((r = rjs_object_get_own_property(rt, v, pn, &pd)) == RJS_ERR)
        goto end;

    if (!r) {
        if ((r = rjs_object_get_prototype_of(rt, v, parent)) == RJS_ERR)
            goto end;

        if (rjs_value_is_null(rt, parent)) {
            rjs_value_set_undefined(rt, pv);
            r = RJS_OK;
            goto end;
        }

        r = rjs_object_get(rt, parent, pn, receiver, pv);
        goto end;
    }

    if (rjs_is_data_descriptor(&pd)) {
        rjs_value_copy(rt, pv, pd.value);
    } else {
        if (rjs_value_is_undefined(rt, pd.get)) {
            rjs_value_set_undefined(rt, pv);
        } else {
            r = rjs_call(rt, pd.get, receiver, NULL, 0, pv);
            goto end;
        }
    }

    r = RJS_OK;
end:
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Set the property value of an ordinary object.
 * \param rt The current runtime.
 * \param v The object value.
 * \param pn The property name.
 * \param pv The new property value.
 * \param receiver The receiver object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_ordinary_object_op_set (RJS_Runtime *rt, RJS_Value *v, RJS_PropertyName *pn, RJS_Value *pv, RJS_Value *receiver)
{
    RJS_PropertyDesc pd;
    RJS_Result       r;
    size_t           top    = rjs_value_stack_save(rt);
    RJS_Value       *parent = rjs_value_stack_push(rt);
    RJS_Value       *tmp    = rjs_value_stack_push(rt);

    rjs_property_desc_init(rt, &pd);

    if ((r = rjs_object_get_own_property(rt, v, pn, &pd)) == RJS_ERR)
        goto end;

    if (!r) {
        if ((r = rjs_object_get_prototype_of(rt, v, parent)) == RJS_ERR)
            goto end;

        if (!rjs_value_is_null(rt, parent)) {
            r = rjs_object_set(rt, parent, pn, pv, receiver);
            goto end;
        }

        pd.flags = RJS_PROP_FL_DATA
                |RJS_PROP_FL_CONFIGURABLE
                |RJS_PROP_FL_ENUMERABLE
                |RJS_PROP_FL_WRITABLE;

        rjs_value_set_undefined(rt, pd.value);
    }

    if (rjs_is_data_descriptor(&pd)) {
        RJS_PropertyDesc ed;

        if (!(pd.flags & RJS_PROP_FL_WRITABLE)) {
            r = RJS_FALSE;
            goto end;
        }

        if (!rjs_value_is_object(rt, receiver)) {
            r = RJS_FALSE;
            goto end;
        }

        rjs_property_desc_init(rt, &ed);

        if ((r = rjs_object_get_own_property(rt, receiver, pn, &ed)) == RJS_ERR)
            goto end1;

        if (r) {
            if (rjs_is_accessor_descriptor(&ed)) {
                r = RJS_FALSE;
                goto end1;
            }

            if (!(ed.flags & RJS_PROP_FL_WRITABLE)) {
                r = RJS_FALSE;
                goto end1;
            }

            ed.flags = RJS_PROP_FL_HAS_VALUE;
            rjs_value_copy(rt, ed.value, pv);

            r = rjs_object_define_own_property(rt, receiver, pn, &ed);
        } else {
            r = rjs_create_data_property(rt, receiver, pn, pv);
        }

end1:
        rjs_property_desc_deinit(rt, &ed);
    } else {
        if (rjs_value_is_undefined(rt, pd.set)) {
            r = RJS_FALSE;
            goto end;
        }

        r = rjs_call(rt, pd.set, receiver, pv, 1, tmp);
    }
end:
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Delete a property of an ordinary object.
 * \param rt The current runtime.
 * \param v The object value.
 * \param pn The property name.
 * \retval RJS_TRUE The property is deleted.
 * \retval RJS_FALSE The property is not deleted.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_ordinary_object_op_delete (RJS_Runtime *rt, RJS_Value *v, RJS_PropertyName *pn)
{
    RJS_PropertyDesc pd;
    RJS_Result       r;
    size_t           top = rjs_value_stack_save(rt);

    rjs_property_desc_init(rt, &pd);

    if ((r = rjs_object_get_own_property(rt, v, pn, &pd)) == RJS_ERR)
        goto end;

    if (!r) {
        r = RJS_TRUE;
        goto end;
    }

    if (pd.flags & RJS_PROP_FL_CONFIGURABLE) {
        RJS_PropertyKey pk;

        prop_key_get(rt, pn->name, &pk);
        prop_delete(rt, v, &pk);

        r = RJS_TRUE;
    } else {
        r = RJS_FALSE;
    }
end:
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Add the object's own keys to the property key list.
 * \param rt The current runtime.
 * \param keysv The property key list value.
 * \param ov The object value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_property_key_list_add_own_keys (RJS_Runtime *rt, RJS_Value *keysv, RJS_Value *ov)
{
    RJS_PropertyKeyList *pkl = (RJS_PropertyKeyList*)rjs_value_get_gc_thing(rt, keysv);
    RJS_Object          *o   = rjs_value_get_object(rt, ov);
    RJS_PropertyNode    *pn;
    RJS_Value           *kv;

    /*Add array index.*/
    if (o->flags & RJS_OBJECT_FL_RBT) {
        RJS_Rbt *rbt;

        rbt = rjs_rbt_first(&o->prop_array.rbt);
        while (rbt) {
            RJS_PropertyRbt *pr = (RJS_PropertyRbt*)rbt;

            kv = pkl->keys.items + pkl->keys.item_num ++;

            rjs_value_set_index_string(rt, kv, pr->index);

            rbt = rjs_rbt_next(rbt);
        }
    } else if (o->array_item_cap) {
        size_t i;

        for (i = 0; i <= o->array_item_max; i ++) {
            RJS_Property *prop = &o->prop_array.vec[i];

            if (prop->attrs & RJS_PROP_ATTR_DELETED)
                continue;

            kv = pkl->keys.items + pkl->keys.item_num ++;

            rjs_value_set_index_string(rt, kv, i);
        }
    }

    /*Add strings.*/
    rjs_list_foreach_c(&o->prop_list, pn, RJS_PropertyNode, ln) {
        RJS_GcThing *gt = pn->he.key;

        if (gt->ops->type == RJS_GC_THING_STRING) {
            kv = pkl->keys.items + pkl->keys.item_num ++;

            rjs_value_set_string(rt, kv, (RJS_String*)gt);
        }
    }

    /*Add symbols.*/
    rjs_list_foreach_c(&o->prop_list, pn, RJS_PropertyNode, ln) {
        RJS_GcThing *gt = pn->he.key;

        if (gt->ops->type == RJS_GC_THING_SYMBOL) {
            kv = pkl->keys.items + pkl->keys.item_num ++;

            rjs_value_set_symbol(rt, kv, (RJS_Symbol*)gt);
        }
    }

    return RJS_OK;
}

/**
 * Get the own properties' keys of an ordinary object.
 * \param rt The current runtime.
 * \param v The object value.
 * \param[out] keys Return the object's keys list object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_ordinary_object_op_own_property_keys (RJS_Runtime *rt, RJS_Value *v, RJS_Value *keys)
{
    RJS_Object *o;

    o = rjs_value_get_object(rt, v);
    
    rjs_property_key_list_new(rt, keys, o->prop_hash.entry_num + o->array_item_num);

    return rjs_property_key_list_add_own_keys(rt, keys, v);
}

/**
 * Delete the array index key type properties from index.
 * \param rt The current runtime.
 * \param o The object.
 * \param old_len The old length.
 * \param new_len The new length.
 * \param[out] last_idx Return the last index after deleted.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE Some properties cannot be deleted.
 */
extern RJS_Result
rjs_ordinary_object_delete_from_index (RJS_Runtime *rt, RJS_Value *v,
        uint32_t old_len, uint32_t new_len, uint32_t *last_idx)
{
    RJS_Object *o = rjs_value_get_object(rt, v);
    RJS_Result  r = RJS_TRUE;
    uint32_t    n = 0;

    if (o->flags & RJS_OBJECT_FL_RBT) {
        RJS_Rbt *rbt;

        rbt = rjs_rbt_last(&o->prop_array.rbt);
        while (rbt) {
            RJS_PropertyRbt *pr = (RJS_PropertyRbt*)rbt;

            if (pr->index < new_len)
                break;

            if (!(pr->prop.attrs & RJS_PROP_ATTR_CONFIGURABLE)) {
                *last_idx = pr->index;
                r = RJS_FALSE;
                break;
            }

            rjs_rbt_remove(&o->prop_array.rbt, rbt);

            RJS_DEL(rt, pr);

            n ++;

            rbt = rjs_rbt_last(&o->prop_array.rbt);
        }
    } else if (o->prop_array.vec) {
        ssize_t i;

        for (i = o->array_item_max; i >= (ssize_t)new_len; i --) {
            RJS_Property *prop = &o->prop_array.vec[i];

            if (prop->attrs & RJS_PROP_ATTR_DELETED)
                continue;

            if (!(prop->attrs & RJS_PROP_ATTR_CONFIGURABLE)) {
                *last_idx = i;
                r = RJS_FALSE;
                break;
            }

            prop->attrs |= RJS_PROP_ATTR_DELETED;

            n ++;
        }
    }

    update_array_item_max(rt, v);
    prop_array_fixup(rt, v, o->array_item_num - n, o->array_item_max);

    return r;
}

/**
 * Initialize the object.
 * \param rt The current runtime.
 * \param v The value to store the object.
 * \param o The object to be initialized.
 * \param proto The prototype value.
 * \param ops The object's operation functions.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_object_init (RJS_Runtime *rt, RJS_Value *v, RJS_Object *o, RJS_Value *proto, const RJS_ObjectOps *ops)
{
    if (!ops)
        ops = &ordinary_object_ops;
        
    o->flags           = RJS_OBJECT_FL_EXTENSIBLE;
    o->array_item_max  = 0;
    o->array_item_cap  = 0;
    o->array_item_num  = 0;
    o->prop_array.vec  = NULL;

    if (proto)
        rjs_value_copy(rt, &o->prototype, proto);
    else
        rjs_value_set_null(rt, &o->prototype);

    rjs_list_init(&o->prop_list);
    rjs_hash_init(&o->prop_hash);

    rjs_value_set_object(rt, v, o);
    rjs_gc_add(rt, o, &ops->gc_thing_ops);

    return RJS_OK;
}

/**
 * Release the object.
 * \param rt The current runtime.
 * \param o The object to be released.
 */
void
rjs_object_deinit (RJS_Runtime *rt, RJS_Object *o)
{
    RJS_PropertyNode *pn, *tpn;

    rjs_hash_deinit(&o->prop_hash, &rjs_hash_size_ops, rt);

    rjs_list_foreach_safe_c(&o->prop_list, pn, tpn, RJS_PropertyNode, ln) {
        RJS_DEL(rt, pn);
    }

    if (o->flags & RJS_OBJECT_FL_RBT) {
        if (o->prop_array.rbt)
            prop_rbt_free_all(rt, o->prop_array.rbt);
    } else {
        if (o->array_item_cap)
            RJS_DEL_N(rt, o->prop_array.vec, o->array_item_cap);
    }
}

/**
 * Create a new object.
 * \param rt The current runtime.
 * \param[out] v Return the object value.
 * \param proto The prototype object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_object_new (RJS_Runtime *rt, RJS_Value *v, RJS_Value *proto)
{
    RJS_Object *o;

    RJS_NEW(rt, o);

    if (!proto)
        proto = rjs_o_Object_prototype(rjs_realm_current(rt));

    return rjs_object_init(rt, v, o, proto, &ordinary_object_ops);
}

/**
 * Convert the ordinary object to primitive type.
 * \param rt The current runtime.
 * \param v The object value.
 * \param[out] prim Return the primitive type.
 * \param type Prefered value type.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_ordinary_to_primitive (RJS_Runtime *rt, RJS_Value *v, RJS_Value *prim, RJS_ValueType type)
{
    size_t            top     = rjs_value_stack_save(rt);
    RJS_Value        *to_prim = rjs_value_stack_push(rt);
    RJS_Result        r;
    RJS_PropertyName *funcs[2];
    int               i;

    if (type == RJS_VALUE_STRING) {
        funcs[0] = rjs_pn_toString(rt);
        funcs[1] = rjs_pn_valueOf(rt);
    } else {
        funcs[0] = rjs_pn_valueOf(rt);
        funcs[1] = rjs_pn_toString(rt);
    }

    for (i = 0; i < 2; i ++) {
        RJS_PropertyName *func = funcs[i];

        if ((r = rjs_get(rt, v, func, to_prim)) == RJS_ERR)
            goto end;

        if (rjs_is_callable(rt, to_prim)) {
            if ((r = rjs_call(rt, to_prim, v, NULL, 0, prim)) == RJS_ERR)
                goto end;

            if (!rjs_value_is_object(rt, prim)) {
                r = RJS_OK;
                goto end;
            }
        }
    }

    r = rjs_throw_type_error(rt, _("cannot convert the value to primitive"));
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Convert the object to primitive type.
 * \param rt The current runtime.
 * \param v The object value.
 * \param[out] prim Return the primitive type.
 * \param type Prefered value type.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_object_to_primitive (RJS_Runtime *rt, RJS_Value *v, RJS_Value *prim, RJS_ValueType type)
{
    size_t     top     = rjs_value_stack_save(rt);
    RJS_Value *to_prim = rjs_value_stack_push(rt);
    RJS_Value *hint    = rjs_value_stack_push(rt);
    RJS_Result r;
    
    if ((r = rjs_get_method(rt, v, rjs_pn_s_toPrimitive(rt), to_prim)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_undefined(rt, to_prim)) {
        switch (type) {
        case RJS_VALUE_STRING:
            rjs_value_copy(rt, hint, rjs_s_string(rt));
            break;
        case RJS_VALUE_NUMBER:
            rjs_value_copy(rt, hint, rjs_s_number(rt));
            break;
        default:
            rjs_value_copy(rt, hint, rjs_s_default(rt));
            break;
        }

        if ((r = rjs_call(rt, to_prim, v, hint, 1, prim)) == RJS_ERR)
            goto end;

        if (rjs_value_is_object(rt, prim)) {
            r = rjs_throw_type_error(rt, _("\"@@toPrimitive\" return an object"));
            goto end;
        }
    } else {
        r = rjs_ordinary_to_primitive(rt, v, prim, type);
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Convert the object to number.
 * \param rt The current runtime.
 * \param v The object value.
 * \param[out] pn Return the number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_object_to_number (RJS_Runtime *rt, RJS_Value *v, RJS_Number *pn)
{
    size_t     top  = rjs_value_stack_save(rt);
    RJS_Value *prim = rjs_value_stack_push(rt);
    RJS_Result r;

    r = rjs_object_to_primitive(rt, v, prim, RJS_VALUE_NUMBER);
    if (r == RJS_OK)
        r = rjs_to_number(rt, prim, pn);

    rjs_value_stack_restore(rt, top);

    return r;
}

/**
 * Assign the properies of destination object to the source.
 * \param rt The current runtime.
 * \param dst The destination object.
 * \param src The source object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_object_assign (RJS_Runtime *rt, RJS_Value *dst, RJS_Value *src)
{
    size_t           top  = rjs_value_stack_save(rt);
    RJS_Value       *kv   = rjs_value_stack_push(rt);
    RJS_Value       *keys = rjs_value_stack_push(rt);
    RJS_Value       *from = rjs_value_stack_push(rt);
    RJS_PropertyDesc pd;
    RJS_Result       r;

    rjs_property_desc_init(rt, &pd);

    if (!rjs_value_is_undefined(rt, src) && !rjs_value_is_null(rt, src)) {
        RJS_PropertyKeyList *pkl;
        size_t               kid;

        if ((r = rjs_to_object(rt, src, from)) == RJS_ERR)
            goto end;

        if ((r = rjs_object_own_property_keys(rt, from, keys)) == RJS_ERR)
            goto end;

        pkl = rjs_value_get_gc_thing(rt, keys);
        for (kid = 0; kid < pkl->keys.item_num; kid ++) {
            RJS_PropertyName pn;
            RJS_Value       *key = &pkl->keys.items[kid];

            rjs_property_name_init(rt, &pn, key);
            r = rjs_object_get_own_property(rt, from, &pn, &pd);

            if ((r == RJS_OK) && (pd.flags & RJS_PROP_FL_ENUMERABLE)) {
                if ((r = rjs_get(rt, from, &pn, kv)) == RJS_OK) {
                    r = rjs_set(rt, dst, &pn, kv, RJS_TRUE);
                }
            }
            rjs_property_name_deinit(rt, &pn);

            if (r == RJS_ERR)
                goto end;
        }
    }

    r = RJS_OK;
end:
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}
