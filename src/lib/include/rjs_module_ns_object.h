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
 * Module namespace object internal header.
 */

#ifndef _RJS_MODULE_NS_OBJECT_INTERNAL_H_
#define _RJS_MODULE_NS_OBJECT_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Module export entry.*/
typedef struct {
    RJS_HashEntry he;   /**< Hash table entry.*/
    RJS_Value     name; /**< Export name.*/
} RJS_ModuleExport;

/**Module namespace object.*/
typedef struct {
    RJS_Object        object;      /**< Base object data.*/
    RJS_Value         module;      /**< The module.*/
    RJS_Hash          export_hash; /**< Export entries hash table.*/
    RJS_ModuleExport *exports;     /**< Export entries.*/
    size_t            export_num;  /**< Export entries' number.*/
} RJS_ModuleNsObject;

/**
 * Create a module namespace object.
 * \param rt The current runtime.
 * \param[out] v Return the module namespace object.
 * \param mod The module.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_module_ns_object_new (RJS_Runtime *rt, RJS_Value *v, RJS_Value *mod);

#ifdef __cplusplus
}
#endif

#endif

