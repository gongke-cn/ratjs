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
 * Character stream input.
 */

#ifndef _RJS_INPUT_H_
#define _RJS_INPUT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Character stream input.*/
typedef struct RJS_Input_s RJS_Input;

/**Character stream input.*/
struct RJS_Input_s {
    int        flags;  /**< the flags of the input.*/
    int        line;   /**< The current line number.*/
    int        column; /**< The current column number.*/
    char      *name;   /**< The input's name.*/
    RJS_Value *str;    /**< The string value.*/
    size_t     length; /**< The length of the string.*/
    size_t     pos;    /**< The current read position.*/
};

/**
 * Initialize a string input.
 * \param rt The current runtime.
 * \param si The string input to be initialized.
 * \param s The string.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_string_input_init (RJS_Runtime *rt, RJS_Input *si, RJS_Value *s);

/**
 * Initialize a file input.
 * \param rt The current runtime.
 * \param fi The file input to be inintialized.
 * \param filename The filename.
 * \param enc The file's character encoding.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_file_input_init (RJS_Runtime *rt, RJS_Input *fi, const char *filename,
        const char *enc);

/**
 * Release an unused input.
 * \param rt The current runtime.
 * \param input The input to be released.
 */
void
rjs_input_deinit (RJS_Runtime *rt, RJS_Input *input);

#ifdef __cplusplus
}
#endif

#endif

