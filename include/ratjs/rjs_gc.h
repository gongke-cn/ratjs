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
 * Garbage collector.
 */

#ifndef _RJS_GC_H_
#define _RJS_GC_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup gc Garbage collector
 * Garbage collector 
 * @{
 */

/**The thing is marked as used.*/
#define RJS_GC_THING_FL_MARKED  1
/**The thing is scaned by GC.*/
#define RJS_GC_THING_FL_SCANNED 2

/**
 * Mark the thing as used.
 * This function only can be used in GC collection process.
 * \param rt The current runtime.
 * \param thing The thing to be marked.
 */
static inline void
rjs_gc_mark (RJS_Runtime *rt, void *thing)
{
    RJS_RuntimeBase *rb = (RJS_RuntimeBase*)rt;
    RJS_GcThing     *gt = thing;

    if (!(gt->next_flags & RJS_GC_THING_FL_MARKED)) {
        gt->next_flags |= RJS_GC_THING_FL_MARKED;

        if (rb->gc_mark_stack.item_cap > rb->gc_mark_stack.item_num) {
            rb->gc_mark_stack.items[rb->gc_mark_stack.item_num ++] = gt;
        } else {
            rb->gc_mark_stack_full = RJS_TRUE;
        }
    }
}

/**
 * Add a thing to the rt. Then the thing is managed by the GC.
 * \param rt The current runtime.
 * \param thing The thing to be added.
 * \param ops The thing's operation functions.
 */
extern void
rjs_gc_add (RJS_Runtime *rt, void *thing, const RJS_GcThingOps *ops);

/**
 * Run the GC process.
 * \param rt The current runtime.
 */
extern void
rjs_gc_run (RJS_Runtime *rt);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

