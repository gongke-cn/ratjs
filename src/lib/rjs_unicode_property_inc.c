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

/**Proeprty entry.*/
typedef struct {
    char      *name; /**< Name or alias of the property*/
    UProperty  prop; /**< Property value.*/
} PropertyEntry;

/**Category value entry.*/
typedef struct {
    char *name; /**< Name of the category.*/
    int   mask; /**< Value of the category mask.*/
} CategoryEntry;

/**Script value entry.*/
typedef struct {
    char        *name; /**< Name of the script code.*/
    UScriptCode  code; /**< Value of the script code.*/
} ScriptEntry;

/*Category value table.*/
static const CategoryEntry
category_values[] = {
    {"C",       U_GC_C_MASK},
    {"Other",   U_GC_C_MASK},
    {"Cc",      U_GC_CC_MASK},
    {"Control", U_GC_CC_MASK},
    {"cntrl",   U_GC_CC_MASK},
    {"Cf",      U_GC_CF_MASK},
    {"Format",  U_GC_CF_MASK},
    {"Cn",      U_GC_CN_MASK},
    {"Unassigned", U_GC_CN_MASK},
    {"Co",      U_GC_CO_MASK},
    {"Private_Use",U_GC_CO_MASK},
    {"Cs",      U_GC_CS_MASK},
    {"Surrogate",  U_GC_CS_MASK},
    {"L",       U_GC_L_MASK},
    {"Letter",  U_GC_L_MASK},
    {"LC",      U_GC_LC_MASK},
    {"Cased_Letter", U_GC_LC_MASK},
    {"Ll",      U_GC_LL_MASK},
    {"Lowercase_Letter", U_GC_LL_MASK},
    {"Lm",      U_GC_LM_MASK},
    {"Modifier_Letter", U_GC_LM_MASK},
    {"Lo",      U_GC_LO_MASK},
    {"Other_Letter", U_GC_LO_MASK},
    {"Lt",      U_GC_LT_MASK},
    {"Titlecase_Letter", U_GC_LT_MASK},
    {"Lu",      U_GC_LU_MASK},
    {"Uppercase_Letter", U_GC_LU_MASK},
    {"M",       U_GC_M_MASK},
    {"Mark",    U_GC_M_MASK},
    {"Combining_Mark",   U_GC_M_MASK},
    {"Mc",      U_GC_MC_MASK},
    {"Spacing_Mark",     U_GC_MC_MASK},
    {"Me",      U_GC_ME_MASK},
    {"Enclosing_Mark",   U_GC_ME_MASK},
    {"Mn",      U_GC_MN_MASK},
    {"Nonspacing_Mark",  U_GC_MN_MASK},
    {"N",       U_GC_N_MASK},
    {"Number",  U_GC_N_MASK},
    {"Nd",      U_GC_ND_MASK},
    {"Decimal_Number",   U_GC_ND_MASK},
    {"digit",   U_GC_ND_MASK},
    {"Nl",      U_GC_NL_MASK},
    {"Letter_Number",    U_GC_NL_MASK},
    {"No",      U_GC_NO_MASK},
    {"Other_Number",     U_GC_NO_MASK},
    {"P",       U_GC_P_MASK},
    {"Punctuation",      U_GC_P_MASK},
    {"punct",   U_GC_P_MASK},
    {"Pc",      U_GC_PC_MASK},
    {"Connector_Punctuation", U_GC_PC_MASK},
    {"Pd",      U_GC_PD_MASK},
    {"Dash_Punctuation", U_GC_PD_MASK},
    {"Pe",      U_GC_PE_MASK},
    {"Close_Punctuation",U_GC_PE_MASK},
    {"Pf",      U_GC_PF_MASK},
    {"Final_Punctuation",U_GC_PF_MASK},
    {"Pi",      U_GC_PI_MASK},
    {"Initial_Punctuation", U_GC_PI_MASK},
    {"Po",      U_GC_PO_MASK},
    {"Other_Punctuation",U_GC_PO_MASK},
    {"Ps",      U_GC_PS_MASK},
    {"Open_Punctuation", U_GC_PS_MASK},
    {"S",       U_GC_S_MASK},
    {"Symbol",  U_GC_S_MASK},
    {"Sc",      U_GC_SC_MASK},
    {"Currency_Symbol",  U_GC_SC_MASK},
    {"Sk",      U_GC_SK_MASK},
    {"Modifier_Symbol",  U_GC_SK_MASK},
    {"Sm",      U_GC_SM_MASK},
    {"Math_Symbol",      U_GC_SM_MASK},
    {"So",      U_GC_SO_MASK},
    {"Other_Symbol",     U_GC_SO_MASK},
    {"Z",       U_GC_Z_MASK},
    {"Separator",        U_GC_Z_MASK},
    {"Zl",      U_GC_ZL_MASK},
    {"Line_Separator",   U_GC_ZL_MASK},
    {"Zp", U_GC_ZP_MASK},
    {"Paragraph_Separator", U_GC_ZP_MASK},
    {"Zs", U_GC_ZS_MASK},
    {"Space_Separator",  U_GC_ZS_MASK},
    {NULL}
};

/*Script code table.*/
static const ScriptEntry
script_code_values[] = {
    {"Adlam",   USCRIPT_ADLAM},
    {"Adlm",    USCRIPT_ADLAM},
    {"Caucasian_Albanian", USCRIPT_CAUCASIAN_ALBANIAN},
    {"Aghb",    USCRIPT_CAUCASIAN_ALBANIAN},
    {"Ahom",    USCRIPT_AHOM},
    {"Arabic",  USCRIPT_ARABIC},
    {"Arab",    USCRIPT_ARABIC},
    {"Imperial_Aramaic", USCRIPT_IMPERIAL_ARAMAIC},
    {"Armi",    USCRIPT_IMPERIAL_ARAMAIC},
    {"Armenian",USCRIPT_ARMENIAN},
    {"Armn",    USCRIPT_ARMENIAN},
    {"Avestan", USCRIPT_AVESTAN},
    {"Avst",    USCRIPT_AVESTAN},
    {"Balinese",USCRIPT_BALINESE},
    {"Bali",    USCRIPT_BALINESE},
    {"Bamum",   USCRIPT_BAMUM},
    {"Bamu",    USCRIPT_BAMUM},
    {"Bassa_Vah", USCRIPT_BASSA_VAH},
    {"Bass",    USCRIPT_BASSA_VAH},
    {"Batak",   USCRIPT_BATAK},
    {"Batk",    USCRIPT_BATAK},
    {"Bengali", USCRIPT_BENGALI},
    {"Beng",    USCRIPT_BENGALI},
    {"Bhaiksuki", USCRIPT_BHAIKSUKI},
    {"Bhks",    USCRIPT_BHAIKSUKI},
    {"Bopomofo",USCRIPT_BOPOMOFO},
    {"Bopo",    USCRIPT_BOPOMOFO},
    {"Brahmi",  USCRIPT_BRAHMI},
    {"Brah",    USCRIPT_BRAHMI},
    {"Braille", USCRIPT_BRAILLE},
    {"Brai",    USCRIPT_BRAILLE},
    {"Buginese",USCRIPT_BUGINESE},
    {"Bugi",    USCRIPT_BUGINESE},
    {"Buhid",   USCRIPT_BUHID},
    {"Buhd",    USCRIPT_BUHID},
    {"Chakma",  USCRIPT_CHAKMA},
    {"Cakm",    USCRIPT_CHAKMA},
    {"Canadian_Aboriginal", USCRIPT_CANADIAN_ABORIGINAL},
    {"Cans",    USCRIPT_CANADIAN_ABORIGINAL},
    {"Carian",  USCRIPT_CARIAN},
    {"Cari",    USCRIPT_CARIAN},
    {"Cham",    USCRIPT_CHAM},
    {"Cherokee",USCRIPT_CHEROKEE},
    {"Cher",    USCRIPT_CHEROKEE},
    {"Chorasmian", USCRIPT_CHORASMIAN},
    {"Chrs",    USCRIPT_CHORASMIAN},
    {"Coptic",  USCRIPT_COPTIC},
    {"Copt",    USCRIPT_COPTIC},
    {"Qaac",    USCRIPT_COPTIC},
    {"Cypro_Minoan", USCRIPT_CYPRO_MINOAN},
    {"Cpmn",    USCRIPT_CYPRO_MINOAN},
    {"Cypriot", USCRIPT_CYPRIOT},
    {"Cprt",    USCRIPT_CYPRIOT},
    {"Cyrillic",USCRIPT_CYRILLIC},
    {"Cyrl",    USCRIPT_CYRILLIC},
    {"Devanagari", USCRIPT_DEVANAGARI},
    {"Deva",    USCRIPT_DEVANAGARI},
    {"Dives_Akuru",USCRIPT_DIVES_AKURU},
    {"Diak",    USCRIPT_DIVES_AKURU},
    {"Dogra",   USCRIPT_DOGRA},
    {"Dogr",    USCRIPT_DOGRA},
    {"Deseret", USCRIPT_DESERET},
    {"Dsrt",    USCRIPT_DESERET},
    {"Duployan",USCRIPT_DUPLOYAN},
    {"Dupl",    USCRIPT_DUPLOYAN},
    {"Egyptian_Hieroglyphs", USCRIPT_EGYPTIAN_HIEROGLYPHS},
    {"Egyp",    USCRIPT_EGYPTIAN_HIEROGLYPHS},
    {"Elbasan", USCRIPT_ELBASAN},
    {"Elba",    USCRIPT_ELBASAN},
    {"Elymaic", USCRIPT_ELYMAIC},
    {"Elym",    USCRIPT_ELYMAIC},
    {"Ethiopic",USCRIPT_ETHIOPIC},
    {"Ethi",    USCRIPT_ETHIOPIC},
    {"Georgian",USCRIPT_GEORGIAN},
    {"Geor",    USCRIPT_GEORGIAN},
    {"Glagolitic", USCRIPT_GLAGOLITIC},
    {"Glag",    USCRIPT_GLAGOLITIC},
    {"Gunjala_Gondi", USCRIPT_GUNJALA_GONDI},
    {"Gong",    USCRIPT_GUNJALA_GONDI},
    {"Masaram_Gondi", USCRIPT_MASARAM_GONDI},
    {"Gonm",    USCRIPT_MASARAM_GONDI},
    {"Gothic",  USCRIPT_GOTHIC},
    {"Goth",    USCRIPT_GOTHIC},
    {"Grantha", USCRIPT_GRANTHA},
    {"Gran",    USCRIPT_GRANTHA},
    {"Greek",   USCRIPT_GREEK},
    {"Grek",    USCRIPT_GREEK},
    {"Gujarati",USCRIPT_GUJARATI},
    {"Gujr",    USCRIPT_GUJARATI},
    {"Gurmukhi",USCRIPT_GURMUKHI},
    {"Guru",    USCRIPT_GURMUKHI},
    {"Hangul",  USCRIPT_HANGUL},
    {"Hang",    USCRIPT_HANGUL},
    {"Han",     USCRIPT_HAN},
    {"Hani",    USCRIPT_HAN},
    {"Hanunoo", USCRIPT_HANUNOO},
    {"Hano",    USCRIPT_HANUNOO},
    {"Hatran",  USCRIPT_HATRAN},
    {"Hatr",    USCRIPT_HATRAN},
    {"Hebrew",  USCRIPT_HEBREW},
    {"Hebr",    USCRIPT_HEBREW},
    {"Hiragana",USCRIPT_HIRAGANA},
    {"Hira",    USCRIPT_HIRAGANA},
    {"Anatolian_Hieroglyphs", USCRIPT_ANATOLIAN_HIEROGLYPHS},
    {"Hluw",    USCRIPT_ANATOLIAN_HIEROGLYPHS},
    {"Pahawh_Hmong", USCRIPT_PAHAWH_HMONG},
    {"Hmng",    USCRIPT_PAHAWH_HMONG},
    {"Nyiakeng_Puachue_Hmong", USCRIPT_NYIAKENG_PUACHUE_HMONG},
    {"Hmnp",    USCRIPT_NYIAKENG_PUACHUE_HMONG},
    {"Katakana_Or_Hiragana", USCRIPT_KATAKANA_OR_HIRAGANA},
    {"Hrkt",    USCRIPT_KATAKANA_OR_HIRAGANA},
    {"Old_Hungarian", USCRIPT_OLD_HUNGARIAN},
    {"Hung",    USCRIPT_OLD_HUNGARIAN},
    {"Old_Italic", USCRIPT_OLD_ITALIC},
    {"Ital",    USCRIPT_OLD_ITALIC},
    {"Javanese",USCRIPT_JAVANESE},
    {"Java",    USCRIPT_JAVANESE},
    {"Kayah_Li",USCRIPT_KAYAH_LI},
    {"Kali",    USCRIPT_KAYAH_LI},
    {"Katakana",USCRIPT_KATAKANA},
    {"Kana",    USCRIPT_KATAKANA},
    {"Kharoshthi", USCRIPT_KHAROSHTHI},
    {"Khar",    USCRIPT_KHAROSHTHI},
    {"Khmer",   USCRIPT_KHMER},
    {"Khmr",    USCRIPT_KHMER},
    {"Khojki",  USCRIPT_KHOJKI},
    {"Khoj",    USCRIPT_KHOJKI},
    {"Khitan_Small_Script", USCRIPT_KHITAN_SMALL_SCRIPT},
    {"Kits",    USCRIPT_KHITAN_SMALL_SCRIPT},
    {"Kannada", USCRIPT_KANNADA},
    {"Knda",    USCRIPT_KANNADA},
    {"Kaithi",  USCRIPT_KAITHI},
    {"Kthi",    USCRIPT_KAITHI},
    {"Tai_Tham",USCRIPT_LANNA},
    {"Lana",    USCRIPT_LANNA},
    {"Lao",     USCRIPT_LAO},
    {"Laoo",    USCRIPT_LAO},
    {"Latin",   USCRIPT_LATIN},
    {"Latn",    USCRIPT_LATIN},
    {"Lepcha",  USCRIPT_LEPCHA},
    {"Lepc",    USCRIPT_LEPCHA},
    {"Limbu",   USCRIPT_LIMBU},
    {"Limb",    USCRIPT_LIMBU},
    {"Linear_A",USCRIPT_LINEAR_A},
    {"Lina",    USCRIPT_LINEAR_A},
    {"Linear_B",USCRIPT_LINEAR_B},
    {"Linb",    USCRIPT_LINEAR_B},
    {"Lisu",    USCRIPT_LISU},
    {"Lycian",  USCRIPT_LYCIAN},
    {"Lyci",    USCRIPT_LYCIAN},
    {"Lydian",  USCRIPT_LYDIAN},
    {"Lydi",    USCRIPT_LYDIAN},
    {"Mahajani",USCRIPT_MAHAJANI},
    {"Mahj",    USCRIPT_MAHAJANI},
    {"Makasar", USCRIPT_MAKASAR},
    {"Maka",    USCRIPT_MAKASAR},
    {"Mandaic", USCRIPT_MANDAIC},
    {"Mand",    USCRIPT_MANDAIC},
    {"Manichaean", USCRIPT_MANICHAEAN},
    {"Mani",    USCRIPT_MANICHAEAN},
    {"Marchen", USCRIPT_MARCHEN},
    {"Marc",    USCRIPT_MARCHEN},
    {"Medefaidrin", USCRIPT_MEDEFAIDRIN},
    {"Medf",    USCRIPT_MEDEFAIDRIN},
    {"Mende_Kikakui", USCRIPT_MENDE},
    {"Mend",    USCRIPT_MENDE},
    {"Meroitic_Cursive", USCRIPT_MEROITIC_CURSIVE},
    {"Merc",    USCRIPT_MEROITIC_CURSIVE},
    {"Meroitic_Hieroglyphs", USCRIPT_MEROITIC_HIEROGLYPHS},
    {"Mero",    USCRIPT_MEROITIC_HIEROGLYPHS},
    {"Malayalam", USCRIPT_MALAYALAM},
    {"Mlym",    USCRIPT_MALAYALAM},
    {"Modi",    USCRIPT_MODI},
    {"Mongolian", USCRIPT_MONGOLIAN},
    {"Mong",    USCRIPT_MONGOLIAN},
    {"Mro",     USCRIPT_MRO},
    {"Mroo",    USCRIPT_MRO},
    {"Meetei_Mayek", USCRIPT_MEITEI_MAYEK},
    {"Mtei",    USCRIPT_MEITEI_MAYEK},
    {"Multani", USCRIPT_MULTANI},
    {"Mult",    USCRIPT_MULTANI},
    {"Myanmar", USCRIPT_MYANMAR},
    {"Mymr",    USCRIPT_MYANMAR},
    {"Nandinagari", USCRIPT_NANDINAGARI},
    {"Nand",    USCRIPT_NANDINAGARI},
    {"Old_North_Arabian", USCRIPT_OLD_NORTH_ARABIAN},
    {"Narb",    USCRIPT_OLD_NORTH_ARABIAN},
    {"Nabataean", USCRIPT_NABATAEAN},
    {"Nbat",    USCRIPT_NABATAEAN},
    {"Newa",    USCRIPT_NEWA},
    {"Nko",     USCRIPT_NKO},
    {"Nkoo",    USCRIPT_NKO},
    {"Nushu",   USCRIPT_NUSHU},
    {"Nshu",    USCRIPT_NUSHU},
    {"Ogham",   USCRIPT_OGHAM},
    {"Ogam",    USCRIPT_OGHAM},
    {"Ol_Chiki",USCRIPT_OL_CHIKI},
    {"Olck",    USCRIPT_OL_CHIKI},
    {"Old_Turkic", USCRIPT_ORKHON},
    {"Orkh",    USCRIPT_ORKHON},
    {"Oriya",   USCRIPT_ORIYA},
    {"Orya",    USCRIPT_ORIYA},
    {"Osage",   USCRIPT_OSAGE},
    {"Osge",    USCRIPT_OSAGE},
    {"Osmanya", USCRIPT_OSMANYA},
    {"Osma",    USCRIPT_OSMANYA},
    {"Old_Uyghur", USCRIPT_OLD_UYGHUR},
    {"Ougr",    USCRIPT_OLD_UYGHUR},
    {"Palmyrene",  USCRIPT_PALMYRENE},
    {"Palm",    USCRIPT_PALMYRENE},
    {"Pau_Cin_Hau",USCRIPT_PAU_CIN_HAU},
    {"Pauc",    USCRIPT_PAU_CIN_HAU},
    {"Old_Permic", USCRIPT_OLD_PERMIC},
    {"Perm",    USCRIPT_OLD_PERMIC},
    {"Phags_Pa",USCRIPT_PHAGS_PA},
    {"Phag",    USCRIPT_PHAGS_PA},
    {"Inscriptional_Pahlavi", USCRIPT_INSCRIPTIONAL_PAHLAVI},
    {"Phli",    USCRIPT_INSCRIPTIONAL_PAHLAVI},
    {"Psalter_Pahlavi", USCRIPT_PSALTER_PAHLAVI},
    {"Phlp",    USCRIPT_PSALTER_PAHLAVI},
    {"Phoenician", USCRIPT_PHOENICIAN},
    {"Phnx",    USCRIPT_PHOENICIAN},
    {"Miao",    USCRIPT_MIAO},
    {"Plrd",    USCRIPT_MIAO},
    {"Inscriptional_Parthian", USCRIPT_INSCRIPTIONAL_PARTHIAN},
    {"Prti",    USCRIPT_INSCRIPTIONAL_PARTHIAN},
    {"Rejang",  USCRIPT_REJANG},
    {"Rjng",    USCRIPT_REJANG},
    {"Hanifi_Rohingya", USCRIPT_HANIFI_ROHINGYA},
    {"Rohg",    USCRIPT_HANIFI_ROHINGYA},
    {"Runic",   USCRIPT_RUNIC},
    {"Runr",    USCRIPT_RUNIC},
    {"Samaritan", USCRIPT_SAMARITAN},
    {"Samr",    USCRIPT_SAMARITAN},
    {"Old_South_Arabian", USCRIPT_OLD_SOUTH_ARABIAN},
    {"Sarb",    USCRIPT_OLD_SOUTH_ARABIAN},
    {"Saurashtra", USCRIPT_SAURASHTRA},
    {"Saur",    USCRIPT_SAURASHTRA},
    {"SignWriting", USCRIPT_SIGN_WRITING},
    {"Sgnw",    USCRIPT_SIGN_WRITING},
    {"Shavian", USCRIPT_SHAVIAN},
    {"Shaw",    USCRIPT_SHAVIAN},
    {"Sharada", USCRIPT_SHARADA},
    {"Shrd",    USCRIPT_SHARADA},
    {"Siddham", USCRIPT_SIDDHAM},
    {"Sidd",    USCRIPT_SIDDHAM},
    {"Khudawadi", USCRIPT_KHUDAWADI},
    {"Sind",    USCRIPT_KHUDAWADI},
    {"Sinhala", USCRIPT_SINHALA},
    {"Sinh",    USCRIPT_SINHALA},
    {"Sogdian", USCRIPT_SOGDIAN},
    {"Sogd",    USCRIPT_SOGDIAN},
    {"Old_Sogdian",  USCRIPT_OLD_SOGDIAN},
    {"Sogo",    USCRIPT_OLD_SOGDIAN},
    {"Sora_Sompeng", USCRIPT_SORA_SOMPENG},
    {"Sora",    USCRIPT_SORA_SOMPENG},
    {"Soyombo", USCRIPT_SOYOMBO},
    {"Soyo",    USCRIPT_SOYOMBO},
    {"Sundanese", USCRIPT_SUNDANESE},
    {"Sund",    USCRIPT_SUNDANESE},
    {"Syloti_Nagri", USCRIPT_SYLOTI_NAGRI},
    {"Sylo",    USCRIPT_SYLOTI_NAGRI},
    {"Syriac",  USCRIPT_SYRIAC},
    {"Syrc",    USCRIPT_SYRIAC},
    {"Tagbanwa",USCRIPT_TAGBANWA},
    {"Tagb",    USCRIPT_TAGBANWA},
    {"Takri",   USCRIPT_TAKRI},
    {"Takr",    USCRIPT_TAKRI},
    {"Tai_Le",  USCRIPT_TAI_LE},
    {"Tale",    USCRIPT_TAI_LE},
    {"New_Tai_Lue", USCRIPT_NEW_TAI_LUE},
    {"Talu",    USCRIPT_NEW_TAI_LUE},
    {"Tamil",   USCRIPT_TAMIL},
    {"Taml",    USCRIPT_TAMIL},
    {"Tangut",  USCRIPT_TANGUT},
    {"Tang",    USCRIPT_TANGUT},
    {"Tai_Viet",USCRIPT_TAI_VIET},
    {"Tavt",    USCRIPT_TAI_VIET},
    {"Telugu",  USCRIPT_TELUGU},
    {"Telu",    USCRIPT_TELUGU},
    {"Tifinagh",USCRIPT_TIFINAGH},
    {"Tfng",    USCRIPT_TIFINAGH},
    {"Tagalog", USCRIPT_TAGALOG},
    {"Tglg",    USCRIPT_TAGALOG},
    {"Thaana",  USCRIPT_THAANA},
    {"Thaa",    USCRIPT_THAANA},
    {"Thai",    USCRIPT_THAI},
    {"Tibetan", USCRIPT_TIBETAN},
    {"Tibt",    USCRIPT_TIBETAN},
    {"Tirhuta", USCRIPT_TIRHUTA},
    {"Tirh",    USCRIPT_TIRHUTA},
    {"Tangsa",  USCRIPT_TANGSA},
    {"Tnsa",    USCRIPT_TANGSA},
    {"Toto",    USCRIPT_TOTO},
    {"Ugaritic",USCRIPT_UGARITIC},
    {"Ugar",    USCRIPT_UGARITIC},
    {"Vai",     USCRIPT_VAI},
    {"Vaii",    USCRIPT_VAI},
    {"Vithkuqi",USCRIPT_VITHKUQI},
    {"Vith",    USCRIPT_VITHKUQI},
    {"Warang_Citi", USCRIPT_WARANG_CITI},
    {"Wara",    USCRIPT_WARANG_CITI},
    {"Wancho",  USCRIPT_WANCHO},
    {"Wcho",    USCRIPT_WANCHO},
    {"Old_Persian", USCRIPT_OLD_PERSIAN},
    {"Xpeo",    USCRIPT_OLD_PERSIAN},
    {"Cuneiform", USCRIPT_CUNEIFORM},
    {"Xsux",    USCRIPT_CUNEIFORM},
    {"Yezidi",  USCRIPT_YEZIDI},
    {"Yezi",    USCRIPT_YEZIDI},
    {"Yi",      USCRIPT_YI},
    {"Yiii",    USCRIPT_YI},
    {"Zanabazar_Square", USCRIPT_ZANABAZAR_SQUARE},
    {"Zanb",    USCRIPT_ZANABAZAR_SQUARE},
    {"Inherited", USCRIPT_INHERITED},
    {"Zinh",    USCRIPT_INHERITED},
    {"Qaai",    USCRIPT_INHERITED},
    {"Common",  USCRIPT_COMMON},
    {"Zyyy",    USCRIPT_COMMON},
    {"Unknown", USCRIPT_UNKNOWN},
    {"Zzzz",    USCRIPT_UNKNOWN},
#if U_ICU_VERSION_MAJOR_NUM >= 72
    {"Kawi",    USCRIPT_KAWI},
    {"Nag_Mundari", USCRIPT_NAG_MUNDARI},
    {"Nagm",    USCRIPT_NAG_MUNDARI},
#endif /*U_ICU_VERSION_MAJOR_NUM >= 72*/
    {NULL}
};

/*Non-binary properties.*/
static const PropertyEntry
non_binary_props[] = {
    {"General_Category", UCHAR_GENERAL_CATEGORY},
    {"gc",               UCHAR_GENERAL_CATEGORY},
    {"Script",           UCHAR_SCRIPT},
    {"sc",               UCHAR_SCRIPT},
    {"Script_Extensions",UCHAR_SCRIPT_EXTENSIONS},
    {"scx",              UCHAR_SCRIPT_EXTENSIONS},
    {NULL}
};

#define UCHAR_ASCII    -1000
#define UCHAR_ANY      -1001
#define UCHAR_ASSIGNED -1002

/*Binary properties.*/
static const PropertyEntry
binary_props[] = {
    {"ASCII",           UCHAR_ASCII},
    {"ASCII_Hex_Digit", UCHAR_ASCII_HEX_DIGIT},
    {"AHex",            UCHAR_ASCII_HEX_DIGIT},
    {"Alphabetic",      UCHAR_ALPHABETIC},
    {"Alpha",           UCHAR_ALPHABETIC},
    {"Any",             UCHAR_ANY},
    {"Assigned",        UCHAR_ASSIGNED},
    {"Bidi_Control",    UCHAR_BIDI_CONTROL},
    {"Bidi_C",          UCHAR_BIDI_CONTROL},
    {"Bidi_Mirrored",   UCHAR_BIDI_MIRRORED},
    {"Bidi_M",          UCHAR_BIDI_MIRRORED},
    {"Case_Ignorable",  UCHAR_CASE_IGNORABLE},
    {"CI",              UCHAR_CASE_IGNORABLE},
    {"Cased",           UCHAR_CASED},
    {"Changes_When_Casefolded", UCHAR_CHANGES_WHEN_CASEFOLDED},
    {"CWCF",            UCHAR_CHANGES_WHEN_CASEFOLDED},
    {"Changes_When_Casemapped", UCHAR_CHANGES_WHEN_CASEMAPPED},
    {"CWCM",            UCHAR_CHANGES_WHEN_CASEMAPPED},
    {"Changes_When_Lowercased", UCHAR_CHANGES_WHEN_LOWERCASED},
    {"CWL",             UCHAR_CHANGES_WHEN_LOWERCASED},
    {"Changes_When_NFKC_Casefolded", UCHAR_CHANGES_WHEN_NFKC_CASEFOLDED},
    {"CWKCF",           UCHAR_CHANGES_WHEN_NFKC_CASEFOLDED},
    {"Changes_When_Titlecased", UCHAR_CHANGES_WHEN_TITLECASED},
    {"CWT",             UCHAR_CHANGES_WHEN_TITLECASED},
    {"Changes_When_Uppercased", UCHAR_CHANGES_WHEN_UPPERCASED},
    {"CWU",             UCHAR_CHANGES_WHEN_UPPERCASED},
    {"Dash",            UCHAR_DASH},
    {"Default_Ignorable_Code_Point", UCHAR_DEFAULT_IGNORABLE_CODE_POINT},
    {"DI",              UCHAR_DEFAULT_IGNORABLE_CODE_POINT},
    {"Deprecated",      UCHAR_DEPRECATED},
    {"Dep",             UCHAR_DEPRECATED},
    {"Diacritic",       UCHAR_DIACRITIC},
    {"Dia",             UCHAR_DIACRITIC},
    {"Emoji",           UCHAR_EMOJI},
    {"Emoji_Component", UCHAR_EMOJI_COMPONENT},
    {"EComp",           UCHAR_EMOJI_COMPONENT},
    {"Emoji_Modifier",  UCHAR_EMOJI_MODIFIER},
    {"EMod",            UCHAR_EMOJI_MODIFIER},
    {"Emoji_Modifier_Base",   UCHAR_EMOJI_MODIFIER_BASE},
    {"EBase",           UCHAR_EMOJI_MODIFIER_BASE},
    {"Emoji_Presentation",    UCHAR_EMOJI_PRESENTATION},
    {"EPres",           UCHAR_EMOJI_PRESENTATION},
    {"Extended_Pictographic", UCHAR_EXTENDED_PICTOGRAPHIC},
    {"ExtPict",         UCHAR_EXTENDED_PICTOGRAPHIC},
    {"Extender",        UCHAR_EXTENDER},
    {"Ext",             UCHAR_EXTENDER},
    {"Grapheme_Base",   UCHAR_GRAPHEME_BASE},
    {"Gr_Base",         UCHAR_GRAPHEME_BASE},
    {"Grapheme_Extend", UCHAR_GRAPHEME_EXTEND},
    {"Gr_Ext",          UCHAR_GRAPHEME_EXTEND},
    {"Hex_Digit", 	    UCHAR_HEX_DIGIT},
    {"Hex",             UCHAR_HEX_DIGIT},
    {"IDS_Binary_Operator",    UCHAR_IDS_BINARY_OPERATOR},
    {"IDSB",            UCHAR_IDS_BINARY_OPERATOR},
    {"IDS_Trinary_Operator",   UCHAR_IDS_TRINARY_OPERATOR},
    {"IDST",            UCHAR_IDS_TRINARY_OPERATOR},
    {"ID_Continue", 	UCHAR_ID_CONTINUE},
    {"IDC",             UCHAR_ID_CONTINUE},
    {"ID_Start",        UCHAR_ID_START},
    {"IDS",             UCHAR_ID_START},
    {"Ideographic",     UCHAR_IDEOGRAPHIC},
    {"Ideo",            UCHAR_IDEOGRAPHIC},
    {"Join_Control",    UCHAR_JOIN_CONTROL},
    {"Join_C",          UCHAR_JOIN_CONTROL},
    {"Logical_Order_Exception",UCHAR_LOGICAL_ORDER_EXCEPTION},
    {"LOE",             UCHAR_LOGICAL_ORDER_EXCEPTION},
    {"Lowercase",       UCHAR_LOWERCASE},
    {"Lower",           UCHAR_LOWERCASE},
    {"Math",            UCHAR_MATH},
    {"Noncharacter_Code_Point",UCHAR_NONCHARACTER_CODE_POINT},
    {"NChar",           UCHAR_NONCHARACTER_CODE_POINT},
    {"Pattern_Syntax",  UCHAR_PATTERN_SYNTAX},
    {"Pat_Syn",         UCHAR_PATTERN_SYNTAX},
    {"Pattern_White_Space",    UCHAR_PATTERN_WHITE_SPACE},
    {"Pat_WS",          UCHAR_PATTERN_WHITE_SPACE},
    {"Quotation_Mark",  UCHAR_QUOTATION_MARK},
    {"QMark",           UCHAR_QUOTATION_MARK},
    {"Radical",         UCHAR_RADICAL},
    {"Regional_Indicator",     UCHAR_REGIONAL_INDICATOR},
    {"RI",              UCHAR_REGIONAL_INDICATOR},
    {"Sentence_Terminal",      UCHAR_S_TERM},
    {"STerm",           UCHAR_S_TERM},
    {"Soft_Dotted", 	UCHAR_SOFT_DOTTED},
    {"SD",              UCHAR_SOFT_DOTTED},
    {"Terminal_Punctuation",   UCHAR_TERMINAL_PUNCTUATION},
    {"Term",            UCHAR_TERMINAL_PUNCTUATION},
    {"Unified_Ideograph",      UCHAR_UNIFIED_IDEOGRAPH},
    {"UIdeo",           UCHAR_UNIFIED_IDEOGRAPH},
    {"Uppercase",       UCHAR_UPPERCASE},
    {"Upper",           UCHAR_UPPERCASE},
    {"Variation_Selector",     UCHAR_VARIATION_SELECTOR},
    {"VS",              UCHAR_VARIATION_SELECTOR},
    {"White_Space",     UCHAR_WHITE_SPACE},
    {"space",           UCHAR_WHITE_SPACE},
    {"XID_Continue", 	UCHAR_XID_CONTINUE},
    {"XIDC",            UCHAR_XID_CONTINUE},
    {"XID_Start",       UCHAR_XID_START},
    {"XIDS",            UCHAR_XID_START},
    {NULL}
};

/*Lookup the property.*/
static const PropertyEntry*
lookup_prop (const PropertyEntry *tab, const char *name)
{
    const PropertyEntry *pe;

    pe = tab;
    while (pe->name) {
        if (!strcmp(pe->name, name))
            return pe;

        pe ++;
    }

    return NULL;
}

/*Lookup the category.*/
static const CategoryEntry*
lookup_catebory (const char *name)
{
    const CategoryEntry *ce;

    ce = category_values;
    while (ce->name) {
        if (!strcmp(ce->name, name))
            return ce;

        ce ++;
    }

    return NULL;
}

/*Lookup the script.*/
static const ScriptEntry*
lookup_script (const char *name)
{
    const ScriptEntry *se;

    se = script_code_values;
    while (se->name) {
        if (!strcmp(se->name, name))
            return se;

        se ++;
    }

    return NULL;
}

/*Lookup the unicode property and its value.*/
static RJS_Result
unicode_property_lookup (const char *name, const char *vstr, int *prop, int *value)
{
    const PropertyEntry *pe;
    const CategoryEntry *ce;
    const ScriptEntry   *se;

    if (*name == 0)
        return RJS_ERR;

    if (*vstr == 0) {
        pe = lookup_prop(binary_props, name);
        if (pe) {
            *prop = pe->prop;
        } else if ((ce = lookup_catebory(name))) {
            *prop  = UCHAR_GENERAL_CATEGORY;
            *value = ce->mask;
        } else if ((se = lookup_script(name))) {
            *prop  = UCHAR_SCRIPT;
            *value = se->code;
        } else {
            return RJS_ERR;
        }
    } else {
        pe = lookup_prop(non_binary_props, name);
        if (!pe)
            return RJS_ERR;

        *prop = pe->prop;

        switch(pe->prop) {
        case UCHAR_GENERAL_CATEGORY: {
            ce = lookup_catebory(vstr);
            if (!ce)
                return RJS_ERR;

            *value = ce->mask;
            break;
        }
        case UCHAR_SCRIPT:
        case UCHAR_SCRIPT_EXTENSIONS: {
            se = lookup_script(vstr);
            if (!se)
                return RJS_ERR;

            *value = se->code;
            break;
        }
        default:
            assert(0);
            break;
        }
    }

    return RJS_OK;
}

/*Match the unicode peoperty.*/
static RJS_Bool
unicode_property_match (int prop, int value, int c)
{
    RJS_Bool r;

    switch (prop) {
    case UCHAR_ASCII:
        r = c < 128;
        break;
    case UCHAR_ANY:
        r = RJS_TRUE;
        break;
    case UCHAR_ASSIGNED:
        r = (u_charType(c) != U_UNASSIGNED);
        break;
    case UCHAR_GENERAL_CATEGORY:
        r = (U_GET_GC_MASK(c) & value) ? RJS_TRUE : RJS_FALSE;
        break;
    case UCHAR_SCRIPT: {
        UErrorCode err = U_ZERO_ERROR;

        r = (uscript_getScript(c, &err) == value);
        break;
    }
    case UCHAR_SCRIPT_EXTENSIONS: {
        r = uscript_hasScript(c, value);
        break;
    }
    default:
        r = u_hasBinaryProperty(c, prop);
        break;
    }

    return r;
}
