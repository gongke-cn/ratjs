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
 * Environment internal header.
 */

#ifndef _RJS_ENVIRONMENT_INTERNAL_H_
#define _RJS_ENVIRONMENT_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Script declaration.*/
typedef struct RJS_ScriptDecl_s RJS_ScriptDecl;

#if ENABLE_BINDING_CACHE
/**Envrionment back reference.*/
typedef struct {
    RJS_List         ln;  /**< List node data.*/
    RJS_Environment *env; /**< The environment has reference.*/
} RJS_EnvBackRef;

/**Environment stack entry.*/
typedef struct {
    RJS_Environment *env;      /**< The referenced environment.*/
    RJS_EnvBackRef   back_ref; /**< The back reference node.*/
} RJS_EnvStackEntry;
#endif /*ENABLE_BINDING_CACHE*/

/**Environment.*/
struct RJS_Environment_s {
    RJS_GcThing        gc_thing;    /**< Base GC thing data.*/
    RJS_Environment   *outer;       /**< The outer environment.*/
    RJS_ScriptDecl    *script_decl; /**< The script declaration.*/
#if ENABLE_BINDING_CACHE
    RJS_EnvStackEntry *outer_stack; /**< The outer environment stack.*/
    RJS_List           back_refs;   /**< The back reference list.*/
    int                depth;       /**< The environment stack's depth.*/
    RJS_Bool           cache_enable;/**< Cache is enabled.*/
#endif /*ENABLE_BINDING_CACHE*/
};

/**
 * Initialize the environment.
 * \param rt The current runtime.
 * \param env The environment to be initialized.
 * \param decl The script declaration.
 * \param outer The outer environment.
 */
RJS_INTERNAL void
rjs_env_init (RJS_Runtime *rt, RJS_Environment *env, RJS_ScriptDecl *decl, RJS_Environment *outer);

/**
 * Release the environment.
 * \param rt The current runtime.
 * \param env The environment to be released.
 */
RJS_INTERNAL void
rjs_env_deinit (RJS_Runtime *rt, RJS_Environment *env);

#if ENABLE_BINDING_CACHE
/**
 * Build the outer environment stack.
 * \param rt The current runtime.
 * \param env The environment
 */
RJS_INTERNAL void
rjs_env_build_outer_stack (RJS_Runtime *rt, RJS_Environment *env);

/**
 * Disable the environment's cache.
 * \param env The environment.
 */
RJS_INTERNAL void
rjs_env_disable_cache (RJS_Environment *env);
#endif /*ENABLE_BINDING_CACHE*/

#ifdef __cplusplus
}
#endif

#endif

