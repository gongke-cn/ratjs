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

/*Scan the referenced things in the global environment.*/
static void
global_env_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_GlobalEnv *ge = (RJS_GlobalEnv*)ptr;
    size_t         i;
    RJS_HashEntry *he;

    rjs_gc_scan_value(rt, &ge->global_this);
    rjs_gc_mark(rt, ge->object_rec);
    rjs_gc_mark(rt, ge->decl_rec);

    rjs_hash_foreach(&ge->var_name_hash, i, he) {
        rjs_gc_mark(rt, he->key);
    }
}

/*Free the global environment.*/
static void
global_env_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_GlobalEnv *ge = (RJS_GlobalEnv*)ptr;
    size_t         i;
    RJS_HashEntry *he, *nhe;

    rjs_hash_foreach_safe(&ge->var_name_hash, i, he, nhe) {
        RJS_DEL(rt, he);
    }

    rjs_hash_deinit(&ge->var_name_hash, &rjs_hash_size_ops, rt);

    RJS_DEL(rt, ge);
}

/*Check if the global environment has the binding.*/
static RJS_Result
global_env_op_has_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n)
{
    RJS_GlobalEnv *ge = (RJS_GlobalEnv*)env;
    RJS_Result     r;

    if ((r = rjs_env_has_binding(rt, ge->decl_rec, n)))
        return r;

    return rjs_env_has_binding(rt, ge->object_rec, n);
}

/*Create a mutable binding in the global environment.*/
static RJS_Result
global_env_op_create_mutable_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Bool del)
{
    RJS_GlobalEnv *ge = (RJS_GlobalEnv*)env;
    RJS_Result     r;

    if ((r = rjs_env_has_binding(rt, ge->decl_rec, n)))
        return rjs_throw_reference_error(rt, _("global binding \"%s\" is not declared"),
                rjs_string_to_enc_chars(rt, n->name, NULL, NULL));

    return rjs_env_create_mutable_binding(rt, ge->decl_rec, n, del);
}

/*Create an immutable binding in the global environment.*/
static RJS_Result
global_env_op_create_immutable_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Bool strict)
{
    RJS_GlobalEnv *ge = (RJS_GlobalEnv*)env;
    RJS_Result     r;

    if ((r = rjs_env_has_binding(rt, ge->decl_rec, n)))
        return rjs_throw_reference_error(rt, _("global binding \"%s\" is not declared"),
                rjs_string_to_enc_chars(rt, n->name, NULL, NULL));

    return rjs_env_create_immutable_binding(rt, ge->decl_rec, n, strict);
}

/*Initialize the binding in the global environment.*/
static RJS_Result
global_env_op_initialize_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Value *v)
{
    RJS_GlobalEnv *ge = (RJS_GlobalEnv*)env;
    RJS_Result     r;

    if ((r = rjs_env_has_binding(rt, ge->decl_rec, n)))
        return rjs_env_initialize_binding(rt, ge->decl_rec, n, v);

    return rjs_env_initialize_binding(rt, ge->object_rec, n, v);
}

/*Set the mutable binding in the global environment.*/
static RJS_Result
global_env_op_set_mutable_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Value *v, RJS_Bool strict)
{
    RJS_GlobalEnv *ge = (RJS_GlobalEnv*)env;
    RJS_Result     r;

    if ((r = rjs_env_has_binding(rt, ge->decl_rec, n)))
        return rjs_env_set_mutable_binding(rt, ge->decl_rec, n, v, strict);

    return rjs_env_set_mutable_binding(rt, ge->object_rec, n, v, strict);
}

/*Get the binding's value in the global environment.*/
static RJS_Result
global_env_op_get_binding_value (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Bool strict, RJS_Value *v)
{
    RJS_GlobalEnv *ge = (RJS_GlobalEnv*)env;
    RJS_Result     r;

    if ((r = rjs_env_has_binding(rt, ge->decl_rec, n)))
        return rjs_env_get_binding_value(rt, ge->decl_rec, n, strict, v);

    return rjs_env_get_binding_value(rt, ge->object_rec, n, strict, v);
}

/*Delete a binding in the global environment.*/
static RJS_Result
global_env_op_delete_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n)
{
    RJS_GlobalEnv   *ge = (RJS_GlobalEnv*)env;
    RJS_ObjectEnv   *oe;
    RJS_Result       r;
    RJS_PropertyName pn;

    if ((r = rjs_env_has_binding(rt, ge->decl_rec, n)))
        return rjs_env_delete_binding(rt, ge->decl_rec, n);

    oe = (RJS_ObjectEnv*)ge->object_rec;

    rjs_property_name_init(rt, &pn, n->name);

    if ((r = rjs_has_own_property(rt, &oe->object, &pn)) == RJS_ERR)
        goto end;

    if (r) {
        r = rjs_env_delete_binding(rt, ge->object_rec, n);
        if (r) {
            RJS_HashEntry *he, **phe;
            RJS_String    *str;
            RJS_Result     hr;

            rjs_string_to_property_key(rt, n->name);
            str = rjs_value_get_string(rt, n->name);

            hr = rjs_hash_lookup(&ge->var_name_hash, str, &he, &phe, &rjs_hash_size_ops, rt);
            if (hr) {
                rjs_hash_remove(&ge->var_name_hash, phe, rt);
                RJS_DEL(rt, he);
            }
        }
    }
end:
    rjs_property_name_deinit(rt, &pn);
    return r;
}

/*Check if the global environment has this binding.*/
static RJS_Result
global_env_op_has_this_binding (RJS_Runtime *rt, RJS_Environment *env)
{
    return RJS_TRUE;
}

/*Check if the global environemnt has the super binding.*/
static RJS_Result
global_env_op_has_super_binding (RJS_Runtime *rt, RJS_Environment *env)
{
    return RJS_FALSE;
}

/*Get base object of the global environment.*/
static RJS_Result
global_env_op_with_base_object (RJS_Runtime *rt, RJS_Environment *env, RJS_Value *base)
{
    rjs_value_set_undefined(rt, base);
    return RJS_OK;
}

/*Get this binding value of the global environment.*/
static RJS_Result
global_env_op_get_this_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_Value *v)
{
    RJS_GlobalEnv *ge = (RJS_GlobalEnv*)env;

    rjs_value_copy(rt, v, &ge->global_this);
    return RJS_OK;
}

/*Global environment operation functions.*/
static const RJS_EnvOps
global_env_ops = {
    {
        RJS_GC_THING_GLOBAL_ENV,
        global_env_op_gc_scan,
        global_env_op_gc_free
    },
    global_env_op_has_binding,
    global_env_op_create_mutable_binding,
    global_env_op_create_immutable_binding,
    global_env_op_initialize_binding,
    global_env_op_set_mutable_binding,
    global_env_op_get_binding_value,
    global_env_op_delete_binding,
    global_env_op_has_this_binding,
    global_env_op_has_super_binding,
    global_env_op_with_base_object,
    global_env_op_get_this_binding
};

/*Get the global environment.*/
static RJS_GlobalEnv*
global_env_get (RJS_Environment *env)
{
    assert(env->gc_thing.ops == &global_env_ops.gc_thing_ops);

    return (RJS_GlobalEnv*)env;
}

/**
 * Create a new global environemnt.
 * \param rt The current runtime.
 * \param[out] pe Return the new environment.
 * \param g Teh global object.
 * \param thiz This value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_global_env_new (RJS_Runtime *rt, RJS_Environment **pe, RJS_Value *g, RJS_Value *thiz)
{
    RJS_GlobalEnv *ge;

    RJS_NEW(rt, ge);

    rjs_value_copy(rt, &ge->global_this, thiz);

    ge->env.outer       = NULL;
    ge->env.script_decl = NULL;
    ge->decl_rec        = NULL;
    ge->object_rec      = NULL;

    rjs_hash_init(&ge->var_name_hash);

    *pe = &ge->env;

    rjs_gc_add(rt, ge, &global_env_ops.gc_thing_ops);

    rjs_object_env_new(rt, &ge->object_rec, g, RJS_FALSE, NULL, NULL);
    rjs_decl_env_new(rt, &ge->decl_rec, NULL, NULL);

    return RJS_OK;
}

/**
 * Check if the global environment has the variable declaration.
 * \param rt The current runtime.
 * \param env The environment.
 * \param bn The binding name of the variable.
 * \retval RJS_TRUE The environment has the declaration.
 * \retval RJS_FALSE The environment has not the declaration.
 */
RJS_Result
rjs_env_has_var_declaration (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *bn)
{
    RJS_GlobalEnv *ge = global_env_get(env);
    RJS_String    *str;
    RJS_HashEntry *he;
    RJS_Result     r;

    rjs_string_to_property_key(rt, bn->name);
    str = rjs_value_get_string(rt, bn->name);

    r = rjs_hash_lookup(&ge->var_name_hash, str, &he, NULL, &rjs_hash_size_ops, rt);

    return r;
}

/**
 * Check if the global environment has the lexical declaration.
 * \param rt The current runtime.
 * \param env The environment.
 * \param bn The binding name of the declaration.
 * \retval RJS_TRUE The environment has the declaration.
 * \retval RJS_FALSE The environment has not the declaration.
 */
RJS_Result
rjs_env_has_lexical_declaration (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *bn)
{
    RJS_GlobalEnv *ge = global_env_get(env);

    return rjs_env_has_binding(rt, ge->decl_rec, bn);
}

/**
 * Check if the global environment has the restirected global property.
 * \param rt The current runtime.
 * \param env The environment.
 * \param bn The binding name of the declaration.
 * \retval RJS_TRUE The environment has the property.
 * \retval RJS_FALSE The environment has not the property.
 */
RJS_Result
rjs_env_has_restricted_global_property (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *bn)
{
    RJS_GlobalEnv   *ge  = global_env_get(env);
    size_t           top = rjs_value_stack_save(rt);
    RJS_ObjectEnv   *oe  = (RJS_ObjectEnv*)ge->object_rec;
    RJS_PropertyDesc pd;
    RJS_PropertyName pn;
    RJS_Result       r;

    rjs_property_desc_init(rt, &pd);
    rjs_property_name_init(rt, &pn, bn->name);

    if ((r = rjs_object_get_own_property(rt, &oe->object, &pn, &pd)) == RJS_ERR)
        goto end;
    if (!r)
        goto end;

    if (pd.flags & RJS_PROP_FL_CONFIGURABLE) {
        r = RJS_FALSE;
        goto end;
    }

    r = RJS_TRUE;
end:
    rjs_property_name_deinit(rt, &pn);
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Check if the global environment can declare the global variable.
 * \param rt The current runtime.
 * \param env The environment.
 * \param bn The binding name of the variable.
 * \retval RJS_TRUE The property can be declared.
 * \retval RJS_FALSE The property cannot be declared.
 */
RJS_Result
rjs_env_can_declare_global_var (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *bn)
{
    RJS_GlobalEnv    *ge = global_env_get(env);
    RJS_ObjectEnv    *oe = (RJS_ObjectEnv*)ge->object_rec;
    RJS_PropertyName  pn;
    RJS_Result        r;

    rjs_property_name_init(rt, &pn, bn->name);

    if ((r = rjs_has_own_property(rt, &oe->object, &pn)) == RJS_ERR)
        goto end;

    if (r)
        goto end;

    r = rjs_object_is_extensible(rt, &oe->object);
end:
    rjs_property_name_deinit(rt, &pn);
    return r;
}

/**
 * Check if the global environment can declare the global function.
 * \param rt The current runtime.
 * \param env The environment.
 * \param bn The binding name of the function.
 * \retval RJS_TRUE The function can be declared.
 * \retval RJS_FALSE The function cannot be declared.
 */
RJS_Result
rjs_env_can_declare_global_function (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *bn)
{
    RJS_GlobalEnv    *ge  = global_env_get(env);
    size_t            top = rjs_value_stack_save(rt);
    RJS_ObjectEnv    *oe  = (RJS_ObjectEnv*)ge->object_rec;
    RJS_PropertyDesc  pd;
    RJS_PropertyName  pn;
    RJS_Result        r;

    rjs_property_desc_init(rt, &pd);
    rjs_property_name_init(rt, &pn, bn->name);

    if ((r = rjs_object_get_own_property(rt, &oe->object, &pn, &pd)) == RJS_ERR)
        goto end;

    if (!r) {
        r = rjs_object_is_extensible(rt, &oe->object);
        goto end;
    }

    if (pd.flags & RJS_PROP_FL_CONFIGURABLE) {
        r = RJS_TRUE;
        goto end;
    }

    if (rjs_is_data_descriptor(&pd)
            && (pd.flags & RJS_PROP_FL_WRITABLE)
            && (pd.flags & RJS_PROP_FL_ENUMERABLE)) {
        r = RJS_TRUE;
        goto end;
    }

    r = RJS_FALSE;
end:
    rjs_property_name_deinit(rt, &pn);
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Create the global variable binding.
 * \param rt The current runtime.
 * \param env The environment.
 * \param bn The binding name of the variable.
 * \param del The binding is deletable.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_env_create_global_var_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *bn, RJS_Bool del)
{
    RJS_GlobalEnv    *ge = global_env_get(env);
    RJS_ObjectEnv    *oe = (RJS_ObjectEnv*)ge->object_rec;
    RJS_HashEntry    *he, **phe;
    RJS_String       *str;
    RJS_PropertyName  pn;
    RJS_Result        r;

    rjs_property_name_init(rt, &pn, bn->name);

    if ((r = rjs_has_own_property(rt, &oe->object, &pn)) == RJS_ERR)
        goto end;

    if (!r) {
        if ((r = rjs_object_is_extensible(rt, &oe->object)) == RJS_ERR)
            goto end;

        if (r) {
            if ((r = rjs_env_create_mutable_binding(rt, ge->object_rec, bn, del)) == RJS_ERR)
                goto end;
            if ((r = rjs_env_initialize_binding(rt, ge->object_rec, bn, rjs_v_undefined(rt))) == RJS_ERR)
                goto end;
        }
    }

    rjs_string_to_property_key(rt, bn->name);
    str = rjs_value_get_string(rt, bn->name);

    r = rjs_hash_lookup(&ge->var_name_hash, str, &he, &phe, &rjs_hash_size_ops, rt);
    if (!r) {
        RJS_NEW(rt, he);
        rjs_hash_insert(&ge->var_name_hash, str, he, phe, &rjs_hash_size_ops, rt);
    }

    r = RJS_OK;
end:
    rjs_property_name_deinit(rt, &pn);
    return r;
}

/**
 * Create the global variable binding.
 * \param rt The current runtime.
 * \param env The environment.
 * \param bn The binding name of the function.
 * \param v The function value.
 * \param del The binding is deletable.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_env_create_global_function_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *bn, RJS_Value *v, RJS_Bool del)
{
    RJS_GlobalEnv    *ge  = global_env_get(env);
    size_t            top = rjs_value_stack_save(rt);
    RJS_ObjectEnv    *oe  = (RJS_ObjectEnv*)ge->object_rec;
    RJS_String       *str;
    RJS_HashEntry    *he, **phe;
    RJS_PropertyDesc  pd;
    RJS_PropertyName  pn;
    RJS_Result        r;

    rjs_property_desc_init(rt, &pd);
    rjs_property_name_init(rt, &pn, bn->name);

    if ((r = rjs_object_get_own_property(rt, &oe->object, &pn, &pd)) == RJS_ERR)
        goto end;

    if (!r || (pd.flags & RJS_PROP_FL_CONFIGURABLE)) {
        pd.flags = RJS_PROP_FL_DATA
                |RJS_PROP_FL_WRITABLE
                |RJS_PROP_FL_ENUMERABLE;
        if (del)
            pd.flags |= RJS_PROP_FL_CONFIGURABLE;
    } else {
        pd.flags = RJS_PROP_FL_HAS_VALUE;
    }

    rjs_value_copy(rt, pd.value, v);

    if ((r = rjs_define_property_or_throw(rt, &oe->object, &pn, &pd)) == RJS_ERR)
        goto end;

    if ((r = rjs_set(rt, &oe->object, &pn, v, RJS_FALSE)) == RJS_ERR)
        goto end;

    rjs_string_to_property_key(rt, bn->name);
    str = rjs_value_get_string(rt, bn->name);

    r = rjs_hash_lookup(&ge->var_name_hash, str, &he, &phe, &rjs_hash_size_ops, rt);
    if (!r) {
        RJS_NEW(rt, he);
        rjs_hash_insert(&ge->var_name_hash, str, he, phe, &rjs_hash_size_ops, rt);
    }

    r = RJS_OK;
end:
    rjs_property_name_deinit(rt, &pn);
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}
