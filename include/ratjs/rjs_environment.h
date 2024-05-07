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

/**
 * \file
 * Environment.
 */

#ifndef _RJS_ENVIRONMENT_H_
#define _RJS_ENVIRONMENT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup env Environment
 * Environment
 * @{
 */

/**
 * Initialize the binding name.
 * \param rt The current runtime.
 * \param bn The binding name to be initialized.
 * \param n The name value.
 */
static inline void
rjs_binding_name_init (RJS_Runtime *rt, RJS_BindingName *bn, RJS_Value *n)
{
    bn->name = n;

#if ENABLE_BINDING_CACHE
    bn->env_idx     = 0xffff;
    bn->binding_idx = 0xffff;
#endif /*ENABLE_BINDING_CACHE*/
}

/**
 * Release the binding name.
 * \param rt The current runtime.
 * \param bn The binding name to be released.
 */
static inline void
rjs_binding_name_deinit (RJS_Runtime *rt, RJS_BindingName *bn)
{
}

/**
 * Check if the environment has the binding.
 * \param rt The current runtime.
 * \param env The environment.
 * \param n The binding's name.
 * \retval RJS_TRUE The environment has the binding.
 * \retval RJS_FALSE The environment has not the binding.
 */
static inline RJS_Result
rjs_env_has_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n)
{
    RJS_EnvOps *ops = (RJS_EnvOps*)((RJS_GcThing*)env)->ops;

    return ops->has_binding(rt, env, n);
}

/**
 * Create a mutable binding.
 * \param rt The current runtime.
 * \param env The environment.
 * \param n The binding's name.
 * \param del The binding may be subsequently deleted.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_env_create_mutable_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Bool del)
{
    RJS_EnvOps *ops = (RJS_EnvOps*)((RJS_GcThing*)env)->ops;

    return ops->create_mutable_binding(rt, env, n, del);
}

/**
 * Create an immutable binding.
 * \param rt The current runtime.
 * \param env The environment.
 * \param n The binding's name.
 * \param strict If strict is true then attempts to set it after it has been initialized will always throw an exception.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_env_create_immutable_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Bool strict)
{
    RJS_EnvOps *ops = (RJS_EnvOps*)((RJS_GcThing*)env)->ops;

    return ops->create_immutable_binding(rt, env, n, strict);
}

/**
 * Initialize the binding.
 * \param rt The current runtime.
 * \param env The environment.
 * \param n The binding's name.
 * \param v The initial value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_env_initialize_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Value *v)
{
    RJS_EnvOps *ops = (RJS_EnvOps*)((RJS_GcThing*)env)->ops;

    return ops->initialize_binding(rt, env, n, v);
}

/**
 * Set the mutable binding.
 * \param rt The current runtime.
 * \param env The environment.
 * \param n The binding's name.
 * \param v The new value of the binding.
 * \param strict If strict is true and the binding cannot be set throw a TypeError exception.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_env_set_mutable_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Value *v, RJS_Bool strict)
{
    RJS_EnvOps *ops = (RJS_EnvOps*)((RJS_GcThing*)env)->ops;

    return ops->set_mutable_binding(rt, env, n, v, strict);
}

/**
 * Get the binding's value.
 * \param rt The current runtime.
 * \param env The environment.
 * \param n The binding's name.
 * \param strict If strict is true and the binding does not exist throw a ReferenceError exception.
 * If the binding exists but is uninitialized a ReferenceError is thrown, regardless of the value of strict.
 * \param[out] v Return the binding's value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_env_get_binding_value (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Bool strict, RJS_Value *v)
{
    RJS_EnvOps *ops = (RJS_EnvOps*)((RJS_GcThing*)env)->ops;

    return ops->get_binding_value(rt, env, n, strict, v);
}

/**
 * Delete a binding.
 * \param rt The current runtime.
 * \param env The environment.
 * \param n The binding's name.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_env_delete_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n)
{
    RJS_EnvOps *ops = (RJS_EnvOps*)((RJS_GcThing*)env)->ops;

    return ops->delete_binding(rt, env, n);
}

/**
 * Check if the environment has this binding.
 * \param rt The current runtime.
 * \param env The environment.
 * \retval RJS_TRUE The environment has this binding.
 * \retval RJS_FALSE The environment has not this binding.
 */
static inline RJS_Result
rjs_env_has_this_binding (RJS_Runtime *rt, RJS_Environment *env)
{
    RJS_EnvOps *ops = (RJS_EnvOps*)((RJS_GcThing*)env)->ops;

    return ops->has_this_binding(rt, env);
}

/**
 * Check if the environemnt has the super binding.
 * \param rt The current runtime.
 * \param env The environment.
 * \retval RJS_TRUE The environment has this binding.
 * \retval RJS_FALSE The environment has not this binding.
 */
static inline RJS_Result
rjs_env_has_super_binding (RJS_Runtime *rt, RJS_Environment *env)
{
    RJS_EnvOps *ops = (RJS_EnvOps*)((RJS_GcThing*)env)->ops;

    return ops->has_super_binding(rt, env);
}

/**
 * Get base object of the with environment.
 * \param rt The current runtime.
 * \param env The environment.
 * \param[out] base Return the base object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_env_with_base_object (RJS_Runtime *rt, RJS_Environment *env, RJS_Value *base)
{
    RJS_EnvOps *ops = (RJS_EnvOps*)((RJS_GcThing*)env)->ops;

    return ops->with_base_object(rt, env, base);
}

/**
 * Get this binding of the environment.
 * \param rt The current runtime.
 * \param env The environment.
 * \param[out] v Return this binding value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_env_get_this_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_Value *v)
{
    RJS_EnvOps *ops = (RJS_EnvOps*)((RJS_GcThing*)env)->ops;

    return ops->get_this_binding(rt, env, v);
}

/**
 * Add the arguments object to the environment.
 * \param rt The current runtime.
 * \param env The environment.
 * \param ao The arguments object.
 * \param strict In strict mode or not.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_env_add_arguments_object (RJS_Runtime *rt, RJS_Environment *env, RJS_Value *ao, RJS_Bool strict);

/**
 * Bind this value to the environemnt..
 * \param rt The current runtime.
 * \param env The environment.
 * \param v This value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_env_bind_this_value (RJS_Runtime *rt, RJS_Environment *env, RJS_Value *v);

/**
 * Get super base object of the environment.
 * \param rt The current runtime.
 * \param env The environment.
 * \param[out] sb Return the super base value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_env_get_super_base (RJS_Runtime *rt, RJS_Environment *env, RJS_Value *sb);

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
extern RJS_Result
rjs_env_create_import_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_Value *n,
        RJS_Value *mod, RJS_Value *n2);

/**
 * Check if the global environment has the variable declaration.
 * \param rt The current runtime.
 * \param env The environment.
 * \param bn The binding name of the variable.
 * \retval RJS_TRUE The environment has the declaration.
 * \retval RJS_FALSE The environment has not the declaration.
 */
extern RJS_Result
rjs_env_has_var_declaration (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *bn);

/**
 * Check if the global environment has the lexical declaration.
 * \param rt The current runtime.
 * \param env The environment.
 * \param bn The binding name of the declaration.
 * \retval RJS_TRUE The environment has the declaration.
 * \retval RJS_FALSE The environment has not the declaration.
 */
extern RJS_Result
rjs_env_has_lexical_declaration (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *bn);

/**
 * Check if the global environment has the restirected global property.
 * \param rt The current runtime.
 * \param env The environment.
 * \param bn The binding name of the declaration.
 * \retval RJS_TRUE The environment has the property.
 * \retval RJS_FALSE The environment has not the property.
 */
extern RJS_Result
rjs_env_has_restricted_global_property (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *bn);

/**
 * Check if the global environment can declare the global variable.
 * \param rt The current runtime.
 * \param env The environment.
 * \param bn The binding name of the variable.
 * \retval RJS_TRUE The property can be declared.
 * \retval RJS_FALSE The property cannot be declared.
 */
extern RJS_Result
rjs_env_can_declare_global_var (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *bn);

/**
 * Check if the global environment can declare the global function.
 * \param rt The current runtime.
 * \param env The environment.
 * \param bn The binding name of the function.
 * \retval RJS_TRUE The function can be declared.
 * \retval RJS_FALSE The function cannot be declared.
 */
extern RJS_Result
rjs_env_can_declare_global_function (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *bn);

/**
 * Create the global variable binding.
 * \param rt The current runtime.
 * \param env The environment.
 * \param bn The binding name of the variable.
 * \param del The binding is deletable.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_env_create_global_var_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *bn, RJS_Bool del);

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
extern RJS_Result
rjs_env_create_global_function_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *bn, RJS_Value *v, RJS_Bool del);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

