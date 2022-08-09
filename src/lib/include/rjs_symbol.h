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
 * Symbol internal header.
 */

#ifndef _RJS_SYMBOL_INTERNAL_H_
#define _RJS_SYMBOL_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Symbol registry.*/
typedef struct {
    RJS_HashEntry key_he; /**< Key hash entry.*/
    RJS_HashEntry sym_he; /**< Symbol hash entry.*/
    RJS_Value     key;    /**< Key string value.*/
    RJS_Value     symbol; /**< Symbol value.*/
} RJS_SymbolRegistry;

/**
 * Initialize the symbol registry.
 * \param rt The current runtime.
 */
extern void
rjs_runtime_symbol_registry_init (RJS_Runtime *rt);

/**
 * Release the symbol registry.
 * \param rt The current runtime.
 */
extern void
rjs_runtime_symbol_registry_deinit (RJS_Runtime *rt);

/**
 * Scan the referenced things in the symbol registry.
 * \param rt The current runtime.
 */
extern void
rjs_gc_scan_symbol_registry (RJS_Runtime *rt);

/**
 * Solve the weak references in the rt.
 * \param rt The current runtime.
 */
extern void
rjs_solve_weak_refs (RJS_Runtime *rt);

/**
 * Initialize the weak reference data in the rt.
 * \param rt The current runtime.
 */
extern void
rjs_runtime_weak_ref_init (RJS_Runtime *rt);

/**
 * Release the weak reference data in the rt.
 * \param rt The current runtime.
 */
extern void
rjs_runtime_weak_ref_deinit (RJS_Runtime *rt);

#ifdef __cplusplus
}
#endif

#endif

