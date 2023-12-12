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

/**
 * Create a typed array.
 * \param rt The runtime.
 * \param type The element type of the typed array.
 * \param args The arguments.
 * \param argc The arguments' count.
 * \param[out] ta Return the typed array.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_create_typed_array (RJS_Runtime *rt, RJS_ArrayElementType type,
        RJS_Value *args, size_t argc, RJS_Value *ta)
{
    RJS_Realm *realm = rjs_realm_current(rt);
    RJS_Value *c;

    switch (type) {
    case RJS_ARRAY_ELEMENT_UINT8:
        c = rjs_o_Uint8Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_INT8:
        c = rjs_o_Int8Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_UINT8C:
        c = rjs_o_Uint8ClampedArray(realm);
        break;
    case RJS_ARRAY_ELEMENT_UINT16:
        c = rjs_o_Uint16Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_INT16:
        c = rjs_o_Int16Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_UINT32:
        c = rjs_o_Uint32Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_INT32:
        c = rjs_o_Int32Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_FLOAT32:
        c = rjs_o_Float32Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_FLOAT64:
        c = rjs_o_Float64Array(realm);
        break;
#if ENABLE_BIG_INT
    case RJS_ARRAY_ELEMENT_BIGUINT64:
        c = rjs_o_BigUint64Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_BIGINT64:
        c = rjs_o_BigInt64Array(realm);
        break;
#endif /*ENABLE_BIG_INT*/
    default:
        assert(0);
    }

    return rjs_construct(rt, c, args, argc, c, ta);
}
