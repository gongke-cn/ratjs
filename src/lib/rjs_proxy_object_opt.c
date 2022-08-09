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

/*Scan the referenced things in the proxy object.*/
static void
proxy_object_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_ProxyObject *po = ptr;

    rjs_object_op_gc_scan(rt, ptr);

    rjs_gc_scan_value(rt, &po->target);
    rjs_gc_scan_value(rt, &po->handler);
}

/*Free the proxy object.*/
static void
proxy_object_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_ProxyObject *po = ptr;

    rjs_object_deinit(rt, &po->object);

    RJS_DEL(rt, po);
}

/*Get the proxy object's prototype.*/
static RJS_Result
proxy_object_op_get_prototype_of (RJS_Runtime *rt, RJS_Value *o, RJS_Value *proto)
{
    RJS_ProxyObject *po      = (RJS_ProxyObject*)rjs_value_get_object(rt, o);
    size_t           top     = rjs_value_stack_save(rt);
    RJS_Value       *target  = rjs_value_stack_push(rt);
    RJS_Value       *handler = rjs_value_stack_push(rt);
    RJS_Value       *trap    = rjs_value_stack_push(rt);
    RJS_Value       *tmp     = rjs_value_stack_push(rt);
    RJS_Result       r;

    rjs_value_copy(rt, target, &po->target);
    rjs_value_copy(rt, handler, &po->handler);

    if (rjs_value_is_null(rt, target)) {
        r = rjs_throw_type_error(rt, _("proxy target is null"));
        goto end;
    }

    if ((r = rjs_get_method(rt, handler, rjs_pn_getPrototypeOf(rt), trap)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, trap)) {
        r = rjs_object_get_prototype_of(rt, target, proto);
        goto end;
    }

    if ((r = rjs_call(rt, trap, handler, target, 1, proto)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_null(rt, proto) && !rjs_value_is_object(rt, proto)) {
        r = rjs_throw_type_error(rt, _("result of \"getPrototypeOf\" is neither object nor null"));
        goto end;
    }

    if (rjs_object_is_extensible(rt, target)) {
        r = RJS_OK;
        goto end;
    }

    if ((r = rjs_object_get_prototype_of(rt, target, tmp)) == RJS_ERR)
        goto end;

    if (!rjs_same_value(rt, proto, tmp)) {
        r = rjs_throw_type_error(rt, _("proxy result mismatch"));
        goto end;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Set the proxy object's prototype*/
static RJS_Result
proxy_object_op_set_prototype_of (RJS_Runtime *rt, RJS_Value *v, RJS_Value *proto)
{
    RJS_ProxyObject *po      = (RJS_ProxyObject*)rjs_value_get_object(rt, v);
    size_t           top     = rjs_value_stack_save(rt);
    RJS_Value       *trap    = rjs_value_stack_push(rt);
    RJS_Value       *target  = rjs_value_stack_push(rt);
    RJS_Value       *protoa  = rjs_value_stack_push(rt);
    RJS_Value       *handler = rjs_value_stack_push(rt);
    RJS_Value       *tr      = rjs_value_stack_push(rt);
    RJS_Result       r;

    rjs_value_copy(rt, target, &po->target);
    rjs_value_copy(rt, handler, &po->handler);

    if (rjs_value_is_null(rt, target)) {
        r = rjs_throw_type_error(rt, _("proxy target is null"));
        goto end;
    }

    if ((r = rjs_get_method(rt, handler, rjs_pn_setPrototypeOf(rt), trap)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, trap)) {
        r = rjs_object_set_prototype_of(rt, target, proto);
        goto end;
    }

    rjs_value_copy(rt, protoa, proto);

    if ((r = rjs_call(rt, trap, handler, target, 2, tr)) == RJS_ERR)
        goto end;

    if (!rjs_to_boolean(rt, tr)) {
        r = RJS_FALSE;
        goto end;
    }

    if ((r = rjs_object_is_extensible(rt, target)) != RJS_FALSE)
        goto end;

    if ((r = rjs_object_get_prototype_of(rt, target, protoa)) == RJS_ERR)
        goto end;

    if (!rjs_same_value(rt, proto, protoa)) {
        r = rjs_throw_type_error(rt, _("proxy result mismatch"));
        goto end;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Check if the proxy object is extensible.*/
static RJS_Result
proxy_object_op_is_extensible (RJS_Runtime *rt, RJS_Value *o)
{
    RJS_ProxyObject *po      = (RJS_ProxyObject*)rjs_value_get_object(rt, o);
    size_t           top     = rjs_value_stack_save(rt);
    RJS_Value       *target  = rjs_value_stack_push(rt);
    RJS_Value       *handler = rjs_value_stack_push(rt);
    RJS_Value       *trap    = rjs_value_stack_push(rt);
    RJS_Value       *tmp     = rjs_value_stack_push(rt);
    RJS_Bool         b1, b2;
    RJS_Result       r;

    rjs_value_copy(rt, target, &po->target);
    rjs_value_copy(rt, handler, &po->handler);

    if (rjs_value_is_null(rt, target)) {
        r = rjs_throw_type_error(rt, _("proxy target is null"));
        goto end;
    }

    if ((r = rjs_get_method(rt, handler, rjs_pn_isExtensible(rt), trap)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, trap)) {
        r = rjs_object_is_extensible(rt, target);
        goto end;
    }

    if ((r = rjs_call(rt, trap, handler, target, 1, tmp)) == RJS_ERR)
        goto end;

    b1 = rjs_to_boolean(rt, tmp);
    b2 = rjs_object_is_extensible(rt, target);

    if (b1 != b2) {
        r = rjs_throw_type_error(rt, _("proxy result mismatch"));
        goto end;
    }

    r = b1;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Prevent extensions of the proxy object*/
static RJS_Result
proxy_object_op_prevent_extensions (RJS_Runtime *rt, RJS_Value *o)
{
    RJS_ProxyObject *po      = (RJS_ProxyObject*)rjs_value_get_object(rt, o);
    size_t           top     = rjs_value_stack_save(rt);
    RJS_Value       *target  = rjs_value_stack_push(rt);
    RJS_Value       *handler = rjs_value_stack_push(rt);
    RJS_Value       *trap    = rjs_value_stack_push(rt);
    RJS_Value       *tmp     = rjs_value_stack_push(rt);
    RJS_Bool         b;
    RJS_Result       r;

    rjs_value_copy(rt, target, &po->target);
    rjs_value_copy(rt, handler, &po->handler);

    if (rjs_value_is_null(rt, target)) {
        r = rjs_throw_type_error(rt, _("proxy target is null"));
        goto end;
    }

    if ((r = rjs_get_method(rt, handler, rjs_pn_preventExtensions(rt), trap)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, trap)) {
        r = rjs_object_prevent_extensions(rt, target);
        goto end;
    }

    if ((r = rjs_call(rt, trap, handler, target, 1, tmp)) == RJS_ERR)
        goto end;

    b = rjs_to_boolean(rt, tmp);
    if (b) {
        if ((r = rjs_object_is_extensible(rt, target)) == RJS_ERR)
            goto end;
        if (r) {
            r = rjs_throw_type_error(rt, _("proxy result mismatch"));
            goto end;
        }
    }

    r = b;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Get the proxy object's own property.*/
static RJS_Result
proxy_object_op_get_own_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_PropertyDesc *pd)
{
    RJS_ProxyObject *po      = (RJS_ProxyObject*)rjs_value_get_object(rt, o);
    size_t           top     = rjs_value_stack_save(rt);
    RJS_Value       *trap    = rjs_value_stack_push(rt);
    RJS_Value       *target  = rjs_value_stack_push(rt);
    RJS_Value       *name    = rjs_value_stack_push(rt);
    RJS_Value       *handler = rjs_value_stack_push(rt);
    RJS_Value       *tr      = rjs_value_stack_push(rt);
    RJS_PropertyDesc target_pd, *tpd = &target_pd;
    RJS_Bool         tpd_init = RJS_FALSE;
    RJS_Result       r;
    RJS_Bool         is_ext;

    rjs_value_copy(rt, target, &po->target);
    rjs_value_copy(rt, handler, &po->handler);

    if (rjs_value_is_null(rt, target)) {
        r = rjs_throw_type_error(rt, _("proxy target is null"));
        goto end;
    }

    if ((r = rjs_get_method(rt, handler, rjs_pn_getOwnPropertyDescriptor(rt), trap)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, trap)) {
        r = rjs_object_get_own_property(rt, target, pn, pd);
        goto end;
    }

    rjs_value_copy(rt, name, pn->name);

    if ((r = rjs_call(rt, trap, handler, target, 2, tr)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_undefined(rt, tr) && !rjs_value_is_object(rt, tr)) {
        r = rjs_throw_type_error(rt, _("result of \"getOwnPropertyDescriptor\" is neither object nor undefined"));
        goto end;
    }

    rjs_property_desc_init(rt, tpd);
    tpd_init = RJS_TRUE;

    if ((r = rjs_object_get_own_property(rt, target, pn, tpd)) == RJS_ERR)
        goto end;

    if (!r)
        tpd = NULL;

    if (rjs_value_is_undefined(rt, tr)) {
        if (!tpd) {
            r = RJS_FALSE;
            goto end;
        }

        if (!(tpd->flags & RJS_PROP_FL_CONFIGURABLE)) {
            r = rjs_throw_type_error(rt, _("the property is not configurable"));
            goto end;
        }

        if ((r = rjs_object_is_extensible(rt, target)) == RJS_ERR)
            goto end;

        if (!r) {
            r = rjs_throw_type_error(rt, _("the object is not extensible"));
            goto end;
        }

        r = RJS_FALSE;
        goto end;
    }

    if ((r = rjs_object_is_extensible(rt, target)) == RJS_ERR)
        goto end;
    is_ext = r;

    if ((r = rjs_to_property_descriptor(rt, tr, pd)) == RJS_ERR)
        goto end;

    rjs_complete_property_descriptor(rt, pd);

    if ((r = rjs_is_compatible_property_descriptor(rt, is_ext, pd, tpd)) == RJS_ERR)
        goto end;
    if (!r) {
        r = rjs_throw_type_error(rt, _("proxy result mismatch"));
        goto end;
    }

    if (!(pd->flags & RJS_PROP_FL_CONFIGURABLE)) {
        if (!tpd || (tpd->flags & RJS_PROP_FL_CONFIGURABLE)) {
            r = rjs_throw_type_error(rt, _("proxy result mismatch"));
            goto end;
        }

        if (!(pd->flags & RJS_PROP_FL_WRITABLE)) {
            if (tpd->flags & RJS_PROP_FL_WRITABLE) {
                r = rjs_throw_type_error(rt, _("proxy result mismatch"));
                goto end;
            }
        }
    }

    r = RJS_TRUE;
end:
    if (tpd_init)
        rjs_property_desc_deinit(rt, &target_pd);

    rjs_value_stack_restore(rt, top);
    return r;
}

/*Define the own property of the proxy object.*/
static RJS_Result
proxy_object_op_define_own_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_PropertyDesc *pd)
{
    RJS_ProxyObject *po      = (RJS_ProxyObject*)rjs_value_get_object(rt, o);
    size_t           top     = rjs_value_stack_save(rt);
    RJS_Value       *trap    = rjs_value_stack_push(rt);
    RJS_Value       *target  = rjs_value_stack_push(rt);
    RJS_Value       *name    = rjs_value_stack_push(rt);
    RJS_Value       *pdo     = rjs_value_stack_push(rt);
    RJS_Value       *handler = rjs_value_stack_push(rt);
    RJS_Value       *tr      = rjs_value_stack_push(rt);
    RJS_PropertyDesc target_pd, *tpd = &target_pd;
    RJS_Bool         tpd_init = RJS_FALSE;
    RJS_Result       r;
    RJS_Bool         is_ext, set_cfg_false;

    rjs_value_copy(rt, target, &po->target);
    rjs_value_copy(rt, handler, &po->handler);

    if (rjs_value_is_null(rt, target)) {
        r = rjs_throw_type_error(rt, _("proxy target is null"));
        goto end;
    }

    if ((r = rjs_get_method(rt, handler, rjs_pn_defineProperty(rt), trap)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, trap)) {
        r = rjs_object_define_own_property(rt, target, pn, pd);
        goto end;
    }

    rjs_value_copy(rt, name, pn->name);

    if ((r = rjs_from_property_descriptor(rt, pd, pdo)) == RJS_ERR)
        goto end;

    if ((r = rjs_call(rt, trap, handler, target, 3, tr)) == RJS_ERR)
        goto end;

    if (!(r = rjs_to_boolean(rt, tr)))
        goto end;

    rjs_property_desc_init(rt, tpd);
    tpd_init = RJS_TRUE;

    if ((r = rjs_object_get_own_property(rt, target, pn, tpd)) == RJS_ERR)
        goto end;
    if (!r)
        tpd = NULL;

    if ((r = rjs_object_is_extensible(rt, target)) == RJS_ERR)
        goto end;
    is_ext = r;

    if ((pd->flags & RJS_PROP_FL_HAS_CONFIGURABLE) && !(pd->flags & RJS_PROP_FL_CONFIGURABLE))
        set_cfg_false = RJS_TRUE;
    else
        set_cfg_false = RJS_FALSE;

    if (!tpd) {
        if (!is_ext || set_cfg_false) {
            r = rjs_throw_type_error(rt, _("proxy result mismatch"));
            goto end;
        }
    } else {
        if (!rjs_is_compatible_property_descriptor(rt, is_ext, pd, tpd)) {
            r = rjs_throw_type_error(rt, _("proxy result mismatch"));
            goto end;
        }

        if (set_cfg_false && (tpd->flags & RJS_PROP_FL_CONFIGURABLE)) {
            r = rjs_throw_type_error(rt, _("proxy result mismatch"));
            goto end;
        }

        if (rjs_is_data_descriptor(tpd)
                && !(tpd->flags & RJS_PROP_FL_CONFIGURABLE)
                && (tpd->flags & RJS_PROP_FL_WRITABLE)) {
            if ((pd->flags & RJS_PROP_FL_HAS_WRITABLE) && !(pd->flags & RJS_PROP_FL_WRITABLE)) {
                r = rjs_throw_type_error(rt, _("proxy result mismatch"));
                goto end;
            }
        }
    }

    r = RJS_TRUE;
end:
    if (tpd_init)
        rjs_property_desc_deinit(rt, &target_pd);

    rjs_value_stack_restore(rt, top);
    return r;
}

/*Check if the proxy object has the property.*/
static RJS_Result
proxy_object_op_has_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn)
{
    RJS_ProxyObject *po      = (RJS_ProxyObject*)rjs_value_get_object(rt, o);
    size_t           top     = rjs_value_stack_save(rt);
    RJS_Value       *trap    = rjs_value_stack_push(rt);
    RJS_Value       *target  = rjs_value_stack_push(rt);
    RJS_Value       *name    = rjs_value_stack_push(rt);
    RJS_Value       *handler = rjs_value_stack_push(rt);
    RJS_Value       *tr      = rjs_value_stack_push(rt);
    RJS_PropertyDesc target_pd;
    RJS_Bool         b;
    RJS_Result       r;

    rjs_value_copy(rt, target, &po->target);
    rjs_value_copy(rt, handler, &po->handler);

    if (rjs_value_is_null(rt, target)) {
        r = rjs_throw_type_error(rt, _("proxy target is null"));
        goto end;
    }

    if ((r = rjs_get_method(rt, handler, rjs_pn_has(rt), trap)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, trap)) {
        r = rjs_object_has_property(rt, target, pn);
        goto end;
    }

    rjs_value_copy(rt, name, pn->name);

    if ((r = rjs_call(rt, trap, handler, target, 2, tr)) == RJS_ERR)
        goto end;

    b = rjs_to_boolean(rt, tr);
    if (!b) {
        rjs_property_desc_init(rt, &target_pd);

        if ((r = rjs_object_get_own_property(rt, target, pn, &target_pd)) == RJS_ERR)
            goto end1;
        if (r) {
            if (!(target_pd.flags & RJS_PROP_FL_CONFIGURABLE)
                    || !rjs_object_is_extensible(rt, target)) {
                r = rjs_throw_type_error(rt, _("proxy result mismatch"));
                goto end1;
            }
        }
end1:
        rjs_property_desc_deinit(rt, &target_pd);
        if (r == RJS_ERR)
            goto end;
    }

    r = b;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Get the proxy object's property value.*/
static RJS_Result
proxy_object_op_get (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *receiver, RJS_Value *pv)
{
    RJS_ProxyObject *po      = (RJS_ProxyObject*)rjs_value_get_object(rt, o);
    size_t           top     = rjs_value_stack_save(rt);
    RJS_Value       *trap    = rjs_value_stack_push(rt);
    RJS_Value       *target  = rjs_value_stack_push(rt);
    RJS_Value       *name    = rjs_value_stack_push(rt);
    RJS_Value       *rec     = rjs_value_stack_push(rt);
    RJS_Value       *handler = rjs_value_stack_push(rt);
    RJS_PropertyDesc target_pd, *tpd = &target_pd;
    RJS_Bool         tpd_init = RJS_FALSE;
    RJS_Result       r;

    rjs_value_copy(rt, target, &po->target);
    rjs_value_copy(rt, handler, &po->handler);

    if (rjs_value_is_null(rt, target)) {
        r = rjs_throw_type_error(rt, _("proxy target is null"));
        goto end;
    }

    if ((r = rjs_get_method(rt, handler, rjs_pn_get(rt), trap)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, trap)) {
        r = rjs_object_get(rt, target, pn, receiver, pv);
        goto end;
    }

    rjs_value_copy(rt, name, pn->name);
    rjs_value_copy(rt, rec, receiver);

    if ((r = rjs_call(rt, trap, handler, target, 3, pv)) == RJS_ERR)
        goto end;

    rjs_property_desc_init(rt, tpd);
    tpd_init = RJS_TRUE;

    if ((r = rjs_object_get_own_property(rt, target, pn, tpd)) == RJS_ERR)
        goto end;
    if (!r)
        tpd = NULL;

    if (tpd && !(tpd->flags & RJS_PROP_FL_CONFIGURABLE)) {
        if (rjs_is_data_descriptor(tpd) && !(tpd->flags & RJS_PROP_FL_WRITABLE)) {
            if (!rjs_same_value(rt, pv, tpd->value)) {
                r = rjs_throw_type_error(rt, _("proxy result mismatch"));
                goto end;
            }
        }

        if (rjs_is_accessor_descriptor(tpd) && rjs_value_is_undefined(rt, tpd->get)) {
            if (!rjs_value_is_undefined(rt, pv)) {
                r = rjs_throw_type_error(rt, _("proxy result mismatch"));
                goto end;
            }
        }
    }

    r = RJS_OK;
end:
    if (tpd_init)
        rjs_property_desc_deinit(rt, &target_pd);

    rjs_value_stack_restore(rt, top);
    return r;
}

/*Set the proxy object's property value.*/
static RJS_Result
proxy_object_op_set (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *pv, RJS_Value *receiver)
{
    RJS_ProxyObject *po      = (RJS_ProxyObject*)rjs_value_get_object(rt, o);
    size_t           top     = rjs_value_stack_save(rt);
    RJS_Value       *trap    = rjs_value_stack_push(rt);
    RJS_Value       *target  = rjs_value_stack_push(rt);
    RJS_Value       *name    = rjs_value_stack_push(rt);
    RJS_Value       *value   = rjs_value_stack_push(rt);
    RJS_Value       *rec     = rjs_value_stack_push(rt);
    RJS_Value       *handler = rjs_value_stack_push(rt);
    RJS_Value       *tr      = rjs_value_stack_push(rt);
    RJS_PropertyDesc target_pd, *tpd = &target_pd;
    RJS_Bool         tpd_init = RJS_TRUE;
    RJS_Bool         b;
    RJS_Result       r;

    rjs_value_copy(rt, target, &po->target);
    rjs_value_copy(rt, handler, &po->handler);

    if (rjs_value_is_null(rt, target)) {
        r = rjs_throw_type_error(rt, _("proxy target is null"));
        goto end;
    }

    if ((r = rjs_get_method(rt, handler, rjs_pn_set(rt), trap)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, trap)) {
        r = rjs_object_set(rt, target, pn, pv, receiver);
        goto end;
    }

    rjs_value_copy(rt, name, pn->name);
    rjs_value_copy(rt, value, pv);
    rjs_value_copy(rt, rec, receiver);

    if ((r = rjs_call(rt, trap, handler, target, 4, tr)) == RJS_ERR)
        goto end;

    b = rjs_to_boolean(rt, tr);
    if (!b) {
        r = b;
        goto end;
    }

    rjs_property_desc_init(rt, tpd);
    tpd_init = RJS_TRUE;

    if ((r = rjs_object_get_own_property(rt, target, pn, tpd)) == RJS_ERR)
        goto end;
    if (!r)
        tpd = NULL;

    if (tpd && !(tpd->flags & RJS_PROP_FL_CONFIGURABLE)) {
        if (rjs_is_data_descriptor(tpd) && !(tpd->flags & RJS_PROP_FL_WRITABLE)) {
            if (!rjs_same_value(rt, pv, tpd->value)) {
                r = rjs_throw_type_error(rt, _("proxy result mismatch"));
                goto end;
            }
        }

        if (rjs_is_accessor_descriptor(tpd)) {
            if (rjs_value_is_undefined(rt, tpd->set)) {
                r = rjs_throw_type_error(rt, _("proxy result mismatch"));
                goto end;
            }
        }
    }

    r = RJS_TRUE;
end:
    if (tpd_init)
        rjs_property_desc_deinit(rt, &target_pd);

    rjs_value_stack_restore(rt, top);
    return r;
}

/*Delete the property of the proxy object.*/
static RJS_Result
proxy_object_op_delete (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn)
{
    RJS_ProxyObject *po      = (RJS_ProxyObject*)rjs_value_get_object(rt, o);
    size_t           top     = rjs_value_stack_save(rt);
    RJS_Value       *trap    = rjs_value_stack_push(rt);
    RJS_Value       *target  = rjs_value_stack_push(rt);
    RJS_Value       *name    = rjs_value_stack_push(rt);
    RJS_Value       *handler = rjs_value_stack_push(rt);
    RJS_Value       *tr      = rjs_value_stack_push(rt);
    RJS_PropertyDesc target_pd, *tpd = &target_pd;
    RJS_Bool         tpd_init = RJS_FALSE;
    RJS_Bool         b;
    RJS_Result       r;

    rjs_value_copy(rt, target, &po->target);
    rjs_value_copy(rt, handler, &po->handler);

    if (rjs_value_is_null(rt, target)) {
        r = rjs_throw_type_error(rt, _("proxy target is null"));
        goto end;
    }

    if ((r = rjs_get_method(rt, handler, rjs_pn_deleteProperty(rt), trap)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, trap)) {
        r = rjs_object_delete(rt, target, pn);
        goto end;
    }

    rjs_value_copy(rt, name, pn->name);

    if ((r = rjs_call(rt, trap, handler, target, 2, tr)) == RJS_ERR)
        goto end;

    b = rjs_to_boolean(rt, tr);
    if (!b) {
        r = b;
        goto end;
    }

    rjs_property_desc_init(rt, tpd);
    tpd_init = RJS_TRUE;

    if ((r = rjs_object_get_own_property(rt, target, pn, tpd)) == RJS_ERR)
        goto end;
    if (!r) {
        r = RJS_TRUE;
        goto end;
    }

    if (!(tpd->flags & RJS_PROP_FL_CONFIGURABLE)
            || !rjs_object_is_extensible(rt, target)) {
        r = rjs_throw_type_error(rt, _("proxy result mismatch"));
        goto end;
    }

    r = RJS_TRUE;
end:
    if (tpd_init)
        rjs_property_desc_deinit(rt, &target_pd);

    rjs_value_stack_restore(rt, top);
    return r;
}

/*Get the property key value.*/
static void*
get_prop_key (RJS_Runtime *rt, RJS_Value *v)
{
    void *k;

    if (rjs_value_is_string(rt, v)) {
        rjs_string_to_property_key(rt, v);
        k = rjs_value_get_string(rt, v);
    } else {
        k = rjs_value_get_symbol(rt, v);
    }

    return k;
}

/*Create property keys list from an object.*/
static RJS_PropertyKeyList*
pkl_from_object (RJS_Runtime *rt, RJS_Value *keys, RJS_Value *o, RJS_Hash *hash)
{
    int64_t              i, len;
    RJS_Result           r;
    RJS_PropertyKeyList *pkl;
    size_t               top;
    RJS_Value           *iv;
    RJS_HashEntry       *he, **phe;

    if (!rjs_value_is_object(rt, o)) {
        rjs_throw_type_error(rt, _("the value is not an object"));
        return NULL;
    }

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        return NULL;

    if (!(pkl = rjs_property_key_list_new(rt, keys, len)))
        return NULL;

    top = rjs_value_stack_save(rt);
    iv  = rjs_value_stack_push(rt);

    for (i = 0; i < len; i ++) {
        RJS_Value *kv;
        void      *k;

        if ((r = rjs_get_index(rt, o, i, iv)) == RJS_ERR) {
            pkl = NULL;
            goto end;
        }
        
        if (!rjs_value_is_string(rt, iv) && !rjs_value_is_symbol(rt, iv)) {
            rjs_throw_type_error(rt, _("proxy result mismatch"));
            pkl = NULL;
            goto end;
        }

        k = get_prop_key(rt, iv);
    
        r = rjs_hash_lookup(hash, k, &he, &phe, &rjs_hash_size_ops, rt);
        if (r) {
            rjs_throw_type_error(rt, _("proxy result mismatch"));
            pkl = NULL;
            goto end;
        }

        RJS_NEW(rt, he);

        rjs_hash_insert(hash, k, he, phe, &rjs_hash_size_ops, rt);

        kv = pkl->keys.items + pkl->keys.item_num ++;

        rjs_value_copy(rt, kv, iv);
    }

end:
    rjs_value_stack_restore(rt, top);
    return pkl;
}

/*Get the proxy object's property keys.*/
static RJS_Result
proxy_object_op_own_property_keys (RJS_Runtime *rt, RJS_Value *o, RJS_Value *keys)
{
    RJS_ProxyObject     *po      = (RJS_ProxyObject*)rjs_value_get_object(rt, o);
    size_t               top     = rjs_value_stack_save(rt);
    RJS_Value           *trap    = rjs_value_stack_push(rt);
    RJS_Value           *tr      = rjs_value_stack_push(rt);
    RJS_Value           *tkeys   = rjs_value_stack_push(rt);
    RJS_Value           *target  = rjs_value_stack_push(rt);
    RJS_Value           *handler = rjs_value_stack_push(rt);
    RJS_PropertyKeyList *pkl, *tpkl;
    RJS_Bool             is_ext;
    RJS_Result           r;
    size_t               i;
    RJS_PropertyDesc     pd;
    RJS_Bool             pd_init = RJS_FALSE;
    RJS_Hash             hash;
    RJS_HashEntry       *he, *nhe, **phe;
    RJS_VECTOR_DECL(RJS_Value) cfg_keys, ncfg_keys;

    rjs_vector_init(&cfg_keys);
    rjs_vector_init(&ncfg_keys);
    rjs_hash_init(&hash);

    rjs_value_copy(rt, target, &po->target);
    rjs_value_copy(rt, handler, &po->handler);

    if (rjs_value_is_null(rt, target)) {
        r = rjs_throw_type_error(rt, _("proxy target is null"));
        goto end;
    }

    if ((r = rjs_get_method(rt, handler, rjs_pn_ownKeys(rt), trap)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, trap)) {
        r = rjs_object_own_property_keys(rt, target, keys);
        goto end;
    }

    if ((r = rjs_call(rt, trap, handler, target, 1, tr)) == RJS_ERR)
        goto end;

    if (!(pkl = pkl_from_object(rt, keys, tr, &hash))) {
        r = RJS_ERR;
        goto end;
    }

    if ((r = rjs_object_is_extensible(rt, target)) == RJS_ERR)
        goto end;
    is_ext = r;

    if ((r = rjs_object_own_property_keys(rt, target, tkeys)) == RJS_ERR)
        goto end;
    tpkl = rjs_value_get_gc_thing(rt, tkeys);

    rjs_property_desc_init(rt, &pd);
    pd_init = RJS_TRUE;

    for (i = 0; i < tpkl->keys.item_num; i ++) {
        RJS_PropertyName pn;
        RJS_Value       *k = &tpkl->keys.items[i];

        rjs_property_name_init(rt, &pn, k);
        r = rjs_object_get_own_property(rt, target, &pn, &pd);
        rjs_property_name_deinit(rt, &pn);
        if (r == RJS_ERR)
            goto end;

        if (r && !(pd.flags & RJS_PROP_FL_CONFIGURABLE))
            rjs_vector_append(&ncfg_keys, *k, rt);
        else
            rjs_vector_append(&cfg_keys, *k, rt);
    }

    if (is_ext && (ncfg_keys.item_num == 0)) {
        r = RJS_OK;
        goto end;
    }

    for (i = 0; i < ncfg_keys.item_num; i ++) {
        RJS_Value *kv = &ncfg_keys.items[i];
        void      *k  = get_prop_key(rt, kv);

        r = rjs_hash_lookup(&hash, k, &he, &phe, &rjs_hash_size_ops, rt);
        if (!r) {
            r = rjs_throw_type_error(rt, _("proxy result mismatch"));
            goto end;
        }

        rjs_hash_remove(&hash, phe, rt);
        RJS_DEL(rt, he);
    }

    if (is_ext) {
        r = RJS_OK;
        goto end;
    }

    for (i = 0; i < cfg_keys.item_num; i ++) {
        RJS_Value *kv = &cfg_keys.items[i];
        void      *k  = get_prop_key(rt, kv);

        r = rjs_hash_lookup(&hash, k, &he, &phe, &rjs_hash_size_ops, rt);
        if (!r) {
            r = rjs_throw_type_error(rt, _("proxy result mismatch"));
            goto end;
        }

        rjs_hash_remove(&hash, phe, rt);
        RJS_DEL(rt, he);
    }

    if (hash.entry_num) {
        r = rjs_throw_type_error(rt, _("proxy result mismatch"));
        goto end;
    }

    r = RJS_OK;
end:
    if (pd_init)
        rjs_property_desc_deinit(rt, &pd);
        
    rjs_hash_foreach_safe(&hash, i, he, nhe) {
        RJS_DEL(rt, he);
    }
    rjs_hash_deinit(&hash, &rjs_hash_size_ops, rt);

    rjs_vector_deinit(&cfg_keys, rt);
    rjs_vector_deinit(&ncfg_keys, rt);

    rjs_value_stack_restore(rt, top);
    return r;
}

/*Call the proxy object.*/
static RJS_Result
proxy_object_op_call (RJS_Runtime *rt, RJS_Value *o, RJS_Value *thiz, RJS_Value *args, size_t argc, RJS_Value *rv)
{
    RJS_ProxyObject *po      = (RJS_ProxyObject*)rjs_value_get_object(rt, o);
    size_t           top     = rjs_value_stack_save(rt);
    RJS_Value       *trap    = rjs_value_stack_push(rt);
    RJS_Value       *handler = rjs_value_stack_push(rt);
    RJS_Value       *target  = rjs_value_stack_push(rt);
    RJS_Value       *tthiz   = rjs_value_stack_push(rt);
    RJS_Value       *targs   = rjs_value_stack_push(rt);
    RJS_Value       *arg;
    size_t           i;
    RJS_Result       r;

    rjs_value_copy(rt, target, &po->target);
    rjs_value_copy(rt, handler, &po->handler);

    if (rjs_value_is_null(rt, target)) {
        r = rjs_throw_type_error(rt, _("proxy target is null"));
        goto end;
    }

    if ((r = rjs_get_method(rt, handler, rjs_pn_apply(rt), trap)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, trap)) {
        r = rjs_object_call(rt, target, thiz, args, argc, rv);
        goto end;
    }

    rjs_value_copy(rt, tthiz, thiz);

    rjs_array_new(rt, targs, argc, NULL);
    for (i = 0; i < argc; i ++) {
        arg = rjs_value_buffer_item(rt, args, i);
        rjs_set_index(rt, targs, i, arg, RJS_TRUE);
    }

    r = rjs_call(rt, trap, handler, target, 3, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Construct a new object.*/
static RJS_Result
proxy_object_op_construct (RJS_Runtime *rt, RJS_Value *o, RJS_Value *args, size_t argc, RJS_Value *nt, RJS_Value *rv)
{
    RJS_ProxyObject *po      = (RJS_ProxyObject*)rjs_value_get_object(rt, o);
    size_t           top     = rjs_value_stack_save(rt);
    RJS_Value       *trap    = rjs_value_stack_push(rt);
    RJS_Value       *handler = rjs_value_stack_push(rt);
    RJS_Value       *target  = rjs_value_stack_push(rt);
    RJS_Value       *targs   = rjs_value_stack_push(rt);
    RJS_Value       *tnt     = rjs_value_stack_push(rt);
    RJS_Value       *arg;
    size_t           i;
    RJS_Result       r;

    rjs_value_copy(rt, target, &po->target);
    rjs_value_copy(rt, handler, &po->handler);

    if (rjs_value_is_null(rt, target)) {
        r = rjs_throw_type_error(rt, _("proxy target is null"));
        goto end;
    }

    if ((r = rjs_get_method(rt, handler, rjs_pn_construct(rt), trap)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, trap)) {
        r = rjs_object_construct(rt, target, args, argc, nt, rv);
        goto end;
    }

    if (nt)
        rjs_value_copy(rt, tnt, nt);

    rjs_array_new(rt, targs, argc, NULL);
    for (i = 0; i < argc; i ++) {
        arg = rjs_value_buffer_item(rt, args, i);
        rjs_set_index(rt, targs, i, arg, RJS_TRUE);
    }

    if ((r = rjs_call(rt, trap, handler, target, 3, rv)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_object(rt, rv)) {
        r = rjs_throw_type_error(rt, _("the result of \"construct\" is not an object"));
        goto end;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Proxy object operation functions.*/
static const RJS_ObjectOps
proxy_object_ops = {
    {
        RJS_GC_THING_PROXY_OBJECT,
        proxy_object_op_gc_scan,
        proxy_object_op_gc_free
    },
    proxy_object_op_get_prototype_of,
    proxy_object_op_set_prototype_of,
    proxy_object_op_is_extensible,
    proxy_object_op_prevent_extensions,
    proxy_object_op_get_own_property,
    proxy_object_op_define_own_property,
    proxy_object_op_has_property,
    proxy_object_op_get,
    proxy_object_op_set,
    proxy_object_op_delete,
    proxy_object_op_own_property_keys,
    NULL,
    NULL
};

/*Callable proxy object operation functions.*/
static const RJS_ObjectOps
callable_proxy_object_ops = {
    {
        RJS_GC_THING_PROXY_OBJECT,
        proxy_object_op_gc_scan,
        proxy_object_op_gc_free
    },
    proxy_object_op_get_prototype_of,
    proxy_object_op_set_prototype_of,
    proxy_object_op_is_extensible,
    proxy_object_op_prevent_extensions,
    proxy_object_op_get_own_property,
    proxy_object_op_define_own_property,
    proxy_object_op_has_property,
    proxy_object_op_get,
    proxy_object_op_set,
    proxy_object_op_delete,
    proxy_object_op_own_property_keys,
    proxy_object_op_call,
    NULL
};

/*Construct proxy object operation functions.*/
static const RJS_ObjectOps
construct_proxy_object_ops = {
    {
        RJS_GC_THING_PROXY_OBJECT,
        proxy_object_op_gc_scan,
        proxy_object_op_gc_free
    },
    proxy_object_op_get_prototype_of,
    proxy_object_op_set_prototype_of,
    proxy_object_op_is_extensible,
    proxy_object_op_prevent_extensions,
    proxy_object_op_get_own_property,
    proxy_object_op_define_own_property,
    proxy_object_op_has_property,
    proxy_object_op_get,
    proxy_object_op_set,
    proxy_object_op_delete,
    proxy_object_op_own_property_keys,
    proxy_object_op_call,
    proxy_object_op_construct
};

/**
 * Create a new proxy object.
 * \param rt The current runtime.
 * \param[out] v Return the new proxy object.
 * \param target The target object.
 * \param handler The handler object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_proxy_object_new (RJS_Runtime *rt, RJS_Value *v, RJS_Value *target, RJS_Value *handler)
{
    RJS_ProxyObject     *po;
    const RJS_ObjectOps *ops;

    if (!rjs_value_is_object(rt, target))
        return rjs_throw_type_error(rt, _("target is not an object"));

    if (!rjs_value_is_object(rt, handler))
        return rjs_throw_type_error(rt, _("handler is not an object"));

    RJS_NEW(rt, po);

    rjs_value_copy(rt, &po->target, target);
    rjs_value_copy(rt, &po->handler, handler);

    if (rjs_is_callable(rt, target)) {
        if (rjs_is_constructor(rt, target))
            ops = &construct_proxy_object_ops;
        else
            ops = &callable_proxy_object_ops;
    } else {
        ops = &proxy_object_ops;
    }

    return rjs_object_init(rt, v, &po->object, NULL, ops);
}
