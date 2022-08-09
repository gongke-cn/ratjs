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

/*Throw an URI error.*/
static RJS_Result
throw_uri_error (RJS_Runtime *rt)
{
    RJS_Realm *realm = rjs_realm_current(rt);
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *e     = rjs_value_stack_push(rt);
    RJS_Value *msg   = rjs_value_stack_push(rt);
    RJS_Result r;

    rjs_string_from_chars(rt, msg, _("URI error"), -1);

    if ((r = rjs_call(rt, rjs_o_URIError(realm), rjs_v_undefined(rt), msg, 1, e)) == RJS_ERR)
        goto end;

    r = rjs_throw(rt, e);
end:
    rjs_value_stack_restore(rt, top);
    return RJS_ERR;
}

/*Decode the URI.*/
static RJS_Result
uri_decode (RJS_Runtime *rt, RJS_Value *str, RJS_Bool comp, RJS_Value *rv)
{
    size_t           len;
    const RJS_UChar *c, *e;
    RJS_UCharBuffer  ucb;
    RJS_Result       r;

    rjs_uchar_buffer_init(rt, &ucb);

    len = rjs_string_get_length(rt, str);
    c   = rjs_string_get_uchars(rt, str);
    e   = c + len;

    while (c < e) {
        if (*c != '%') {
            rjs_uchar_buffer_append_uchar(rt, &ucb, *c);
            c ++;
        } else {
            int uc, n;

            if (e - c < 2) {
                r = throw_uri_error(rt);
                goto end;
            }

            if (!rjs_uchar_is_xdigit(c[1]) || !rjs_uchar_is_xdigit(c[2])) {
                r = throw_uri_error(rt);
                goto end;
            }

            uc = (rjs_hex_char_to_number(c[1]) << 4) | rjs_hex_char_to_number(c[2]);

            if (!comp) {
                switch (uc) {
                case ';':
                case '/':
                case '?':
                case ':':
                case '@':
                case '&':
                case '=':
                case '+':
                case '$':
                case ',':
                case '#':
                    rjs_uchar_buffer_append_uchar(rt, &ucb, '%');
                    rjs_uchar_buffer_append_uchar(rt, &ucb, c[1]);
                    rjs_uchar_buffer_append_uchar(rt, &ucb, c[2]);
                    c += 3;
                    continue;
                default:
                    break;
                }
            }

            c += 3;

            if (!(uc & 0x80)) {
                n = 0;
            } else if ((uc & 0xe0) == 0xc0) {
                n   = 1;
                uc &= 0x1f;
            } else if ((uc & 0xf0) == 0xe0) {
                n   = 2;
                uc &= 0x0f;
            } else if ((uc & 0xf8) == 0xf0) {
                n   = 3;
                uc &= 0x07;
            } else {
                r = throw_uri_error(rt);
                goto end;
            }

            if (e - c < n * 3) {
                r = throw_uri_error(rt);
                goto end;
            }

            while (n > 0) {
                int v;

                if (*c != '%') {
                    r = throw_uri_error(rt);
                    goto end;
                }

                if (!rjs_uchar_is_xdigit(c[1]) || !rjs_uchar_is_xdigit(c[2])) {
                    r = throw_uri_error(rt);
                    goto end;
                }

                v = (rjs_hex_char_to_number(c[1]) << 4) | rjs_hex_char_to_number(c[2]);
                if ((v & 0xc0) != 0x80) {
                    r = throw_uri_error(rt);
                    goto end;
                }

                uc <<= 6;
                uc |= (v & 0x3f);

                c += 3;
                n --;
            }

            rjs_uchar_buffer_append_uc(rt, &ucb, uc);
        }
    }

    rjs_string_from_uchars(rt, rv, ucb.items, ucb.item_num);
    r = RJS_OK;
end:
    rjs_uchar_buffer_deinit(rt, &ucb);
    return r;
}

/*Encode the URI.*/
static RJS_Result
uri_encode (RJS_Runtime *rt, RJS_Value *str, RJS_Bool comp, RJS_Value *rv)
{
    size_t           len;
    const RJS_UChar *c, *e;
    RJS_UCharBuffer  ucb;
    RJS_Result       r;

    rjs_uchar_buffer_init(rt, &ucb);

    len = rjs_string_get_length(rt, str);
    c   = rjs_string_get_uchars(rt, str);
    e   = c + len;

    while (c < e) {
        RJS_Bool escape = RJS_TRUE;

        switch (*c) {
        case '-':
        case '_':
        case '.':
        case '!':
        case '~':
        case '*':
        case '\'':
        case '(':
        case ')':
            escape = RJS_FALSE;
            break;
        default:
            if (rjs_uchar_is_alnum(*c)) {
                escape = RJS_FALSE;
            } else if (!comp) {
                switch (*c) {
                case ';':
                case '/':
                case '?':
                case ':':
                case '@':
                case '&':
                case '=':
                case '+':
                case '$':
                case ',':
                case '#':
                    escape = RJS_FALSE;
                    break;
                default:
                    break;
                }
            }
        }

        if (!escape) {
            rjs_uchar_buffer_append_uchar(rt, &ucb, *c);
            c ++;
        } else {
            int  uc;
            char buf[12];

            if (rjs_uchar_is_trailing_surrogate(*c)) {
                r = throw_uri_error(rt);
                goto end;
            }

            if (rjs_uchar_is_leading_surrogate(*c)) {
                if ((e - c < 2) || !rjs_uchar_is_trailing_surrogate(c[1])) {
                    r = throw_uri_error(rt);
                    goto end;
                }

                uc = rjs_surrogate_pair_to_uc(c[0], c[1]);
                c += 2;
            } else {
                uc = *c;
                c ++;
            }

            if (uc <= 0x7f) {
                buf[0] = '%';
                buf[1] = rjs_number_to_hex_char_u(uc >> 4);
                buf[2] = rjs_number_to_hex_char_u(uc & 0xf);

                rjs_uchar_buffer_append_chars(rt, &ucb, buf, 3);
            } else if (uc <= 0x7ff) {
                buf[0] = '%';
                buf[1] = rjs_number_to_hex_char_u((uc >> 10) | 0xc);
                buf[2] = rjs_number_to_hex_char_u((uc >> 6) & 0xf);

                buf[3] = '%';
                buf[4] = rjs_number_to_hex_char_u(((uc >> 4) & 0x3) | 0x8);
                buf[5] = rjs_number_to_hex_char_u(uc & 0xf);

                rjs_uchar_buffer_append_chars(rt, &ucb, buf, 6);
            } else if (uc <= 0xffff) {
                buf[0] = '%';
                buf[1] = rjs_number_to_hex_char_u(0xe);
                buf[2] = rjs_number_to_hex_char_u(uc >> 12);

                buf[3] = '%';
                buf[4] = rjs_number_to_hex_char_u(((uc >> 10) & 0x3) | 0x8);
                buf[5] = rjs_number_to_hex_char_u((uc >> 6) & 0xf);

                buf[6] = '%';
                buf[7] = rjs_number_to_hex_char_u(((uc >> 4) & 0x3) | 0x8);
                buf[8] = rjs_number_to_hex_char_u(uc & 0xf);

                rjs_uchar_buffer_append_chars(rt, &ucb, buf, 9);
            } else {
                buf[0] = '%';
                buf[1] = rjs_number_to_hex_char_u(0xf);
                buf[2] = rjs_number_to_hex_char_u((uc >> 18));

                buf[3] = '%';
                buf[4] = rjs_number_to_hex_char_u(((uc >> 16) & 0x3) | 0x8);
                buf[5] = rjs_number_to_hex_char_u((uc >> 12) & 0xf);

                buf[6] = '%';
                buf[7] = rjs_number_to_hex_char_u(((uc >> 10) & 0x3) | 0x8);
                buf[8] = rjs_number_to_hex_char_u((uc >> 6) & 0xf);

                buf[9]  = '%';
                buf[10] = rjs_number_to_hex_char_u(((uc >> 4) & 0x3) | 0x8);
                buf[11] = rjs_number_to_hex_char_u(uc & 0xf);

                rjs_uchar_buffer_append_chars(rt, &ucb, buf, 12);
            }
        }
    }

    rjs_string_from_uchars(rt, rv, ucb.items, ucb.item_num);
    r = RJS_OK;
end:
    rjs_uchar_buffer_deinit(rt, &ucb);
    return r;
}

/*decodeURI*/
static RJS_NF(global_decodeURI)
{
    RJS_Value *arg = rjs_argument_get(rt, args, argc, 0);
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *str = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_to_string(rt, arg, str)) == RJS_ERR)
        goto end;

    r = uri_decode(rt, str, RJS_FALSE, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*decodeURIComponent*/
static RJS_NF(global_decodeURIComponent)
{
    RJS_Value *arg = rjs_argument_get(rt, args, argc, 0);
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *str = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_to_string(rt, arg, str)) == RJS_ERR)
        goto end;

    r = uri_decode(rt, str, RJS_TRUE, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*encodeURI*/
static RJS_NF(global_encodeURI)
{
    RJS_Value *arg = rjs_argument_get(rt, args, argc, 0);
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *str = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_to_string(rt, arg, str)) == RJS_ERR)
        goto end;

    r = uri_encode(rt, str, RJS_FALSE, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*encodeURIComponent*/
static RJS_NF(global_encodeURIComponent)
{
    RJS_Value *arg = rjs_argument_get(rt, args, argc, 0);
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *str = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_to_string(rt, arg, str)) == RJS_ERR)
        goto end;

    r = uri_encode(rt, str, RJS_TRUE, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}
