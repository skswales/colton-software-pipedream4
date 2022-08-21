/* ev_math.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Mathematical and Matrix function routines for evaluator */

#include "common/gflags.h"

#include "handlist.h"
#include "cmodules/ev_eval.h"

#include "cmodules/mathxtra.h"

/* local header file */
#include "ev_evali.h"

#include <math.h>
#include <float.h>
#include <errno.h>

/*
internally exported functions
*/

/******************************************************************************
*
* computes the product of all integers
* in the range start..end inclusive
* start, end assumed to be +ve, non-zero
*
******************************************************************************/

extern void
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

_Check_return_
extern STATUS
status_from_errno(void)
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
* Mathematical functions
*
******************************************************************************/

/******************************************************************************
*
* NUMBER abs(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_abs)
{
    exec_func_ignore_parms();

    if(RPN_DAT_REAL == args[0]->did_num)
        ev_data_set_real(p_ev_data_res, fabs(args[0]->arg.fp));
    else
        ev_data_set_integer(p_ev_data_res, abs(args[0]->arg.integer));
}

/******************************************************************************
*
* REAL exp(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_exp)
{
    F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    ev_data_set_real(p_ev_data_res, exp(number));

    /* exp() overflowed? - don't test for underflow case */
    if(p_ev_data_res->arg.fp == HUGE_VAL)
        ev_data_set_error(p_ev_data_res, EVAL_ERR_OUTOFRANGE);
}

/******************************************************************************
*
* NUMBER fact(n)
*
******************************************************************************/

_Check_return_
static inline F64
ln_fact(
    _InVal_     S32 n)
{
    return(lgamma(n + 1.0));
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
            err = status_from_errno();
        }

    if(status_fail(err))
        ev_data_set_error(p_ev_data_res, err);
}

/******************************************************************************
*
* NUMBER int(number)
*
* NB truncates towards zero
*
******************************************************************************/

PROC_EXEC_PROTO(c_int)
{
    exec_func_ignore_parms();

    if(RPN_DAT_REAL == args[0]->did_num)
    {
        /* SKS as per Fireworkz, try pre-rounding an ickle bit so INT((0.06-0.04)/0.01) is 2 not 1 */
        int exponent;
        F64 mantissa = frexp(args[0]->arg.fp, &exponent);
        F64 f64;
        F64 result;

        if(mantissa > (+1.0 - 1E-14))
            mantissa = +1.0;
        else if(mantissa < (-1.0 + 1E-14))
            mantissa = -1.0;
        f64 = ldexp(mantissa, exponent);

        (void) modf(f64, &result);

        ev_data_set_real(p_ev_data_res, result);
        real_to_integer_try(p_ev_data_res);
    }
    else
    {   /*NOP*/
        *p_ev_data_res = *(args[0]);
    }
}

/******************************************************************************
*
* REAL ln(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_ln)
{
    F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, log(number));

    if(errno /* == EDOM, ERANGE */)
        ev_data_set_error(p_ev_data_res, EVAL_ERR_BAD_LOG);
}

/******************************************************************************
*
* REAL log(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_log)
{
    F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, log10(number));

    if(errno /* == EDOM, ERANGE */)
        ev_data_set_error(p_ev_data_res, EVAL_ERR_BAD_LOG);
}

/******************************************************************************
*
* NUMBER mod(a, b)
*
******************************************************************************/

PROC_EXEC_PROTO(c_mod)
{
    exec_func_ignore_parms();

    switch(two_nums_type_match(args[0], args[1], FALSE))
    {
    case TWO_INTS:
        {
        S32 s32_a = args[0]->arg.integer;
        S32 s32_b = args[1]->arg.integer;
        S32 s32_mod_result;

        /* SKS after 4.11 03feb92 - gave FP error not zero if trap taken */
        if(s32_b == 0)
        {
            ev_data_set_error(p_ev_data_res, EVAL_ERR_DIVIDEBY0);
            return;
        }

        s32_mod_result = (s32_a % s32_b);

        ev_data_set_integer(p_ev_data_res, s32_mod_result);
        break;
        }

    case TWO_REALS:
        {
        F64 f64_a = args[0]->arg.fp;
        F64 f64_b = args[1]->arg.fp;
        F64 f64_mod_result;

        errno = 0;

        f64_mod_result = fmod(f64_a, f64_b);

        ev_data_set_real(p_ev_data_res, f64_mod_result);

        /* would have divided by zero? */
        if(errno /* == EDOM */)
            ev_data_set_error(p_ev_data_res, EVAL_ERR_DIVIDEBY0);

        break;
        }
    }
}

/* a little note from SKS:

 * it seems that fmod(a, b) guarantees the sign of the result
 * to be the same sign as a, whereas integer ops give an
 * implementation defined sign for both a / b and a % b.
*/

/******************************************************************************
*
* REAL round(n, decimals)
*
******************************************************************************/

extern void
round_common(
    P_EV_DATA args[EV_MAX_ARGS],
    _InVal_     S32 n_args,
    _InoutRef_  P_EV_DATA p_ev_data_res,
    _InVal_     U32 rpn_did_num)
{
    BOOL negate_result = FALSE;
    F64 f64, multiplier;

    {
        S32 decimal_places = 2;

        if(n_args > 1)
            decimal_places = MIN(15, args[1]->arg.integer);

        switch(args[0]->did_num)
        {
        case RPN_DAT_BOOL8:
        case RPN_DAT_WORD16:
        case RPN_DAT_WORD32:
            if(decimal_places >= 0)
            {   /* if we have an integer number to be rounded at, or to the right of, the decimal, it's already there */
                /* NOP */
                *p_ev_data_res = *(args[0]);
                return;
            }

            f64 = (F64) args[0]->arg.integer; /* have to do it like this */
            break;

        default:
            f64 = args[0]->arg.fp;
            break;
        }

        /* operate on positive number */
        if(f64 < 0.0)
        {
            negate_result = TRUE;
            f64 = -f64;
        }

        multiplier = pow(10.0, (F64) decimal_places);
    }

    f64 = f64 * multiplier;

    switch(rpn_did_num)
    {
    default: default_unhandled();
#if CHECKING
    case RPN_FNV_ROUND:
#endif
        f64 = floor(f64 + 0.5); /* got a positive number here */
        break;
    }

    f64 = f64 / multiplier;

    ev_data_set_real(p_ev_data_res, negate_result ? -f64 : f64);
}

PROC_EXEC_PROTO(c_round)
{
    exec_func_ignore_parms();

    round_common(args, n_args, p_ev_data_res, RPN_FNV_ROUND);

    real_to_integer_try(p_ev_data_res);
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
* REAL sqr(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sqr)
{
    F64 number = args[0]->arg.fp;

    exec_func_ignore_parms();

    errno = 0;

    ev_data_set_real(p_ev_data_res, sqrt(number));

    if(errno /* == EDOM */)
        ev_data_set_error(p_ev_data_res, EVAL_ERR_NEG_ROOT);
}

/******************************************************************************
*
*  Matrix functions
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
        break; /* not as silly a question as RJM first thought! (see Bloom, Linear Algebra and Geometry) */

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
    S32 xs_0, ys_0;
    S32 xs_1, ys_1;

    exec_func_ignore_parms();

    /* check it is an array and get x and y sizes */
    array_range_sizes(args[0], &xs_0, &ys_0);
    array_range_sizes(args[1], &xs_1, &ys_1);

    if(xs_0 != ys_1)
        {
        /* whinge about dimensions */
        ev_data_set_error(p_ev_data_res, EVAL_ERR_MISMATCHED_MATRICES);
        return;
        }

    if(status_ok(ss_array_make(p_ev_data_res, xs_1, ys_0)))
        {
        S32 x, y;

        for(x = 0; x < xs_1; x++)
            {
            for(y = 0; y < ys_0; y++)
                {
                F64 product = 0.0;
                S32 elem;
                P_EV_DATA elep;

                for(elem = 0; elem < xs_0; elem++)
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

/* end of ev_math.c */
