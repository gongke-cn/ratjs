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

#include "ratjs_internal.h"

/*Scan the referenced things in the value list.*/
static void
value_list_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_ValueList        *vl = ptr;
    RJS_ValueListSegment *vls;

    rjs_list_foreach_c(&vl->seg_list, vls, RJS_ValueListSegment, ln) {
        rjs_gc_scan_value_buffer(rt, vls->v, vls->num);
    }
}

/*Free the value list.*/
static void
value_list_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_ValueList        *vl = ptr;
    RJS_ValueListSegment *vls, *nvls;

    rjs_list_foreach_safe_c(&vl->seg_list, vls, nvls, RJS_ValueListSegment, ln) {
        RJS_DEL(rt, vls);
    }

    RJS_DEL(rt, vl);
}

/*Value list operation functions.*/
static const RJS_GcThingOps
value_list_ops = {
    RJS_GC_THING_VALUE_LIST,
    value_list_op_gc_scan,
    value_list_op_gc_free
};

/**
 * Create a new value list.
 * \param rt The current runtime.
 * \param[out] v Return value store the list.
 * \retval The value list's pointer.
 */
RJS_ValueList*
rjs_value_list_new (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_ValueList *vl;

    RJS_NEW(rt, vl);

    vl->len = 0;

    rjs_list_init(&vl->seg_list);

    rjs_value_set_gc_thing(rt, v, vl);
    rjs_gc_add(rt, vl, &value_list_ops);

    return vl;
}

/*Append an item to the value list.*/
void
rjs_value_list_append (RJS_Runtime *rt, RJS_ValueList *vl, RJS_Value *i)
{
    RJS_ValueListSegment *vls;

    if (rjs_list_is_empty(&vl->seg_list)) {
        vls = NULL;
    } else {
        vls = RJS_CONTAINER_OF(vl->seg_list.prev, RJS_ValueListSegment, ln);

        if (vls->num == RJS_VALUE_LIST_SEGMENT_SIZE)
            vls = NULL;
    }

    if (vls == NULL) {
        RJS_NEW(rt, vls);

        vls->num = 0;

        rjs_list_append(&vl->seg_list, &vls->ln);
    }

    rjs_value_copy(rt, vls->v + vls->num, i);
    vls->num ++;
    
    vl->len ++;
}
