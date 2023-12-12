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
 * Realm.
 */

#ifndef _RJS_REALM_H_
#define _RJS_REALM_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup realm Realm
 * Realm
 * @{
 */

/**
 * Get the global object's pointer.
 * \param realm The realm.
 * \return The global object's pointer.
 */
static inline RJS_Value*
rjs_global_object (RJS_Realm *realm)
{
    RJS_RealmBase *rb = (RJS_RealmBase*)realm;

    return &rb->global_object;
}

/**
 * Get the global environment.
 * \param realm The realm.
 * \return The global environment.
 */
static inline RJS_Environment*
rjs_global_env (RJS_Realm *realm)
{
    RJS_RealmBase *rb = (RJS_RealmBase*)realm;

    return rb->global_env;
}

/**
 * Create a new realm.
 * \param rt The current runtime.
 * \param[out] v The value buffer store the realm.
 * \return The new realm.
 */
extern RJS_Realm*
rjs_realm_new (RJS_Runtime *rt, RJS_Value *v);

/**
 * Load extension functions to the realm.
 * \param rt The current runtime.
 * \param realm The realm which extension functions to be loaded in.
 * If realm == NULL, functions will be loaded in current realm.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_realm_load_extension (RJS_Runtime *rt, RJS_Realm *realm);

/**
 * Get the "%IteratorPrototype%" object of the realm.
 * \param realm The realm.
 * \return "%IteratorPrototype%" value's pointer.
 */
extern RJS_Value*
rjs_realm_iterator_prototype (RJS_Realm *realm);

/**
 * Get the "Function.prototype" object of the realm.
 * \param realm The realm.
 * \return "Function.prototype" value's pointer.
 */
extern RJS_Value*
rjs_realm_function_prototype (RJS_Realm *realm);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

