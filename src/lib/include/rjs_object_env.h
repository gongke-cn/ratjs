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
 * Object environment internal header.
 */

#ifndef _RJS_OBJECT_ENV_INTERNAL_H_
#define _RJS_OBJECT_ENV_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Object environment.*/
typedef struct {
    RJS_Environment env;     /**< Environment data.*/
    RJS_Value       object;  /**< The object.*/
    RJS_Bool        is_with; /**< Is with environment.*/
} RJS_ObjectEnv;

/**
 * Create a new object environment.
 * \param rt The current runtime.
 * \param[out] pe Return the new environment.
 * \param o The object.
 * \param is_with Is with environment.
 * \param decl The script declaration.
 * \param outer The outer environment.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_object_env_new (RJS_Runtime *rt, RJS_Environment **pe, RJS_Value *o, RJS_Bool is_with,
        RJS_ScriptDecl *decl, RJS_Environment *outer);

#ifdef __cplusplus
}
#endif

#endif

