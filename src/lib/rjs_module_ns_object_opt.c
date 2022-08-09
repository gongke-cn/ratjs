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

/*Scan the referenced things in the module namespace object.*/
static void
module_ns_object_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_ModuleNsObject *mno = ptr;
    size_t              i;

    rjs_object_op_gc_scan(rt, ptr);

    rjs_gc_scan_value(rt, &mno->module);

    for (i = 0; i < mno->export_num; i ++) {
        RJS_ModuleExport *me;

        me = &mno->exports[i];

        rjs_gc_scan_value(rt, &me->name);
    }
}

/*Free the module namespace object.*/
static void
module_ns_object_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_ModuleNsObject *mno = ptr;

    rjs_object_deinit(rt, &mno->object);

    rjs_hash_deinit(&mno->export_hash, &rjs_hash_size_ops, rt);

    if (mno->exports)
        RJS_DEL_N(rt, mno->exports, mno->export_num);

    RJS_DEL(rt, mno);
}

/*Get the module namespace's prototype.*/
static RJS_Result
module_ns_object_op_get_prototype_of (RJS_Runtime *rt, RJS_Value *o, RJS_Value *proto)
{
    rjs_value_set_null(rt, proto);
    return RJS_OK;
}

/*Set the module namespace's prototype.*/
static RJS_Result
module_ns_object_op_set_prototype_of (RJS_Runtime *rt, RJS_Value *v, RJS_Value *proto)
{
    RJS_Object *o = rjs_value_get_object(rt, v);

    if (rjs_same_value(rt, &o->prototype, proto))
        return RJS_TRUE;

    return RJS_FALSE;
}

/*Check if the module namespace is extensible.*/
static RJS_Result
module_ns_object_op_is_extensible (RJS_Runtime *rt, RJS_Value *o)
{
    return RJS_FALSE;
}

/*Prevent extensions of the module namespace*/
static RJS_Result
module_ns_object_op_prevent_extensions (RJS_Runtime *rt, RJS_Value *o)
{
    return RJS_TRUE;
}

/*Get the module namespace's own property.*/
static RJS_Result
module_ns_object_op_get_own_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_PropertyDesc *pd)
{
    RJS_Result  r;
    size_t      top = rjs_value_stack_save(rt);
    RJS_Value  *sv  = rjs_value_stack_push(rt);

    if (rjs_value_is_symbol(rt, pn->name)) {
        r = rjs_ordinary_object_op_get_own_property(rt, o, pn, pd);
    } else {
        RJS_String         *str;
        RJS_HashEntry      *he;
        RJS_ModuleNsObject *mno = (RJS_ModuleNsObject*)rjs_value_get_object(rt, o);

        if ((r = rjs_to_string(rt, pn->name, sv)) == RJS_ERR)
            goto end;

        rjs_string_to_property_key(rt, sv);

        str = rjs_value_get_string(rt, sv);
        r   = rjs_hash_lookup(&mno->export_hash, str, &he, NULL, &rjs_hash_size_ops, rt);
        if (!r)
            goto end;

        if ((r = rjs_object_get(rt, o, pn, o, pd->value)) == RJS_ERR)
            goto end;

        pd->flags = RJS_PROP_FL_DATA
                |RJS_PROP_FL_WRITABLE
                |RJS_PROP_FL_ENUMERABLE;

        r = RJS_OK;
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Define the own property of the module namespace.*/
static RJS_Result
module_ns_object_op_define_own_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_PropertyDesc *pd)
{
    RJS_Result       r;
    RJS_PropertyDesc curr;
    size_t           top = rjs_value_stack_save(rt);

    if (rjs_value_is_symbol(rt, pn->name)) {
        r = rjs_ordinary_object_op_define_own_property(rt, o, pn, pd);
    } else {
        rjs_property_desc_init(rt, &curr);

        if ((r = rjs_object_get_own_property(rt, o, pn, &curr)) == RJS_ERR)
            goto end;
        if (!r)
            goto end;

        if ((pd->flags & RJS_PROP_FL_HAS_CONFIGURABLE) && (pd->flags & RJS_PROP_FL_CONFIGURABLE)) {
            r = RJS_FALSE;
            goto end;
        }

        if ((pd->flags & RJS_PROP_FL_HAS_ENUMERABLE) && !(pd->flags & RJS_PROP_FL_ENUMERABLE)) {
            r = RJS_FALSE;
            goto end;
        }

        if (rjs_is_accessor_descriptor(pd)) {
            r = RJS_FALSE;
            goto end;
        }

        if ((pd->flags & RJS_PROP_FL_HAS_WRITABLE) && !(pd->flags & RJS_PROP_FL_WRITABLE)) {
            r = RJS_FALSE;
            goto end;
        }

        if (pd->flags & RJS_PROP_FL_HAS_VALUE) {
            r = rjs_same_value(rt, pd->value, curr.value);
            goto end;
        }

        r = RJS_TRUE;
end:
        rjs_property_desc_deinit(rt, &curr);
    }
    
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Check if the module namespace has the property.*/
static RJS_Result
module_ns_object_op_has_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn)
{
    RJS_Result  r;
    size_t      top = rjs_value_stack_save(rt);
    RJS_Value  *sv  = rjs_value_stack_push(rt);

    if (rjs_value_is_symbol(rt, pn->name)) {
        r = rjs_ordinary_object_op_has_property(rt, o, pn);
    } else {
        RJS_String         *str;
        RJS_HashEntry      *he;
        RJS_ModuleNsObject *mno = (RJS_ModuleNsObject*)rjs_value_get_object(rt, o);

        if ((r = rjs_to_string(rt, pn->name, sv)) == RJS_ERR)
            goto end;

        rjs_string_to_property_key(rt, sv);

        str = rjs_value_get_string(rt, sv);
        r   = rjs_hash_lookup(&mno->export_hash, str, &he, NULL, &rjs_hash_size_ops, rt);
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Get the module namespace's property value.*/
static RJS_Result
module_ns_object_op_get (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *receiver, RJS_Value *pv)
{
    RJS_Result         r;
    RJS_ResolveBinding rb;
    size_t             top = rjs_value_stack_save(rt);
    RJS_Value         *sv  = rjs_value_stack_push(rt);

    rjs_resolve_binding_init(rt, &rb);

    if (rjs_value_is_symbol(rt, pn->name)) {
        r = rjs_ordinary_object_op_get(rt, o, pn, receiver, pv);
    } else {
        RJS_String         *str;
        RJS_HashEntry      *he;
        RJS_ModuleNsObject *mno = (RJS_ModuleNsObject*)rjs_value_get_object(rt, o);

        if ((r = rjs_to_string(rt, pn->name, sv)) == RJS_ERR)
            goto end;

        rjs_string_to_property_key(rt, sv);

        str = rjs_value_get_string(rt, sv);
        r   = rjs_hash_lookup(&mno->export_hash, str, &he, NULL, &rjs_hash_size_ops, rt);
        if (!r) {
            rjs_value_set_undefined(rt, pv);
            r = RJS_OK;
            goto end;
        }

        r = rjs_module_resolve_export(rt, &mno->module, sv, &rb);
        assert(r == RJS_OK);

        if (rjs_value_is_undefined(rt, rb.name)) {
            r = rjs_module_get_namespace(rt, rb.module, pv);
        } else {
            RJS_Module     *mod;
            RJS_BindingName bn;

            mod = (RJS_Module*)rjs_value_get_gc_thing(rt, rb.module);

            if (!mod->env) {
                r = rjs_throw_reference_error(rt, _("the module environment is not created"));
                goto end;
            }

            rjs_binding_name_init(rt, &bn, rb.name);
            r = rjs_env_get_binding_value(rt, mod->env, &bn, RJS_TRUE, pv);
            rjs_binding_name_deinit(rt, &bn);
            if (r == RJS_ERR)
                goto end;
        }

        r = RJS_TRUE;
    }
end:
    rjs_resolve_binding_deinit(rt, &rb);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Set the module namespace's property value.*/
static RJS_Result
module_ns_object_op_set (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *pv, RJS_Value *receiver)
{
    return RJS_FALSE;
}

/*Delete the property of the module namespace.*/
static RJS_Result
module_ns_object_op_delete (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn)
{
    RJS_Result  r;
    size_t      top = rjs_value_stack_save(rt);
    RJS_Value  *sv  = rjs_value_stack_push(rt);

    if (rjs_value_is_symbol(rt, pn->name)) {
        r = rjs_ordinary_object_op_delete(rt, o, pn);
    } else {
        RJS_String         *str;
        RJS_HashEntry      *he;
        RJS_ModuleNsObject *mno = (RJS_ModuleNsObject*)rjs_value_get_object(rt, o);

        if ((r = rjs_to_string(rt, pn->name, sv)) == RJS_ERR)
            goto end;

        rjs_string_to_property_key(rt, sv);

        str = rjs_value_get_string(rt, sv);
        r   = rjs_hash_lookup(&mno->export_hash, str, &he, NULL, &rjs_hash_size_ops, rt);
        if (r) {
            r = RJS_FALSE;
            goto end;
        }

        r = RJS_TRUE;
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Get the module namespace's property keys.*/
static RJS_Result
module_ns_object_op_own_property_keys (RJS_Runtime *rt, RJS_Value *o, RJS_Value *keys)
{
    RJS_ModuleNsObject  *mno = (RJS_ModuleNsObject*)rjs_value_get_object(rt, o);
    RJS_PropertyKeyList *pkl;
    size_t               cap, i;

    cap = mno->export_num + mno->object.array_item_num + mno->object.prop_hash.entry_num;
    pkl = rjs_property_key_list_new(rt, keys, cap);

    for (i = 0; i < mno->export_num; i ++) {
        RJS_Value        *kv = pkl->keys.items + pkl->keys.item_num ++;
        RJS_ModuleExport *me = &mno->exports[i];

        rjs_value_copy(rt, kv, &me->name);
    }

    rjs_property_key_list_add_own_keys(rt, keys, o);

    return RJS_OK;
}

/*Module namespace object operation functions.*/
static const RJS_ObjectOps
module_ns_object_ops = {
    {
        RJS_GC_THING_OBJECT,
        module_ns_object_op_gc_scan,
        module_ns_object_op_gc_free
    },
    module_ns_object_op_get_prototype_of,
    module_ns_object_op_set_prototype_of,
    module_ns_object_op_is_extensible,
    module_ns_object_op_prevent_extensions,
    module_ns_object_op_get_own_property,
    module_ns_object_op_define_own_property,
    module_ns_object_op_has_property,
    module_ns_object_op_get,
    module_ns_object_op_set,
    module_ns_object_op_delete,
    module_ns_object_op_own_property_keys,
    NULL,
    NULL
};

/*Export vector.*/
typedef struct {
    RJS_List      ln;   /**< List node data.*/
    RJS_HashEntry he;   /**< Hash table entry.*/
    RJS_Value     name; /**< Export name.*/
} RJS_ExportName;

static RJS_Result
get_export_names (RJS_Runtime *rt, RJS_Value *mod, RJS_Hash *hash, RJS_List *list, RJS_List *star_set)
{
    RJS_Module *m, *oldm;
    RJS_Script *script;
    RJS_Result  r;
    size_t      i, cnt;

    m      = rjs_value_get_gc_thing(rt, mod);
    script = &m->script;

    rjs_list_foreach_c(star_set, oldm, RJS_Module, star_ln) {
        if (oldm == m)
            return RJS_OK;
    }

    rjs_list_append(star_set, &m->star_ln);

    cnt = m->local_export_entry_num + m->indir_export_entry_num + m->star_export_entry_num;
    for (i = 0; i < cnt; i ++) {
        RJS_ExportEntry *ee;

        ee = &m->export_entries[i];

        if (ee->export_name_idx != RJS_INVALID_VALUE_INDEX) {
            RJS_ExportName *en;
            RJS_HashEntry  *he, **phe;
            RJS_Value      *v   = &script->value_table[ee->export_name_idx];
            RJS_String     *str = rjs_value_get_string(rt, v);

            if (rjs_same_value(rt, v, rjs_s_default(rt))) {
                RJS_Bool added = RJS_FALSE;

                rjs_list_foreach_c(list, en, RJS_ExportName, ln) {
                    if (rjs_same_value(rt, &en->name, rjs_s_default(rt))) {
                        added = RJS_TRUE;
                        break;
                    }
                }

                if (added)
                    continue;
            }

            r = rjs_hash_lookup(hash, str, &he, &phe, &rjs_hash_size_ops, rt);
            if (r)
                continue;

            RJS_NEW(rt, en);

            rjs_value_copy(rt, &en->name, &script->value_table[ee->export_name_idx]);
            rjs_list_append(list, &en->ln);
            rjs_hash_insert(hash, str, &en->he, phe, &rjs_hash_size_ops, rt);
        } else {
            RJS_ModuleRequest *mr = &m->module_requests[ee->module_request_idx];

            if (rjs_value_is_undefined(rt, &mr->module)) {
                RJS_Value *name = &script->value_table[mr->module_name_idx];

                r = rjs_resolve_imported_module(rt, mod, name, &mr->module);
                if (r == RJS_ERR)
                    goto end;
            }

            if ((r = get_export_names(rt, &mr->module, hash, list, star_set)) == RJS_ERR)
                goto end;
        }
    }

    r = RJS_OK;
end:
    return r;
}

/*Compare 2 strings.*/
static RJS_CompareResult
string_compare (const void *p1, const void *p2, void *arg)
{
    RJS_Value *v1    = (RJS_Value*)p1;
    RJS_Value *v2    = (RJS_Value*)p2;
    RJS_Runtime *rt = arg;

    return rjs_string_compare(rt, v1, v2);
}

/**
 * Create a module namespace object.
 * \param rt The current runtime.
 * \param[out] v Return the module namespace object.
 * \param mod The module.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_module_ns_object_new (RJS_Runtime *rt, RJS_Value *v, RJS_Value *mod)
{
    RJS_Result          r;
    RJS_Hash            export_hash;
    RJS_List            export_list, star_list;
    RJS_ResolveBinding  rb;
    RJS_ExportName     *n, *nn;
    size_t              en_cap, en_num, i;
    RJS_ModuleNsObject *mno;
    RJS_Value          *export_names = NULL;
    size_t              top = rjs_value_stack_save(rt);

    assert(rjs_value_is_module(rt, mod));

    rjs_resolve_binding_init(rt, &rb);

    rjs_hash_init(&export_hash);
    rjs_list_init(&export_list);
    rjs_list_init(&star_list);

    /*Get export names.*/
    if ((r = get_export_names(rt, mod, &export_hash, &export_list, &star_list)) == RJS_ERR)
        goto end;

    /*Resolve export names.*/
    en_cap = export_hash.entry_num;
    en_num = 0;
    RJS_NEW_N(rt, export_names, en_cap);

    rjs_list_foreach_c(&export_list, n, RJS_ExportName, ln) {
        if ((r = rjs_module_resolve_export(rt, mod, &n->name, &rb)) == RJS_ERR)
            goto end;

        if (r == RJS_OK) {
            RJS_Value *d = &export_names[en_num ++];

            rjs_value_copy(rt, d, &n->name);
        }
    }

    /*Sort the export names.*/
    rjs_sort(export_names, en_num, sizeof(RJS_Value), string_compare, rt);

    /*Create the namespace object.*/
    RJS_NEW(rt, mno);

    rjs_hash_init(&mno->export_hash);
    rjs_value_copy(rt, &mno->module, mod);

    mno->export_num = en_num;

    RJS_NEW_N(rt, mno->exports, en_num);
    for (i = 0; i < en_num; i ++) {
        RJS_String       *str;
        RJS_Value        *src;
        RJS_ModuleExport *dst;

        src = &export_names[i];
        dst = &mno->exports[i];

        rjs_value_copy(rt, &dst->name, src);

        str = rjs_value_get_string(rt, &dst->name);

        rjs_hash_insert(&mno->export_hash, str, &dst->he, NULL, &rjs_hash_size_ops, rt);
    }

    /*Create the module namespace object and add [@@toStringTag] property.*/
    rjs_object_init(rt, v, &mno->object, NULL, NULL);
    rjs_create_data_property_attrs_or_throw(rt, v, rjs_pn_s_toStringTag(rt), rjs_s_Module(rt), 0);

    /*Reset the module operation functions.*/
    mno->object.gc_thing.ops = (RJS_GcThingOps*)&module_ns_object_ops;

    r = RJS_OK;
end:
    rjs_resolve_binding_deinit(rt, &rb);
    
    if (export_names)
        RJS_DEL_N(rt, export_names, en_cap);

    rjs_list_foreach_safe_c(&export_list, n, nn, RJS_ExportName, ln) {
        RJS_DEL(rt, n);
    }
    rjs_hash_deinit(&export_hash, &rjs_hash_size_ops, rt);

    rjs_value_stack_restore(rt, top);
    return r;
}
