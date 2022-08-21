/* nd-colourtran.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2012-2016 Stuart Swales */

#define INCLUDE_FOR_NO_DELTA 1

#include "include.h"

#undef ColourTrans_SelectTable
#undef ColourTrans_SelectGCOLTable
#undef ColourTrans_ReturnGCOL
#undef ColourTrans_SetGCOL
#undef ColourTrans_ReturnColourNumber
#undef ColourTrans_ReturnGCOLForMode
#undef ColourTrans_ReturnColourNumberForMode
#undef ColourTrans_ReturnOppGCOL
#undef ColourTrans_SetOppGCOL
#undef ColourTrans_ReturnOppColourNumber
#undef ColourTrans_ReturnOppGCOLForMode
#undef ColourTrans_ReturnOppColourNumberForMode
#undef ColourTrans_GCOLToColourNumber
#undef ColourTrans_ColourNumberToGCOL
#undef ColourTrans_ReturnFontColours
#undef ColourTrans_SetFontColours
#undef ColourTrans_InvalidateCache
#undef ColourTrans_SetCalibration
#undef ColourTrans_ReadCalibration
#undef ColourTrans_ConvertDeviceColour
#undef ColourTrans_ConvertDevicePalette
#undef ColourTrans_ConvertRGBToCIE
#undef ColourTrans_ConvertCIEToRGB

#include "colourtran.c"

/* nd-colourtran.c */
