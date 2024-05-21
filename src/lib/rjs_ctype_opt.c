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
    RJS_CPtrType  t1, t2;

    if (p1->ptype == RJS_CPTR_TYPE_C_WRAPPER)
        t1 = RJS_CPTR_TYPE_C_FUNC;
    else if (p1->ptype == RJS_CPTR_TYPE_UNKNOWN)
        t1 = p2->ptype;
    else
        t1 = p1->ptype;

    if (p2->ptype == RJS_CPTR_TYPE_C_WRAPPER)
        t2 = RJS_CPTR_TYPE_C_FUNC;
    else if (p2->ptype == RJS_CPTR_TYPE_UNKNOWN)
        t2 = p1->ptype;
    else
        t2 = p1->ptype;

    return (p1->ptr == p2->ptr)
            && (p1->ctype == p2->ctype)
            && (p1->nitem == p2->nitem)
            && (t1 == t2);
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
    if (ty->model == RJS_CTYPE_MODEL_FUNC) {
        if (ty->t.ffi.atypes)
            RJS_DEL_N(rt, ty->t.ffi.atypes, ty->t.ffi.nargs);
    }

    RJS_DEL(rt, ty);
}

/*Scan the reference things in the C pointer.*/
static void
cptr_op_gc_scan (RJS_Runtime *rt, void *p)
{
    RJS_CPtr *cptr = p;

    switch (cptr->info.ptype) {
    case RJS_CPTR_TYPE_C_WRAPPER:
        if (cptr->p.w.fo)
            rjs_gc_mark(rt, cptr->p.w.fo);
        break;
    default:
        break;
    }

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

    switch (cptr->info.ptype) {
    case RJS_CPTR_TYPE_C_FUNC:
        break;
    case RJS_CPTR_TYPE_C_WRAPPER:
        /*Free the ffi closure.*/
        if (cptr->p.w.closure)
            ffi_closure_free(cptr->p.w.closure);
        break;
    default:
        /*Free the C pointr.*/
        if (cptr->p.flags & RJS_CPTR_FL_AUTO_FREE) {
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
        break;
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
    RJS_Object          *proto = NULL;
    size_t               pn    = 0;

    if (ctype->model == RJS_CTYPE_MODEL_STRUCT) {
        if (rjs_value_is_object(rt, &ctype->t.prototype)) {
            proto = rjs_value_get_object(rt, &ctype->t.prototype);

            pn = proto->prop_hash.entry_num;
        }
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
    RJS_CPtr  *cptr = (RJS_CPtr*)rjs_value_get_object(rt, o);
    RJS_CType *cty  = cptr->info.ctype;
    ffi_cif   *cif  = &cty->t.ffi.cif;

    assert(cptr->info.ptype == RJS_CPTR_TYPE_C_FUNC);

    return cty->t.ffi.js2ffi(rt, cif, args, argc, cptr->info.ptr, rv, cty->t.ffi.data);
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

    if (!(cptr->p.flags & RJS_CPTR_FL_READONLY))
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

    if (ctype->model == RJS_CTYPE_MODEL_FUNC)
        r = rjs_create_c_func_ptr(rt, ctype, iptr, pd->value);
    else
        r = rjs_create_c_ptr(rt, ctype, iptr, RJS_CPTR_TYPE_VALUE, 1, 0, pd->value);

    return r;
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
            RJS_Bool b1 = (cptr->p.flags & RJS_CPTR_FL_READONLY) ? RJS_FALSE : RJS_TRUE;
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
                    if (cptr->p.flags & RJS_CPTR_FL_READONLY) {
                        r = rjs_throw_type_error(rt, _("the C array is readonly"));
                        goto end;
                    }

                    if (cptr->info.ptype == RJS_CPTR_TYPE_PTR_ARRAY) {
                        if (cptr->p.flags & RJS_CPTR_FL_AUTO_FREE) {
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

                if (ctype->model == RJS_CTYPE_MODEL_FUNC) {
                    r = rjs_create_c_func_ptr(rt, ctype, item, icptr);
                } else {
                    r = rjs_create_c_ptr(rt, ctype, item, RJS_CPTR_TYPE_VALUE, 1, 0, icptr);
                }
                if (r == RJS_ERR)
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
 * \param size Size of the type in bytes.
 * \param proto The prototype value.
 * \param[out] pty Return the C type.
 * \retval RJS_OK On success.
 * \retval RJS_ERR Cannot find the C type.
 */
RJS_Result
rjs_create_c_type (RJS_Runtime *rt, RJS_Value *name, size_t size,
        RJS_Value *proto, RJS_CType **pty)
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

    ty->model = RJS_CTYPE_MODEL_STRUCT;
    ty->size  = size;

    if (proto)
        rjs_value_copy(rt, &ty->t.prototype, proto);
    else
        rjs_value_set_null(rt, &ty->t.prototype);

    rjs_hash_insert(&rt->ctype_hash, str, &ty->he, phe, &rjs_hash_size_ops, rt);

    *pty = ty;
    return RJS_OK;
}

/**
 * Create a new C function type.
 * \param rt The current runtime.
 * \param name The name of the C type.
 * \param nargs Number of the function's arguments.
 * \param rtype The return value's type.
 * \param atypes The arguments' types.
 * \param js2ffi Javacript to FFI invoker.
 * \param ffi2js FFI 2 javascript invoker.
 * \param data The private data of the function type.
 * \param[out] pty Return the C type.
 * \retval RJS_OK On success.
 * \retval RJS_ERR Cannot find the C type.
 */
RJS_Result
rjs_create_c_func_type (RJS_Runtime *rt, RJS_Value *name, int nargs,
        ffi_type *rtype, ffi_type **atypes, RJS_JS2FFIFunc js2ffi,
        RJS_FFI2JSFunc ffi2js, void *data, RJS_CType **pty)
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

    ty->model = RJS_CTYPE_MODEL_FUNC;
    ty->size  = sizeof(void*);

    ty->t.ffi.nargs  = nargs;
    ty->t.ffi.js2ffi = js2ffi;
    ty->t.ffi.ffi2js = ffi2js;
    ty->t.ffi.data   = data;

    if (nargs) {
        RJS_NEW_N(rt, ty->t.ffi.atypes, nargs);
        RJS_ELEM_CPY(ty->t.ffi.atypes, atypes, nargs);
    } else {
        ty->t.ffi.atypes = NULL;
    }

    if (ffi_prep_cif(&ty->t.ffi.cif, FFI_DEFAULT_ABI, nargs, rtype, ty->t.ffi.atypes) != FFI_OK)
        return rjs_throw_type_error(rt, _("\"ffi_prep_cif\" failed"));

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
    assert((ty->model != RJS_CTYPE_MODEL_FUNC) || (ptype == RJS_CPTR_TYPE_ARRAY));

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
        rjs_value_copy(rt, proto, &ty->t.prototype);
    }

    ptr->info.ptr   = p;
    ptr->info.ctype = ty;
    ptr->info.ptype = ptype;
    ptr->info.nitem = nitem;
    ptr->p.flags    = flags;

    rjs_hash_insert(&rt->cptr_hash, &ptr->info, &ptr->he, phe, &ptr_hash_ops, rt);

    switch (ptype) {
    case RJS_CPTR_TYPE_VALUE:
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

    rjs_object_init(rt, rv, &ptr->o, proto, ops);

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
    key.ptype = et;

    r = rjs_hash_lookup(&rt->cptr_hash, &key, &he, &phe, &ptr_hash_ops, rt);
    if (r) {
        iio = RJS_CONTAINER_OF(he, RJS_IntIndexedObject, cptr_he);
        rjs_value_set_object(rt, rv, &iio->object);
        r = RJS_OK;
        goto end;
    }

    /*Allocate a new typed array.*/
    len = rjs_typed_array_element_size(et) * nitem;

    if ((r = rjs_allocate_array_buffer(rt, NULL, 0, ab)) == RJS_ERR)
        goto end;

    a = (RJS_ArrayBuffer*)rjs_value_get_object(rt, ab);
    db = a->data_block;

    db->data   = p;
    db->size   = len;
    db->flags |= RJS_DATA_BLOCK_FL_EXTERN;

    a->byte_length = len;

    if ((r = rjs_create_typed_array(rt, et, ab, 1, ta)) == RJS_ERR)
        goto end;

    /*Add the typed array to hash table.*/
    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, ta);

    iio->cptr_info.ptr   = p;
    iio->cptr_info.ctype = NULL;
    iio->cptr_info.ptype = key.ptype;
    iio->cptr_info.nitem = nitem;

    rjs_hash_insert(&rt->cptr_hash, &iio->cptr_info, &iio->cptr_he, phe, &ptr_hash_ops, rt);

    rjs_value_copy(rt, rv, ta);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

#endif /*ENABLE_INT_INDEXED_OBJECT*/

/**
 * Create a C function pointer object.
 * \param rt The current runtime.
 * \param ty The function's type.
 * \param cp The C function's pointer.
 * \param[out] rv Return the C pointer object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_create_c_func_ptr (RJS_Runtime *rt, RJS_CType *ty, void *cp, RJS_Value *rv)
{
    RJS_Realm     *realm = rjs_realm_current(rt);
    size_t         top   = rjs_value_stack_save(rt);
    RJS_CPtrInfo   key;
    RJS_CPtr      *ptr;
    RJS_HashEntry *he, **phe;
    RJS_Result     r;

    assert(ty && (ty->model == RJS_CTYPE_MODEL_FUNC));

    key.ptr   = cp;
    key.ctype = ty;
    key.nitem = 1;
    key.ptype = RJS_CPTR_TYPE_C_FUNC;

    r = rjs_hash_lookup(&rt->cptr_hash, &key, &he, &phe, &ptr_hash_ops, rt);
    if (r) {
        ptr = RJS_CONTAINER_OF(he, RJS_CPtr, he);

        if (ptr->info.ptype == RJS_CPTR_TYPE_C_WRAPPER) {
            if (ptr->p.w.fo)
                rjs_value_set_object(rt, rv, ptr->p.w.fo);
            else
                rjs_value_set_null(rt, rv);
        } else {
            rjs_value_set_object(rt, rv, &ptr->o);
        }
        r = RJS_OK;
        goto end;
    }

    RJS_NEW(rt, ptr);

    ptr->info.ptr   = cp;
    ptr->info.ctype = ty;
    ptr->info.ptype = RJS_CPTR_TYPE_C_FUNC;
    ptr->info.nitem = 1;

    rjs_hash_insert(&rt->cptr_hash, &ptr->info, &ptr->he, phe, &ptr_hash_ops, rt);

    rjs_object_init(rt, rv, &ptr->o, rjs_o_Function_prototype(realm), &cptr_func_ops);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Call FFI function pointer.*/
static void
call_ffi (ffi_cif *fid, void *ret, void **args, void *user_data)
{
    RJS_CPtr    *cptr  = user_data;
    RJS_Runtime *rt    = cptr->p.w.rt;
    RJS_CType   *cty   = cptr->info.ctype;
    int          nargs = cty->t.ffi.nargs;
    size_t       top   = rjs_value_stack_save(rt);
    RJS_Value   *fn    = rjs_value_stack_push_n(rt, nargs);
    RJS_Result   r;

    rjs_value_set_object(rt, fn, cptr->p.w.fo);

    r = cty->t.ffi.ffi2js(rt, args, nargs, fn, ret, cty->t.ffi.data);
    if (r == RJS_ERR)
        RJS_LOGE("ffi call failed");

    rjs_value_stack_restore(rt, top);
}

/**
 * Create a javascript function's C wrapper object.
 * \param rt The current runtime.
 * \param ty The function's type.
 * \param f The function value.
 * \param[out] cf Return the C function's pointer.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static RJS_Result
create_c_func_wrapper (RJS_Runtime *rt, RJS_CType *ty, RJS_Value *f, void **cf)
{
    RJS_Realm      *realm = rjs_realm_current(rt);
    size_t          top   = rjs_value_stack_save(rt);
    RJS_Value      *v     = rjs_value_stack_push(rt);
    RJS_BaseFuncObject *bfo;
    RJS_CPtr       *ptr;
    RJS_GcThingType gtt;
    RJS_Result      r;

    if (ty)
        assert(ty->model == RJS_CTYPE_MODEL_FUNC);

    assert(rjs_is_callable(rt, f));

    gtt = rjs_value_get_gc_thing_type(rt, f);

    /*The value is a C function.*/
    if (gtt == RJS_GC_THING_CPTR) {
        ptr = (RJS_CPtr*)rjs_value_get_object(rt, f);
        assert(ptr->info.ptype == RJS_CPTR_TYPE_C_FUNC);

        *cf = ptr->info.ptr;
        r = RJS_OK;
        goto end;
    }

    if (!ty) {
        r = rjs_throw_type_error(rt, _("C type is not specified"));
        goto end;
    }

    if ((gtt != RJS_GC_THING_SCRIPT_FUNC)
            && (gtt != RJS_GC_THING_BUILTIN_FUNC)
            && (gtt != RJS_GC_THING_NATIVE_FUNC)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    /*Check if the wrapper is already created.*/
    bfo = (RJS_BaseFuncObject*)rjs_value_get_object(rt, f);
    if (bfo->cptr) {
        ptr = bfo->cptr;
        *cf = ptr->info.ptr;
        r = RJS_OK;
        goto end;
    }

    /*Create a new C pointer.*/
    RJS_NEW(rt, ptr);

    ptr->p.w.closure = ffi_closure_alloc(sizeof(ffi_closure), &ptr->info.ptr);
    if (!ptr->p.w.closure) {
        RJS_DEL(rt, ptr);
        r = RJS_ERR;
        goto end;
    }

    if (ffi_prep_closure_loc(ptr->p.w.closure, &ty->t.ffi.cif, call_ffi, ptr, ptr->info.ptr) != FFI_OK) {
        ffi_closure_free(ptr->p.w.closure);
        RJS_DEL(rt, ptr);
        r = RJS_ERR;
        goto end;
    }

    ptr->p.w.rt     = rt;
    ptr->p.w.fo     = &bfo->object;
    ptr->info.ctype = ty;
    ptr->info.ptype = RJS_CPTR_TYPE_C_WRAPPER;
    ptr->info.nitem = 1;

    rjs_hash_insert(&rt->cptr_hash, &ptr->info, &ptr->he, NULL, &ptr_hash_ops, rt);

    rjs_object_init(rt, v, &ptr->o, rjs_o_Function_prototype(realm), &cptr_func_ops);

    /*Store the C pointer to the function object.*/
    bfo->cptr = ptr;

    *cf = ptr->info.ptr;
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Check if the value is a C pointer.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE The value is a C pointer.
 * \retval RJS_FALSE The value is not a C pointer.
 */
RJS_Bool
rjs_is_c_ptr (RJS_Runtime *rt, RJS_Value *v)
{
    return rjs_value_get_gc_thing_type(rt, v) == RJS_GC_THING_CPTR;
}

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
    RJS_GcThingType gtt;

    gtt = rjs_value_get_gc_thing_type(rt, cptrv);

#if ENABLE_INT_INDEXED_OBJECT
    if (!ty && (gtt == RJS_GC_THING_INT_INDEXED_OBJECT)) {
        /*Typed array.*/
        RJS_IntIndexedObject *iio;
        RJS_DataBlock        *db;

        iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, cptrv);

        if ((ptype != RJS_CPTR_TYPE_UNKNOWN) && (iio->type != (RJS_ArrayElementType)ptype)) {
            rjs_throw_type_error(rt, _("the array buffer type mismatch"));
            goto end;
        }

        if (rjs_is_detached_buffer(rt, &iio->buffer)) {
            rjs_throw_type_error(rt, _("the array buffer is detached"));
            goto end;
        }

        db = rjs_array_buffer_get_data_block(rt, &iio->buffer);
        r  = db->data;
    } else
#endif /*ENABLE_INT_INDEXED_OBJECT*/
#if ENABLE_ARRAY_BUFFER
    if (!ty && (ptype == RJS_CPTR_TYPE_UNKNOWN)
            && (gtt == RJS_GC_THING_ARRAY_BUFFER)) {
        /*Array buffer.*/
        RJS_DataBlock *db;

        if (rjs_is_detached_buffer(rt, cptrv)) {
            rjs_throw_type_error(rt, _("the array buffer is detached"));
            goto end;
        }

        db = rjs_array_buffer_get_data_block(rt, cptrv);
        r  = db->data;
    } else
#endif /*ENABLE_ARRAY_BUFFER*/
    if ((!ty || (ty->model == RJS_CTYPE_MODEL_FUNC))
            && ((ptype == RJS_CPTR_TYPE_UNKNOWN) || (ptype == RJS_CPTR_TYPE_C_FUNC) || (ptype == RJS_CPTR_TYPE_C_WRAPPER))
            && ((gtt == RJS_GC_THING_SCRIPT_FUNC) || (gtt == RJS_GC_THING_BUILTIN_FUNC) || (gtt == RJS_GC_THING_NATIVE_FUNC))) {
        /*Function.*/
        if (create_c_func_wrapper(rt, ty, cptrv, &r) == RJS_ERR)
            goto end;
    } else if (gtt == RJS_GC_THING_CPTR) {
        /*C pointer object.*/
        cptr = (RJS_CPtr*)rjs_value_get_object(rt, cptrv);

        if (ty && (cptr->info.ctype != ty)) {
            rjs_value_set_string(rt, str, ty->he.key);
            rjs_throw_type_error(rt, _("the pointer is not in type \"%s\""),
                    rjs_string_to_enc_chars(rt, str, NULL, NULL));
            goto end;
        }

        if ((ptype != RJS_CPTR_TYPE_UNKNOWN) && (cptr->info.ptype != ptype)) {
            rjs_throw_type_error(rt, _("the C pointer type mismatch"));
            goto end;
        }

        r = cptr->info.ptr;
    } else {
        rjs_throw_type_error(rt, _("the value is not a C pointer"));
        goto end;
    }
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
    RJS_CPtr       *cptr;
    RJS_GcThingType gtt;
    size_t          len;

    gtt = rjs_value_get_gc_thing_type(rt, cptrv);

#if ENABLE_INT_INDEXED_OBJECT
    if (gtt == RJS_GC_THING_INT_INDEXED_OBJECT) {
        RJS_IntIndexedObject *iio;

        iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, cptrv);

        len = iio->array_length;
    } else
#endif /*ENABLE_INT_INDEXED_OBJECT*/
#if ENABLE_ARRAY_BUFFER
    if (gtt == RJS_GC_THING_ARRAY_BUFFER) {
        RJS_ArrayBuffer *ab;

        ab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, cptrv);

        len = ab->byte_length;
    } else
#endif /*ENABLE_ARRAY_BUFFER*/

    if (gtt == RJS_GC_THING_CPTR) {
        cptr = (RJS_CPtr*)rjs_value_get_object(rt, cptrv);
        len  = cptr->info.nitem;
    } else {
        rjs_throw_type_error(rt, _("the value is not a C pointer"));
        return (size_t)-1;
    }

    return len;
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
            rjs_gc_scan_value(rt, &ty->t.prototype);
        }
    }
}
