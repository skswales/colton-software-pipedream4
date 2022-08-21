/* ev_fnstm.c */

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

/******************************************************************************
*
* Statistical functions (Multivariate linear/logarihmic fit using least squares)
*
******************************************************************************/

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

/* end of ev_fnstm.c */
