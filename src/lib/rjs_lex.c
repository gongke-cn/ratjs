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

/**Lexical table's entry.*/
typedef struct {
    int c;     /**< The character.*/
    int next;  /**< The next entry's index.*/
    int child; /**< The first entry's index.*/
    int token; /**< The token type.*/
} RJS_LexCharEntry;

#include <rjs_lex_table_inc.c>

/*Get an unicode from the input.*/
static int
lex_get_uc (RJS_Runtime *rt, RJS_Lex *lex)
{
    return rjs_input_get_uc(rt, lex->input);
}

/*Push back an unicode to the input.*/
static void
lex_unget_uc (RJS_Runtime *rt, RJS_Lex *lex, int c)
{
    rjs_input_unget_uc(rt, lex->input, c);
}

/*Output error message.*/
static void
lex_error (RJS_Runtime *rt, RJS_Lex *lex, RJS_Location *loc, const char *fmt, ...)
{
    lex->status |= RJS_LEX_ST_ERROR;

    if (!(lex->status & RJS_LEX_ST_NO_MSG)) {
        va_list ap;

        va_start(ap, fmt);
        rjs_message_v(rt, lex->input, RJS_MESSAGE_ERROR, loc, fmt, ap);
        va_end(ap);
    }
}

/*Output warning message.*/
static void
lex_warning (RJS_Runtime *rt, RJS_Lex *lex, RJS_Location *loc, const char *fmt, ...)
{
    if (!(lex->status & RJS_LEX_ST_NO_MSG)) {
        va_list ap;

        va_start(ap, fmt);
        rjs_message_v(rt, lex->input, RJS_MESSAGE_WARNING, loc, fmt, ap);
        va_end(ap);
    }
}

/*Append an unicode to unicode text and raw unicode text buffer.*/
static void
lex_append_uc (RJS_Runtime *rt, RJS_Lex *lex, int c)
{
    rjs_uchar_buffer_append_uc(rt, &lex->uc_text, c);
    rjs_uchar_buffer_append_uc(rt, &lex->raw_uc_text, c);
}

/**
 * Initialize a lexical analyzer.
 * \param rt The current runtime.
 * \param lex The lexical analyzer to be initialized.
 * \param input The characters input to be used.
 */
void
rjs_lex_init (RJS_Runtime *rt, RJS_Lex *lex, RJS_Input *input)
{
    lex->input       = input;
    lex->status      = RJS_LEX_ST_FIRST_TOKEN;
    lex->flags       = 0;
    lex->brace_level = 0;

    rjs_char_buffer_init(rt, &lex->c_text);
    rjs_uchar_buffer_init(rt, &lex->uc_text);
    rjs_uchar_buffer_init(rt, &lex->raw_uc_text);
    rjs_vector_init(&lex->template_brace_level);
}

/**
 * Release an unused lexical analyzer.
 * \param rt The current runtime.
 * \param lex The lexical analyzer to be released.
 */
void
rjs_lex_deinit (RJS_Runtime *rt, RJS_Lex *lex)
{
    rjs_char_buffer_deinit(rt, &lex->c_text);
    rjs_uchar_buffer_deinit(rt, &lex->uc_text);
    rjs_uchar_buffer_deinit(rt, &lex->raw_uc_text);
    rjs_vector_deinit(&lex->template_brace_level, rt);
}

/*Read single line comment.*/
static void
lex_single_line_comment (RJS_Runtime *rt, RJS_Lex *lex)
{
    int c;

    lex->input->flags |= RJS_INPUT_FL_NO_MSG;

    while (1) {
        c = lex_get_uc(rt, lex);
        if ((c < 0) || rjs_uchar_is_line_terminator(c))
            break;
    }

    lex->input->flags &= ~RJS_INPUT_FL_NO_MSG;
}

/*Read multi line comment.*/
static void
lex_multi_line_comment (RJS_Runtime *rt, RJS_Lex *lex)
{
    int c;

    lex->input->flags |= RJS_INPUT_FL_NO_MSG;

    while (1) {
        c = lex_get_uc(rt, lex);
        if (c == '*') {
            int c1 = lex_get_uc(rt, lex);
            if (c1 == '/')
                break;

            lex_unget_uc(rt, lex, c1);
        } else if (c == RJS_INPUT_END) {
            RJS_Location loc;

            rjs_input_get_location(lex->input, &loc);
            lex_error(rt, lex, &loc, _("expect `*/\' at the end of multi line comment"));
            break;
        } else if (c < 0) {
            break;
        }
    }

    lex->input->flags &= ~RJS_INPUT_FL_NO_MSG;
}

/*Octal escape character.*/
static int
lex_octal_escape (RJS_Runtime *rt, RJS_Lex *lex)
{
    int c1, c2, c3;

    c1 = lex_get_uc(rt, lex);
    c2 = lex_get_uc(rt, lex);
    c3 = lex_get_uc(rt, lex);

    if ((c1 >= '0') && (c1 <= '3')
            && rjs_uchar_is_octal(c2)
            && rjs_uchar_is_octal(c3)) {
        return ((c1 - '0') << 6) | ((c2 - '0') << 3) | (c3 - '0');
    } else if (rjs_uchar_is_octal(c1) && rjs_uchar_is_octal(c2)) {
        lex_unget_uc(rt, lex, c3);

        return ((c1 - '0') << 3) | (c2 - '0');
    } else {
        lex_unget_uc(rt, lex, c3);
        lex_unget_uc(rt, lex, c2);

        return c1 - '0';
    }
}

/*Check a hexadecimal character.*/
static int
lex_hex_char (RJS_Runtime *rt, RJS_Lex *lex, int c, RJS_Bool is_templ)
{
    if (!rjs_uchar_is_xdigit(c)) {
        RJS_Location loc;

        lex_unget_uc(rt, lex, c);
        rjs_input_get_location(lex->input, &loc);

        if (is_templ)
            lex_warning(rt, lex, &loc, _("illegal hexadecimal escape character"));
        else
            lex_error(rt, lex, &loc, _("illegal hexadecimal escape character"));
        return -1;
    }

    return rjs_hex_char_to_number(c);
}

/*Hexadecimal escape character.*/
static int
lex_hex_escape (RJS_Runtime *rt, RJS_Lex *lex, RJS_Bool is_templ)
{
    int c, n;
    int v;

    c = lex_get_uc(rt, lex);
    if ((n = lex_hex_char(rt, lex, c, is_templ)) < 0)
        return -1;
    lex_append_uc(rt, lex, c);

    v = n;

    c = lex_get_uc(rt, lex);
    if ((n = lex_hex_char(rt, lex, c, is_templ)) < 0)
        return -1;
    lex_append_uc(rt, lex, c);

    v <<= 4;
    v |= n;

    return v;
}

/*Unicode escape character.*/
static int
lex_unicode_escape (RJS_Runtime *rt, RJS_Lex *lex, RJS_Bool is_templ)
{
    int c;
    int i, n, v = 0;
    RJS_Location loc;

    c = lex_get_uc(rt, lex);
    if (c == '{') {
        lex_append_uc(rt, lex, c);

        while (1) {
            c = lex_get_uc(rt, lex);
            if (c == RJS_INPUT_END) {
                rjs_input_get_location(lex->input, &loc);
                if (is_templ)
                    lex_warning(rt, lex, &loc, _("expect `}\' at end of unicode escapce sequence `\\u{\'"));
                else
                    lex_error(rt, lex, &loc, _("expect `}\' at end of unicode escapce sequence `\\u{\'"));
                break;
            }

            if (c == '}') {
                lex_append_uc(rt, lex, c);
                break;
            }

            if ((n = lex_hex_char(rt, lex, c, is_templ)) < 0)
                return -1;
            lex_append_uc(rt, lex, c);

            v <<= 4;
            v |= n;

            if (v > 0x10ffff) {
                rjs_input_get_location(lex->input, &loc);
                if (is_templ)
                    lex_warning(rt, lex, &loc, _("code point must <= 0x10ffff"));
                else
                    lex_error(rt, lex, &loc, _("code point must <= 0x10ffff"));
                    
                v = -1;
            }
        }
    } else {
        lex_unget_uc(rt, lex, c);

        for (i = 0; i < 4; i ++) {
            c = lex_get_uc(rt, lex);
            if ((n = lex_hex_char(rt, lex, c, is_templ)) < 0)
                return -1;
            lex_append_uc(rt, lex, c);

            v <<= 4;
            v |= n;
        }
    }

    return v;
}

/*String.*/
static void
lex_string (RJS_Runtime *rt, RJS_Lex *lex, RJS_Token *token, int start)
{
    int          c;
    RJS_UChar   *puc, *puc_end;
    RJS_Location loc;

    token->flags = 0;

    rjs_uchar_buffer_clear(rt, &lex->uc_text);
    rjs_uchar_buffer_clear(rt, &lex->raw_uc_text);

    while (1) {
        c = lex_get_uc(rt, lex);
        if (c == start)
            break;

        if (c == RJS_INPUT_END) {
            rjs_input_get_location(lex->input, &loc);
            lex_error(rt, lex, &loc, _("expect `%c' at the end of string"), start);
            break;
        }

        if ((c != 0x2028) && (c != 0x2029) && rjs_uchar_is_line_terminator(c)) {
            rjs_input_get_location(lex->input, &loc);
            lex_error(rt, lex, &loc, _("unexpected line terminator in the string"));
            break;
        }

        if (c == '\\') {
            int c1 = lex_get_uc(rt, lex);
            int pos;

            token->flags |= RJS_TOKEN_FL_ESCAPE;

            if (c1 == RJS_INPUT_END) {
                rjs_input_get_location(lex->input, &loc);
                lex_error(rt, lex, &loc, _("expect `%c' at the end of string"), start);
                break;
            }

            switch (c1) {
            case 'b':
                rjs_uchar_buffer_append_uc(rt, &lex->uc_text, '\b');
                break;
            case 'f':
                rjs_uchar_buffer_append_uc(rt, &lex->uc_text, '\f');
                break;
            case 'n':
                rjs_uchar_buffer_append_uc(rt, &lex->uc_text, '\n');
                break;
            case 'r':
                rjs_uchar_buffer_append_uc(rt, &lex->uc_text, '\r');
                break;
            case 't':
                rjs_uchar_buffer_append_uc(rt, &lex->uc_text, '\t');
                break;
            case 'v':
                rjs_uchar_buffer_append_uc(rt, &lex->uc_text, '\v');
                break;
            case 'x':
                pos = lex->uc_text.item_num;
                c = lex_hex_escape(rt, lex, RJS_FALSE);
                lex->uc_text.item_num = pos;
                if (c >= 0) {
                    rjs_uchar_buffer_append_uc(rt, &lex->uc_text, c);
                } else {
                    rjs_input_get_location(lex->input, &loc);
                    lex_error(rt, lex, &loc, _("illegal hexadecimal escape character"));
                }
                break;
            case 'u':
                pos = lex->uc_text.item_num;
                c = lex_unicode_escape(rt, lex, RJS_FALSE);
                lex->uc_text.item_num = pos;
                if (c >= 0) {
                    rjs_uchar_buffer_append_uc(rt, &lex->uc_text, c);
                } else {
                    rjs_input_get_location(lex->input, &loc);
                    lex_error(rt, lex, &loc, _("illegal unicode escape character"));
                }
                break;
            default:
                if (rjs_uchar_is_digit(c1)) {
                    if (c1 == '0') {
                        int c2 = lex_get_uc(rt, lex);

                        lex_unget_uc(rt, lex, c2);

                        if (!rjs_uchar_is_digit(c2)) {
                            rjs_uchar_buffer_append_uc(rt, &lex->uc_text, 0);
                            break;
                        }
                    }

                    token->flags |= RJS_TOKEN_FL_LEGACY_ESCAPE;
                    
                    if ((c1 == '8') || (c1 == '9')) {
                        rjs_uchar_buffer_append_uc(rt, &lex->uc_text, c1);
                        break;
                    }

                    lex_unget_uc(rt, lex, c1);

                    c = lex_octal_escape(rt, lex);
                    if (c >= 0)
                        rjs_uchar_buffer_append_uc(rt, &lex->uc_text, c);
                } else if (!rjs_uchar_is_line_terminator(c1)) {
                    rjs_uchar_buffer_append_uc(rt, &lex->uc_text, c1);
                }
                break;
            }
        } else {
            rjs_uchar_buffer_append_uc(rt, &lex->uc_text, c);
        }
    }

    /*Chekc if the string is well formed.*/
    puc     = lex->uc_text.items;
    puc_end = puc + lex->uc_text.item_num;
    while (puc < puc_end) {
        if (rjs_uchar_is_trailing_surrogate(*puc)) {
            token->flags |= RJS_TOKEN_FL_UNPAIRED_SURROGATE;
            break;
        } else if (rjs_uchar_is_leading_surrogate(*puc)) {
            if (puc + 1 < puc_end) {
                if (!rjs_uchar_is_trailing_surrogate(puc[1])) {
                    token->flags |= RJS_TOKEN_FL_UNPAIRED_SURROGATE;
                    break;
                }

                puc += 2;
            } else {
                token->flags |= RJS_TOKEN_FL_UNPAIRED_SURROGATE;
                break;
            }
        } else {
            puc ++;
        }
    }

    token->type = RJS_TOKEN_STRING;

    rjs_string_from_uchars(rt, token->value,
            lex->uc_text.items, lex->uc_text.item_num);
}

/*Output the last character is separator.*/
static void
last_char_sep_error (RJS_Runtime *rt, RJS_Lex *lex)
{
    RJS_Location loc;

    rjs_input_get_location(lex->input, &loc);
    lex_error(rt, lex, &loc, _("the separator cannot be the last character of number"));
}

/*Number.*/
static void
lex_number (RJS_Runtime *rt, RJS_Lex *lex, RJS_Token *token)
{
    int          c;
    int          dn;
    int          base       = 10;
    RJS_Bool     can_sep    = RJS_TRUE;
    RJS_Bool     no_sep     = RJS_FALSE;
    RJS_Bool     legacy_oct = RJS_FALSE;
    RJS_Bool     is_float   = RJS_FALSE;
#if ENABLE_BIG_INT
    RJS_Bool     is_big     = RJS_FALSE;
#endif /*ENABLE_BIG_INT*/
    const char  *cstr;
    RJS_Location loc;

    if (lex->status & RJS_LEX_ST_NO_SEP)
        no_sep = RJS_TRUE;

    token->type = RJS_TOKEN_NUMBER;
    rjs_value_set_number(rt, token->value, 0);

    rjs_char_buffer_clear(rt, &lex->c_text);

    c = lex_get_uc(rt, lex);
    if (c == '.')
        goto int_end;

    if (c == '0') {
        c = lex_get_uc(rt, lex);

        switch (c) {
        case 'b':
        case 'B':
            base = 2;
            break;
        case 'o':
        case 'O':
            base = 8;
            break;
        case 'x':
        case 'X':
            base = 16;
            break;
        default:
            if (!(lex->status & RJS_LEX_ST_NO_LEGACY_OCT)
                    && rjs_uchar_is_octal(c)) {
                lex_unget_uc(rt, lex, c);
                base       = 8;
                legacy_oct = RJS_TRUE;
                no_sep     = RJS_TRUE;
            } else if (c == '_') {
                last_char_sep_error(rt, lex);
                return;
            } else {
                if ((c == '8') || (c == '9')) {
                    no_sep     = RJS_TRUE;
                    legacy_oct = RJS_TRUE;
                }

                lex_unget_uc(rt, lex, c);
                lex_unget_uc(rt, lex, '0');
            }
            break;
        }
    } else {
        lex_unget_uc(rt, lex, c);
    }

    can_sep = RJS_FALSE;
    dn      = 0;
    while (1) {
        c = lex_get_uc(rt, lex);

        if (!no_sep && (c == '_') && can_sep) {
            can_sep = RJS_FALSE;
            continue;
        }

        switch (base) {
        case 2:
            if ((c != '0') && (c != '1')) {
                if (!dn) {
                    rjs_input_get_location(lex->input, &loc);
                    lex_error(rt, lex, &loc, _("expect `0\' or `1\' in a binary integer"));
                    lex_unget_uc(rt, lex, c);
                    return;
                } else if (!no_sep && !can_sep) {
                    last_char_sep_error(rt, lex);
                    return;
                } else {
                    goto int_end;
                }
            }
            break;
        case 8:
            if (!rjs_uchar_is_octal(c)) {
                if (legacy_oct && (rjs_uchar_is_digit(c) || (c == '.') || (c == 'e') || (c == 'E'))) {
                    base = 10;
                } else if (!dn) {
                    rjs_input_get_location(lex->input, &loc);
                    lex_error(rt, lex, &loc, _("expect `0\' ~ `7\' in an octal integer"));
                    lex_unget_uc(rt, lex, c);
                    return;
                } else if (!no_sep && !can_sep) {
                    last_char_sep_error(rt, lex);
                    return;
                } else {
                    goto int_end;
                }
            }
            break;
        case 10:
            if (!rjs_uchar_is_digit(c)) {
                if (!dn) {
                    rjs_input_get_location(lex->input, &loc);
                    lex_error(rt, lex, &loc, _("expect a decimal character in a decimal integer"));
                    lex_unget_uc(rt, lex, c);
                    return;
                } else if (!no_sep && !can_sep) {
                    last_char_sep_error(rt, lex);
                    return;
                } else {
                    goto int_end;
                }
            }
            break;
        case 16:
            if (!rjs_uchar_is_xdigit(c)) {
                if (!dn) {
                    rjs_input_get_location(lex->input, &loc);
                    lex_error(rt, lex, &loc, _("expect a hexadecimal character in a hexadecimal integer"));
                    lex_unget_uc(rt, lex, c);
                    return;
                } else if (!no_sep && !can_sep) {
                    last_char_sep_error(rt, lex);
                    return;
                } else {
                    goto int_end;
                }
            }
            break;
        }

        rjs_char_buffer_append_char(rt, &lex->c_text, c);
        can_sep = RJS_TRUE;
        dn ++;
    }

int_end:

    if ((base == 10) && !(lex->flags & RJS_LEX_FL_BIG_INT)) {
        if (c == '.') {
            dn       = 0;
            is_float = RJS_TRUE;
            can_sep  = RJS_FALSE;

            rjs_char_buffer_append_char(rt, &lex->c_text, c);

            while (1) {
                c = lex_get_uc(rt, lex);
                if (!no_sep && can_sep && (c == '_')) {
                    can_sep = RJS_FALSE;
                    continue;
                }

                if (!rjs_uchar_is_digit(c)) {
                    if (!no_sep && dn && !can_sep) {
                        last_char_sep_error(rt, lex);
                        return;
                    }
                    break;
                }

                rjs_char_buffer_append_char(rt, &lex->c_text, c);
                can_sep = RJS_TRUE;
                dn ++;
            }
        }

        if ((c == 'e') || (c == 'E')) {
            dn       = 0;
            is_float = RJS_TRUE;
            can_sep  = RJS_FALSE;

            rjs_char_buffer_append_char(rt, &lex->c_text, c);

            c = lex_get_uc(rt, lex);
            if ((c == '+') || (c == '-')) {
                rjs_char_buffer_append_char(rt, &lex->c_text, c);
            } else {
                lex_unget_uc(rt, lex, c);
            }

            while (1) {
                c = lex_get_uc(rt, lex);
                if (!no_sep && can_sep && (c == '_')) {
                    can_sep = RJS_FALSE;
                    continue;
                }

                if (!rjs_uchar_is_digit(c)) {
                    if (!dn) {
                        rjs_input_get_location(lex->input, &loc);
                        lex_error(rt, lex, &loc, _("expect a decimal character here"));
                        lex_unget_uc(rt, lex, c);
                        return;
                    } else if (!no_sep && !can_sep) {
                        last_char_sep_error(rt, lex);
                        return;
                    }
                    break;
                }

                rjs_char_buffer_append_char(rt, &lex->c_text, c);
                can_sep = RJS_TRUE;
                dn ++;
            }
        }
    }

#if ENABLE_BIG_INT
    if ((c == 'n')
            && !(lex->flags & RJS_LEX_FL_BIG_INT)
            && !legacy_oct
            && !is_float) {
        is_big = RJS_TRUE;

        c = lex_get_uc(rt, lex);
    }
#endif /*ENABLE_BIG_INT*/

    if (rjs_uchar_is_digit(c) || rjs_uchar_is_id_start(c)) {
        rjs_input_get_location(lex->input, &loc);
        lex_error(rt, lex, &loc,
                _("number cannot be followed by an identifier start or a digit character"));
    }

    lex_unget_uc(rt, lex, c);

    if (no_sep) {
        RJS_Parser *parser = rt->parser;

        if (parser && (parser->flags & RJS_PARSE_FL_STRICT)) {
            rjs_input_get_position(lex->input,
                    &token->location.last_line,
                    &token->location.last_column,
                    &token->location.last_pos);
            lex_error(rt, lex, &token->location, _("legacy number cannot be used in strict mode"));
        }
    }

#if ENABLE_BIG_INT
    if (lex->flags & RJS_LEX_FL_BIG_INT)
        is_big = RJS_TRUE;
#endif /*ENABLE_BIG_INT*/

    cstr = rjs_char_buffer_to_c_string(rt, &lex->c_text);

    if (base == 10)
        token->flags |= RJS_TOKEN_FL_DECIMAL;

#if ENABLE_BIG_INT
    if (is_big) {
        rjs_big_int_from_chars(rt, token->value, cstr, base);
    } else
#endif /*ENABLE_BIG_INT*/
    if (is_float) {
        double d;

        d = rjs_strtod(cstr, NULL);

        rjs_value_set_number(rt, token->value, d);
    } else {
        double d;

        d = rjs_strtoi(cstr, NULL, base);

        rjs_value_set_number(rt, token->value, d);
    }
}

#if ENABLE_PRIV_NAME

/*Private identifier.*/
static void
lex_private_identifier (RJS_Runtime *rt, RJS_Lex *lex, RJS_Token *token)
{
    int c, c1, pos;

    rjs_uchar_buffer_clear(rt, &lex->uc_text);

    lex_append_uc(rt, lex, '#');

    pos = lex->uc_text.item_num;

    c = lex_get_uc(rt, lex);
    if (c == '\\') {
        c1 = lex_get_uc(rt, lex);
        if (c1 != 'u')
            goto error;
        if ((c = lex_unicode_escape(rt, lex, RJS_FALSE)) < 0)
            return;
    }

    if (!rjs_uchar_is_id_start(c))
        goto error;

    lex->uc_text.item_num = pos;
    rjs_uchar_buffer_append_uc(rt, &lex->uc_text, c);

    while (1) {
        pos = lex->uc_text.item_num;

        c = lex_get_uc(rt, lex);
        if (c == '\\') {
            c1 = lex_get_uc(rt, lex);
            if (c1 == 'u') {
                if ((c = lex_unicode_escape(rt, lex, RJS_FALSE)) < 0)
                    break;
            } else {
                lex_unget_uc(rt, lex, c1);
                lex_unget_uc(rt, lex, '\\');
                break;
            }

            if (!rjs_uchar_is_id_continue(c))
                goto error;
        } else if (!rjs_uchar_is_id_continue(c)) {
            lex_unget_uc(rt, lex, c);
            break;
        }

        lex->uc_text.item_num = pos;
        rjs_uchar_buffer_append_uc(rt, &lex->uc_text, c);
    }

    token->type = RJS_TOKEN_PRIVATE_IDENTIFIER;

    rjs_string_from_uchars(rt, token->value,
            lex->uc_text.items, lex->uc_text.item_num);

    return;
error:
    rjs_input_get_position(lex->input,
            &token->location.last_line,
            &token->location.last_column,
            &token->location.last_pos);
    lex_error(rt, lex, &token->location, _("illegal private identifier"));
    return;
}

#endif /*ENABLE_PRIV_NAME*/

/*Identifier.*/
static void
lex_identifier (RJS_Runtime *rt, RJS_Lex *lex, RJS_Token *token)
{
    const RJS_LexCharEntry *e;
    int                     c, c1, cp, eid, pos;
    RJS_Bool                escape = RJS_FALSE;

    rjs_uchar_buffer_clear(rt, &lex->uc_text);

    pos = lex->uc_text.item_num;

    c = lex_get_uc(rt, lex);
    if (c == '\\') {
        c1 = lex_get_uc(rt, lex);
        if (c1 != 'u') {
            RJS_Location loc;

            rjs_input_get_location(lex->input, &loc);
            lex_error(rt, lex, &loc, _("expect `u\' after `\\\'"));
            return;
        }

        escape = RJS_TRUE;
        if ((c = lex_unicode_escape(rt, lex, RJS_FALSE)) < 0)
            return;
    }

    if (!rjs_uchar_is_id_start(c))
        goto error;

    lex->uc_text.item_num = pos;
    rjs_uchar_buffer_append_uc(rt, &lex->uc_text, c);

    while (1) {
        pos = lex->uc_text.item_num;

        c = lex_get_uc(rt, lex);
        if (c == '\\') {
            c1 = lex_get_uc(rt, lex);
            if (c1 == 'u') {
                escape = RJS_TRUE;
                if ((c = lex_unicode_escape(rt, lex, RJS_FALSE)) < 0)
                    break;
            } else {
                lex_unget_uc(rt, lex, c1);
                lex_unget_uc(rt, lex, '\\');
                break;
            }

            if (!rjs_uchar_is_id_continue(c))
                goto error;
        } else if (!rjs_uchar_is_id_continue(c)) {
            lex_unget_uc(rt, lex, c);
            break;
        }

        lex->uc_text.item_num = pos;
        rjs_uchar_buffer_append_uc(rt, &lex->uc_text, c);
    }

    token->type = RJS_TOKEN_IDENTIFIER;

    rjs_string_from_uchars(rt, token->value,
            lex->uc_text.items, lex->uc_text.item_num);

    if (!(lex->status & RJS_LEX_ST_JSON)) {
        /*Check if the token is reserved word.*/
        cp  = 0;
        eid = 0;

        e = &identifier_lex_table[eid];

        while (1) {
            int cid;

            c   = lex->uc_text.items[cp ++];
            cid = e->child;

            while (cid != -1) {
                const RJS_LexCharEntry *child = &identifier_lex_table[cid];

                if (child->c == c)
                    break;

                cid = child->next;
            }

            if (cid == -1)
                break;

            eid = cid;
            e   = &identifier_lex_table[eid];

            if (cp == lex->uc_text.item_num) {
                if (e->token != -1)
                    token->flags = e->token;
                break;
            }
        }
    }

    if (escape)
        token->flags |= RJS_TOKEN_FL_ESCAPE;

    return;
error:
    rjs_input_get_position(lex->input,
            &token->location.last_line,
            &token->location.last_column,
            &token->location.last_pos);
    lex_error(rt, lex, &token->location, _("illegal identifier"));
    return;
}

/*Punctuator.*/
static void
lex_punctuator (RJS_Runtime *rt, RJS_Lex *lex, RJS_Token *token)
{
    int           eid = 0;
    RJS_TokenType tt = -1;
    int           uc_buf[8];
    int           uc_num = 0;

    while (1) {
        const RJS_LexCharEntry *e = &punctuator_lex_table[eid];
        int c;
        int cid;

        if (e->token != -1) {
            tt = e->token;
            uc_num = 0;
        }

        c = lex_get_uc(rt, lex);
        if (c >= 0)
            uc_buf[uc_num ++] = c;

        cid = e->child;
        while (cid != -1) {
            const RJS_LexCharEntry *child = &punctuator_lex_table[cid];

            if (child->c == c)
                break;

            cid = child->next;
        }

        if (cid == -1) {
            if (tt == -1) {
                rjs_input_get_position(lex->input,
                        &token->location.last_line,
                        &token->location.last_column,
                        &token->location.last_pos);
                lex_error(rt, lex, &token->location, _("illegal punctuator"));
            } else {
                while (uc_num) {
                    uc_num --;
                    lex_unget_uc(rt, lex, uc_buf[uc_num]);
                }

                token->type = tt;
            }

            /* "?." must look ahead not a decimal digit.*/
            if (token->type == RJS_TOKEN_ques_dot) {
                c = lex_get_uc(rt, lex);
                lex_unget_uc(rt, lex, c);
                if (rjs_uchar_is_digit(c)) {
                    lex_unget_uc(rt, lex, '.');
                    token->type = RJS_TOKEN_ques;
                }
            }

            break;
        } else {
            eid = cid;
        }
    }
}

/*Regular expression.*/
static void
lex_regexp (RJS_Runtime *rt, RJS_Lex *lex, RJS_Token *token)
{
    RJS_Location loc;
    int          c;
    size_t       flag_pos;
    RJS_Result   r;
    size_t       top   = rjs_value_stack_save(rt);
    RJS_Value   *src   = rjs_value_stack_push(rt);
    RJS_Value   *flags = rjs_value_stack_push(rt);

    rjs_uchar_buffer_clear(rt, &lex->uc_text);

    /*Regular expression.*/
    while (1) {
        c = lex_get_uc(rt, lex);
        if ((c == RJS_INPUT_END) || rjs_uchar_is_line_terminator(c)) {
            rjs_input_get_location(lex->input, &loc);
            lex_error(rt, lex, &loc, _("expect `/\' at end of the regular expression"));
            break;
        }
        if (c == '/')
            break;

        if (c == '\\') {
            c = lex_get_uc(rt, lex);
            if ((c == RJS_INPUT_END) || rjs_uchar_is_line_terminator(c)) {
                rjs_input_get_location(lex->input, &loc);
                lex_error(rt, lex, &loc, _("expect `/\' at end of the regular expression"));
                break;
            }

            rjs_uchar_buffer_append_uc(rt, &lex->uc_text, '\\');
            rjs_uchar_buffer_append_uc(rt, &lex->uc_text, c);
        } else if (c == '[') {
            rjs_uchar_buffer_append_uc(rt, &lex->uc_text, c);

            while (1) {
                c = lex_get_uc(rt, lex);
                if ((c == RJS_INPUT_END) || rjs_uchar_is_line_terminator(c)) {
                    rjs_input_get_location(lex->input, &loc);
                    lex_error(rt, lex, &loc, _("expect `/\' at end of the regular expression"));
                    break;
                }

                rjs_uchar_buffer_append_uc(rt, &lex->uc_text, c);

                if (c == ']')
                    break;
            }
        } else {
            rjs_uchar_buffer_append_uc(rt, &lex->uc_text, c);
        }
    }

    /*Flags.*/
    flag_pos = lex->uc_text.item_num;

    if (c == '/') {
        while (1) {
            c = lex_get_uc(rt, lex);
            if (!rjs_uchar_is_id_continue(c)) {
                lex_unget_uc(rt, lex, c);
                break;
            }

            rjs_uchar_buffer_append_uc(rt, &lex->uc_text, c);
        }
    }

    token->type = RJS_TOKEN_REGEXP;

    rjs_string_from_uchars(rt, src, lex->uc_text.items, flag_pos);

    if (flag_pos == lex->uc_text.item_num)
        rjs_value_set_undefined(rt, flags);
    else
        rjs_string_from_uchars(rt, flags,
                lex->uc_text.items + flag_pos,
                lex->uc_text.item_num - flag_pos);

    rjs_input_get_position(lex->input,
            &token->location.last_line,
            &token->location.last_column,
            &token->location.last_pos);

    lex->regexp_loc = token->location;

    if ((r = rjs_regexp_new(rt, token->value, src, flags)) == RJS_ERR)
        lex_error(rt, lex, &token->location, _("illegal regular expression"));

    rjs_value_stack_restore(rt, top);
}

/*Template.*/
static void
lex_template (RJS_Runtime *rt, RJS_Lex *lex, RJS_Bool is_head, RJS_Token *token)
{
    RJS_Location loc;
    int          c;
    RJS_Bool     is_end       = RJS_FALSE;
    RJS_Bool     escape_error = RJS_FALSE;
    size_t       top          = rjs_value_stack_save(rt);
    RJS_Value   *str          = rjs_value_stack_push(rt);
    RJS_Value   *raw          = rjs_value_stack_push(rt);

    if (is_head)
        rjs_vector_append(&lex->template_brace_level, lex->brace_level, rt);

    rjs_uchar_buffer_clear(rt, &lex->uc_text);
    rjs_uchar_buffer_clear(rt, &lex->raw_uc_text);

    while (1) {
        c = lex_get_uc(rt, lex);
        if (c == RJS_INPUT_END) {
            rjs_input_get_location(lex->input, &loc);
            lex_error(rt, lex, &loc, _("expect ``\' at end of the template"));
            break;
        }

        if (c == '`') {
            is_end = RJS_TRUE;
            break;
        }

        if ((c == '$') && !(lex->status & RJS_LEX_ST_JSON)) {
            c = lex_get_uc(rt, lex);
            if (c == '{') {
                break;
            }

            lex_unget_uc(rt, lex, c);
            lex_append_uc(rt, lex, '$');
        } else if (c == '\\') {
            int pos = lex->uc_text.item_num;

            lex_append_uc(rt, lex, c);

            c = lex_get_uc(rt, lex);
            switch (c) {
            case 'b':
                lex->uc_text.item_num = pos;
                rjs_uchar_buffer_append_uc(rt, &lex->uc_text, '\b');
                rjs_uchar_buffer_append_uc(rt, &lex->raw_uc_text, 'b');
                break;
            case 'f':
                lex->uc_text.item_num = pos;
                rjs_uchar_buffer_append_uc(rt, &lex->uc_text, '\f');
                rjs_uchar_buffer_append_uc(rt, &lex->raw_uc_text, 'f');
                break;
            case 'n':
                lex->uc_text.item_num = pos;
                rjs_uchar_buffer_append_uc(rt, &lex->uc_text, '\n');
                rjs_uchar_buffer_append_uc(rt, &lex->raw_uc_text, 'n');
                break;
            case 'r':
                lex->uc_text.item_num = pos;
                rjs_uchar_buffer_append_uc(rt, &lex->uc_text, '\r');
                rjs_uchar_buffer_append_uc(rt, &lex->raw_uc_text, 'r');
                break;
            case 't':
                lex->uc_text.item_num = pos;
                rjs_uchar_buffer_append_uc(rt, &lex->uc_text, '\t');
                rjs_uchar_buffer_append_uc(rt, &lex->raw_uc_text, 't');
                break;
            case 'v':
                lex->uc_text.item_num = pos;
                rjs_uchar_buffer_append_uc(rt, &lex->uc_text, '\v');
                rjs_uchar_buffer_append_uc(rt, &lex->raw_uc_text, 'v');
                break;
            case 'x':
                lex_append_uc(rt, lex, 'x');

                c = lex_hex_escape(rt, lex, RJS_TRUE);
                if (c >= 0) {
                    lex->uc_text.item_num = pos;
                    rjs_uchar_buffer_append_uc(rt, &lex->uc_text, c);
                } else {
                    escape_error = RJS_TRUE;
                }
                break;
            case 'u':
                lex_append_uc(rt, lex, 'u');

                c = lex_unicode_escape(rt, lex, RJS_TRUE);
                if (c >= 0) {
                    lex->uc_text.item_num = pos;
                    rjs_uchar_buffer_append_uc(rt, &lex->uc_text, c);
                } else {
                    escape_error = RJS_TRUE;
                }
                break;
            case '0':
                c = lex_get_uc(rt, lex);
                lex_unget_uc(rt, lex, c);
                rjs_uchar_buffer_append_uc(rt, &lex->raw_uc_text, '0');
                if (!rjs_uchar_is_digit(c)) {
                    lex->uc_text.item_num = pos;
                    rjs_uchar_buffer_append_uc(rt, &lex->uc_text, 0);
                } else {
                    rjs_input_get_location(lex->input, &loc);
                    lex_warning(rt, lex, &loc, _("illegal escape character"));
                    escape_error = RJS_TRUE;
                }
                break;
            default:
                if (rjs_uchar_is_digit(c)) {
                    rjs_uchar_buffer_append_uc(rt, &lex->raw_uc_text, c);

                    rjs_input_get_location(lex->input, &loc);
                    lex_warning(rt, lex, &loc, _("illegal escape character"));
                    escape_error = RJS_TRUE;
                } else if (rjs_uchar_is_line_terminator(c)) {
                    lex->uc_text.item_num = pos;
                    rjs_uchar_buffer_append_uc(rt, &lex->raw_uc_text, c);
                } else {
                    lex->uc_text.item_num = pos;
                    rjs_uchar_buffer_append_uc(rt, &lex->uc_text, c);
                    rjs_uchar_buffer_append_uc(rt, &lex->raw_uc_text, c);
                }
                break;
            }
        } else {
            lex_append_uc(rt, lex, c);
        }
    }

    if (lex->status & RJS_LEX_ST_JSON) {
        /*JSON*/
        token->type = RJS_TOKEN_STRING;
        rjs_string_from_uchars(rt, token->value, lex->uc_text.items, lex->uc_text.item_num);
    } else {
        /*Script*/
        if (is_end) {
            lex->template_brace_level.item_num --;

            if (is_head)
                token->type = RJS_TOKEN_TEMPLATE;
            else
                token->type = RJS_TOKEN_TEMPLATE_TAIL;
        } else {
            if (is_head)
                token->type = RJS_TOKEN_TEMPLATE_HEAD;
            else
                token->type = RJS_TOKEN_TEMPLATE_MIDDLE;
        }

        if (escape_error) {
            token->flags |= RJS_TOKEN_FL_INVALIE_ESCAPE;
            rjs_value_set_undefined(rt, str);
        } else {
            rjs_string_from_uchars(rt, str, lex->uc_text.items, lex->uc_text.item_num);
        }

        if (!escape_error && (lex->raw_uc_text.item_num == lex->uc_text.item_num)) {
            rjs_value_copy(rt, raw, str);
        } else {
            rjs_string_from_uchars(rt, raw,
                    lex->raw_uc_text.items,
                    lex->raw_uc_text.item_num);
        }

        rjs_input_get_position(lex->input,
                &token->location.last_line,
                &token->location.last_column,
                &token->location.last_pos);

        rjs_template_entry_new(rt, &token->location, str, raw, token);
    }

    rjs_value_stack_restore(rt, top);
}

/**
 * Get the next token from the input.
 * \param rt The current runtime.
 * \param lex The lexical analyzer.
 * \param[out] token Return the new token.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_lex_get_token (RJS_Runtime *rt, RJS_Lex *lex, RJS_Token *token)
{
    int c, c1;

    token->type  = -1;
    token->flags = 0;

    if (lex->status & RJS_LEX_ST_FIRST_TOKEN) {
#if ENABLE_HASHBANG_COMMENT
        c = lex_get_uc(rt, lex);
        if (c == '#') {
            c1 = lex_get_uc(rt, lex);
            if (c1 == '!') {
                while (1) {
                    c = lex_get_uc(rt, lex);
                    if (c == RJS_INPUT_END)
                        break;
                    if (rjs_uchar_is_line_terminator(c))
                        break;
                }
            } else {
                lex_unget_uc(rt, lex, c1);
                lex_unget_uc(rt, lex, c);
            }
        } else {
            lex_unget_uc(rt, lex, c);
        }
#endif /*ENABLE_HASHBANG_COMMENT*/
        lex->status &= ~RJS_LEX_ST_FIRST_TOKEN;
    }

retry:
    /*Eatup space and comment.*/
    while (1) {
        c = lex_get_uc(rt, lex);
        if (c == RJS_INPUT_END) {
            rjs_input_get_location(lex->input, &token->location);
            token->location.first_column ++;
            token->location.last_column ++;
            token->type = RJS_TOKEN_END;
            goto end;
        }

        if (c == '/') {
            c1 = lex_get_uc(rt, lex);
            if (c1 == '/') {
                lex_single_line_comment(rt, lex);
                continue;
            } else if (c1 == '*') {
                lex_multi_line_comment(rt, lex);
                continue;
            } else {
                lex_unget_uc(rt, lex, c1);
                break;
            }
        }

        if (!rjs_uchar_is_white_space(c))
            break;
    }

    /*Store the first character's position.*/
    rjs_input_get_position(lex->input,
            &token->location.first_line,
            &token->location.first_column,
            &token->location.first_pos);
    token->location.first_pos --;

    switch (c) {
    case '\"':
    case '\'':
        lex_string(rt, lex, token, c);
        break;
    case '.':
        c = lex_get_uc(rt, lex);

        lex_unget_uc(rt, lex, c);
        lex_unget_uc(rt, lex, '.');

        if (rjs_uchar_is_digit(c)) {
            lex_number(rt, lex, token);
        } else {
            lex_punctuator(rt, lex, token);
        }
        break;
    case '/':
        if (lex->flags & RJS_LEX_FL_DIV) {
            lex_unget_uc(rt, lex, c);
            lex_punctuator(rt, lex, token);
        } else {
            lex_regexp(rt, lex, token);
        }
        break;
    case '`':
        lex_template(rt, lex, RJS_TRUE, token);
        break;
    case '{':
        lex->brace_level ++;
        token->type = RJS_TOKEN_lbrace;
        break;
    case '}':
        if (lex->template_brace_level.item_num
                && (lex->template_brace_level.items[lex->template_brace_level.item_num - 1]
                        == lex->brace_level)) {
            lex_template(rt, lex, RJS_FALSE, token);
        } else {
            lex->brace_level --;

            token->type = RJS_TOKEN_rbrace;
        }
        break;
    case '\\':
        c = lex_get_uc(rt, lex);

        lex_unget_uc(rt, lex, c);
        lex_unget_uc(rt, lex, '\\');

        if (c == 'u') {
            lex_identifier(rt, lex, token);
        } else {
            lex_punctuator(rt, lex, token);
        }
        break;
#if ENABLE_PRIV_NAME
    case '#':
        lex_private_identifier(rt, lex, token);
        break;
#endif /*ENABLE_PRIV_NAME*/
    default:
        lex_unget_uc(rt, lex, c);

        if (rjs_uchar_is_digit(c)) {
            lex_number(rt, lex, token);
        } else if (rjs_uchar_is_id_start(c)) {
            lex_identifier(rt, lex, token);
        } else {
            lex_punctuator(rt, lex, token);
        }
        break;
    }

    if (token->type == -1) {
        token->flags = 0;
        goto retry;
    }

    /*Store the last character's position.*/
    rjs_input_get_position(lex->input,
            &token->location.last_line,
            &token->location.last_column,
            &token->location.last_pos);
end:
    return RJS_OK;
}

/**
 * Get the next JSON token from the input.
 * \param rt The current runtime.
 * \param lex The lexical analyzer.
 * \param[out] token Return the new token.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_lex_get_json_token (RJS_Runtime *rt, RJS_Lex *lex, RJS_Token *token)
{
    int c, c1;

    token->type  = -1;
    token->flags = 0;

    /*Eatup space and comment.*/
    while (1) {
        c = lex_get_uc(rt, lex);
        if (c == RJS_INPUT_END) {
            rjs_input_get_location(lex->input, &token->location);
            token->location.first_column ++;
            token->location.last_column ++;
            token->type = RJS_TOKEN_END;
            goto end;
        }

        if (c == '/') {
            c1 = lex_get_uc(rt, lex);
            if (c1 == '/') {
                lex_single_line_comment(rt, lex);
                continue;
            } else if (c1 == '*') {
                lex_multi_line_comment(rt, lex);
                continue;
            } else {
                lex_unget_uc(rt, lex, c1);
                break;
            }
        }

        if (!rjs_uchar_is_white_space(c))
            break;
    }

    /*Store the first character's position.*/
    rjs_input_get_position(lex->input,
            &token->location.first_line,
            &token->location.first_column,
            &token->location.first_pos);
    token->location.first_pos --;

    switch (c) {
    case '\"':
    case '\'':
        lex_string(rt, lex, token, c);
        break;
    case '`':
        lex_template(rt, lex, RJS_TRUE, token);
        break;
    case '.':
        lex_unget_uc(rt, lex, c);
        lex_number(rt, lex, token);
        break;
    case '{':
        token->type = RJS_TOKEN_lbrace;
        break;
    case '}':
        token->type = RJS_TOKEN_rbrace;
        break;
    case '[':
        token->type = RJS_TOKEN_lbracket;
        break;
    case ']':
        token->type = RJS_TOKEN_rbracket;
        break;
    case ',':
        token->type = RJS_TOKEN_comma;
        break;
    case ':':
        token->type = RJS_TOKEN_colon;
        break;
    case '+':
        token->type = RJS_TOKEN_plus;
        break;
    case '-':
        token->type = RJS_TOKEN_minus;
        break;
    case '\\':
        lex_unget_uc(rt, lex, c);
        lex_identifier(rt, lex, token);
        break;
    default:
        lex_unget_uc(rt, lex, c);

        if (rjs_uchar_is_digit(c)) {
            lex_number(rt, lex, token);
        } else if (rjs_uchar_is_id_start(c)) {
            lex_identifier(rt, lex, token);
        } else {
            RJS_Location loc;

            rjs_input_get_location(lex->input, &loc);
            lex_error(rt, lex, &loc, _("illegal character"));
        }
        break;
    }

    /*Store the last character's position.*/
    rjs_input_get_position(lex->input,
            &token->location.last_line,
            &token->location.last_column,
            &token->location.last_pos);
end:
    if (rjs_lex_error(lex))
        return RJS_ERR;

    return RJS_OK;
}

/**
 * Get the token type's name.
 * \param type The token type.
 * \param flags The flags of the token.
 * \return The name of the token type.
 */
const char*
rjs_token_type_get_name (RJS_TokenType type, int flags)
{
    const char *r = NULL;

    switch (type) {
    case RJS_TOKEN_END:
        r = "END";
        break;
    case RJS_TOKEN_NUMBER:
        r = "number";
        break;
    case RJS_TOKEN_REGEXP:
        r = "regexp";
        break;
    case RJS_TOKEN_STRING:
        r = "string";
        break;
    case RJS_TOKEN_TEMPLATE:
    case RJS_TOKEN_TEMPLATE_HEAD:
    case RJS_TOKEN_TEMPLATE_MIDDLE:
    case RJS_TOKEN_TEMPLATE_TAIL:
        r = "template";
        break;
#if ENABLE_PRIV_NAME
    case RJS_TOKEN_PRIVATE_IDENTIFIER:
        r = "private identifier";
        break;
#endif /*ENABLE_PRIV_NAME*/
    case RJS_TOKEN_IDENTIFIER:
        if (flags & (RJS_TOKEN_FL_RESERVED
                |RJS_TOKEN_FL_STRICT_RESERVED
                |RJS_TOKEN_FL_KNOWN_IDENTIFIER))
            r = identifier_names[(flags & RJS_TOKEN_IDENTIFIER_MASK) - RJS_IDENTIFIER_START - 1];
        else
            r = "identifier";
        break;
    default:
        r = token_names[type - RJS_TOKEN_PUNCT_START - 1];
        break;
    }

    return r;
}
