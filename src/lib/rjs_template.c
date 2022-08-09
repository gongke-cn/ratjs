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
 * Create a new template object.
 * \param rt The current runtime.
 * \param v The template object.
 * \param raw The raw items array.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_template_new (RJS_Runtime *rt, RJS_Value *v, RJS_Value *raw)
{
    size_t           top = rjs_value_stack_save(rt);
    RJS_PropertyDesc pd;

    rjs_set_integrity_level(rt, raw, RJS_INTEGRITY_FROZEN);

    rjs_property_desc_init(rt, &pd);
    pd.flags = RJS_PROP_FL_DATA;
    rjs_value_copy(rt, pd.value, raw);
    rjs_define_property_or_throw(rt, v, rjs_pn_raw(rt), &pd);
    rjs_property_desc_deinit(rt, &pd);

    rjs_set_integrity_level(rt, v, RJS_INTEGRITY_FROZEN);

    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}
