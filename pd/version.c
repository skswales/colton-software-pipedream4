/* version.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Version information for PipeDream */

/*#include "common/gflags.h"*/

/* NB See also
 * Relnotes/html
 * Build.r32.Makefile (for !Boot/!Run building)
 * Build.r32.MakeRiscPkg
 * Build.r32.RiscPkg.Control
 * &.extrautils.PlingStore.MakeStore
 */

#define HexVersion   0x46200
#define Version      "4.62"
#define CurrentDate  "28-Dec-2022"

/*
* 28-Dec-2022 - 4.62
* 12-Dec-2022 - 4.61
* 23-Oct-2022 - 4.60.03
* 18-Aug-2022 - 4.60.02
* 04-Aug-2022 - 4.60.01
* 22 Jul 2022 - 4.60
* 01 Oct 2021 - 4.59.01
* 06 Sep 2021 - 4.59
* 31 Jan 2021 - 4.58.03
* 27 Sep 2020 - 4.58.02
* 24 Sep 2020 - 4.58.01
* 29 Jul 2020 - 4.58
* 21 Mar 2020 - 4.57.01
* 20 Dec 2019 - 4.57
* 16 Aug 2019 - 4.57 Don't think anyone got this preview
* 19 Mar 2019 - 4.56.02
* 21 Dec 2018 - 4.56.01
* 17 Dec 2018 - 4.56
* 01 Aug 2016 - 4.55.04
* 27 Jul 2016 - 4.55.03
* 12 Jul 2016 - 4.55.02
* 03 Jul 2016 - 4.55.01 Move to x.yy.zz scheme for !Store version ease
* 21 May 2016 - 4.55
* 22 Nov 2015 - 4.54
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
*
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
*
* 11 Oct 1990 - 3.14 - the last PipeDream 3
* 14 Sep 1990 - 3.13
* 22 May 1990 - 3.11
* 03 May 1990 - 3.10
* 15 Jan 1990 - 3.07
* 16 Oct 1989 - 3.06
* 22 Sep 1989 - 3.05
* 26 Jul 1989 - 3.00 - the first PipeDream 3
* 21 Jul 1989 - 3.00 (prerelease)
*
* 04 Jun 1992 - 2.21 - the last PipeDream 2
* 18 Nov 1988 - 2.2
*/

#include "version_x.h"

const int hexversion = HexVersion;

const char applicationversion[] = Version " (" CurrentDate ")";

/* end of version.c */
