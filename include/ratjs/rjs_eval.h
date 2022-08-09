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
 * Eval.
 */

#ifndef _RJS_EVAL_H_
#define _RJS_EVAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup eval eval
 * "eval" function object
 * @{
 */

/**
 * Compile and evalute script.
 * \param rt The current runtime.
 * \param[out] script Return the script value.
 * \param x The input string.
 * \param realm The realm.
 * \param strict For the eval as strict mode code.
 * \param direct Direct call "eval".
 * \retval RJS_OK On success.
 * \retval RJS_FALSE The input is not a string.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_eval_from_string (RJS_Runtime *rt, RJS_Value *script, RJS_Value *x, RJS_Realm *realm,
        RJS_Bool strict, RJS_Bool direct);

/**
 * Evaluate the "eval" script.
 * \param rt The current runtime.
 * \param scriptv The script.
 * \param direct Direct call "eval".
 * \param[out] rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_eval_evaluation (RJS_Runtime *rt, RJS_Value *scriptv, RJS_Bool direct, RJS_Value *rv);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

