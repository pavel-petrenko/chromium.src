/* ANSI-C code produced by gperf version 2.7.2 */
/* Command-line: gperf -c -a -L ANSI-C -D -E -C -o -t -k '*' -NfindAttr -Hhash_attr -Wwordlist_attr -s 2 htmlattrs.gperf  */
/* This file is automatically generated from
#htmlattrs.in by makeattrs, do not edit */
/* Copyright 1999 Lars Knoll */
#include "htmlattrs.h"
struct attrs {
    const char *name;
    int id;
};
/* maximum key range = 894, duplicates = 1 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash_attr (register const char *str, register unsigned int len)
{
  static const unsigned short asso_values[] =
    {
      917, 917, 917, 917, 917, 917, 917, 917, 917, 917,
      917, 917, 917, 917, 917, 917, 917, 917, 917, 917,
      917, 917, 917, 917, 917, 917, 917, 917, 917, 917,
      917, 917, 917, 917, 917, 917, 917, 917, 917, 917,
      917, 917, 917, 917, 917,  10, 917, 917, 917, 917,
      917, 917, 917, 917, 917, 917, 917, 917, 917, 917,
      917, 917, 917, 917, 917, 917, 917, 917, 917, 917,
      917, 917, 917, 917, 917, 917, 917, 917, 917, 917,
      917, 917, 917, 917, 917, 917, 917, 917, 917, 917,
      917, 917, 917, 917, 917, 917, 917, 100, 120,  10,
      150,   0,  40, 115, 165,   0,   5, 210,   0,  80,
        0,   0, 125,   0,  20,  35,  60,  50, 110, 215,
      100,  15,  15, 917, 917, 917, 917, 917, 917, 917,
      917, 917, 917, 917, 917, 917, 917, 917, 917, 917,
      917, 917, 917, 917, 917, 917, 917, 917, 917, 917,
      917, 917, 917, 917, 917, 917, 917, 917, 917, 917,
      917, 917, 917, 917, 917, 917, 917, 917, 917, 917,
      917, 917, 917, 917, 917, 917, 917, 917, 917, 917,
      917, 917, 917, 917, 917, 917, 917, 917, 917, 917,
      917, 917, 917, 917, 917, 917, 917, 917, 917, 917,
      917, 917, 917, 917, 917, 917, 917, 917, 917, 917,
      917, 917, 917, 917, 917, 917, 917, 917, 917, 917,
      917, 917, 917, 917, 917, 917, 917, 917, 917, 917,
      917, 917, 917, 917, 917, 917, 917, 917, 917, 917,
      917, 917, 917, 917, 917, 917, 917, 917, 917, 917,
      917, 917, 917, 917, 917, 917
    };
  register int hval = len;

  switch (hval)
    {
      default:
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
const struct attrs *
findAttr (register const char *str, register unsigned int len)
{
  enum
    {
      TOTAL_KEYWORDS = 174,
      MIN_WORD_LENGTH = 2,
      MAX_WORD_LENGTH = 15,
      MIN_HASH_VALUE = 23,
      MAX_HASH_VALUE = 916
    };

  static const struct attrs wordlist_attr[] =
    {
      {"rel", ATTR_REL},
      {"color", ATTR_COLOR},
      {"cols", ATTR_COLS},
      {"size", ATTR_SIZE},
      {"for", ATTR_FOR},
      {"onerror", ATTR_ONERROR},
      {"src", ATTR_SRC},
      {"onscroll", ATTR_ONSCROLL},
      {"cite", ATTR_CITE},
      {"noresize", ATTR_NORESIZE},
      {"onresize", ATTR_ONRESIZE},
      {"min", ATTR_MIN},
      {"left", ATTR_LEFT},
      {"rules", ATTR_RULES},
      {"onselect", ATTR_ONSELECT},
      {"style", ATTR_STYLE},
      {"onreset", ATTR_ONRESET},
      {"title", ATTR_TITLE},
      {"loop", ATTR_LOOP},
      {"rev", ATTR_REV},
      {"clear", ATTR_CLEAR},
      {"content", ATTR_CONTENT},
      {"onfocus", ATTR_ONFOCUS},
      {"id", ATTR_ID},
      {"face", ATTR_FACE},
      {"alt", ATTR_ALT},
      {"code", ATTR_CODE},
      {"version", ATTR_VERSION},
      {"dir", ATTR_DIR},
      {"scope", ATTR_SCOPE},
      {"action", ATTR_ACTION},
      {"name", ATTR_NAME},
      {"class", ATTR_CLASS},
      {"top", ATTR_TOP},
      {"scrolling", ATTR_SCROLLING},
      {"profile", ATTR_PROFILE},
      {"onblur", ATTR_ONBLUR},
      {"precision", ATTR_PRECISION},
      {"object", ATTR_OBJECT},
      {"oversrc", ATTR_OVERSRC},
      {"type", ATTR_TYPE},
      {"results", ATTR_RESULTS},
      {"link", ATTR_LINK},
      {"defer", ATTR_DEFER},
      {"enctype", ATTR_ENCTYPE},
      {"lang", ATTR_LANG},
      {"align", ATTR_ALIGN},
      {"coords", ATTR_COORDS},
      {"text", ATTR_TEXT},
      {"label", ATTR_LABEL},
      {"href", ATTR_HREF},
      {"plain", ATTR_PLAIN},
      {"nohref", ATTR_NOHREF},
      {"onclick", ATTR_ONCLICK},
      {"axis", ATTR_AXIS},
      {"oninput", ATTR_ONINPUT},
      {"frame", ATTR_FRAME},
      {"direction", ATTR_DIRECTION},
      {"nosave", ATTR_NOSAVE},
      {"onload", ATTR_ONLOAD},
      {"selected", ATTR_SELECTED},
      {"span", ATTR_SPAN},
      {"value", ATTR_VALUE},
      {"bgcolor", ATTR_BGCOLOR},
      {"rows", ATTR_ROWS},
      {"colspan", ATTR_COLSPAN},
      {"start", ATTR_START},
      {"incremental", ATTR_INCREMENTAL},
      {"z-index", ATTR_Z_INDEX},
      {"max", ATTR_MAX},
      {"onmouseout", ATTR_ONMOUSEOUT},
      {"declare", ATTR_DECLARE},
      {"readonly", ATTR_READONLY},
      {"scheme", ATTR_SCHEME},
      {"char", ATTR_CHAR},
      {"ondrop", ATTR_ONDROP},
      {"onmouseover", ATTR_ONMOUSEOVER},
      {"onabort", ATTR_ONABORT},
      {"onunload", ATTR_ONUNLOAD},
      {"html", ATTR_HTML},
      {"accept", ATTR_ACCEPT},
      {"alink", ATTR_ALINK},
      {"border", ATTR_BORDER},
      {"longdesc", ATTR_LONGDESC},
      {"composite", ATTR_COMPOSITE},
      {"multiple", ATTR_MULTIPLE},
      {"vlink", ATTR_VLINK},
      {"cellborder", ATTR_CELLBORDER},
      {"valign", ATTR_VALIGN},
      {"media", ATTR_MEDIA},
      {"classid", ATTR_CLASSID},
      {"onsearch", ATTR_ONSEARCH},
      {"scrolldelay", ATTR_SCROLLDELAY},
      {"ismap", ATTR_ISMAP},
      {"onmouseup", ATTR_ONMOUSEUP},
      {"visibility", ATTR_VISIBILITY},
      {"bordercolor", ATTR_BORDERCOLOR},
      {"onsubmit", ATTR_ONSUBMIT},
      {"pagey", ATTR_PAGEY},
      {"target", ATTR_TARGET},
      {"abbr", ATTR_ABBR},
      {"onmousemove", ATTR_ONMOUSEMOVE},
      {"scrollamount", ATTR_SCROLLAMOUNT},
      {"codetype", ATTR_CODETYPE},
      {"pluginurl", ATTR_PLUGINURL},
      {"oncontextmenu", ATTR_ONCONTEXTMENU},
      {"charoff", ATTR_CHAROFF},
      {"vspace", ATTR_VSPACE},
      {"summary", ATTR_SUMMARY},
      {"ondrag", ATTR_ONDRAG},
      {"compact", ATTR_COMPACT},
      {"usemap", ATTR_USEMAP},
      {"charset", ATTR_CHARSET},
      {"onchange", ATTR_ONCHANGE},
      {"challenge", ATTR_CHALLENGE},
      {"cellspacing", ATTR_CELLSPACING},
      {"onkeyup", ATTR_ONKEYUP},
      {"archive", ATTR_ARCHIVE},
      {"data", ATTR_DATA},
      {"prompt", ATTR_PROMPT},
      {"codebase", ATTR_CODEBASE},
      {"accesskey", ATTR_ACCESSKEY},
      {"leftmargin", ATTR_LEFTMARGIN},
      {"shape", ATTR_SHAPE},
      {"keytype", ATTR_KEYTYPE},
      {"hspace", ATTR_HSPACE},
      {"pagex", ATTR_PAGEX},
      {"hreflang", ATTR_HREFLANG},
      {"truespeed", ATTR_TRUESPEED},
      {"onkeypress", ATTR_ONKEYPRESS},
      {"mayscript", ATTR_MAYSCRIPT},
      {"noshade", ATTR_NOSHADE},
      {"datetime", ATTR_DATETIME},
      {"method", ATTR_METHOD},
      {"autosave", ATTR_AUTOSAVE},
      {"wrap", ATTR_WRAP},
      {"nowrap", ATTR_NOWRAP},
      {"valuetype", ATTR_VALUETYPE},
      {"hidden", ATTR_HIDDEN},
      {"ondragenter", ATTR_ONDRAGENTER},
      {"headers", ATTR_HEADERS},
      {"unknown", ATTR_UNKNOWN},
      {"standby", ATTR_STANDBY},
      {"language", ATTR_LANGUAGE},
      {"autocomplete", ATTR_AUTOCOMPLETE},
      {"rowspan", ATTR_ROWSPAN},
      {"topmargin", ATTR_TOPMARGIN},
      {"ondblclick", ATTR_ONDBLCLICK},
      {"height", ATTR_HEIGHT},
      {"behavior", ATTR_BEHAVIOR},
      {"ondragover", ATTR_ONDRAGOVER},
      {"tabindex", ATTR_TABINDEX},
      {"onmousedown", ATTR_ONMOUSEDOWN},
      {"ondragend", ATTR_ONDRAGEND},
      {"checked", ATTR_CHECKED},
      {"frameborder", ATTR_FRAMEBORDER},
      {"disabled", ATTR_DISABLED},
      {"contenteditable", ATTR_CONTENTEDITABLE},
      {"placeholder", ATTR_PLACEHOLDER},
      {"http-equiv", ATTR_HTTP_EQUIV},
      {"width", ATTR_WIDTH},
      {"onkeydown", ATTR_ONKEYDOWN},
      {"tableborder", ATTR_TABLEBORDER},
      {"ondragleave", ATTR_ONDRAGLEAVE},
      {"maxlength", ATTR_MAXLENGTH},
      {"bgproperties", ATTR_BGPROPERTIES},
      {"pluginpage", ATTR_PLUGINPAGE},
      {"cellpadding", ATTR_CELLPADDING},
      {"ondragstart", ATTR_ONDRAGSTART},
      {"pluginspage", ATTR_PLUGINSPAGE},
      {"accept-charset", ATTR_ACCEPT_CHARSET},
      {"background", ATTR_BACKGROUND},
      {"marginheight", ATTR_MARGINHEIGHT},
      {"marginwidth", ATTR_MARGINWIDTH}
    };

  static const short lookup[] =
    {
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,    0,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,    1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,    2,   -1,   -1,   -1,   -1,    3,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,    4,
        -1,   -1,   -1,    5,    6,   -1,   -1,   -1,
        -1,    7,    8,   -1,   -1,   -1, -254, -165,
        -2,   -1,   -1,   11,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        12,   -1,   -1,   -1,   -1,   -1,   13,   -1,
        -1,   14,   -1,   15,   -1,   -1,   -1,   -1,
        -1,   -1,   16,   -1,   -1,   17,   -1,   -1,
        -1,   18,   -1,   -1,   -1,   19,   -1,   20,
        -1,   21,   -1,   -1,   -1,   -1,   22,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        23,   -1,   24,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   25,   26,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   27,   28,   -1,   29,
        30,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        31,   32,   -1,   -1,   33,   34,   -1,   -1,
        35,   -1,   -1,   -1,   36,   -1,   -1,   37,
        -1,   38,   39,   -1,   40,   -1,   -1,   41,
        -1,   -1,   -1,   -1,   -1,   -1,   42,   43,
        -1,   44,   -1,   45,   46,   47,   -1,   -1,
        48,   49,   -1,   -1,   -1,   50,   51,   52,
        -1,   -1,   -1,   -1,   -1,   53,   -1,   54,
        -1,   -1,   55,   -1,   -1,   56,   -1,   -1,
        -1,   57,   -1,   58,   -1,   -1,   -1,   -1,
        59,   -1,   -1,   -1,   -1,   -1,   -1,   60,
        61,   62,   -1,   -1,   -1,   -1,   -1,   -1,
        63,   -1,   64,   -1,   -1,   65,   -1,   -1,
        66,   67,   68,   69,   -1,   70,   -1,   71,
        -1,   -1,   -1,   -1,   -1,   72,   -1,   -1,
        73,   -1,   -1,   74,   -1,   75,   -1,   -1,
        -1,   -1,   76,   77,   78,   79,   -1,   80,
        -1,   -1,   -1,   81,   82,   -1,   83,   84,
        -1,   -1,   -1,   85,   -1,   86,   -1,   -1,
        -1,   -1,   87,   88,   -1,   -1,   -1,   89,
        -1,   90,   91,   -1,   -1,   92,   -1,   -1,
        -1,   93,   -1,   -1,   -1,   94,   95,   96,
        -1,   97,   -1,   -1,   -1,   -1,   -1,   -1,
        98,   99,   -1,   -1,  100,   -1,  101,  102,
       103,  104,   -1,   -1,   -1,  105,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,  106,   -1,
        -1,   -1,  107,  108,   -1,   -1,   -1,  109,
       110,   -1,   -1,   -1,  111,  112,  113,  114,
        -1,   -1,   -1,   -1,   -1,   -1,  115,  116,
        -1,   -1,   -1,   -1,  117,   -1,  118,   -1,
       119,   -1,   -1,   -1,   -1,   -1,   -1,  120,
       121,  122,   -1,   -1,   -1,   -1,  123,   -1,
       124,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,  125,   -1,   -1,   -1,  126,   -1,   -1,
       127,  128,  129,   -1,   -1,   -1,  130,   -1,
        -1,  131,  132,   -1,   -1,  133,   -1,  134,
       135,   -1,  136,   -1,   -1,  137,   -1,  138,
        -1,   -1,   -1,   -1,  139,  140,   -1,   -1,
        -1,   -1,  141,   -1,   -1,   -1,   -1,  142,
       143,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,  144,   -1,   -1,   -1,   -1,  145,   -1,
        -1,   -1,   -1,   -1,   -1,  146,  147,  148,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,  149,   -1,  150,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,  151,   -1,   -1,  152,   -1,   -1,
       153,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       154,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,  155,   -1,  156,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,  157,
        -1,   -1,   -1,   -1,   -1,  158,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,  159,   -1,
        -1,   -1,   -1,  160,   -1,   -1,   -1,  161,
        -1,  162,   -1,   -1,   -1,   -1,  163,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,  164,   -1,   -1,
       165,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       166,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,  167,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,  168,
        -1,   -1,   -1,   -1,  169,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,  170,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,  171,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       172,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,  173
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash_attr (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int index = lookup[key];

          if (index >= 0)
            {
              register const char *s = wordlist_attr[index].name;

              if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
                return &wordlist_attr[index];
            }
          else if (index < -TOTAL_KEYWORDS)
            {
              register int offset = - 1 - TOTAL_KEYWORDS - index;
              register const struct attrs *wordptr = &wordlist_attr[TOTAL_KEYWORDS + lookup[offset]];
              register const struct attrs *wordendptr = wordptr + -lookup[offset + 1];

              while (wordptr < wordendptr)
                {
                  register const char *s = wordptr->name;

                  if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
                    return wordptr;
                  wordptr++;
                }
            }
        }
    }
  return 0;
}


static const char * const attrList[] = {
    "ABBR",
    "ACCEPT-CHARSET",
    "ACCEPT",
    "ACCESSKEY",
    "ACTION",
    "ALIGN",
    "ALINK",
    "ALT",
    "ARCHIVE",
    "AUTOCOMPLETE",
    "AUTOSAVE",
    "AXIS",
    "BACKGROUND",
    "BEHAVIOR",
    "BGCOLOR",
    "BGPROPERTIES",
    "BORDER",
    "BORDERCOLOR",
    "CELLPADDING",
    "CELLSPACING",
    "CHAR",
    "CHALLENGE",
    "CHAROFF",
    "CHARSET",
    "CHECKED",
    "CELLBORDER",
    "CITE",
    "CLASS",
    "CLASSID",
    "CLEAR",
    "CODE",
    "CODEBASE",
    "CODETYPE",
    "COLOR",
    "COLS",
    "COLSPAN",
    "COMPACT",
    "COMPOSITE",
    "CONTENT",
    "CONTENTEDITABLE",
    "COORDS",
    "DATA",
    "DATETIME",
    "DECLARE",
    "DEFER",
    "DIR",
    "DIRECTION",
    "DISABLED",
    "ENCTYPE",
    "FACE",
    "FOR",
    "FRAME",
    "FRAMEBORDER",
    "HEADERS",
    "HEIGHT",
    "HIDDEN",
    "HREF",
    "HREFLANG",
    "HSPACE",
    "HTML",
    "HTTP-EQUIV",
    "ID",
    "INCREMENTAL",
    "ISMAP",
    "KEYTYPE",
    "LABEL",
    "LANG",
    "LANGUAGE",
    "LEFT",
    "LEFTMARGIN",
    "LINK",
    "LONGDESC",
    "LOOP",
    "MARGINHEIGHT",
    "MARGINWIDTH",
    "MAX",
    "MAXLENGTH",
    "MAYSCRIPT",
    "MEDIA",
    "METHOD",
    "MIN",
    "MULTIPLE",
    "NAME",
    "NOHREF",
    "NORESIZE",
    "NOSAVE",
    "NOSHADE",
    "NOWRAP",
    "OBJECT",
    "ONABORT",
    "ONBLUR",
    "ONCHANGE",
    "ONCLICK",
    "ONCONTEXTMENU",
    "ONDBLCLICK",
    "ONDRAG",
    "ONDRAGEND",
    "ONDRAGENTER",
    "ONDRAGLEAVE",
    "ONDRAGOVER",
    "ONDRAGSTART",
    "ONDROP",
    "ONERROR",
    "ONFOCUS",
    "ONINPUT",
    "ONKEYDOWN",
    "ONKEYPRESS",
    "ONKEYUP",
    "ONLOAD",
    "ONMOUSEDOWN",
    "ONMOUSEMOVE",
    "ONMOUSEOUT",
    "ONMOUSEOVER",
    "ONMOUSEUP",
    "ONRESET",
    "ONRESIZE",
    "ONSCROLL",
    "ONSEARCH",
    "ONSELECT",
    "ONSUBMIT",
    "ONUNLOAD",
    "OVERSRC",
    "PAGEX",
    "PAGEY",
    "PLACEHOLDER",
    "PLAIN",
    "PLUGINPAGE",
    "PLUGINSPAGE",
    "PLUGINURL",
    "PRECISION",
    "PROFILE",
    "PROMPT",
    "READONLY",
    "REL",
    "RESULTS",
    "REV",
    "ROWS",
    "ROWSPAN",
    "RULES",
    "SCHEME",
    "SCOPE",
    "SCROLLAMOUNT",
    "SCROLLDELAY",
    "SCROLLING",
    "SELECTED",
    "SHAPE",
    "SIZE",
    "SPAN",
    "SRC",
    "STANDBY",
    "START",
    "STYLE",
    "SUMMARY",
    "TABINDEX",
    "TABLEBORDER",
    "TARGET",
    "TEXT",
    "TITLE",
    "TOP",
    "TOPMARGIN",
    "TRUESPEED",
    "TYPE",
    "UNKNOWN",
    "USEMAP",
    "VALIGN",
    "VALUE",
    "VALUETYPE",
    "VERSION",
    "VISIBILITY",
    "VLINK",
    "VSPACE",
    "WIDTH",
    "WRAP",
    "Z-INDEX",
    0
};
DOM::DOMString getAttrName(unsigned short id)
{
    return attrList[id-1];
}
