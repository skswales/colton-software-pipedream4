/* coltsoft.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Definition of standard types and source rules */

/* MRJC September 1989 / December 1991 */

#ifndef __coltsoft_h
#define __coltsoft_h

#if RISCOS

/*
Stuff defined on Windows
*/

/*
"crtdefs.h"
*/

#define errno_t int

#ifndef EINVAL
#define EINVAL -1
#endif

/*
"windef.h"
*/

typedef /*unsigned*/ char  BYTE; /* NB char IS unsigned on Norcroft - stop winges about pointer type diffs */
typedef unsigned short WORD;
typedef unsigned long  DWORD;

#define BOOL int /* or RISC_OSLib:os.h winges ... */ /*typedef int BOOL;*/
typedef unsigned int UINT;

/*
"winnt.h"
*/

typedef void * HANDLE;
typedef void * HGLOBAL;

#define __TEXT(s) s /* No wide character constants */
#define TEXT(s) __TEXT(s)

/*
"tchar.h"
*/

#define     TCHAR       U8
#define    PTCH       P_U8
#define   PCTCH      PC_U8
#define P_PCTCH    P_PC_U8

/* NULLCH-terminated variants */

#define     TCHARZ      U8Z
#define    PTSTR      P_U8Z
#define  P_PTSTR    P_P_U8Z
#define   PCTSTR     PC_U8Z
#define P_PCTSTR   P_PC_U8Z

#endif /* RISCOS */

#define PTSTR_NONE _P_DATA_NONE(PTSTR)

#define tstr_empty_string TEXT("")

/*
Never use a native C type unless dealing with a native API
Where you might have used an int, use an S32

Never use a '*' in a variable definition/declaration;
'*' is OK in two places: typedefs and pointer dereferences

All types are upper case

Structures are defined thus:

    typedef struct _FRED
    {
    }
    FRED;

and the 'struct' word should never appear outside a structure definition

Pointer types are derived by prefixing P_ to base type:

    typedef FRED * P_FRED;

Where possible, instances of a type should be lower case versions of the type name:

    P_FRED p_fred;

When you define a constant for the size of a buffer in bytes
eg STRING_MAX, always define two constants, the second being
larger by 1 and prefixed with BUF_:

#define STRING_MAX 10
#define BUF_STRING_MAX 11

Where the parameters of a function include a source and a destination,
the destination should appear first in the parameter list,
as it does for example safe_strkpy(dst, elemof_dst, src)

When you have start and end pointers / indexes they must be
inclusive, exclusive whether they point to memory, slots, arrays or anything
*/

/*
Our additions to SAL
*/

/*
Most non-trivial pointer args here (i.e. P_XXX or PC_XXX) are really references
to a single instance of an 'object' so annotate as such -
the callee function shouldn't be modifying the pointer,
just referencing/setting/updating the contents referred to
*/

#define _InRef_         _In_        const
#define _InRef_opt_     _In_opt_    const

#define _InoutRef_      _Inout_     const
#define _InoutRef_opt_  _Inout_opt_ const

#define _OutRef_        _Out_       const
#define _OutRef_opt_    _Out_opt_   const

/*
Simple, or derived, type value args that the callee function shouldn't be modifying
*/

#define _InVal_         /*_In_*/    const

/*
void
*/

typedef void * P_ANY, ** P_P_ANY; typedef const void * PC_ANY;
#if defined(__cplusplus) || defined(__CC_NORCROFT) || defined(__GNUC__)
#define P_P_ANY_PEDANTIC(pp) ((P_P_ANY) (pp)) /* needs cast */
#else
#define P_P_ANY_PEDANTIC(pp) (pp)
#endif

#define    P_DATA_NONE       _P_DATA_NONE(P_ANY)
#define IS_P_DATA_NONE(p) _IS_P_DATA_NONE(PC_ANY, p)

/*
BYTE
*/

typedef BYTE * P_BYTE, ** P_P_BYTE; typedef const BYTE * PC_BYTE;

#define P_BYTE_NONE _P_DATA_NONE(P_BYTE)

/*
U8 char
*/

#ifdef _CHAR_UNSIGNED
typedef          char    U8;
#else
#error _CHAR_UNSIGNED is not defined
typedef unsigned char    U8;
#endif
typedef U8 * P_U8, ** P_P_U8; typedef const U8 * PC_U8; typedef PC_U8 * P_PC_U8;

#define P_U8_NONE _P_DATA_NONE(P_U8)

/* Z suffix signifying these to be known to be NULLCH-terminated */

#if 1 /* Distinct (but convertible) where possible for usage clarity and IntelliSense hover */
typedef      U8      U8Z;
typedef    P_U8    P_U8Z; /*_NullTerminated_*/
typedef  P_P_U8  P_P_U8Z;
typedef   PC_U8   PC_U8Z; /*_NullTerminated_*/
typedef P_PC_U8 P_PC_U8Z;
#else
#define      U8Z      U8
#define    P_U8Z    P_U8 /*_NullTerminated_*/
#define  P_P_U8Z  P_P_U8
#define   PC_U8Z   PC_U8 /*_NullTerminated_*/
#define P_PC_U8Z P_PC_U8
#endif

#define P_U8Z_NONE _P_DATA_NONE(P_U8Z)

#define empty_string ""

#define U8_is_ascii7(u8) ( \
    (u8) < 0x80)

typedef signed char S8; typedef S8 * P_S8, ** P_P_S8; typedef const S8 * PC_S8;

typedef unsigned short U16; typedef U16 * P_U16, ** P_P_U16; typedef const U16 * PC_U16;

typedef signed short S16; typedef S16 * P_S16, ** P_P_S16; typedef const S16 * PC_S16;

typedef unsigned int U32; typedef U32 * P_U32, ** P_P_U32; typedef const U32 * PC_U32;

typedef /*signed*/ int S32; typedef S32 * P_S32, ** P_P_S32; typedef const S32 * PC_S32;

#define P_S32_NONE _P_DATA_NONE(P_S32)

typedef uint64_t U64; typedef U64 * P_U64, ** P_P_U64; typedef const U64 * PC_U64;

typedef double F64; typedef F64 * P_F64, ** P_P_F64; typedef const F64 * PC_F64;

typedef BOOL * P_BOOL; typedef const BOOL * PC_BOOL;

typedef uintptr_t CLIENT_HANDLE; typedef CLIENT_HANDLE * P_CLIENT_HANDLE;

#if !WINDOWS
typedef U16 WCHAR; typedef U16 * PWCH; typedef const U16 * PCWCH;

/* NULLCH-terminated character strings */
typedef U16 * PWSTR; typedef const U16 * PCWSTR;
#endif

/*
a UCS-4 32-bit character (ISO 10646-1)
*/
typedef          U32      UCS4;
typedef         UCS4 *  P_UCS4;
typedef   const UCS4 * PC_UCS4;

_Check_return_
static inline BOOL
ucs4_is_ascii7(
    _InVal_     UCS4 ucs4)
{
    return((ucs4) < 0x00000080U);
}

_Check_return_
static inline BOOL
ucs4_is_latin1(
    _InVal_     UCS4 ucs4)
{
    return((ucs4) < 0x00000100U);
}

/*
an ASCII 7-bit character (subset of Latin-1 where top bit is zero)
Useful for things that we know are limited e.g. spreadsheet TYPE()
and key names that go in files for mapping
*/

#if 1
typedef      U8       A7_U8;

typedef      U8Z      A7_U8Z;
typedef    P_U8Z    P_A7STR; /*_NullTerminated_*/
typedef  P_P_U8Z  P_P_A7STR;
typedef   PC_U8Z   PC_A7STR; /*_NullTerminated_*/
typedef P_PC_U8Z P_PC_A7STR;
#else
#define      A7_U8        U8

#define      A7_U8Z       U8Z
#define    P_A7STR      P_U8Z /*_NullTerminated_*/
#define  P_P_A7STR    P_P_U8Z
#define   PC_A7STR     PC_U8Z /*_NullTerminated_*/
#define P_PC_A7STR   P_PC_U8Z
#endif

/*
a Latin-1 8-bit character (ISO 8859-1) (may have top-bit-set)
*/

#if 1
typedef      U8       L1_U8;
typedef    P_U8     P_L1CHARS;
typedef  P_P_U8   P_P_L1CHARS;
typedef   PC_U8    PC_L1CHARS;
typedef P_PC_U8  P_PC_L1CHARS;

typedef      U8Z      L1_U8Z;
typedef    P_U8Z    P_L1STR; /*_NullTerminated_*/
typedef  P_P_U8Z  P_P_L1STR;
typedef   PC_U8Z   PC_L1STR; /*_NullTerminated_*/
typedef P_PC_U8Z P_PC_L1STR;
#else
#define      L1_U8        U8
#define    P_L1CHARS    P_U8
#define  P_P_L1CHARS  P_P_U8
#define   PC_L1CHARS   PC_U8
#define P_PC_L1CHARS P_PC_U8

#define      L1_U8Z       U8Z
#define    P_L1STR      P_U8Z /*_NullTerminated_*/
#define  P_P_L1STR    P_P_U8Z
#define   PC_L1STR     PC_U8Z /*_NullTerminated_*/
#define P_PC_L1STR   P_PC_U8Z
#endif

#define L1STR_TEXT(text) ((PC_L1STR) (text)) /* akin to the TEXT() macro */

/*
RISC OS non-UNICODE
TCHAR is Latin-1
UCHARZ is Latin-1
*/

#define TSTR_IS_L1STR 1

#define USTR_IS_L1STR 1

#if 1
#define    UCHARB    U8

#define  P_UCHARS  P_U8
#define PC_UCHARS PC_U8

#define    UCHARZ      U8Z
#define    P_USTR    P_U8Z /*_NullTerminated_*/
#define  P_P_USTR  P_P_U8Z
#define   PC_USTR   PC_U8Z /*_NullTerminated_*/
#define P_PC_USTR P_PC_U8Z
#endif

/* using inline function can spot any bad buffer-to-pointer casts that would otherwise happen silently */
static inline P_USTR
ustr_bptr(UCHARZ buf[])
{
    return((P_USTR) buf);
}

#define P_USTR_NONE _P_DATA_NONE(P_USTR)

#define USTR_TEXT(text) ((PC_USTR) (text)) /* akin to the TEXT() macro */

#define ustr_empty_string USTR_TEXT("")

/*
printf format strings
*/

#define  U8_FMT         "%c"
#define  S8_FMT         "%c"
#define S16_FMT        "%hd"
#define U16_FMT        "%hu"
#define U16_XFMT       "%hX"
#define S32_FMT         "%d"
#define S32_FMT_POSTFIX  "d"
#define U32_FMT         "%u"
#define U32_XFMT        "%X"
#define F64_FMT         "%g"
#define PTR_FMT         "%p"

#define  U8_TFMT         TEXT("%c")
#define  S8_TFMT         TEXT("%c")
#define S16_TFMT        TEXT("%hd")
#define U16_TFMT        TEXT("%hu")
#define U16_XTFMT       TEXT("%hX")
#define INT_TFMT         TEXT("%d")
#define UINT_TFMT        TEXT("%u")
#define S32_TFMT         TEXT("%d")
#define S32_TFMT_POSTFIX  TEXT("d")
#define U32_TFMT         TEXT("%u")
#define U32_XTFMT      TEXT("0x%X")
#define F64_TFMT         TEXT("%g")
#define PTR_TFMT         TEXT("%p")
#define PTR_XTFMT      TEXT("0x%p")

#define ENUM_XTFMT      U32_XTFMT

#if defined(_WIN64)
#define UINT3264_TFMT   TEXT("%I64u") /* NB %llu doesn't work on Windows 2000 runtime */
#define UINT3264_XTFMT  TEXT("0x%I64x")
#else
#define UINT3264_TFMT   U32_TFMT
#define UINT3264_XTFMT  U32_XTFMT
#endif /* _WIN64 */

#define UINTPTR_TFMT    UINT3264_TFMT
#define UINTPTR_XTFMT   UINT3264_XTFMT

#define  F64_SCANF_FMT "%lg"

/*
type limits
*/

#define  U8_MAX   UCHAR_MAX
#define  S8_MAX   SCHAR_MAX
#define U16_MAX   USHRT_MAX
#define S16_MAX    SHRT_MAX
#define U32_MAX    UINT_MAX
#define S32_MAX     INT_MAX

#define  S8_MIN   SCHAR_MIN
#define S16_MIN    SHRT_MIN
#define S32_MIN     INT_MIN /* but you might not want to use this! */
#define F64_MIN     DBL_MIN

/*
buffer sizes for printf conversions
*/

#define BUF_MAX_U8_FMT  (1+ 3)  /* 255*/
#define BUF_MAX_S8_FMT  (1+ 4)  /*-128*/
#define BUF_MAX_S16_FMT (1+ 6)  /*-32768*/
#define BUF_MAX_U16_FMT (1+ 5)  /* 65535*/
#define BUF_MAX_S32_FMT (1+ 11) /*-2147483648*/
#define BUF_MAX_U32_FMT (1+ 10) /* 4294967296*/
#define BUF_MAX_F64_FMT (1+ 1 + 1 + 1 + 15 + 1 + 3 + 4) /*approx eg -1.2345678901234566e-128, 4 for good luck */

/* get byte from pointer (plus offset) / put byte at pointer (plus offset) */

#define PtrGetByte(ptr) /*byte*/ \
    PtrGetByteOff(ptr, 0)

#define PtrGetByteOff(ptr, off) /*byte*/ \
    (((PC_BYTE) (ptr))[off])

#define PtrPutByte(ptr, b) /*void*/ \
    PtrPutByteOff(ptr, 0, b)

#define PtrPutByteOff(ptr, off, b) /*void*/ \
    (((P_BYTE) (ptr))[off]) = (b)

/* increment / decrement pointer by one bytes (or N bytes) */

#define PtrDecByte(__ptr_type, ptr__ref) \
    PtrDecBytes(__ptr_type, ptr__ref, 1)

#define PtrDecBytes(__ptr_type, ptr__ref, sub) \
    (ptr__ref) = ((__ptr_type) (((uintptr_t)(ptr__ref)) - (sub)))

#define PtrIncByte(__ptr_type, ptr__ref) \
    PtrIncBytes(__ptr_type, ptr__ref, 1)

#define PtrIncBytes(__ptr_type, ptr__ref, add) \
    (ptr__ref) = ((__ptr_type) (((uintptr_t)(ptr__ref)) + (add)))

/* add bytes to pointer / subtract bytes from pointer */

#define PtrAddBytes(__ptr_type, ptr, add) /*ptr*/ \
    ((__ptr_type) (((uintptr_t)(ptr)) + (add)))

#define PtrSubBytes(__ptr_type, ptr, sub) /*ptr*/ \
    ((__ptr_type) (((uintptr_t)(ptr)) - (sub)))

/* 32-bit difference between two pointers in bytes */

#define PtrDiffBytesU32(ptr, base) /*num*/ \
    ((U32) (((uintptr_t)(ptr)) - ((uintptr_t)(base))))

#define PtrDiffBytesS32(ptr, base) /*num*/ \
    ((S32) (((uintptr_t)(ptr)) - ((uintptr_t)(base))))

/* 32-bit difference between two pointers in elements */

#define PtrDiffElemU32(ptr, base) /*num*/ \
    ((U32) ((ptr) - (base)))

#define PtrDiffElemS32(ptr, base) /*num*/ \
    ((S32) ((ptr) - (base)))

/* size_t difference between two pointers in elements */

#define PtrDiffElem(ptr, base) /*num*/ \
    ((size_t) ((ptr) - (base)))

/* skip spaces on byte-oriented string (either U8 or UTF-8) */

#define PtrSkipSpaces(__ptr_type, ptr) /*void*/ \
    while(CH_SPACE == PtrGetByte(ptr)) \
        PtrIncByte(__ptr_type, ptr)

/*
remove const from pointer
*/

#ifdef __cplusplus
#define de_const_cast(__type, __expr) (const_cast < __type > ( __expr ))
#else
#define de_const_cast(__type, __expr) (( /*de-const*/ __type ) ( __expr ))
#endif

/*
bit field qualifiers
*/

#define UBF unsigned int
#define SBF   signed int

#define UBF_PACK(value) \
    ((UBF) (value))

#define UBF_UNPACK(__unpacked_type, packed_value) \
    ((__unpacked_type) (packed_value))

/*
pointers to procedures are derived by adding P_PROC_
*/

/*
atexit()
*/

typedef void (__cdecl * P_PROC_ATEXIT) (void);

#define PROC_ATEXIT_PROTO(_e_s, _proc_atexit) \
_e_s void __cdecl _proc_atexit(void)

/*
bsearch() / bfind()
*/

typedef /*_Check_return_*/ int (__cdecl * P_PROC_BSEARCH) (
    _Pre_valid_ const void * key,
    _Pre_valid_ const void * datum);

#define PROC_BSEARCH_PROTO(_e_s, _proc_bsearch, __key_base_type, __datum_base_type) \
_Check_return_ \
_e_s int __cdecl _proc_bsearch( \
    _In_bytecount_c_(sizeof(__key_base_type))   const void * key, \
    _In_bytecount_c_(sizeof(__datum_base_type)) const void * datum)

/* for NULLCH-terminated string lookup */
/* Removing z_ avoids the C6510 'Invalid annotation: 'NullTerminated'' warning */
#define PROC_BSEARCH_PROTO_Z(_e_s, _proc_bsearch, __key_base_type, __datum_base_type) \
_Check_return_ \
_e_s int __cdecl _proc_bsearch( \
    _Pre_valid_ /*_In_z_*/ /*__key_base_type unused*/ const void *key_in, \
    _In_bytecount_c_(sizeof(__datum_base_type)) const void *datum_in)

/*
qsort()
*/

typedef int (__cdecl * P_PROC_QSORT) (
    const void * arg1,
    const void * arg2);

#define PROC_QSORT_PROTO(_e_s, _proc_qsort, __arg_base_type) \
_e_s int  __cdecl _proc_qsort( \
    _In_bytecount_c_(sizeof(__arg_base_type)) const void * arg1, \
    _In_bytecount_c_(sizeof(__arg_base_type)) const void * arg2)

/*
qsort_s()
*/

typedef int (__cdecl * P_PROC_QSORT_S) (
    _In_        void *context,
    _In_        const void *arg1,
    _In_        const void *arg2);

/*    _In_bytecount_c_(sizeof(__ctx_base_type)) void *context, \ */

#define PROC_QSORT_S_PROTO(_e_s_, _proc_qsort_s, __ctx_base_type, __arg_base_type) \
_e_s_ int __cdecl _proc_qsort_s( \
    _In_        void * context, \
    _In_bytecount_c_(sizeof(__arg_base_type)) const void *arg1, \
    _In_bytecount_c_(sizeof(__arg_base_type)) const void *arg2)

#if RISCOS
#define HOST_HWND int   /* really wimp_w but don't tell everyone */
#define HOST_WND_NONE ((HOST_WND) 0)

#define HOST_FONT int   /* font */
#endif

/*
GDI coordinates are large signed things (device units: OS units on RISC OS)
*/

typedef S32 GDI_COORD; typedef GDI_COORD * P_GDI_COORD; typedef const GDI_COORD * PC_GDI_COORD;

#define GDI_COORD_MAX S32_MAX

#define GDI_COORD_TFMT TEXT("%d")

/*
points, or simply pairs of coordinates
*/

typedef struct _GDI_POINT
{
    GDI_COORD x, y;
}
GDI_POINT, * P_GDI_POINT; typedef const GDI_POINT * PC_GDI_POINT;

typedef struct _GDI_SIZE
{
    GDI_COORD cx, cy;
}
GDI_SIZE, * P_GDI_SIZE; typedef const GDI_SIZE * PC_GDI_SIZE;

/*
boxes, or simply pairs of points
*/

typedef struct _GDI_BOX
{
    GDI_COORD x0, y0, x1, y1;
}
GDI_BOX, * P_GDI_BOX; typedef const GDI_BOX * PC_GDI_BOX;

#define GDI_BOX_TFMT \
    TEXT("x0 = ") GDI_COORD_TFMT TEXT(", y0 = ") GDI_COORD_TFMT TEXT("; ") \
    TEXT("x1 = ") GDI_COORD_TFMT TEXT(", y1 = ") GDI_COORD_TFMT

/*
ordered rectangles
*/

typedef struct _GDI_RECT
{
    GDI_POINT tl, br;
}
GDI_RECT, * P_GDI_RECT; typedef const GDI_RECT * PC_GDI_RECT;

#define GDI_RECT_TFMT \
    TEXT("tl = ") GDI_COORD_TFMT TEXT(",") GDI_COORD_TFMT TEXT("; ") \
    TEXT("br = ") GDI_COORD_TFMT TEXT(",") GDI_COORD_TFMT

/* NB suppressing C4127: conditional expression is constant */
#define while_constant(c) \
    __pragma(warning(push)) __pragma(warning(disable:4127)) while(c) __pragma(warning(pop))

/* NB suppressing C4127: conditional expression is constant */
#define if_constant(expr) \
    __pragma(warning(push)) __pragma(warning(disable:4127)) if(expr) __pragma(warning(pop))

static inline int
memcmp32(
    _In_reads_bytes_(n_bytes) PC_ANY src_1,
    _In_reads_bytes_(n_bytes) PC_ANY src_2,
    _InVal_     U32 n_bytes)
{
    return(memcmp(src_1, src_2, n_bytes));
}

static inline void
memcpy32(
    _Out_writes_bytes_(n_bytes) P_ANY dst,
    _In_reads_bytes_(n_bytes) PC_ANY src,
    _InVal_     U32 n_bytes)
{
    (void) memcpy(dst, src, n_bytes);
}

static inline void
tiny_memcpy32(
    _Out_writes_bytes_(n_bytes) P_ANY dst,
    _In_reads_bytes_(n_bytes) PC_ANY src,
    _InVal_     U32 n_bytes) /* must be non-zero */
{
    P_BYTE to = (P_BYTE) dst;
    PC_BYTE from = (PC_BYTE) src;
    PC_BYTE from_end = from + n_bytes;

    do  {
        *to++ = *from++;
        }
    while(from != from_end);
}

static inline void
memmove32(
    _Out_writes_bytes_(n_bytes) P_ANY dst,
    _In_reads_bytes_(n_bytes) PC_ANY src,
    _InVal_     U32 n_bytes)
{
    (void) memmove(dst, src, n_bytes);
}

static inline void
memset32(
    _Out_writes_bytes_(n_bytes) P_ANY dst,
    _In_        int byteval,
    _InVal_     U32 n_bytes)
{
    (void) memset(dst, byteval, n_bytes);
}

#define zero_array(_array) \
    (void) memset(_array, 0, sizeof(_array))

#define zero_struct(_struct) \
    (void) memset(&_struct, 0, sizeof(_struct))

#define zero_struct_ptr(_ptr) \
    (void) memset(_ptr, 0, sizeof(*(_ptr)))

#define zero_32(_struct) \
    * (P_U32) (&_struct) = 0

#define zero_32_ptr(_ptr) \
    * (P_U32) (_ptr) = 0

#if defined(_PREFAST_)
#define PREFAST_ONLY_ZERO(_ptr, _size) \
    (void) memset(_ptr, 0, _size)
#define PREFAST_ONLY_ZERO_STRUCT(_struct) \
    zero_struct(_struct)
#define PREFAST_ONLY_ZERO_STRUCT_PTR(_ptr) \
    zero_struct_ptr(_ptr)
#else
#define PREFAST_ONLY_ZERO(_ptr, _size) /* nothing */
#define PREFAST_ONLY_ZERO_STRUCT(_struct) /* nothing */
#define PREFAST_ONLY_ZERO_STRUCT_PTR(_ptr) /* nothing */
#endif

#if defined(_PREFAST_)
#define PREFAST_ONLY_ARG(arg) , arg
#else
#define PREFAST_ONLY_ARG(arg) /* no arg */
#endif

/******************************************************************************

where possible, functions should return a STATUS
STATUS is a system-wide error code:

    >= 0 no error, function dependent success code
    <  0 error, system wide error code

helper macros are given:

BOOL    status_fail(STATUS)         TRUE if status is not OK
BOOL    status_ok(STATUS)           TRUE if status is OK
BOOL    status_done(STATUS)         TRUE if action taken: ie +ve
void    status_break(STATUS)        breaks on error condition
void    status_consume(STATUS)      executes expression, discards result
void    status_return(STATUS)       returns from current function with STATUS error code
void    status_accumulate(S1, S2)   executes expression S2, accumulating error into S1
void    status_assert(STATUS)       executes expression, discards result, also reports error in debug version
STATUS  status_wrap(STATUS)         executes expression, returns result, also reports error in debug version

******************************************************************************/

typedef S32 STATUS;
#define STATUS_TFMT S32_TFMT
typedef STATUS * P_STATUS;

#define STATUS_MSG_INCREMENT ((STATUS) ((STATUS) 1 << 16))
#define STATUS_ERR_INCREMENT (-STATUS_MSG_INCREMENT)

/* +ve status return values are good */
#define STATUS_DONE         ((STATUS) (+1))
#define STATUS_OK           ((STATUS)  (0))

/* -ve status return values are errors */
#define STATUS_ERROR_RQ     ((STATUS) (-1))
#define STATUS_FAIL         ((STATUS) (-2))
#define STATUS_NOMEM        ((STATUS) (-3))
#define STATUS_CANCEL       ((STATUS) (-4))
#define STATUS_CHECK        ((STATUS) (-5))

/* TRUE if status is not OK, ie -ve */
#define status_fail(status) ( \
    (status)  < STATUS_OK )

/* TRUE if status is OK, ie +ve or zero */
#define status_ok(status) ( \
    (status) >= STATUS_OK )

/* TRUE if action taken: ie +ve, non-zero */
#define status_done(status) ( \
    (status)  > STATUS_OK )

/* breaks out of loop on error condition */
#define status_break(status) \
    { \
    if(status_fail(status)) \
        break; \
    }

#define status_consume(expr) \
    consume(STATUS, expr)

/* returns from current function with STATUS error code */
#define status_return(expr) \
    do { \
    STATUS status_e = (expr); \
    if(status_fail(status_e)) \
        return(status_e); \
    } while_constant(0)

/* accumulate status failure from expression into status_a
 * NB this will preserve an initial STATUS_DONE if expression returns STATUS_OK
 */
#define status_accumulate(status_a, expr) \
    do { \
    STATUS status_e = (expr); \
    if(status_fail(status_e) && status_ok(status_a)) \
        status_a = status_e; \
    } while_constant(0)

_Check_return_
extern STATUS
create_error(
    _InVal_     STATUS status);

_Check_return_
extern STATUS
status_nomem(void);

_Check_return_
extern STATUS
status_check(void);

#if RELEASED
#define create_error(status)    (status)
#define status_nomem()          (STATUS_NOMEM)
#define status_check()          (STATUS_CHECK)
#endif

/* standard constants */
#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define INDETERMINATE 2

/*
Trivial character definitions
*/

#ifndef NULLCH
#define NULLCH '\0'
#endif

#ifndef LF
#define LF 10
#endif

#ifndef CR
#define CR 13
#endif

/*
UCS-4 character definitions
*/

/*
UCS-4 0000..001F C0 Controls
*/

#define CH_INLINE                   0x07    /* Fireworkz inline identifier code */
#define CH_TAB                      0x09
#define CH_ESCAPE                   0x1B

/*
UCS-4 0020..007F Basic Latin
*/

#define CH_SPACE                    0x20
#define CH_QUOTATION_MARK           0x22
#define CH_APOSTROPHE               0x27
#define CH_HYPHEN_MINUS             0x2D
#define CH_LOW_LINE                 0x5F
#define CH_UNDERSCORE               CH_LOW_LINE
#define CH_DELETE                   0x7F    /* NB DELETE is defined in winnt.h */

/*
UCS-4 0080..009F C1 Controls
*/

/*
UCS-4 00A0..00FF Latin-1 Supplement
*/

#define CH_NO_BREAK_SPACE           0xA0U
#define CH_SOFT_HYPHEN              0xADU
#define CH_INVERTED_QUESTION_MARK   0xBFU

/*
UCS-4 D800..DBFF High, Low Surrogates (these are used to obtain characters >= U+10000 under UTF-16 - requires Windows 2000 or later)
*/

#define CH_SURROGATE_HIGH           0xD800U
#define CH_SURROGATE_HIGH_END       0xDBFFU
#define CH_SURROGATE_LOW            0xDC00U
#define CH_SURROGATE_LOW_END        0xDFFFU

/*
UCS-4 Noncharacters
*/

#define CH_NONCHARACTER_STT         0xFDD0U
#define CH_NONCHARACTER_END         0xFDEFU

/*
UCS-4 Specials
*/

#define CH_REPLACEMENT_CHARACTER    0xFFFDU
#define CH_NONCHARACTER_XXFFFE      0xFFFEU
#define CH_NONCHARACTER_XXFFFF      0xFFFFU

#define CH_UNICODE_END              0x10FFFFU /* only UCS-4 values <= this character value are valid */

/* type for worst case alignment */
#if RISCOS
typedef int  align_t;
#define SIZEOF_ALIGN_T 4
#elif WINDOWS
typedef char align_t;
#define SIZEOF_ALIGN_T 1
#endif

/* useful max/min macros (NB watch out for multiple-evaluation side-effects) */

#ifndef MAX
#define MAX(A,B) ((A) > (B) ? (A) : (B))
#endif
#ifndef MIN
#define MIN(A,B) ((A) < (B) ? (A) : (B))
#endif

/* useful division a/b macros to round result to +/- infinity rather than towards zero (NB watch out for multiple-evaluation side-effects) */

#define div_round_ceil_u(a, b) ( \
    ((a) + (b) - 1) / (b)) /* for unsigned a */

#define div_round_ceil(a, b) ( \
    (((a) >  0) ? ((a) + (b) - 1) : (a)) / (b))

#define div_round_floor(a, b) ( \
    (((a) >= 0) ? (a) : ((a) - (b) + 1)) / (b))

#define IGNOREPARM(p)   p=p
#define IGNOREPARM_CONST(p) (void)p
#define IGNOREPARM_InRef_(p)                (void)p
#define IGNOREPARM_InoutRef_(p)             (void)p
#define IGNOREPARM_OutRef_(p)               (void)p
#define IGNOREPARM_InVal_(p)                (void)p

#if defined(_PREFAST_)
#define PREFAST_ONLY_IGNOREPARM(p)          IGNOREPARM(p)
#define PREFAST_ONLY_IGNOREPARM_InVal_(p)   IGNOREPARM_InVal_(p)
#else
#define PREFAST_ONLY_IGNOREPARM(p)          /*nothing*/
#define PREFAST_ONLY_IGNOREPARM_InVal_(p)   /*nothing*/
#endif

#define consume(__base_type, expr) \
    do { \
    __base_type v = (expr); IGNOREPARM(v); \
    } while_constant(0)

#define consume_ptr(expr) consume(PC_ANY, expr)

#define consume_bool(expr) consume(BOOL, expr)

/*
a pointer that will hopefully give a trap when dereferenced even when not CHECKING
*/

#if defined(_WIN64)

#define BAD_POINTER_X(__ptr_type, X) ((__ptr_type) \
    (uintptr_t) (0xFF00000000000000U | (X)))

#define BAD_POINTER_X_RANGE ( \
    (uintptr_t) (0x00FFFFFFFFFFFFFFU))

#else

#define BAD_POINTER_X(__ptr_type, X) ((__ptr_type) \
    (uintptr_t) (0xFF000000U | (X)))

#define BAD_POINTER_X_RANGE ( \
    (uintptr_t) (0x00FFFFFFU))

#endif /* _WIN64 */

#define IS_BAD_POINTER(p) ( \
    ((uintptr_t) (p) - BAD_POINTER_X(uintptr_t, 0)) <= BAD_POINTER_X_RANGE )

/* paranoid compatibility NULL == NONE for all */

#define _Ret_notnone_       _Ret_notnull_
#define _Ret_maybenone_     _Ret_maybenull_

#define PTR_NONE_X(__ptr_type, X) ( \
    (__ptr_type) NULL)

/*#define IS_PTR_NONE(p) FALSE*/ /* don't use directly if not CHECKING */

#define IS_PTR_NONE_X(p, X) ( \
    (NULL == (p))

#define IS_PTR_NONE_OR_NULL(__ptr_type, p) ( \
    (__ptr_type) NULL == (p) )

#define IS_PTR_NONE_X_OR_NULL(__ptr_type, p, X) ( \
    (__ptr_type) NULL == (p) )

#define _IS_P_DATA_NONE(__ptr_type, p) ( \
    (__ptr_type) NULL == (p) )

#define _P_DATA_NONE(__ptr_type) ( \
    PTR_NONE_X(__ptr_type, 0))

/*
round up v if needed to the next r (r must be a power of 2)
*/

#define round_up(v, r) ( \
    ((v) + ((r) - 1)) & ~((r) - 1))

#define sizeof32(__base_type) \
    ((U32) sizeof(__base_type))

/* sizeof the type obtained by dereferencing the pointer type */
#define sizeof_deref(__ptr_type) \
    sizeof((__ptr_type) 0)

#define sizeof32_deref(__ptr_type) \
    sizeof32((__ptr_type) 0)

#define offsetof32(__base_type, _member) \
    ((U32) offsetof(__base_type, _member))

/*
deviant of sizeof{32}(exp) formed from offsetof{32}(type, member)
*/

#define sizeofmemb(__base_type, _member) \
    sizeof(((__base_type *) 0)->_member)

#define sizeofmemb32(__base_type, _member) \
    sizeof32(((__base_type *) 0)->_member)
   /*
    * expands to an integral constant expression that has type size_t/U32, the
    * value of which is the size in bytes of a member designated by the
    * identifier member of a structure designated by type (if the
    * specified member is a bit-field, the behaviour is undefined).
    */

/*
number of elements in an array (use rather than sizeof()/sizeof32() in many instances)
*/

#define elemof(_array) \
    (sizeof(_array)/sizeof(_array[0]))

#define elemof32(_array) \
    (sizeof32(_array)/sizeof32(_array[0]))

/* is a (zero-based) index valid for this array? */
#define IS_ARRAY_INDEX_VALID(_idx, _elemof32_array) \
    ((U32) (_idx) < _elemof32_array)

/*
Enumeration handling
*/

#define ENUM_NEXT(__enum_type, value) \
    ((__enum_type) ((int) (value) + 1))

#define ENUM_PREV(__enum_type, value) \
    ((__enum_type) ((int) (value) - 1))

#define ENUM_INCR(__enum_type, value__ref) \
    (value__ref = ENUM_NEXT(__enum_type, (value__ref)))

#define ENUM_DECR(__enum_type, value__ref) \
    (value__ref = ENUM_PREV(__enum_type, (value__ref)))

/* enumerations are sometimes stored in unsigned bit fields or other shorter types */
#define ENUM_PACK(__packed_type, value) \
    ((__packed_type) (value))

#define ENUM_UNPACK(__enum_type, packed_value) \
    ((__enum_type) (packed_value))

#if CHECKING
#define CHECKING_ONLY_ARG(arg) , arg
#else
#define CHECKING_ONLY_ARG(arg) /* no arg */
#endif

#if (defined(PROFILING) || defined(FULL_FRAMES)) && RISCOS
extern void
profile_ensure_frame(void); /* ensure procedure gets a stack frame we can trace out of at the cost of two branches */
#else
#define profile_ensure_frame() /* nothing */
#endif

#endif /* __coltsoft_h */

/* end of coltsoft.h */
