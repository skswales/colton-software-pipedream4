/***
*mysetjmp.h
*
*   Copyright (c) 1990, Colton Software Ltd.
*
*Purpose:
*   Define access to setjmp/longjmp-like functions
*
*******************************************************************************/

#ifndef __mysetjmp_h
#define __mysetjmp_h

#if WINDOWS

#define jmp_buf                    CATCHBUF

#define longjmp(lpCATCHBUF, iParm) Throw(lpCATCHBUF, iParm)
#define setjmp(lpCATCHBUF)         Catch(lpCATCHBUF)

#else
/* use standard definitions */

#include <setjmp.h>

#endif /* WINDOWS */

#endif /* __mysetjmp_h */

/* end of mysetjmp.h */

