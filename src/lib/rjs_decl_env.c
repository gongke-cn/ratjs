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

/*Free the declarative environment.*/
static void
decl_env_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_DeclEnv *de = ptr;

    rjs_decl_env_deinit(rt, de);

    RJS_DEL(rt, de);
}

/*Declarative environment operation functions.*/
static const RJS_EnvOps
decl_env_ops = {
    {
        RJS_GC_THING_DECL_ENV,
        rjs_decl_env_op_gc_scan,
        decl_env_op_gc_free
    },
    rjs_decl_env_op_has_binding,
    rjs_decl_env_op_create_mutable_binding,
    rjs_decl_env_op_create_immutable_binding,
    rjs_decl_env_op_initialize_binding,
    rjs_decl_env_op_set_mutable_binding,
    rjs_decl_env_op_get_binding_value,
    rjs_decl_env_op_delete_binding,
    rjs_decl_env_op_has_this_binding,
    rjs_decl_env_op_has_super_binding,
    rjs_decl_env_op_with_base_object,
    NULL
};

/**
 * Create a new declarative environment.
 * \param rt The current runtime.
 * \param[out] pe Return the new environment.
 * \param decl The script declaration.
 * \param outer The outer environment.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_decl_env_new (RJS_Runtime *rt, RJS_Environment **pe, RJS_ScriptDecl *decl, RJS_Environment *outer)
{
    RJS_DeclEnv *de;

    RJS_NEW(rt, de);

    rjs_decl_env_init(rt, de, decl, outer);

    *pe = &de->env;

    rjs_gc_add(rt, de, &decl_env_ops.gc_thing_ops);

    return RJS_OK;
}

/**
 * Initialize the declarative environment.
 * \param rt The current runtime.
 * \param de The declarative environment.
 * \param decl The script declaration.
 * \param outer The outer environment.
 */
void
rjs_decl_env_init (RJS_Runtime *rt, RJS_DeclEnv *de, RJS_ScriptDecl *decl, RJS_Environment *outer)
{
    de->env.outer       = outer;
    de->env.script_decl = decl;

    rjs_hash_init(&de->binding_hash);
}

/**
 * Release the declarative environment.
 * \param rt The current runtime.
 * \param de The declarative environment.
 */
void
rjs_decl_env_deinit (RJS_Runtime *rt, RJS_DeclEnv *de)
{
    rjs_decl_env_clear(rt, &de->env);
    rjs_hash_deinit(&de->binding_hash, &rjs_hash_size_ops, rt);
}

/**
 * Scan the referenced things in the declarative environment.
 * \param rt The current runtime.
 * \param ptr The declarative environment.
 */
void
rjs_decl_env_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_DeclEnv *de = ptr;
    RJS_Binding *b;
    size_t       i;

    if (de->env.outer)
        rjs_gc_mark(rt, de->env.outer);

    rjs_hash_foreach_c(&de->binding_hash, i, b, RJS_Binding, he) {
        rjs_gc_mark(rt, b->he.key);

        if (b->flags & RJS_BINDING_FL_IMPORT) {
            rjs_gc_scan_value(rt, &b->b.import.module);
            rjs_gc_scan_value(rt, &b->b.import.name);
        } else {
            rjs_gc_scan_value(rt, &b->b.value);
        }
    }
}

/*Get the string from the binding name.*/
static RJS_String*
binding_name_get_string (RJS_Runtime *rt, RJS_BindingName *n)
{
    rjs_string_to_property_key(rt, n->name);
    
    return rjs_value_get_string(rt, n->name);
}

/**
 * Check if the declarative environment has the binding.
 * \param rt The current runtime.
 * \param env The environment.
 * \param n The binding's name.
 * \retval RJS_TRUE The environment has the binding.
 * \retval RJS_FALSE The environment has not the binding.
 */
RJS_Result
rjs_decl_env_op_has_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n)
{
    RJS_DeclEnv   *de  = (RJS_DeclEnv*)env;
    RJS_String    *str = binding_name_get_string(rt, n);
    RJS_HashEntry *he;

    return rjs_hash_lookup(&de->binding_hash, str, &he, NULL, &rjs_hash_size_ops, rt);
}

/**
 * Create a mutable binding in a declarative environment.
 * \param rt The current runtime.
 * \param env The environment.
 * \param n The binding's name.
 * \param del The binding may be subsequently deleted.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_decl_env_op_create_mutable_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Bool del)
{
    RJS_DeclEnv   *de  = (RJS_DeclEnv*)env;
    RJS_String    *str = binding_name_get_string(rt, n);
    RJS_HashEntry *he, **phe;
    RJS_Binding   *b;
    RJS_Result     r;

    r = rjs_hash_lookup(&de->binding_hash, str, &he, &phe, &rjs_hash_size_ops, rt);
    assert(r == RJS_FALSE);

    RJS_NEW(rt, b);

    b->flags = del ? RJS_BINDING_FL_DELETABLE : 0;
    rjs_value_set_undefined(rt, &b->b.value);

    rjs_hash_insert(&de->binding_hash, str, &b->he, phe, &rjs_hash_size_ops, rt);

    return RJS_OK;
}

/**
 * Create an immutable binding in a declarative environment.
 * \param rt The current runtime.
 * \param env The environment.
 * \param n The binding's name.
 * \param strict If strict is true then attempts to set it after it has been initialized will always throw an exception.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_decl_env_op_create_immutable_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Bool strict)
{
    RJS_DeclEnv   *de  = (RJS_DeclEnv*)env;
    RJS_String    *str = binding_name_get_string(rt, n);
    RJS_HashEntry *he, **phe;
    RJS_Binding   *b;
    RJS_Result     r;

    r = rjs_hash_lookup(&de->binding_hash, str, &he, &phe, &rjs_hash_size_ops, rt);
    assert(r == RJS_FALSE);

    RJS_NEW(rt, b);

    b->flags = RJS_BINDING_FL_IMMUTABLE;
    if (strict)
        b->flags |= RJS_BINDING_FL_STRICT;

    rjs_value_set_undefined(rt, &b->b.value);

    rjs_hash_insert(&de->binding_hash, str, &b->he, phe, &rjs_hash_size_ops, rt);

    return RJS_OK;
}

/**
 * Initialize the binding in a declarative environment.
 * \param rt The current runtime.
 * \param env The environment.
 * \param n The binding's name.
 * \param v The initial value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_decl_env_op_initialize_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Value *v)
{
    RJS_DeclEnv   *de  = (RJS_DeclEnv*)env;
    RJS_String    *str = binding_name_get_string(rt, n);
    RJS_HashEntry *he;
    RJS_Binding   *b;
    RJS_Result     r;

    r = rjs_hash_lookup(&de->binding_hash, str, &he, NULL, &rjs_hash_size_ops, rt);
    assert(r == RJS_TRUE);

    b = RJS_CONTAINER_OF(he, RJS_Binding, he);
    assert(!(b->flags & RJS_BINDING_FL_INITIALIZED));

    rjs_value_copy(rt, &b->b.value, v);
    b->flags |= RJS_BINDING_FL_INITIALIZED;

    return RJS_OK;
}

/**
 * Set the mutable binding in a declarative environment.
 * \param rt The current runtime.
 * \param env The environment.
 * \param n The binding's name.
 * \param v The new value of the binding.
 * \param strict If strict is true and the binding cannot be set throw a TypeError exception.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_decl_env_op_set_mutable_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Value *v, RJS_Bool strict)
{
    RJS_DeclEnv   *de  = (RJS_DeclEnv*)env;
    RJS_String    *str = binding_name_get_string(rt, n);
    RJS_HashEntry *he;
    RJS_Binding   *b;
    RJS_Result     r;

    r = rjs_hash_lookup(&de->binding_hash, str, &he, NULL, &rjs_hash_size_ops, rt);
    if (r == RJS_FALSE) {
        if (strict)
            return rjs_throw_reference_error(rt, _("binding cannot be created automatically in strict mode"));

        rjs_env_create_mutable_binding(rt, env, n, RJS_TRUE);
        rjs_env_initialize_binding(rt, env, n, v);
        return RJS_OK;
    }

    b = RJS_CONTAINER_OF(he, RJS_Binding, he);
    if (b->flags & RJS_BINDING_FL_STRICT)
        strict = RJS_TRUE;

    if (!(b->flags & RJS_BINDING_FL_INITIALIZED))
        return rjs_throw_reference_error(rt, _("binding \"%s\" is not initialized"),
                rjs_string_to_enc_chars(rt, n->name, NULL, NULL));

    if (b->flags & RJS_BINDING_FL_IMMUTABLE) {
        if (strict) {
            return rjs_throw_type_error(rt, _("binding \"%s\" is immutable"),
                    rjs_string_to_enc_chars(rt, n->name, NULL, NULL));
        }
    } else {
        rjs_value_copy(rt, &b->b.value, v);
    }

    return RJS_OK;
}

/**
 * Get the binding's value in a declarative environment.
 * \param rt The current runtime.
 * \param env The environment.
 * \param n The binding's name.
 * \param strict If strict is true and the binding does not exist throw a ReferenceError exception.
 * If the binding exists but is uninitialized a ReferenceError is thrown, regardless of the value of strict.
 * \param[out] v Return the binding's value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_decl_env_op_get_binding_value (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Bool strict, RJS_Value *v)
{
    RJS_DeclEnv   *de  = (RJS_DeclEnv*)env;
    RJS_String    *str = binding_name_get_string(rt, n);
    RJS_HashEntry *he;
    RJS_Binding   *b;
    RJS_Result     r;

    r = rjs_hash_lookup(&de->binding_hash, str, &he, NULL, &rjs_hash_size_ops, rt);
    assert(r == RJS_TRUE);

    b = RJS_CONTAINER_OF(he, RJS_Binding, he);

    if (!(b->flags & RJS_BINDING_FL_INITIALIZED))
        return rjs_throw_reference_error(rt, _("binding \"%s\" is not initialized"),
                rjs_string_to_enc_chars(rt, n->name, NULL, NULL));

    rjs_value_copy(rt, v, &b->b.value);
    return RJS_OK;
}

/**
 * Delete a binding in a declarative environment.
 * \param rt The current runtime.
 * \param env The environment.
 * \param n The binding's name.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_decl_env_op_delete_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n)
{
    RJS_DeclEnv   *de  = (RJS_DeclEnv*)env;
    RJS_String    *str = binding_name_get_string(rt, n);
    RJS_HashEntry *he, **phe;
    RJS_Binding   *b;
    RJS_Result     r;

    r = rjs_hash_lookup(&de->binding_hash, str, &he, &phe, &rjs_hash_size_ops, rt);
    assert(r == RJS_TRUE);

    b = RJS_CONTAINER_OF(he, RJS_Binding, he);

    if (!(b->flags & RJS_BINDING_FL_DELETABLE))
        return RJS_FALSE;

    rjs_hash_remove(&de->binding_hash, phe, rt);
    RJS_DEL(rt, b);
    
    return RJS_TRUE;
}

/**
 * Check if the declarative environment has this binding.
 * \param rt The current runtime.
 * \param env The environment.
 * \retval RJS_TRUE The environment has this binding.
 * \retval RJS_FALSE The environment has not this binding.
 */
RJS_Result
rjs_decl_env_op_has_this_binding (RJS_Runtime *rt, RJS_Environment *env)
{
    return RJS_FALSE;
}

/**
 * Check if the declarative environemnt has the super binding.
 * \param rt The current runtime.
 * \param env The environment.
 * \retval RJS_TRUE The environment has this binding.
 * \retval RJS_FALSE The environment has not this binding.
 */
RJS_Result
rjs_decl_env_op_has_super_binding (RJS_Runtime *rt, RJS_Environment *env)
{
    return RJS_FALSE;
}

/**
 * Get base object of the declarative environment.
 * \param rt The current runtime.
 * \param env The environment.
 * \param[out] base Return the base object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_decl_env_op_with_base_object (RJS_Runtime *rt, RJS_Environment *env, RJS_Value *base)
{
    rjs_value_set_undefined(rt, base);
    return RJS_OK;
}

/**
 * Clear the declarative environment.
 * \param rt The current runtime.
 * \param env The environment.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_decl_env_clear (RJS_Runtime *rt, RJS_Environment *env)
{
    RJS_DeclEnv *de = (RJS_DeclEnv*)env;
    RJS_Binding *b, *nb;
    size_t       i;

    rjs_hash_foreach_safe_c(&de->binding_hash, i, b, nb, RJS_Binding, he) {
        RJS_DEL(rt, b);
    }

    rjs_hash_deinit(&de->binding_hash, &rjs_hash_size_ops, rt);
    rjs_hash_init(&de->binding_hash);

    return RJS_OK;
}
