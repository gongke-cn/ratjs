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

/**Convert UTF-8 to unicode.*/
static RJS_Result
utf8_to_uc (uint8_t *c, size_t len, uint32_t *puc)
{
    if (len < 1)
        return 0;

    if (c[0] < 0x80) {
        *puc = *c;
        return 1;
    } else if ((c[0] & 0xe0) == 0xc0) {
        if (len < 2)
            return 0;

        *puc = ((c[0] & 0x1f) << 6) | (c[1] & 0x3f);
        return 2;
    } else if ((c[0] & 0xf0) == 0xe0) {
        if (len < 3)
            return 0;

        *puc = ((c[0] & 0x0f) << 12) | ((c[1] & 0x3f) << 6) | (c[2] & 0x3f);
        return 3;
    } else if ((c[0] & 0xf8) == 0xf0) {
        if (len < 4)
            return 0;

        *puc = ((c[0] & 0x07) << 18)
                | ((c[1] & 0x3f) << 12)
                | ((c[2] & 0x3f) << 6)
                | (c[3] & 0x3f);
        return 4;
    } else {
        RJS_LOGE("illegal UTF-8 character");
        return RJS_ERR;
    }
}

/**Convert UTF-8 from unicode.*/
static RJS_Result
utf8_from_uc (uint32_t uc, uint8_t *c, size_t len)
{
    if (uc < 0x80) {
        c[0] = uc;
        return 1;
    } else if (uc < 0x800) {
        c[0] = (uc >> 6) | 0xc0;
        c[1] = (uc & 0x3f) | 0x80;
        return 2;
    } else if (uc < 0x10000) {
        c[0] = (uc >> 12) | 0xe0;
        c[1] = ((uc >> 6) & 0x3f) | 0x80;
        c[2] = (uc & 0x3f) | 0x80;
        return 3;
    } else if (uc < 0x200000) {
        c[0] = (uc >> 18) | 0xf0;
        c[1] = ((uc >> 12) & 0x3f) | 0x80;
        c[2] = ((uc >> 6) & 0x3f) | 0x80;
        c[3] = (uc & 0x3f) | 0x80;
        return 4;
    } else {
        RJS_LOGE("illegal unicode");
        return RJS_ERR;
    }
}

/**Convert UCS-2LE to unicode.*/
static RJS_Result
ucs_2le_to_uc (uint8_t *c, size_t len, uint32_t *puc)
{
    uint16_t c1, c2;
    uint32_t uc;

    if (len < 2)
        return 0;

    c1 = c[0] | (c[1] << 8);
    if (!rjs_uchar_is_leading_surrogate(c1)) {
        *puc = c1;
        return 2;
    }

    if (len < 4)
        return 0;

    c2 = c[2] | (c[3] << 8);
    if (!rjs_uchar_is_trailing_surrogate(c2)) {
        *puc = c1;
        return 2;
    }

    uc = rjs_surrogate_pair_to_uc(c1, c2);
    *puc = uc;

    return 4;
}

/**Convert UCS-2LE from unicode.*/
static RJS_Result
ucs_2le_from_uc (uint32_t uc, uint8_t *c, size_t len)
{
    if (uc > 0xffff) {
        uint16_t c1, c2;

        if (len < 4)
            return 0;

        rjs_uc_to_surrogate_pair(uc, &c1, &c2);

        c[0] = c1;
        c[1] = c1 >> 8;
        c[2] = c2;
        c[3] = c2 >> 8;
        return 4;
    } else {
        if (len < 2)
            return 0;

        c[0] = uc;
        c[1] = uc >> 8;
        return 2;
    }
}

/**Convert UCS-2BE to unicode.*/
static RJS_Result
ucs_2be_to_uc (uint8_t *c, size_t len, uint32_t *puc)
{
    uint16_t c1, c2;
    uint32_t uc;

    if (len < 2)
        return 0;

    c1 = c[1] | (c[0] << 8);
    if (!rjs_uchar_is_leading_surrogate(c1)) {
        *puc = c1;
        return 2;
    }

    if (len < 4)
        return 0;

    c2 = c[3] | (c[2] << 8);
    if (!rjs_uchar_is_trailing_surrogate(c2)) {
        *puc = c1;
        return 2;
    }

    uc = rjs_surrogate_pair_to_uc(c1, c2);
    *puc = uc;

    return 4;
}

/**Convert UCS-2BE from unicode.*/
static RJS_Result
ucs_2be_from_uc (uint32_t uc, uint8_t *c, size_t len)
{
    if (uc > 0xffff) {
        uint16_t c1, c2;

        if (len < 4)
            return 0;

        rjs_uc_to_surrogate_pair(uc, &c1, &c2);

        c[0] = c1 >> 8;
        c[1] = c1;
        c[2] = c2 >> 8;
        c[3] = c2;
        return 4;
    } else {
        if (len < 2)
            return 0;

        c[0] = uc >> 8;
        c[1] = uc;
        return 2;
    }
}

/**Convert UCS-4LE to unicode.*/
static RJS_Result
ucs_4le_to_uc (uint8_t *c, size_t len, uint32_t *puc)
{
    uint32_t uc;

    if (len < 4)
        return 0;

    uc   = (c[3] << 24) | (c[2] << 16) | (c[1] << 8) | c[0];
    *puc = uc;
    return 4;
}

/**Convert UCS-4LE from unicode.*/
static RJS_Result
ucs_4le_from_uc (uint32_t uc, uint8_t *c, size_t len)
{
    if (len < 4)
        return 0;

    c[0] = uc;
    c[1] = (uc >> 8) & 0xff;
    c[2] = (uc >> 16) & 0xff;
    c[3] = uc >> 24;
    return 4;
}

/**Convert UCS-4BE to unicode.*/
static RJS_Result
ucs_4be_to_uc (uint8_t *c, size_t len, uint32_t *puc)
{
    uint32_t uc;

    if (len < 4)
        return 0;

    uc   = (c[0] << 24) | (c[1] << 16) | (c[2] << 8) | c[3];
    *puc = uc;
    return 4;
}

/**Convert UCS-4BE from unicode.*/
static RJS_Result
ucs_4be_from_uc (uint32_t uc, uint8_t *c, size_t len)
{
    if (len < 4)
        return 0;

    c[0] = uc >> 24;
    c[1] = (uc >> 16) & 0xff;
    c[2] = (uc >> 8) & 0xff;
    c[3] = uc;
    return 4;
}

/**Encodings.*/
static const RJS_EncOps
encodings[] = {
    {
        "UTF-8",
        utf8_to_uc,
        utf8_from_uc
    },
    {
        "UCS-2LE",
        ucs_2le_to_uc,
        ucs_2le_from_uc
    },
    {
        "UCS-2BE",
        ucs_2be_to_uc,
        ucs_2be_from_uc
    },
    {
        "UCS-4LE",
        ucs_4le_to_uc,
        ucs_4le_from_uc
    },
    {
        "UCS-4BE",
        ucs_4be_to_uc,
        ucs_4be_from_uc
    },
    {NULL}
};

/*Lookup the encoding by its name.*/
static const RJS_EncOps*
enc_lookup (const char *name)
{
    const RJS_EncOps *p = encodings;

    while (p->name) {
        if (!strcasecmp(p->name, name))
            return p;

        p ++;
    }

    RJS_LOGE("do not support encoding \"%s\"", name);
    return NULL;
}

/**
 * Initialize a character encoding convertor.
 * \param rt The current runtime.
 * \param conv The convertor to be initlized.
 * \param enc_in The input encoding.
 * \param enc_out The output encoding.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_conv_init (RJS_Runtime *rt, RJS_Conv *conv, const char *enc_in, const char *enc_out)
{
    if (!(conv->enc_in  = enc_lookup(enc_in)))
        return RJS_ERR;

    if (!(conv->enc_out = enc_lookup(enc_out)))
        return RJS_ERR;

    return RJS_OK;
}

/**
 * Convert encoding.
 * \param rt The current runtime.
 * \param conv The convertor.
 * \param in The input characters' pointer.
 * \param in_left The left characters number in the input buffer.
 * \param out The output characters' pointer.
 * \param out_left The left space of the output buffer.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_conv_run (RJS_Runtime *rt, RJS_Conv *conv, const char **in,
        size_t *in_left, char **out, size_t *out_left)
{
    uint8_t   *ip = *(uint8_t**)in;
    size_t     il = *in_left;
    uint8_t   *op = *(uint8_t**)out;
    size_t     ol = *out_left;
    RJS_Result r  = RJS_OK;

    while (il && ol) {
        int      in_n, out_n;
        uint32_t uc;

        in_n = conv->enc_in->to_uc(ip, il, &uc);
        if (in_n < 0) {
            r = RJS_ERR;
            break;
        }

        if (in_n == 0)
            break;

        out_n = conv->enc_out->from_uc(uc, op, ol);
        if (out_n < 0) {
            r = RJS_ERR;
            break;
        }

        if (out_n == 0) {
            r = RJS_FALSE;
            break;
        }

        ip += in_n;
        il -= in_n;
        op += out_n;
        ol -= out_n;
    }

    *in  = (char*)ip;
    *out = (char*)op;
    *in_left  = il;
    *out_left = ol;

    return r;
}

/**
 * Release the character convertor.
 * \param rt The current runtime.
 * \param conv The convertor to be released.
 */
void
rjs_conv_deinit (RJS_Runtime *rt, RJS_Conv *conv)
{
}
