/* drawxtra.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* RISC OS Draw file / object structure definition */

#ifndef __drawxtra_h
#define __drawxtra_h

/*
structure
*/

/*
Draw coordinates are large signed things
*/

typedef S32 DRAW_COORD; typedef DRAW_COORD * P_DRAW_COORD; typedef const DRAW_COORD * PC_DRAW_COORD;

#define DRAW_COORD_MAX S32_MAX

/*
points, or simply pairs of coordinates
*/

typedef struct DRAW_POINT
{
    DRAW_COORD x, y;
}
DRAW_POINT, * P_DRAW_POINT; typedef const DRAW_POINT * PC_DRAW_POINT;

typedef struct DRAW_SIZE
{
    DRAW_COORD cx, cy;
}
DRAW_SIZE, * P_DRAW_SIZE; typedef const DRAW_SIZE * PC_DRAW_SIZE;

/*
boxes, or simply pairs of points
*/

typedef struct DRAW_BOX
{
    DRAW_COORD x0, y0, x1, y1;
}
DRAW_BOX, * P_DRAW_BOX; typedef const DRAW_BOX * PC_DRAW_BOX;

/*
matrix transforms
*/

typedef struct DRAW_TRANSFORM
{
    S32 a, b, c, d;
    S32 e, f;
}
DRAW_TRANSFORM;

/*
Draw file header
*/

typedef struct DRAW_FILE_HEADER
{
    U8 title[4];                    /* 1 word   */ /* 'Draw' */
    S32 major_stamp;                /* 1 word   */
    S32 minor_stamp;                /* 1 word   */
    U8 creator_id[12];              /* 3 words  */ /* NB filled with spaces, NOT NULLCH terminated */
    DRAW_BOX bbox;                  /* 4 words  */
}                                   /* 10 words */
DRAW_FILE_HEADER, * P_DRAW_FILE_HEADER; typedef const DRAW_FILE_HEADER * PC_DRAW_FILE_HEADER;

/* offset in an Draw diagram/group/object */
typedef U32 DRAW_DIAG_OFFSET; typedef DRAW_DIAG_OFFSET * P_DRAW_DIAG_OFFSET; typedef const DRAW_DIAG_OFFSET * PC_DRAW_DIAG_OFFSET;

#define DRAW_DIAG_OFFSET_NONE  ((DRAW_DIAG_OFFSET) 0U)
#define DRAW_DIAG_OFFSET_FIRST ((DRAW_DIAG_OFFSET) 1U)
#define DRAW_DIAG_OFFSET_LAST  ((DRAW_DIAG_OFFSET) 0xFFFFFFFFU)

/*
Draw object header (sans bounding box)
*/

#define DRAW_OBJECT_HEADER_NO_BBOX_ /* 2 words  */ \
    U32 type;                       /* 1 word   */ \
    U32 size                        /* 1 word   */

typedef struct DRAW_OBJECT_HEADER_NO_BBOX
{
    DRAW_OBJECT_HEADER_NO_BBOX_;    /* 2 words  */
}                                   /* 2 words  */
DRAW_OBJECT_HEADER_NO_BBOX, * P_DRAW_OBJECT_HEADER_NO_BBOX;

/*
Draw object header (with bounding box)
*/

#define DRAW_OBJECT_HEADER_         /* 6 words  */ \
    U32 type;                       /* 1 word   */ \
    U32 size;                       /* 1 word   */ \
    DRAW_BOX bbox                   /* 4 words  */

typedef struct DRAW_OBJECT_HEADER
{
    DRAW_OBJECT_HEADER_;            /* 6 words  */
}                                   /* 6 words  */
DRAW_OBJECT_HEADER, * P_DRAW_OBJECT_HEADER, ** P_P_DRAW_OBJECT_HEADER; typedef const DRAW_OBJECT_HEADER * PC_DRAW_OBJECT_HEADER;

/*
Draw colours are BGR0, except for transparent
*/

typedef U32 DRAW_COLOUR;

#define DRAW_COLOUR_Transparent ((DRAW_COLOUR) 0xFFFFFFFFU)

/*
A (RISC OS) font list
*/

/* NB Just a byte on RISC OS, but we need to cater for Windows font tables with >= 256 entries */
typedef U16 DRAW_FONT_REF16;        /* 2 bytes  */

typedef struct DRAW_OBJECT_FONTLIST
{
#define DRAW_OBJECT_TYPE_FONTLIST 0
    DRAW_OBJECT_HEADER_NO_BBOX_;
}
DRAW_OBJECT_FONTLIST, * P_DRAW_OBJECT_FONTLIST;

/* followed by a number of DRAW_FONTLIST_ELEM */

typedef struct DRAW_FONTLIST_ELEM
{
    U8 fontref8;                    /* 1 byte   */
    U8Z szHostFontName[31];         /* >= 2 bytes */ /* Latin-1 string, NULLCH terminated (size only for compiler and debugger - do not use sizeof()) */
}
DRAW_FONTLIST_ELEM, * P_DRAW_FONTLIST_ELEM; typedef const DRAW_FONTLIST_ELEM * PC_DRAW_FONTLIST_ELEM;

/*
A text object
*/

typedef struct DRAW_TEXT_STYLE
{
    DRAW_FONT_REF16 fontref16;      /* 2 bytes  */
    U16 reserved16;                 /* 2 bytes  */
}                                   /* 1 word   */
DRAW_TEXT_STYLE;

/* Draw font sizes are 1/640 point */

#define draw_fontsize_from_mp(mp) ( \
    ((mp) * 64) / 100 )

typedef struct DRAW_OBJECT_TEXT
{
#define DRAW_OBJECT_TYPE_TEXT 1
    DRAW_OBJECT_HEADER_;            /* 6 words  */

    DRAW_COLOUR textcolour;         /* 1 word   */
    DRAW_COLOUR background;         /* 1 word   */
    DRAW_TEXT_STYLE textstyle;      /* 1 word   */
    U32 fsize_x;                    /* 1 word, unsigned */
    U32 fsize_y;                    /* 1 word, unsigned */
    DRAW_POINT coord;               /* 2 words  */
}                                   /* 13 words */
DRAW_OBJECT_TEXT;

/* followed by text bytes */

/*
A path object
*/

typedef struct DRAW_PATH_STYLE
{
    U8 flags;                       /* 1 byte   */  /* bit 0..1 join         */
                                                    /* bit 2..3 end cap      */
                                                    /* bit 4..5 start cap    */
                                                    /* bit 6    winding rule */
                                                    /* bit 7    dashed       */

/* Values used for pack/unpack of Draw path style data */
#define DRAW_PS_JOIN_PACK_MASK           0x03
#define DRAW_PS_JOIN_PACK_SHIFT             0
#define DRAW_PS_JOIN_MITRED         ((U8)0x00) /* mitred joins */
#define DRAW_PS_JOIN_ROUND          ((U8)0x01) /* round joins */
#define DRAW_PS_JOIN_BEVELLED       ((U8)0x02) /* bevelled joins */

#define DRAW_PS_ENDCAP_PACK_MASK         0x0C
#define DRAW_PS_ENDCAP_PACK_SHIFT           2
#define DRAW_PS_CAP_BUTT            ((U8)0x00) /* butt caps */
#define DRAW_PS_CAP_ROUND           ((U8)0x01) /* round caps */
#define DRAW_PS_CAP_SQUARE          ((U8)0x02) /* projecting square caps */
#define DRAW_PS_CAP_TRIANGLE        ((U8)0x03) /* triangular caps */

#define DRAW_PS_STARTCAP_PACK_MASK       0x30
#define DRAW_PS_STARTCAP_PACK_SHIFT         4

#define DRAW_PS_WINDRULE_PACK_MASK       0x40
#define DRAW_PS_WINDRULE_PACK_SHIFT         6
#define DRAW_PS_WINDRULE_NONZERO            0
#define DRAW_PS_WINDRULE_EVENODD            1

#define DRAW_PS_DASH_PACK_MASK           0x80
#define DRAW_PS_DASH_PACK_SHIFT             7
#define DRAW_PS_DASH_ABSENT                 0
#define DRAW_PS_DASH_PRESENT                1

    U8 reserved;                    /* 1 byte   */

    U8 tricap_w;                    /* 1 byte   */  /* 1/16th of line width */
    U8 tricap_h;                    /* 1 byte   */  /* 1/16th of line width */
}                                   /* 1 word   */
DRAW_PATH_STYLE;

typedef struct DRAW_OBJECT_PATH
{
#define DRAW_OBJECT_TYPE_PATH 2
    DRAW_OBJECT_HEADER_;            /* 6 words  */

    DRAW_COLOUR fillcolour;         /* 1 word   */
    DRAW_COLOUR pathcolour;         /* 1 word   */
    S32 pathwidth;                  /* 1 word   */
    DRAW_PATH_STYLE pathstyle;      /* 1 word   */
}                                   /* 10 words */
DRAW_OBJECT_PATH, * P_DRAW_OBJECT_PATH;

/* followed by optional dash header + elements and then path elements */

typedef struct DRAW_DASH_HEADER
{
    U32 dashstart;                  /* 1 word   */  /* distance into pattern */
    U32 dashcount;                  /* 1 word   */  /* number of elements    */
}
DRAW_DASH_HEADER, * P_DRAW_DASH_HEADER; typedef const DRAW_DASH_HEADER * PC_DRAW_DASH_HEADER;

/* followed by dashcount dash elements (each is S32) */

/*
Elements within a path
*/

typedef struct DRAW_PATH_MOVE
{
#define DRAW_PATH_TYPE_MOVE     2 /* move to (x,y), starts new subpath */
    U32 tag;
    DRAW_POINT pt;
}
DRAW_PATH_MOVE;

typedef struct DRAW_PATH_LINE
{
#define DRAW_PATH_TYPE_LINE     8 /* line to (x,y) */
    U32 tag;
    DRAW_POINT pt;
}
DRAW_PATH_LINE;

typedef struct DRAW_PATH_CURVE
{
#define DRAW_PATH_TYPE_CURVE    6 /* bezier curve to (x3,y3) with 2 control points */
    U32 tag;
    DRAW_POINT cp1;
    DRAW_POINT cp2;
    DRAW_POINT end;
}
DRAW_PATH_CURVE;

typedef struct DRAW_PATH_CLOSE
{
#define DRAW_PATH_TYPE_CLOSE    5 /* close current subpath with a line */
    U32 tag;
}
DRAW_PATH_CLOSE;

typedef struct DRAW_PATH_TERM
{
#define DRAW_PATH_TYPE_TERM     0 /* end of path */
    U32 tag;
}
DRAW_PATH_TERM;

/*
A RISC OS sprite
*/

typedef struct DRAW_OBJECT_SPRITE
{
#define DRAW_OBJECT_TYPE_SPRITE 5
    DRAW_OBJECT_HEADER_;            /* 6 words  */

    struct DRAW_OBJECT_SPRITE_SPRITE /* same as sprite_header */
    {
        S32 next;       /*  Offset to next sprite                */
        char name[12];  /*  Sprite name                          */
        S32 width;      /*  Width in words-1      (0..639)       */
        S32 height;     /*  Height in scanlines-1 (0..255/511)   */
        S32 lbit;       /*  First bit used (left end of row)     */
        S32 rbit;       /*  Last bit used (right end of row)     */
        S32 image;      /*  Offset to sprite image               */
        S32 mask;       /*  Offset to transparency mask          */
        S32 mode;       /*  Mode sprite was defined in           */
    }                               /* 11 words */
    sprite;
}                                   /* 17 words */
DRAW_OBJECT_SPRITE, * P_DRAW_OBJECT_SPRITE;

/*
A group of objects
*/

typedef struct DRAW_OBJECT_GROUP
{
#define DRAW_OBJECT_TYPE_GROUP 6
    DRAW_OBJECT_HEADER_;            /* 6 words  */

    U8 name[12];                    /* 3 words  */
}                                   /* 9 words  */
DRAW_OBJECT_GROUP;

/*
A tagged object
*/

typedef struct DRAW_OBJECT_TAG
{
#define DRAW_OBJECT_TYPE_TAG 7
    DRAW_OBJECT_HEADER_;            /* 6 words  */

    U32 intTag;                     /* 1 word   */

    /* the tagged object follows */

    /* then size - tagged object->size bytes of 'goop' */
}
DRAW_OBJECT_TAG;

#define DRAW_OBJECT_TYPE_OPTIONS    11 /*RO3*/

#define DRAW_OBJECT_TYPE_TRFMTEXT   12 /*RO3*/

#define DRAW_OBJECT_TYPE_TRFMSPRITE 13 /*RO3*/

/*
A JPEG object
*/

typedef struct DRAW_OBJECT_JPEG
{
#define DRAW_OBJECT_TYPE_JPEG 16 /*RO3*/
    DRAW_OBJECT_HEADER_;            /* 6 words  */

    S32             width;          /* 1 word   */
    S32             height;         /* 1 word   */
    S32             dpi_x;          /* 1 word   */
    S32             dpi_y;          /* 1 word   */
    DRAW_TRANSFORM  trfm;           /* 6 words  */
    S32             len;            /* 1 word   */
}                                   /* 17 words */
DRAW_OBJECT_JPEG, * P_DRAW_OBJECT_JPEG;

/*
Dial Solutions extensions
*/

#define DRAW_OBJECT_TYPE_DS_WINFONTLIST 0x310

/* this could have been cast to a LOGFONT on 16-bit Windows */
typedef struct DRAW_WINDOWS_LOGFONT
{
    S16 lfHeight;
    S16 lfWidth;
    S16 lfEscapement;
    S16 lfOrientation;
    S16 lfWeight;
    BYTE lfItalic;
    BYTE lfUnderline;
    BYTE lfStrikeOut;
    BYTE lfCharSet;
    BYTE lfOutPrecision;
    BYTE lfClipPrecision;
    BYTE lfQuality;
    BYTE lfPitchAndFamily;
    U8Z lfFaceName[32];
}
DRAW_WINDOWS_LOGFONT;

typedef struct DRAW_WIN_FONTLIST_ELEM
{
    DRAW_FONT_REF16 fontref16;
    DRAW_WINDOWS_LOGFONT logfont;
}
DRAW_WIN_FONTLIST_ELEM, * P_DRAW_WIN_FONTLIST_ELEM;

/*
A Windows BMP object (DIB)
*/

#define DRAW_OBJECT_TYPE_DS_DIB         0x311
#define DRAW_OBJECT_TYPE_DS_DIBROT      0x312

#if RISCOS
#define sizeof_BITMAPFILEHEADER 14

/* Should be...

typedef struct tagBITMAPFILEHEADER
{
   WORD  bfType;
   DWORD bfSize;
   WORD  bfreserved1;
   WORD  bfreserved2;
   DWORD bfOffBits;
}
BITMAPFILEHEADER, * P_BITMAPFILEHEADER;
*/

typedef struct tagRGBQUAD
{
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
}
RGBQUAD;

typedef struct tagBITMAPINFOHEADER
{
    U32 biSize;
    U32 biWidth;
    U32 biHeight;
    U16 biPlanes;
    U16 biBitCount; /* TUTU says it'll pack */
    U32 biCompression;
    U32 biSizeImage;
    U32 biXPelsPerMeter;
    U32 biYPelsPerMeter;
    U32 biClrUsed;
    U32 biClrImportant;
}
BITMAPINFOHEADER;

typedef struct tagBITMAPINFO
{
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[1]; /* depends on bpp (maybe null) */
}
BITMAPINFO;

#else
#define sizeof_BITMAPFILEHEADER sizeof(BITMAPFILEHEADER)
#endif

typedef BITMAPINFO * P_BITMAPINFO;

/* DRAW_OBJECT_HEADER followed by BITMAPINFO and bitmap */

#ifdef __kernel_h
#elif RISCOS
#include "kernel.h" /* C: */
#define far
#endif

/* same structure (but different member names) as sprite_area from RISC_OSLib:sprite.h */
typedef struct SAH /* ie NOT a spritefileheader */
{
    S32 area_size; /* this word omitted from Sprite (FF9) files */
    S32 number_of_sprites;
    S32 offset_to_first;
    S32 offset_to_free;
}
SAH, * P_SAH;

typedef struct SCB
{
    S32 offset_to_next;
    char name[12];
    S32 lwidth;
    S32 lheight;
    S32 fbused;
    S32 lbused;
    S32 offset_to_data;
    S32 offset_to_mask;
    S32 mode;
}
SCB, * P_SCB; typedef const SCB * PC_SCB;

typedef struct SPRITE_TYPE
{
    unsigned int sprite_type_bit : 1;
    unsigned int h_dpi           : 13;
    unsigned int v_dpi           : 13;
    unsigned int type            : 5;
}
SPRITE_TYPE;

#define SPRITE_TYPE_OLD   0
#define SPRITE_TYPE_1BPP  1
#define SPRITE_TYPE_2BPP  2
#define SPRITE_TYPE_4BPP  3
#define SPRITE_TYPE_8BPP  4
#define SPRITE_TYPE_16BPP 5
#define SPRITE_TYPE_32BPP 6

#if WINDOWS

#include "cmodules/coltsoft/oak_draw.h"

typedef enum __drawmod_filltype
{
    fill_FBext          = 0x00000008,  /* plot boundary exterior pixels */
    fill_FNonbint       = 0x00000010,  /* plot boundary interior pixels */
    fill_FBint          = 0x00000020,  /* plot non-boundary interior pixels */

    fill_PClose         = 0x08000000,  /* close open subpaths */
    fill_PFlatten       = 0x10000000,  /* flatten the path */
    fill_PThicken       = 0x20000000,  /* thicken the path */
    fill_PReflatten     = 0x40000000   /* re-flatten the path */
}
_drawmod_filltype;

#endif /* OS */

#endif /* __drawxtra_h */

/* end of drawxtra.h */
