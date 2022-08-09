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
 * Dynamic vector.
 */

#ifndef _RJS_VECTOR_H_
#define _RJS_VECTOR_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup vector Vector
 * Dynamic vector
 * @{
 */

/**
 * \cond
 */
#ifndef RJS_VECTOR_REALLOC_FUNC
    /*Memory buffer resize function for vector.*/
    #define RJS_VECTOR_REALLOC_FUNC rjs_realloc_assert
#endif
/**
 * \endcond
 */

/**Declare a vector.*/
#define RJS_VECTOR_DECL(_t)\
    struct {\
        _t    *items;\
        size_t item_num;\
        size_t item_cap;\
    }

/**Initialize a vector.*/
#define rjs_vector_init(_v)\
    RJS_STMT_BEGIN\
        (_v)->items = NULL;\
        (_v)->item_num = 0;\
        (_v)->item_cap = 0;\
    RJS_STMT_END

/**Release a vector.*/
#define rjs_vector_deinit(_v, _d)\
    RJS_STMT_BEGIN\
        if ((_v)->items)\
            RJS_VECTOR_REALLOC_FUNC(_d, (_v)->items, (_v)->item_cap * sizeof(*(_v)->items), 0);\
    RJS_STMT_END

/**Get the size of the vector.*/
#define rjs_vector_get_size(_v) ((_v)->item_num)

/**Set the capacity of the vector.*/
#define rjs_vector_set_capacity(_v, _s, _d)\
    RJS_STMT_BEGIN\
        size_t _size = _s;\
        if ((_v)->item_cap < _size) {\
            (_v)->items = RJS_VECTOR_REALLOC_FUNC(_d, (_v)->items,\
                    (_v)->item_cap * sizeof(*(_v)->items),\
                    (_s) * sizeof(*(_v)->items));\
            (_v)->item_cap = _size;\
        }\
    RJS_STMT_END

/**Resize the vector.*/
#define rjs_vector_resize(_v, _s, _d)\
    RJS_STMT_BEGIN\
        size_t _size = _s;\
        if ((_v)->item_cap < _size) {\
            size_t _cap = RJS_MAX_3(_size, (_v)->item_cap * 2, 8);\
            rjs_vector_set_capacity(_v, _cap, _d);\
        }\
        (_v)->item_num = _size;\
    RJS_STMT_END

/**Resize the vector and initialize the new items.*/
#define rjs_vector_resize_init(_v, _s, _d, _init)\
    RJS_STMT_BEGIN\
        size_t _size = _s;\
        if ((_v)->item_cap < _size) {\
            size_t _cap = RJS_MAX_3(_size, (_v)->item_cap * 2, 8);\
            rjs_vector_set_capacity(_v, _cap, _d);\
        }\
        if (_size > (_v)->item_num)\
            _init(_d, (_v)->items + (_v)->item_num, _size - (_v)->item_num);\
        (_v)->item_num = _size;\
    RJS_STMT_END

/**Get the item of the vector.*/
#define rjs_vector_get_item(_v, _i) ((_v)->items[_i])

/**Set the item value of the vector.*/
#define rjs_vector_set_item(_v, _i, _n, _d)\
    RJS_STMT_BEGIN\
        size_t _idx = _i;\
        if ((_v)->item_num <= _idx)\
            rjs_vector_resize(_v, _idx + 1, _d);\
        (_v)->items[_idx] = (_n);\
    RJS_STMT_END

/**Set the item value of the vector and initialize the new items if the vector is resized.*/
#define rjs_vector_set_item_init(_v, _i, _n, _d, _init)\
    RJS_STMT_BEGIN\
        if ((_v)->item_num <= (_i))\
            rjs_vector_resize(_v, _i + 1, _d, _init);\
        (_v)->items[_i] = (_n);\
    RJS_STMT_END

/**Append a new item to the end of the vector.*/
#define rjs_vector_append(_v, _n, _d)\
    rjs_vector_set_item(_v, (_v)->item_num, _n, _d)

/**Traverse the items in the vector.*/
#define rjs_vector_foreach(_v, _i, _n)\
    for ((_i) = 0; (_i) < (_v)->item_num ? ((_n = &(_v)->items[_i])) : 0; (_i) ++)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

