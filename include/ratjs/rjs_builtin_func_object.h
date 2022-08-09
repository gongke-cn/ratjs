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
 * Built-in function object.
 */

#ifndef _RJS_BUILTIN_FUNC_OBJECT_H_
#define _RJS_BUILTIN_FUNC_OBJECT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup builtin_func Built-in function
 * Built-in function object
 * @{
 */

/**
 * Create a new built-in function.
 * \param rt The current runtime.
 * \param mod The module contains this function.
 * \param nf The native function.
 * \param len The parameters length.
 * \param name The function's name.
 * \param realm The realm.
 * \param proto The prototype value.
 * \param prefix The name prefix.
 * \param[out] f Return the new built-in function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_create_builtin_function (RJS_Runtime *rt, RJS_Value *mod, RJS_NativeFunc nf, size_t len,
        RJS_Value *name, RJS_Realm *realm, RJS_Value *proto, RJS_Value *prefix, RJS_Value *f);

/**
 * Get the module contains this function.
 * \param rt The current runtime.
 * \param func The function.
 * \param[out] mod Return the module contains this function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_get_function_module (RJS_Runtime *rt, RJS_Value *func, RJS_Value *mod);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

