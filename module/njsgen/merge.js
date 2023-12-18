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
 * File merge function.
 * @module merge
 */

import { Log } from "../log";

let log = new Log("mege");

//log.enable = false;

/**
 * File data slot.
 */
class Slot {
    /**
     * The tag string.
     * @type {string}
     */
    tag;

    /**
     * The parent slot.
     * @type {Slot}
     */
    parent;

    /**
     * Slot map.
     * @type {Map<string, Slot>}
     */
    slotMap = new Map();

    /**
     * File data lines.
     */
    lines = [];

    /**
     * Create a new tag.
     * @param {string} tag The slot's tag.
     */
    constructor (tag) {
        this.tag = tag;
    };

    /**
     * Load the file data.
     * @param {string} str The text string.
     */
    load (str) {
        let lines = str.split("\n");
        let slot = this;

        for (let line of lines) {
            let m = line.match(/\/\*NJSGEN (.+) (BEGIN|END)\*\//);

            if (m !== null) {
                let tag = m[1];
                let cmd = m[2];

                if (cmd == "BEGIN") {
                    let cSlot = new Slot(tag);

                    this.slotMap.set(tag, cSlot);

                    slot.appendLine(cSlot);

                    cSlot.parent = slot;
                    slot = cSlot;
                } else {
                    if (tag != slot.tag)
                        throw new TypeError(`slot tag "${tag}" mismatch`);

                    slot = slot.parent;
                }
            } else {
                slot.appendLine(line);
            }
        }
    };

    /**
     * Append text line to the slot.
     * @param {*} line 
     */
    appendLine (line) {
        this.lines.push(line);
    };

    /**
     * Convert the slot to string.
     * @returns {string}
     */
    toString () {
        let data = this.lines.join("\n");

        if (this.tag == undefined)
            return data;

        return `\
/*NJSGEN ${this.tag} BEGIN*/
${data}
/*NJSGEN ${this.tag} END*/`
    };
}

/**
 * Merge the new data with the old file data.
 * @param {string} filename The filename.
 * @param {string} data The new file data.
 */
export function merge (filename, data)
{
    if (FileState(filename) == undefined) {
        File.storeString(filename, data);
        return;
    }

    let oStr = File.loadString(filename);
    let oSlot = new Slot();
    let nSlot = new Slot();

    oSlot.load(oStr);
    nSlot.load(data);

    nSlot.slotMap.forEach(slot => {
        let rSlot = oSlot.slotMap.get(slot.tag);

        if (rSlot !== undefined)
            slot.lines = rSlot.lines;
    });

    File.storeString(filename, nSlot.toString());
}