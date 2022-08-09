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

#include "ratjs_internal.h"

/*Initialize an input.*/
static void
input_init (RJS_Runtime *rt, RJS_Input *input)
{
    input->flags  = 0;
    input->line   = 1;
    input->column = 0;
    input->name   = NULL;
    input->pos    = 0;
    input->length = 0;
    input->str    = rjs_value_stack_push(rt);
}

/*Release an input.*/
static void
input_deinit (RJS_Runtime *rt, RJS_Input *input)
{
    if (input->name)
        rjs_char_star_free(rt, input->name);

    rjs_value_stack_restore_pointer(rt, input->str);
}

/*Get the name of the string input.*/
static RJS_Result
string_input_get_name (RJS_Runtime *rt, RJS_Input *input, RJS_Value *str)
{
    size_t           len = rjs_string_get_length(rt, str);
    const RJS_UChar *uc  = rjs_string_get_uchars(rt, str);
    const RJS_UChar *euc = uc + len;
    RJS_CharBuffer   cb;

    rjs_char_buffer_init(rt, &cb);

    while (uc < euc) {
        switch (*uc) {
        case '\n':
            rjs_char_buffer_append_chars(rt, &cb, "\\n", 2);
            break;
        case '\r':
            rjs_char_buffer_append_chars(rt, &cb, "\\r", 2);
            break;
        case '\t':
            rjs_char_buffer_append_chars(rt, &cb, "\\t", 2);
            break;
        case '\v':
            rjs_char_buffer_append_chars(rt, &cb, "\\v", 2);
            break;
        case '\f':
            rjs_char_buffer_append_chars(rt, &cb, "\\f", 2);
            break;
        case '\a':
            rjs_char_buffer_append_chars(rt, &cb, "\\a", 2);
            break;
        case '\b':
            rjs_char_buffer_append_chars(rt, &cb, "\\b", 2);
            break;
        case '\"':
            rjs_char_buffer_append_chars(rt, &cb, "\\\"", 2);
            break;
        case '\\':
            rjs_char_buffer_append_chars(rt, &cb, "\\\\", 2);
            break;
        default:
            if ((*uc >= 0x20) && (*uc <= 0x7e)) {
                rjs_char_buffer_append_char(rt, &cb, *uc);
            } else if (*uc <= 0xff) {
                int c1, c2;

                c1 = rjs_number_to_hex_char_l(*uc >> 4);
                c2 = rjs_number_to_hex_char_l(*uc & 0xf);

                rjs_char_buffer_append_chars(rt, &cb, "\\x", 2);
                rjs_char_buffer_append_char(rt, &cb, c1);
                rjs_char_buffer_append_char(rt, &cb, c2);
            } else {
                int c1, c2, c3, c4;

                c1 = rjs_number_to_hex_char_l(*uc >> 12);
                c2 = rjs_number_to_hex_char_l((*uc >> 8) & 0xf);
                c3 = rjs_number_to_hex_char_l((*uc >> 4) & 0xf);
                c4 = rjs_number_to_hex_char_l(*uc & 0xf);

                rjs_char_buffer_append_chars(rt, &cb, "\\u", 2);
                rjs_char_buffer_append_char(rt, &cb, c1);
                rjs_char_buffer_append_char(rt, &cb, c2);
                rjs_char_buffer_append_char(rt, &cb, c3);
                rjs_char_buffer_append_char(rt, &cb, c4);
            }
            break;
        }

        uc ++;

        if (cb.item_num >= 32) {
            rjs_char_buffer_append_chars(rt, &cb, "...", 3);
            break;
        }
    }

    input->name = rjs_char_star_dup(rt, rjs_char_buffer_to_c_string(rt, &cb));

    rjs_char_buffer_deinit(rt, &cb);
    return RJS_OK;
}

/**
 * Initialize a string input.
 * \param rt The current runtime.
 * \param si The string input to be initialized.
 * \param s The string.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_string_input_init (RJS_Runtime *rt, RJS_Input *si, RJS_Value *s)
{
    input_init(rt, si);

    rjs_value_copy(rt, si->str, s);

    si->length = rjs_string_get_length(rt, s);

    return RJS_OK;
}

/*Get the filename.*/
static RJS_Result
file_input_get_name (RJS_Runtime *rt, RJS_Input *input, const char *fn)
{
    char  path[PATH_MAX];
    char *base;

    snprintf(path, sizeof(path), "%s", fn);
    base = basename(path);

    input->name = rjs_char_star_dup(rt, base);

    return RJS_OK;
}

/**
 * Initialize a file input.
 * \param rt The current runtime.
 * \param fi The file input to be inintialized.
 * \param filename The filename.
 * \param enc The file's character encoding.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_file_input_init (RJS_Runtime *rt, RJS_Input *fi, const char *filename,
        const char *enc)
{
    RJS_Result r;

    input_init(rt, fi);

    if ((r = rjs_string_from_file(rt, fi->str, filename, enc)) == RJS_ERR) {
        input_deinit(rt, fi);
        return RJS_ERR;
    }

    fi->length = rjs_string_get_length(rt, fi->str);

    file_input_get_name(rt, fi, filename);

    return RJS_OK;
}

/**
 * Release an unused input.
 * \param rt The current runtime.
 * \param input The input to be released.
 */
void
rjs_input_deinit (RJS_Runtime *rt, RJS_Input *input)
{
    input_deinit(rt, input);
}

/**
 * Get an unicode from the input.
 * \param rt The current runtime.
 * \param input The input.
 * \return The next unicode.
 * \retval RJS_INPUT_END The input is end.
 */
int
rjs_input_get_uc (RJS_Runtime *rt, RJS_Input *input)
{
    int c;

    if (input->flags & RJS_INPUT_FL_NEW_LINE) {
        input->flags &= ~RJS_INPUT_FL_NEW_LINE;
        input->line ++;
        input->column = 0;
    }

    if (input->pos >= input->length) {
        c = RJS_INPUT_END;
    } else {
        const RJS_UChar *chars = rjs_string_get_uchars(rt, input->str);

        c = chars[input->pos ++];

        if (rjs_uchar_is_leading_surrogate(c)
                && (input->pos < input->length)
                && rjs_uchar_is_trailing_surrogate(chars[input->pos])) {
            int c1 = chars[input->pos ++];

            c = rjs_surrogate_pair_to_uc(c, c1);
        } else if ((input->flags & RJS_INPUT_FL_CRLF_TO_LF) && (c == 0x0d)) {
            if (input->pos < input->length) {
                int c1 = chars[input->pos];

                if (c1 == 0x0a)
                    input->pos ++;
            }

            c = 0x0a;
        }
    }

    if (c >= 0) {
        input->column ++;

        if (rjs_uchar_is_line_terminator(c)) {
            input->flags |= RJS_INPUT_FL_NEW_LINE;
        }
    }

    return c;
}

/**
 * Push back an unicode to the input.
 * \param rt The current runtime.
 * \param input The input.
 * \param c The unicode to be pushed back.
 */
void
rjs_input_unget_uc (RJS_Runtime *rt, RJS_Input *input, int c)
{
    if (c >= 0) {
        const RJS_UChar *chars;

        input->flags &= ~ RJS_INPUT_FL_NEW_LINE;
        input->column --;

        chars = rjs_string_get_uchars(rt, input->str);

        if (input->pos >= 2) {
            if (rjs_uchar_is_trailing_surrogate(chars[input->pos - 1])
                    && rjs_uchar_is_leading_surrogate(chars[input->pos - 2])) {
                input->pos -= 2;
            } else if ((input->flags & RJS_INPUT_FL_CRLF_TO_LF)
                    && (chars[input->pos - 1] == 0x0a)
                    && (chars[input->pos - 2] == 0x0d)) {
                input->pos -= 2;
            } else {
                input->pos --;
            }
        } else {
            assert(input->pos > 0);

            input->pos --;
        }
    }
}

/**
 * Output the input message's head.
 * \param rt The current runtime.
 * \param input The input.
 * \param type The message type.
 * \param loc The location where generate the message.
 */
void
rjs_message_head (RJS_Runtime *rt, RJS_Input *input,
        RJS_MessageType type, RJS_Location *loc)
{
    const char *tstr;
    const char *col;

    if (!input->name)
        string_input_get_name(rt, input, input->str);

    switch (type) {
    case RJS_MESSAGE_NOTE:
        tstr = "note";
        col  = "\033[36;1m";
        break;
    case RJS_MESSAGE_WARNING:
        tstr = "warning";
        col  = "\033[35;1m";
        break;
    case RJS_MESSAGE_ERROR:
    default:
        tstr = "error";
        col  = "\033[31;1m";
        break;
    }

#if ENABLE_COLOR_CONSOLE
    if (!isatty(2))
#endif /*ENABLE_COLOR_CONSOLE*/
        col = NULL;

    fprintf(stderr, "\"%s\": ", input->name);

    if (loc) {
        if (loc->first_line == loc->last_line) {
            if (loc->first_column == loc->last_column)
                fprintf(stderr, "%d.%d", loc->first_line, loc->first_column);
            else
                fprintf(stderr, "%d.%d-%d", loc->first_line, loc->first_column,
                        loc->last_column);
        } else {
            fprintf(stderr, "%d.%d-%d.%d",
                    loc->first_line, loc->first_column,
                    loc->last_line, loc->last_column);
        }

        fprintf(stderr, ": ");
    }

    fprintf(stderr, "%s%s%s: ",
            col ? col : "",
            tstr,
            col ? "\033[0m" : "");
}

/**
 * Output the input message.
 * \param rt The current runtime.
 * \param input The input.
 * \param type The message type.
 * \param loc The location where generate the message.
 * \param fmt The output format pattern.
 * \param ... Arguments.
 */
void
rjs_message (RJS_Runtime *rt, RJS_Input *input, RJS_MessageType type,
        RJS_Location *loc, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    rjs_message_v(rt, input, type, loc, fmt, ap);
    va_end(ap);
}

/**
 * Output the input message with va_list argument.
 * \param rt The current runtime.
 * \param input The input.
 * \param type The message type.
 * \param loc The location where generate the message.
 * \param fmt The output format pattern.
 * \param ap The variable argument list.
 */
void
rjs_message_v (RJS_Runtime *rt, RJS_Input *input, RJS_MessageType type,
        RJS_Location *loc, const char *fmt, va_list ap)
{
    rjs_message_head(rt, input, type, loc);

    vfprintf(stderr, fmt, ap);

    fprintf(stderr, "\n");
}

/**
 * Output the note message.
 * \param rt The current runtime.
 * \param input The input.
 * \param loc The location where generate the message.
 * \param fmt The output format pattern.
 * \param ... Arguments.
 */
void
rjs_note (RJS_Runtime *rt, RJS_Input *input, RJS_Location *loc,
        const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    rjs_message_v(rt, input, RJS_MESSAGE_NOTE, loc, fmt, ap);
    va_end(ap);
}

/**
 * Output the warning message.
 * \param rt The current runtime.
 * \param input The input.
 * \param loc The location where generate the message.
 * \param fmt The output format pattern.
 * \param ... Arguments.
 */
void
rjs_warning (RJS_Runtime *rt, RJS_Input *input, RJS_Location *loc,
        const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    rjs_message_v(rt, input, RJS_MESSAGE_WARNING, loc, fmt, ap);
    va_end(ap);
}

/**
 * Output the error message.
 * \param rt The current runtime.
 * \param input The input.
 * \param loc The location where generate the message.
 * \param fmt The output format pattern.
 * \param ... Arguments.
 */
void
rjs_error (RJS_Runtime *rt, RJS_Input *input, RJS_Location *loc,
        const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    rjs_message_v(rt, input, RJS_MESSAGE_ERROR, loc, fmt, ap);
    va_end(ap);
}
