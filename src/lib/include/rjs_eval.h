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
 * Eval internal header.
 */

#ifndef _RJS_EVAL_INTERNAL_H_
#define _RJS_EVAL_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Eval declaration instantiation.
 * \param rt The current runtime.
 * \param script The script.
 * \param decl The declaration.
 * \param var_grp The variable declaration group.
 * \param lex_grp The lexical declaration group.
 * \param func_grp The functions to be initialized.
 * \param strict In strict mode or not.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_eval_declaration_instantiation (RJS_Runtime *rt,
        RJS_Script *script,
        RJS_ScriptDecl *decl,
        RJS_ScriptBindingGroup *var_grp,
        RJS_ScriptBindingGroup *lex_grp,
        RJS_ScriptFuncDeclGroup *func_grp,
        RJS_Bool strict);

#ifdef __cplusplus
}
#endif

#endif

