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
 * Lexical analyzer internal header.
 */

#ifndef _RJS_LEX_INTERNAL_H_
#define _RJS_LEX_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <rjs_token_types.h>

/**Token type.*/
typedef enum {
    RJS_TOKEN_END,        /**< Reach the end of the input.*/
    RJS_TOKEN_NUMBER,     /**< Number.*/
    RJS_TOKEN_REGEXP,     /**< Regular expression.*/
    RJS_TOKEN_STRING,     /**< String.*/
    RJS_TOKEN_TEMPLATE,   /**< Template.*/
    RJS_TOKEN_TEMPLATE_HEAD,   /**< Head of the template.*/
    RJS_TOKEN_TEMPLATE_MIDDLE, /**< Middle of the template.*/
    RJS_TOKEN_TEMPLATE_TAIL,   /**< Tail of the template.*/
    RJS_TOKEN_PRIVATE_IDENTIFIER, /**< Private identifier.*/
    RJS_TOKEN_IDENTIFIER, /**< Identifier.*/
    RJS_PUNCTUATOR_TOKENS
} RJS_TokenType;

/**The identifier mask of the token.*/
#define RJS_TOKEN_IDENTIFIER_MASK       0xff
/**The token is a reserved word.*/
#define RJS_TOKEN_FL_RESERVED           0x100
/**The token is a reserved word in strict mode.*/
#define RJS_TOKEN_FL_STRICT_RESERVED    0x200
/**The token is an well known identifier.*/
#define RJS_TOKEN_FL_KNOWN_IDENTIFIER   0x400
/**The token has escape sequence in the identifier.*/
#define RJS_TOKEN_FL_ESCAPE             0x800
/**The token is a decimal integer.*/
#define RJS_TOKEN_FL_DECIMAL            0x1000
/**The template has invalid unicode escape sequence.*/
#define RJS_TOKEN_FL_INVALIE_ESCAPE     0x2000
/**The string has unpaired surrogate character in it.*/
#define RJS_TOKEN_FL_UNPAIRED_SURROGATE 0x4000
/**The string has legacy escape sequence.*/
#define RJS_TOKEN_FL_LEGACY_ESCAPE      0x8000

/**Token.*/
typedef struct {
    RJS_TokenType type;     /**< The token type.*/
    int           flags;    /**< The flags of the token.*/
    RJS_Location  location; /**< Location of the token.*/
    RJS_Value    *value;    /**< The value 1 of the token.*/
} RJS_Token;

/**
 * Initialize the token.
 * \param rt The current runtime.
 * \param tok The token to be initialized.
 */
static inline void
rjs_token_init (RJS_Runtime *rt, RJS_Token *tok)
{
    tok->value = rjs_value_stack_push(rt);
}

/**
 * Release the token.
 * \param rt The current runtime.
 * \param tok The token to be released.
 */
static inline void
rjs_token_deinit (RJS_Runtime *rt, RJS_Token *tok)
{
}

/**"/" and "/=" can be here.*/
#define RJS_LEX_FL_DIV           1
/**Expect a big integer token.*/
#define RJS_LEX_FL_BIG_INT       2
/**Has error when lexical analyse.*/
#define RJS_LEX_ST_ERROR         1
/**Do not output error message.*/
#define RJS_LEX_ST_NO_MSG        4
/**The first token.*/
#define RJS_LEX_ST_FIRST_TOKEN   8
/**No number separators.*/
#define RJS_LEX_ST_NO_SEP        16
/**No legacy octal integer.*/
#define RJS_LEX_ST_NO_LEGACY_OCT 32
/**Analyse the JSON string.*/
#define RJS_LEX_ST_JSON          64

/**Lexical analyzer.*/
typedef struct {
    int              flags;       /**< The flags of the lexical analyzer.*/
    int              status;      /**< Status.*/
    RJS_Input       *input;       /**< The characters input.*/
    RJS_UCharBuffer  uc_text;     /**< Unicode characters buffer.*/
    RJS_UCharBuffer  raw_uc_text; /**< Raw unicode characters buffer.*/
    RJS_CharBuffer   c_text;      /**< Characters buffer.*/
    int              brace_level; /**< Brace level.*/
    RJS_Location     regexp_loc;  /**< The regular expression's location.*/
    RJS_VECTOR_DECL(int) template_brace_level; /**< The template's brace level stack.*/
} RJS_Lex;

/**
 * Check if the lexical analyzer has error.
 * \param lex The lexical analyzer.
 * \retval RJS_TRUE The lexical analyzer has error.
 * \retval RJS_FALSE The lexical analyzer has not any error.
 */
static inline RJS_Bool
rjs_lex_error (RJS_Lex *lex)
{
    return lex->status & RJS_LEX_ST_ERROR;
}

/**
 * Initialize a lexical analyzer.
 * \param rt The current runtime.
 * \param lex The lexical analyzer to be initialized.
 * \param input The characters input to be used.
 */
RJS_INTERNAL void
rjs_lex_init (RJS_Runtime *rt, RJS_Lex *lex, RJS_Input *input);

/**
 * Release an unused lexical analyzer.
 * \param rt The current runtime.
 * \param lex The lexical analyzer to be released.
 */
RJS_INTERNAL void
rjs_lex_deinit (RJS_Runtime *rt, RJS_Lex *lex);

/**
 * Get the next token from the input.
 * \param rt The current runtime.
 * \param lex The lexical analyzer.
 * \param[out] token Return the new token.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_lex_get_token (RJS_Runtime *rt, RJS_Lex *lex, RJS_Token *token);

/**
 * Get the next JSON token from the input.
 * \param rt The current runtime.
 * \param lex The lexical analyzer.
 * \param[out] token Return the new token.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_lex_get_json_token (RJS_Runtime *rt, RJS_Lex *lex, RJS_Token *token);

/**
 * Get the token type's name.
 * \param type The token type.
 * \param flags The flags of the token.
 * \return The name of the token type.
 */
RJS_INTERNAL const char*
rjs_token_type_get_name (RJS_TokenType type, int flags);

#ifdef __cplusplus
}
#endif

#endif

