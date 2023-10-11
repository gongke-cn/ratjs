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
 * Arguments object internal header.
 */

#ifndef _RJS_ARGUMENTS_OBJECT_INTERNAL_H_
#define _RJS_ARGUMENTS_OBJECT_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Mapped arguments object.*/
typedef struct {
    RJS_Object       object; /**< Base object data.*/
    RJS_Environment *env;    /**< The environment contains arguments.*/
    RJS_Value       *names;  /**< The arguments' names.*/
    size_t           argc;   /**< The arguments' count.*/
} RJS_ArgumentsObject;

/**
 * Create a new unmapped arguments object.
 * \param rt The current runtime.
 * \param[out] v Return the arguments object.
 * \param args The arguments.
 * \param argc The arguments' count.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error. 
 */
RJS_INTERNAL RJS_Result
rjs_unmapped_arguments_object_new (RJS_Runtime *rt, RJS_Value *v, RJS_Value *args, size_t argc);

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
RJS_INTERNAL RJS_Result
rjs_mapped_arguments_object_new (RJS_Runtime *rt, RJS_Value *v, RJS_Value *f,
        RJS_ScriptBindingGroup *bg, RJS_Value *args, size_t argc, RJS_Environment *env);

#ifdef __cplusplus
}
#endif

#endif

