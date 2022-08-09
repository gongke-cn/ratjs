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

/*Scan the referenced things in the script class.*/
static void
script_class_gc_scan (RJS_Runtime *rt, RJS_ScriptClass *c)
{
    size_t i;

#if ENABLE_PRIV_NAME
    for (i = 0; i < c->priv_method_num; i ++) {
        RJS_ScriptMethod *method = &c->priv_methods[i];

        rjs_gc_scan_value(rt, &method->name);
        rjs_gc_scan_value(rt, &method->value);
    }
#endif /*ENABLE_PRIV_NAME*/

    for (i = 0; i < c->field_num; i ++) {
        RJS_ScriptField *field = &c->fields[i];

        rjs_gc_scan_value(rt, &field->name);
        rjs_gc_scan_value(rt, &field->init);
    }
}

/*Free the script class.*/
static void
script_class_gc_free (RJS_Runtime *rt, RJS_ScriptClass *c)
{
#if ENABLE_PRIV_NAME
    if (c->priv_methods)
        RJS_DEL_N(rt, c->priv_methods, c->priv_method_num);
#endif /*ENABLE_PRIV_NAME*/

    if (c->fields)
        RJS_DEL_N(rt, c->fields, c->field_num);

    RJS_DEL(rt, c);
}

/**
 * Scan referenced things in the base function object.
 * \param rt The current runtime.
 * \param bfo The base function object.
 */
void
rjs_base_func_object_op_gc_scan (RJS_Runtime *rt, RJS_BaseFuncObject *bfo)
{
    rjs_object_op_gc_scan(rt, bfo);

    if (bfo->clazz)
        script_class_gc_scan(rt, bfo->clazz);
}

/**
 * Release the base function object.
 * \param rt The current runtime.
 * \param bfo The base function object to be released.
 */
void
rjs_base_func_object_deinit (RJS_Runtime *rt, RJS_BaseFuncObject *bfo)
{
    if (bfo->clazz)
        script_class_gc_free(rt, bfo->clazz);

    rjs_object_deinit(rt, &bfo->object);
}

/**
 * Initialize the instance's elements.
 * \param rt The current runtime.
 * \param o The instance object.
 * \param f The constructor function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_initialize_instance_elements (RJS_Runtime *rt, RJS_Value *o, RJS_Value *f)
{
    RJS_BaseFuncObject *bfo;
    RJS_GcThingType     gtt;
    RJS_Result          r = RJS_OK;

    gtt = rjs_value_get_gc_thing_type(rt, f);
    assert((gtt == RJS_GC_THING_SCRIPT_FUNC) || (gtt == RJS_GC_THING_BUILTIN_FUNC));

    bfo = (RJS_BaseFuncObject*)rjs_value_get_object(rt, f);

    if (bfo->clazz) {
        RJS_ScriptClass *clazz = bfo->clazz;
        size_t           i;

#if ENABLE_PRIV_NAME
        for (i = 0; i < clazz->priv_method_num; i ++) {
            RJS_ScriptMethod *method = &clazz->priv_methods[i];

            switch (method->type) {
            case RJS_SCRIPT_CLASS_ELEMENT_METHOD:
                r = rjs_private_method_add(rt, o, &method->name, &method->value);
                break;
            case RJS_SCRIPT_CLASS_ELEMENT_GET:
                r = rjs_private_accessor_add(rt, o, &method->name, &method->value, NULL);
                break;
            case RJS_SCRIPT_CLASS_ELEMENT_SET:
                r = rjs_private_accessor_add(rt, o, &method->name, NULL, &method->value);
                break;
            default:
                assert(0);
                r = RJS_ERR;
            }

            if (r == RJS_ERR)
                return r;
        }
#endif /*ENABLE_PRIV_NAME*/

        for (i = 0; i < clazz->field_num; i ++) {
            RJS_ScriptField *field = &clazz->fields[i];

            if ((r = rjs_define_field(rt, o, &field->name, &field->init, field->is_af)) == RJS_ERR)
                return r;
        }
    }

   return RJS_OK;
}
