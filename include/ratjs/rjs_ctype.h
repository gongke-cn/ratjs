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

#include <ffi.h>

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
    RJS_CPTR_TYPE_UINT8_ARRAY  = RJS_ARRAY_ELEMENT_UINT8,  /**< Pointer to a unsigned 8 bits integer buffer.*/
    RJS_CPTR_TYPE_INT8_ARRAY   = RJS_ARRAY_ELEMENT_INT8,   /**< Pointer to a signed 8 bits integer buffer.*/
    RJS_CPTR_TYPE_UINT8C_ARRAY = RJS_ARRAY_ELEMENT_UINT8C, /**< Pointer to a unsigned 8 bits integer buffer. (clamped)*/
    RJS_CPTR_TYPE_INT16_ARRAY  = RJS_ARRAY_ELEMENT_INT16,  /**< Pointer to a signed 16 bits integer buffer.*/
    RJS_CPTR_TYPE_UINT16_ARRAY = RJS_ARRAY_ELEMENT_UINT16, /**< Pointer to a unsigned 16 bits integer buffer.*/
    RJS_CPTR_TYPE_INT32_ARRAY  = RJS_ARRAY_ELEMENT_INT32,  /**< Pointer to a signed 32 bits integer buffer.*/
    RJS_CPTR_TYPE_UINT32_ARRAY = RJS_ARRAY_ELEMENT_UINT32, /**< Pointer to a unsigned 32 bits integer buffer.*/
    RJS_CPTR_TYPE_FLOAT32_ARRAY= RJS_ARRAY_ELEMENT_FLOAT32,/**< Pointer to a 32 bits float point number buffer.*/
    RJS_CPTR_TYPE_FLOAT64_ARRAY= RJS_ARRAY_ELEMENT_FLOAT64,/**< Pointer to a 64 bits float point number buffer.*/
    RJS_CPTR_TYPE_UINT64_ARRAY = RJS_ARRAY_ELEMENT_BIGUINT64, /**< Pointer to a unsigned 64 bits integer buffer.*/
    RJS_CPTR_TYPE_INT64_ARRAY  = RJS_ARRAY_ELEMENT_BIGINT64,  /**< Pointer to a signed 64 bits integer buffer.*/
    RJS_CPTR_TYPE_VALUE,        /**< Pointer to a value.*/
    RJS_CPTR_TYPE_ARRAY,        /**< Pointer to an array.*/
    RJS_CPTR_TYPE_PTR_ARRAY,    /**< Pointer to a pointer array.*/
    RJS_CPTR_TYPE_C_FUNC,       /**< The function pointer.*/
    RJS_CPTR_TYPE_C_WRAPPER,    /**< The function wrapper.*/
    RJS_CPTR_TYPE_UNKNOWN       /**< Unknown pointer type.*/
} RJS_CPtrType;

/**
 * Javascript to FFI function invoker function.
 * \param rt The current runtime.
 * \param cif FFI cif data.
 * \param args The input javascript arguments.
 * \param argc The input javascript arguments' count.
 * \param cptr The C function's pointer.
 * \param[out] rv The return javascript value buffer.
 * \param data The private data of the function type.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error. 
 */
typedef RJS_Result (*RJS_JS2FFIFunc) (RJS_Runtime *rt, ffi_cif *cif, RJS_Value *args, size_t argc,
        void *cptr, RJS_Value *rv, void *data);

/**
 * FFI to javascript function invoker function.
 * \param rt The current runtime.
 * \param args The input FFI arguments buffer.
 * \param nrgs Number of input FFI arguments.
 * \param fn The javascript function value.
 * \param r The return FFI value's pointer.
 * \param data The private data of the function type.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error. 
 */
typedef RJS_Result (*RJS_FFI2JSFunc) (RJS_Runtime *rt, void **args, int nargs, RJS_Value *fn, void *r,
        void *data);

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
 * \param size Size of the type in bytes.
 * \param proto The prototype value.
 * \param[out] pty Return the C type.
 * \retval RJS_OK On success.
 * \retval RJS_ERR Cannot find the C type.
 */
extern RJS_Result
rjs_create_c_type (RJS_Runtime *rt, RJS_Value *name, size_t size,
        RJS_Value *proto, RJS_CType **pty);

/**
 * Create a new C function type.
 * \param rt The current runtime.
 * \param name The name of the C type.
 * \param nargs Number of the function's arguments.
 * \param rtype The return value's type.
 * \param atypes The arguments' types.
 * \param js2ffi Javacript to FFI invoker.
 * \param ffi2js FFI 2 javascript invoker.
 * \param data The private data of the function type.
 * \param[out] pty Return the C type.
 * \retval RJS_OK On success.
 * \retval RJS_ERR Cannot find the C type.
 */
extern RJS_Result
rjs_create_c_func_type (RJS_Runtime *rt, RJS_Value *name, int nargs,
        ffi_type *rtype, ffi_type **atypes, RJS_JS2FFIFunc js2ffi,
        RJS_FFI2JSFunc ffi2js, void *data, RJS_CType **pty);

/**
 * Create a C pointer object.
 * \param rt The current runtime.
 * \param ty The C type of the value.
 * \param p The pointer.
 * \param ptype The pointer's type.
 * \param nitem Number of items in the pointer.
 * If ptype == RJS_CPTR_TYPE_VALUE, it must be 1.
 * \param flags Flags of the pointer.
 * \param[out] rv Return the C pointer object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_create_c_ptr (RJS_Runtime *rt, RJS_CType *ty, void *p,
        RJS_CPtrType ptype, size_t nitem, int flags, RJS_Value *rv);

/**
 * Create a C function pointer object.
 * \param rt The current runtime.
 * \param ty The function's type.
 * \param cp The C function's pointer.
 * \param[out] rv Return the C pointer object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_create_c_func_ptr (RJS_Runtime *rt, RJS_CType *ty, void *cp, RJS_Value *rv);

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
 * Check if the value is a C pointer.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE The value is a C pointer.
 * \retval RJS_FALSE The value is not a C pointer.
 */
extern RJS_Bool
rjs_is_c_ptr (RJS_Runtime *rt, RJS_Value *v);

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

