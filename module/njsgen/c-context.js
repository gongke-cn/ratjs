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
 * C context.
 * @module c-context
 */

import { Log } from "../log";

let log = new Log("c-context");

//log.enable = false;

/**
 * C data type model.
 * @typedef {number} CTypeModel
 * @type {number}
 * @property {number} VOID void.
 * @property {number} PRIMITIVE Primitive data.
 * @property {number} POINTER Pointer.
 * @property {number} ARRAY Array.
 * @property {number} FUNCTION Function.
 * @property {number} BOX Structure or union.
 */
export const CTypeModel = {
    VOID:      0,
    PRIMITIVE: 1,
    POINTER:   2,
    ARRAY:     3,
    FUNCTION:  4,
    BOX:       5
};

/**
 * C data type.
 */
export class CType {
    /**
     * Model.
     * @type {CTypeModel}
     */
    model;

    /**
     * The name of the C type.
     * @type {string}
     */
    name;

    /**
     * Create a new C type.
     * @param {CContext} ctxt The C context.
     * @param {string} str The C type string.
     */
    constructor (ctxt, str) {
        let m;

        if (str == undefined)
            return;

        str = str.trim();

        m = str.match(/(.+)\[(.*)\]/);
        if (m != null) {
            this.model  = CTypeModel.ARRAY;
            this.length = m[2].trim();
            this.itemType = new CType(ctxt, m[1]);
            return;
        }

        m = str.match(/(.+)\*\s*const/);
        if (m != null) {
            this.model   = CTypeModel.POINTER;
            this.isConst = true;
            this.valueType = new CType(ctxt, m[1]);
            return;
        }

        m = str.match(/(.+)\*/);
        if (m != null) {
            this.model = CTypeModel.POINTER;
            this.valueType = new CType(ctxt, m[1]);
            return;
        }

        m = str.match(/const\s+(.+)/);
        if (m != null) {
            this.isConst = true;
            str = m[1].trim();
        }

        if (ctxt.primTypeSet.has(str)) {
            this.model = CTypeModel.PRIMITIVE;
            this.name = str;
        } else if (this.funcType = ctxt.funcTypeMap.get(str)) {
            this.model = CTypeModel.FUNCTION;
            this.name = str;
        } else if (str == "void") {
            this.model = CTypeModel.VOID;
            this.name = str;
        } else {
            m = str.match(/enum\s+/);
            if (m != null) {
                this.model = CTypeModel.PRIMITIVE;
                this.name = str;
            } else {
                let boxType;

                m = str.match(/(struct|union)\s+(.+)/);
                if (m != null) {
                    str = m[2];

                    if (m[1] == "struct")
                        boxType = CBoxType.STRUCT;
                    else
                        boxType = CBoxType.UNION;
                }

                let box = ctxt.boxMap.get(str);
                
                if (box) {
                    if (boxType != undefined) {
                        if ((box.type != boxType) || (!box.noTypeDef && (boxType === undefined)))
                            throw new TypeError(`unknown type "${str}"`);
                    }

                    this.model = CTypeModel.BOX;
                    this.name = str;
                    this.box = box;
                } else {
                    throw new TypeError(`unknown type "${str}"`);
                }
            }
        }
    };

    /**
     * Declare a variable 
     * @param {string} vn The name of the variable.
     * @param {string} pre The prefixed string.
     * @param {string} post The postfixed string.
     * @returns {string} The variable declaration string.
     */
    var (vn, pre = "", post = "") {
        let r, cf = "";

        if (this.isConst)
            cf = "const ";

        switch (this.model) {
        case CTypeModel.VOID:
            r = `${cf}void${pre} ${vn}${post}`;
            break;
        case CTypeModel.PRIMITIVE:
            r = `${cf}${this.name}${pre} ${vn}${post}`;
            break;
        case CTypeModel.POINTER:
            let ptrPre = this.isConst ? "* const" : "*";

            if (pre != "")
                pre = `${ptrPre}${pre}`;
            else
                pre = ptrPre;

            r = this.valueType.var(vn, pre, post);
            break;
        case CTypeModel.ARRAY:
            post = `[${this.length}]${post}`;
            r = this.itemType.var(vn, pre, post);
            break;
        case CTypeModel.FUNCTION:
            r = `${cf}${this.name}${pre} ${vn}${post}`;
            break;
        case CTypeModel.BOX:
            let bn;
            
            if (this.box.noTypeDef) {
                if (this.box.type == CBoxType.STRUCT)
                    bn = `struct ${this.name}`;
                else
                    bn = `union ${this.name}`;
            } else {
                bn = this.name;
            }

            r = `${cf}${bn}${pre} ${vn}${post}`;
            break;
        }

        return r;
    };

    /**
     * Convert the C type to string.
     * @returns {string}
     */
    toString (pre = "", post = "") {
        let r, cf = "";

        if (this.isConst)
            cf = "const ";

        switch (this.model) {
        case CTypeModel.VOID:
            r = `${cf}void${pre}${post}`;
            break;
        case CTypeModel.PRIMITIVE:
            r = `${cf}${this.name}${pre}${post}`;
            break;
        case CTypeModel.POINTER:
            let ptrPre = this.isConst ? "* const" : "*";

            if (pre != "")
                pre = `${ptrPre}${pre}`;
            else
                pre = ptrPre;

            r = this.valueType.toString(pre, post);
            break;
        case CTypeModel.ARRAY:
            post = `[${this.length}]${post}`;
            r = this.itemType.toString(pre, post);
            break;
        case CTypeModel.FUNCTION:
            r = `${cf}${this.name}${pre}${post}`;
            break;
        case CTypeModel.BOX:
            let bn;
            
            if (this.box.noTypeDef) {
                if (this.box.type == CBoxType.STRUCT)
                    bn = `struct ${this.name}`;
                else
                    bn = `union ${this.name}`;
            } else {
                bn = this.name;
            }

            r = `${cf}${bn}${pre}${post}`;
            break;
        }

        return r;
    };

    /**
     * Get the pointer type.
     * @returns {CType} The pointer type.
     */
    pointer () {
        let pty = new CType();

        pty.model = CTypeModel.POINTER;
        pty.valueType = this;

        return pty;
    };
};

/**
 * Box data type.
 * @typedef {number} CBoxType
 * @type {CBoxType}
 * @property {number} STRUCT Structure declaration.
 * @property {number} UNION Union declaration.
 * @property {number} VALUE Value.
 */
export const CBoxType = {
    STRUCT: 0,
    UNION:  1,
    VALUE:  2,
    VARS:   3
};

/**
 * Box member.
 */
class CBoxMember {
    /**
     * Name of the member.
     * @type {string}
     */
    name;

    /**
     * The C name of the member.
     * @type {string}
     */
    cName;

    /**
     * The data type of the member.
     * @type {string}
     */
    type;

    /**
     * Length of the array member.
     * @type {string}
     */
    length;

    /**
     * The member is null terminated array.
     * @type {boolean}
     */
    nullTerminated;

    /**
     * Create a new member.
     * @param {CContext} ctxt The C context.
     * @param {CBox} box The box contains this member.
     * @param {string} name The name of the member.
     * @param {object} decl The member's declaration.
     */
    constructor (ctxt, box, name, decl) {
        this.name = name;
        this.cName = name.replaceAll(".", "_");

        if (box.readonly)
            this.readonly = box.readonly;

        if (typeof decl == "string") {
            this.type = decl;
        } else if (decl.type !== undefined) {
            this.type = decl.type;
            if (decl.readonly !== undefined)
                this.readonly = decl.readonly;
            if (decl.length != undefined)
                this.length = decl.length;
            if (decl.nullTerminated != undefined)
                this.nullTerminated = decl.nullTerminated;
        }

        box.memberMap.set(name, this);
    };
};

/**
 * Box data declaration.
 */
class CBox {
    /**
     * Box type.
     * @type {CBoxType}
     */
    type;

    /**
     * Name of the declaration.
     * @type {string}
     */
    name;

    /**
     * C name.
     * @type {string}
     */
    cName;

    /**
     * Members of the data are readonly.
     * @type {boolean}
     */
    readonly;

    /**
     * The box has not constructor.
     * @type {boolean}
     */
    noConstructor;

    /**
     * Members map.
     * @type {Map<string, CBoxMember>}
     */
    memberMap = new Map();

    /**
     * Add a members.
     * @param {CContext} ctxt The C context.
     * @param {string} prefix The name prefix string.
     * @param {object} members The members object. 
     */
    addMembers (ctxt, prefix, members) {
        if (members) {
            for (let [mn, md] of Object.entries(members)) {
                let mp;

                if (prefix !== undefined)
                    mp = `${prefix}.${mn}`;
                else
                    mp = mn;

                if (md.struct || md.union) {
                    let members = md.struct ? md.struct : md.union;
                    this.addMembers(ctxt, mp, members);
                } else {
                    new CBoxMember(ctxt, this, mp, md);
                }
            }
        }
    };

    /**
     * Create a new box declaration.
     * @param {CContext} ctxt The C context.
     * @param {CBoxType} type Box data type. 
     * @param {string} name name of the box declaration. 
     * @param {object} decl Declaration object.
     */
    constructor (ctxt, type, name, decl) {
        if (ctxt.boxMap.get(name))
            throw new TypeError(`"${name}" is already defined`);

        ctxt.boxMap.set(name, this);

        this.type = type;
        this.name = name;

        if (decl !== undefined) {
            this.readonly = decl.readonly;

            if (decl.noConstructor)
                this.noConstructor = decl.noConstructor;

            if (decl.noTypeDef) {
                if (type == CBoxType.STRUCT)
                    this.cName = `struct ${name}`;
                else
                    this.cName = `union ${name}`;
                this.noTypeDef = true;
            } else {
                this.cName = name;
            }

            if (decl.members) {
                for (let [mn, md] of Object.entries(decl.members)) {
                    if (md.struct || md.union) {
                        let members = md.struct ? md.struct : md.union;
                        this.addMembers(ctxt, mn, members);
                    } else {
                        new CBoxMember(ctxt, this, mn, md);
                    }
                }
            }
        }
    };

    /**
     * Resolve the length string.
     * @param {string} src The source length string.
     * @param {string} prefix The prefix string.
     * @returns {string} The result length string.
     */
    resolveLength (src, prefix) {
        return src.replaceAll(/\w+/g, s => {
            if (this.memberMap.has(s))
                return `${prefix}${s}`;
            else
                return s;
        });
    };
};

/**
 * Parameter direction.
 * @typedef {number} CDirection
 * @type {CDirection}
 * @property {number} IN Input parameter.
 * @property {number} OUT Output parameter.
 * @property {number} INOUT Input and output parameter.
 */
export const CDirection = {
    IN:    0,
    OUT:   1,
    INOUT: 3
};

/**
 * C parameter.
 */
class CParameter {
    /**
     * Name of the parameter.
     * @type {string}
     */
    name;

    /**
     * Data type of the parameter.
     * @type {string}
     */
    type;

    /**
     * Length of the array parameter.
     * @type {string}
     */
    length;

    /**
     * Parameter's direction.
     * @type {CDirection}
     */
    direction = CDirection.IN;

    /**
     * Is null terminated array.
     * @type {boolean}
     */
    nullTerminated;

    /**
     * Output a new object.
     * @type {boolean}
     */
    isNew;

    /**
     * Free the input object.
     * @type {boolean}
     */
    isFree;

    /**
     * -1 is a special value.
     * @type {boolean}
     */
    hasNegativeOne;

    /**
     * Create a new parameter.
     * @param {CContext} ctxt The C context.
     * @param {CFunction} func The function has this parameter.
     * @param {string} name The name of this parameter.
     * @param {object} decl The parameter's declaration.
     */
    constructor (ctxt, func, name, decl) {
        if (name === undefined)
            this.name = "return";
        else
            this.name = name;

        if (typeof decl == "string") {
            this.type = decl;
        } else {
            this.type = decl.type;

            if (decl.direction !== undefined) {
                switch (decl.direction) {
                case "in":
                    this.direction = CDirection.IN;
                    break;
                case "out":
                    this.direction = CDirection.OUT;
                    break;
                case "inout":
                    this.direction = CDirection.INOUT;
                    break;
                }
            } else if (name == undefined) {
                this.direction = CDirection.OUT;
            }

            if (decl.nullTerminated)
                this.nullTerminated = true;
            if (decl["-1"])
                this.hasNegativeOne = true;
            if (decl["new"])
                this.isNew = true;
            if (decl.free)
                this.isFree = true;
            if (decl.length)
                this.length = decl.length;
        }

        if (name === undefined)
            func.return = this;
        else
            func.parameters.set(name, this);
    };
};

/**
 * C function.
 */
class CFunction {
    /**
     * The function's name.
     * @type {string}
     */
    name;

    /**
     * Parameters array.
     * @type {Map<string, CParameter>}
     */
    parameters = new Map();

    /**
     * Return value.
     * @type {CParameter}
     */
    return;

    /**
     * Create a new C function.
     * @param {CContext} ctxt The C context.
     * @param {string} name The function's name.
     * @param {object} decl The function's declaration object.
     * @param {boolean} isType The object is a function type.
     */
    constructor (ctxt, name, decl, isType = false) {
        this.name = name;

        if (decl.parameters) {
            for (let [n, p] of Object.entries(decl.parameters)) {
                new CParameter(ctxt, this, n, p);
            }
        }

        if (decl.return) {
            new CParameter(ctxt, this, undefined, decl.return);
        }

        if (isType) {
            ctxt.funcTypeMap.set(name, this);
        } else
            ctxt.functions.push(this);
    };

    /**
     * Resolve the length string.
     * @param {string} src The source length string.
     * @param {string} prefix The prefix string.
     * @returns {string} The result length string.
     */
    resolveLength (src, prefix) {
        return src.replaceAll(/\w+/g, s => {
            if (this.parameters.has(s))
                return `${prefix}${s}`;
            else
                return s;
        });
    };
};

/**
 * C context.
 */
export class CContext {
    /**
     * Primitive type set.
     * @type {Set<string>}
     */
    primTypeSet = new Set();

    /**
     * Number symbols set.
     * @type {Set<string>}
     */
    numSet = new Set();

    /**
     * Box declaration map.
     * @type {Map<string, object>}
     */
    boxMap = new Map();

    /**
     * Function type map.
     * @type {Map<string, CFunction>}
     */
    funcTypeMap = new Map();

    /**
     * Functions array.
     * @type {Array.<CFunction>}
     */
    functions = [];

    /**
     * Add a number symbol.
     * @param {string} n The symbol's name.
     */
    addNumber (n) {
        if (this.numSet.has(n))
            throw new TypeError(`"${n}" is already defined`);

        this.numSet.add(n);
    };

    /**
     * Add box declarations.
     * @param {CBoxType} type Box data type.
     * @param {object} decls Declarations.
     */
    addBoxes (type, decls) {
        if (decls === undefined)
            return;

        for (let [n, decl] of Object.entries(decls)) {
            new CBox(this, type, n, decl);
        }
    };

    /**
     * Add a function type.
     * @param {string} n The name of the function.
     * @param {object} decl The function declaration.
     */
    addFuncType (n, decl) {
        new CFunction(this, n, decl, true);
    };

    /**
     * Add a function.
     * @param {string} n The name of the function.
     * @param {object} decl The function declaration.
     */
    addFunction (n, decl) {
        new CFunction(this, n, decl);
    };

    /**
     * Resolve the box members' types.
     * @param {CBox} box The box.
     */
    resolveBoxMemberTypes (box) {
        box.memberMap.forEach(m => {
            m.type = new CType(this, m.type);

            if (m.type.isConst)
                m.readonly = true;
        });
    };

    /**
     * Create a new C context.
     * @param {object} decl The declaration object.
     */
    constructor (decl) {
        let primTypes = ["char", "short", "long", "long long",
                "signed char", "signed short", "signed long", "signed long long",
                "unsigned char", "unsigned short", "unsigned int", "unsigned long", "unsigned long long",
                "int", "int8_t", "int16_t", "int32_t", "int64_t",
                "uint8_t", "uint16_t", "uint32_t", "uint64_t",
                "size_t", "ssize_t", "float", "double", "bool", "RJS_Result", "RJS_Bool"];

        /*Add basic primitive types.*/
        for (let t of primTypes)
            this.primTypeSet.add(t);

        /*Add numer type macros.*/
        if (decl.numberMacros) {
            for (let n of decl.numberMacros)
                this.addNumber(n)
        }

        /*Add enumerations.*/
        if (decl.enumerations) {
            for (let [n, items] of Object.entries(decl.enumerations)) {
                this.primTypeSet.add(n);

                for (let i of items) {
                    this.addNumber(i);
                }
            }
        }

        /*Add box datas.*/
        this.addBoxes(CBoxType.STRUCT, decl.structures);
        this.addBoxes(CBoxType.UNION, decl.unions);

        /*Add function types.*/
        if (decl.functionTypes) {
            for (let [n, f] of Object.entries(decl.functionTypes)) {
                this.addFuncType(n, f);
            }
        }

        /*Add functions.*/
        if (decl.functions) {
            for (let [n, f] of Object.entries(decl.functions)) {
                this.addFunction(n, f);
            }
        }

        /*Add variables.*/
        if (decl.variables) {
            let varBox = new CBox(this, CBoxType.VARS, "$");
            
            varBox.noConstructor = true;

            for (let [vn, vd] of Object.entries(decl.variables)) {
                new CBoxMember(this, varBox, vn, vd);
            }
        }

        /*Resolve the data types.*/
        this.boxMap.forEach(box => {
            this.resolveBoxMemberTypes(box);
        });

        for (let ft of this.funcTypeMap.values()) {
            ft.parameters.forEach(p => {
                p.type = new CType(this, p.type);

                let dir = CDirection.IN;

                if (p.type.model == CTypeModel.POINTER) {
                    let vty = p.type.valueType;

                    if (((vty.model == CTypeModel.PRIMITIVE) || (vty.model == CTypeModel.POINTER))
                            && (p.length == undefined)) {
                        dir = p.direction;
                    }
                }

                p.direction = dir;
            });

            let ret = ft.return;
            if (ret) {
                ret.type = new CType(this, ret.type);
            }
        }

        for (let func of this.functions) {
            func.parameters.forEach(p => {
                p.type = new CType(this, p.type);

                let dir = CDirection.IN;

                if (p.type.model == CTypeModel.POINTER) {
                    let vty = p.type.valueType;

                    if (((vty.model == CTypeModel.PRIMITIVE) || (vty.model == CTypeModel.POINTER))
                            && (p.length == undefined)) {
                        dir = p.direction;
                    }
                }

                p.direction = dir;
            });

            let ret = func.return;
            if (ret) {
                ret.type = new CType(this, ret.type);
            }
        }
    };
};
