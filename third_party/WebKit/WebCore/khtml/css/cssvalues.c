/* ANSI-C code produced by gperf version 2.7.2 */
/* Command-line: gperf -L ANSI-C -E -C -n -o -t -k '*' -NfindValue -Hhash_val -Wwordlist_value -D cssvalues.gperf  */
/* This file is automatically generated from cssvalues.in by makevalues, do not edit */
/* Copyright 1999 W. Bastian */
#include "cssvalues.h"
struct css_value {
    const char *name;
    int id;
};
/* maximum key range = 1609, duplicates = 1 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash_val (register const char *str, register unsigned int len)
{
  static const unsigned short asso_values[] =
    {
      1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609,
      1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609,
      1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609,
      1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609,
      1609, 1609, 1609, 1609, 1609,   25, 1609, 1609,    0,   10,
        15,   20,   25,   30,   35,   45,    5,    0, 1609, 1609,
      1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609,
      1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609,
      1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609,
      1609, 1609, 1609, 1609, 1609, 1609, 1609,    0,   73,  242,
        15,    4,   30,  250,  145,   65,    0,  215,    0,  153,
        40,    5,  200,  120,  135,   10,    0,  154,  113,  201,
       190,  234,  144, 1609, 1609, 1609, 1609, 1609, 1609, 1609,
      1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609,
      1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609,
      1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609,
      1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609,
      1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609,
      1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609,
      1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609,
      1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609,
      1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609,
      1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609,
      1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609,
      1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609, 1609,
      1609, 1609, 1609, 1609, 1609, 1609
    };
  register int hval = 0;

  switch (len)
    {
      default:
      case 22:
        hval += asso_values[(unsigned char)str[21]];
      case 21:
        hval += asso_values[(unsigned char)str[20]];
      case 20:
        hval += asso_values[(unsigned char)str[19]];
      case 19:
        hval += asso_values[(unsigned char)str[18]];
      case 18:
        hval += asso_values[(unsigned char)str[17]];
      case 17:
        hval += asso_values[(unsigned char)str[16]];
      case 16:
        hval += asso_values[(unsigned char)str[15]];
      case 15:
        hval += asso_values[(unsigned char)str[14]];
      case 14:
        hval += asso_values[(unsigned char)str[13]];
      case 13:
        hval += asso_values[(unsigned char)str[12]];
      case 12:
        hval += asso_values[(unsigned char)str[11]];
      case 11:
        hval += asso_values[(unsigned char)str[10]];
      case 10:
        hval += asso_values[(unsigned char)str[9]];
      case 9:
        hval += asso_values[(unsigned char)str[8]];
      case 8:
        hval += asso_values[(unsigned char)str[7]];
      case 7:
        hval += asso_values[(unsigned char)str[6]];
      case 6:
        hval += asso_values[(unsigned char)str[5]];
      case 5:
        hval += asso_values[(unsigned char)str[4]];
      case 4:
        hval += asso_values[(unsigned char)str[3]];
      case 3:
        hval += asso_values[(unsigned char)str[2]];
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

#ifdef __GNUC__
__inline
#endif
const struct css_value *
findValue (register const char *str, register unsigned int len)
{
  enum
    {
      TOTAL_KEYWORDS = 257,
      MIN_WORD_LENGTH = 2,
      MAX_WORD_LENGTH = 22,
      MIN_HASH_VALUE = 0,
      MAX_HASH_VALUE = 1608
    };

  static const struct css_value wordlist_value[] =
    {
      {"900", CSS_VAL_900},
      {"teal", CSS_VAL_TEAL},
      {"800", CSS_VAL_800},
      {"100", CSS_VAL_100},
      {"200", CSS_VAL_200},
      {"300", CSS_VAL_300},
      {"400", CSS_VAL_400},
      {"500", CSS_VAL_500},
      {"left", CSS_VAL_LEFT},
      {"600", CSS_VAL_600},
      {"dotted", CSS_VAL_DOTTED},
      {"fast", CSS_VAL_FAST},
      {"700", CSS_VAL_700},
      {"end", CSS_VAL_END},
      {"table", CSS_VAL_TABLE},
      {"none", CSS_VAL_NONE},
      {"bold", CSS_VAL_BOLD},
      {"slide", CSS_VAL_SLIDE},
      {"solid", CSS_VAL_SOLID},
      {"inset", CSS_VAL_INSET},
      {"level", CSS_VAL_LEVEL},
      {"ltr", CSS_VAL_LTR},
      {"rtl", CSS_VAL_RTL},
      {"start", CSS_VAL_START},
      {"red", CSS_VAL_RED},
      {"auto", CSS_VAL_AUTO},
      {"small", CSS_VAL_SMALL},
      {"ahead", CSS_VAL_AHEAD},
      {"outset", CSS_VAL_OUTSET},
      {"loud", CSS_VAL_LOUD},
      {"alternate", CSS_VAL_ALTERNATE},
      {"olive", CSS_VAL_OLIVE},
      {"dashed", CSS_VAL_DASHED},
      {"text", CSS_VAL_TEXT},
      {"above", CSS_VAL_ABOVE},
      {"baseline", CSS_VAL_BASELINE},
      {"avoid", CSS_VAL_AVOID},
      {"inside", CSS_VAL_INSIDE},
      {"hand", CSS_VAL_HAND},
      {"default", CSS_VAL_DEFAULT},
      {"top", CSS_VAL_TOP},
      {"inline", CSS_VAL_INLINE},
      {"slow", CSS_VAL_SLOW},
      {"lime", CSS_VAL_LIME},
      {"both", CSS_VAL_BOTH},
      {"hide", CSS_VAL_HIDE},
      {"blue", CSS_VAL_BLUE},
      {"bolder", CSS_VAL_BOLDER},
      {"initial", CSS_VAL_INITIAL},
      {"bottom", CSS_VAL_BOTTOM},
      {"sub", CSS_VAL_SUB},
      {"serif", CSS_VAL_SERIF},
      {"absolute", CSS_VAL_ABSOLUTE},
      {"embed", CSS_VAL_EMBED},
      {"thin", CSS_VAL_THIN},
      {"double", CSS_VAL_DOUBLE},
      {"middle", CSS_VAL_MIDDLE},
      {"outside", CSS_VAL_OUTSIDE},
      {"down", CSS_VAL_DOWN},
      {"wait", CSS_VAL_WAIT},
      {"aqua", CSS_VAL_AQUA},
      {"move", CSS_VAL_MOVE},
      {"below", CSS_VAL_BELOW},
      {"hidden", CSS_VAL_HIDDEN},
      {"smaller", CSS_VAL_SMALLER},
      {"fixed", CSS_VAL_FIXED},
      {"infinite", CSS_VAL_INFINITE},
      {"fantasy", CSS_VAL_FANTASY},
      {"inline-table", CSS_VAL_INLINE_TABLE},
      {"static", CSS_VAL_STATIC},
      {"relative", CSS_VAL_RELATIVE},
      {"list-item", CSS_VAL_LIST_ITEM},
      {"silver", CSS_VAL_SILVER},
      {"sans-serif", CSS_VAL_SANS_SERIF},
      {"visible", CSS_VAL_VISIBLE},
      {"disc", CSS_VAL_DISC},
      {"normal", CSS_VAL_NORMAL},
      {"infotext", CSS_VAL_INFOTEXT},
      {"maroon", CSS_VAL_MAROON},
      {"pre", CSS_VAL_PRE},
      {"repeat", CSS_VAL_REPEAT},
      {"lower", CSS_VAL_LOWER},
      {"table-cell", CSS_VAL_TABLE_CELL},
      {"help", CSS_VAL_HELP},
      {"menu", CSS_VAL_MENU},
      {"icon", CSS_VAL_ICON},
      {"separate", CSS_VAL_SEPARATE},
      {"up", CSS_VAL_UP},
      {"invert", CSS_VAL_INVERT},
      {"show", CSS_VAL_SHOW},
      {"overline", CSS_VAL_OVERLINE},
      {"single", CSS_VAL_SINGLE},
      {"italic", CSS_VAL_ITALIC},
      {"condensed", CSS_VAL_CONDENSED},
      {"x-small", CSS_VAL_X_SMALL},
      {"navy", CSS_VAL_NAVY},
      {"large", CSS_VAL_LARGE},
      {"e-resize", CSS_VAL_E_RESIZE},
      {"scroll", CSS_VAL_SCROLL},
      {"blink", CSS_VAL_BLINK},
      {"s-resize", CSS_VAL_S_RESIZE},
      {"se-resize", CSS_VAL_SE_RESIZE},
      {"cross", CSS_VAL_CROSS},
      {"reverse", CSS_VAL_REVERSE},
      {"status-bar", CSS_VAL_STATUS_BAR},
      {"mix", CSS_VAL_MIX},
      {"no-repeat", CSS_VAL_NO_REPEAT},
      {"white", CSS_VAL_WHITE},
      {"wider", CSS_VAL_WIDER},
      {"oblique", CSS_VAL_OBLIQUE},
      {"square", CSS_VAL_SQUARE},
      {"text-top", CSS_VAL_TEXT_TOP},
      {"center", CSS_VAL_CENTER},
      {"n-resize", CSS_VAL_N_RESIZE},
      {"ne-resize", CSS_VAL_NE_RESIZE},
      {"green", CSS_VAL_GREEN},
      {"orange", CSS_VAL_ORANGE},
      {"armenian", CSS_VAL_ARMENIAN},
      {"table-row", CSS_VAL_TABLE_ROW},
      {"yellow", CSS_VAL_YELLOW},
      {"always", CSS_VAL_ALWAYS},
      {"pointer", CSS_VAL_POINTER},
      {"inherit", CSS_VAL_INHERIT},
      {"text-bottom", CSS_VAL_TEXT_BOTTOM},
      {"underline", CSS_VAL_UNDERLINE},
      {"read-only", CSS_VAL_READ_ONLY},
      {"run-in", CSS_VAL_RUN_IN},
      {"collapse", CSS_VAL_COLLAPSE},
      {"buttontext", CSS_VAL_BUTTONTEXT},
      {"expanded", CSS_VAL_EXPANDED},
      {"ridge", CSS_VAL_RIDGE},
      {"katakana", CSS_VAL_KATAKANA},
      {"lower-latin", CSS_VAL_LOWER_LATIN},
      {"decimal", CSS_VAL_DECIMAL},
      {"overlay", CSS_VAL_OVERLAY},
      {"justify", CSS_VAL_JUSTIFY},
      {"super", CSS_VAL_SUPER},
      {"inline-axis", CSS_VAL_INLINE_AXIS},
      {"landscape", CSS_VAL_LANDSCAPE},
      {"groove", CSS_VAL_GROOVE},
      {"unfurl", CSS_VAL_UNFURL},
      {"larger", CSS_VAL_LARGER},
      {"window", CSS_VAL_WINDOW},
      {"black", CSS_VAL_BLACK},
      {"forwards", CSS_VAL_FORWARDS},
      {"block", CSS_VAL_BLOCK},
      {"stretch", CSS_VAL_STRETCH},
      {"horizontal", CSS_VAL_HORIZONTAL},
      {"portrait", CSS_VAL_PORTRAIT},
      {"medium", CSS_VAL_MEDIUM},
      {"menutext", CSS_VAL_MENUTEXT},
      {"buttonface", CSS_VAL_BUTTONFACE},
      {"caption", CSS_VAL_CAPTION},
      {"open-quote", CSS_VAL_OPEN_QUOTE},
      {"repeat-x", CSS_VAL_REPEAT_X},
      {"vertical", CSS_VAL_VERTICAL},
      {"hebrew", CSS_VAL_HEBREW},
      {"transparent", CSS_VAL_TRANSPARENT},
      {"xx-small", CSS_VAL_XX_SMALL},
      {"close-quote", CSS_VAL_CLOSE_QUOTE},
      {"marquee", CSS_VAL_MARQUEE},
      {"multiple", CSS_VAL_MULTIPLE},
      {"threedface", CSS_VAL_THREEDFACE},
      {"nowrap", CSS_VAL_NOWRAP},
      {"crop", CSS_VAL_CROP},
      {"read-write", CSS_VAL_READ_WRITE},
      {"w-resize", CSS_VAL_W_RESIZE},
      {"right", CSS_VAL_RIGHT},
      {"-khtml-left", CSS_VAL__KHTML_LEFT},
      {"sw-resize", CSS_VAL_SW_RESIZE},
      {"lighter", CSS_VAL_LIGHTER},
      {"scrollbar", CSS_VAL_SCROLLBAR},
      {"lowercase", CSS_VAL_LOWERCASE},
      {"repeat-y", CSS_VAL_REPEAT_Y},
      {"x-large", CSS_VAL_X_LARGE},
      {"gray", CSS_VAL_GRAY},
      {"grey", CSS_VAL_GREY},
      {"no-open-quote", CSS_VAL_NO_OPEN_QUOTE},
      {"nw-resize", CSS_VAL_NW_RESIZE},
      {"semi-condensed", CSS_VAL_SEMI_CONDENSED},
      {"hiragana", CSS_VAL_HIRAGANA},
      {"no-close-quote", CSS_VAL_NO_CLOSE_QUOTE},
      {"small-caps", CSS_VAL_SMALL_CAPS},
      {"fuchsia", CSS_VAL_FUCHSIA},
      {"buttonshadow", CSS_VAL_BUTTONSHADOW},
      {"table-caption", CSS_VAL_TABLE_CAPTION},
      {"narrower", CSS_VAL_NARROWER},
      {"monospace", CSS_VAL_MONOSPACE},
      {"thick", CSS_VAL_THICK},
      {"threedshadow", CSS_VAL_THREEDSHADOW},
      {"circle", CSS_VAL_CIRCLE},
      {"ultra-condensed", CSS_VAL_ULTRA_CONDENSED},
      {"purple", CSS_VAL_PURPLE},
      {"table-column", CSS_VAL_TABLE_COLUMN},
      {"lower-roman", CSS_VAL_LOWER_ROMAN},
      {"lower-alpha", CSS_VAL_LOWER_ALPHA},
      {"bidi-override", CSS_VAL_BIDI_OVERRIDE},
      {"capitalize", CSS_VAL_CAPITALIZE},
      {"windowtext", CSS_VAL_WINDOWTEXT},
      {"-khtml-auto", CSS_VAL__KHTML_AUTO},
      {"cursive", CSS_VAL_CURSIVE},
      {"message-box", CSS_VAL_MESSAGE_BOX},
      {"semi-expanded", CSS_VAL_SEMI_EXPANDED},
      {"extra-condensed", CSS_VAL_EXTRA_CONDENSED},
      {"small-caption", CSS_VAL_SMALL_CAPTION},
      {"higher", CSS_VAL_HIGHER},
      {"captiontext", CSS_VAL_CAPTIONTEXT},
      {"crosshair", CSS_VAL_CROSSHAIR},
      {"georgian", CSS_VAL_GEORGIAN},
      {"-khtml-text", CSS_VAL__KHTML_TEXT},
      {"inline-block", CSS_VAL_INLINE_BLOCK},
      {"ultra-expanded", CSS_VAL_ULTRA_EXPANDED},
      {"activeborder", CSS_VAL_ACTIVEBORDER},
      {"xx-large", CSS_VAL_XX_LARGE},
      {"graytext", CSS_VAL_GRAYTEXT},
      {"extra-expanded", CSS_VAL_EXTRA_EXPANDED},
      {"upper-latin", CSS_VAL_UPPER_LATIN},
      {"block-axis", CSS_VAL_BLOCK_AXIS},
      {"-khtml-box", CSS_VAL__KHTML_BOX},
      {"compact", CSS_VAL_COMPACT},
      {"katakana-iroha", CSS_VAL_KATAKANA_IROHA},
      {"windowframe", CSS_VAL_WINDOWFRAME},
      {"-khtml-link", CSS_VAL__KHTML_LINK},
      {"-khtml-body", CSS_VAL__KHTML_BODY},
      {"backwards", CSS_VAL_BACKWARDS},
      {"inactiveborder", CSS_VAL_INACTIVEBORDER},
      {"uppercase", CSS_VAL_UPPERCASE},
      {"line-through", CSS_VAL_LINE_THROUGH},
      {"activecaption", CSS_VAL_ACTIVECAPTION},
      {"lower-greek", CSS_VAL_LOWER_GREEK},
      {"-khtml-center", CSS_VAL__KHTML_CENTER},
      {"hiragana-iroha", CSS_VAL_HIRAGANA_IROHA},
      {"-khtml-baseline-middle", CSS_VAL__KHTML_BASELINE_MIDDLE},
      {"threeddarkshadow", CSS_VAL_THREEDDARKSHADOW},
      {"table-footer-group", CSS_VAL_TABLE_FOOTER_GROUP},
      {"upper-roman", CSS_VAL_UPPER_ROMAN},
      {"upper-alpha", CSS_VAL_UPPER_ALPHA},
      {"highlight", CSS_VAL_HIGHLIGHT},
      {"-khtml-inline-box", CSS_VAL__KHTML_INLINE_BOX},
      {"inactivecaption", CSS_VAL_INACTIVECAPTION},
      {"background", CSS_VAL_BACKGROUND},
      {"threedlightshadow", CSS_VAL_THREEDLIGHTSHADOW},
      {"-khtml-nowrap", CSS_VAL__KHTML_NOWRAP},
      {"-khtml-right", CSS_VAL__KHTML_RIGHT},
      {"table-header-group", CSS_VAL_TABLE_HEADER_GROUP},
      {"decimal-leading-zero", CSS_VAL_DECIMAL_LEADING_ZERO},
      {"table-row-group", CSS_VAL_TABLE_ROW_GROUP},
      {"highlighttext", CSS_VAL_HIGHLIGHTTEXT},
      {"infobackground", CSS_VAL_INFOBACKGROUND},
      {"inactivecaptiontext", CSS_VAL_INACTIVECAPTIONTEXT},
      {"-khtml-activelink", CSS_VAL__KHTML_ACTIVELINK},
      {"buttonhighlight", CSS_VAL_BUTTONHIGHLIGHT},
      {"threedhighlight", CSS_VAL_THREEDHIGHLIGHT},
      {"appworkspace", CSS_VAL_APPWORKSPACE},
      {"table-column-group", CSS_VAL_TABLE_COLUMN_GROUP},
      {"-khtml-xxx-large", CSS_VAL__KHTML_XXX_LARGE},
      {"cjk-ideographic", CSS_VAL_CJK_IDEOGRAPHIC}
    };

  static const short lookup[] =
    {
         0,   -1,   -1,   -1,    1,    2,   -1,   -1,
        -1,   -1,    3,   -1,   -1,   -1,   -1,    4,
        -1,   -1,   -1,   -1,    5,   -1,   -1,   -1,
        -1,    6,   -1,   -1,   -1,   -1,    7,   -1,
        -1,   -1,    8,    9,   -1,   -1,   -1,   10,
        11,   -1,   -1,   -1,   -1,   12,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   13,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   14,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   15,   -1,   -1,   -1,   16,   17,   18,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   19,
        -1,   20,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1, -394,
      -236,   -2,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   23,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   24,   -1,   -1,   -1,   -1,   25,
        -1,   -1,   -1,   26,   27,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   28,   29,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   30,
        -1,   -1,   -1,   31,   -1,   32,   -1,   -1,
        -1,   -1,   33,   34,   35,   -1,   36,   37,
        38,   -1,   -1,   39,   -1,   40,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   41,   -1,
        42,   -1,   -1,   -1,   -1,   -1,   43,   44,
        -1,   -1,   -1,   -1,   -1,   45,   -1,   46,
        47,   -1,   -1,   48,   49,   50,   -1,   -1,
        -1,   -1,   -1,   -1,   51,   -1,   52,   -1,
        -1,   53,   54,   55,   56,   57,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   58,   -1,   -1,
        -1,   -1,   59,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   60,   61,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   62,   63,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   64,   -1,
        65,   -1,   -1,   -1,   -1,   66,   -1,   -1,
        -1,   -1,   67,   -1,   68,   69,   -1,   -1,
        -1,   70,   71,   -1,   -1,   -1,   -1,   72,
        -1,   73,   74,   -1,   75,   76,   77,   -1,
        -1,   -1,   78,   79,   -1,   -1,   -1,   80,
        -1,   81,   -1,   -1,   82,   83,   -1,   84,
        85,   86,   87,   -1,   -1,   88,   -1,   -1,
        -1,   89,   -1,   -1,   -1,   -1,   90,   -1,
        -1,   91,   -1,   -1,   92,   -1,   -1,   93,
        -1,   -1,   94,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   95,   -1,   96,   -1,   97,
        98,   99,   -1,   -1,   -1,  100,   -1,   -1,
        -1,  101,  102,   -1,   -1,  103,   -1,  104,
       105,   -1,   -1,   -1,   -1,  106,   -1,  107,
        -1,   -1,   -1,   -1,  108,  109,   -1,  110,
       111,  112,   -1,  113,   -1,   -1,   -1,  114,
        -1,  115,  116,   -1,   -1,  117,   -1,   -1,
        -1,   -1,   -1,  118,  119,  120,   -1,   -1,
        -1,  121,   -1,   -1,   -1,   -1,  122,  123,
        -1,  124,  125,  126,   -1,  127,   -1,   -1,
        -1,   -1,  128,   -1,  129,  130,  131,   -1,
        -1,   -1,   -1,  132,   -1,   -1,   -1,  133,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,  134,   -1,  135,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,  136,
       137,   -1,   -1,   -1,   -1,   -1,   -1,  138,
       139,  140,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,  141,   -1,   -1,  142,
        -1,   -1,  143,  144,   -1,   -1,   -1,  145,
       146,   -1,   -1,  147,  148,   -1,   -1,   -1,
       149,  150,   -1,   -1,  151,   -1,   -1,   -1,
       152,   -1,   -1,   -1,   -1,  153,  154,  155,
        -1,   -1,  156,   -1,  157,   -1,   -1,   -1,
       158,  159,  160,   -1,   -1,   -1,   -1,   -1,
       161,   -1,   -1,  162,   -1,  163,  164,   -1,
       165,   -1,   -1,   -1,  166,   -1,   -1,   -1,
        -1,   -1,   -1,  167,   -1,  168,  169,  170,
       171,  172,  173,   -1,  174,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,  175,   -1,   -1,   -1,  176,
        -1,   -1,   -1,  177,  178,   -1,   -1,   -1,
       179,   -1,   -1,  180,   -1,   -1,   -1,  181,
       182,   -1,   -1,   -1,   -1,   -1,  183,   -1,
       184,   -1,   -1,   -1,   -1,   -1,  185,  186,
        -1,   -1,   -1,  187,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,  188,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,  189,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       190,  191,   -1,   -1,   -1,  192,   -1,   -1,
       193,   -1,   -1,   -1,   -1,   -1,   -1,  194,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,  195,   -1,   -1,   -1,  196,
       197,  198,  199,  200,  201,  202,   -1,   -1,
        -1,  203,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,  204,   -1,   -1,   -1,
       205,   -1,  206,  207,   -1,  208,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,  209,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,  210,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,  211,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,  212,
        -1,   -1,  213,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,  214,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,  215,  216,
        -1,  217,   -1,   -1,   -1,   -1,   -1,  218,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,  219,   -1,   -1,  220,   -1,   -1,
        -1,  221,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,  222,   -1,   -1,   -1,   -1,
        -1,   -1,  223,  224,   -1,   -1,   -1,   -1,
       225,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,  226,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       227,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       228,   -1,  229,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,  230,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,  231,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,  232,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,  233,   -1,   -1,   -1,
        -1,   -1,  234,  235,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,  236,
        -1,  237,   -1,   -1,   -1,   -1,  238,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,  239,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,  240,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,  241,   -1,   -1,   -1,   -1,
       242,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,  243,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,  244,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,  245,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,  246,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,  247,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,  248,   -1,   -1,
        -1,   -1,   -1,  249,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,  250,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,  251,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       252,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,  253,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,  254,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,  255,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       256
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash_val (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int index = lookup[key];

          if (index >= 0)
            {
              register const char *s = wordlist_value[index].name;

              if (*str == *s && !strcmp (str + 1, s + 1))
                return &wordlist_value[index];
            }
          else if (index < -TOTAL_KEYWORDS)
            {
              register int offset = - 1 - TOTAL_KEYWORDS - index;
              register const struct css_value *wordptr = &wordlist_value[TOTAL_KEYWORDS + lookup[offset]];
              register const struct css_value *wordendptr = wordptr + -lookup[offset + 1];

              while (wordptr < wordendptr)
                {
                  register const char *s = wordptr->name;

                  if (*str == *s && !strcmp (str + 1, s + 1))
                    return wordptr;
                  wordptr++;
                }
            }
        }
    }
  return 0;
}
static const char * const valueList[] = {
"",
"inherit", 
"initial", 
"none", 
"hidden", 
"inset", 
"groove", 
"ridge", 
"outset", 
"dotted", 
"dashed", 
"solid", 
"double", 
"caption", 
"icon", 
"menu", 
"message-box", 
"small-caption", 
"status-bar", 
"italic", 
"oblique", 
"small-caps", 
"normal", 
"bold", 
"bolder", 
"lighter", 
"100", 
"200", 
"300", 
"400", 
"500", 
"600", 
"700", 
"800", 
"900", 
"xx-small", 
"x-small", 
"small", 
"medium", 
"large", 
"x-large", 
"xx-large", 
"-khtml-xxx-large", 
"smaller", 
"larger", 
"wider", 
"narrower", 
"ultra-condensed", 
"extra-condensed", 
"condensed", 
"semi-condensed", 
"semi-expanded", 
"expanded", 
"extra-expanded", 
"ultra-expanded", 
"serif", 
"sans-serif", 
"cursive", 
"fantasy", 
"monospace", 
"-khtml-body", 
"aqua", 
"black", 
"blue", 
"fuchsia", 
"gray", 
"green", 
"lime", 
"maroon", 
"navy", 
"olive", 
"orange", 
"purple", 
"red", 
"silver", 
"teal", 
"white", 
"yellow", 
"transparent", 
"-khtml-link", 
"-khtml-activelink", 
"activeborder", 
"activecaption", 
"appworkspace", 
"background", 
"buttonface", 
"buttonhighlight", 
"buttonshadow", 
"buttontext", 
"captiontext", 
"graytext", 
"highlight", 
"highlighttext", 
"inactiveborder", 
"inactivecaption", 
"inactivecaptiontext", 
"infobackground", 
"infotext", 
"menutext", 
"scrollbar", 
"threeddarkshadow", 
"threedface", 
"threedhighlight", 
"threedlightshadow", 
"threedshadow", 
"window", 
"windowframe", 
"windowtext", 
"grey", 
"-khtml-text", 
"repeat", 
"repeat-x", 
"repeat-y", 
"no-repeat", 
"baseline", 
"middle", 
"sub", 
"super", 
"text-top", 
"text-bottom", 
"top", 
"bottom", 
"-khtml-baseline-middle", 
"-khtml-auto", 
"left", 
"right", 
"center", 
"justify", 
"-khtml-left", 
"-khtml-right", 
"-khtml-center", 
"outside", 
"inside", 
"disc", 
"circle", 
"square", 
"decimal", 
"decimal-leading-zero", 
"lower-roman", 
"upper-roman", 
"lower-greek", 
"lower-alpha", 
"lower-latin", 
"upper-alpha", 
"upper-latin", 
"hebrew", 
"armenian", 
"georgian", 
"cjk-ideographic", 
"hiragana", 
"katakana", 
"hiragana-iroha", 
"katakana-iroha", 
"inline", 
"block", 
"list-item", 
"run-in", 
"compact", 
"inline-block", 
"table", 
"inline-table", 
"table-row-group", 
"table-header-group", 
"table-footer-group", 
"table-row", 
"table-column-group", 
"table-column", 
"table-cell", 
"table-caption", 
"-khtml-box", 
"-khtml-inline-box", 
"auto", 
"crosshair", 
"default", 
"pointer", 
"move", 
"e-resize", 
"ne-resize", 
"nw-resize", 
"n-resize", 
"se-resize", 
"sw-resize", 
"s-resize", 
"w-resize", 
"text", 
"wait", 
"help", 
"ltr", 
"rtl", 
"capitalize", 
"uppercase", 
"lowercase", 
"visible", 
"collapse", 
"above", 
"absolute", 
"always", 
"avoid", 
"below", 
"bidi-override", 
"blink", 
"both", 
"close-quote", 
"crop", 
"cross", 
"embed", 
"fixed", 
"hand", 
"hide", 
"higher", 
"invert", 
"landscape", 
"level", 
"line-through", 
"loud", 
"lower", 
"marquee", 
"mix", 
"no-close-quote", 
"no-open-quote", 
"nowrap", 
"open-quote", 
"overlay", 
"overline", 
"portrait", 
"pre", 
"relative", 
"scroll", 
"separate", 
"show", 
"static", 
"thick", 
"thin", 
"underline", 
"-khtml-nowrap", 
"stretch", 
"start", 
"end", 
"reverse", 
"horizontal", 
"vertical", 
"inline-axis", 
"block-axis", 
"single", 
"multiple", 
"forwards", 
"backwards", 
"ahead", 
"up", 
"down", 
"slow", 
"fast", 
"infinite", 
"slide", 
"alternate", 
"unfurl", 
"read-only", 
"read-write", 
    0
};
DOMString getValueName(unsigned short id)
{
    if(id >= CSS_VAL_TOTAL || id == 0)
      return DOMString();
    else
      return DOMString(valueList[id]);
};

