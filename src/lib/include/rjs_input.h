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
 * Character stream input internal header.
 */

#ifndef _RJS_INPUT_INTERNAL_H_
#define _RJS_INPUT_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**The last character is new line.*/
#define RJS_INPUT_FL_NEW_LINE   1

/**Read error.*/
#define RJS_INPUT_FL_ERROR      2

/**Do not output error message.*/
#define RJS_INPUT_FL_NO_MSG     4

/**Convert \r\n to \n*/
#define RJS_INPUT_FL_CRLF_TO_LF 8

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

/**The location.*/
typedef struct {
    int    first_line;    /**< The first character's line number*/
    int    first_column;  /**< The first character's column number.*/
    size_t first_pos;     /**< The first character's position.*/
    int    last_line;     /**< The last character's line number.*/
    int    last_column;   /**< The last character's column number.*/
    size_t last_pos;      /**< The last character's position.*/
} RJS_Location;

/**Message type.*/
typedef enum {
    RJS_MESSAGE_NOTE,    /**< Note message.*/
    RJS_MESSAGE_WARNING, /**< Warning message.*/
    RJS_MESSAGE_ERROR    /**< Error message.*/
} RJS_MessageType;

/**End of the input.*/
#define RJS_INPUT_END -1

/**
 * Initialize a string input.
 * \param rt The current runtime.
 * \param si The string input to be initialized.
 * \param s The string.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
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
RJS_INTERNAL RJS_Result
rjs_file_input_init (RJS_Runtime *rt, RJS_Input *fi, const char *filename,
        const char *enc);

/**
 * Release an unused input.
 * \param rt The current runtime.
 * \param input The input to be released.
 */
RJS_INTERNAL void
rjs_input_deinit (RJS_Runtime *rt, RJS_Input *input);

/**
 * Check if the input has error.
 * \param input The input.
 * \retval RJS_TRUE The input has error.
 * \retval RJS_FALSE The input has not any error. 
 */
static inline RJS_Bool
rjs_input_error (RJS_Input *input)
{
    return input->flags & RJS_INPUT_FL_ERROR;
}

/**
 * Get an unicode from the input.
 * \param rt The current runtime.
 * \param input The input.
 * \return The next unicode.
 * \retval RJS_INPUT_END The input is end.
 */
RJS_INTERNAL int
rjs_input_get_uc (RJS_Runtime *rt, RJS_Input *input);

/**
 * Push back an unicode to the input.
 * \param rt The current runtime.
 * \param input The input.
 * \param c The unicode to be pushed back.
 */
RJS_INTERNAL void
rjs_input_unget_uc (RJS_Runtime *rt, RJS_Input *input, int c);

/**
 * Get the input's current position.
 * \param input The input.
 * \param[out] pline Return the current line number.
 * \param[out] pcol Return the current column number.
 * \param[out] ppos Return the current position.
 */
static inline void
rjs_input_get_position (RJS_Input *input, int *pline, int *pcol, size_t *ppos)
{
    *pline = input->line;
    *pcol  = input->column;
    *ppos  = input->pos;
}

/**
 * Get the location of the current character read from the input.
 * \param input The input.
 * \param loc The location.
 */
static inline void
rjs_input_get_location (RJS_Input *input, RJS_Location *loc)
{
    rjs_input_get_position(input, &loc->first_line,
            &loc->first_column, &loc->first_pos);
    rjs_input_get_position(input, &loc->last_line,
            &loc->last_column, &loc->last_pos);
}

/**
 * Output the input message's head.
 * \param rt The current runtime.
 * \param input The input.
 * \param type The message type.
 * \param loc The location where generate the message.
 */
RJS_INTERNAL void
rjs_message_head (RJS_Runtime *rt, RJS_Input *input,
        RJS_MessageType type, RJS_Location *loc);

/**
 * Output the input message.
 * \param rt The current runtime.
 * \param input The input.
 * \param type The message type.
 * \param loc The location where generate the message.
 * \param fmt The output format pattern.
 * \param ... Arguments.
 */
RJS_INTERNAL void
rjs_message (RJS_Runtime *rt, RJS_Input *input, RJS_MessageType type,
        RJS_Location *loc, const char *fmt, ...);

/**
 * Output the input message with va_list argument.
 * \param rt The current runtime.
 * \param input The input.
 * \param type The message type.
 * \param loc The location where generate the message.
 * \param fmt The output format pattern.
 * \param ap The variable argument list.
 */
RJS_INTERNAL void
rjs_message_v (RJS_Runtime *rt, RJS_Input *input, RJS_MessageType type,
        RJS_Location *loc, const char *fmt, va_list ap);

/**
 * Output the note message.
 * \param rt The current runtime.
 * \param input The input.
 * \param loc The location where generate the message.
 * \param fmt The output format pattern.
 * \param ... Arguments.
 */
RJS_INTERNAL void
rjs_note (RJS_Runtime *rt, RJS_Input *input, RJS_Location *loc,
        const char *fmt, ...);

/**
 * Output the warning message.
 * \param rt The current runtime.
 * \param input The input.
 * \param loc The location where generate the message.
 * \param fmt The output format pattern.
 * \param ... Arguments.
 */
RJS_INTERNAL void
rjs_warning (RJS_Runtime *rt, RJS_Input *input, RJS_Location *loc,
        const char *fmt, ...);

/**
 * Output the error message.
 * \param rt The current runtime.
 * \param input The input.
 * \param loc The location where generate the message.
 * \param fmt The output format pattern.
 * \param ... Arguments.
 */
RJS_INTERNAL void
rjs_error (RJS_Runtime *rt, RJS_Input *input, RJS_Location *loc,
        const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif

