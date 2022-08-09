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

/*Scan the referenced things in the arguments.*/
static void
arguments_object_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_ArgumentsObject *ao = ptr;

    rjs_object_op_gc_scan(rt, ptr);

    if (ao->env)
        rjs_gc_mark(rt, ao->env);

    rjs_gc_scan_value_buffer(rt, ao->names, ao->argc);
}

/*Free the arguments.*/
static void
arguments_object_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_ArgumentsObject *ao = ptr;

    rjs_object_deinit(rt, &ao->object);

    if (ao->names)
        RJS_DEL_N(rt, ao->names, ao->argc);

    RJS_DEL(rt, ao);
}

/*Check if the arguments has the property.*/
static RJS_Bool
arguments_has_property (RJS_Runtime *rt, RJS_Value *o, RJS_Value *p, uint32_t *pidx)
{
    RJS_ArgumentsObject *ao;
    RJS_Bool             r;
    int64_t              idx;

    if (!rjs_value_is_string(rt, p))
        return RJS_FALSE;

    if (!(r = rjs_string_to_index(rt, p, &idx)))
        return r;

    ao = (RJS_ArgumentsObject*)rjs_value_get_object(rt, o);

    if (idx >= ao->argc)
        return RJS_FALSE;

    if (rjs_value_is_undefined(rt, &ao->names[idx]))
        return RJS_FALSE;

    *pidx = idx;
    return RJS_TRUE;
}

/*Get the arguments' property value.*/
static RJS_Result
arguments_get (RJS_Runtime *rt, RJS_Value *o, uint32_t idx, RJS_Value *v)
{
    RJS_ArgumentsObject *ao;
    RJS_BindingName      bn;
    RJS_Result           r;

    ao = (RJS_ArgumentsObject*)rjs_value_get_object(rt, o);

    rjs_binding_name_init(rt, &bn, &ao->names[idx]);

    r = rjs_env_get_binding_value(rt, ao->env, &bn, RJS_FALSE, v);
    
    rjs_binding_name_deinit(rt, &bn);

    return r;
}

/*Set the arguments' property value.*/
static RJS_Result
arguments_set (RJS_Runtime *rt, RJS_Value *o, uint32_t idx, RJS_Value *v)
{
    RJS_ArgumentsObject *ao;
    RJS_BindingName      bn;
    RJS_Result           r;

    ao = (RJS_ArgumentsObject*)rjs_value_get_object(rt, o);

    rjs_binding_name_init(rt, &bn, &ao->names[idx]);

    r = rjs_env_set_mutable_binding(rt, ao->env, &bn, v, RJS_FALSE);

    rjs_binding_name_deinit(rt, &bn);

    return r;
}

/*Delete the arguments' property.*/
static RJS_Result
arguments_delete (RJS_Runtime *rt, RJS_Value *o, uint32_t idx)
{
    RJS_ArgumentsObject *ao;

    ao = (RJS_ArgumentsObject*)rjs_value_get_object(rt, o);

    rjs_value_set_undefined(rt, &ao->names[idx]);

    return RJS_OK;
}

/*Get the arguments own property.*/
static RJS_Result
arguments_object_op_get_own_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_PropertyDesc *pd)
{
    RJS_Result  r;
    uint32_t    idx;

    if ((r = rjs_ordinary_object_op_get_own_property(rt, o, pn, pd)) == RJS_ERR)
        return r;
    if (!r)
        return r;

    if (arguments_has_property(rt, o, pn->name, &idx)) {
        r = arguments_get(rt, o, idx, pd->value);
    }

    return r;
}

/*Define own property in the arguments.*/
static RJS_Result
arguments_object_op_define_own_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_PropertyDesc *pd)
{
    RJS_Result       r;
    uint32_t         idx;
    RJS_Bool         is_mapped;
    RJS_PropertyDesc new_pd;

    rjs_property_desc_init(rt, &new_pd);
    rjs_property_desc_copy(rt, &new_pd, pd);

    is_mapped = arguments_has_property(rt, o, pn->name, &idx);
    if (is_mapped && rjs_is_data_descriptor(pd)) {
        if (!(pd->flags & RJS_PROP_FL_HAS_VALUE)
                && (pd->flags & RJS_PROP_FL_HAS_WRITABLE)
                && !(pd->flags & RJS_PROP_FL_WRITABLE)) {
            if ((r = arguments_get(rt, o, idx, new_pd.value)) == RJS_ERR)
                goto end;

            new_pd.flags |= RJS_PROP_FL_HAS_VALUE;
        }
    }

    if ((r = rjs_ordinary_object_op_define_own_property(rt, o, pn, &new_pd)) == RJS_ERR)
        goto end;
    if (!r)
        goto end;

    if (is_mapped) {
        if (rjs_is_accessor_descriptor(pd)) {
            arguments_delete(rt, o, idx);
        } else {
            if (pd->flags & RJS_PROP_FL_HAS_VALUE) {
                arguments_set(rt, o, idx, pd->value);
            }

            if ((pd->flags & RJS_PROP_FL_HAS_WRITABLE)
                    && !(pd->flags & RJS_PROP_FL_WRITABLE)) {
                arguments_delete(rt, o, idx);
            }
        }
    }

    r = RJS_TRUE;
end:
    rjs_property_desc_deinit(rt, &new_pd);
    return r;
}

/*Get the arguments' property value.*/
static RJS_Result
arguments_object_op_get (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *receiver, RJS_Value *pv)
{
    uint32_t   idx;
    RJS_Result r;

    if ((r = arguments_has_property(rt, o, pn->name, &idx)) == RJS_ERR)
        return r;

    if (!r) {
        r = rjs_ordinary_object_op_get(rt, o, pn, receiver, pv);
    } else {
        r = arguments_get(rt, o, idx, pv);
    }

    return r;
}

/*Set the arguments' property value.*/
static RJS_Result
arguments_object_op_set (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *pv, RJS_Value *receiver)
{
    RJS_Bool   is_mapped;
    uint32_t   idx;
    RJS_Result r;

    if (!rjs_same_value(rt, o, receiver))
        is_mapped = RJS_FALSE;
    else
        is_mapped = arguments_has_property(rt, o, pn->name, &idx);

    if (is_mapped)
        arguments_set(rt, o, idx, pv);

    r = rjs_ordinary_object_op_set(rt, o, pn, pv, receiver);

    return r;
}

/*Deleate a property of the arguments.*/
static RJS_Result
arguments_object_op_delete (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn)
{
    RJS_Bool   is_mapped;
    uint32_t   idx;
    RJS_Result r;

    is_mapped = arguments_has_property(rt, o, pn->name, &idx);

    if ((r = rjs_ordinary_object_op_delete(rt, o, pn)) == RJS_ERR)
        return r;

    if (r && is_mapped)
        arguments_delete(rt, o, idx);

    return r;
}

/*Arguments object operation functions.*/
static const RJS_ObjectOps
arguments_object_ops = {
    {
        RJS_GC_THING_ARGUMENTS,
        arguments_object_op_gc_scan,
        arguments_object_op_gc_free
    },
    rjs_ordinary_object_op_get_prototype_of,
    rjs_ordinary_object_op_set_prototype_of,
    rjs_ordinary_object_op_is_extensible,
    rjs_ordinary_object_op_prevent_extensions,
    arguments_object_op_get_own_property,
    arguments_object_op_define_own_property,
    rjs_ordinary_object_op_has_property,
    arguments_object_op_get,
    arguments_object_op_set,
    arguments_object_op_delete,
    rjs_ordinary_object_op_own_property_keys,
    NULL,
    NULL
};

/*Unmapped object operation functions.*/
static const RJS_ObjectOps
unmapped_arguments_object_ops = {
    {
        RJS_GC_THING_ARGUMENTS,
        rjs_object_op_gc_scan,
        rjs_object_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS
};

/**
 * Create a new unmapped arguments object.
 * \param rt The current runtime.
 * \param[out] v Return the arguments object.
 * \param args The arguments.
 * \param argc The arguments' count.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error. 
 */
RJS_Result
rjs_unmapped_arguments_object_new (RJS_Runtime *rt, RJS_Value *v, RJS_Value *args, size_t argc)
{
    RJS_Realm       *realm = rjs_realm_current(rt);
    size_t           top   = rjs_value_stack_save(rt);
    size_t           i;
    RJS_Object      *o;
    RJS_PropertyDesc pd;
    RJS_Result       r;

    rjs_property_desc_init(rt, &pd);

    RJS_NEW(rt, o);
    if ((r = rjs_object_init(rt, v, o, rjs_o_Object_prototype(realm), &unmapped_arguments_object_ops)) == RJS_ERR) {
        RJS_DEL(rt, o);
        goto end;
    }

    /*length*/
    pd.flags = RJS_PROP_FL_DATA|RJS_PROP_FL_WRITABLE|RJS_PROP_FL_CONFIGURABLE;
    rjs_value_set_number(rt, pd.value, argc);
    if ((r = rjs_define_property_or_throw(rt, v, rjs_pn_length(rt), &pd)) == RJS_ERR)
        goto end;

    for (i = 0; i < argc; i ++) {
        RJS_Value *arg = rjs_value_buffer_item(rt, args, i);

        if ((r = rjs_create_data_property_or_throw_index(rt, v, i, arg)) == RJS_ERR)
            goto end;
    }

    /*@@iterator*/
    pd.flags = RJS_PROP_FL_DATA|RJS_PROP_FL_WRITABLE|RJS_PROP_FL_CONFIGURABLE;
    rjs_value_copy(rt, pd.value, rjs_o_Array_prototype_values(realm));
    if ((r = rjs_define_property_or_throw(rt, v, rjs_pn_s_iterator(rt), &pd)) == RJS_ERR)
        goto end;

    /*callee*/
    pd.flags = RJS_PROP_FL_ACCESSOR;
    rjs_value_copy(rt, pd.get, rjs_o_ThrowTypeError(realm));
    rjs_value_copy(rt, pd.set, rjs_o_ThrowTypeError(realm));
    if ((r = rjs_define_property_or_throw(rt, v, rjs_pn_callee(rt), &pd)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Create a mapped arguments object.
 * \param rt The current runtime.
 * \param[out] v Return the arguments object.
 * \param f The function.
 * \param args The arguments.
 * \param srgc The arguments' count.
 * \param bg The parameters binding group.
 * \param env The environment contains the arguments.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_mapped_arguments_object_new (RJS_Runtime *rt, RJS_Value *v, RJS_Value *f,
        RJS_ScriptBindingGroup *bg, RJS_Value *args, size_t argc, RJS_Environment *env)
{
    RJS_ArgumentsObject  *ao;
    RJS_ScriptFuncObject *sfo;
    RJS_Script           *script;
    size_t                i, bn;
    RJS_PropertyDesc      pd;
    RJS_Result            r;
    RJS_Realm            *realm = rjs_realm_current(rt);
    size_t                top   = rjs_value_stack_save(rt);

    rjs_property_desc_init(rt, &pd);

    sfo    = (RJS_ScriptFuncObject*)rjs_value_get_object(rt, f);
    script = sfo->bfo.script;

    RJS_NEW(rt, ao);

    ao->env  = env;
    ao->argc = argc;

    if (ao->argc) {
        RJS_NEW_N(rt, ao->names, ao->argc);

        rjs_value_buffer_fill_undefined(rt, ao->names, ao->argc);
    } else {
        ao->names = NULL;
    }

    rjs_object_init(rt, v, &ao->object, rjs_o_Object_prototype(realm), &arguments_object_ops);

    for (i = 0; i < argc; i ++) {
        RJS_Value *arg = rjs_value_buffer_item(rt, args, i);

        rjs_create_data_property_or_throw_index(rt, v, i, arg);
    }

    bn = bg ? bg->binding_num : 0;
    for (i = 0; i < bn; i ++) {
        RJS_ScriptDecl       *decl = &script->decl_table[bg->decl_idx];
        RJS_ScriptBinding    *b    = &script->binding_table[bg->binding_start + i];
        RJS_BindingRefIndex   bri  = b->ref_idx;
        int                   pid  = b->bot_ref_idx;

        if (pid != RJS_INVALID_BINDING_REF_INDEX) {
            RJS_ScriptBindingRef *br = &script->binding_ref_table[decl->binding_ref_start + bri];
            RJS_Value            *d  = ao->names + pid;

            if (pid < ao->argc)
                rjs_value_copy(rt, d, br->binding_name.name);
        }
    }

    /*length*/
    pd.flags = RJS_PROP_FL_DATA|RJS_PROP_FL_WRITABLE|RJS_PROP_FL_CONFIGURABLE;
    rjs_value_set_number(rt, pd.value, argc);
    rjs_define_property_or_throw(rt, v, rjs_pn_length(rt), &pd);

    /*@@iterator*/
    pd.flags = RJS_PROP_FL_DATA|RJS_PROP_FL_WRITABLE|RJS_PROP_FL_CONFIGURABLE;
    rjs_value_copy(rt, pd.value, rjs_o_Array_prototype_values(realm));
    if ((r = rjs_define_property_or_throw(rt, v, rjs_pn_s_iterator(rt), &pd)) == RJS_ERR)
        goto end;

    /*callee*/
    pd.flags = RJS_PROP_FL_DATA|RJS_PROP_FL_WRITABLE|RJS_PROP_FL_CONFIGURABLE;
    rjs_value_copy(rt, pd.value, f);
    if ((r = rjs_define_property_or_throw(rt, v, rjs_pn_callee(rt), &pd)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}
