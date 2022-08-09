/**Character name table.*/
typedef struct {
    char  c;     /**< The character.*/
    char *name;  /**< The name of the character.*/
} LEX_CharName;

/**Character names table.*/
static const LEX_CharName punct_char_names[] = {
    {'{', "lbrace"},
    {'}', "rbrace"},
    {'(', "lparenthese"},
    {')', "rparenthese"},
    {'[', "lbracket"},
    {']', "rbracket"},
    {'.', "dot"},
    {';', "semicolon"},
    {',', "comma"},
    {'<', "lt"},
    {'>', "gt"},
    {'=', "eq"},
    {'!', "exclamation"},
    {'+', "plus"},
    {'-', "minus"},
    {'*', "star"},
    {'/', "slash"},
    {'%', "percent"},
    {'&', "amp"},
    {'|', "pipe"},
    {'^', "caret"},
    {'~', "tilde"},
    {'?', "ques"},
    {':', "colon"},
    {0, NULL}
};
