/* ev_fnsta.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Statistical function routines for evaluator */

#include "common/gflags.h"

#include "cmodules/ev_eval.h"

#include "cmodules/ev_evali.h"

#include "cmodules/mathxtr2.h" /* for linest() */
#include "cmodules/mathxtr3.h" /* for uniform_distribution() */

static BOOL
uniform_distribution_seeded = FALSE;

/******************************************************************************
*
* Statistical functions
*
******************************************************************************/

/*
Statistical functions calculated using array_range processing in ev_exec.c:
AVG()
COUNT()
COUNTA()
MAX()
MIN()
STD()
STDP()
VAR()
VARP()
*/

/*
flatten an x by y array into a 1 by x*y array
*/

static void
ss_data_array_flatten(
    _InoutRef_  P_EV_DATA p_ev_data)
{
    if(RPN_TMP_ARRAY == p_ev_data->did_num)
    {
        /* this is just so easy given the current array representation */
        p_ev_data->arg.ev_array.y_size *= p_ev_data->arg.ev_array.x_size;
        p_ev_data->arg.ev_array.x_size = 1;
    }
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
    const F64 a = lgamma(z);
    const F64 b = lgamma(w);
    const F64 c = lgamma(z + w);
    beta_result = exp(a + b - c);
    } /*block*/

    ev_data_set_real(p_ev_data_res, beta_result);

    if(errno)
        err = status_from_errno();

    if(status_fail(err))
        ev_data_set_error(p_ev_data_res, err);
}

/******************************************************************************
*
* bin(array1, array2)
*
******************************************************************************/

PROC_EXEC_PROTO(c_bin)
{
    const P_EV_DATA array_data = args[0];
    const P_EV_DATA array_bins = args[1];
    S32 xs_0, ys_0;
    S32 xs_1, ys_1;

    exec_func_ignore_parms();

    array_range_sizes(array_data, &xs_0, &ys_0);
    array_range_sizes(array_bins, &xs_1, &ys_1);

    /* make result array */
    if(status_ok(ss_array_make(p_ev_data_res, 1, ys_1 + 1)))
    {
        { /* clear result array to zero as widest integers */
        S32 y;
        for(y = 0; y < ys_1 + 1; ++y)
        {
            P_EV_DATA p_ev_data = ss_array_element_index_wr(p_ev_data_res, 0, y);
            ev_data_set_WORD32(p_ev_data, 0);
        }
        } /*block*/

        { /* put each item into a bin */
        S32 x, y;
        for(x = 0; x < xs_0; ++x)
        {
            for(y = 0; y < ys_0; ++y)
            {
                EV_DATA ev_data;

                switch(array_range_index(&ev_data, array_data, x, y, EM_REA | EM_INT | EM_DAT | EM_STR))
                {
                case RPN_DAT_REAL:
                case RPN_DAT_WORD8:
                case RPN_DAT_WORD16:
                case RPN_DAT_WORD32:
                case RPN_DAT_DATE:
                case RPN_DAT_STRING:
                    {
                    S32 bin_y, bin_out_y = ys_1;

                    for(bin_y = 0; bin_y < ys_1; ++bin_y)
                    {
                        EV_DATA ev_data_bin;
                        S32 res;
                        (void) array_range_index(&ev_data_bin, array_bins, 0, bin_y, EM_REA | EM_INT | EM_DAT | EM_STR);
                        res = ss_data_compare(&ev_data, &ev_data_bin);
                        ss_data_free_resources(&ev_data_bin);
                        if(res <= 0)
                        {
                            bin_out_y = bin_y;
                            break;
                        }
                    }

                    ss_array_element_index_wr(p_ev_data_res, 0, bin_out_y)->arg.integer += 1;
                    break;
                    }

                default:
                    break;
                }

                ss_data_free_resources(&ev_data);
            }
        }
        } /*block*/

    }
}

/******************************************************************************
*
* NUMBER combin(n, k)
*
******************************************************************************/

_Check_return_
static inline F64
ln_fact(
    _InVal_     S32 n)
{
    return(lgamma(n + 1.0));
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
            err = status_from_errno();
        }

    if(status_fail(err))
        ev_data_set_error(p_ev_data_res, err);
}

/******************************************************************************
*
* REAL gammaln(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_gammaln)
{
    STATUS err = STATUS_OK;
    const F64 number = args[0]->arg.fp;
    F64 gammaln_result;

    exec_func_ignore_parms();

    errno = 0;

    gammaln_result = lgamma(number);

    ev_data_set_real(p_ev_data_res, gammaln_result);

    if(errno)
        err = status_from_errno();

    if(status_fail(err))
        ev_data_set_error(p_ev_data_res, err);
}

/******************************************************************************
*
* REAL grand({mean {, sd}})
*
******************************************************************************/

PROC_EXEC_PROTO(c_grand)
{
    F64 s = 1.0;
    F64 m = 0.0;
    F64 r;

    exec_func_ignore_parms();

    switch(n_args)
    {
    case 2:
        if((s = args[1]->arg.fp) < 0.0)
            s = -s;

        /*FALLTHRU*/

    case 1:
        m = args[0]->arg.fp;
        break;

    default:
        break;
    }

    if(!uniform_distribution_seeded)
    {
        if((n_args > 1) && (args[1]->arg.fp < 0.0))
            uniform_distribution_seed((unsigned int) -args[1]->arg.fp);
        else
            uniform_distribution_seed((unsigned int) time(NULL));

        uniform_distribution_seeded = TRUE;
    }

    r  = normal_distribution();

    r *= s;
    r += m;

    ev_data_set_real(p_ev_data_res, r);
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

    switch(n_args)
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
    if(n_args < 2)
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
            if(n_args > 1)
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
* ARRAY listcount(array)
*
******************************************************************************/

PROC_EXEC_PROTO(c_listcount)
{
    STATUS status = STATUS_OK;
    EV_DATA ev_data_temp_array;

    exec_func_ignore_parms();

    ev_data_set_blank(&ev_data_temp_array);

    for(;;)
    {
        S32 xs, ys, n_unique = 0, y = 0;

        status_assert(ss_data_resource_copy(&ev_data_temp_array, args[0]));
        data_ensure_constant(&ev_data_temp_array);

        if(RPN_DAT_ERROR == ev_data_temp_array.did_num)
        {
            ss_data_free_resources(p_ev_data_res);
            *p_ev_data_res = ev_data_temp_array;
            return;
        }

        status_break(status = array_sort(&ev_data_temp_array, 0));

        status_break(status = ss_array_make(p_ev_data_res, 0, 0));

        array_range_sizes(&ev_data_temp_array, &xs, &ys);

        while(y < ys && status_ok(status))
        {
            EV_DATA ev_data;
            F64 count = 0;

            array_range_index(&ev_data, &ev_data_temp_array, 0, y, EM_CONST);

            while(status_ok(status))
            {
                EV_DATA ev_data_t;
                array_range_index(&ev_data_t, &ev_data_temp_array, 0, y, EM_CONST);
                if(ss_data_compare(&ev_data, &ev_data_t))
                {
                    if(status_ok(status = ss_array_element_make(p_ev_data_res, 1, n_unique)))
                    {
                        P_EV_DATA p_ev_data = ss_array_element_index_wr(p_ev_data_res, 0, n_unique);
                        status_assert(ss_data_resource_copy(p_ev_data, &ev_data));
                        ev_data_set_real(&p_ev_data[1], count);
                        real_to_integer_try(&p_ev_data[1]);
                        n_unique += 1;
                    }
                    break;
                }
                else if(xs > 1)
                {
                    EV_DATA ev_data_count;
                    /* ignore content of all columns between the first (data) and last (count) */
                    if(RPN_DAT_REAL == array_range_index(&ev_data_count, &ev_data_temp_array, xs-1, y, EM_REA))
                        count += ev_data_count.arg.fp; /* SKS 19may97 was just using [1, y] before */
                    ss_data_free_resources(&ev_data_count);
                }
                else
                    count += 1.0;

                y += 1;
                ss_data_free_resources(&ev_data_t);
            }

            ss_data_free_resources(&ev_data);
        }

        break;
        /*NOTREACHED*/
    }

    ss_data_free_resources(&ev_data_temp_array);

    if(status_fail(status))
    {
        ss_data_free_resources(p_ev_data_res);
        ev_data_set_error(p_ev_data_res, status);
    }
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

    if(n_args > 3)
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

    /* ask MRJC whether this is legal */
    targs0  = args[0];
    args[0] = &y_data;

    if(n_args > 3)
        {
        targs3  = args[3];
        args[3] = &a_data;
        }
    else
        targs3  = NULL;

    /* call my friend to do the hard work */
    c_linest(args, n_args, p_ev_data_res, p_cur_slr);

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


/******************************************************************************
*
* NUMBER median(array)
*
******************************************************************************/

PROC_EXEC_PROTO(c_median)
{
    STATUS status = STATUS_OK;
    EV_DATA ev_data_temp_array;

    exec_func_ignore_parms();

    ev_data_set_blank(&ev_data_temp_array);

    for(;;)
    {
        S32 xs, ys, ys_half;
        F64 median;

        /* find median by sorting a copy of the source */
        status_assert(ss_data_resource_copy(&ev_data_temp_array, args[0]));
        data_ensure_constant(&ev_data_temp_array);

        if(ev_data_temp_array.did_num == RPN_DAT_ERROR)
        {
            ss_data_free_resources(p_ev_data_res);
            *p_ev_data_res = ev_data_temp_array;
            return;
        }

        ss_data_array_flatten(&ev_data_temp_array); /* SKS 04jun96 - flatten so we can median() on horizontal and rectangular ranges */

        status_break(status = array_sort(&ev_data_temp_array, 0));

        array_range_sizes(&ev_data_temp_array, &xs, &ys);
        ys_half = ys / 2;

        {
        EV_DATA ev_data;
        array_range_index(&ev_data, &ev_data_temp_array, 0, ys_half, EM_REA);
        median = ev_data.arg.fp;
        } /*block*/

        if(0 == (ys & 1))
        {
            /* if there are an even number of elements, the median is the mean of the two middle ones */
            EV_DATA ev_data;
            array_range_index(&ev_data, &ev_data_temp_array, 0, ys_half - 1, EM_REA);
            median = (median + ev_data.arg.fp) / 2.0;
        }

        ev_data_set_real(p_ev_data_res, median);
        real_to_integer_try(p_ev_data_res);

        break;
        /*NOTREACHED*/
    }

    ss_data_free_resources(&ev_data_temp_array);

    if(status_fail(status))
    {
        ss_data_free_resources(p_ev_data_res);
        ev_data_set_error(p_ev_data_res, status);
    }
}

/******************************************************************************
*
* NUMBER permut(n, k)
*
******************************************************************************/

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
            err = status_from_errno();
        }

    if(status_fail(err))
        ev_data_set_error(p_ev_data_res, err);
}

/******************************************************************************
*
* REAL rand({n})
*
******************************************************************************/

PROC_EXEC_PROTO(c_rand)
{
    exec_func_ignore_parms();

    if(!uniform_distribution_seeded)
    {
        if((0 != n_args) && (args[0]->arg.fp != 0.0))
            uniform_distribution_seed((unsigned int) args[0]->arg.fp);
        else
            uniform_distribution_seed((unsigned int) time(NULL));

        uniform_distribution_seeded = TRUE;
    }

    ev_data_set_real(p_ev_data_res, uniform_distribution());
}

/******************************************************************************
*
* ARRAY rank(array {, spearflag})
*
******************************************************************************/

PROC_EXEC_PROTO(c_rank)
{
    BOOL spearman_correct = FALSE;
    S32 xs, ys;

    exec_func_ignore_parms();

    if(n_args > 1)
        spearman_correct = (0 != args[1]->arg.integer);

    array_range_sizes(args[0], &xs, &ys);

    /* make result array */
    if(status_ok(ss_array_make(p_ev_data_res, 2, ys)))
    {
        S32 y;

        for(y = 0; y < ys; ++y)
        {
            S32 iy, position = 1, equal = 1;
            EV_DATA ev_data;

            array_range_index(&ev_data, args[0], 0, y, EM_CONST);
            for(iy = 0; iy < ys; ++iy)
            {
                if(iy != y)
                {
                    EV_DATA ev_data_t;
                    S32 res;
                    array_range_index(&ev_data_t, args[0], 0, iy, EM_CONST);
                    res = ss_data_compare(&ev_data, &ev_data_t);
                    if(res < 0)
                        position += 1;
                    else if(res == 0)
                        equal += 1;
                    ss_data_free_resources(&ev_data_t);
                }
            }

            {
            P_EV_DATA p_ev_data = ss_array_element_index_wr(p_ev_data_res, 0, y);

            if(spearman_correct) /* SKS 12apr95 make suitable for passing to spearman with equal values */
                ev_data_set_real(p_ev_data, position + (equal - 1.0) / 2.0);
            else
                ev_data_set_integer(p_ev_data, position);

            p_ev_data = ss_array_element_index_wr(p_ev_data_res, 1, y);
            ev_data_set_integer(p_ev_data, equal);
            } /*block*/

            ss_data_free_resources(&ev_data);
        }
    }
}

#if 0 /* just for diff minimization */

/******************************************************************************
*
* INTEGER rank_xls(number, array {, order})
*
******************************************************************************/

PROC_EXEC_PROTO(c_rank_xls)
{
    S32 xs, ys;
    S32 order = 0;
    S32 position = 1, equal = 1;
    S32 ix;

    exec_func_ignore_parms();

    if(nargs > 2)
        order = args[2]->arg.integer;

    array_range_sizes(args[1], &xs, &ys);

    for(ix = 0; ix < xs; ++ix)
    {
        S32 iy;

        for(iy = 0; iy < ys; ++iy)
        {
            EV_DATA ev_data_t;
            S32 res;
            array_range_index(&ev_data_t, args[1], ix, iy, EM_CONST);
            res = ss_data_compare(args[0], &ev_data_t, FALSE, FALSE);
            if(res == 0)
                equal += 1;
            else
            {
                if(0 != order)
                    res = -res; /* reverse order */
                if(res < 0)
                    position += 1;
            }
            ss_data_free_resources(&ev_data_t);
        }
    }

    ev_data_set_integer(p_ev_data_res, position);
}

#endif

/******************************************************************************
*
* REAL spearman(array1, array2)
*
******************************************************************************/

PROC_EXEC_PROTO(c_spearman)
{
    F64 spearman_result;
    S32 xs_0, ys_0;
    S32 xs_1, ys_1;
    S32 y, ys;
    F64 sum_d_squared = 0.0;
    S32 n_counted = 0;

    exec_func_ignore_parms();

    array_range_sizes(args[0], &xs_0, &ys_0);
    array_range_sizes(args[1], &xs_1, &ys_1);

    ys = MAX(ys_0, ys_1);

    for(y = 0; y < ys; ++y)
    {
        EV_DATA ev_data_0;
        EV_DATA ev_data_1;
        EV_IDNO ev_idno_0 = array_range_index(&ev_data_0, args[0], 0, y, EM_REA);
        EV_IDNO ev_idno_1 = array_range_index(&ev_data_1, args[1], 0, y, EM_REA);

        if(ev_idno_0 == RPN_DAT_REAL && ev_idno_1 == RPN_DAT_REAL
           &&
           ev_data_0.arg.fp != 0. && ev_data_1.arg.fp != 0.)
        {
            F64 d = ev_data_1.arg.fp - ev_data_0.arg.fp;
            n_counted += 1;
            sum_d_squared += d * d;
        }

        ss_data_free_resources(&ev_data_0);
        ss_data_free_resources(&ev_data_1);
    }

    if(0 == n_counted)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_NO_VALID_DATA);
        return;
    }

    spearman_result = (1.0 - (6.0 * sum_d_squared) / ((F64) n_counted * ((F64) n_counted * (F64) n_counted - 1.0)));

    ev_data_set_real(p_ev_data_res, spearman_result);
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

/* end of ev_fnsta.c */
