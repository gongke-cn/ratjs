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

/*This program demonstrates how to add native objects to the script.*/

#include <stdlib.h>
#include <ratjs.h>

/*Script source.*/
static const char*
source =
"(new MyObject(1234)).dump();"
"MyObject(\"abcdefg\").dump();"
;

/*The global native data.*/
typedef struct {
    RJS_Value my_object_proto; /*MyObject's prototype.*/
} GlobalData;

/*
 * GC scan function for Global object.
 */
static void
global_scan (RJS_Runtime *rt, void *ptr)
{
    GlobalData *gd = ptr;

    rjs_gc_scan_value(rt, &gd->my_object_proto);
}

/*Free the global object's native data.*/
static void
global_free (RJS_Runtime *rt, void *ptr)
{
    GlobalData *gd = ptr;

    free(gd);
}

/*Native data of MyObject.*/
typedef struct {
    RJS_Value v;
} MyObjectData;

/*
 * GC scan function for MyObject.
 * This function is invoked by garbage collocter to scan the referenced things in MyObject.
 */
static void
my_object_scan (RJS_Runtime *rt, void *ptr)
{
    MyObjectData *mod = ptr;

    rjs_gc_scan_value(rt, &mod->v);
}

/**
 * Free MyObject.
 */
static void
my_object_free (RJS_Runtime *rt, void *ptr)
{
    MyObjectData *mod = ptr;

    free(mod);
}

/*The tag is used to check the data type of native object.*/
static const char *my_object_tag = "MyObject";

/*Constructor of MyObject.*/
static RJS_NF(constructor)
{
    GlobalData   *gd  = rjs_runtime_get_data(rt);
    RJS_Value    *arg = rjs_argument_get(rt, args, argc, 0);
    MyObjectData *mod;
    RJS_Result    r;

    /*
     * Create the native object from the NewTarget.
     * If NewTarget == NULL, use MyObject.prototype stored in global native data as the object's prototype.
     */
    if ((r = rjs_native_object_from_constructor(rt, nt, &gd->my_object_proto, rv)) == RJS_ERR)
        goto end;

    /*Store the native data to the object.*/
    mod = malloc(sizeof(MyObjectData));
    rjs_value_copy(rt, &mod->v, arg);
    rjs_native_object_set_data(rt, rv, my_object_tag, mod, my_object_scan, my_object_free);

    r = RJS_OK;
end:
    return r;
}

/*function MyObject.prototype.dump()*/
static RJS_NF(MyObject_prototype_dump)
{
    MyObjectData *mod;
    RJS_Result    r;
    const char   *cstr;

    if (rjs_native_object_get_tag(rt, thiz) != my_object_tag) {
        r = rjs_throw_type_error(rt, "this is not MyObject");
        goto end;
    }

    mod = rjs_native_object_get_data(rt, thiz);

    cstr = rjs_to_desc_chars(rt, &mod->v, NULL, NULL);

    printf("dump: %s\n", cstr);

    rjs_value_copy(rt, rv, thiz);
    r = RJS_OK;
end:
    return r;
}

/*Add native objects.*/
static void
add_native (RJS_Runtime *rt)
{
    RJS_Realm *realm;
    RJS_Value *global;
    RJS_PropertyName pn;
    GlobalData *gd;
    /*Store the native value stack's pointer.*/
    size_t     top    = rjs_value_stack_save(rt);
    /*Allocate temporary value to store the function name.*/
    RJS_Value *name   = rjs_value_stack_push(rt);
    /*Allocate temporary value to store the constructor.*/
    RJS_Value *constr = rjs_value_stack_push(rt);
    /*Allocate temporary value to store the prototype.*/
    RJS_Value *proto  = rjs_value_stack_push(rt);
    /*Allocate temporary value to store the property value.*/
    RJS_Value *pv     = rjs_value_stack_push(rt);

    /*Get the current realm.*/
    realm = rjs_realm_current(rt);

    /*Get the global object of the current realm.*/
    global = rjs_global_object(realm);

    /*Create the name of the constructor.*/
    rjs_string_from_enc_chars(rt, name, "MyObject", -1, NULL);

    /*Create the constructor "MyObject", arguments count = 1.*/
    rjs_create_builtin_function(rt, NULL, constructor, 1, name, NULL, NULL, NULL, constr);

    /*Add the constructor to the global object.*/
    rjs_property_name_init(rt, &pn, name);
    rjs_create_data_property(rt, global, &pn, constr);
    rjs_property_name_deinit(rt, &pn);

    /*Create the prototype object.*/
    rjs_ordinary_object_create(rt, NULL, proto);

    /*Create the name of the dump function.*/
    rjs_string_from_enc_chars(rt, name, "dump", -1, NULL);

    /*Add MyObject.prototype.dump funciton.*/
    rjs_create_builtin_function(rt, NULL, MyObject_prototype_dump, 0, name, NULL, NULL, NULL, pv);

    /*Add the function to the prototype object.*/
    rjs_property_name_init(rt, &pn, name);
    rjs_create_data_property(rt, proto, &pn, pv);
    rjs_property_name_deinit(rt, &pn);

    /*Make the constructor.*/
    rjs_make_constructor(rt, constr, RJS_FALSE, proto);

    /*Store the prototype to global native data.*/
    gd = malloc(sizeof(GlobalData));
    rjs_value_copy(rt, &gd->my_object_proto, proto);
    rjs_runtime_set_data(rt, gd, global_scan, global_free);

    /*Restore the old native value stack top to release all temporary values.*/
    rjs_value_stack_restore(rt, top);
}

int
main (int argc, char **argv)
{
    RJS_Runtime *rt;
    RJS_Value   *str, *script;
    RJS_Result   r;
    int          rc = 1;

    /*Create the JS runtime.*/
    rt = rjs_runtime_new();

    /*Add native objects.*/
    add_native(rt);

    /*Allocate the string value to store the source of the script.*/
    str = rjs_value_stack_push(rt);

    /*Create the source string.*/
    rjs_string_from_enc_chars(rt, str, source, -1, NULL);

    /*Allocate the string value to store the script.*/
    script = rjs_value_stack_push(rt);

    /*Load the script.*/
    r = rjs_script_from_string(rt, script, str, NULL, RJS_FALSE);
    if (r == RJS_ERR) {
        fprintf(stderr, "parse script faled\n");
        goto end;
    }

    /*Run the script.*/
    r = rjs_script_evaluation(rt, script, NULL);
    if (r == RJS_ERR) {
        fprintf(stderr, "execure the script failed\n");
        goto end;
    }

    rc = 0;

end:
    /*Free the JS runtime.*/
    rjs_runtime_free(rt);
    return rc;
}
