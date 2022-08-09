static const LEX_Token reserved_word[] = {
    {"await"},
    {"break"},
    {"case"},
    {"catch"},
    {"class"},
    {"const"},
    {"continue"},
    {"debugger"},
    {"default"},
    {"delete"},
    {"do"},
    {"else"},
    {"enum"},
#if ENABLE_MODULE
    {"export"},
#endif
    {"extends"},
    {"false"},
    {"finally"},
    {"for"},
    {"function"},
    {"if"},
#if ENABLE_MODULE
    {"import"},
#endif
    {"in"},
    {"instanceof"},
    {"new"},
    {"null"},
    {"return"},
    {"super"},
    {"switch"},
    {"this"},
    {"throw"},
    {"true"},
    {"try"},
    {"typeof"},
    {"var"},
    {"void"},
    {"while"},
    {"with"},
    {"yield"},
    {NULL}
};

static const LEX_Token strict_reserved_word[] = {
    {"implements"},
    {"interface"},
    {"package"},
    {"private"},
    {"protected"},
    {"public"},
    {"let"},
    {"static"},
    {NULL}
};

static const LEX_Token identifier[] = {
    {"as"},
    {"async"},
    {"from"},
    {"get"},
    {"meta"},
    {"of"},
    {"set"},
    {"target"},
    {NULL}
};
