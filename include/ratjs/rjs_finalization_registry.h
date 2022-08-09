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
 * Finalization registry.
 */

#ifndef _RJS_FINALIZATION_REGISTRY_H_
#define _RJS_FINALIZATION_REGISTRY_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup final_reg Finalization registry
 * Register callback function which is invoked when the object is finalized
 * @{
 */

/**
 * Create a new finalization registry.
 * \param rt The current runtime.
 * \param[out] registry Return the new registry object.
 * \param nt The new target.
 * \param func The callback function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_finalization_registry_new (RJS_Runtime *rt, RJS_Value *registry, RJS_Value *nt, RJS_Value *func);

/**
 * Register a finalization callback.
 * \param rt The current runtime.
 * \param registry The registry.
 * \param target The target value.
 * \param held The held value.
 * \param token Unregister token.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_finalization_register (RJS_Runtime *rt, RJS_Value *registry, RJS_Value *target,
        RJS_Value *held, RJS_Value *token);

/**
 * Unregister a finalization callback.
 * \param rt The current runtime.
 * \param registry The registry.
 * \param token Unregister token.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE Cannot find the callback.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_finalization_unregister (RJS_Runtime *rt, RJS_Value *registry, RJS_Value *token);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

