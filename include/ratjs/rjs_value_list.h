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
 * Value list.
 */

#ifndef _RJS_VALUE_LIST_H_
#define _RJS_VALUE_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup vlist Value list
 * Value list
 * @{
 */

/**Value count in a list segment.*/
#define RJS_VALUE_LIST_SEGMENT_SIZE 64

/**Value list segment.*/
typedef struct {
    RJS_List   ln;  /**< List node data.*/
    size_t     num; /**< Valid value number in the segment.*/
    RJS_Value  v[RJS_VALUE_LIST_SEGMENT_SIZE]; /**< Value buffer.*/
} RJS_ValueListSegment;

/**Value list.*/
typedef struct {
    RJS_GcThing gc_thing; /**< Base GC thing data.*/
    RJS_List    seg_list; /**< Segment list.*/
    size_t      len;      /**< Length of the list.*/
} RJS_ValueList;

/**
 * Create a new value list.
 * \param rt The current runtime.
 * \param[out] v Return value store the list.
 * \retval The value list's pointer.
 */
extern RJS_ValueList*
rjs_value_list_new (RJS_Runtime *rt, RJS_Value *v);

/**
 * Append an item to the value list.
 * \param rt The current runtime.
 * \param vl The value list.
 * \param i The item to be added.
 */
extern void
rjs_value_list_append (RJS_Runtime *rt, RJS_ValueList *vl, RJS_Value *i);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

