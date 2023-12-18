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
 * C/Javascript value converter.
 * @module converter
 */

import { Log } from "../log";
import { CType, CTypeModel } from "./c-context";
import { indent } from "./njs-generator";

let log = new Log("converter");

//log.enable = false;

/**
 * Get the element type.
 * @param {string} t The C type string.
 * @returns {string} The element type string. 
 */
function getElementType (t)
{
    let r;

    switch (t) {
    case "int8_t":
        r = "RJS_ARRAY_ELEMENT_INT8";
        break;
    case "int16_t":
        r = "RJS_ARRAY_ELEMENT_INT16";
        break;
    case "int32_t":
        r = "RJS_ARRAY_ELEMENT_INT32";
        break;
    case "int64_t":
        r = "RJS_ARRAY_ELEMENT_BIGINT64";
        break;
    case "uint8_t":
        r = "RJS_ARRAY_ELEMENT_UINT8";
        break;
    case "uint16_t":
        r = "RJS_ARRAY_ELEMENT_UINT16";
        break;
    case "uint32_t":
        r = "RJS_ARRAY_ELEMENT_UINT32";
        break;
    case "uint64_t":
        r = "RJS_ARRAY_ELEMENT_BIGUINT64";
        break;
    case "float":
        r = "RJS_ARRAY_ELEMENT_FLOAT32";
        break;
    case "double":
        r = "RJS_ARRAY_ELEMENT_FLOAT64";
        break;
    case "char":
        r = "RJS_ARRAY_ELEMENT_CHAR";
        break;
    case "unsigned char":
        r = "RJS_ARRAY_ELEMENT_UCHAR";
        break;
    case "short":
        r = "RJS_ARRAY_ELEMENT_SHORT";
        break;
    case "unsigned short":
        r = "RJS_ARRAY_ELEMENT_USHORT";
        break;
    case "int":
        r = "RJS_ARRAY_ELEMENT_INT";
        break;
    case "unsigned int":
        r = "RJS_ARRAY_ELEMENT_UINT";
        break;
    case "long":
        r = "RJS_ARRAY_ELEMENT_LONG";
        break;
    case "unsigned long":
        r = "RJS_ARRAY_ELEMENT_ULONG";
        break;
    case "long long":
        r = "RJS_ARRAY_ELEMENT_LLONG";
        break;
    case "unsigned long long":
        r = "RJS_ARRAY_ELEMENT_ULLONG";
        break;
    case "ssize_t":
        r = "RJS_ARRAY_ELEMENT_SSIZE_T";
        break;
    case "size_t":
        r = "RJS_ARRAY_ELEMENT_SIZE_T";
        break;
    default:
        break;
    }

    return r;
}

/**
 * Convert C value to javascript value.
 */
export const C2Js = {
    /**
     * Converters array.
     * @type {Array}
     */
    converts: [],

    /**
     * Add a converter.
     * @param {*} c 
     */
    addConverter (c) {
        this.converts.push(c);

        return this;
    },

    /**
     * Convert the C value to javascript value.
     * @param {FuncGenerator} f The function generator.
     * @param {string} c The C variable name.
     * @param {string} js The javascript value variable name.
     * @param {object} t Type declaration.
     */
    convert (f, c, js, t) {
        for (let conv of this.converts) {
            if (conv.test(t)) {
                conv.c2js(f, c, js, t);
                break;
            }
        }
    }
}.addConverter({
    /*Number.*/
    test (t) {
        return t.type.model == CTypeModel.PRIMITIVE;
    },
    c2js (f, c, js, t) {
        f.addOutput(`rjs_value_set_number(njs_rt, ${js}, ${c});\n`);
    }
}).addConverter({
    /*String.*/
    test (t) {
        if ((t.type.model == CTypeModel.POINTER) && (t.type.valueType.name == "char"))
            return true;
        if ((t.type.model == CTypeModel.ARRAY) && (t.type.itemType.name == "char"))
            return true;

        return false;
    },
    c2js (f, c, js, t) {
        f.addOutput(`\
if ((njs_r = rjs_string_from_enc_chars(njs_rt, ${js}, ${c}, -1, NULL)) == RJS_ERR)
    goto end;
`       );
    }
}).addConverter({
    /*Box.*/
    test (t) {
        return t.type.model == CTypeModel.BOX;
    },
    c2js (f, c, js, t) {
        let tn = t.type.box.name;

        f.moduleData();
        f.addOutput(`\
if ((njs_r = rjs_create_c_ptr(njs_rt, njs_md->ctype_${tn}, (void*)&${c}, RJS_CPTR_TYPE_VALUE, 1, 0, ${js})) == RJS_ERR)
    goto end;
`       );
    }
}).addConverter({
    /*Box pointer.*/
    test (t) {
        return (t.type.model == CTypeModel.POINTER) && (t.type.valueType.model == CTypeModel.BOX) && (t.length === undefined);
    },
    c2js (f, c, js, t) {
        let tn = t.type.valueType.box.name;

        f.moduleData();
        f.addOutput(`\
if (${c} == NULL) {
    rjs_value_set_null(njs_rt, ${js});
} else if ((njs_r = rjs_create_c_ptr(njs_rt, njs_md->ctype_${tn}, (void*)${c}, RJS_CPTR_TYPE_VALUE, 1, 0, ${js})) == RJS_ERR) {
    goto end;
}
`       );
    }
}).addConverter({
    /*Box array.*/
    test (t) {
        if ((t.type.model == CTypeModel.POINTER) && (t.type.valueType.model == CTypeModel.BOX) && (t.length !== undefined))
            return true;
        if ((t.type.model == CTypeModel.ARRAY) && (t.type.itemType.model == CTypeModel.BOX) && (t.mode != "getter"))
            return true;
        return false;
    },
    c2js (f, c, js, t) {
        let tn, len;

        if (t.type.model == CTypeModel.POINTER)
            tn = t.type.valueType.box.name;
        else
            tn = t.type.itemType.box.name;

        if (t.length !== undefined)
            len = t.length;
        else
            len = "-1";

        f.moduleData();
        f.addOutput(`\
if (${c} == NULL) {
    rjs_value_set_null(njs_rt, ${js});
} else if ((njs_r = rjs_create_c_ptr(njs_rt, njs_md->ctype_${tn}, (void*)${c}, RJS_CPTR_TYPE_ARRAY, ${len}, 0, ${js})) == RJS_ERR) {
    goto end;
}
`       );
    }
}).addConverter({
    /*Box array.*/
    test (t) {
        if ((t.type.model == CTypeModel.ARRAY) && (t.type.itemType.model == CTypeModel.BOX) && (t.mode == "getter"))
            return true;
        return false;
    },
    c2js (f, c, js, t) {
        let tn, len;

        if (t.type.model == CTypeModel.POINTER)
            tn = t.type.valueType.box.name;
        else
            tn = t.type.itemType.box.name;

        len = `RJS_N_ELEM(${c})`;

        f.moduleData();
        f.addOutput(`\
if ((njs_r = rjs_create_c_ptr(njs_rt, njs_md->ctype_${tn}, (void*)${c}, RJS_CPTR_TYPE_ARRAY, ${len}, 0, ${js})) == RJS_ERR)
    goto end;
`       );
    }
}).addConverter({
    /*Box pointer array.*/
    test (t) {
        if ((t.type.model == CTypeModel.POINTER)
                && (t.type.valueType.model == CTypeModel.POINTER)
                && (t.type.valueType.valueType.model == CTypeModel.BOX)
                && ((t.length !== undefined) || t.nullTerminated))
            return true;
        if ((t.type.model == CTypeModel.ARRAY)
                && (t.type.itemType.model == CTypeModel.POINTER)
                && (t.type.itemType.valueType.model == CType.BOX))
            return true;
        return false;
    },
    c2js (f, c, js, t) {
        let tn, len;

        if (t.type.model == CTypeModel.POINTER)
            tn = t.type.valueType.valueType.box.name;
        else
            tn = t.type.itemType.valueType.box.name;

        let lenCode = "";

        if (t.length !== undefined) {
            len = t.length;
        } else if (t.nullTerminated) {
            len = `njs_${t.name}_len`;
            lenCode = `\
size_t ${len} = 0;
${tn} **njs_pp;

for (njs_pp = ${c}; *njs_pp; njs_pp ++)
    ${len} ++;`;
        } else {
            len = "-1";
        }


        f.moduleData();
        f.addOutput(indent `\
if (${c} == NULL) {
    rjs_value_set_null(njs_rt, ${js});
} else {
    ${lenCode}
    if ((njs_r = rjs_create_c_ptr(njs_rt, njs_md->ctype_${tn}, (void*)${c}, RJS_CPTR_TYPE_ARRAY, ${len}, 0, ${js})) == RJS_ERR)
        goto end;
}
`       );
    }
}).addConverter({
    /*Typed array pointer.*/
    test (t) {
        if ((t.type.model == CTypeModel.POINTER) && (t.type.valueType.model == CTypeModel.PRIMITIVE) && (t.length !== undefined))
            return true;
        if ((t.type.model == CTypeModel.ARRAY) && (t.type.itemType.model == CTypeModel.PRIMITIVE) && (t.mode != "getter"))
            return true;
        return false;
    },
    c2js (f, c, js, t) {
        let tn, len;

        if (t.type.model == CTypeModel.POINTER)
            tn = t.type.valueType.name;
        else
            tn = t.type.itemType.name;

        if (t.length !== undefined)
            len = t.length;
        else
            len = "-1";

        let et = getElementType(tn);

        f.moduleData();
        f.addOutput(`\
if (${c} == NULL) {
    rjs_value_set_null(njs_rt, ${js});
} else if ((njs_r = rjs_create_c_typed_array(njs_rt, ${et}, ${c}, ${len}, ${js})) == RJS_ERR) {
    goto end;
}
`       );
    }
}).addConverter({
    /*Typed array pointer.*/
    test (t) {
        if ((t.type.model == CTypeModel.ARRAY) && (t.type.itemType.model == CTypeModel.PRIMITIVE) && (t.mode == "getter"))
            return true;
        return false;
    },
    c2js (f, c, js, t) {
        let tn, len;

        if (t.type.model == CTypeModel.POINTER)
            tn = t.type.valueType.name;
        else
            tn = t.type.itemType.name;

        if (t.length !== undefined)
            len = t.length;
        else
            len = `RJS_N_ELEM(${c})`;

        let et = getElementType(tn);

        f.moduleData();
        f.addOutput(`\
if ((njs_r = rjs_create_c_typed_array(njs_rt, ${et}, ${c}, ${len}, ${js})) == RJS_ERR) {
    goto end;
}
`       );
    }
}).addConverter({
    /*Function.*/
    test (t) {
        return t.type.model == CTypeModel.FUNCTION;
    },
    c2js (f, c, js, t) {
        f.moduleData();
        f.addOutput(`\
if (${c} == NULL)
    rjs_value_set_null(njs_rt, ${js});
else if ((njs_r = rjs_create_c_func_ptr(njs_rt, njs_md->ctype_${t.type.name}, ${c}, ${js})) == RJS_ERR)
    goto end;
`       );
    }
});

/**
 * Convert javascript value to C value.
 */
export const Js2C = {
    /**
     * Converters array.
     * @type {Array}
     */
    converts: [],

    /**
     * Add a converter.
     * @param {*} c 
     */
    addConverter (c) {
        this.converts.push(c);

        return this;
    },

    /**
     * Convert the javascript value to C value.
     * @param {FuncGenerator} f The function generator.
     * @param {string} js The javascript value variable name.
     * @param {string} c The C variable name.
     * @param {object} t Type declaration.
     */
    convert (f, js, c, t) {
        for (let conv of this.converts) {
            if (conv.test(t)) {
                conv.js2c(f, js, c, t);
                break;
            }
        }
    }
}.addConverter({
    /*Number*/
    test (t) {
        return t.type.model == CTypeModel.PRIMITIVE;
    },
    js2c (f, js, c, t) {
        let nv = `njs_${t.name}_num`;

        f.addDecl(`RJS_Number ${nv};\n`);
        f.addInput(`\
if ((njs_r = rjs_to_number(njs_rt, ${js}, &${nv})) == RJS_ERR)
    goto end;
${c} = ${nv};
`       );
    }
}).addConverter({
    /*String.*/
    test (t) {
        if (t.length !== undefined)
            return false;
        if ((t.type.model == CTypeModel.POINTER) && (t.type.valueType.name == "char"))
            return true;
        return false;
    },
    js2c (f, js, c, t) {
        let sv = `njs_${t.name}_str`;
        let cbv = `njs_${t.name}_cb`;

        f.valueDecl(sv);
        f.charBufDecl(cbv);
        f.addInput(`\
if (rjs_value_is_null(njs_rt, ${js}) || rjs_value_is_undefined(njs_rt, ${js})) {
    ${c} = NULL;
} else {
    if ((njs_r = rjs_to_string(njs_rt, ${js}, ${sv})) == RJS_ERR)
        goto end;
    if (!(${c} = rjs_string_to_enc_chars(njs_rt, ${sv}, &${cbv}, NULL))) {
        njs_r = RJS_ERR;
        goto end;
    }
}
`       );
    }
}).addConverter({
    /*String.*/
    test (t) {
        if ((t.type.model == CTypeModel.POINTER) && (t.type.valueType.name == "char") && (t.length !== undefined))
            return true;
        if ((t.type.model == CTypeModel.ARRAY) && (t.type.itemType.name == "char"))
            return true;
        return false;
    },
    js2c (f, js, c, t) {
        let sv = `njs_${t.name}_str`;
        let cbv = `njs_${t.name}_cb`;
        let cstrv = `njs_${t.name}_cstr`;

        let len = t.length;

        if (len === undefined)
            len = `sizeof(${c})`;

        f.valueDecl(sv);
        f.addDecl(`const char *${cstrv};\n`);
        f.charBufDecl(cbv);
        f.addInput(`\
if ((njs_r = rjs_to_string(njs_rt, ${js}, ${sv})) == RJS_ERR)
    goto end;
if (!(${cstrv} = rjs_string_to_enc_chars(njs_rt, ${sv}, &${cbv}, NULL))) {
    njs_r = RJS_ERR;
    goto end;
}
if (snprintf(${c}, ${len}, "%s", ${cstrv}) >= ${len}) {
    njs_r = rjs_throw_range_error(njs_rt, "input string is too long");
    goto end;
}
`       );
    }
}).addConverter({
    /*Box.*/
    test (t) {
        return t.type.model == CTypeModel.BOX;
    },
    js2c (f, js, c, t) {
        let tn = t.type.box.name;
        let cptr = `njs_${t.name}_cptr`;
        let cptrv = `njs_${t.name}_cptrv`;

        f.moduleData();
        f.addDecl(`${t.type.box.cName}* ${cptr};\n`);
        f.valueDecl(cptrv);
        f.addInput(`\
if (rjs_is_c_ptr(njs_rt, ${js})) {
    if (!(${cptr} = rjs_get_c_ptr(njs_rt, njs_md->ctype_${tn}, RJS_CPTR_TYPE_VALUE, ${js}))) {
        njs_r = RJS_ERR;
        goto end;
    }
    ${c} = *${cptr};
} else {
    if ((njs_r = rjs_create_c_ptr(njs_rt, njs_md->ctype_${tn}, &${c}, RJS_CPTR_TYPE_VALUE, 1, 0, ${cptrv})) == RJS_ERR)
        goto end;
    if ((njs_r = rjs_object_assign(njs_rt, ${cptrv}, ${js})) == RJS_ERR)
        goto end;
}
`       );
    }
}).addConverter({
    /*Box pointer.*/
    test (t) {
        return (t.type.model == CTypeModel.POINTER) && (t.type.valueType.model == CTypeModel.BOX) && (t.length === undefined);
    },
    js2c (f, js, c, t) {
        let tn = t.type.valueType.box.name;

        f.moduleData();
        f.addInput(`\
if (!(${c} = rjs_get_c_ptr(njs_rt, njs_md->ctype_${tn}, RJS_CPTR_TYPE_VALUE, ${js}))) {
    njs_r = RJS_ERR;
    goto end;
}
`       );
    }
}).addConverter({
    /*void pointer.*/
    test (t) {
        return (t.type.model == CTypeModel.POINTER) && (t.type.valueType.model == CTypeModel.VOID);
    },
    js2c (f, js, c, t) {
        f.addInput(`\
if (!(${c} = rjs_get_c_ptr(njs_rt, NULL, RJS_CPTR_TYPE_UNKNOWN, ${js}))) {
    njs_r = RJS_ERR;
    goto end;
}
`       );
    }
}).addConverter({
    /*Box array pointer.*/
    test (t) {
        if ((t.type.model == CTypeModel.POINTER) && (t.type.valueType.model == CTypeModel.BOX) && (t.length !== undefined))
            return true;
        if ((t.type.model == CTypeModel.ARRAY) && (t.type.itemType.model == CTypeModel.BOX) && (t.mode != "setter"))
            return true;
        return false;
    },
    js2c (f, js, c, t) {
        let tn, len;

        if (t.type.model == CTypeModel.POINTER)
            tn = t.type.valueType.box.name;
        else
            tn = t.type.itemType.box.name;

        if (t.length !== undefined)
            len = t.length;

        f.moduleData();
        f.addInput(`\
if (!(${c} = rjs_get_c_ptr(njs_rt, njs_md->ctype_${tn}, RJS_CPTR_TYPE_ARRAY, ${js}))) {
    njs_r = RJS_ERR;
    goto end;
}
`       );
        if (len !== undefined) {
            f.addCheck(`\
/*Check the length of "${t.name}".*/
if (rjs_get_c_array_length(njs_rt, ${js}) < ${len}) {
    njs_r = rjs_throw_range_error(njs_rt, "the C array length is less than expected lenth");;
    goto end;
}
`           );
        }
    }
}).addConverter({
    /*Box array.*/
    test (t) {
        if ((t.type.model == CTypeModel.ARRAY) && (t.type.itemType.model == CTypeModel.BOX) && (t.mode == "setter"))
            return true;
        return false;
    },
    js2c (f, js, c, t) {
        let cptr = `njs_${t.name}_cptr`;
        let lv = `njs_${t.name}_len`;
        let tn, len;

        if (t.type.model == CTypeModel.POINTER)
            tn = t.type.valueType.box.name;
        else
            tn = t.type.itemType.box.name;

        if (t.length !== undefined)
            len = t.length;

        f.addDecl(`${tn} *${cptr};\n`);
        f.addDecl(`size_t ${lv};\n`);

        f.moduleData();
        f.addInput(`\
if (!(${cptr} = rjs_get_c_ptr(njs_rt, njs_md->ctype_${tn}, RJS_CPTR_TYPE_ARRAY, ${js}))) {
    njs_r = RJS_ERR;
    goto end;
}
${lv} = rjs_get_c_array_length(njs_rt, ${js});
RJS_ELEM_CPY(${c}, ${cptr}, RJS_MIN(${lv}, RJS_N_ELEM(${c})));
`       );
    }
}).addConverter({
    /*Typed array pointer.*/
    test (t) {
        if ((t.type.model == CTypeModel.POINTER) && (t.type.valueType.model == CTypeModel.PRIMITIVE) && (t.length !== undefined))
            return true;
        if ((t.type.model == CTypeModel.ARRAY) && (t.type.itemType.model == CTypeModel.PRIMITIVE) && (t.mode != "setter"))
            return true;
        return false;
    },
    js2c (f, js, c, t) {
        let tn, et, len;

        if (t.type.model == CTypeModel.POINTER)
            tn = t.type.valueType.name;
        else
            tn = t.type.itemType.name;

        et = getElementType(tn);

        if (t.length !== undefined)
            len = t.length;

        f.addInput(`\
if (rjs_value_is_null(njs_rt, ${js}) || rjs_value_is_undefined(njs_rt, ${js})) {
    ${c} = NULL;
} else if (!(${c} = rjs_get_c_ptr(njs_rt, NULL, ${et}, ${js}))) {
    njs_r = rjs_throw_type_error(njs_rt, "typed array type mismatch");
    goto end;
}
`       );
        if (len !== undefined) {
            f.addCheck(`\
/*Check the length of "${t.name}".*/
if (rjs_get_c_array_length(njs_rt, ${js}) < ${len}) {
    njs_r = rjs_throw_range_error(njs_rt, "the typed array length is less than expected lenth");;
    goto end;
}
`           );
        }
    }
}).addConverter({
    /*Typed array.*/
    test (t) {
        if ((t.type.model == CTypeModel.ARRAY) && (t.type.itemType.model == CTypeModel.PRIMITIVE) && (t.mode == "setter"))
            return true;
        return false;
    },
    js2c (f, js, c, t) {
        let tn, et, len;

        if (t.type.model == CTypeModel.POINTER)
            tn = t.type.valueType.name;
        else
            tn = t.type.itemType.name;

        et = getElementType(tn);

        if (t.length !== undefined)
            len = t.length;
        else
            len = `RJS_N_ELEM(${c})`;

        let cv = `njs_${t.name}_cptr`;
        f.addDecl(`${tn} *${cv};\n`);

        f.addInput(`\
if (!(${cv} = rjs_get_c_ptr(njs_rt, NULL, ${et}, ${js}))) {
    njs_r = rjs_throw_type_error(njs_rt, "typed array type mismatch");
    goto end;
}
`       );
        if (len !== undefined) {
            f.addCheck(`\
/*Check the length of "${t.name}".*/
if (rjs_get_c_array_length(njs_rt, ${js}) < ${len}) {
    njs_r = rjs_throw_range_error(njs_rt, "the typed array length is less than expected lenth");;
    goto end;
}
`           );
        }
        f.addBody(`\
/*Copy "${t.name}".*/
RJS_ELEM_CPY(${c}, ${cv}, ${len});
`       );
    }
}).addConverter({
    /*Function.*/
    test (t) {
        return t.type.model == CTypeModel.FUNCTION;
    },
    js2c (f, js, c, t) {
        f.moduleData();
        f.addInput(`\
if (rjs_value_is_null(njs_rt, ${js}) || rjs_value_is_undefined(njs_rt, ${js}))
    ${c} = NULL;
else if (!(${c} = rjs_get_c_ptr(njs_rt, njs_md->ctype_${t.type.name}, RJS_CPTR_TYPE_C_FUNC, ${js}))) {
    njs_r = RJS_ERR;
    goto end;
}
`);
    }
});