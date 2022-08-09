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
 * Weak reference.
 */

#ifndef _RJS_WEAK_REF_H_
#define _RJS_WEAK_REF_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup weak_ref Weak reference
 * Weak reference
 * @{
 */

/**Weak reference.*/
typedef struct RJS_WeakRef_s RJS_WeakRef;

/**Weak reference on finalize function.*/
typedef void (*RJS_WeakRefOnFinalFunc) (RJS_Runtime *rt, RJS_WeakRef *ref);

/**
 * Add a weak reference.
 * \param rt The current runtime.
 * \param base The base object.
 * \param ref The referenced object.
 * \param on_final On final function.
 * \return The weak reference.
 * \retval The value is not an GC thing.
 */
extern RJS_WeakRef*
rjs_weak_ref_add (RJS_Runtime *rt, RJS_Value *base, RJS_Value *ref,
        RJS_WeakRefOnFinalFunc on_final);

/**
 * Free a weak reference.
 * \param rt The current runtime.
 * \param wr The weak reference to be freed.
 */
extern void
rjs_weak_ref_free (RJS_Runtime *rt, RJS_WeakRef *wr);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

