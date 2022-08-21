/* mathxtr2.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* SKS September 1991 */

#ifndef __mathxtr2_h
#define __mathxtr2_h

/* M[j,k] = mat[n*j+k] - data stored by cols across row, then cols across next row ... */

#define em(_array, _n_cols_in_row, _j_idx, _k_idx) ( \
    (_array) + (_j_idx) * (_n_cols_in_row))[_k_idx]

/*
colID states
*/

#define LINEST_A_COLOFF (-1)
#define LINEST_Y_COLOFF 0
#define LINEST_X_COLOFF (+1)

#define LINEST_COL_ID int

typedef S32 LINEST_COLOFF;
typedef S32 LINEST_ROWOFF;

typedef /*_Check_return_*/ F64 (* P_PROC_LINEST_DATA_GET) (
    P_ANY handle,
    _InVal_     LINEST_COLOFF colID,
    _InVal_     LINEST_ROWOFF row);

typedef /*_Check_return_*/ STATUS (* P_PROC_LINEST_DATA_PUT) (
    P_ANY handle,
    _InVal_     LINEST_COLOFF colID,
    _InVal_     LINEST_ROWOFF row,
    _InRef_     PC_F64 value);

/*
function declarations
*/

extern S32
linest(
    P_PROC_LINEST_DATA_GET p_proc_get,
    P_PROC_LINEST_DATA_PUT p_proc_put,
    P_ANY client_handle,
    _InVal_     U32 ext_m   /* number of independent x variables */,
    _InVal_     U32 n       /* number of data points */);

#endif /* __mathxtr2_h */

/* end of mathxtr2.h */
