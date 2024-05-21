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
 * Object with native data.
 */

#ifndef _RJS_NATIVE_OBJECT_H_
#define _RJS_NATIVE_OBJECT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup native_obj Native object
 * Native object
 * @{
 */

/**
 * Create a new native object.
 * \param rt The current runtime.
 * \param[out] o Return the new native object.
 * \param proto The prototype of the object.
 * If proto == NULL, use Object.prototype.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_native_object_new (RJS_Runtime *rt, RJS_Value *o, RJS_Value *proto);

/**
 * Create a new native object from the constructor.
 * \param rt The current runtime.
 * \param c The constructor.
 * \param proto The default prototype object.
 * \param[out] o REturn the new native object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_native_object_from_constructor (RJS_Runtime *rt, RJS_Value *c, RJS_Value *proto, RJS_Value *o);

/**
 * Create a new native function object.
 * \param rt The current runtime.
 * \param[out] v Return the new native function object value.
 * \param realm The function's realm.
 * \param proto The prototype of the function.
 * \param script The script contains the function.
 * \param nf The native function pointer.
 * \param flags The flags of the function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_native_func_object_new (RJS_Runtime *rt, RJS_Value *v, RJS_Realm *realm,
        RJS_Value *proto, RJS_Script *script, RJS_NativeFunc nf, int flags);

/**
 * Create a new native function.
 * \param rt The current runtime.
 * \param mod The module contains this function.
 * \param nf The native function.
 * \param len The parameters length.
 * \param name The function's name.
 * \param realm The realm.
 * \param proto The prototype value.
 * \param prefix The name prefix.
 * \param[out] f Return the new built-in function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_create_native_function (RJS_Runtime *rt, RJS_Value *mod, RJS_NativeFunc nf, size_t len,
        RJS_Value *name, RJS_Realm *realm, RJS_Value *proto, RJS_Value *prefix, RJS_Value *f);

/**
 * Set the native data of the object.
 * \param rt The current runtime.
 * \param o The native object.
 * \param tag The tag of the data.
 * \param data The data's pointer.
 * \param scan The function scan the referenced things in the data.
 * \param free The function free the data.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_native_object_set_data (RJS_Runtime *rt, RJS_Value *o, const void *tag,
        void *data, RJS_ScanFunc scan, RJS_FreeFunc free);

/**
 * Get the native data's tag.
 * \param rt The current runtime.
 * \param o The native object.
 * \return The native object's tag.
 */
extern const void*
rjs_native_object_get_tag (RJS_Runtime *rt, RJS_Value *o);

/**
 * Get the native data's pointer of the object.
 * \param rt The current runtime.
 * \param o The native object.
 * \retval The native data's pointer.
 */
extern void*
rjs_native_object_get_data (RJS_Runtime *rt, RJS_Value *o);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

