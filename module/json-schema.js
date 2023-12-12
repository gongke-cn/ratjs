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
 * JSON schema.
 * @module json-schema
 */

import { Log } from "./log";

let log = new Log("json-schema");

//log.enable = false;

/**
 * JSON schema validate error.
 */
export class ValidateError extends Error {
};

/**
 * JSON schema validate context.
 * @private
 */
class JSONSchemaContext {
    /**
     * JSON schema object.
     * @type {Object}
     */
    schema;

    /**
     * Tag of current value.
     * @type {string}
     */
    tag;

    /**
     * Error message.
     * @type {string}
     */
    message;

    /**
     * Create a new JSON schema validate context.
     * @param {Object} sch JSON schema object.
     * @param {string} [tag] The tag of the current value.
     */
    constructor (sch, tag) {
        this.schema = sch;
        this.tag    = tag;
    };

    /**
     * Validate failed.
     * @param {string} msg Error message.
     */
    fail (msg) {
        let str = `${this.tag} validate failed: ${msg}`;

        this.message = str;
    };

    /**
     * Validate a number.
     * @param {number} v Number value.
     * @returns {boolean} true when validate success, false when validate failed.
     */
    numberValidate (v) {
        let n;

        n = this.schema.multipleOf;
        if (n != undefined) {
            if (n <= 0) {
                this.fail('"multipleOf" value must > 0');
                return false;
            }

            if (v % n != 0) {
                this.fail(`${v} is not multiple of ${n}`);
                return false;
            }
        }

        n = this.schema.maximum;
        if (n != undefined) {
            if (v > n) {
                this.fail(`${v} > maximum value ${n}`);
                return false;
            }
        }

        n = this.schema.exclusiveMaximum;
        if (n != undefined) {
            if (v >= n) {
                this.fail(`${v} >= exclusive maximum value ${n}`);
                return false;
            }
        }

        n = this.schema.minimum;
        if (n != undefined) {
            if (v < n) {
                this.fail(`${v} < minimum value ${n}`);
                return false;
            }
        }

        n = this.schema.exclusiveMinimum;
        if (n != undefined) {
            if (v <= n) {
                this.fail(`${v} <= exclusive minimum value ${n}`);
                return false;
            }
        }

        return true;
    };

    /**
     * String type value validate.
     * @param {string} v String value.
     * @returns {boolean} true when validate success, false when validate failed.
     */
    stringValidate (v) {
        let len;

        len = this.schema.maxLength;
        if (len != undefined) {
            if (v.length > len) {
                this.fail(`length of "${v}" > maximum length ${len}`);
                return false;
            }
        }

        len = this.schema.minLength;
        if (len != undefined) {
            if (v.length < len) {
                this.fail(`length of "${v}" < minimum length ${len}`);
                return false;
            }
        }

        let pat = this.schema.pattern;
        if (pat != undefined) {
            let mr = v.match(new RegExp(this.schema.pattern));
            if ((mr == null) || (mr[0] != v)) {
                this.fail(`"${v}" mismatch the pattern "${pat}"`);
                return false;
            }
        }

        return true;
    };

    /**
     * Array type value validate
     * @param {Array} v Array value.
     * @returns {boolean} true when validate success, false when validate failed.
     */
    arrayValidate (v) {
        let items;
        let idx = 0;

        items = this.schema.prefixItems;
        if (items != undefined) {
            for (let item of items) {
                let ctxt = new JSONSchemaContext(item, `${this.tag}[${idx}]`);

                if (!ctxt.validate(v[idx])) {
                    this.message = ctxt.message;
                    return false;
                }

                idx ++;
            }
        }

        items = this.schema.items;
        if (items != undefined) {
            let ctxt = new JSONSchemaContext(items);

            while (idx < v.length) {
                ctxt.tag = `${this.tag}[${idx}]`;
                if (!ctxt.validate(v[idx])) {
                    this.message = ctxt.message;
                    return false;
                }

                idx ++;
            }
        }

        items = this.schema.unevaluatedItems;
        if (items != undefined) {
            let ctxt = new JSONSchemaContext(items);

            while (idx < v.length) {
                ctxt.tag = `${this.tag}[${idx}]`;
                if (!ctxt.validate(v[idx])) {
                    this.message = ctxt.message;
                    return false;
                }

                idx ++;
            }
        }

        items = this.schema.maxItems;
        if (items != undefined) {
            if (v.length > items) {
                this.fail(`length of array > maximum items number ${items}`);
                return false;
            }
        }

        items = this.schema.minItems;
        if (items != undefined) {
            if (v.length < items) {
                this.fail(`length of array < minimum items number ${items}`);
                return false;
            }
        }

        let unique = this.schema.uniqueItems;
        if ((unique != undefined) && unique) {
            let set = new Set();

            for (let item of v) {
                if (set.has(item)) {
                    this.fail(`array's items are not unique`);
                    return false;
                }
                set.add(item);
            }
        }

        let contains = this.schema.contains;
        if (contains != undefined) {
            let ctxt = new JSONSchemaContext(contains);
            let cn = 0;
            let i  = 0;

            for (let item of v) {
                ctxt.tag = `${this.tag}[${i}]`;
                if (ctxt.validate(item))
                    cn ++;
            }

            let max = this.schema.maxContains;
            if (max != undefined) {
                if (cn > max) {
                    this.fail(`contains number ${cn} > maximum contains number ${max}`);
                    return false;
                }
            }

            let min = this.schema.minContains;
            if (min != undefined) {
                if (cn < min) {
                    this.fail(`contains number ${cn} < minimum contains number ${min}`);
                    return false;
                }
            }
        }

        return true;
    };

    /**
     * Object type value validate.
     * @param {Object} v The object value.
     * @returns {boolean} true when validate success, false when validate failed.
     */
    objectValidate (v) {
        let keys = Object.keys(v);

        let props = this.schema.properties;
        let patProps = this.schema.patternProperties;
        let addValidator;
        let addProps = this.schema.additionalProperties;
        if (addProps != undefined)
            addValidator = new JSONSchemaContext(addProps);
        let unevalValidator;
        let unevalProps = this.schema.unevaluatedProperties;
        if (unevalProps != undefined)
            unevalValidator = new JSONSchemaContext(unevalProps);
        let pnValidator;
        let names = this.schema.propertyNames;
        if (names != undefined)
            pnValidator = new JSONSchemaContext(names);

        for (let pn of keys) {
            if (pnValidator != undefined) {
                pnValidator.tag = `"${pn}"`;
                if (!pnValidator.validate(pn)) {
                    this.message = pnValidator.message;
                    return false;
                }
            }

            let sch;

            if (props != undefined) {
                let prop = props[pn];
                if (prop != undefined)
                    sch = new JSONSchemaContext(prop);
            }

            if ((sch == undefined) && (patProps != undefined)) {
                for (let pat of Object.getOwnPropertyNames(patProps)) {
                    let mr = pn.match(new RegExp(pat));
                    if ((mr != null) && (mr[0] == pn))
                        sch = new JSONSchemaContext(patProps[pat]);
                }
            }
            if (sch == undefined) {
                if ((pn == "$ref") || (pn == "$schema"))
                    sch = new JSONSchemaContext({type: "string"});
                else if (pn == "$ref")
                    sch = new JSONSchemaContext(true);
            }
            if ((sch == undefined) && (addValidator != undefined))
                sch = addValidator;
            if ((sch == undefined) && (unevalValidator != undefined)) {
                sch = unevalValidator;
            }

            if (sch != undefined) {
                sch.tag = `${this.tag}.${pn}`;
                if (!sch.validate(v[pn])) {
                    this.message = sch.message;
                    return false;
                }
            }
        }

        let max = this.schema.maxProperties;
        if (max != undefined) {
            if (keys.length > max) {
                this.fail(`properties number ${keys.length} > maximum properties number ${max}`);
                return false;
            }
        }

        let min = this.schema.minProperties;
        if (min != undefined) {
            if (keys.length < min) {
                this.fail(`properties number ${keys.length} < minimum properties number ${min}`);
                return false;
            }
        }

        let required = this.schema.required;
        if (required != undefined) {
            for (let key of required) {
                if (!keys.includes(key)) {
                    this.fail(`the object has not required property "${key}"`);
                    return false;
                }
            }
        }

        let dep = this.schema.dependentRequired;
        if (dep != undefined) {
            for (let p of Object.getOwnPropertyNames(dep)) {
                if (keys.includes(p)) {
                    let props = dep[p];

                    for (let key of props) {
                        if (!keys.includes(key)) {
                            this.fail(`the object has not dependent required property "${key}"`);
                            return false;
                        }
                    }
                }
            }
        }

        dep = this.schema.dependentSchemas;
        if (dep != undefined) {
            for (let p of Object.getOwnPropertyNames(dep)) {
                if (keys.includes(p)) {
                    let validator = new JSONSchemaContext(dep[p], this.tag);

                    if (!validator.validate(v)) {
                        this.message = validator.message;
                        return false;
                    }
                }
            }
        }

        return true;
    };

    /**
     * Validate a value.
     * @param {*} v The value.
     * @returns {boolean} true when validate success, false when validate failed.
     */
    valueValidate (v) {
        if (typeof this.schema == "boolean") {
            if (this.schema) {
                return true;
            } else {
                this.fail("schema always failed");
                return false;
            }
        }

        let sch;

        sch = this.schema.allOf;
        if (sch != undefined) {
            for (let sub of sch) {
                let validator = new JSONSchemaContext(sub, this.tag);

                if (!validator.validate(v)) {
                    this.message = validator.message;
                    return false;
                }
            }
        }

        sch = this.schema.anyOf;
        if (sch != undefined) {
            let match = false;

            for (let sub of sch) {
                let validator = new JSONSchemaContext(sub, this.tag);

                if (validator.validate(v)) {
                    match = true;
                    break;
                }
            }

            if (!match) {
                this.fail("do not match any of the subschemas");
                return false;
            }
        }

        sch = this.schema.oneOf;
        if (sch != undefined) {
            let match = 0;

            for (let sub of sch) {
                let validator = new JSONSchemaContext(sub, this.tag);

                if (validator.validate(v))
                    match ++;
            }

            if (match == 0) {
                this.fail("do not match any of the subschemas");
                return false;
            } else if (match > 1) {
                this.fail("match more than one subschemas");
                return false;
            }
        }

        sch = this.schema.not;
        if (sch != undefined) {
            let validator = new JSONSchemaContext(sch, this.tag);

            if (validator.validate(v)) {
                this.fail("match the not schema");
                return false;
            }
        }

        sch = this.schema.if;
        if (sch != undefined) {
            let validator = new JSONSchemaContext(sch, this.tag);

            if (validator.validate(v)) {
                sch = this.schema.then;
            } else {
                sch = this.schema.else;
            }

            if (sch != undefined) {
                let validator = new JSONSchemaContext(sch, this.tag);

                if (!validator.validate(v)) {
                    this.message = validator.message;
                    return false;
                }
            }
        }

        let type;
        if (v === null) {
            type = "null";
        } else {
            type = typeof v;
            switch (type) {
            case "object":
            case "function":
                if (Array.isArray(v))
                    type = "array";
                else
                    type = "object";
                break;
            }
        }

        if (this.schema.type != undefined) {
            let match = false;

            if (Array.isArray(this.schema.type)) {
                if ((type == "number") && isFinite(v) && (Math.floor(v) == v)) {
                    if (this.schema.type.includes("integer"))
                        match = true;
                }

                if (!match && this.schema.type.includes(type))
                    match = true;
            } else {
                if ((type == "number") && isFinite(v) && (Math.floor(v) == v)) {
                    if (this.schema.type == "integer")
                        match = true;
                }

                if (!match && (this.schema.type == type))
                    match = true;
            }

            if (!match) {
                this.fail(`value type mismatch, expect type is "${this.schema.type}", actual type is "${type}"`);
                return false;
            }
        }

        let res;

        switch (type) {
        case "number":
            res = this.numberValidate(v);
            break;
        case "string":
            res = this.stringValidate(v);
            break;
        case "array":
            res = this.arrayValidate(v);
            break;
        case "object":
            res = this.objectValidate(v);
            break;
        default:
            res = true;
            break;
        }

        if (!res)
            return false;

        if (this.schema.enum != undefined) {
            let match = false;

            for (let ev of this.schema.enum) {
                if (ev === v) {
                    match = true;
                    break;
                }
            }

            if (!match) {
                this.fail(`${v} is not in [${this.schema.enum}]`);
                return false;
            }
        }

        if (this.schema.const != undefined) {
            if (v !== this.schema.const) {
                this.fail(`${v} != const ${this.schema.const}`);
                return false;
            }
        }

        return true;
    };

    /**
     * Validate.
     * @param {*} v Value to be validated.
     * @returns {boolean} true when validate success, false when validate failed.
     */
    validate (v) {
        return this.valueValidate(v);
    };
};

/**
 * JSON schema object.
 */
export class JSONSchema {
    /**
     * Schema object.
     * @private
     * @type {Object}
     */
    #schema;

    /**
     * Error message.
     * @type {string}
     */
    error;

    /**
     * Create a JSON schema object.
     * @param {Object} sch The JSON schema object.
     */
    constructor (sch) {
        let type = typeof sch;

        if ((type != "object") && (type != "boolean"))
            throw new TypeError("JSON schema is not an object or boolean value");

        this.#schema = sch;
    };

    /**
     * Use JSON schema validate a value.
     * @param {*} v The value to be validated. 
     * @returns {boolean} Validate result.
     */
    validate (v) {
        let ctxt = new JSONSchemaContext(this.#schema, "value");
        let ret;

        this.error = undefined;

        ret = ctxt.validate(v);

        if (!ret)
            this.error = ctxt.message;

        return ret;
    };

    /**
     * Use JSON schema validate a value.
     * @param {*} v The value to be validated. 
     * @throws {ValidateError}
     */
    validateThrow (v) {
        let ctxt = new JSONSchemaContext(this.#schema, "value");
        let ret;

        this.error = undefined;

        ret = ctxt.validate(v);

        if (!ret)
            throw new ValidateError(ctxt.message);
    };
};
