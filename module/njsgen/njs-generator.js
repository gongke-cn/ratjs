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
 * Native javascript source generator.
 * @module njs-generator
 */

import { Log } from "../log";
import { CContext, CBoxType, CType, CDirection, CTypeModel } from "./c-context";
import { C2Js, Js2C } from "./converter";

let log = new Log("njs-generator");

//log.enable = false;

/**
 * Indent template function.
 * @returns {string} The output string.
 */
export function indent (templ, ...args)
{
    let r = "";
    let head = "";
    let line = "";

    function addLine (l) {
        if (r != "")
            r += "\n";

        if (!l.match(/^\s*$/))
            r += l;
    }

    for (let i = 0; i < templ.length; i ++) {
        let seg = templ[i];
        let lines = seg.split("\n");

        if (lines.length > 1) {
            line += lines[0];

            if (!line.match(/^\s*$/))
                addLine(line);

            for (let i = 1; i < lines.length - 1; i ++)
                addLine(lines[i])

            line = lines[lines.length - 1];
            head = line.match(/^\s*/)[0];
        } else {
            line += seg;
        }

        if (i != templ.length - 1) {
            let arg = args[i];

            if (arg !== undefined) {
                lines = arg.toString().split("\n");

                if (lines.length > 1) {
                    line += lines[0];
                    addLine(line);

                    for (let i = 1; i < lines.length - 1; i ++) {
                        addLine(`${head}${lines[i]}`);
                    }

                    line = `${head}${lines[lines.length - 1]}`;
                } else {
                    line += arg;
                }
            }
        }
    }

    if (!line.match(/^\s*$/))
        addLine(line);

    return r;
}

/**
 * Get the slot.
 * @param {string} tag The slot's tag.
 * @param {string} data The data of the slot.
 */
function slot (tag, data = "/*NJSGEN TODO*/")
{
    return indent `\
/*NJSGEN ${tag} BEGIN*/
${data}
/*NJSGEN ${tag} END*/`;
}

/**
 * Export table.
 */
class ExportTable {
    /**
     * Export symbols.
     * @type {Set<string>}
     */
    symbols = new Set();

    /**
     * Get export table declaration code.
     * @type {string}
     */
    get declCode () {
        return indent `\
/*Local export symbols.*/
static const RJS_ModuleExportDesc
njs_local_exports[] = {
    ${Array.from(this.symbols).map(s => `{NULL, NULL, "${s}", "${s}"},`).join("\n")}
    ${slot("EXPORT_TABLE_DECL")}
    {NULL, NULL, NULL, NULL}
};`;
    };
}

/**
 * Module data.
 */
class ModuleData {
    /**
     * C type set.
     * @type {Set<string>}
     */
    cTypeSet = new Set();

    /**
     * Function type set.
     * @type {Set<string>}
     */
    funcTypeSet = new Set();

    /**
     * Property name set.
     * @type {Set<string>}
     */
    propSet = new Set();

    /**
     * Check if the module data is needed.
     * @type {boolean}
     */
    get isNeeded () {
        if ((this.cTypeSet.size == 0) && (this.propSet.size == 0))
            return false;

        return true;
    };

    /**
     * Get declaration code.
     * @type {string}
     */
    get declCode () {
        if (!this.isNeeded)
            return "";

        return indent `\
/*NJS module data.*/
typedef struct {
    ${Array.from(this.cTypeSet).map(s => `RJS_CType *ctype_${s};`).join("\n")}
    ${Array.from(this.funcTypeSet).map(s => `RJS_CType *ctype_${s};`).join("\n")}
    ${Array.from(this.propSet).map(s => `RJS_Value str_${s};`).join("\n")}
    ${Array.from(this.propSet).map(s => `RJS_PropertyName pn_${s};`).join("\n")}
    ${slot("MODULE_DATA_DECL")}
} NJS_ModuleData;

/*Scan the referenced things in the module data.*/
static void
njs_module_data_scan (RJS_Runtime *njs_rt, void *njs_p)
{
    NJS_ModuleData *njs_md = njs_p;

    njs_md = njs_md;
    ${Array.from(this.propSet).map(s => `rjs_gc_scan_value(njs_rt, &njs_md->str_${s});`).join("\n")}

    ${slot("MODULE_DATA_SCAN")}
}

/*Free the module data.*/
static void
njs_module_data_free (RJS_Runtime *njs_rt, void *njs_p)
{
    NJS_ModuleData *njs_md = njs_p;

    ${Array.from(this.propSet).map(s => `rjs_property_name_deinit(njs_rt, &njs_md->pn_${s});`).join("\n")}

    ${slot("MODULE_DATA_FREE")}

    free(njs_md);
}`;
    };

    /**
     * Get initialize code.
     * @type {string}
     */
    get initCode () {
        if (!this.isNeeded)
            return "";

        return indent `\
NJS_ModuleData *njs_md;

/*Initialize the module data.*/
njs_md = malloc(sizeof(NJS_ModuleData));
assert(njs_md);
${Array.from(this.cTypeSet).map(s => `njs_md->ctype_${s} = NULL;`).join("\n")}
${Array.from(this.funcTypeSet).map(s => `njs_md->ctype_${s} = NULL;`).join("\n")}
${Array.from(this.propSet).map(s => `rjs_value_set_undefined(njs_rt, &njs_md->str_${s});`).join("\n")}
rjs_module_set_data(njs_rt, njs_mod, njs_md, njs_module_data_scan, njs_module_data_free);
${Array.from(this.propSet).map(s => `rjs_string_from_chars(njs_rt, &njs_md->str_${s}, "${s}", -1);`).join("\n")}
${Array.from(this.propSet).map(s => `rjs_property_name_init(njs_rt, &njs_md->pn_${s}, &njs_md->str_${s});`).join("\n")}`;
    };
};

/**
 * The function generator.
 */
class FuncGenerator {
    /**
     * The function's name
     * @type {string}
     */
    name;

    /**
     * Description string.
     * @type {string}
     */
    description;

    /**
     * Variable declaration code.
     * @type {string}
     */
    declCode = "";

    /**
     * Initialize code.
     * @type {string}
     */
    initCode = "";

    /**
     * Input parameter conversion code.
     * @type {string}
     */
    inputCode = "";

    /**
     * Parameter check code.
     * @type {string}
     */
    checkCode = "";

    /**
     * Body code.
     * @type {string}
     */
    bodyCode = "";

    /**
     * Output parameter conversion code.
     * @type {string}
     */
    outputCode = "";

    /**
     * Release code.
     * @type {string}
     */
    releaseCode = "";

    /**
     * Add declaration code.
     * @param {string} code The code string.
     */
    addDecl (code) {
        this.declCode += code;
    };

    /**
     * Add initialize code.
     * @param {string} code The code string.
     */
    addInit (code) {
        this.initCode += code;
    };

    /**
     * Add input parameter code.
     * @param {string} code The code string.
     */
    addInput (code) {
        this.inputCode += code;
    };

    /**
     * Add parameter check code.
     * @param {string} code The code string.
     */
    addCheck (code) {
        this.checkCode += code;
    };

    /**
     * Add body code.
     * @param {string} code The code string.
     */
    addBody (code) {
        this.bodyCode += code;
    };

    /**
     * Add output parameter code.
     * @param {string} code The code string.
     */
    addOutput (code) {
        this.outputCode += code;
    };

    /**
     * Add release code.
     * @param {string} code The code string.
     */
    addRelease (code) {
        this.releaseCode += code;
    };

    hasModuleData = false;

    /**
     * Add module data code.
     */
    moduleData () {
        if (this.hasModuleData)
            return;

        this.hasModuleData = true;

        this.valueDecl("njs_mod");
        this.addDecl(`NJS_ModuleData *njs_md;\n`);
        this.addInit(`\
/*Get module data.*/
rjs_get_function_module(njs_rt, njs_func, njs_mod);
njs_md = rjs_module_get_data(njs_rt, njs_mod);
`       );
    };

    /**
     * Add a value declaration.
     * @param {string} n The name of the variable.
     */
    valueDecl (n) {
        this.addDecl(`RJS_Value *${n} = rjs_value_stack_push(njs_rt);\n`);
    };

    /**
     * Add a variable declaration.
     * @param {CType} type The type of the variable.
     * @param {string} n The name of the variable.
     */
    varDecl (type, n) {
        this.addDecl(`${type.var(n)};\n`);
    };

    /**
     * Add a character buffer declaration.
     * @param {string} n The name of the variable.
     */
    charBufDecl (n) {
        this.addDecl(`RJS_CharBuffer ${n};\n`);
        this.addInit(`rjs_char_buffer_init(njs_rt, &${n});\n`);
        this.addRelease(`rjs_char_buffer_deinit(njs_rt, &${n});\n`);
    };

    /**
     * Create a new function generator.
     * @param {string} name The function's name.
     * @param {string} desc The description string.
     */
    constructor (name, desc) {
        this.name = name;
        this.description = desc;
    };

    /**
     * Get the function declaration string.
     * @returns {string} The declaration string.
     */
    funcDecl () {
        return `NJS_NATIVE_FUNC(${this.name})`;
    };

    /**
     * Invoke the function.
     * @param {string} fn The function's name.
     * @param {string} params The parameters list.
     * @param {string} ret The left return value.
     * @returns {string} The invoke string.
     */
    funcInvoke (fn, params, ret) {
        return `${ret}${fn}(${params});`;
    };

    /**
     * Get the code string.
     * @type {string}
     */
    get code () {
        if (this.initCode != "")
            this.initCode += "\n";
        if (this.inputCode != "")
            this.inputCode += "\n";
        if (this.checkCode != "")
            this.checkCode += "\n";
        if (this.bodyCode != "")
            this.bodyCode += "\n";
        if (this.outputCode != "")
            this.outputCode += "\n";

        return indent `\
/*${this.description}*/
static ${this.funcDecl()}
{
    size_t njs_top = rjs_value_stack_save(njs_rt);
    RJS_Result njs_r;
    ${this.declCode}

    ${this.initCode}
    ${this.inputCode}
    ${this.checkCode}
    ${this.bodyCode}
    ${this.outputCode}
    njs_r = RJS_OK;
    goto end;
end:
    ${this.releaseCode}
    rjs_value_stack_restore(njs_rt, njs_top);
    return njs_r;
}
`;
    }
};

/**
 * Javascript to FFI function generator.
 */
class JS2FFIFuncGenerator extends FuncGenerator {
    /**
     * Get the function declaration string.
     * @returns {string} The declaration string.
     */
    funcDecl () {
        return `RJS_Result\n${this.name} (RJS_Runtime *njs_rt, ffi_cif *njs_cif, RJS_Value *njs_args, size_t njs_argc, void *njs_cf, RJS_Value *njs_rv, void *njs_data)`;
    };

    /**
     * Add module data.
     */
    moduleData () {
        if (this.hasModuleData)
            return;

        this.hasModuleData = true;
        this.addDecl(`NJS_ModuleData *njs_md = njs_data;\n`);
    };

    /**
     * Get the function invoke code.
     * @param {string} fn The function's name.
     * @param {string} params The parameters list.
     * @param {string} ret The left return string.
     */
    funcInvoke (fn, params, ret) {
        return `ffi_call(njs_cif, njs_cf, njs_rp, njs_argp);`;
    };
};

/**
 * FFI to Javascript function generator.
 */
class FFI2JSFuncGenerator extends FuncGenerator {
    /**
     * Get the function declaration string.
     * @returns {string} The declaration string.
     */
    funcDecl () {
        return `RJS_Result\n${this.name} (RJS_Runtime *njs_rt, void **njs_argp, int njs_argc, RJS_Value *njs_fn, void *njs_rp, void *njs_data)`;
    };

    /**
     * Add module data.
     */
    moduleData () {
        if (this.hasModuleData)
            return;

        this.hasModuleData = true;
        this.addDecl(`NJS_ModuleData *njs_md = njs_data;\n`);
    };

    /**
     * Get the code string.
     * @type {string}
     */
    get code () {
        if (this.initCode != "")
            this.initCode += "\n";
        if (this.inputCode != "")
            this.inputCode += "\n";
        if (this.checkCode != "")
            this.checkCode += "\n";
        if (this.bodyCode != "")
            this.bodyCode += "\n";
        if (this.outputCode != "")
            this.outputCode += "\n";

        return indent `\
/*${this.description}*/
static ${this.funcDecl()}
{
    size_t njs_top = rjs_value_stack_save(njs_rt);
    RJS_Result njs_r;
    ${this.declCode}

    ${this.initCode}
    ${this.outputCode}
    ${this.checkCode}
    ${this.bodyCode}
    ${this.inputCode}
    njs_r = RJS_OK;
    goto end;
end:
    ${this.releaseCode}
    rjs_value_stack_restore(njs_rt, njs_top);
    return njs_r;
}
`;
    }
};

/**
 * Native javascript source generator.
 */
export class NjsGenerator {
    /**
     * The C context.
     * @type {CContext}
     */
    c;

    /**
     * Module data.
     * @type {ModuleData}
     */
    moduleData = new ModuleData();

    /**
     * Functions code.
     * @type {string}
     */
    funcCode = "";

    /**
     * Create a new NJS generator.
     * @param {CContext} ctxt The C context.
     */
    constructor (ctxt) {
        this.c = ctxt;
    };

    /**
     * Add function code.
     * @param {FuncGenerator} f The function generator.
     */
    addFuncCode (f) {
        if (this.funcCode != "")
            this.funcCode += "\n\n";
        this.funcCode += f.code;
    };

    /**
     * Add a constructor function.
     * @param {CBox} box The box data.
     */
    addConstructor (box) {
        let f = new FuncGenerator(`njs_${box.name}_constructor`, `Constructor of "${box.name}"`);

        f.moduleData();
        f.addDecl(`\
RJS_Value *njs_arg = rjs_argument_get(njs_rt, njs_args, njs_argc, 0);
size_t njs_nitem = 1;
RJS_Value *njs_init_obj = NULL;
RJS_CPtrType njs_cptr_type = RJS_CPTR_TYPE_VALUE;
int njs_cptr_flags = RJS_CPTR_FL_AUTO_FREE;
${box.cName} *njs_cptr;`
        );
        f.addInit(`\
if (rjs_value_is_number(njs_rt, njs_arg)) {
    /*Get the array's length.*/
    int64_t njs_len;
    if ((njs_r = rjs_to_length(njs_rt, njs_arg, &njs_len)) == RJS_ERR)
        goto end;
    njs_nitem = njs_len;
    njs_cptr_type = RJS_CPTR_TYPE_ARRAY;
    njs_arg = rjs_argument_get(njs_rt, njs_args, njs_argc, 1);
} else if (rjs_value_is_object(njs_rt, njs_arg)) {
    /*Get the initialize object.*/
    njs_init_obj = njs_arg;
    njs_arg = rjs_argument_get(njs_rt, njs_args, njs_argc, 1);
}

/*Check the automatically free flag.*/
if (!rjs_value_is_undefined(njs_rt, njs_arg)) {
    if (!rjs_to_boolean(njs_rt, njs_arg))
        njs_cptr_flags &= ~RJS_CPTR_FL_AUTO_FREE;
}
`       );
        f.addBody(`\
/*Allocate the ${box.name} buffer.*/
njs_cptr = malloc(sizeof(${box.cName}) * njs_nitem);
if (!njs_cptr) {
    njs_r = rjs_throw_range_error(njs_rt, "cannot allocate enough memory");
    goto end;
}
memset(njs_cptr, 0, njs_nitem * sizeof(${box.cName}));

/*Create the C pointer object.*/
if ((njs_r = rjs_create_c_ptr(njs_rt, njs_md->ctype_${box.name}, njs_cptr, njs_cptr_type, njs_nitem, njs_cptr_flags, njs_rv)) == RJS_ERR)
    goto end;

/*Initialize the ${box.name}.*/
if (njs_init_obj) {
    if ((njs_r = rjs_object_assign(njs_rt, njs_rv, njs_init_obj)) == RJS_ERR)
        goto end;
}
`       );

        this.addFuncCode(f);
    };

    /**
     * Add a getter function.
     * @param {CBox} box The box data.
     * @param {CMember} m The member.
     */
    addGetter (box, m) {
        let name;

        if (box.name == "$")
            name = `njs_${m.cName}_get`;
        else
            name = `njs_${box.name.replaceAll(".", "_")}_${m.cName}_get`;

        let f = new FuncGenerator(name, `Getter of "${box.name}.${m.name}"`);
        let cv;
        let length = box.type.length;

        if (box.name != "$") {
            f.moduleData();
            f.addDecl(`${box.cName} *njs_cptr;\n`);
            f.addInput(`\
/*Get the ${box.name} pointer.*/
if (!(njs_cptr = rjs_get_c_ptr(njs_rt, njs_md->ctype_${box.name}, RJS_CPTR_TYPE_VALUE, njs_thiz))) {
    njs_r = RJS_ERR;
    goto end;
}
`           );

            cv = `njs_cptr->${m.name}`;

            if (m.length !== undefined)
                length = box.resolveLength(m.length, "njs_cptr->");
        } else {
            cv = m.name;
        }

        let jsv = "njs_rv";

        f.addOutput(`/*Get "${m.name}".*/\n`);
        C2Js.convert(f, cv, jsv, {
            name: m.cName,
            type: m.type,
            length,
            nullTerminated: m.nullTerminated,
            mode: "getter"
        });

        this.addFuncCode(f);
    };

    /**
     * Add a setter function.
     * @param {CBox} box The box data.
     * @param {CMember} m The member.
     */
    addSetter (box, m) {
        let name;

        if (box.name == "$")
            name = `njs_${m.cName}_set`;
        else
            name = `njs_${box.name.replaceAll(".", "_")}_${m.cName}_set`;

        let f = new FuncGenerator(name, `Setter of "${box.name}.${m.name}"`);

        f.addDecl(`RJS_Value *njs_arg = rjs_argument_get(njs_rt, njs_args, njs_argc, 0);\n`);

        let cv;
        let length = box.type.length;

        if (box.name != "$") {
            f.moduleData();
            f.addDecl(`${box.cName} *njs_cptr;\n`);
            f.addInput(`\
/*Get the ${box.name} pointer.*/
if (!(njs_cptr = rjs_get_c_ptr(njs_rt, njs_md->ctype_${box.name}, RJS_CPTR_TYPE_VALUE, njs_thiz))) {
    njs_r = RJS_ERR;
    goto end;
}
`           );
            cv = `njs_cptr->${m.name}`;

            if (m.length !== undefined)
                length = box.resolveLength(m.length, "njs_cptr->");
        } else {
            cv = m.name;
        }

        let jsv = "njs_arg";

        f.addInput(`/*Set "${m.name}".*/\n`);
        Js2C.convert(f, jsv, cv, {
            name: m.cName,
            type: m.type,
            length,
            nullTerminated: m.nullTerminated,
            mode: "setter"
        });

        f.addBody("rjs_value_set_undefined(njs_rt, njs_rv);\n");

        this.addFuncCode(f);
    };

    /**
     * Build the function generator.
     * @param {FuncGenerator} f The function generator.
     * @param {CFunction} fn The function.
     */
    funcGen (f, fn) {
        let params = "";
        let needRetObj = false;

        /*Add parameters.*/
        let aid = 0;
        fn.parameters.forEach(p => {
            let v = `njs_arg_${p.name}`;
            let vd = v;
            let jsv = `njs_arg_v_${p.name}`;

            if (p.type.model == CTypeModel.ARRAY) {
                f.varDecl(p.type.itemType.pointer(), v);
            } else {
                f.varDecl(p.type, v);
            }

            let type = p.type;
            let length = p.length;

            if (length != undefined)
                length = fn.resolveLength(length, "njs_arg_");
            else
                length = type.length;

            if (p.direction != CDirection.IN) {
                needRetObj = true;

                type = type.valueType;

                vd = `njs_arg_d_${p.name}`;

                f.varDecl(type, vd);
                f.addInit(`${v} = &${vd};\n`);

                this.moduleData.propSet.add(p.name);

                f.addOutput(`/*Convert the output parameter "${p.name}".*/\n`);
                C2Js.convert(f, vd, "njs_ret_p", {
                    name: p.name,
                    type,
                    length,
                    nullTerminated: p.nullTerminated,
                    isNew: p.isNew,
                    isFree: p.isFree,
                    mode: "outp"
                });

                f.addOutput(`rjs_create_data_property_or_throw(njs_rt, njs_rv, &njs_md->pn_${p.name}, njs_ret_p);\n`);
            }

            if (p.direction != CDirection.OUT) {
                f.addDecl(`RJS_Value *${jsv} = rjs_argument_get(njs_rt, njs_args, njs_argc, ${aid++});\n`);
                f.addInput(`/*Convert input parameter "${p.name}".*/\n`);
                Js2C.convert(f, jsv, vd, {
                    name: p.name,
                    type,
                    length,
                    nullTerminated: p.nullTerminated,
                    isNew: p.isNew,
                    isFree: p.isFree,
                    mode: "inp"
                });
            }

            if (params != "")
                params += ", ";
            params += v;
        });

        if (needRetObj)
            f.moduleData();

        let retAssi = "";
        let ret = fn.return;
        if (ret) {
            f.varDecl(ret.type, "njs_ret");
            retAssi = "njs_ret = ";

            let length = ret.length;

            if (length != undefined)
                length = fn.resolveLength(length, "njs_arg_");
            else
                length = ret.type.length;

            let jsv = needRetObj ? "njs_ret_p" : "njs_rv";

            f.addOutput("/*Convert the return value.*/\n");
            if (ret.hasNegativeOne) {
                f.addOutput(`\
if (njs_ret == (${ret.type})-1) {
    rjs_value_set_number(njs_rt, ${jsv}, -1);
} else `);
            }

            f.addOutput("{\n");
            C2Js.convert(f, "njs_ret", jsv, {
                name: "ret",
                type: ret.type,
                length,
                nullTerminated: ret.nullTerminated,
                isNew: ret.isNew,
                isFree: ret.isFree,
                mode: "return"
            });
            f.addOutput("}\n")

            if (needRetObj) {
                this.moduleData.propSet.add("return");
                f.addOutput("rjs_create_data_property_or_throw(njs_rt, njs_rv, &njs_md->pn_return, njs_ret_p);\n");
            }
        } else if (!needRetObj) {
            f.addOutput("rjs_value_set_undefined(njs_rt, njs_rv);\n");
        }

        f.addBody(`\
/*Call "${fn.name}".*/
${f.funcInvoke(fn.name, params, retAssi)}
`       );

        if (needRetObj) {
            f.valueDecl("njs_ret_p");
            f.addBody(`\
/*Create return object.*/
rjs_ordinary_object_create(njs_rt, NULL, njs_rv);
`           );
        }
    };

    /**
     * Build the ffi2js function generator.
     * @param {FuncGenerator} f The function generator.
     * @param {CFunction} fn The function.
     */
    ffi2jsFuncGen (f, fn) {
        let aid = 0, jsAid = 0;
        let hasRetObj = false;

        fn.parameters.forEach(p => {
            let ty = p.type;
            let v = `njs_arg_${p.name}`;

            if (ty.model == CTypeModel.ARRAY)
                ty = ty.itemType.pointer();

            if (p.direction != CDirection.IN)
                hasRetObj = true;

            if (p.direction != CDirection.OUT) {
                f.varDecl(ty, v);
                f.addOutput(`/*Get parameter "${p.name}".*/\n`);
                f.addOutput(`${v} = *(${ty}*)njs_argp[${aid}];\n`);
            }

            aid ++;
        });

        if (hasRetObj) {
            f.valueDecl("njs_ret_p");
        }

        aid = 0;
        fn.parameters.forEach(p => {
            let ty = p.type;
            let v = `njs_arg_${p.name}`;
            let length = p.length;

            if (length != undefined)
                length = fn.resolveLength(length, "njs_arg_");
            else
                length = p.type.length;

            if (p.direction != CDirection.OUT) {
                f.addOutput(`/*Convert parameter "${p.name}".*/\n`);
                f.addOutput(`njs_arg = rjs_value_buffer_item(njs_rt, njs_args, ${jsAid ++});\n`);

                let cv = v;
                let cty = ty;

                if (p.direction == CDirection.INOUT) {
                    cv = `*${v}`;
                    cty = ty.valueType;
                }

                C2Js.convert(f, cv, "njs_arg", {
                    name: p.name,
                    type: cty,
                    length,
                    nullTerminated: p.nullTerminated,
                    isNew: p.isNew,
                    isFree: p.isFree,
                    mode: "ffi2js"
                });
            }

            if (p.direction != CDirection.IN) {
                f.addInput(`/*Convert output value of "${p.name}".*/\n`);

                let cv = `**(${p.type}*)njs_argp[${aid}]`;

                f.moduleData();
                f.addInput(`\
if ((njs_r = rjs_get_v(njs_rt, njs_rv, &njs_md->pn_${p.name}, njs_ret_p)) == RJS_ERR)
    goto end;
`               );

                Js2C.convert(f, "njs_ret_p", cv, {
                    name: p.name,
                    type: p.type.valueType,
                    length: length,
                    nullTerminated: p.nullTerminated,
                    isNew: p.isNew,
                    isFree: p.isFree,
                    mode: "ffi2js"
                });
            }

            aid ++;
        });

        f.addDecl(`RJS_Value *njs_args = rjs_value_stack_push_n(njs_rt, ${jsAid});\n`);

        f.addBody(`\
/*Call the JS function.*/
if ((njs_r = rjs_call(njs_rt, njs_fn, rjs_v_undefined(njs_rt), njs_args, ${jsAid}, njs_rv)) == RJS_ERR)
    goto end;
`       );

        let ret = fn.return;

        if (ret) {
            let length = ret.length;

            if (length != undefined)
                length = fn.resolveLength(length, "njs_arg_");
            else
                length = ret.type.length;

            let cv = `*(${ret.type}*)njs_rp`;
            let jsv;

            f.addInput(`/*Convert return value.*/\n`);

            if (hasRetObj) {
                f.moduleData();
                this.moduleData.propSet.add("return");
                f.addInput(`\
if ((njs_r = rjs_get_v(njs_rt, njs_rv, &njs_md->pn_return, njs_ret_p)) == RJS_ERR)
    goto end;
`               );
                jsv = "njs_ret_p";
            } else {
                jsv = "njs_rv";
            }

            Js2C.convert(f, jsv, cv, {
                name: "return",
                type: ret.type,
                length: length,
                nullTerminated: ret.nullTerminated,
                isNew: ret.isNew,
                mode: "ffi2js"
            });
        }
    };

    /**
     * Add a function.
     * @param {CFunction} fn The function.
     */
    addFunc (fn) {
        let f = new FuncGenerator(`njs_${fn.name}_nf`, `Wrapper of "${fn.name}"`);

        this.funcGen(f, fn);
        this.addFuncCode(f);
    };

    /**
     * Add accessors.
     * @param {CBox} box The box.
     * @param {Array} tab The accessors table.
     */
    addAccessors (box, tab) {
        box.memberMap.forEach(m => {
            let prefix;

            if (box.name == "$")
                prefix = m.cName;
            else
                prefix = `${box.name}_${m.cName}`;

            let get = `njs_${prefix}_get`;
            let set;

            this.addGetter(box, m);

            if (m.readonly) {
                set = "NULL";
            } else {
                set = `njs_${prefix}_set`;
                this.addSetter(box, m);
            }

            tab.push({box: box.name, member: m.name, get, set});

            if ((m.mType == CBoxType.STRUCT) || (m.mType == CBoxType.UNION)) {
                this.addAccessors(m.box, tab);
            }
        });
    };

    /**
     * Set FFI type.
     * @param {string} v The variable to store the FFI type.
     * @param {CType} ty The C type.
     * @returns {string} The code string.
     */
    setFFIType (v, ty) {
        let right;

        if ((ty.model == CTypeModel.POINTER) || (ty.model == CTypeModel.ARRAY)) {
            right = `&ffi_type_pointer`;
        } else if (ty.model == CTypeModel.PRIMITIVE) {
            switch (ty.name) {
            case "char":
                right = `&ffi_type_schar`;
                break;
            case "unsigned char":
                right = `&ffi_type_uchar`;
                break;
            case "short":
                right = `&ffi_type_sshort`;
                break;
            case "unsigned short":
                right = `&ffi_type_ushort`;
                break;
            case "int":
                right = `&ffi_type_sint`;
                break;
            case "unsigned int":
                right = `&ffi_type_uint`;
                break;
            case "long":
                right = `&ffi_type_slong`;
                break;
            case "unsigned long":
                right = `&ffi_type_ulong`;
                break;
            case "long long":
            case "int64_t":
                right = `&ffi_type_slong`;
                break;
            case "unsigned long":
            case "uint64_t":
                right = `&ffi_type_ulong`;
                break;
            case "size_t":
            case "ssize_t":
                right = `&ffi_type_pointer`;
                break;
            case "int8_t":
                right = `&ffi_type_sint8`;
                break;
            case "uint8_t":
                right = `&ffi_type_uint8`;
                break;
            case "int16_t":
                right = `&ffi_type_sint16`;
                break;
            case "uint16_t":
                right = `&ffi_type_uint16`;
                break;
            case "int32_t":
                right = `&ffi_type_sint32`;
                break;
            case "uint32_t":
                right = `&ffi_type_uint32`;
                break;
            }
        }

        return `${v} = ${right};\n`
    };

    /**
     * Add a function type.
     * @param {CFunction} ft The function type.
     */
    addFuncType (ft) {
        let pn = ft.parameters.size;
        let addCode = "";
        let js2ffi = new JS2FFIFuncGenerator(`njs_${ft.name}_js2ffi`, `"${ft.name}" js -> ffi.`);
        let ffi2js = new FFI2JSFuncGenerator(`njs_${ft.name}_ffi2js`, `"${ft.name}" ffi -> js.`);
        let constr = new FuncGenerator(`njs_${ft.name}_constructor`, `Constructor of "${ft.name}"`);

        js2ffi.addDecl(`\
void *njs_rp;
void *njs_argp[njs_cif->nargs];
`       );

        this.funcGen(js2ffi, ft);

        ffi2js.addDecl(`\
RJS_Value *njs_rv = rjs_value_stack_push(njs_rt);
RJS_Value *njs_arg;
`       );

        this.ffi2jsFuncGen(ffi2js, ft);

        let aid = 0;

        js2ffi.addInput(`/*Store arguments to ffi arguments pointer array.*/\n`);
        ft.parameters.forEach(p => {
            addCode += this.setFFIType(`njs_atypes[${aid}]`, p.type);

            js2ffi.addInput(`njs_argp[${aid}] = &njs_arg_${p.name};\n`);

            let v = `njs_arg_${p.name}`;
            let ty = p.type;
            if (p.type.model == CTypeModel.ARRAY) {
                ty = p.type.itemType.pointer();
            } else {
                ty = p.type;
            }

            aid ++;
        });

        let ret = ft.return;
        if (ret) {
            addCode += this.setFFIType("njs_rtype", ret.type);

            js2ffi.addInput(`njs_rp = &njs_ret;\n`);
        } else {
            addCode += `njs_rtype = &ffi_type_void;\n`
        }

        constr.addDecl(`\
RJS_Value *njs_arg = rjs_argument_get(njs_rt, njs_args, njs_argc, 0);
size_t njs_nitem = 1;
RJS_CPtrType njs_cptr_type = RJS_CPTR_TYPE_C_WRAPPER;
void *njs_cptr;
`       );

        constr.moduleData();
        constr.addBody(`\
if (rjs_value_is_number(njs_rt, njs_arg)) {
    /*Get the array's length.*/
    int64_t njs_len;
    int njs_cptr_flags = RJS_CPTR_FL_AUTO_FREE;

    if ((njs_r = rjs_to_length(njs_rt, njs_arg, &njs_len)) == RJS_ERR)
        goto end;
    njs_nitem = njs_len;
    njs_cptr_type = RJS_CPTR_TYPE_ARRAY;

    /*Allocate the ${ft.name} buffer.*/
    njs_cptr = malloc(sizeof(void*) * njs_nitem);
    if (!njs_cptr) {
        njs_r = rjs_throw_range_error(njs_rt, "cannot allocate enough memory");
        goto end;
    }
    memset(njs_cptr, 0, njs_nitem * sizeof(void*));

    njs_arg = rjs_argument_get(njs_rt, njs_args, njs_argc, 1);
    if (!rjs_to_boolean(njs_rt, njs_arg))
        njs_cptr_flags &= ~RJS_CPTR_FL_AUTO_FREE;

    /*Create the C pointer object.*/
    if ((njs_r = rjs_create_c_ptr(njs_rt, njs_md->ctype_${ft.name}, njs_cptr, njs_cptr_type, njs_nitem, njs_cptr_flags, njs_rv)) == RJS_ERR)
        goto end;
} else {
    if (!(njs_cptr = rjs_get_c_ptr(njs_rt, njs_md->ctype_${ft.name}, RJS_CPTR_TYPE_C_FUNC, njs_arg))) {
        njs_r = RJS_ERR;
        goto end;
    }

    rjs_value_copy(njs_rt, njs_rv, njs_arg);
}
`       );

        if (this.funcCode != "")
            this.funcCode += "\n\n";

        this.funcCode += indent `\
${js2ffi.code}

${ffi2js.code}

${constr.code}

/*Add function type "${ft.name}".*/
static void
njs_add_func_type_${ft.name} (RJS_Runtime *njs_rt, RJS_Value *njs_mod, RJS_CType **njs_pctype, void *njs_data)
{
    size_t     njs_top  = rjs_value_stack_save(njs_rt);
    RJS_Value *njs_name = rjs_value_stack_push(njs_rt);
    RJS_Value *njs_proto = rjs_value_stack_push(njs_rt);
    RJS_Value *njs_constr = rjs_value_stack_push(njs_rt);
    RJS_Realm *njs_realm = rjs_realm_current(njs_rt);
    ffi_type  *njs_rtype;
    ffi_type  *njs_atypes[${pn}];

    rjs_string_from_chars(njs_rt, njs_name, "${ft.name}", -1);
    ${addCode}
    rjs_create_c_func_type(njs_rt, njs_name, ${pn}, njs_rtype, njs_atypes, njs_${ft.name}_js2ffi, njs_${ft.name}_ffi2js, njs_data, njs_pctype);

    rjs_ordinary_object_create(njs_rt, rjs_realm_function_prototype(njs_realm), njs_proto);
    rjs_create_builtin_function(njs_rt, njs_mod, njs_${ft.name}_constructor, 1, njs_name, NULL, NULL, NULL, njs_constr);
    rjs_make_constructor(njs_rt, njs_constr, RJS_FALSE, njs_proto);
    rjs_module_add_binding(njs_rt, njs_mod, njs_name, njs_constr);

    rjs_value_stack_restore(njs_rt, njs_top);
}
`;
    };

    /**
     * Generate the source code.
     * @returns {string} The code string.
     */
    generate () {
        let exportTable = new ExportTable();
        let accessorTable = [];

        this.c.numSet.forEach(s => {
            exportTable.symbols.add(s);
        });

        this.c.boxMap.forEach(box => {
            if (box.name != "$")
                this.moduleData.cTypeSet.add(box.name);

            if (!box.noConstructor) {
                exportTable.symbols.add(box.name);
                this.addConstructor(box);
            } else if (box.name == "$") {
                exportTable.symbols.add(box.name);
            }

            this.addAccessors(box, accessorTable);
        });

        this.c.funcTypeMap.forEach(f => {
            exportTable.symbols.add(f.name);
            this.moduleData.funcTypeSet.add(f.name);
            this.addFuncType(f);
        });

        this.c.functions.forEach(f => {
            exportTable.symbols.add(f.name);
            this.addFunc(f);
        })

        return indent `\
/**
 * Generated by njsgen
 */

${slot("INCLUDE_HEAD")}
#include <string.h>
#include <stdlib.h>
#include <ratjs.h>
${slot("INCLUDE_TAIL")}

#define NJS_NATIVE_FUNC(n)\\
    RJS_Result n (RJS_Runtime *njs_rt, RJS_Value *njs_func, RJS_Value *njs_thiz, RJS_Value *njs_args, size_t njs_argc, RJS_Value *njs_nt, RJS_Value *njs_rv)

${slot("DECL")}

${exportTable.declCode}

${this.moduleData.declCode}

${slot("FUNC_HEAD")}

${this.funcCode}

${slot("FUNC_TAIL")}

/*Add a number symbol.*/
static void
njs_add_number (RJS_Runtime *njs_rt, RJS_Value *njs_mod, const char *njs_ncstr, RJS_Number njs_n)
{
    size_t njs_top = rjs_value_stack_save(njs_rt);
    RJS_Value *njs_name = rjs_value_stack_push(njs_rt);
    RJS_Value *njs_v = rjs_value_stack_push(njs_rt);

    rjs_string_from_chars(njs_rt, njs_name, njs_ncstr, -1);
    rjs_value_set_number(njs_rt, njs_v, njs_n);
    rjs_module_add_binding(njs_rt, njs_mod, njs_name, njs_v);

    rjs_value_stack_restore(njs_rt, njs_top);
}

/*Add structure or union object.*/
static void
njs_add_box (RJS_Runtime *njs_rt, RJS_Value *njs_mod, const char *njs_ncstr, size_t njs_size, RJS_Value *njs_proto, RJS_NativeFunc njs_constr_nf)
{
    rjs_ordinary_object_create(njs_rt, NULL, njs_proto);

    if (njs_constr_nf) {
        size_t njs_top = rjs_value_stack_save(njs_rt);
        RJS_Value *njs_name = rjs_value_stack_push(njs_rt);
        RJS_Value *njs_constr = rjs_value_stack_push(njs_rt);
        RJS_Value *njs_sizep = rjs_value_stack_push(njs_rt);
        RJS_PropertyName njs_pn;

        rjs_string_from_chars(njs_rt, njs_name, njs_ncstr, -1);
        rjs_create_builtin_function(njs_rt, njs_mod, njs_constr_nf, 1, njs_name, NULL, NULL, NULL, njs_constr);
        rjs_make_constructor(njs_rt, njs_constr, RJS_FALSE, njs_proto);
        rjs_module_add_binding(njs_rt, njs_mod, njs_name, njs_constr);

        rjs_string_from_chars(njs_rt, njs_name, "size", -1);
        rjs_value_set_number(njs_rt, njs_sizep, njs_size);
        rjs_property_name_init(njs_rt, &njs_pn, njs_name);
        rjs_create_data_property(njs_rt, njs_constr, &njs_pn, njs_sizep);
        rjs_property_name_deinit(njs_rt, &njs_pn);

        rjs_value_stack_restore(njs_rt, njs_top);
    } else if (!strcmp(njs_ncstr, "$")) {
        size_t njs_top = rjs_value_stack_save(njs_rt);
        RJS_Value *njs_name = rjs_value_stack_push(njs_rt);

        rjs_string_from_chars(njs_rt, njs_name, njs_ncstr, -1);
        rjs_module_add_binding(njs_rt, njs_mod, njs_name, njs_proto);

        rjs_value_stack_restore(njs_rt, njs_top);
    }
}

/*Add a C type.*/
static void
njs_add_ctype (RJS_Runtime *njs_rt, const char *njs_ncstr, size_t njs_size, RJS_Value *njs_proto, RJS_CType **njs_pctype)
{
    size_t njs_top = rjs_value_stack_save(njs_rt);
    RJS_Value *njs_name = rjs_value_stack_push(njs_rt);

    rjs_string_from_chars(njs_rt, njs_name, njs_ncstr, -1);
    rjs_create_c_type(njs_rt, njs_name, njs_size, njs_proto, njs_pctype);

    rjs_value_stack_restore(njs_rt, njs_top);
}

/*Add member accessor.*/
static void
njs_add_accessor (RJS_Runtime *njs_rt, RJS_Value *njs_mod, RJS_Value *njs_proto, const char *njs_ncstr, RJS_NativeFunc njs_get, RJS_NativeFunc njs_set)
{
    size_t njs_top = rjs_value_stack_save(njs_rt);
    RJS_Value *njs_name = rjs_value_stack_push(njs_rt);

    rjs_string_from_chars(njs_rt, njs_name, njs_ncstr, -1);
    rjs_create_builtin_accessor(njs_rt, njs_mod, njs_proto, njs_get, njs_set, njs_name, NULL);

    rjs_value_stack_restore(njs_rt, njs_top);
}

/*Add function.*/
static void
njs_add_func (RJS_Runtime *njs_rt, RJS_Value *njs_mod, const char *njs_ncstr, RJS_NativeFunc njs_nf, size_t njs_pn)
{
    size_t njs_top = rjs_value_stack_save(njs_rt);
    RJS_Value *njs_name = rjs_value_stack_push(njs_rt);
    RJS_Value *njs_func = rjs_value_stack_push(njs_rt);

    rjs_string_from_chars(njs_rt, njs_name, njs_ncstr, -1);
    rjs_create_builtin_function(njs_rt, njs_mod, njs_nf, njs_pn, njs_name, NULL, NULL, NULL, njs_func);
    rjs_module_add_binding(njs_rt, njs_mod, njs_name, njs_func);

    rjs_value_stack_restore(njs_rt, njs_top);
}

/*Set the export symbols.*/
RJS_Result
ratjs_module_init (RJS_Runtime *njs_rt, RJS_Value *njs_mod)
{
    /*Add export entries.*/
    rjs_module_set_import_export(njs_rt, njs_mod, NULL, njs_local_exports, NULL, NULL);

    ${slot("MODULE_INIT")}

    return RJS_OK;
}   

/*Initialize the local bindings.*/
RJS_Result
ratjs_module_exec (RJS_Runtime *njs_rt, RJS_Value *njs_mod)
{
    size_t njs_top = rjs_value_stack_save(njs_rt);
    ${Array.from(this.c.boxMap.values()).map(b => {
        let n = (b.name == "$") ? "njs_vars" : `njs_${b.name}_proto`;
        return `RJS_Value *${n} = rjs_value_stack_push(njs_rt);`
    }).join("\n")}
    ${this.moduleData.initCode}

    /*Set number symbols.*/
    ${Array.from(this.c.numSet).map(s => `njs_add_number(njs_rt, njs_mod, "${s}", ${s});`).join("\n")}

    /*Add box objects.*/
    ${Array.from(this.c.boxMap.values()).map(b => {
        let constr = b.noConstructor ? "NULL" : `njs_${b.name}_constructor`;
        let size = (b.name == "$") ? "0" : `sizeof(${b.cName})`;
        let proto = (b.name == "$") ? "njs_vars" : `njs_${b.name}_proto`;
        return `njs_add_box(njs_rt, njs_mod, "${b.name}", ${size}, ${proto}, ${constr});`
    }).join("\n")}

    /*Add C types.*/
    ${Array.from(this.c.boxMap.values()).filter(b => b.name != "$").map(
        b => `njs_add_ctype(njs_rt, "${b.name}", sizeof(${b.cName}), njs_${b.name}_proto, &njs_md->ctype_${b.name});`
    ).join("\n")}

    /*Add accessors.*/
    ${accessorTable.map(a => {
        let proto = (a.box == "$") ? "njs_vars" : `njs_${a.box}_proto`;
        return `njs_add_accessor(njs_rt, njs_mod, ${proto}, "${a.member}", ${a.get}, ${a.set});`
    }).join("\n")}

    /*Add function types.*/
    ${Array.from(this.c.funcTypeMap.values()).map(f => `njs_add_func_type_${f.name}(njs_rt, njs_mod, &njs_md->ctype_${f.name}, njs_md);`).join("\n")}

    /*Add functions.*/
    ${this.c.functions.map(f => `njs_add_func(njs_rt, njs_mod, "${f.name}", njs_${f.name}_nf, ${f.parameters.size});`).join("\n")}

    ${slot("MODULE_EXEC")}

    rjs_value_stack_restore(njs_rt, njs_top);
    return RJS_OK;
}
`
    };
};
