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
 * Symbol.
 */

#ifndef _RJS_SYMBOL_H_
#define _RJS_SYMBOL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup symbol Symbol
 * Symbol
 * @{
 */

/**
 * Create a new symbol.
 * \param rt The current runtime.
 * \param[out] v Return the symbol value.
 * \param desc The description of the symbol.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_symbol_new (RJS_Runtime *rt, RJS_Value *v, RJS_Value *desc);

/**
 * Lookup the internal symbol by its name.
 * \param rt The current runtime.
 * \param name The name of the symbol.
 * \return The symbol value.
 */
extern RJS_Value*
rjs_internal_symbol_lookup (RJS_Runtime *rt, const char *name);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

