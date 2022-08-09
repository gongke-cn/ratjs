static const STR_Entry
strings[] = {
    {"", "empty"},
    {" ", "space"},
    {",", "comma"},
    {"#constructor", "hash_constructor"},
    {"g"},
    {"use strict", "use_strict"},
    {"default"},
    {"*default*", "star_default_star"},
    {"eval"},
    {"arguments"},
    {"async"},
    {"let"},
    {"constructor"},
    {"__proto__"},
    {"undefined"},
    {"null"},
    {"boolean"},
    {"true"},
    {"false"},
    {"string"},
    {"number"},
    {"prototype"},
    {"length"},
    {"get"},
    {"set"},
    {"object"},
    {"function"},
    {"symbol"},
    {"Error"},
    {"NaN"},
    {"Infinity"},
    {"-Infinity", "negative_Infinity"},
    {"fulfilled"},
    {"rejected"},
    {"yield"},
    {"await"},
    {"NFC"},
    {"NFD"},
    {"NFKC"},
    {"NFKD"},
#if ENABLE_BIG_INT
    {"bigint"},
#endif
#if ENABLE_BOUND_FUNC
    {"bound"},
#endif
#if ENABLE_MODULE
    {"Module"},
#endif
    {NULL, NULL}
};

static const STR_PropEntry
str_props[] = {
    {"prototype"},
    {"toString"},
    {"toLocaleString"},
    {"valueOf"},
    {"lastIndex"},
    {"length"},
    {"index"},
    {"input"},
    {"groups"},
    {"indices"},
    {"raw"},
    {"callee"},
    {"enumerable"},
    {"configurable"},
    {"value"},
    {"writable"},
    {"get"},
    {"set"},
    {"name"},
    {"next"},
    {"done"},
    {"return"},
    {"then"},
    {"throw"},
    {"constructor"},
    {"message"},
    {"cause"},
    {"errors"},
    {"source"},
    {"flags"},
    {"global"},
    {"unicode"},
    {"join"},
    {"resolve"},
    {"status"},
    {"exec"},
    {"hasIndices"},
    {"ignoreCase"},
    {"multiline"},
    {"dotAll"},
    {"sticky"},
    {"reason"},
#if ENABLE_PROXY
    {"getPrototypeOf"},
    {"setPrototypeOf"},
    {"isExtensible"},
    {"preventExtensions"},
    {"getOwnPropertyDescriptor"},
    {"defineProperty"},
    {"has"},
    {"deleteProperty"},
    {"ownKeys"},
    {"apply"},
    {"construct"},
    {"proxy"},
    {"revoke"},
#endif
#if ENABLE_DATE
    {"toISOString"},
#endif
#if ENABLE_SET || ENABLE_WEAK_SET
    {"add"},
#endif
#if ENABLE_JSON
    {"toJSON"},
#endif
#if ENABLE_CALLER
    {"caller"},
#endif
#if ENABLE_EXTENSION
    {"size"},
    {"mode"},
    {"format"},
    {"atime"},
    {"mtime"},
    {"ctime"},
#endif
    {NULL}
};

static const SYM_PropEntry
sym_props[] = {
    {"iterator", "Symbol.iterator"},
    {"asyncIterator", "Symbol.asyncIterator"},
    {"toPrimitive", "Symbol.toPrimitive"},
    {"unscopables", "Symbol.unscopables"},
    {"hasInstance", "Symbol.hasInstance"},
    {"isConcatSpreadable", "Symbol.isConcatSpreadable"},
    {"match", "Symbol.match"},
    {"matchAll", "Symbol.matchAll"},
    {"replace", "Symbol.replace"},
    {"search", "Symbol.search"},
    {"split", "Symbol.split"},
    {"species", "Symbol.species"},
    {"toStringTag", "Symbol.toStringTag"},
    {NULL, NULL}
};
