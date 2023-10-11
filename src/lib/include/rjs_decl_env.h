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
 * Declarative environment internal header.
 */

#ifndef _RJS_DECL_ENV_INTERNAL_H_
#define _RJS_DECL_ENV_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**The binding is immutable.*/
#define RJS_BINDING_FL_IMMUTABLE   1
/**The binding has been initialized.*/
#define RJS_BINDING_FL_INITIALIZED 2
/**The binding may be deleted.*/
#define RJS_BINDING_FL_DELETABLE   4
/**Strict binding.*/
#define RJS_BINDING_FL_STRICT      8
/**Imported binding reference.*/
#define RJS_BINDING_FL_IMPORT      16

/**Binding.*/
typedef struct {
    RJS_HashEntry he;    /**< Hash table entry.*/
    int           flags; /**< The flags of the binding.*/
    union {
        RJS_Value       value;  /**< The value of the binding.*/
        struct {
            RJS_Value   module; /**< The referenced module.*/
            RJS_Value   name;   /**< the name of the binding in the referened module.*/
        } import;               /**< Imported binding.*/
    } b;
} RJS_Binding;

/**Declarative environment.*/
typedef struct {
    RJS_Environment env;          /**< Base environment data.*/
    RJS_Hash        binding_hash; /**< The binding hash table.*/
} RJS_DeclEnv;

/**
 * Create a new declarative environment.
 * \param rt The current runtime.
 * \param[out] pe Return the new environment.
 * \param decl The script declaration.
 * \param outer The outer environment.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_decl_env_new (RJS_Runtime *rt, RJS_Environment **pe, RJS_ScriptDecl *decl, RJS_Environment *outer);

/**
 * Initialize the declarative environment.
 * \param rt The current runtime.
 * \param de The declarative environment.
 * \param decl The script declaration.
 * \param outer The outer environment.
 */
RJS_INTERNAL void
rjs_decl_env_init (RJS_Runtime *rt, RJS_DeclEnv *de, RJS_ScriptDecl *decl, RJS_Environment *outer);

/**
 * Release the declarative environment.
 * \param rt The current runtime.
 * \param de The declarative environment.
 */
RJS_INTERNAL void
rjs_decl_env_deinit (RJS_Runtime *rt, RJS_DeclEnv *de);

/**
 * Scan the referenced things in the declarative environment.
 * \param relam The current runtime.
 * \param ptr The declarative environment.
 */
RJS_INTERNAL void
rjs_decl_env_op_gc_scan (RJS_Runtime *rt, void *ptr);

/**
 * Check if the declarative environment has the binding.
 * \param rt The current runtime.
 * \param env The environment.
 * \param n The binding's name.
 * \retval RJS_TRUE The environment has the binding.
 * \retval RJS_FALSE The environment has not the binding.
 */
RJS_INTERNAL RJS_Result
rjs_decl_env_op_has_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n);

/**
 * Create a mutable binding in a declarative environment.
 * \param rt The current runtime.
 * \param env The environment.
 * \param n The binding's name.
 * \param del The binding may be subsequently deleted.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_decl_env_op_create_mutable_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Bool del);

/**
 * Create an immutable binding in a declarative environment.
 * \param rt The current runtime.
 * \param env The environment.
 * \param n The binding's name.
 * \param strict If strict is true then attempts to set it after it has been initialized will always throw an exception.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_decl_env_op_create_immutable_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Bool strict);

/**
 * Initialize the binding in a declarative environment.
 * \param rt The current runtime.
 * \param env The environment.
 * \param n The binding's name.
 * \param v The initial value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_decl_env_op_initialize_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Value *v);

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
RJS_INTERNAL RJS_Result
rjs_decl_env_op_set_mutable_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Value *v, RJS_Bool strict);

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
RJS_INTERNAL RJS_Result
rjs_decl_env_op_get_binding_value (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Bool strict, RJS_Value *v);

/**
 * Delete a binding in a declarative environment.
 * \param rt The current runtime.
 * \param env The environment.
 * \param n The binding's name.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_decl_env_op_delete_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n);

/**
 * Check if the declarative environment has this binding.
 * \param rt The current runtime.
 * \param env The environment.
 * \retval RJS_TRUE The environment has this binding.
 * \retval RJS_FALSE The environment has not this binding.
 */
RJS_INTERNAL RJS_Result
rjs_decl_env_op_has_this_binding (RJS_Runtime *rt, RJS_Environment *env);

/**
 * Check if the declarative environemnt has the super binding.
 * \param rt The current runtime.
 * \param env The environment.
 * \retval RJS_TRUE The environment has this binding.
 * \retval RJS_FALSE The environment has not this binding.
 */
RJS_INTERNAL RJS_Result
rjs_decl_env_op_has_super_binding (RJS_Runtime *rt, RJS_Environment *env);

/**
 * Get base object of the with environment.
 * \param rt The current runtime.
 * \param env The environment.
 * \param[out] base Return the base object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_decl_env_op_with_base_object (RJS_Runtime *rt, RJS_Environment *env, RJS_Value *base);

/**
 * Clear the declarative environment.
 * \param rt The current runtime.
 * \param env The environment.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_decl_env_clear (RJS_Runtime *rt, RJS_Environment *env);

#ifdef __cplusplus
}
#endif

#endif

