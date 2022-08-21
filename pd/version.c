/* version.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Version information for PipeDream */

/*#include "common/gflags.h"*/

/* NB See also
 * r32.Makefile (for !Boot/!Run building)
 * r32.RiscPkg.Control
 * r32.!PipeDream.Resource.Relnotes
 */

#define Version      "4.53/06"
#define CurrentDate  "(21-Sep-2015)"

/*
* 21 Sep 2015 - 4.53/06
* 14 Sep 2015 - 4.53/05
* 04 Sep 2015 - 4.53/04
* 06 Aug 2015 - 4.53/03
* 09 Jul 2015 - 4.53/02
* 07 Jul 2015 - 4.53/01
* 10 Jun 2015 - 4.53
* 18 Dec 2014 - 4.52/08
* 05 Nov 2014 - 4.52/07
* 01 Nov 2014 - 4.52/06
* 16 Jul 2014 - 4.52/05
* 04 Jul 2014 - 4.52/04
* 18 Feb 2014 - 4.52/03
* 12 Feb 2014 - 4.52/02
* 09 Feb 2014 - 4.52/01
* 07 Feb 2014 - 4.52/00
* 06 Jan 2014 - 4.51/00
* 08 Aug 2013 - 4.50/50
* 02 Aug 2013 - 4.50/49
* 28 Jun 2013 - 4.50/48
* 24 Jun 2013 - 4.50/47
* 20 Jun 2013 - 4.50/46
* 05 Jun 2013 - 4.50/45
* 13 May 2013 - 4.50/44
* 09 May 2013 - 4.50/43
* 22 Mar 2013 - 4.50/42
* 19 Mar 2013 - 4.50/41
* 30 Oct 2012 - 4.50/40
* 26 Oct 2012 - 4.50/39
* 11 Oct 2012 - 4.50/38
* 19 Sep 2012 - 4.50/37
* 31 Aug 2012 - 4.50/36
* 23 Mar 2012 - 4.50/35
* 22 Mar 2012 - 4.50/34
* 22 Mar 2012 - 4.50/33
* 21 Mar 2012 - 4.50/32 - the first 32-bit PipeDream 4
* 10 Aug 2000 - 4.50/23 - the last 26-bit PipeDream 4
* 05 Apr 1999 - 4.50/22
* 17 Jan 1999 - 4.50/21
* 14 Nov 1998 - 4.50/20
* 12 Jan 1998 - 4.50/14 as we'd made two /13s
* 28 Nov 1997 - 4.50/13
* 10 Nov 1997 - 4.50/12
* 08 Sep 1997 - 4.50/11
* 28 May 1997 - 4.50/10
* 15 Jan 1997 - 4.50/08
* 17 Jan 1997 - 4.50/08
* 16 Dec 1996 - 4.50/07
* 01 Dec 1996 - 4.50/06
* 13 Nov 1996 - 4.50/05
* 30 Oct 1996 - 4.50/04
* 30 Oct 1996 - 4.50/03
* 24 Jul 1996 - 4.50/01
* 18 Feb 1996 - 4.50 - the first StrongARM PipeDream 4
* 28 Apr 1992 - 4.13
* 10 Oct 1991 - 4.00 - the first PipeDream 4
* 11 Oct 1990 - 3.14 - the last PipeDream 3
*/

#include "version_x.h"

const char applicationversion[] = Version " " CurrentDate;

/* end of version.c */
