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
 * Native data internal header.
 */

#ifndef _RJS_NATIVE_DATA_INTERNAL_H_
#define _RJS_NATIVE_DATA_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**The native data.*/
typedef struct {
    void        *data; /**< Pointer.*/
    RJS_ScanFunc scan; /**< Scan the reference things in the data.*/
    RJS_FreeFunc free; /**< Free the data.*/
} RJS_NativeData;

/**
 * Initialize the native data record.
 * \param nd The native data record to be initialized.
 */
static inline void
rjs_native_data_init (RJS_NativeData *nd)
{
    nd->data = NULL;
    nd->scan = NULL;
    nd->free = NULL;
}

/**
 * Set the native data record.
 * \param nd The native data record to be set.
 * \param data The data's pointer.
 * \param scan The function scan all the referenced things in the native data.
 * \param free The function free the native data.
 */
static inline void
rjs_native_data_set (RJS_NativeData *nd, void *data, RJS_ScanFunc scan, RJS_FreeFunc free)
{
    nd->data = data;
    nd->scan = scan;
    nd->free = free;
}

/**
 * Scan the referenced things in the native data.
 * \param rt The current runtime.
 * \param nd The native data record to be scanned.
 */
static inline void
rjs_native_data_scan (RJS_Runtime *rt, RJS_NativeData *nd)
{
    if (nd->scan)
        nd->scan(rt, nd->data);
}

/**
 * Free the native data.
 * \param rt The current runtime.
 * \param nd The native data record to be freed.
 */
static inline void
rjs_native_data_free (RJS_Runtime *rt, RJS_NativeData *nd)
{
    if (nd->free)
        nd->free(rt, nd->data);
}

#ifdef __cplusplus
}
#endif

#endif

