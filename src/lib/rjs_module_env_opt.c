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

/*Scan the referenced things in the module environment.*/
static void
module_env_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    rjs_decl_env_op_gc_scan(rt, ptr);
}

/*Free the module environment.*/
static void
module_env_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_ModuleEnv *me = ptr;

    rjs_decl_env_deinit(rt, &me->decl_env);

    RJS_DEL(rt, me);
}

/*Get the binding's value in the module environment.*/
static RJS_Result
module_env_op_get_binding_value (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Bool strict, RJS_Value *v)
{
    RJS_ModuleEnv *me = (RJS_ModuleEnv*)env;
    RJS_String    *str;
    RJS_HashEntry *he;
    RJS_Binding   *b;
    RJS_Result     r;

    assert(strict);

    rjs_string_to_property_key(rt, n->name);
    str = rjs_value_get_string(rt, n->name);

    r = rjs_hash_lookup(&me->decl_env.binding_hash, str, &he, NULL, &rjs_hash_size_ops, rt);
    assert(r);

    b = RJS_CONTAINER_OF(he, RJS_Binding, he);
    if (b->flags & RJS_BINDING_FL_IMPORT) {
        RJS_Module     *mod;
        RJS_BindingName bn;

        mod = rjs_value_get_gc_thing(rt, &b->b.import.module);
        if (!mod->env)
            return rjs_throw_reference_error(rt, _("module environment is not created"));

        rjs_binding_name_init(rt, &bn, &b->b.import.name);

        r = rjs_env_get_binding_value(rt, mod->env, &bn, strict, v);

        rjs_binding_name_deinit(rt, &bn);

        return r;
    }

    if (!(b->flags & RJS_BINDING_FL_INITIALIZED))
        return rjs_throw_reference_error(rt, _("binding \"%s\" is not initialized"),
                rjs_string_to_enc_chars(rt, n->name, NULL, NULL));

    rjs_value_copy(rt, v, &b->b.value);
    return RJS_OK;
}

/*Deleate a binding from the module environment.*/
static RJS_Result
module_env_op_delete_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n)
{
    assert(0);
    return RJS_ERR;
}

/*Check if the module environment has this binding.*/
static RJS_Result
module_env_op_has_this_binding (RJS_Runtime *rt, RJS_Environment *env)
{
    return RJS_TRUE;
}

/*Get this binding of the module environment.*/
static RJS_Result
module_env_op_get_this_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_Value *v)
{
    rjs_value_set_undefined(rt, v);
    return RJS_OK;
}

/*Module environment operation functions.*/
static const RJS_EnvOps
module_env_ops = {
    {
        RJS_GC_THING_MODULE_ENV,
        module_env_op_gc_scan,
        module_env_op_gc_free
    },
    rjs_decl_env_op_has_binding,
    rjs_decl_env_op_create_mutable_binding,
    rjs_decl_env_op_create_immutable_binding,
    rjs_decl_env_op_initialize_binding,
    rjs_decl_env_op_set_mutable_binding,
    module_env_op_get_binding_value,
    module_env_op_delete_binding,
    module_env_op_has_this_binding,
    rjs_decl_env_op_has_super_binding,
    rjs_decl_env_op_with_base_object,
    module_env_op_get_this_binding
};

/**
 * Create a new module environment.
 * \param rt The current runtime.
 * \param[out] pe Return the new environment.
 * \param outer The outer environment.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_module_env_new (RJS_Runtime *rt, RJS_Environment **pe, RJS_Environment *outer)
{
    RJS_ModuleEnv *me;

    RJS_NEW(rt, me);

    rjs_decl_env_init(rt, &me->decl_env, NULL, outer);

    *pe = &me->decl_env.env;

    rjs_gc_add(rt, me, &module_env_ops.gc_thing_ops);

    return RJS_OK;
}

/**
 * Create an import binding.
 * \param rt The current runtime.
 * \param env The module environment.
 * \param n The name of the binding.
 * \param mod The reference module.
 * \param n2 The name in the referenced module.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_env_create_import_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_Value *n,
        RJS_Value *mod, RJS_Value *n2)
{
    RJS_ModuleEnv *me;
    RJS_HashEntry *he, **phe;
    RJS_Binding   *b;
    RJS_Result     r;
    RJS_String    *str;

    assert(env->gc_thing.ops == &module_env_ops.gc_thing_ops);
    me = (RJS_ModuleEnv*)env;

    rjs_string_to_property_key(rt, n);
    str = rjs_value_get_string(rt, n);

    r = rjs_hash_lookup(&me->decl_env.binding_hash, str, &he, &phe, &rjs_hash_size_ops, rt);
    assert(r == RJS_FALSE);

    RJS_NEW(rt, b);

    b->flags = RJS_BINDING_FL_IMPORT|RJS_BINDING_FL_INITIALIZED|RJS_BINDING_FL_IMMUTABLE;

    rjs_value_copy(rt, &b->b.import.module, mod);
    rjs_value_copy(rt, &b->b.import.name, n2);

    rjs_hash_insert(&me->decl_env.binding_hash, str, &b->he, phe, &rjs_hash_size_ops, rt);

    return RJS_OK;
}
