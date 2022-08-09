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
 * Script function object.
 */

#ifndef _RJS_SCRIPT_FUNC_OBJECT_H_
#define _RJS_SCRIPT_FUNC_OBJECT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup script_func Script function
 * Script function object
 * @{
 */

/**
 * Create a dynamic function.
 * \param rt The current runtime.
 * \param constr The constructor.
 * \param nt The new target.
 * \param flags RJS_FUNC_FL_ASYNC, RJS_FUNC_FL_GENERATOR.
 * \param args The arguments.
 * \param argc The arguments' count.
 * \param[out] func Return the function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_create_dynamic_function (RJS_Runtime *rt, RJS_Value *constr, RJS_Value *nt,
        int flags, RJS_Value *args, size_t argc, RJS_Value *func);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

