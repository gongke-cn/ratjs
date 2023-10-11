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
 * Regular expression internal header.
 */

#ifndef _RJS_REGEXP_INTERNAL_H_
#define _RJS_REGEXP_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Has indices*/
#define RJS_REGEXP_FL_D   1
/**Global match.*/
#define RJS_REGEXP_FL_G   2
/**Ignore case.*/
#define RJS_REGEXP_FL_I   4
/**Multiline match.*/
#define RJS_REGEXP_FL_M   8
/**Dot all match.*/
#define RJS_REGEXP_FL_S   16
/**Full unicode.*/
#define RJS_REGEXP_FL_U   32
/**Sticky match.*/
#define RJS_REGEXP_FL_Y   64
/**N flag.*/
#define RJS_REGEXP_FL_N   128

/**Regular expression pattern.*/
typedef struct RJS_RegExpPattern_s RJS_RegExpPattern;

/**Regular expression model.*/
typedef struct {
    RJS_GcThing        gc_thing;    /**< Base GC thing data.*/
    RJS_Value          source;      /**< Pattern string.*/
    int                flags;       /**< Flags.*/
    int                group_num;   /**< Groups' number.*/
    int                name_num;    /**< Names' number.*/
    int               *group_names; /**< Groups' names.*/
    RJS_Value         *names;       /**< Names' array.*/
    RJS_RegExpPattern *pattern;     /**< The pattern data.*/
} RJS_RegExpModel;

/**Regular expression.*/
typedef struct {
    RJS_Object       object; /**< Base object data.*/
    RJS_RegExpModel *model;  /**< The model of the regular expression.*/
} RJS_RegExp;

/**
 * Create a new regular expression.
 * \param rt The current runtime.
 * \param[out] v Return the regular expression value.
 * \param src The source string.
 * \param flags The flags.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_regexp_new (RJS_Runtime *rt, RJS_Value *v, RJS_Value *src, RJS_Value *flags);

/**
 * Execute the regular expression built-in process.
 * \param rt The current runtime.
 * \param v The regular expression value.
 * \param str The string.
 * \param[out] rv Return the match result.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_regexp_builtin_exec (RJS_Runtime *rt, RJS_Value *v, RJS_Value *str, RJS_Value *rv);

#ifdef __cplusplus
}
#endif

#endif

