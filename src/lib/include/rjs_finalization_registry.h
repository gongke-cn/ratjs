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
 * Finalization registry internal header.
 */

#ifndef _RJS_FINALIZATION_REGISTRY_INTERNAL_H_
#define _RJS_FINALIZATION_REGISTRY_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Finalization registry.*/
typedef struct {
    RJS_Object  object;  /**< The base object data.*/
    RJS_Hash    cb_hash; /**< The callback hash table.*/
    RJS_Value   func;    /**< The callback function.*/
} RJS_FinalizationRegistry;

/**Finalization callback.*/
typedef struct {
    RJS_HashEntry he;       /**< Hash table entry.*/
    RJS_List      ln;       /**< List node data.*/
    RJS_Value     registry; /**< The registry.*/
    RJS_Value     target;   /**< The target value.*/
    RJS_Value     held;     /**< Held value.*/
    RJS_Value     token;    /**< Token value.*/
} RJS_FinalizationCallback;

/**
 * Initialize the finalization registry data in the rt.
 * \param rt The current runtime.
 */
RJS_INTERNAL void
rjs_runtime_finalization_registry_init (RJS_Runtime *rt);

/**
 * Release tje finalization registry data in the rt.
 * \param rt The current runtime.
 */
RJS_INTERNAL void
rjs_runtime_finalization_registry_deinint (RJS_Runtime *rt);

/**
 * Scan the referenced things in the finalization registry data.
 * \param rt The current runtime.
 */
RJS_INTERNAL void
rjs_gc_scan_finalization_registry (RJS_Runtime *rt);

/**
 * Solve the callback functions in the finalization registry.
 * \param rt The current runtime.
 */
RJS_INTERNAL void
rjs_solve_finalization_registry (RJS_Runtime *rt);

#ifdef __cplusplus
}
#endif

#endif

