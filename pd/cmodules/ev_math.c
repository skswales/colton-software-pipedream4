/* ev_math.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Semantic routines for evaluator */

/* MRJC May 1991 */

#include "common/gflags.h"

#include "handlist.h"
#include "cmodules/ev_eval.h"

#include "cmodules/mathxtra.h"
#include "cmodules/mathxtr2.h"

/* local header file */
#include "ev_evali.h"

#include <math.h>
#include <float.h>
#include <errno.h>

/*
internal functions
*/

_Check_return_
static STATUS
err_from_errno(void)
{
    switch(errno)
    {
    default:        return(STATUS_OK);
    case EDOM:      return(EVAL_ERR_ARGRANGE);
    case ERANGE:    return(EVAL_ERR_OUTOFRANGE);
    }
}

/******************************************************************************
*
* acosh(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_acosh)
{
    const F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, mx_acosh(number));

    /* input less than 1 invalid */
    /* large positive input causes overflow ***in current algorithm*** */
    if(errno /* == EDOM, ERANGE */)
        ev_data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* acosec(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_acosec)
{
    const F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, mx_acosec(number));

    /* input of magnitude less than 1 invalid */
    if(errno /* == EDOM */)
        ev_data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* acosech(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_acosech)
{
    const F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, mx_acosech(number));

    /* input of zero invalid */
    /* input of magnitude near zero causes overflow */
    if(errno /* == EDOM, ERANGE */)
        ev_data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* acot(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_acot)
{
    const F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    ev_data_set_real(p_ev_data_res, mx_acot(number));

    /* no error cases */
}

/******************************************************************************
*
* acoth(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_acoth)
{
    const F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, mx_acoth(number));

    /* input of magnitude less than 1 invalid */
    /* input of magnitude near 1 causes overflow */
    if(errno /* == EDOM */)
        ev_data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* asec(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_asec)
{
    const F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, mx_asec(number));

    /* input of values less than 1 invalid */
    if(errno /* == EDOM */)
        ev_data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* asech(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_asech)
{
    const F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, mx_asech(number));

    /* negative input or positive values greater than one invalid */
    /* input of zero or small positive value causes overflow */
    if(errno /* == EDOM, ERANGE */)
        ev_data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* asinh(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_asinh)
{
    const F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    ev_data_set_real(p_ev_data_res, mx_asinh(number));

    /* no error case */
}

/******************************************************************************
*
* atan_2(a, b)
*
******************************************************************************/

PROC_EXEC_PROTO(c_atan_2)
{
    const F64 a = args[0]->arg.fp;
    const F64 b = args[1]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, atan2(b, a));

    /* both input args zero? */
    if(errno /* == EDOM */)
        ev_data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* atanh(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_atanh)
{
    const F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, mx_atanh(number));

    /* input of magnitude 1 or greater invalid */
    /* input of near magnitude 1 causes overflow */
    if(errno /* == EDOM, ERANGE */)
        ev_data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* cosh(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_cosh)
{
    const F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, cosh(number));

    /* large magnitude input causes overflow */
    if(errno /* == ERANGE */)
        ev_data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* cosec(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_cosec)
{
    const F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, mx_cosec(number));

    /* various periodic input yields infinity or overflows */
    /* large magnitude input yields imprecise value */
    if(errno /* == ERANGE */)
        ev_data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* cosech(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_cosech)
{
    const F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, mx_cosech(number));

    /* zero | small magnitude input -> infinity | overflows */
    if(errno /* == EDOM, ERANGE */)
        ev_data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* cot(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_cot)
{
    const F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, mx_cot(number));

    /* various periodic input yields infinity or overflows */
    /* large magnitude input yields imprecise value */
    if(errno /* == ERANGE */)
        ev_data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* coth(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_coth)
{
    const F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, mx_coth(number));

    /* zero | small magnitude input -> infinity | overflows */
    if(errno /* == EDOM, ERANGE */)
        ev_data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* NUMBER sgn(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sgn)
{
    const F64 number = args[0]->arg.fp;
    S32 sgn_result;

    exec_func_ignore_parms();

    if(number > F64_MIN)
        sgn_result = 1;
    else if(number < -F64_MIN)
        sgn_result = -1;
    else
        sgn_result = 0;

    ev_data_set_integer(p_ev_data_res, sgn_result);
}

/******************************************************************************
*
* sec(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sec)
{
    const F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, mx_sec(number));

    /* various periodic input yields infinity or overflows */
    /* large magnitude input yields imprecise value */
    if(errno /* == ERANGE */)
        ev_data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* sech(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sech)
{
    const F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    ev_data_set_real(p_ev_data_res, mx_sech(number));

    /* no error cases */
}

/******************************************************************************
*
* sinh(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sinh)
{
    const F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, sinh(number));

    /* large magnitude input causes overflow */
    if(errno /* == ERANGE */)
        ev_data_set_error(p_ev_data_res, err_from_errno());
}

/******************************************************************************
*
* tanh(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_tanh)
{
    const F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    ev_data_set_real(p_ev_data_res, tanh(number));

    /* no error cases */
}

/******************************************************************************
*
*  Matrices
*
******************************************************************************/

/* M[j,k] = mat[n*j+k] - data stored by cols across row, then cols across next row ... */

#define _em(_array, _n_cols_in_row, _row_idx, _col_idx) ( \
    (_array) + (_row_idx) * (_n_cols_in_row))[_col_idx]

/* Conventionally, the entry in the (one-based)
 * i-th row and the j-th column of a matrix is referred to
 * as the i,j, (i,j), or (i,j)th entry of the matrix
 * Easiest for C to keep zero-based...
 */

#define _Aij(_array, _n_cols_in_row, _i_idx, _j_idx) \
    _em(_array, _n_cols_in_row, _i_idx, _j_idx)

/* So watch out when using array_range_index() to fill and
 * ss_array_element_index_borrow() for result as those use x=col, y=row args
 */

/******************************************************************************
*
* evaluate the determinant of an m-square matrix
*
******************************************************************************/

_Check_return_
static STATUS
determinant(
    _In_count_x_(m*m) PC_F64 ap /*[m][m]*/,
    _InVal_     U32 m,
    _OutRef_    P_F64 dp)
{
    STATUS status = STATUS_OK;

    switch(m)
    {
    case 0:
        *dp = 1.0;
        break; /* not as silly a question as rjm first thought! (see Bloom, Linear Algebra and Geometry) */

    case 1:
        *dp = *ap;
        break;

    case 2:
#define a11 (_Aij(ap, 2, 0, 0))
#define a12 (_Aij(ap, 2, 0, 1))
#define a21 (_Aij(ap, 2, 1, 0))
#define a22 (_Aij(ap, 2, 1, 1))
        *dp = (a11 * a22) - (a12 * a21);
#undef a11
#undef a12
#undef a21
#undef a22
        break;

    case 3:
#define a11 (_Aij(ap, 3, 0, 0))
#define a12 (_Aij(ap, 3, 0, 1))
#define a13 (_Aij(ap, 3, 0, 2))
#define a21 (_Aij(ap, 3, 1, 0))
#define a22 (_Aij(ap, 3, 1, 1))
#define a23 (_Aij(ap, 3, 1, 2))
#define a31 (_Aij(ap, 3, 2, 0))
#define a32 (_Aij(ap, 3, 2, 1))
#define a33 (_Aij(ap, 3, 2, 2))
        *dp = + a11 * (a22 * a33 - a23 * a32)
              - a12 * (a21 * a33 - a23 * a31)
              + a13 * (a21 * a32 - a22 * a31);
#undef a11
#undef a12
#undef a13
#undef a21
#undef a22
#undef a23
#undef a31
#undef a32
#undef a33
        break;

    default:
        { /* recurse evaluating minors */
        P_F64 minor;
        U32 minor_m = m - 1;
        F64 minor_D;
        U32 col_idx, i_col_idx, o_col_idx;
        U32 i_row_idx, o_row_idx;

        *dp = 0.0;

        if(NULL == (minor = al_ptr_alloc_elem(F64, minor_m * minor_m, &status)))
            return(status); /* unable to determine */

        for(col_idx = 0; col_idx < m; ++col_idx)
        {
            /* build minor by removing all of top row and current column */
            for(i_col_idx = o_col_idx = 0; o_col_idx < minor_m; ++i_col_idx, ++o_col_idx)
            {
                if(i_col_idx == col_idx)
                    ++i_col_idx;

                for(i_row_idx = 1, o_row_idx = 0; o_row_idx < minor_m; ++i_row_idx, ++o_row_idx)
                {
                    _em(minor, minor_m, o_row_idx, o_col_idx) = _em(ap, m, i_row_idx, i_col_idx);
                }
            }

            status_break(status = determinant(minor, minor_m, &minor_D));

            if(col_idx & 1)
                minor_D = -minor_D; /* make into cofactor */

            *dp += (_em(ap, m, 0, col_idx) * minor_D);
        }

        al_ptr_dispose(P_P_ANY_PEDANTIC(&minor));

        break;
        }
    }

    return(status);
}

/*
* NUMBER m_determ(square-array)
*/

PROC_EXEC_PROTO(c_m_determ)
{
    F64 m_determ_result;
    S32 xs, ys;

    exec_func_ignore_parms();

    /* get x and y sizes */
    array_range_sizes(args[0], &xs, &ys);

    if(xs != ys)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_MATRIX_NOT_SQUARE);
        return;
    }

    if(xs == 0)
    {
        ev_data_set_integer(p_ev_data_res, 1); /* yes, really */
        return;
    }

    {
    STATUS status = STATUS_OK;
    S32 m = xs;
    F64 nums[3*3]; /* don't really need to allocate for small ones */
    P_F64 a /*[m][m]*/;

    if(m <= 3)
        a = nums;
    else
        a = al_ptr_alloc_bytes(P_F64, m * (m * sizeof32(*a)), &status);

    if(NULL != a)
    {
        S32 i, j;

        assert(&_Aij(a, m, m - 1, m - 1) + 1 == &a[m * m]);

        /* load up the matrix (a) */
        for(i = 0; i < m; ++i)
        {
            for(j = 0; j < m; ++j)
            {
                EV_DATA ev_data;

                if(array_range_index(&ev_data, args[0], j, i, EM_REA) != RPN_DAT_REAL) /* NB j,i */
                    status_break(status = EVAL_ERR_MATRIX_NOT_NUMERIC);

                _Aij(a, m, i, j) = ev_data.arg.fp;
            }
        }

        if(status_ok(status = determinant(a, m, &m_determ_result)))
        {
            ev_data_set_real(p_ev_data_res, m_determ_result);
            real_to_integer_try(p_ev_data_res);
        }

        if(a != nums)
            al_ptr_dispose(P_P_ANY_PEDANTIC(&a));
    }

    if(status_fail(status))
    {
        ss_data_free_resources(p_ev_data_res);
        ev_data_set_error(p_ev_data_res, status);
    }
    } /*block*/
}

/*
inverse of square matrix A is 1/det * adjunct(A)
*/

PROC_EXEC_PROTO(c_m_inverse)
{
    STATUS status = STATUS_OK;
    S32 xs, ys;
    U32 m, minor_m;
    P_F64 a /*[m][m]*/ = NULL;
    P_F64 adj /*[m][m]*/ = NULL;
    P_F64 minor /*[minor_m][minor_m]*/ = NULL;
    U32 i, j;
    F64 D, minor_D;

    exec_func_ignore_parms();

    /* get x and y sizes */
    array_range_sizes(args[0], &xs, &ys);

    if(xs != ys)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_MATRIX_NOT_SQUARE);
        return;
    }

    m = xs;

    if(status_fail(ss_array_make(p_ev_data_res, m, m)))
        return;

    if(NULL == (a = al_ptr_alloc_bytes(P_F64, m * (m * sizeof32(*a)), &status)))
        goto endpoint;

    assert(&_Aij(a, m, m - 1, m - 1) + 1 == &a[m * m]);

    if(NULL == (adj = al_ptr_alloc_bytes(P_F64, m * (m * sizeof32(*adj)), &status)))
        goto endpoint;

    /* load up the matrix (a) */
    for(i = 0; i < m; ++i)
    {
        for(j = 0; j < m; ++j)
        {
            EV_DATA ev_data;

            if(array_range_index(&ev_data, args[0], j, i, EM_REA) != RPN_DAT_REAL) /* NB j,i */
            {
                status = EVAL_ERR_MATRIX_NOT_NUMERIC;
                goto endpoint;
            }

            _Aij(a, m, i, j) = ev_data.arg.fp;
        }
    }

    if(status_fail(status = determinant(a, m, &D)))
        goto endpoint;

    if(0.0 == D)
    {
        status = EVAL_ERR_MATRIX_SINGULAR;
        goto endpoint;
    }

    /* compute adjunct(A) */

    /* step 1 - compute matrix of minors */
    minor_m = m - 1;

    if(NULL == (minor = al_ptr_alloc_bytes(P_F64, minor_m * (minor_m * sizeof32(*minor)), &status)))
        goto endpoint;

    for(i = 0; i < m; ++i)
    {
        for(j = 0; j < m; ++j)
        {
            /* build minor(i,j) by removing all of current row and current column */
            U32 in_i, in_j, out_i, out_j;

            for(in_i = 0; in_i < m; ++in_i)
            {
                if(in_i == i) continue;

                out_i = in_i;

                if(out_i > i) --out_i;

                for(in_j = 0; in_j < m; ++in_j)
                {
                    if(in_j == j) continue;

                    out_j = in_j;

                    if(out_j > j) --out_j;

                    _Aij(minor, minor_m, out_i, out_j) = _Aij(a, m, in_i, in_j);
                }
            }

            if(status_fail(status = determinant(minor, minor_m, &minor_D)))
                goto endpoint;

            _Aij(adj, m, i, j) = minor_D;
       }
    }

    /* step 2 - convert to matrix of cofactors (multiply by +/- checkerboard) */
    for(i = 0; i < m; ++i)
    {
        for(j = 0; j < m; ++j)
        {
            if(0 != ((i & 1) ^ (j & 1)))
                _Aij(adj, m, i, j) = - _Aij(adj, m, i, j);
        }
    }

    /* step 3 - transpose the matrix of cofactors => adjunct(A) (simple to do this in formation of result) */

    /* copy out the inverse(A) == adjunct(A) / determinant(A) */
    for(i = 0; i < m; ++i)
    {
        for(j = 0; j < m; ++j)
        {
            P_EV_DATA p_ev_data = ss_array_element_index_wr(p_ev_data_res, j, i); /* NB j,i */
            ev_data_set_real(p_ev_data, _Aij(adj, m, j, i) / D); /* NB j,i here too for transpose step 3 above */
        }
    }

endpoint:

    al_ptr_dispose(P_P_ANY_PEDANTIC(&minor));
    al_ptr_dispose(P_P_ANY_PEDANTIC(&adj));
    al_ptr_dispose(P_P_ANY_PEDANTIC(&a));

    if(status_fail(status))
    {
        ss_data_free_resources(p_ev_data_res);
        ev_data_set_error(p_ev_data_res, status);
    }
}

/******************************************************************************
*
*  Matrix Multiply
*
*  arg[0] is of dimensions a*b
*
*  arg[1] is of dimensions b*c
*
*  return matrix of dimensions a*c
*
******************************************************************************/

PROC_EXEC_PROTO(c_m_mult)
{
    S32 x1s, x2s, y1s, y2s;

    exec_func_ignore_parms();

    /* check it is an array and get x and y sizes */
    array_range_sizes(args[0], &x1s, &y1s);
    array_range_sizes(args[1], &x2s, &y2s);

    if(x1s != y2s)
        {
        /* whinge about dimensions */
        ev_data_set_error(p_ev_data_res, EVAL_ERR_MISMATCHED_MATRICES);
        return;
        }

    if(status_ok(ss_array_make(p_ev_data_res, x2s, y1s)))
        {
        S32 x, y;

        for(x = 0; x < x2s; x++)
            {
            for(y = 0; y < y1s; y++)
                {
                F64 product = 0.0;
                S32 elem;
                P_EV_DATA elep;

                for(elem = 0; elem < x1s; elem++)
                    {
                    EV_DATA data1, data2;

                    array_range_index(&data1, args[0], elem, y, EM_REA);
                    array_range_index(&data2, args[1], x, elem, EM_REA);

                    if(data1.did_num == RPN_DAT_REAL && data2.did_num == RPN_DAT_REAL)
                        product += data1.arg.fp * data2.arg.fp;
                    else
                        {
                        ss_data_free_resources(p_ev_data_res);
                        ev_data_set_error(p_ev_data_res, EVAL_ERR_MATRIX_NOT_NUMERIC);
                        return;
                        }
                    }

                elep = ss_array_element_index_wr(p_ev_data_res, x, y);
                ev_data_set_real(elep, product);
                real_to_integer_try(elep);
                }
            }
        }
}

PROC_EXEC_PROTO(c_transpose)
{
    S32 xs, ys;
    S32 x, y;

    exec_func_ignore_parms();

    /* get x and y sizes */
    array_range_sizes(args[0], &xs, &ys);

    /* make a y-dimension by x-dimension result array and swap elements */
    if(status_ok(ss_array_make(p_ev_data_res, ys, xs)))
        {
        for(x = 0; x < xs; x++)
            {
            for(y = 0; y < ys; y++)
                {
                EV_DATA temp_data;
                array_range_index(&temp_data, args[0], x, y, EM_ANY);
                status_assert(ss_data_resource_copy(ss_array_element_index_wr(p_ev_data_res, y, x), &temp_data));
                }
            }
        }
}

/******************************************************************************
*
* GROWTH(logest_result_data, known_x's) is pretty trivial
*
******************************************************************************/

PROC_EXEC_PROTO(c_growth)
{
    S32 x_vars;
    S32 x, y;
    S32 err = 0;

    exec_func_ignore_parms();

    array_range_sizes(args[0], &x, &y);

    x_vars = x - 1;

    if( (x < 2  /*at least c,m*/)  ||
        ((y != 1 /*no stats*/)  &&
         (y != 3 /*stats*/   )  )  )
        {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_MATRIX_WRONG_SIZE);
        return;
        }

    array_range_sizes(args[1], &x, &y);

    /* SKS after 4.12 28apr92 - same reason as TREND() update */
    if(x == x_vars)
        {
        if(status_ok(ss_array_make(p_ev_data_res, 1, y)))
            {
            S32 row;

            for(row = 0; row < y; ++row)
                {
                EV_DATA x_data;
                EV_DATA a_data;
                F64 product; /* NB. product computed carefully using logs */
                S32 ci;
                BOOL negative;
                P_EV_DATA elep;

                /* start with the constant */
                array_range_index(&a_data, args[0],
                                  0, /* NB!*/
                                  0,
                                  EM_REA);

                if(a_data.did_num != RPN_DAT_REAL)
                    status_break(err = EVAL_ERR_MATRIX_NOT_NUMERIC);

                errno = 0;

                /* if initial y data was all -ve then this is a possibility ... */
                negative = (a_data.arg.fp < 0);

                product = fabs(a_data.arg.fp);

                if(product != 0.0)
                    {
                    product = log(product);
                    assert(errno == 0); /* log(+ve) cannot fail ho ho */

                    /* loop across a row multiplying product by coefficients ^ x variables*/
                    for(ci = 0; ci < x_vars; ++ci)
                        {
                        array_range_index(&a_data, args[0],
                                          ci + 1, /* NB. skip constant! */
                                          0,
                                          EM_REA);

                        if(a_data.did_num != RPN_DAT_REAL)
                            status_break(err = EVAL_ERR_MATRIX_NOT_NUMERIC);

                        array_range_index(&x_data, args[1],
                                          ci, /* NB. extract from nth col ! */
                                          row,
                                          EM_REA);

                        if(x_data.did_num != RPN_DAT_REAL)
                            status_break(err = EVAL_ERR_MATRIX_NOT_NUMERIC);

                        product += log(a_data.arg.fp) * x_data.arg.fp;
                        }

                    if(errno /* == EDOM, ERANGE */)
                        err = EVAL_ERR_BAD_LOG;

                    status_break(err);

                    product = exp(product); /* convert sum of products into a0 * PI(ai ** xi) */

                    if(product == HUGE_VAL) /* don't test for underflow case */
                        status_break(err = EVAL_ERR_OUTOFRANGE);
                    }

                if(negative)
                    product = -product;

                elep = ss_array_element_index_wr(p_ev_data_res, 0, row);
                ev_data_set_real(elep, product);
                }

            } /*fi*/
        }
    else if(y == x_vars)
        {
        if(status_ok(ss_array_make(p_ev_data_res, x, 1)))
            {
            S32 col;

            for(col = 0; col < x; ++col)
                {
                EV_DATA x_data;
                EV_DATA a_data;
                F64 product; /* NB. product computed carefully using logs */
                S32 ci;
                BOOL negative;
                P_EV_DATA elep;

                /* start with the constant */
                array_range_index(&a_data, args[0],
                                  0, /* NB!*/
                                  0,
                                  EM_REA);

                if(a_data.did_num != RPN_DAT_REAL)
                    status_break(err = EVAL_ERR_MATRIX_NOT_NUMERIC);

                errno = 0;

                /* if initial y data was all -ve then this is a possibility ... */
                negative = (a_data.arg.fp < 0);

                product = fabs(a_data.arg.fp);

                if(product != 0.0)
                    {
                    product = log(product);
                    assert(errno == 0); /* log(+ve) cannot fail ho ho */

                    /* loop multiplying product by coefficients ^ x variables*/
                    for(ci = 0; ci < x_vars; ++ci)
                        {
                        array_range_index(&a_data, args[0],
                                          ci + 1, /* NB. skip constant! */
                                          0,
                                          EM_REA);

                        if(a_data.did_num != RPN_DAT_REAL)
                            status_break(err = EVAL_ERR_MATRIX_NOT_NUMERIC);

                        array_range_index(&x_data, args[1],
                                          col,
                                          ci, /* NB. extract from nth row ! */
                                          EM_REA);

                        if(x_data.did_num != RPN_DAT_REAL)
                            status_break(err = EVAL_ERR_MATRIX_NOT_NUMERIC);

                        product += log(a_data.arg.fp) * x_data.arg.fp;
                        }

                    if(errno /* == EDOM, ERANGE */)
                        err = EVAL_ERR_BAD_LOG;

                    status_break(err);

                    product = exp(product); /* convert sum of products into a0 * PI(ai ** xi) */

                    if(product == HUGE_VAL) /* don't test for underflow case */
                        status_break(err = EVAL_ERR_OUTOFRANGE);
                    }

                if(negative)
                    product = -product;

                elep = ss_array_element_index_wr(p_ev_data_res, col, 0);
                ev_data_set_real(elep, product);
                }

            } /*fi*/
        }
    else
        {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_MATRIX_WRONG_SIZE);
        return;
        }

    if(status_fail(err))
        {
        ss_data_free_resources(p_ev_data_res);
        ev_data_set_error(p_ev_data_res, err);
        }
}

/******************************************************************************
*
* TREND(linest_result_data, known_x's) is pretty trivial
*
******************************************************************************/

PROC_EXEC_PROTO(c_trend)
{
    S32 x_vars;
    S32 x, y;
    S32 err = 0;

    exec_func_ignore_parms();

    array_range_sizes(args[0], &x, &y);

    x_vars = x - 1;

    if( (x < 2  /*at least c,m*/)  ||
        ((y != 1 /*no stats*/)  &&
         (y != 3 /*stats*/  )   )  )
        {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_MATRIX_WRONG_SIZE);
        return;
        }

    array_range_sizes(args[1], &x, &y);

    /* SKS after 4.12 28apr92 - allow TREND() to receive data
     * in untransposed form ie. more naturally matched to linest data
    */
    if(x == x_vars)
        {
        if(status_ok(ss_array_make(p_ev_data_res, 1, y)))
            {
            S32 row;

            for(row = 0; row < y; ++row)
                {
                EV_DATA x_data;
                EV_DATA a_data;
                F64 sum;
                S32 ci;
                P_EV_DATA elep;

                /* start with the constant */
                array_range_index(&a_data, args[0],
                                  0, /* NB!*/
                                  0,
                                  EM_REA);

                if(a_data.did_num != RPN_DAT_REAL)
                    status_break(err = EVAL_ERR_MATRIX_NOT_NUMERIC);

                sum = a_data.arg.fp;

                /* loop across a row summing coefficients * x variables */
                for(ci = 0; ci < x_vars; ++ci)
                    {
                    array_range_index(&a_data, args[0],
                                      ci + 1, /* NB. skip constant! */
                                      0,
                                      EM_REA);

                    if(a_data.did_num != RPN_DAT_REAL)
                        status_break(err = EVAL_ERR_MATRIX_NOT_NUMERIC);

                    array_range_index(&x_data, args[1],
                                      ci, /* NB. extract from nth col! */
                                      row,
                                      EM_REA);

                    if(x_data.did_num != RPN_DAT_REAL)
                        status_break(err = EVAL_ERR_MATRIX_NOT_NUMERIC);

                    sum += a_data.arg.fp * x_data.arg.fp;
                    }

                status_break(err);

                elep = ss_array_element_index_wr(p_ev_data_res, 0, row);
                ev_data_set_real(elep, sum);
                }
            } /*fi*/
        }
    else if(y == x_vars)
        {
        if(status_ok(ss_array_make(p_ev_data_res, x, 1)))
            {
            S32 col;

            for(col = 0; col < x; ++col)
                {
                EV_DATA x_data;
                EV_DATA a_data;
                F64 sum;
                S32 ci;
                P_EV_DATA elep;

                /* start with the constant */
                array_range_index(&a_data, args[0],
                                  0, /* NB!*/
                                  0,
                                  EM_REA);

                if(a_data.did_num != RPN_DAT_REAL)
                    status_break(err = EVAL_ERR_MATRIX_NOT_NUMERIC);

                sum = a_data.arg.fp;

                /* loop down a column summing coefficients * x variables */
                for(ci = 0; ci < x_vars; ++ci)
                    {
                    array_range_index(&a_data, args[0],
                                      ci + 1, /* NB. skip constant! */
                                      0,
                                      EM_REA);

                    if(a_data.did_num != RPN_DAT_REAL)
                        status_break(err = EVAL_ERR_MATRIX_NOT_NUMERIC);

                    array_range_index(&x_data, args[1],
                                      col,
                                      ci, /* NB. extract from nth row! */
                                      EM_REA);

                    if(x_data.did_num != RPN_DAT_REAL)
                        status_break(err = EVAL_ERR_MATRIX_NOT_NUMERIC);

                    sum += a_data.arg.fp * x_data.arg.fp;
                    }

                status_break(err);

                elep = ss_array_element_index_wr(p_ev_data_res, col, 0);
                ev_data_set_real(elep, sum);
                }

            } /*fi*/
        }
    else
        {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_MATRIX_WRONG_SIZE);
        return;
        }

    if(status_fail(err))
        {
        ss_data_free_resources(p_ev_data_res);
        ev_data_set_error(p_ev_data_res, err);
        }
}

/******************************************************************************
*
* linest(known_y's [, known_x's [, stats [, known_a's [, known_ye's ]]]])
*
* result is an array of {estimation parameters[;
*                        estimated errors for the above;
*                        chi-squared]}
*
******************************************************************************/

typedef struct FOR_FIRST_LINEST
{
    PC_F64 x; /* const */
    S32 x_vars;
    PC_F64 y; /* const */
    P_F64 a;
}
FOR_FIRST_LINEST, * P_FOR_FIRST_LINEST;

PROC_LINEST_DATA_GET_PROTO(static, ligp, client_handle, colID, row)
{
    P_FOR_FIRST_LINEST lidatap = (P_FOR_FIRST_LINEST) client_handle;

    switch(colID)
        {
        case LINEST_A_COLOFF:
            assert0();
            return(0.0);

        case LINEST_Y_COLOFF:
            return(lidatap->y[row]);

        default:
            {
            S32 coloff = colID - LINEST_X_COLOFF;
            return((lidatap->x + row * lidatap->x_vars) [coloff]);
            }
        }
}

PROC_LINEST_DATA_PUT_PROTO(static, lipp, client_handle, colID, row, value)
{
    P_FOR_FIRST_LINEST lidatap = (P_FOR_FIRST_LINEST) client_handle;

    switch(colID)
        {
        case LINEST_A_COLOFF:
            lidatap->a[row] = *value;
            break;

        default: default_unhandled(); break;
        }

    return(STATUS_DONE);
}

typedef struct LINEST_ARRAY
{
    P_F64 val;
    S32 rows;
    S32 cols;
}
LINEST_ARRAY;

PROC_EXEC_PROTO(c_linest)
{
    static const LINEST_ARRAY empty = { NULL, 0, 0 };

    LINEST_ARRAY known_y;
    LINEST_ARRAY known_x;
    S32 stats;
    LINEST_ARRAY known_a;
    LINEST_ARRAY known_e;
    LINEST_ARRAY result_a;
    S32 data_in_cols;
    S32 y_items;
    S32 x_vars;
    STATUS status = STATUS_OK;
    FOR_FIRST_LINEST lidata;

    exec_func_ignore_parms();

    stats    = 0;

    known_y  = empty;
    known_x  = empty;
    known_a  = empty;
    known_e  = empty;
    result_a = empty;

    switch(nargs)
        {
        default:
        case 5:
            /* check known_ye's is an array and get x and y sizes */
            array_range_sizes(args[4], &known_e.cols, &known_e.rows);

            /*FALLTHRU*/

        case 4:
            /* check known_a's is an array and get x and y sizes */
            array_range_sizes(args[3], &known_a.cols, &known_a.rows);

            /*FALLTHRU*/

        case 3:
             stats = (args[2]->arg.fp != 0.0);

            /*FALLTHRU*/

        case 2:
            /* check known_x's is an array and get x and y sizes */
            array_range_sizes(args[1], &known_x.cols, &known_x.rows);

            /*FALLTHRU*/

        case 1:
            /* check known_y's is an array and get x and y sizes */
            array_range_sizes(args[0], &known_y.cols, &known_y.rows);

            /* y data usually in a column, but allow for a row */
            data_in_cols = (known_y.rows > known_y.cols);
            y_items = data_in_cols ? known_y.rows : known_y.cols;
            break;
        }

    /* number of independent x variables (usually separate columns,
     * but can be separate rows if y data is so arranged)
    */
    if(nargs < 2)
        /* we always invent x if not supplied */
        x_vars = 1;
    else
        x_vars = (data_in_cols) ? known_x.cols : known_x.rows;

    /* simple to allocate y and x arrays together */
    if(NULL == (known_y.val = al_ptr_alloc_elem(F64, (U32) y_items * ((U32) x_vars + 1), &status)))
        goto endlabel;
    known_x.val = &known_y.val[y_items];

    result_a.cols = x_vars + 1;
    result_a.rows = stats ? 3 : 1;
    if(NULL == (result_a.val = al_ptr_alloc_elem(F64, result_a.cols * (U32) result_a.rows, &status)))
        goto endlabel;

    if(known_a.cols != 0)
        {
        if(known_a.cols > result_a.cols)
            {
            status = EVAL_ERR_MISMATCHED_MATRICES;
            goto endlabel;
            }

        if(NULL == (known_a.val = al_ptr_alloc_elem(F64, result_a.cols, &status)))
            goto endlabel;
        }

    if(known_e.rows != 0)
        {
        /* neatly covers both arrangements of y,e */
        if((known_e.rows > known_y.rows) || (known_e.rows > known_y.rows))
            {
            status = EVAL_ERR_MISMATCHED_MATRICES;
            goto endlabel;
            }

        if(NULL == (known_e.val = al_ptr_alloc_elem(F64, y_items, &status)))
            goto endlabel;
        }

    if(status_ok(ss_array_make(p_ev_data_res, result_a.cols, result_a.rows)))
        {
        S32     i, j;
        EV_DATA data;

        /* copy y data into working array */
        for(i = 0; i < y_items; ++i)
            {
            array_range_index(&data, args[0],
                              (data_in_cols) ? 0 : i,
                              (data_in_cols) ? i : 0,
                              EM_REA);

            if(data.did_num != RPN_DAT_REAL)
                {
                status = EVAL_ERR_MATRIX_NOT_NUMERIC;
                goto endlabel;
                }

            known_y.val[i] = data.arg.fp;
            }

        /* copy x data into working array */
        for(i = 0; i < y_items; ++i)
            {
            if(nargs > 1)
                for(j = 0; j < x_vars; ++j)
                    {
                    array_range_index(&data, args[1],
                                      (data_in_cols) ? j : i,
                                      (data_in_cols) ? i : j,
                                      EM_REA);

                    if(data.did_num != RPN_DAT_REAL)
                        {
                        status = EVAL_ERR_MATRIX_NOT_NUMERIC;
                        goto endlabel;
                        }

                    (known_x.val + i * x_vars)[j] = data.arg.fp;
                    }
            else
                known_x.val[i] = (F64) i + 1.0; /* make simple { 1.0, 2.0, 3.0 ... } */
            }

        /* <<< ignore the a data for the mo */
        if(known_a.val)
            { /*EMPTY*/ }

        /* copy (possibly partial) ye data into working array */
        if(known_e.val)
            for(i = 0; i < y_items; ++i)
                {
                if(i < ((data_in_cols) ? known_e.rows : known_e.cols))
                    {
                    array_range_index(&data, args[4],
                                      (data_in_cols) ? 0 : i,
                                      (data_in_cols) ? i : 0,
                                      EM_REA);

                    if(data.did_num != RPN_DAT_REAL)
                        {
                        status = EVAL_ERR_MATRIX_NOT_NUMERIC;
                        goto endlabel;
                        }

                    known_e.val[i] = data.arg.fp;
                    }
                else
                    known_e.val[i] = 1.0;
                }

        /* and then ignore it! */

/* bodgey bit from here ... */

        lidata.x = known_x.val;
        lidata.x_vars = x_vars;

        lidata.y = known_y.val;

        lidata.a = result_a.val;

        if(status_fail(status = linest(ligp, lipp, (CLIENT_HANDLE) &lidata, x_vars, y_items)))
            goto endlabel;

/* ... to here */

        /* copy result a data from working array */

        for(j = 0; j < result_a.cols; ++j)
            {
            F64 res;
            P_EV_DATA elep;

            res = (result_a.val + 0 * result_a.cols)[j];

            elep = ss_array_element_index_wr(p_ev_data_res, j, 0); /* NB j,i */
            ev_data_set_real(elep, res);

            if(stats)
                {
                /* estimated estimation errors */
#if 1 /* until yielded */
                res = 0;
#else
                res = (result_a.val + 1 * result_a.cols)[j];
#endif

                elep = ss_array_element_index_wr(p_ev_data_res, j, 1); /* NB j,i */
                ev_data_set_real(elep, res);
                }
            }

        if(stats)
            {
            /* chi-squared */
            F64 chi_sq;
            P_EV_DATA elep;

            chi_sq = 0.0;

            elep = ss_array_element_index_wr(p_ev_data_res, 0, 2); /* NB j,i */
            ev_data_set_real(elep, chi_sq);
            }

        } /*fi*/

endlabel:;

    if(status_fail(status))
        {
        ss_data_free_resources(p_ev_data_res);
        ev_data_set_error(p_ev_data_res, status);
        }

    al_ptr_dispose(P_P_ANY_PEDANTIC(&known_e.val));
    al_ptr_dispose(P_P_ANY_PEDANTIC(&known_a.val));
    al_ptr_dispose(P_P_ANY_PEDANTIC(&result_a.val));
    al_ptr_dispose(P_P_ANY_PEDANTIC(&known_y.val)); /* and x too */
}

/******************************************************************************
*
* oh god its logest. what a pathetic excuse of a routine
* when all you have to do is to tell the user to transform
* his data in an intelligent manner in the first place
* and use linest... I blame Excel personally
*
* it can't even fit y = k * m ^ -x !
*
******************************************************************************/

PROC_EXEC_PROTO(c_logest)
{
    EV_DATA y_data;
    EV_DATA a_data = { RPN_DAT_BLANK };
    EV_DATA ev_data;
    P_EV_DATA elep;
    P_EV_DATA targs0, targs3;
    F64 val;
    S32 x, y;
    S32 col, row;
    S32 err = 0;
    S32 sign = 0; /* not yet set */

    exec_func_ignore_parms();

    /* perform quick preprocess of a data, taking logs */

    if(nargs > 3)
        {
        array_range_sizes(args[0], &x, &y);

        if(args[3]->did_num == RPN_DAT_RANGE)
            {
            if(status_fail(ss_array_make(&a_data, x, y)))
                return;
            }
        else
            status_assert(ss_data_resource_copy(&a_data, args[3]));

        for(col = 0; col < x; ++col)
            {
            if(array_range_index(&ev_data, args[3], col, 0, EM_REA) != RPN_DAT_REAL)
                {
                err = EVAL_ERR_MATRIX_NOT_NUMERIC;
                goto endlabel2;
                }

            val = ev_data.arg.fp;
            /* do sign forcing before y transform */
            if(col == 0)
                sign = (val < 0) ? -1 : +1;
            ev_data.arg.fp = log(val);

            *ss_array_element_index_wr(&a_data, col, 0) = ev_data;
            }
        }

    /* perform quick preprocess of y data, taking logs */

    array_range_sizes(args[0], &x, &y);

    if(args[0]->did_num == RPN_DAT_RANGE)
        {
        if(status_fail(ss_array_make(&y_data, x, y)))
            return;
        }
    else
        status_assert(ss_data_resource_copy(&y_data, args[0]));

    /* simple layout-independent transform of y data */
    for(col = 0; col < x; ++col)
        for(row = 0; row < y; ++row)
            {
            if(array_range_index(&ev_data, args[0], col, row, EM_REA) != RPN_DAT_REAL)
                {
                err = EVAL_ERR_MATRIX_NOT_NUMERIC;
                goto endlabel;
                }

            if(ev_data.arg.fp < 0.0)
                {
                if(!sign)
                    sign = -1;
                else if(sign != -1)
                    {
                    err = EVAL_ERR_MIXED_SIGNS;
                    goto endlabel;
                    }

                ev_data.arg.fp = log(-ev_data.arg.fp);
                }
            else if(ev_data.arg.fp > 0.0)
                {
                if(!sign)
                    sign = +1;
                else if(sign != +1)
                    {
                    err = EVAL_ERR_MIXED_SIGNS;
                    goto endlabel;
                    }

                ev_data.arg.fp = log(ev_data.arg.fp);
                }
            /* else 0.0 ... interesting case ... not necessarily wrong but very hard */

            /* poke back transformed data */
            elep = ss_array_element_index_wr(&y_data, col, row);
            ev_data_set_real(elep, ev_data.arg.fp);
            }

    /* ask mrjc whether this is legal */
    targs0  = args[0];
    args[0] = &y_data;

    if(nargs > 3)
        {
        targs3  = args[3];
        args[3] = &a_data;
        }
    else
        targs3  = NULL;

    /* call my friend to do the hard work */
    c_linest(args, nargs, p_ev_data_res, p_cur_slr);

    args[0] = targs0;

    if(targs3)
        args[3] = targs3;

    /* and transform first row of coefficients insitu using exp, and sign the constant if needed */

    if(p_ev_data_res->did_num == RPN_TMP_ARRAY)
        {
        array_range_sizes(p_ev_data_res, &x, &y);

        for(col = 0; col < x; ++col)
            {
            elep  = ss_array_element_index_wr(p_ev_data_res, col, 0);
            assert(elep->did_num == RPN_DAT_REAL);
            val   = exp(elep->arg.fp);
            if((col == 0) && (sign == -1))
                val = -val;
            elep->arg.fp = val;
            }
        }

endlabel:;

    ss_data_free_resources(&y_data);

endlabel2:;

    ss_data_free_resources(&a_data);

    if(err)
        ev_data_set_error(p_ev_data_res, err);
}

_Check_return_
extern double lgamma_r(double, int *);

_Check_return_
static F64
ln_fact(
    _InVal_     S32 n)
{
    int sign;
    return(lgamma_r(n + 1.0, &sign));
}

/* Beta(a,b) = Gamma(a) * Gamma(b) / Gamma(a+b) */

PROC_EXEC_PROTO(c_beta)
{
    STATUS err = STATUS_OK;
    const F64 z = args[0]->arg.fp;
    const F64 w = args[1]->arg.fp;
    F64 beta_result;

    exec_func_ignore_parms();

    errno = 0;

    {
    int sign;
    const F64 a = lgamma_r(z, &sign);
    const F64 b = lgamma_r(w, &sign);
    const F64 c = lgamma_r(z + w, &sign);
    beta_result = exp(a + b - c);
    } /*block*/

    ev_data_set_real(p_ev_data_res, beta_result);

    if(errno)
        err = err_from_errno();

    if(status_fail(err))
        ev_data_set_error(p_ev_data_res, err);
}

PROC_EXEC_PROTO(c_binom)
{
    /* SKS 02apr99 call my friend to do the hard work */
    c_combin(args, nargs, p_ev_data_res, p_cur_slr);
}

/******************************************************************************
*
* computes the product of all integers
* in the range start..end inclusive
* start, end assumed to be +ve, non-zero
*
******************************************************************************/

static void
product_between(
    _InoutRef_  P_EV_DATA p_ev_data_res,
    _InVal_     S32 start,
    _InVal_     S32 end)
{
    S32 i;

    if(RPN_DAT_REAL == p_ev_data_res->did_num)
        p_ev_data_res->arg.fp = start;
    else
        p_ev_data_res->arg.integer = start;

    for(i = start + 1; i <= end; ++i)
        {
        /* keep in integer where possible */
        if(RPN_DAT_REAL != p_ev_data_res->did_num)
            {
            UMUL64_RESULT result;

            umul64(p_ev_data_res->arg.integer, i, &result);

            if((0 == result.HighPart) && (0 == (result.LowPart >> 31)))
                { /* result still fits in integer */
                p_ev_data_res->arg.integer = (S32) result.LowPart;
                continue;
                }

            /* have to go to fp now result has outgrown integer and retry the multiply */
            ev_data_set_real(p_ev_data_res, (F64) p_ev_data_res->arg.integer);
            }

        p_ev_data_res->arg.fp *= i;
        }

    if(RPN_DAT_REAL != p_ev_data_res->did_num)
        p_ev_data_res->did_num = ev_integer_size(p_ev_data_res->arg.integer);
}

PROC_EXEC_PROTO(c_combin) /* nCk = n!/((n-k)!*k!) */
{
    STATUS err = STATUS_OK;
    const S32 n = args[0]->arg.integer;
    const S32 k = args[1]->arg.integer;

    exec_func_ignore_parms();

    errno = 0;

    if((n < 0) || (k < 0) || ((n - k) < 0))
        err = EVAL_ERR_ARGRANGE;
    else if(n <= 163) /* SKS 02apr99 bigger range */
        {
        EV_DATA ev_data_divisor;

        /* assume result will be integer to start with */
        p_ev_data_res->did_num = RPN_DAT_WORD32;

        /* function will go to fp as necessary */
        product_between(p_ev_data_res, n - k + 1, n);

        /* calculate divisor in same format as dividend
         * NB divisor is always smaller than dividend so this is ok
         */
        ev_data_divisor.did_num = p_ev_data_res->did_num;
        product_between(&ev_data_divisor, 1, k);

        if(RPN_DAT_REAL == p_ev_data_res->did_num)
            {
            assert(ev_data_divisor.did_num == p_ev_data_res->did_num);

            p_ev_data_res->arg.fp /= ev_data_divisor.arg.fp;

            /* combination always integer result - see if we can get one! */
            real_to_integer_try(p_ev_data_res);
            }
        else
            {
            /* no worries about remainders - combination always integer result! */
            p_ev_data_res->arg.integer /= ev_data_divisor.arg.integer;

            p_ev_data_res->did_num = ev_integer_size(p_ev_data_res->arg.integer);
            }
        }
    else
        {
        const F64 ln_numer = ln_fact(n);
        const F64 ln_denom = ln_fact(n - k) + ln_fact(k);

        ev_data_set_real(p_ev_data_res, exp(ln_numer - ln_denom));

        if(errno)
            err = err_from_errno();
        }

    if(status_fail(err))
        ev_data_set_error(p_ev_data_res, err);
}

PROC_EXEC_PROTO(c_fact)
{
    STATUS err = STATUS_OK;
    const S32 n = args[0]->arg.integer;

    exec_func_ignore_parms();

    errno = 0;

    if(n < 0)
        err = EVAL_ERR_ARGRANGE;
    else if(n <= 163) /* SKS 13nov96 bigger range */
        {
        /* assume result will be integer to start with */
        p_ev_data_res->did_num = RPN_DAT_WORD32;

        /* function will go to fp as necessary */
        product_between(p_ev_data_res, 1, n);
        }
    else
        {
        ev_data_set_real(p_ev_data_res, exp(ln_fact(n)));

        if(errno)
            err = err_from_errno();
        }

    if(status_fail(err))
        ev_data_set_error(p_ev_data_res, err);
}

PROC_EXEC_PROTO(c_gammaln)
{
    STATUS err = STATUS_OK;
    const F64 number = args[0]->arg.fp;
    F64 gammaln_result;

    exec_func_ignore_parms();

    errno = 0;

    {
    int sign;
    gammaln_result = lgamma_r(number, &sign);
    } /*block*/

    ev_data_set_real(p_ev_data_res, gammaln_result);

    if(errno)
        err = err_from_errno();

    if(status_fail(err))
        ev_data_set_error(p_ev_data_res, err);
}

PROC_EXEC_PROTO(c_permut) /* nPk = n!/(n-k)! */
{
    STATUS err = STATUS_OK;
    const S32 n = args[0]->arg.integer;
    const S32 k = args[1]->arg.integer;

    exec_func_ignore_parms();

    errno = 0;

    if((n < 0) || (k < 0) || ((n - k) < 0))
        err = EVAL_ERR_ARGRANGE;
    else if(n <= 163) /* SKS 02apr99 bigger range */
        {
        /* every number in the divisor term (n-k)!
         * is present in the dividend n! so cancels out
         * leaving just the product of the numbers from
         * n-k+1 to n to be calculated
         */

        /* assume result will be integer to start with */
        p_ev_data_res->did_num = RPN_DAT_WORD32;

        /* function will go to fp as necessary */
        product_between(p_ev_data_res, n - k + 1, n);
        }
    else
        {
        const F64 ln_numer = ln_fact(n);
        const F64 ln_denom = ln_fact(n - k);

        ev_data_set_real(p_ev_data_res, exp(ln_numer - ln_denom));

        if(errno)
            err = err_from_errno();
        }

    if(status_fail(err))
        ev_data_set_error(p_ev_data_res, err);
}

/* end of ev_math.c */
