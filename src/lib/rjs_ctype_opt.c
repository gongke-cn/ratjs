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

/**C type.*/
struct RJS_CType_s {
    RJS_HashEntry  he;    /**< The hash table entry.*/
    RJS_CTypeModel model; /**< The type's module.*/
    size_t         size;  /**< Size of the type.*/
    void          *data;  /**< The data of the type.*/
};

/**C pointer.*/
typedef struct {
    RJS_Object     o;     /**< Base object data.*/
    RJS_HashEntry  he;    /**< The hash table entry.*/
    RJS_CPtrInfo   info;  /**< The pointer information.*/
    int            flags; /**< The flags of the pointer.*/
} RJS_CPtr;

/*Pointer hash key function.*/
static size_t
ptr_hash_op_key (void *data, void *k)
{
    RJS_CPtrInfo *p = k;

    return RJS_PTR2SIZE(p->ptr);
}

/*Pointer hash key equal check function.*/
static RJS_Bool
ptr_hash_op_equal (void *data, void *k1, void *k2)
{
    RJS_CPtrInfo *p1 = k1;
    RJS_CPtrInfo *p2 = k2;

    return (p1->ptr == p2->ptr)
            && (p1->ctype == p2->ctype)
            && (p1->nitem == p2->nitem)
            && (p1->ptype == p2->ptype);
}

/*C pointer hash table operations.*/
static const RJS_HashOps
ptr_hash_ops = {
    rjs_hash_op_realloc,
    ptr_hash_op_key,
    ptr_hash_op_equal
};

/*Free the C type.*/
static void
ctype_free (RJS_Runtime *rt, RJS_CType *ty)
{
    RJS_DEL(rt, ty);
}

/*Scan the reference things in the C pointer.*/
static void
cptr_op_gc_scan (RJS_Runtime *rt, void *p)
{
    RJS_CPtr *cptr = p;

    rjs_object_op_gc_scan(rt, &cptr->o);
}

/*Free the C pointer*/
static void
cptr_op_gc_free (RJS_Runtime *rt, void *p)
{
    RJS_CPtr *cptr = p;

    /*Remove from hash table.*/
    rjs_cptr_remove(rt, &cptr->info);

    rjs_object_deinit(rt, &cptr->o);

    /*Free the C pointr.*/
    if (cptr->flags & RJS_CPTR_FL_AUTO_FREE) {
        if (cptr->info.ptr) {
            if (cptr->info.ptype == RJS_CPTR_TYPE_PTR_ARRAY) {
                void **ip  = (void**)cptr->info.ptr;
                void **lip = ip + cptr->info.nitem;

                while (ip < lip) {
                    if (*ip)
                        free(*ip);
                    ip ++;
                }
            }
            
            free(cptr->info.ptr);
        }
    }

    RJS_DEL(rt, cptr);
}

/*Get the C pointer's own property keys.*/
static RJS_Result
cptr_op_own_property_keys (RJS_Runtime *rt, RJS_Value *o, RJS_Value *keys)
{
    RJS_CPtr            *cptr  = (RJS_CPtr*)rjs_value_get_object(rt, o);
    RJS_CType           *ctype = cptr->info.ctype;
    RJS_PropertyKeyList *pkl;
    size_t               cap;
    RJS_Value           *kv;
    size_t               pn    = 0;
    RJS_Object          *proto = NULL;

    if (ctype->model == RJS_CTYPE_MODEL_STRUCT) {
        proto = ctype->data;

        pn = proto->prop_hash.entry_num;
    }

    cap = cptr->o.prop_hash.entry_num + cptr->o.array_item_num + pn;
    pkl = rjs_property_key_list_new(rt, keys, cap);

    if (pn) {
        RJS_PropertyNode *n;

        rjs_list_foreach_c(&proto->prop_list, n, RJS_PropertyNode, ln) {
            if (n->prop.attrs & RJS_PROP_ATTR_ACCESSOR) {
                kv = pkl->keys.items + pkl->keys.item_num ++;

                rjs_value_set_string(rt, kv, n->he.key);
            }
        }
    }

    rjs_property_key_list_add_own_keys(rt, keys, o);
    return RJS_OK;
}

/*C pointer operation functions.*/
static const RJS_ObjectOps
cptr_ops = {
    {
        RJS_GC_THING_CPTR,
        cptr_op_gc_scan,
        cptr_op_gc_free
    },
    rjs_ordinary_object_op_get_prototype_of,
    rjs_ordinary_object_op_set_prototype_of,
    rjs_ordinary_object_op_is_extensible,
    rjs_ordinary_object_op_prevent_extensions,
    rjs_ordinary_object_op_get_own_property,
    rjs_ordinary_object_op_define_own_property,
    rjs_ordinary_object_op_has_property,
    rjs_ordinary_object_op_get,
    rjs_ordinary_object_op_set,
    rjs_ordinary_object_op_delete,
    cptr_op_own_property_keys
};

/*Call the C function.*/
static inline RJS_Result
cptr_op_call (RJS_Runtime *rt, RJS_Value *o, RJS_Value *thiz, RJS_Value *args, size_t argc, RJS_Value *rv)
{
    RJS_CPtr      *cptr = (RJS_CPtr*)rjs_value_get_object(rt, o);
    RJS_CType     *cty  = cptr->info.ctype;
    RJS_NativeFunc nf   = cty->data;

    assert(cty->model == RJS_CTYPE_MODEL_FUNC);

    if (nf == NULL)
        return rjs_throw_type_error(rt, _("the function is null"));

    return nf(rt, o, thiz, args, argc, NULL, rv);
}

/*C pointer function operation functions.*/
static const RJS_ObjectOps
cptr_func_ops = {
    {
        RJS_GC_THING_CPTR,
        cptr_op_gc_scan,
        cptr_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS,
    cptr_op_call
};

/*Get the C array's item index.*/
static RJS_Result
cptr_get_item_index (RJS_Runtime *rt, RJS_CPtr *cptr, RJS_Value *p, size_t *pi)
{
    RJS_Result r;
    RJS_Number n;
    uint64_t   idx;

    if (!cptr->info.ptr)
        return RJS_FALSE;

    if (rjs_value_is_string(rt, p)) {
        if ((r = rjs_to_number(rt, p, &n)) == RJS_ERR)
            return r;
    } else {
        return RJS_FALSE;
    }

    if (isinf(n) || isnan(n))
        return RJS_FALSE;

    if (signbit(n))
        return RJS_FALSE;

    if (floor(n) != n)
        return RJS_FALSE;

    idx = n;

    if (idx >= cptr->info.nitem)
        return RJS_FALSE;

    *pi = idx;
    return RJS_TRUE;
}

/*Get the C pointer array's own property.*/
static RJS_Result
cptr_array_op_get_own_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_PropertyDesc *pd)
{
    RJS_CPtr   *cptr  = (RJS_CPtr*) rjs_value_get_object(rt, o);
    RJS_CType  *ctype = cptr->info.ctype;
    size_t      idx;
    void       *iptr;
    RJS_Result  r;

    r = rjs_ordinary_object_op_get_own_property(rt, o, pn, pd);
    if (r)
        return r;

    if ((r = cptr_get_item_index(rt, cptr, pn->name, &idx)) != RJS_OK)
        return r;

    pd->flags = RJS_PROP_FL_DATA|RJS_PROP_FL_ENUMERABLE;

    if (!(cptr->flags & RJS_CPTR_FL_READONLY))
        pd->flags |= RJS_PROP_FL_WRITABLE;

    if (cptr->info.ptype == RJS_CPTR_TYPE_PTR_ARRAY) {
        iptr = ((void**)cptr->info.ptr)[idx];

        if (!iptr) {
            rjs_value_set_null(rt, pd->value);
            return RJS_OK;
        }
    } else {
        iptr = ((uint8_t*)cptr->info.ptr) + ctype->size * idx;
    }

    if ((r = rjs_create_c_ptr(rt, ctype, iptr, RJS_CPTR_TYPE_VALUE, 1, 0, pd->value)) == RJS_ERR)
        return r;

    return RJS_OK;
}

/**Define an own property of a C pointer array.*/
extern RJS_Result
cptr_array_op_define_own_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_PropertyDesc *pd)
{
    RJS_CPtr        *cptr  = (RJS_CPtr*) rjs_value_get_object(rt, o);
    RJS_CType       *ctype = cptr->info.ctype;
    size_t           idx;
    RJS_Result       r;
    RJS_PropertyDesc old;
    size_t           top   = rjs_value_stack_save(rt);
    RJS_Value       *icptr = rjs_value_stack_push(rt);

    rjs_property_desc_init(rt, &old);

    if ((r = cptr_get_item_index(rt, cptr, pn->name, &idx)) == RJS_ERR)
        goto end;

    if (r) {
        if (pd->flags & (RJS_PROP_FL_HAS_GET|RJS_PROP_FL_HAS_SET)) {
            r = RJS_FALSE;
            goto end;
        }

        if (pd->flags & RJS_PROP_FL_HAS_CONFIGURABLE) {
            if (pd->flags & RJS_PROP_FL_CONFIGURABLE) {
                r = RJS_FALSE;
                goto end;
            }
        }

        if (pd->flags & RJS_PROP_FL_HAS_ENUMERABLE) {
            if (!(pd->flags & RJS_PROP_FL_ENUMERABLE)) {
                r = RJS_FALSE;
                goto end;
            }
        }

        if (pd->flags & RJS_PROP_FL_HAS_WRITABLE) {
            RJS_Bool b1 = (cptr->flags & RJS_CPTR_FL_READONLY) ? RJS_FALSE : RJS_TRUE;
            RJS_Bool b2 = pd->flags & RJS_PROP_FL_WRITABLE ? RJS_TRUE : RJS_FALSE;

            if (b1 != b2) {
                r = RJS_FALSE;
                goto end;
            }
        }

        if (pd->flags & RJS_PROP_FL_HAS_VALUE) {
            void *item;

            if (cptr->info.ptype == RJS_CPTR_TYPE_PTR_ARRAY) {
                item = ((void**)cptr->info.ptr)[idx];
            } else {
                item = ((uint8_t*)cptr->info.ptr) + ctype->size * idx;
            }

            if (rjs_value_get_gc_thing_type(rt, pd->value) == RJS_GC_THING_CPTR) {
                RJS_CPtr *nptr;

                nptr = (RJS_CPtr*)rjs_value_get_object(rt, pd->value);

                if ((nptr->info.ctype != ctype) && (nptr->info.ptype != RJS_CPTR_TYPE_VALUE)) {
                    r = rjs_throw_type_error(rt, _("C type mismatch"));
                    goto end;
                }

                if (nptr->info.ptr != item) {
                    if (cptr->flags & RJS_CPTR_FL_READONLY) {
                        r = rjs_throw_type_error(rt, _("the C array is readonly"));
                        goto end;
                    }

                    if (cptr->info.ptype == RJS_CPTR_TYPE_PTR_ARRAY) {
                        if (cptr->flags & RJS_CPTR_FL_AUTO_FREE) {
                            if (item)
                                free(item);
                        }

                        ((void**)cptr->info.ptr)[idx] = nptr->info.ptr;
                    } else if (ctype->size == 0) {
                        r = rjs_throw_type_error(rt, _("unknown C type size"));
                        goto end;
                    } else {
                        memcpy(item, nptr->info.ptr, ctype->size);
                    }
                }
            } else {
                if (!item) {
                    r = rjs_throw_type_error(rt, _("the item pointer is null"));
                    goto end;
                }

                if ((r = rjs_create_c_ptr(rt, ctype, item, RJS_CPTR_TYPE_VALUE, 1, 0, icptr)) == RJS_ERR)
                    goto end;

                if ((r = rjs_object_assign(rt, icptr, pd->value)) == RJS_ERR)
                    goto end;
            }
        }

        r = RJS_OK;
    } else {
        r = rjs_ordinary_object_op_define_own_property(rt, o, pn, pd);
    }
end:
    rjs_property_desc_deinit(rt, &old);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Get the C pointer array's own property keys.*/
static RJS_Result
cptr_array_op_own_property_keys (RJS_Runtime *rt, RJS_Value *o, RJS_Value *keys)
{
    RJS_CPtr            *cptr = (RJS_CPtr*)rjs_value_get_object(rt, o);
    RJS_PropertyKeyList *pkl;
    size_t               cap, len, i;
    RJS_Value           *kv;
    size_t               top  = rjs_value_stack_save(rt);
    RJS_Value           *idx  = rjs_value_stack_push(rt);

    len = cptr->info.nitem;
    cap = len + cptr->o.prop_hash.entry_num + cptr->o.array_item_num;

    pkl = rjs_property_key_list_new(rt, keys, cap);

    for (i = 0; i < len; i ++) {
        kv = pkl->keys.items + pkl->keys.item_num ++;

        rjs_value_set_number(rt, idx, i);
        rjs_to_string(rt, idx, kv);
    }

    rjs_property_key_list_add_own_keys(rt, keys, o);

    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}

/*C pointer array operation functions.*/
static const RJS_ObjectOps
cptr_array_ops = {
    {
        RJS_GC_THING_CPTR,
        cptr_op_gc_scan,
        cptr_op_gc_free
    },
    rjs_ordinary_object_op_get_prototype_of,
    rjs_ordinary_object_op_set_prototype_of,
    rjs_ordinary_object_op_is_extensible,
    rjs_ordinary_object_op_prevent_extensions,
    cptr_array_op_get_own_property,
    cptr_array_op_define_own_property,
    rjs_ordinary_object_op_has_property,
    rjs_ordinary_object_op_get,
    rjs_ordinary_object_op_set,
    rjs_ordinary_object_op_delete,
    cptr_array_op_own_property_keys
};

/**
 * Lookup a C type by its name.
 * \param rt The current runtime.
 * \param name The name of the C type.
 * \param[out] pty Return the C type.
 * \retval RJS_OK On success.
 * \retval RJS_ERR Cannot find the C type.
 */
RJS_Result
rjs_get_c_type (RJS_Runtime *rt, RJS_Value *name, RJS_CType **pty)
{
    RJS_String    *str;
    RJS_CType     *ty;
    RJS_HashEntry *he;
    RJS_Result     r;

    if ((r = rjs_string_to_property_key(rt, name)) == RJS_ERR)
        return r;

    str = rjs_value_get_string(rt, name);

    r = rjs_hash_lookup(&rt->ctype_hash, str, &he, NULL, &rjs_hash_size_ops, rt);
    if (!r) {
        return rjs_throw_type_error(rt, _("cannot find the C type \"%s\""),
                rjs_string_to_enc_chars(rt, name, NULL, NULL));
    }

    ty = RJS_CONTAINER_OF(he, RJS_CType, he);

    *pty = ty;
    return RJS_OK;
}

/**
 * Create a new C type.
 * \param rt The current runtime.
 * \param name The name of the C type.
 * \param model The type model.
 * \param size Size of the type in bytes.
 * \param data The private data of the type.
 * If model == RJS_CTYPE_MODEL_STRUCT, data is the prototype value' pointer.
 * If model == RJS_CTYPE_MODEL_FUNC, data is the native wrapper function's pointer.
 * \param[out] pty Return the C type.
 * \retval RJS_OK On success.
 * \retval RJS_ERR Cannot find the C type.
 */
RJS_Result
rjs_create_c_type (RJS_Runtime *rt, RJS_Value *name, RJS_CTypeModel model,
        size_t size, void *data, RJS_CType **pty)
{
    RJS_String    *str;
    RJS_CType     *ty;
    RJS_HashEntry *he, **phe;
    RJS_Result     r;

    if ((r = rjs_string_to_property_key(rt, name)) == RJS_ERR)
        return r;

    str = rjs_value_get_string(rt, name);

    r = rjs_hash_lookup(&rt->ctype_hash, str, &he, &phe, &rjs_hash_size_ops, rt);
    if (r) {
        return rjs_throw_type_error(rt, _("C type \"%s\" is already defined"),
                rjs_string_to_enc_chars(rt, name, NULL, NULL));
    }

    RJS_NEW(rt, ty);

    ty->model = model;
    ty->size  = size;

    if (ty->model == RJS_CTYPE_MODEL_STRUCT) {
        if (data) {
            RJS_Value *proto = data;

            ty->data = rjs_value_get_object(rt, proto);
        }
    } else {
        ty->data = data;
    }

    rjs_hash_insert(&rt->ctype_hash, str, &ty->he, phe, &rjs_hash_size_ops, rt);

    *pty = ty;
    return RJS_OK;
}

/**
 * Create a C pointer object.
 * \param rt The current runtime.
 * \param ty The C type of the value.
 * \param p The pointer.
 * \param ptype The pointer's type.
 * \param nitem Number of items in the pointer.
 * If ptype == RJS_CPTR_TYPE_VALUE, it must be 1.
 * \param flags Flags of the pointer.
 * \param[out] rv Return the object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_create_c_ptr (RJS_Runtime *rt, RJS_CType *ty, void *p,
        RJS_CPtrType ptype, size_t nitem, int flags, RJS_Value *rv)
{
    size_t         top   = rjs_value_stack_save(rt);
    RJS_Value     *proto = rjs_value_stack_push(rt);
    RJS_Value     *lenv  = rjs_value_stack_push(rt);
    RJS_Realm     *realm = rjs_realm_current(rt);
    RJS_CPtrInfo   key;
    RJS_CPtr      *ptr;
    RJS_HashEntry *he, **phe;
    RJS_Result     r;
    const RJS_ObjectOps *ops;

    assert(ty && p && rv);

    if ((ptype == RJS_CPTR_TYPE_ARRAY) && (ty->size == 0)) {
        r = rjs_throw_type_error(rt, _("unknown C type size"));
        goto end;
    }

    key.ptr   = p;
    key.ctype = ty;
    key.ptype = ptype;
    key.nitem = nitem;

    r = rjs_hash_lookup(&rt->cptr_hash, &key, &he, &phe, &ptr_hash_ops, rt);
    if (r) {
        ptr = RJS_CONTAINER_OF(he, RJS_CPtr, he);
        rjs_value_set_object(rt, rv, &ptr->o);
        r = RJS_OK;
        goto end;
    }

    RJS_NEW(rt, ptr);

    if ((ptype == RJS_CPTR_TYPE_ARRAY) || (ptype == RJS_CPTR_TYPE_PTR_ARRAY)) {
        rjs_value_copy(rt, proto, rjs_o_Array_prototype(realm));
    } else {
        if (ty->model == RJS_CTYPE_MODEL_STRUCT) {
            rjs_value_set_object(rt, proto, ty->data);
        } else {
            rjs_value_copy(rt, proto, rjs_o_Function_prototype(realm));
        }
    }

    ptr->info.ptr   = p;
    ptr->info.ctype = ty;
    ptr->info.ptype = ptype;
    ptr->info.nitem = nitem;
    ptr->flags      = flags;

    rjs_hash_insert(&rt->cptr_hash, &ptr->info, &ptr->he, phe, &ptr_hash_ops, rt);

    switch (ptype) {
    case RJS_CPTR_TYPE_VALUE:
        if (ty->model == RJS_CTYPE_MODEL_FUNC)
            ops = &cptr_func_ops;
        else
            ops = &cptr_ops;
        break;
    case RJS_CPTR_TYPE_ARRAY:
    case RJS_CPTR_TYPE_PTR_ARRAY:
        ops = &cptr_array_ops;
        break;
    default:
        assert(0);
        break;
    }

    if ((r = rjs_object_init(rt, rv, &ptr->o, proto, ops)) == RJS_ERR) {
        RJS_DEL(rt, ptr);
        goto end;
    }

    if (ptype == RJS_CPTR_TYPE_ARRAY) {
        rjs_value_set_number(rt, lenv, nitem);
        if ((r = rjs_create_data_property_or_throw(rt, rv, rjs_pn_length(rt), lenv)) == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

#if ENABLE_INT_INDEXED_OBJECT

/**
 * Create a typed array pointed to a C buffer.
 * \param rt The current runtime.
 * \param et The element type.
 * \param p The pointer.
 * \param nitem Number of elements in the buffer.
 * \param[out] rv Return the object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_create_c_typed_array (RJS_Runtime *rt, RJS_ArrayElementType et, void *p,
        size_t nitem, RJS_Value *rv)
{
    size_t         top = rjs_value_stack_save(rt);
    RJS_Value     *ab  = rjs_value_stack_push(rt);
    RJS_Value     *ta  = rjs_value_stack_push(rt);
    RJS_CPtrInfo   key;
    RJS_HashEntry *he, **phe;
    RJS_DataBlock *db;
    RJS_ArrayBuffer      *a;
    RJS_IntIndexedObject *iio;
    size_t         len;
    RJS_Result     r;

    key.ptr   = p;
    key.ctype = NULL;
    key.nitem = nitem;

    switch (et) {
    case RJS_ARRAY_ELEMENT_UINT8:
        key.ptype = RJS_CPTR_TYPE_UINT8_ARRAY;
        break;
    case RJS_ARRAY_ELEMENT_INT8:
        key.ptype = RJS_CPTR_TYPE_INT8_ARRAY;
        break;
    case RJS_ARRAY_ELEMENT_UINT8C:
        key.ptype = RJS_CPTR_TYPE_UINT8C_ARRAY;
        break;
    case RJS_ARRAY_ELEMENT_UINT16:
        key.ptype = RJS_CPTR_TYPE_UINT16_ARRAY;
        break;
    case RJS_ARRAY_ELEMENT_INT16:
        key.ptype = RJS_CPTR_TYPE_INT16_ARRAY;
        break;
    case RJS_ARRAY_ELEMENT_UINT32:
        key.ptype = RJS_CPTR_TYPE_UINT32_ARRAY;
        break;
    case RJS_ARRAY_ELEMENT_INT32:
        key.ptype = RJS_CPTR_TYPE_INT32_ARRAY;
        break;
    case RJS_ARRAY_ELEMENT_FLOAT32:
        key.ptype = RJS_CPTR_TYPE_FLOAT32_ARRAY;
        break;
    case RJS_ARRAY_ELEMENT_FLOAT64:
        key.ptype = RJS_CPTR_TYPE_FLOAT64_ARRAY;
        break;
    case RJS_ARRAY_ELEMENT_BIGUINT64:
        key.ptype = RJS_CPTR_TYPE_UINT64_ARRAY;
        break;
    case RJS_ARRAY_ELEMENT_BIGINT64:
        key.ptype = RJS_CPTR_TYPE_INT64_ARRAY;
        break;
    default:
        assert(0);
    }

    r = rjs_hash_lookup(&rt->cptr_hash, &key, &he, &phe, &ptr_hash_ops, rt);
    if (r) {
        iio = RJS_CONTAINER_OF(he, RJS_IntIndexedObject, cptr_he);
        rjs_value_set_object(rt, rv, &iio->object);
        r = RJS_OK;
        goto end;
    }

    /*Allocate a new typed array.*/
    len = rjs_typed_array_element_size(et) * nitem;

    if (!(db = rjs_data_block_new(p, len, RJS_DATA_BLOCK_FL_EXTERN)))
        goto end;

    if ((r = rjs_allocate_array_buffer(rt, NULL, len, ab)) == RJS_ERR)
        goto end;

    a = (RJS_ArrayBuffer*)rjs_value_get_object(rt, ab);
    a->data_block = db;

    if ((r = rjs_create_typed_array(rt, et, ab, 1, ta)) == RJS_ERR)
        goto end;

    /*Add the typed array to hash table.*/
    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, ta);

    iio->cptr_info.ptr   = p;
    iio->cptr_info.ctype = NULL;
    iio->cptr_info.ptype = key.ptype;
    iio->cptr_info.nitem = nitem;

    rjs_hash_insert(&rt->cptr_hash, &iio->cptr_info, &iio->cptr_he, phe, &ptr_hash_ops, rt);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

#endif /*ENABLE_INT_INDEXED_OBJECT*/

/**
 * Get the C pointer from the value.
 * \param rt The current runtime.
 * \param ty The C type of the value.
 * \param ptype The pointer type.
 * \param cptrv The C pointer value.
 * \return The real C pointer.
 * \retval NULL The value is not a valid C pointer.
 */
void*
rjs_get_c_ptr (RJS_Runtime *rt, RJS_CType *ty, RJS_CPtrType ptype, RJS_Value *cptrv)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *str = rjs_value_stack_push(rt);
    RJS_CPtr  *cptr;
    void      *r   = NULL;

    if (rjs_value_get_gc_thing_type(rt, cptrv) != RJS_GC_THING_CPTR) {
        rjs_throw_type_error(rt, _("the value is not a C pointer"));
        goto end;
    }

    cptr = (RJS_CPtr*)rjs_value_get_object(rt, cptrv);

    if (cptr->info.ctype != ty) {
        rjs_value_set_string(rt, str, ty->he.key);
        rjs_throw_type_error(rt, _("the pointer is not in type \"%s\""),
                rjs_string_to_enc_chars(rt, str, NULL, NULL));
        goto end;
    }

    if (cptr->info.ptype != ptype) {
        rjs_throw_type_error(rt, _("the C pointer type mismatch"));
        goto end;
    }

    r = cptr->info.ptr;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Get the length of the C array.
 * @param rt The current runtime.
 * @param cptrv The C pointer.
 * @return The length of the C array.
 */
size_t
rjs_get_c_array_length (RJS_Runtime *rt, RJS_Value *cptrv)
{
    RJS_CPtr  *cptr;

    if (rjs_value_get_gc_thing_type(rt, cptrv) != RJS_GC_THING_CPTR) {
        rjs_throw_type_error(rt, _("the value is not a C pointer"));
        return (size_t)-1;
    }

    cptr = (RJS_CPtr*)rjs_value_get_object(rt, cptrv);

    return cptr->info.nitem;
}

/**
 * Remove the C pointer from the hash table.
 * \param rt The current runtime.
 * \param info The C pointer information.
 */
void
rjs_cptr_remove (RJS_Runtime *rt, RJS_CPtrInfo *info)
{
    RJS_HashEntry *he, **phe;
    RJS_Result     r;

    /*Remove from hash table.*/
    r = rjs_hash_lookup(&rt->cptr_hash, info, &he, &phe, &ptr_hash_ops, rt);
    assert(r);
    rjs_hash_remove(&rt->cptr_hash, phe, rt);
}

/**
 * Initialize the C type data in the rt.
 * \param rt The current runtime.
 */
void
rjs_runtime_ctype_init (RJS_Runtime *rt)
{
    rjs_hash_init(&rt->ctype_hash);
    rjs_hash_init(&rt->cptr_hash);
}

/**
 * Release the C type data in the rt.
 * \param rt The current runtime.
 */
void
rjs_runtime_ctype_deinit (RJS_Runtime *rt)
{
    RJS_CType *ty, *nty;
    size_t     i;

    rjs_hash_foreach_safe_c(&rt->ctype_hash, i, ty, nty, RJS_CType, he) {
        ctype_free(rt, ty);
    }

    rjs_hash_deinit(&rt->ctype_hash, &rjs_hash_size_ops, rt);
    rjs_hash_deinit(&rt->cptr_hash, &ptr_hash_ops, rt);
}

/**
 * Scan the C types in the runtime.
 * \param rt The current runtime.
 */
void
rjs_gc_scan_ctype (RJS_Runtime *rt)
{
    RJS_CType *ty;
    size_t     i;

    rjs_hash_foreach_c(&rt->ctype_hash, i, ty, RJS_CType, he) {
        /*Mark the name string.*/
        rjs_gc_mark(rt, ty->he.key);

        /*Scan the type's prototype object.*/
        if (ty->model == RJS_CTYPE_MODEL_STRUCT) {
            if (ty->data) {
                rjs_gc_mark(rt, ty->data);
            }
        }
    }
}
