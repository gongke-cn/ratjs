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
 * C type manager.
 */

#ifndef _RJS_CTYPE_H_
#define _RJS_CTYPE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * C type model.
 */
typedef enum {
    RJS_CTYPE_MODEL_STRUCT, /**< Structure or union.*/
    RJS_CTYPE_MODEL_FUNC    /**< Function.*/
} RJS_CTypeModel;

/**
 * C pointer type.
 */
typedef enum {
    RJS_CPTR_TYPE_VALUE,        /**< Pointer to a value.*/
    RJS_CPTR_TYPE_ARRAY,        /**< Pointer to an array.*/
    RJS_CPTR_TYPE_PTR_ARRAY,    /**< Pointer to a pointer array.*/
    RJS_CPTR_TYPE_UINT8_ARRAY,  /**< Pointer to a unsigned 8 bits integer buffer.*/
    RJS_CPTR_TYPE_INT8_ARRAY,   /**< Pointer to a signed 8 bits integer buffer.*/
    RJS_CPTR_TYPE_UINT8C_ARRAY, /**< Pointer to a unsigned 8 bits integer buffer. (clamped)*/
    RJS_CPTR_TYPE_INT16_ARRAY,  /**< Pointer to a signed 16 bits integer buffer.*/
    RJS_CPTR_TYPE_UINT16_ARRAY, /**< Pointer to a unsigned 16 bits integer buffer.*/
    RJS_CPTR_TYPE_INT32_ARRAY,  /**< Pointer to a signed 32 bits integer buffer.*/
    RJS_CPTR_TYPE_UINT32_ARRAY, /**< Pointer to a unsigned 32 bits integer buffer.*/
    RJS_CPTR_TYPE_FLOAT32_ARRAY,/**< Pointer to a 32 bits float point number buffer.*/
    RJS_CPTR_TYPE_FLOAT64_ARRAY,/**< Pointer to a 64 bits float point number buffer.*/
    RJS_CPTR_TYPE_UINT64_ARRAY, /**< Pointer to a unsigned 64 bits integer buffer.*/
    RJS_CPTR_TYPE_INT64_ARRAY   /**< Pointer to a signed 64 bits integer buffer.*/
} RJS_CPtrType;

/**The pointer will be freed when the object is collected.*/
#define RJS_CPTR_FL_AUTO_FREE 1
/**The pointed buffer is readoly.*/
#define RJS_CPTR_FL_READONLY  2

/**
 * C type.
 */
typedef struct RJS_CType_s RJS_CType;

/**
 * Lookup a C type by its name.
 * \param rt The current runtime.
 * \param name The name of the C type.
 * \param[out] pty Return the C type.
 * \retval RJS_OK On success.
 * \retval RJS_ERR Cannot find the C type.
 */
extern RJS_Result
rjs_get_c_type (RJS_Runtime *rt, RJS_Value *name, RJS_CType **pty);

/**
 * Create a new C type.
 * \param rt The current runtime.
 * \param name The name of the C type.
 * \param model The type model.
 * \param size Size of the type in bytes.
 * \param data The private data of the type.
 * If model == RJS_CTYPE_MODEL_STRUCT, data is the prototype value' pointer.
 * If model == RJS_CTYPE_MODEL_FUNC, data is the native wrapper function's pointer.
 * \param[out] pty Return the C type.
 * \retval RJS_OK On success.
 * \retval RJS_ERR Cannot find the C type.
 */
extern RJS_Result
rjs_create_c_type (RJS_Runtime *rt, RJS_Value *name, RJS_CTypeModel model,
        size_t size, void *data, RJS_CType **pty);

/**
 * Create a C pointer object.
 * \param rt The current runtime.
 * \param ty The C type of the value.
 * \param p The pointer.
 * \param ptype The pointer's type.
 * \param nitem Number of items in the pointer.
 * If ptype == RJS_CPTR_TYPE_VALUE, it must be 1.
 * \param flags Flags of the pointer.
 * \param[out] rv Return the object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_create_c_ptr (RJS_Runtime *rt, RJS_CType *ty, void *p,
        RJS_CPtrType ptype, size_t nitem, int flags, RJS_Value *rv);

#if ENABLE_INT_INDEXED_OBJECT

/**
 * Create a typed array pointed to a C buffer.
 * \param rt The current runtime.
 * \param et The element type.
 * \param p The pointer.
 * \param nitem Number of elements in the buffer.
 * \param[out] rv Return the object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_create_c_typed_array (RJS_Runtime *rt, RJS_ArrayElementType et, void *p,
        size_t nitem, RJS_Value *rv);

#endif /*ENABLE_INT_INDEXED_OBJECT*/

/**
 * Get the C pointer from the value.
 * \param rt The current runtime.
 * \param ty The C type of the value.
 * \param ptype The pointer type.
 * \param cptr The C pointer value.
 * \return The real C pointer.
 * \retval NULL The value is not a valid C pointer.
 */
extern void*
rjs_get_c_ptr (RJS_Runtime *rt, RJS_CType *ty, RJS_CPtrType ptype, RJS_Value *cptr);

/**
 * Get the length of the C array.
 * @param rt The current runtime.
 * @param cptr The C pointer.
 * @return The length of the C array.
 */
extern size_t
rjs_get_c_array_length (RJS_Runtime *rt, RJS_Value *cptr);

#ifdef __cplusplus
}
#endif

#endif

