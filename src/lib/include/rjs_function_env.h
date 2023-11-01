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
 * Function environment internal header.
 */

#ifndef _RJS_FUNCTION_ENV_INTERNAL_H_
#define _RJS_FUNCTION_ENV_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**This binding's status.*/
typedef enum {
    RJS_THIS_STATUS_LEXICAL,       /**< Arrow function without this value.*/
    RJS_THIS_STATUS_INITIALIZED,   /**< Initialized.*/
    RJS_THIS_STATUS_UNINITIALIZED  /**< Uninitialized.*/
} RJS_ThisBindingStatus;

/**Function environment.*/
typedef struct {
    RJS_DeclEnv           decl_env;    /**< Base declarative environment.*/;
    RJS_ThisBindingStatus this_status; /**< This binding's status.*/
    RJS_Value             this_value;  /**< This value.*/
    RJS_Value             function;    /**< The function object.*/
    RJS_Value             new_target;  /**< The new target.*/
} RJS_FunctionEnv;

/**
 * Create a new function environment.
 * \param rt The current runtime.
 * \param[out] pe Return the new environment.
 * \param func The function.
 * \param new_target The new target.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_function_env_new (RJS_Runtime *rt, RJS_Environment **pe, RJS_Value *func, RJS_Value *new_target);

/**
 * Clear the function environment.
 * \param rt The current runtime.
 * \param env The function environment to be cleared.
 */
RJS_INTERNAL void
rjs_function_env_clear (RJS_Runtime *rt, RJS_Environment *env);

#ifdef __cplusplus
}
#endif

#endif

