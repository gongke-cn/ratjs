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
 * Add the arguments object to the environment.
 * \param rt The current runtime.
 * \param env The environment.
 * \param ao The arguments object.
 * \param strict In strict mode or not.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_env_add_arguments_object (RJS_Runtime *rt, RJS_Environment *env, RJS_Value *ao, RJS_Bool strict)
{
    RJS_Result      r;
    RJS_BindingName bn;

    rjs_binding_name_init(rt, &bn, rjs_s_arguments(rt));

    if (strict)
        r = rjs_env_create_immutable_binding(rt, env, &bn, RJS_FALSE);
    else
        r = rjs_env_create_mutable_binding(rt, env, &bn, RJS_FALSE);

    if (r == RJS_ERR)
        goto end;

    r = rjs_env_initialize_binding(rt, env, &bn, ao);
end:
    rjs_binding_name_deinit(rt, &bn);
    return r;
}
