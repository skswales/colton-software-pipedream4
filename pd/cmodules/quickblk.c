/* quickblk.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/******************************************************************************
*
* quick_block allocation module (both BYTE- and TCHAR-based)
*
* efficient routines for handling temporary buffers
*
* you supply a 'static' buffer; if the item to be added to the buffer
* would overflow the supplied buffer, a handle-based buffer is allocated
*
* SKS November 2006
*
******************************************************************************/

#include "common/gflags.h"

/*#include "ob_skel/flags.h"*/

/******************************************************************************
*
* BYTE-based quick block
*
******************************************************************************/

/******************************************************************************
*
* add some bytes to a quick_block
*
******************************************************************************/

_Check_return_
extern STATUS
quick_block_bytes_add(
    _InoutRef_  P_QUICK_BLOCK p_quick_block,
    _In_reads_bytes_(n_bytes) PC_ANY p_any_in,
    _InVal_     U32 n_bytes)
{
    STATUS status = STATUS_OK;

    if(0 != n_bytes) /* SKS 23may06 - adding 0 bytes is ok */
    {
        P_U8 p_u8;

        assert(n_bytes < 0xF0000000U); /* check possible -ve client */

        if(NULL != (p_u8 = quick_block_extend_by(p_quick_block, n_bytes, &status)))
            void_memcpy32(p_u8, p_any_in, n_bytes);
    }

    return(status);
}

/******************************************************************************
*
* dispose of a quick_block of data but leave converted text in place (see charts)
*
******************************************************************************/

extern void
quick_block_dispose_leaving_text_valid(
    _InoutRef_  P_QUICK_BLOCK p_quick_block)
{
    /* have we overflowed the buffer and gone into handle allocation? */
    if(0 != p_quick_block->h_array_buffer)
    {   /* copy as much stuff as possible back down before deleting the handle */
        U32 n = MIN((U32) p_quick_block->static_buffer_size, array_elements32(&p_quick_block->h_array_buffer));

        void_memcpy32(p_quick_block->p_static_buffer, array_basec(&p_quick_block->h_array_buffer, BYTE), n);

        al_array_dispose(&p_quick_block->h_array_buffer);
    }

    p_quick_block->static_buffer_used = 0;
}

/******************************************************************************
*
* empty a quick_block of data for efficient reuse
*
******************************************************************************/

extern void
quick_block_empty(
    _InoutRef_  P_QUICK_BLOCK p_quick_block)
{
    if(p_quick_block->h_array_buffer)
    {
#if QUICK_BLOCK_CHECK /* SKS 23jan95 trash buffer on empty */
        __aqb_fill(array_base(&p_quick_block->h_array_buffer, U8), array_elements32(&p_quick_block->h_array_buffer));
#endif
        al_array_empty(&p_quick_block->h_array_buffer);
    }
    else
    {
#if QUICK_BLOCK_CHECK /* SKS 23jan95 trash buffer on exit */
        if(p_quick_block->static_buffer_size)
            __aqb_fill(p_quick_block->p_static_buffer, p_quick_block->static_buffer_size);
#endif
        p_quick_block->static_buffer_used = 0;
    }
}

/******************************************************************************
*
* ensure that the given quick_block is not NULLCH terminated
*
******************************************************************************/

extern void
quick_block_nullch_strip(
    _InoutRef_  P_QUICK_BLOCK p_quick_block)
{
    const U32 len = quick_block_bytes(p_quick_block);

    if(len)
    {
        P_U8 p_u8 = quick_block_ptr(p_quick_block);

        if(p_u8[len-1] == NULLCH)
        {
#if QUICK_BLOCK_CHECK
            p_u8[len-1] = 0xEE; /* SKS 23jan95 trash terminator */
#endif
            quick_block_shrink_by(p_quick_block, -1);
        }
    }
}

/******************************************************************************
*
* efficient routine for allocating temporary buffers
*
* you supply a buffer; if the item is bigger than this,
* an indirect buffer is allocated
*
* remember to call quick_block_dispose afterwards!
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern P_U8
quick_block_extend_by(
    _InoutRef_  P_QUICK_BLOCK p_quick_block,
    _InVal_     U32 size_wanted,
    _OutRef_    P_STATUS p_status)
{
    *p_status = STATUS_OK;

    if(0 == size_wanted) /* SKS 23may96 - realloc by 0 is ok, but return a non-NULL rubbish pointer */
        return((P_U8) (uintptr_t) 1);

    assert(size_wanted < 0xF0000000U); /* real world use always +ve; check possible -ve client */

#if 0
    if(size_wanted < 0)
    {
        quick_block_shrink_by(p_quick_block, size_wanted);
        return((P_U8) (uintptr_t) 1);
    }
#endif

    if(p_quick_block->h_array_buffer)
        return(al_array_extend_by(&p_quick_block->h_array_buffer, U8, size_wanted, PC_ARRAY_INIT_BLOCK_NONE, p_status));

    if(size_wanted <= p_quick_block->static_buffer_size - p_quick_block->static_buffer_used)
    {
        P_U8 p_output = (P_U8) p_quick_block->p_static_buffer + p_quick_block->static_buffer_used;
        p_quick_block->static_buffer_used += size_wanted;
        return(p_output);
    }

    {
    static /*poked*/ ARRAY_INIT_BLOCK array_init_block = aib_init(4, sizeof32(U8), FALSE);
    S32 incr = p_quick_block->static_buffer_size / 4;
    P_U8 p_output;

    if( incr < 4)
        incr = 4;

    array_init_block.size_increment = incr;

    if(NULL != (p_output = al_array_alloc(&p_quick_block->h_array_buffer, U8, p_quick_block->static_buffer_used + size_wanted, &array_init_block, p_status)))
    {
        void_memcpy32(p_output, p_quick_block->p_static_buffer, p_quick_block->static_buffer_used);
#if QUICK_BLOCK_CHECK
        __aqb_fill(p_quick_block->p_static_buffer, p_quick_block->static_buffer_size);
#endif
        p_output += p_quick_block->static_buffer_used;
        p_quick_block->static_buffer_used = 0;
    }

    return(p_output);
    } /*block*/
}

extern void
quick_block_shrink_by(
    _InoutRef_  P_QUICK_BLOCK p_quick_block,
    _In_        S32 size_wanted)
{
    assert(size_wanted <= 0);

    if(size_wanted >= 0)
        return;

    if(p_quick_block->h_array_buffer)
        al_array_shrink_by(&p_quick_block->h_array_buffer, size_wanted);
    else
    {
        p_quick_block->static_buffer_used += size_wanted;
        assert((S32) p_quick_block->static_buffer_used >= 0);
    }
}

/******************************************************************************
*
* add a string to a quick_block
*
******************************************************************************/

_Check_return_
extern STATUS
quick_block_string_add(
    _InoutRef_  P_QUICK_BLOCK p_quick_block,
    _In_z_      PC_U8Z p_u8)
{
    return(quick_block_bytes_add(p_quick_block, p_u8, strlen32(p_u8)));
}

_Check_return_
extern STATUS
quick_block_string_with_nullch_add(
    _InoutRef_  P_QUICK_BLOCK p_quick_block,
    _In_z_      PC_U8Z p_u8)
{
    return(quick_block_bytes_add(p_quick_block, p_u8, strlen32p1(p_u8)));
}

/******************************************************************************
*
* add a u8 to a quick_block
*
******************************************************************************/

_Check_return_
extern STATUS
quick_block_u8_add(
    _InoutRef_  P_QUICK_BLOCK p_quick_block,
   _InVal_      U8 u8)
{
    STATUS status;
    P_U8 p_u8;

    if(NULL != (p_u8 = quick_block_extend_by(p_quick_block, 1, &status)))
        *p_u8 = u8;

    return(status);
}

static U8 u8_large_buffer[4 * 1024];

_Check_return_
_Ret_z_ /* never NULL */
static PC_L1STR /*low-lifetime*/
_l1str_from_wchars(
    _In_reads_(n_wchars) PCWCH pwch,
    _In_        U32 n_wchars)
{
    int avail = 0;
    int used = 0;
    P_U8Z dstptr;
    int n;

    if(0 == n_wchars)
        return(empty_string);

#if CHECKING
    if(NULL == pwch)
        return(("<<l1str_from_wstr - NULL>>"));
#endif

    avail = elemof32(u8_large_buffer);
    used = 0;

    dstptr = &u8_large_buffer[used];

#if WINDOWS && 0

    n = WideCharToMultiByte(CP_ACP /*CodePage*/, 0 /*dwFlags*/,
                            pwch, n_wchars /*tstrlen32(wstr) + NULLCH*/, (PSTR) dstptr, avail,
                            NULL /*lpDefaultChar*/, NULL /*lpUsedDefaultChar*/);

    if(n > 0)
    {
        used += n;
        avail -= n;
    }

#else /* portable */

    for(n = 0; n < avail; n++)
    {
        WCHAR ch;
        if((U32) n >= n_wchars)
            break;
        ch = pwch[n];
        if(NULLCH == ch)
            break; /* unexpected end of string */
        if(0 != (ch & 0xFF00))
            dstptr[n] = '?';
        else
            dstptr[n] = (U8) (ch & 0xFF);
        used++;
    }

#endif /* OS */

    if(used < avail)
        dstptr[used] = NULLCH;

    if(used == avail)
        dstptr[used - 1] = NULLCH;

    return(dstptr);
}

/******************************************************************************
*
* add some WCHARs to a quick_block
*
******************************************************************************/

_Check_return_
extern STATUS
quick_block_wchars_add(
    _InoutRef_  P_QUICK_BLOCK p_quick_block,
    _In_reads_(len) PCWCH pwch_in,
    _InVal_     U32 len)
{
    if(0 != len) /* SKS 23may06 - adding 0 WCHARs is ok */
    {
        PC_L1STR l1str = _l1str_from_wchars(pwch_in, len);

        return(quick_block_bytes_add(p_quick_block, l1str, strlen32(l1str)));
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* sprintf/vsprintf into ARRAY_QUICK_BLOCKs
*
* NB. does NOT add terminating NULLCH
*
******************************************************************************/

_Check_return_
extern STATUS __cdecl
quick_block_printf(
    _InoutRef_  P_QUICK_BLOCK p_quick_block,
    _In_z_ _Printf_format_string_ PC_U8Z format,
    /**/        ...)
{
    va_list args;
    STATUS status;

    va_start(args, format);
    status = quick_block_vprintf(p_quick_block, format, args);
    va_end(args);

    return(status);
}

_Check_return_
extern STATUS
quick_block_vprintf(
    _InoutRef_  P_QUICK_BLOCK p_quick_block,
    _In_z_ _Printf_format_string_ PC_U8Z format,
    /**/        va_list args)
{
    PC_U8Z p_u8;
    STATUS status = STATUS_OK;

    /* loop finding format specifications */
    while(NULL != (p_u8 = strchr(format, '%')))
    {
#if WINDOWS
        U8 preceding;
        U8 conversion;
#endif

        /* output what we have so far */
        if(p_u8 - format)
            status_break(status = quick_block_bytes_add(p_quick_block, format, PtrDiffBytesU32(p_u8, format)));

        format = p_u8 + 1; /* skip the % */

        if(*format == '%')
        {
            status_break(status = quick_block_u8_add(p_quick_block, '%'));
            format++; /* skip the escaped % too */
            continue;
        }

        /* skip goop till conversion found */
        if(NULL == (p_u8 = strpbrk(format, "diouxXcsfeEgGpnCS")))
        {
            assert0();
            break;
        }

        /* skip over conversion */
#if WINDOWS
        preceding = p_u8[-1];
        conversion = *p_u8++;
#else
        p_u8++;
#endif

        {
        U8Z buffer[512]; /* hopefully big enough for a single conversion (was 256, but consider a f.p. number numform()ed to 308 digits...) */
        U8Z buffer_format[32];
        S32 len;

        void_strnkpy(buffer_format, elemof32(buffer_format), format - 1, PtrDiffBytesU32(p_u8, format) + 1);

#if RISCOS
        len = vsnprintf(buffer, elemof32(buffer), buffer_format, args /* will be updated accordingly */);
        if(len >= elemof32(buffer))
            len = strlen32(buffer); /* limit transfer to what actually was achieved */
#elif WINDOWS
        len = _vsnprintf_s(buffer, elemof32(buffer), _TRUNCATE, buffer_format, args /* will be updated accordingly */);
        if(-1 == len)
            len = strlen32(buffer); /* limit transfer to what actually was achieved */
#endif

        status_break(status = quick_block_bytes_add(p_quick_block, buffer, len));
#if WINDOWS
        { /* Wot a bummer. Microsoft seems to pass args by value not reference so that args ain't updated like Norcroft compiler */
        switch(conversion)
        {
        case 'f':
        case 'e':
        case 'E':
        case 'g':
        case 'G':
            if(preceding == 'L')
            {
                volatile long double ld = va_arg(args, long double); ld=ld;
            }
            else if(preceding == 'l')
            {
                volatile double d = va_arg(args, double); d=d;
            }
            else
            {
                volatile float f = va_arg(args, float); f=f;
            }
            break;

        case 's':
        case 'S':
        case 'p':
        case 'n':
            if(preceding == 'l')
            {
                volatile char /*__far*/ * lp = va_arg(args, char /*__far*/ *); lp=lp;
            }
            else if(preceding == 'h')
            {
                volatile char /*__near*/ * hp = va_arg(args, char /*__near*/ *); hp=hp;
            }
            else
            {
                volatile char * p = va_arg(args, char *); p=p;
            }
            break;

        case 'c':
        case 'C':
            {
            /* chars are widened to int when passing as parameters */
            volatile int ci = va_arg(args, int); ci=ci;
            break;
            }

        default:
        case 'd':
        case 'i':
        case 'o':
        case 'u':
        case 'x':
        case 'X':
            if(preceding == 'l')
            {
                volatile long li = va_arg(args, long); li=li;
            }
            else if(preceding == 'h')
            {
                volatile short hi = va_arg(args, short); hi=hi;
            }
            else
            {
                volatile int i = va_arg(args, int); i=i;
            }
            break;
        }
        } /*block*/
#endif
        } /*block*/

        format = p_u8; /* skip whole conversion sequence */
    }

    /* output trailing fragment */
    if(*format && status_ok(status))
        status = quick_block_string_add(p_quick_block, format);

    return(status);
}

/******************************************************************************
*
* TCHAR-based quick block
*
******************************************************************************/

#if TSTR_IS_L1STR

/* defined as macros */

#else /* TSTR_IS_L1STR */

/******************************************************************************
*
* dispose of a quick_tblock of data but leave converted text in place (see charts)
*
******************************************************************************/

extern void
quick_tblock_dispose_leaving_text_valid(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock)
{
    /* have we overflowed the buffer and gone into handle allocation? */
    if(0 != p_quick_tblock->tb_h_array_buffer)
    {   /* copy as much stuff as possible back down before deleting the handle */
        U32 n = MIN((U32) p_quick_tblock->tb_static_buffer_elem, array_elements32(&p_quick_tblock->tb_h_array_buffer));

        void_memcpy32(p_quick_tblock->tb_p_static_buffer, array_basec(&p_quick_tblock->tb_h_array_buffer, BYTE), n * sizeof32(TCHAR));

        al_array_dispose(&p_quick_tblock->tb_h_array_buffer);
    }

    p_quick_tblock->tb_static_buffer_used = 0;
}

/******************************************************************************
*
* empty a quick_tblock of data for efficient reuse
*
******************************************************************************/

extern void
quick_tblock_empty(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock)
{
    if(p_quick_tblock->tb_h_array_buffer)
    {
#if QUICK_BLOCK_CHECK /* trash buffer on empty */
        __tqb_fill(array_base(&p_quick_tblock->tb_h_array_buffer, TCHAR), array_elements32(&p_quick_tblock->tb_h_array_buffer));
#endif
        al_array_empty(&p_quick_tblock->tb_h_array_buffer);
    }
    else
    {
#if QUICK_BLOCK_CHECK /* trash buffer on exit */
        if(p_quick_tblock->tb_static_buffer_elem)
            __tqb_fill(p_quick_tblock->tb_p_static_buffer, p_quick_tblock->tb_static_buffer_elem);
#endif
        p_quick_tblock->tb_static_buffer_used = 0;
    }
}

/******************************************************************************
*
* ensure that the given quick_tblock is not NULLCH terminated
*
******************************************************************************/

extern void
quick_tblock_nullch_strip(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock)
{
    U32 len = quick_tblock_elements(p_quick_tblock);

    if(len)
    {
        PTCH ptch = quick_tblock_tptr(p_quick_tblock);

        if(ptch[len-1] == NULLCH)
        {
#if QUICK_BLOCK_CHECK
            ptch[len-1] = 0xEE; /* SKS 23jan95 trash terminator */
#endif
            quick_tblock_shrink_by(p_quick_tblock, -1);
        }
    }
}

/******************************************************************************
*
* efficient routine for allocating temporary buffers of TCHAR
*
* you supply a buffer; if the item is bigger than this,
* an indirect buffer is allocated
*
* remember to call quick_tblock_dispose afterwards!
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern PTCH
quick_tblock_extend_by(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock,
    _InVal_     U32 elem_wanted,
    _OutRef_    P_STATUS p_status)
{
    *p_status = STATUS_OK;

    if(0 == elem_wanted) /* SKS 23may96 - realloc by 0 is ok, but return a non-NULL rubbish pointer */
        return((PTCH) (uintptr_t) 1);

    assert(elem_wanted < 0xF0000000U); /* real world use always +ve; check possible -ve client */

#if 0
    if(elem_wanted < 0)
    {
        quick_tblock_shrink_by(p_quick_tblock, elem_wanted);
        return((PTCH) (uintptr_t) 1);
    }
#endif

    if(p_quick_tblock->tb_h_array_buffer)
        return(al_array_extend_by(&p_quick_tblock->tb_h_array_buffer, TCHAR, elem_wanted, PC_ARRAY_INIT_BLOCK_NONE, p_status));

    if(elem_wanted <= p_quick_tblock->tb_static_buffer_elem - p_quick_tblock->tb_static_buffer_used)
    {
        PTCH p_output = p_quick_tblock->tb_p_static_buffer + p_quick_tblock->tb_static_buffer_used;
        p_quick_tblock->tb_static_buffer_used += elem_wanted;
        return(p_output);
    }

    {
    static /*poked*/ ARRAY_INIT_BLOCK array_init_block = aib_init(4, sizeof32(TCHAR), FALSE);
    S32 incr = p_quick_tblock->tb_static_buffer_elem / 4;
    PTCH p_output;

    if( incr < 4)
        incr = 4;

    array_init_block.size_increment = incr;

    if(NULL != (p_output = al_array_alloc(&p_quick_tblock->tb_h_array_buffer, TCHARZ, p_quick_tblock->tb_static_buffer_used + elem_wanted, &array_init_block, p_status)))
    {
        void_memcpy32(p_output, p_quick_tblock->tb_p_static_buffer, p_quick_tblock->tb_static_buffer_used * sizeof32(TCHAR));
#if QUICK_BLOCK_CHECK
        __tqb_fill(p_quick_tblock->tb_p_static_buffer, p_quick_tblock->tb_static_buffer_elem);
#endif
        p_output += p_quick_tblock->tb_static_buffer_used;
        p_quick_tblock->tb_static_buffer_used = 0;
    }

    return(p_output);
    } /*block*/
}

extern void
quick_tblock_shrink_by(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock,
    _In_        S32 elem_wanted)
{
    assert(elem_wanted <= 0);

    if(elem_wanted >= 0)
        return;

    if(p_quick_tblock->tb_h_array_buffer)
        al_array_shrink_by(&p_quick_tblock->tb_h_array_buffer, elem_wanted);
    else
    {
        p_quick_tblock->tb_static_buffer_used += elem_wanted;
        assert((S32) p_quick_tblock->tb_static_buffer_used >= 0);
    }
}

/******************************************************************************
*
* add a TCHAR to a quick_tblock
*
******************************************************************************/

_Check_return_
extern STATUS
quick_tblock_tchar_add(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock,
    _In_        TCHAR tchar)
{
    PTCH ptch;

    if(NULL == (ptch = quick_tblock_extend_by(p_quick_tblock, 1)))
        return(status_nomem());

    *ptch = tchar;

    return(STATUS_OK);
}

/******************************************************************************
*
* add some TCHARs to a quick_tblock
*
******************************************************************************/

_Check_return_
extern STATUS
quick_tblock_tchars_add(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock,
    _In_reads_(len) PCTCH ptch_in,
    _InVal_     U32 len)
{
    if(0 != len) /* adding 0 TCHARs is ok */
    {
        PTCH ptch;

        if(NULL == (ptch = quick_tblock_extend_by(p_quick_tblock, len)))
            return(status_nomem());

        void_memcpy32(ptch, ptch_in, len * sizeof32(*ptch));
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* add a TCHARZ string to a quick_tblock
*
******************************************************************************/

_Check_return_
extern STATUS
quick_tblock_tstr_add(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock,
    _In_z_      PCTSTR tstr)
{
    return(quick_tblock_tchars_add(p_quick_tblock, tstr, tstrlen32(tstr)));
}

_Check_return_
extern STATUS
quick_tblock_tstr_with_nullch_add(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock,
    _In_z_      PCTSTR tstr)
{
    return(quick_tblock_tchars_add(p_quick_tblock, tstr, tstrlen32p1(tstr)));
}

/******************************************************************************
*
* sprintf/vsprintf into ARRAY_QUICK_TBLOCKs
*
* NB. does NOT add terminating NULLCH
*
******************************************************************************/

_Check_return_
extern STATUS __cdecl
quick_tblock_printf(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        ...)
{
    va_list args;
    STATUS status;

    va_start(args, format);
    status = quick_tblock_vprintf(p_quick_tblock, format, args);
    va_end(args);

    return(status);
}

extern STATUS
quick_tblock_vprintf(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        va_list args)
{
    PCTSTR tstr;
    STATUS status = STATUS_OK;

    /* loop finding format specifications */
    while(NULL != (tstr = tstrchr(format, '%')))
    {
#if WINDOWS
        TCHAR preceding;
        TCHAR conversion;
#endif

        /* output what we have so far */
        if(tstr - format)
            status_break(status = quick_tblock_tchars_add(p_quick_tblock, format, PtrDiffElemU32(tstr, format)));

        format = tstr + 1; /* skip the % */

        if(*format == '%')
        {
            status_break(status = quick_tblock_tchar_add(p_quick_tblock, '%'));
            format++; /* skip the escaped % too */
            continue;
        }

        /* skip goop till conversion found */
        if(NULL == (tstr = tstrpbrk(format, TEXT("diouxXcsfeEgGpnCS"))))
        {
            assert0();
            break;
        }

        /* skip over conversion */
#if WINDOWS
        preceding = tstr[-1];
        conversion = *tstr++;
#else
        tstr++;
#endif

        {
        TCHARZ buffer[256]; /* hopefully big enough for a single conversion */
        TCHARZ buffer_format[32];
        S32 len;

        void_tstrnkpy(buffer_format, elemof32(buffer_format), format - 1, PtrDiffElemU32(tstr, format) + 1);

#if RISCOS
        len = vsnprintf(buffer, elemof32(buffer), buffer_format, args /* will be updated accordingly */);
        if(len >= elemof32(buffer))
            len = tstrlen32(buffer); /* limit transfer to what actually was achieved */
#elif WINDOWS
        len = _vsntprintf_s(buffer, elemof32(buffer), _TRUNCATE, buffer_format, args /* will be updated accordingly */);
        if(-1 == len)
            len = tstrlen32(buffer); /* limit transfer to what actually was achieved */
#endif

        status_break(status = quick_tblock_tchars_add(p_quick_tblock, buffer, len));
#if WINDOWS
        { /* Wot a bummer. Microsoft seems to pass args by value not reference so that args ain't updated like Norcroft compiler */
        switch(conversion)
        {
        case 'f':
        case 'e':
        case 'E':
        case 'g':
        case 'G':
            if(preceding == 'L')
            {
                volatile long double ld = va_arg(args, long double); ld=ld;
            }
            else if(preceding == 'l')
            {
                volatile double d = va_arg(args, double); d=d;
            }
            else
            {
                volatile float f = va_arg(args, float); f=f;
            }
            break;

        case 's':
        case 'S':
        case 'p':
        case 'n':
            if(preceding == 'l')
            {
                volatile char /*__far*/ * lp = va_arg(args, char /*__far*/ *); lp=lp;
            }
            else if(preceding == 'h')
            {
                volatile char /*__near*/ * hp = va_arg(args, char /*__near*/ *); hp=hp;
            }
            else
            {
                volatile char * p = va_arg(args, char *); p=p;
            }
            break;

        case 'c':
        case 'C':
            {
            /* chars are widened to int when passing as parameters */
            volatile int ci = va_arg(args, int); ci=ci;
            break;
            }

        default:
        case 'd':
        case 'i':
        case 'o':
        case 'u':
        case 'x':
        case 'X':
            if(preceding == 'l')
            {
                volatile long li = va_arg(args, long); li=li;
            }
            else if(preceding == 'h')
            {
                volatile short hi = va_arg(args, short); hi=hi;
            }
            else
            {
                volatile int i = va_arg(args, int); i=i;
            }
            break;
        }
        } /*block*/
#endif
        } /*block*/

        format = tstr; /* skip whole conversion sequence */
    }

    /* output trailing fragment */
    if(*format && status_ok(status))
        status = quick_tblock_tstr_add(p_quick_tblock, format);

    return(status);
}

/******************************************************************************
*
* add a UCS-4 character encoded as TCHAR(s) to a quick_tblock
*
******************************************************************************/

_Check_return_
extern STATUS
quick_tblock_ucs4_add(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _InVal_     UCS4 ucs4)
{
    TCHAR buffer[2];
    U32 n_bytes; /* NB not chars */

#if TSTR_IS_L1STR
    if(UCS4_is_latin1(ucs4))
    {
        buffer[0] = (TCHAR) ucs4;
        n_bytes = sizeof32(TCHAR);
    }
    else
    {
        n_bytes = 0;
    }
#else /* NOT TSTR_IS_L1STR */
#if 1
    if(ucs4 <= U16_MAX)
    {
        buffer[0] = (TCHAR) ucs4;
        n_bytes = sizeof32(TCHAR);
    }
    else
    {
        n_bytes = 0;
    }
#else
    n_bytes = UTF16_CharEncode(buffer, /*NB*/ sizeof32(buffer), ucs4);
#endif
#endif /* TSTR_IS_L1STR  */

    return(quick_tblock_tchars_add(p_quick_tblock, buffer, n_bytes / sizeof32(TCHAR)));
}

#endif /* TSTR_IS_L1STR */

/* end of quickblk.c */
