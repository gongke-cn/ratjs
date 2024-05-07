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

/**
 * Initialize the environment.
 * \param rt The current runtime.
 * \param env The environment to be initialized.
 * \param decl The script declaration.
 * \param outer The outer environment.
 */
void
rjs_env_init (RJS_Runtime *rt, RJS_Environment *env, RJS_ScriptDecl *decl, RJS_Environment *outer)
{
    env->outer       = outer;
    env->script_decl = decl;

#if ENABLE_BINDING_CACHE
    env->outer_stack  = NULL;
    env->depth        = outer ? outer->depth + 1 : 0;
    env->cache_enable = RJS_TRUE;

    rjs_list_init(&env->back_refs);
#endif /*ENABLE_BINDING_CACHE*/
}

/**
 * Release the environment.
 * \param rt The current runtime.
 * \param env The environment to be released.
 */
void
rjs_env_deinit (RJS_Runtime *rt, RJS_Environment *env)
{
#if ENABLE_BINDING_CACHE
    RJS_EnvBackRef *br, *nbr;

    if (env->outer_stack) {
        RJS_EnvStackEntry *e, *le;

        e  = env->outer_stack;
        le = e + env->depth;

        while (e < le) {
            rjs_list_remove(&e->back_ref.ln);
            e ++;
        }

        RJS_DEL_N(rt, env->outer_stack, env->depth);
    }

    rjs_list_foreach_safe_c(&env->back_refs, br, nbr, RJS_EnvBackRef, ln) {
        rjs_list_remove(&br->ln);
        rjs_list_init(&br->ln);
    }
#endif /*ENABLE_BINDING_CACHE*/
}

#if ENABLE_BINDING_CACHE

/**
 * Build the outer environment stack.
 * \param rt The current runtime.
 * \param env The environment
 */
void
rjs_env_build_outer_stack (RJS_Runtime *rt, RJS_Environment *env)
{
    assert(!env->outer_stack);

    if (env->depth) {
        RJS_EnvStackEntry *e, *le;
        RJS_Environment   *outer;

        RJS_NEW_N(rt, env->outer_stack, env->depth);

        e  = env->outer_stack;
        le = e + env->depth;
        outer = env->outer;

        /*Add back references.*/
        do {
            rjs_list_append(&outer->back_refs, &e->back_ref.ln);
            e->back_ref.env = env;
            e->env = outer;

            e ++;
            outer = outer->outer;
        } while (e < le);
    }
}

/**
 * Disable the environment's cache.
 * \param env The environment.
 */
void
rjs_env_disable_cache (RJS_Environment *env)
{
    RJS_EnvBackRef *br;

    env->cache_enable = RJS_FALSE;

    rjs_list_foreach_c(&env->back_refs, br, RJS_EnvBackRef, ln) {
        RJS_Environment *er = br->env;

        er->cache_enable = RJS_FALSE;
    }
}

#endif /*ENABLE_BINDING_CACHE*/

/**
 * Add the arguments object to the environment.
 * \param rt The current runtime.
 * \param env The environment.
 * \param ao The arguments object.
 * \param strict In strict mode or not.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_env_add_arguments_object (RJS_Runtime *rt, RJS_Environment *env, RJS_Value *ao, RJS_Bool strict)
{
    RJS_Result      r;
    RJS_BindingName bn;

    rjs_binding_name_init(rt, &bn, rjs_s_arguments(rt));

    if (strict)
        r = rjs_env_create_immutable_binding(rt, env, &bn, RJS_FALSE);
    else
        r = rjs_env_create_mutable_binding(rt, env, &bn, RJS_FALSE);

    if (r == RJS_ERR)
        goto end;

    r = rjs_env_initialize_binding(rt, env, &bn, ao);
end:
    rjs_binding_name_deinit(rt, &bn);
    return r;
}
