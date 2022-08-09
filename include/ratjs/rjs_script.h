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
 * Script.
 */

#ifndef _RJS_SCRIPT_H_
#define _RJS_SCRIPT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup script Script
 * Script
 * @{
 */

#if ENABLE_SCRIPT

/**
 * Create a script from the file.
 * \param rt The current runtime.
 * \param[out] v Return the script value.
 * \param filename The script filename.
 * \param realm The realm.
 * \param force_strict Force the script as strict mode script.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_script_from_file (RJS_Runtime *rt, RJS_Value *v, const char *filename,
        RJS_Realm *realm, RJS_Bool force_strict);

/**
 * Create a script from a string.
 * \param rt The current runtime.
 * \param[out] v Return the script value.
 * \param str The string.
 * \param realm The realm.
 * \param force_strict Force the script as strict mode script.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_script_from_string (RJS_Runtime *rt, RJS_Value *v, RJS_Value *str,
        RJS_Realm *realm, RJS_Bool force_strict);

/**
 * Evaluate the script.
 * \param rt The current runtime.
 * \param v The script value.
 * \param rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_script_evaluation (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv);

#endif /*ENABLE_SCRIPT*/

/**
 * Disassemble the script.
 * \param rt The current runtime.
 * \param v The script value.
 * \param fp The output file.
 * \param flags The flags.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_script_disassemble (RJS_Runtime *rt, RJS_Value *v, FILE *fp, int flags);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

