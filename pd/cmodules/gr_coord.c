/* gr_coord.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Coordinate manipulation */

/* SKS July 1991 */

#include "common/gflags.h"

#include "gr_coord.h"

#include "muldiv.h"

#define muldiv64_a_b_GR_SCALE_ONE(a, b) muldiv64(a, b, GR_SCALE_ONE)

/******************************************************************************
*
* create a fixed point binary representation of an integer fraction
*
******************************************************************************/

_Check_return_
extern GR_SCALE
gr_scale_from_s32_pair(
    _InVal_     S32 numerator,
    _InVal_     S32 denominator)
{
    GR_SCALE num;
    S32 overflow;

    if(numerator == 0)
        return(GR_SCALE_ZERO);

    num = muldiv64(numerator, GR_SCALE_ONE, denominator);

    /* check not OTT */
    overflow = muldiv64_overflow();

    if(overflow > 0)
        return(+GR_SCALE_MAX);

    if(overflow < 0)
        return(-GR_SCALE_MAX);

    return(num);
}

/******************************************************************************
*
* compute the intersection of a point and a box of coordinates
*
******************************************************************************/

_Check_return_
extern BOOL
gr_box_hit(
    _InRef_     PC_GR_BOX box,
    _InRef_     PC_GR_POINT point,
    _InRef_opt_ PC_GR_SIZE size /*NULL->exact hit required*/)
{
    if(NULL != size)
    {
        /* inclusive */
        if((point->x + size->cx) < box->x0)
            return(FALSE);
        if((point->y + size->cy) < box->y0)
            return(FALSE);

        /* exclusive */
        if((point->x - size->cx) > box->x1)
            return(FALSE);
        if((point->y - size->cy) > box->y1)
            return(FALSE);
    }
    else
    {
        /* inclusive */
        if(point->x < box->x0)
            return(FALSE);
        if(point->y < box->y0)
            return(FALSE);

        /* exclusive */
        if(point->x > box->x1)
            return(FALSE);
        if(point->y > box->y1)
            return(FALSE);
    }

    return(TRUE);
}

/******************************************************************************
*
* compute the intersection of a pair of boxes of coordinates
*
******************************************************************************/

_Check_return_
extern S32
gr_box_intersection(
    _OutRef_    P_GR_BOX ibox,
    _In_opt_    PC_GR_BOX abox /*NULL->src ibox*/,
    _InRef_     PC_GR_BOX bbox)
{
    if(!abox)
        abox = ibox;

    /* all coordinates independent so using input as output is trivial */
    ibox->x0 = MAX(abox->x0, bbox->x0);
    ibox->y0 = MAX(abox->y0, bbox->y0);
    ibox->x1 = MIN(abox->x1, bbox->x1);
    ibox->y1 = MIN(abox->y1, bbox->y1);

    if((ibox->x0 == ibox->x1)  ||  (ibox->y0 == ibox->y1))
        /* zero sized intersection, point or line */
        return(0);

    if((ibox->x0  > ibox->x1)  ||  (ibox->y0  > ibox->y1))
        /* no intersection */
        return(-1);

    return(1);
}

/******************************************************************************
*
* create a reversed box of coordinates suitable for unioning into
*
******************************************************************************/

extern void
gr_box_make_bad(
    _OutRef_    P_GR_BOX abox)
{
    abox->x0 = +GR_COORD_MAX;
    abox->y0 = +GR_COORD_MAX;
    abox->x1 = -GR_COORD_MAX;
    abox->y1 = -GR_COORD_MAX;
}

/******************************************************************************
*
* create a fully spanning box of coordinates
*
******************************************************************************/

extern void
gr_box_make_huge(
    _OutRef_    P_GR_BOX abox)
{
    abox->x0 = -GR_COORD_MAX;
    abox->y0 = -GR_COORD_MAX;
    abox->x1 = +GR_COORD_MAX;
    abox->y1 = +GR_COORD_MAX;
}

/******************************************************************************
*
* create a null box of coordinates
*
******************************************************************************/

extern void
gr_box_make_null(
    _OutRef_    P_GR_BOX abox)
{
    abox->x0 = 0;
    abox->y0 = 0;
    abox->x1 = 0;
    abox->y1 = 0;
}

#if defined(UNUSED)

/******************************************************************************
*
* rotate a box of coordinates anticlockwise about a point
* NB. if an axis-aligned box is required, sort the result
*
******************************************************************************/

extern P_GR_BOX
gr_box_rotate(
    _OutRef_    P_GR_BOX xbox,
    _InRef_     PC_GR_BOX abox,
    _InRef_     PC_GR_POINT spoint,
    _InRef_     PC_F64 angle)
{
    GR_XFORMMATRIX xform;
    return(gr_box_xform(xbox, abox, gr_xform_make_rotation(&xform, spoint, angle)));
}

#endif /* UNUSED */

/******************************************************************************
*
* scale a box of coordinates about a point
*
******************************************************************************/

extern P_GR_BOX
gr_box_scale(
    _OutRef_    P_GR_BOX xbox,
    _InRef_     PC_GR_BOX abox,
    _InRef_     PC_GR_POINT spoint,
    _InRef_     PC_GR_SCALE_PAIR scale)
{
    GR_XFORMMATRIX xform;
    return(gr_box_xform(xbox, abox, gr_xform_make_scale(&xform, spoint, scale->x, scale->y)));
}

/******************************************************************************
*
* sort a box of coordinates
*
******************************************************************************/

extern void
gr_box_sort(
    _OutRef_    P_GR_BOX sbox,
    _InRef_     PC_GR_BOX abox /*may==sbox*/)
{
    GR_COORD tmp;

    if(abox->x1 < abox->x0)
    {
        tmp      = abox->x1;
        sbox->x1 = abox->x0;
        sbox->x0 = tmp;
    }
    else if(sbox != abox)
    {
        sbox->x0 = abox->x0;
        sbox->x1 = abox->x1;
    }

    if(abox->y1 < abox->y0)
    {
        tmp      = abox->y1;
        sbox->y1 = abox->y0;
        sbox->y0 = tmp;
    }
    else if(sbox != abox)
    {
        sbox->y0 = abox->y0;
        sbox->y1 = abox->y1;
    }
}

/******************************************************************************
*
* translate a box of coordinates
*
******************************************************************************/

extern P_GR_BOX
gr_box_translate(
    _OutRef_    P_GR_BOX xbox,
    _In_opt_    PC_GR_BOX abox /*NULL->src xbox*/,
    _InRef_     PC_GR_POINT spoint)
{
    /* can do rather faster by adding than by general transformation! */

    if(!abox)
        abox = xbox;

    /* all coordinates independent so using input as output is trivial */
    if(spoint->x)
    {
        xbox->x0 = abox->x0 + spoint->x;
        xbox->x1 = abox->x1 + spoint->x;
    }

    if(spoint->y)
    {
        xbox->y0 = abox->y0 + spoint->y;
        xbox->y1 = abox->y1 + spoint->y;
    }

    return(xbox);
}

/******************************************************************************
*
* create the union of a pair of boxes of coordinates
*
#ifdef GR_COORD_NO_UNION_EMPTIES
* --- take care not to union empty boxes! ---
#endif
*
******************************************************************************/

extern P_GR_BOX
gr_box_union(
    _OutRef_    P_GR_BOX ubox,
    _In_opt_    PC_GR_BOX abox /*NULL->src ubox*/,
    _InRef_     PC_GR_BOX bbox)
{
    if(!abox)
        abox = ubox;

    /* all coordinates independent so using input as output is trivial */
#ifdef GR_COORD_NO_UNION_EMPTIES
    if((abox->x0 == abox->x1)  ||  (abox->y0 == abox->y1))
    {
        if(ubox != (voidp) bbox)
            *ubox = *bbox;
    }
    else if((bbox->x0 == bbox->x1)  ||  (bbox->y0 == bbox->y1))
    {
        if(ubox != (voidp) abox)
            *ubox = *abox;
    }
    else
#else
    /* SKS doesn't think this is a good idea after all!
     * consider the case of unioning into a group bbox
     * where all grouped objects have 'empty' bboxes ...
    */
#endif
    {
        ubox->x0 = MIN(abox->x0, bbox->x0);
        ubox->y0 = MIN(abox->y0, bbox->y0);
        ubox->x1 = MAX(abox->x1, bbox->x1);
        ubox->y1 = MAX(abox->y1, bbox->y1);
    }

    return(ubox);
}

/******************************************************************************
*
* transform a box of coordinates
* NB if an axis aligned box is required then
* (i)  you may doing the wrong thing
* (ii) just apply gr_box_sort() to the result
*
******************************************************************************/

extern P_GR_BOX
gr_box_xform(
    _OutRef_    P_GR_BOX xbox,
    _In_opt_    PC_GR_BOX abox /*NULL->src xbox*/,
    _InRef_     PC_GR_XFORMMATRIX xform)
{
    GR_BOX tmp;
    P_GR_BOX p;

    if(!abox)
        abox = xbox;

    /* enable user to use input as output */
    if(abox == (PC_GR_BOX) xbox)
        p = &tmp;
    else
        p = xbox;

    p->x0 = muldiv64(xform->a, abox->x0, GR_SCALE_ONE) + muldiv64(xform->c, abox->y0, GR_SCALE_ONE) + xform->e;
    p->y0 = muldiv64(xform->c, abox->x0, GR_SCALE_ONE) + muldiv64(xform->d, abox->y0, GR_SCALE_ONE) + xform->f;
    p->x1 = muldiv64(xform->a, abox->x1, GR_SCALE_ONE) + muldiv64(xform->c, abox->y1, GR_SCALE_ONE) + xform->e;
    p->y1 = muldiv64(xform->c, abox->x1, GR_SCALE_ONE) + muldiv64(xform->d, abox->y1, GR_SCALE_ONE) + xform->f;

    if(p == &tmp)
        *xbox = tmp;

    return(xbox);
}

/******************************************************************************
*
* scale a coordinate
*
******************************************************************************/

_Check_return_
extern GR_COORD
gr_coord_scale(
    _InVal_     GR_COORD coord,
    _InVal_     GR_SCALE scale)
{
    return(muldiv64(coord, scale, GR_SCALE_ONE));
}

/******************************************************************************
*
* sort a pair of coordinates
*
******************************************************************************/

extern void
gr_coord_sort(
    _InoutRef_  P_GR_COORD acoord,
    _InoutRef_  P_GR_COORD bcoord)
{
    GR_COORD tmp;

    if(*bcoord < *acoord)
    {
        tmp     = *bcoord;
        *bcoord = *acoord;
        *acoord = tmp;
    }
}

#if defined(UNUSED)

/******************************************************************************
*
* rotate a point anticlockwise about a point
*
******************************************************************************/

extern P_GR_POINT
gr_point_rotate(
    _OutRef_    P_GR_POINT xpoint,
    _InRef_     PC_GR_POINT apoint,
    _InRef_     PC_GR_POINT spoint,
    _InRef_     PC_F64 angle)
{
    GR_XFORMMATRIX xform;
    return(gr_point_xform(xpoint, apoint, gr_xform_make_rotation(&xform, spoint, angle)));
}

#endif /* UNUSED*/

/******************************************************************************
*
* scale a point about a point
*
******************************************************************************/

extern P_GR_POINT
gr_point_scale(
    _OutRef_    P_GR_POINT xpoint,
    _InRef_     PC_GR_POINT apoint,
    _InRef_opt_ PC_GR_POINT spoint,
    _InRef_     PC_GR_SCALE_PAIR scale)
{
    GR_XFORMMATRIX xform;
    return(gr_point_xform(xpoint, apoint, gr_xform_make_scale(&xform, spoint, scale->x, scale->y)));
}

#if defined(UNUSED)

/******************************************************************************
*
* translate a point
*
******************************************************************************/

extern P_GR_POINT
gr_point_translate(
    _OutRef_    P_GR_POINT xpoint,
    _InRef_     PC_GR_POINT apoint,
    _InRef_     PC_GR_POINT spoint)
{
    /* can do rather faster by adding than by general transformation! */

    /* all coordinates independent so using input as output is trivial */
    xpoint->x = apoint->x + spoint->x;
    xpoint->y = apoint->y + spoint->y;

    return(xpoint);
}

#endif /* UNUSED*/

/******************************************************************************
*
* transform a point
*
******************************************************************************/

extern P_GR_POINT
gr_point_xform(
    _OutRef_    P_GR_POINT xpoint,
    _InRef_     PC_GR_POINT apoint,
    _InRef_     PC_GR_XFORMMATRIX xform)
{
    GR_POINT tmp;
    P_GR_POINT p;

    /* enable user to use input as output */
    if(apoint == xpoint)
        p = &tmp;
    else
        p = xpoint;

    p->x = muldiv64(xform->a, apoint->x, GR_SCALE_ONE) + muldiv64(xform->c, apoint->y, GR_SCALE_ONE) + xform->e;
    p->y = muldiv64(xform->c, apoint->x, GR_SCALE_ONE) + muldiv64(xform->d, apoint->y, GR_SCALE_ONE) + xform->f;

    if(p == &tmp)
        *xpoint = tmp;

    return(xpoint);
}

/******************************************************************************
*
* create a transform that is the combination of two others
* axform is to be applied first
* bxform is to then be applied
*
******************************************************************************/

/*ncr*/
extern P_GR_XFORMMATRIX
gr_xform_make_combination(
    _OutRef_    P_GR_XFORMMATRIX xform,
    _InRef_     PC_GR_XFORMMATRIX axform,
    _InRef_     PC_GR_XFORMMATRIX bxform)
{
    GR_XFORMMATRIX tmp;
    P_GR_XFORMMATRIX p;

    /* enable user to use input as output */
    if((axform == (PC_GR_XFORMMATRIX) xform)  ||  (bxform == (PC_GR_XFORMMATRIX) xform))
        p = &tmp;
    else
        p = xform;

    /* 16.16p 16.16 = 32.32 i.e. take middle word for new 16.16 */

    p->a  = muldiv64(bxform->a, axform->a, GR_SCALE_ONE);
    p->a += muldiv64(bxform->c, axform->b, GR_SCALE_ONE);

    p->c  = muldiv64(bxform->a, axform->c, GR_SCALE_ONE);
    p->c += muldiv64(bxform->c, axform->d, GR_SCALE_ONE);

    p->e  = muldiv64(bxform->a, axform->e, GR_SCALE_ONE);
    p->e += muldiv64(bxform->c, axform->f, GR_SCALE_ONE);
    p->e += bxform->e;

    p->b  = muldiv64(bxform->b, axform->a, GR_SCALE_ONE);
    p->b += muldiv64(bxform->d, axform->b, GR_SCALE_ONE);

    p->d  = muldiv64(bxform->b, axform->c, GR_SCALE_ONE);
    p->d += muldiv64(bxform->d, axform->d, GR_SCALE_ONE);

    p->f  = muldiv64(bxform->b, axform->e, GR_SCALE_ONE);
    p->f += muldiv64(bxform->d, axform->f, GR_SCALE_ONE);
    p->f += bxform->f;

    if(p == &tmp)
        *xform = tmp;

    return(xform);
}

/******************************************************************************
*
* create a transform that is a rotation about a given point
*
******************************************************************************/

/*ncr*/
extern P_GR_XFORMMATRIX
gr_xform_make_rotation(
    _OutRef_    P_GR_XFORMMATRIX xform,
    _InRef_     PC_GR_POINT spoint,
    _InRef_     PC_F64 angle)
{
    F64 c = cos(*angle);
    F64 s = sin(*angle);

    xform->a = gr_scale_from_f64(+c);
    xform->b = gr_scale_from_f64(+s);
    xform->c = gr_scale_from_f64(-s);
    xform->d = gr_scale_from_f64(+c);
    xform->e = gr_coord_from_f64(spoint->x * (1 - c) + s * spoint->y);
    xform->f = gr_coord_from_f64(spoint->y * (1 - c) - s * spoint->x);

    return(xform);
}

/******************************************************************************
*
* create a transform that is a scaling about a given point
*
******************************************************************************/

/*ncr*/
extern P_GR_XFORMMATRIX
gr_xform_make_scale(
    _OutRef_    P_GR_XFORMMATRIX xform,
    _InRef_opt_ PC_GR_POINT spoint /*NULL->origin*/,
    _InVal_     GR_SCALE xscale,
    _InVal_     GR_SCALE yscale)
{
    /* spoint == NULL -> scale about origin */
    GR_COORD x = spoint ? spoint->x : 0;
    GR_COORD y = spoint ? spoint->y : 0;

    xform->a = xscale;
    xform->b = GR_SCALE_ZERO;
    xform->c = GR_SCALE_ZERO;
    xform->d = yscale;
    xform->e = muldiv64_a_b_GR_SCALE_ONE(x, (GR_SCALE_ONE - xscale));
    xform->f = muldiv64_a_b_GR_SCALE_ONE(y, (GR_SCALE_ONE - yscale));

    return(xform);
}

/******************************************************************************
*
* create a transform that is a translation of the origin to a given point
*
******************************************************************************/

extern void
gr_xform_make_translation(
    _OutRef_    P_GR_XFORMMATRIX xform,
    _InRef_     PC_GR_POINT spoint)
{
    xform->a = GR_SCALE_ONE;
    xform->b = GR_SCALE_ZERO;
    xform->c = GR_SCALE_ZERO;
    xform->d = GR_SCALE_ONE;
    xform->e = spoint->x;
    xform->f = spoint->y;
}

/* end of gr_coord.c */
