/* colour.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2015-2022 Stuart Swales */

#include "common/gflags.h"

#include "datafmt.h"

#include "riscos_x.h"

#if defined(EXTENDED_COLOUR)

/* decode / encode a decimal_index,hex_colour pair for F_COLOUR options
 * in dialogue boxes / documents / command sequences
 */

extern U32
decode_wimp_colour_value(
    const char * from)
{
    char * eptr;
    U32 wimp_colour_value = (U32) strtoul((const char *) from, &eptr, 0);

    wimp_colour_value &= 0xF;

    if((NULL != eptr) && (',' == *eptr) && ('0' == eptr[1]) && ('x' == eptr[2]))
    {   /* optional BBGGRR00 present */
        wimp_colour_value = (U32) strtoul(eptr + 1, &eptr, 0);

        riscos_set_wimp_colour_value_index_byte(&wimp_colour_value);
    }

    reportf("decode_wimp_colour_value(%s): " U32_XTFMT, from, wimp_colour_value);
    return(wimp_colour_value);
}

extern int /* sprintf-like */
encode_wimp_colour_value(
    /*out*/ char * buffer,
    U32 buffer_bytes,
    U32 wimp_colour_value)
{
    int len;

    if(0 != (0x10 & wimp_colour_value))
        len = snprintf(buffer, buffer_bytes, "%d,0x%.8X", (int) (wimp_colour_value & 0x0F), (int) (wimp_colour_value & ~0x1F)); /* decode will map index */
    else
        len = snprintf(buffer, buffer_bytes, "%d",        (int) (wimp_colour_value & 0x0F));

    return(len);
}

#endif /* EXTENDED_COLOUR */

/******************************************************************************
*
* get a colour value from a string
*
******************************************************************************/

#define html_colr(rgb, name) \
    { (rgb), sizeof(name)-1, (name) }

static struct HTML_COLOR_NAME
{
    U32 rgb;
    U32 name_length;
    const char * name;
}
html_color_name[] =
{
    html_colr(0xF0F8FF, "AliceBlue"), 
    html_colr(0xFAEBD7, "AntiqueWhite"), 
    html_colr(0x00FFFF, "Aqua"), 
    html_colr(0x7FFFD4, "Aquamarine"), 
    html_colr(0xF0FFFF, "Azure"), 
    html_colr(0xF5F5DC, "Beige"), 
    html_colr(0xFFE4C4, "Bisque"), 
    html_colr(0x000000, "Black"), 
    html_colr(0xFFEBCD, "BlanchedAlmond"), 
    html_colr(0x0000FF, "Blue"), 
    html_colr(0x8A2BE2, "BlueViolet"), 
    html_colr(0xA52A2A, "Brown"), 
    html_colr(0xDEB887, "BurlyWood"), 
    html_colr(0x5F9EA0, "CadetBlue"), 
    html_colr(0x7FFF00, "Chartreuse"), 
    html_colr(0xD2691E, "Chocolate"), 
    html_colr(0xFF7F50, "Coral"), 
    html_colr(0x6495ED, "CornflowerBlue"), 
    html_colr(0xFFF8DC, "Cornsilk"), 
    html_colr(0xDC143C, "Crimson"), 
    html_colr(0x00FFFF, "Cyan"), 
    html_colr(0x00008B, "DarkBlue"), 
    html_colr(0x008B8B, "DarkCyan"), 
    html_colr(0xB8860B, "DarkGoldenRod"), 
    html_colr(0xA9A9A9, "DarkGray"), 
    html_colr(0xA9A9A9, "DarkGrey"), 
    html_colr(0x006400, "DarkGreen"), 
    html_colr(0xBDB76B, "DarkKhaki"), 
    html_colr(0x8B008B, "DarkMagenta"), 
    html_colr(0x556B2F, "DarkOliveGreen"), 
    html_colr(0xFF8C00, "DarkOrange"), 
    html_colr(0x9932CC, "DarkOrchid"), 
    html_colr(0x8B0000, "DarkRed"), 
    html_colr(0xE9967A, "DarkSalmon"), 
    html_colr(0x8FBC8F, "DarkSeaGreen"), 
    html_colr(0x483D8B, "DarkSlateBlue"), 
    html_colr(0x2F4F4F, "DarkSlateGray"), 
    html_colr(0x2F4F4F, "DarkSlateGrey"), 
    html_colr(0x00CED1, "DarkTurquoise"), 
    html_colr(0x9400D3, "DarkViolet"), 
    html_colr(0xFF1493, "DeepPink"), 
    html_colr(0x00BFFF, "DeepSkyBlue"), 
    html_colr(0x696969, "DimGray"), 
    html_colr(0x696969, "DimGrey"), 
    html_colr(0x1E90FF, "DodgerBlue"), 
    html_colr(0xB22222, "FireBrick"), 
    html_colr(0xFFFAF0, "FloralWhite"), 
    html_colr(0x228B22, "ForestGreen"), 
    html_colr(0xFF00FF, "Fuchsia"), 
    html_colr(0xDCDCDC, "Gainsboro"), 
    html_colr(0xF8F8FF, "GhostWhite"), 
    html_colr(0xFFD700, "Gold"), 
    html_colr(0xDAA520, "GoldenRod"), 
    html_colr(0x808080, "Gray"), 
    html_colr(0x808080, "Grey"), 
    html_colr(0x008000, "Green"), 
    html_colr(0xADFF2F, "GreenYellow"), 
    html_colr(0xF0FFF0, "HoneyDew"), 
    html_colr(0xFF69B4, "HotPink"), 
    html_colr(0xCD5C5C, "IndianRed"),
    html_colr(0x4B0082, "Indigo"),
    html_colr(0xFFFFF0, "Ivory"), 
    html_colr(0xF0E68C, "Khaki"), 
    html_colr(0xE6E6FA, "Lavender"), 
    html_colr(0xFFF0F5, "LavenderBlush"), 
    html_colr(0x7CFC00, "LawnGreen"), 
    html_colr(0xFFFACD, "LemonChiffon"), 
    html_colr(0xADD8E6, "LightBlue"), 
    html_colr(0xF08080, "LightCoral"), 
    html_colr(0xE0FFFF, "LightCyan"), 
    html_colr(0xFAFAD2, "LightGoldenRodYellow"), 
    html_colr(0xD3D3D3, "LightGray"), 
    html_colr(0xD3D3D3, "LightGrey"), 
    html_colr(0x90EE90, "LightGreen"), 
    html_colr(0xFFB6C1, "LightPink"), 
    html_colr(0xFFA07A, "LightSalmon"), 
    html_colr(0x20B2AA, "LightSeaGreen"), 
    html_colr(0x87CEFA, "LightSkyBlue"), 
    html_colr(0x778899, "LightSlateGray"), 
    html_colr(0x778899, "LightSlateGrey"), 
    html_colr(0xB0C4DE, "LightSteelBlue"), 
    html_colr(0xFFFFE0, "LightYellow"), 
    html_colr(0x00FF00, "Lime"), 
    html_colr(0x32CD32, "LimeGreen"), 
    html_colr(0xFAF0E6, "Linen"), 
    html_colr(0xFF00FF, "Magenta"), 
    html_colr(0x800000, "Maroon"), 
    html_colr(0x66CDAA, "MediumAquaMarine"), 
    html_colr(0x0000CD, "MediumBlue"), 
    html_colr(0xBA55D3, "MediumOrchid"), 
    html_colr(0x9370DB, "MediumPurple"), 
    html_colr(0x3CB371, "MediumSeaGreen"), 
    html_colr(0x7B68EE, "MediumSlateBlue"), 
    html_colr(0x00FA9A, "MediumSpringGreen"), 
    html_colr(0x48D1CC, "MediumTurquoise"), 
    html_colr(0xC71585, "MediumVioletRed"), 
    html_colr(0x191970, "MidnightBlue"), 
    html_colr(0xF5FFFA, "MintCream"), 
    html_colr(0xFFE4E1, "MistyRose"), 
    html_colr(0xFFE4B5, "Moccasin"), 
    html_colr(0xFFDEAD, "NavajoWhite"), 
    html_colr(0x000080, "Navy"), 
    html_colr(0xFDF5E6, "OldLace"), 
    html_colr(0x808000, "Olive"), 
    html_colr(0x6B8E23, "OliveDrab"), 
    html_colr(0xFFA500, "Orange"), 
    html_colr(0xFF4500, "OrangeRed"), 
    html_colr(0xDA70D6, "Orchid"), 
    html_colr(0xEEE8AA, "PaleGoldenRod"), 
    html_colr(0x98FB98, "PaleGreen"), 
    html_colr(0xAFEEEE, "PaleTurquoise"), 
    html_colr(0xDB7093, "PaleVioletRed"), 
    html_colr(0xFFEFD5, "PapayaWhip"), 
    html_colr(0xFFDAB9, "PeachPuff"), 
    html_colr(0xCD853F, "Peru"), 
    html_colr(0xFFC0CB, "Pink"), 
    html_colr(0xDDA0DD, "Plum"), 
    html_colr(0xB0E0E6, "PowderBlue"), 
    html_colr(0x800080, "Purple"), 
    html_colr(0x663399, "RebeccaPurple"), 
    html_colr(0xFF0000, "Red"), 
    html_colr(0xBC8F8F, "RosyBrown"), 
    html_colr(0x4169E1, "RoyalBlue"), 
    html_colr(0x8B4513, "SaddleBrown"), 
    html_colr(0xFA8072, "Salmon"), 
    html_colr(0xF4A460, "SandyBrown"), 
    html_colr(0x2E8B57, "SeaGreen"), 
    html_colr(0xFFF5EE, "SeaShell"), 
    html_colr(0xA0522D, "Sienna"), 
    html_colr(0xC0C0C0, "Silver"), 
    html_colr(0x87CEEB, "SkyBlue"), 
    html_colr(0x6A5ACD, "SlateBlue"), 
    html_colr(0x708090, "SlateGray"), 
    html_colr(0x708090, "SlateGrey"), 
    html_colr(0xFFFAFA, "Snow"), 
    html_colr(0x00FF7F, "SpringGreen"), 
    html_colr(0x4682B4, "SteelBlue"), 
    html_colr(0xD2B48C, "Tan"), 
    html_colr(0x008080, "Teal"), 
    html_colr(0xD8BFD8, "Thistle"), 
    html_colr(0xFF6347, "Tomato"), 
    html_colr(0x40E0D0, "Turquoise"), 
    html_colr(0xEE82EE, "Violet"), 
    html_colr(0xF5DEB3, "Wheat"), 
    html_colr(0xFFFFFF, "White"), 
    html_colr(0xF5F5F5, "WhiteSmoke"), 
    html_colr(0xFFFF00, "Yellow"), 
    html_colr(0x9ACD32, "YellowGreen")
};

struct HTML_COLOR_NAME_BSEARCH_KEY
{
    PC_UCHARS name;
    U32 name_length;
};

PROC_BSEARCH_PROTO(static, html_color_name_lookcomp, struct HTML_COLOR_NAME_BSEARCH_KEY *, struct HTML_COLOR_NAME)
{
    BSEARCH_KEY_VAR_DECL(struct HTML_COLOR_NAME_BSEARCH_KEY *, key);
    BSEARCH_DATUM_VAR_DECL(struct HTML_COLOR_NAME *, datum);
    int res;

    reportf("k:%d(%s) d:%d/%d(%s)", key->name_length, key->name, datum->name_length, strlen(datum->name), datum->name);

    /* NB no current_p_docu global register furtling required */

    /* compare leading substrings */
    if(0 != (res = C_strnicmp(key->name, datum->name, MIN(key->name_length, datum->name_length))))
        return(res);

    return((int) key->name_length - (int) datum->name_length);
}

static U32
get_colour_value_html_name(
    PC_USTR pos,
    _OutRef_    P_U32 p_r,
    _OutRef_    P_U32 p_g,
    _OutRef_    P_U32 p_b,
    _InVal_     uchar text_at_char)
{
    PC_USTR spos = pos;
    PC_USTR epos = strchr(pos, text_at_char);
    U32 name_length;
    U32 hex_value;
    U32 r, g, b;

    *p_r = *p_g = *p_b = 0;

    assert(NULL != epos);
    if(NULL == epos)
        return(0);

    name_length = epos - spos;

    {
    struct HTML_COLOR_NAME * p;
    struct HTML_COLOR_NAME_BSEARCH_KEY key;
    key.name = pos;
    key.name_length = name_length;
    p = bsearch(&key, html_color_name, elemof(html_color_name), sizeof(html_color_name[0]), html_color_name_lookcomp);
    if(NULL == p)
        return(0);
    hex_value = p->rgb;
    } /*block*/

    r = (hex_value >> 16) & 0xFF;
    g = (hex_value >>  8) & 0xFF;
    b = (hex_value >>  0) & 0xFF;

    *p_r = r;
    *p_g = g;
    *p_b = b;

    return(epos - spos);
}

static U32
get_colour_value_html_hex(
    PC_USTR pos,
    _OutRef_    P_U32 p_r,
    _OutRef_    P_U32 p_g,
    _OutRef_    P_U32 p_b)
{
    PC_USTR spos = pos;
    PC_USTR hex_pos = pos + 1; /* skip # */
    PC_USTR epos;
    U32 hex_value;
    U32 r, g, b;

    *p_r = *p_g = *p_b = 0;

    assert('#' == pos[0]);
    hex_value = (U32) strtoul(hex_pos, (char **) &epos, 16);
    switch(epos - hex_pos)
    {
        case 0:
        default:
            return(0);

        case 3: /* #rgb */
        case 6: /* #rrggbb */
            break;
    }

    r = (hex_value >> 16) & 0xFF;
    g = (hex_value >>  8) & 0xFF;
    b = (hex_value >>  0) & 0xFF;

    *p_r = r;
    *p_g = g;
    *p_b = b;

    return(epos - spos);
}

static U32
get_colour_value_html_rgb(
    PC_USTR pos,
    _OutRef_    P_U32 p_r,
    _OutRef_    P_U32 p_g,
    _OutRef_    P_U32 p_b)
{
    PC_USTR spos = pos;
    PC_USTR bra_pos = pos + 3; /* skip rgb */
    PC_USTR epos;
    unsigned int u_r, u_g, u_b;
    int i_chars;

    *p_r = *p_g = *p_b = 0;

    assert(('r' == pos[0]) && ('g' == pos[1]) && ('b' == pos[2]));
    if(3 != sscanf(bra_pos, "(%u,%u,%u)%n", &u_r, &u_g, &u_b, &i_chars))
        return(0);
    epos = bra_pos + i_chars;

    *p_r = u_r;
    *p_g = u_g;
    *p_b = u_b;

    return(epos - spos);
}

extern U32
get_colour_value(
    PC_USTR pos,
    _OutRef_    P_U32 p_r,
    _OutRef_    P_U32 p_g,
    _OutRef_    P_U32 p_b,
    _InVal_     uchar text_at_char)
{
    PC_USTR spos = pos;
    PC_USTR epos;
    U32 r, g, b;

    if('#' == pos[0])
        return(get_colour_value_html_hex(pos, p_r, p_g, p_b));

    if(('r' == pos[0]) && ('g' == pos[1]) && ('b' == pos[2]))
        return(get_colour_value_html_rgb(pos, p_r, p_g, p_b));

    if(isalpha(pos[0]))
        return(get_colour_value_html_name(pos, p_r, p_g, p_b, text_at_char));

    *p_r = *p_g = *p_b = 0;

    r = fast_ustrtoul(pos, &epos);
    if(pos == epos) return(0);
    pos = epos;

    if(*pos++ != ',') return(0);

    g = fast_ustrtoul(pos, &epos);
    if(pos == epos)
        return(0);
    pos = epos;

    if(*pos++ != ',') return(0);

    b = fast_ustrtoul(pos, &epos);
    if(pos == epos)
        return(0);

    *p_r = r;
    *p_g = g;
    *p_b = b;

    return(epos - spos);
}

/* end of colour.c */
