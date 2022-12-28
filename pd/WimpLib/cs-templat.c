/* cs-templat.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* SKS 03 Aug 1990 This code written to replace the supplied RISC_OSLib code, slightly different API
 * SKS 05 Nov 1997 Hack template__resolve_icon to get round R6,3 icon problems on RISC OS 3.1
 * SKS 27 Nov 1997 Change ESG used to 1 from 15 to stop other radio buttons being nobbled
 * SKS 14 Jan 1998 Refrain from hacking ESG used! All templates should now have action buttons with ESG=1
 * SKS 21 Apr 1998 Support for tristate icons
 * SKS 19 Dec 2022 Support for deferred loading
 */

#include "include.h"

#include "cmodules/aligator.h"
#include "cmodules/typepack.h"

static int    template__nameeq(const char * s, const char * s12);
static void   template__resolve(unsigned int b);

#ifdef TEMPLATE_FONT_RESOLVE
static void   template__atexit(void);
static void   template__convert_font_ref(unsigned int b, WimpIconFlagsWithBitset * p_flags_ua);
#endif

/*
internal functions
*/

#define TEMPLATE__N_BLOCKS 10U

static char *       template__block[TEMPLATE__N_BLOCKS];

/*
invented here
*/

static int          template__initialised;
static unsigned int template__nTemplates[TEMPLATE__N_BLOCKS];
static char *       template__ind_block[TEMPLATE__N_BLOCKS];
static const char * template__require[TEMPLATE__N_BLOCKS];

/*
header of a template file
*/

typedef struct TEMPLATE_HEADER
{
    int fontDataOffset;
    int reserved0, reserved1, reserved2;

    /* followed by template_index_entries */
}
TEMPLATE_HEADER;

/*
index blocks in template file header
*/

typedef struct TEMPLATE_INDEX_ENTRY
{
    int     dataOffset; /* offset of 0 -> end of list */
    size_t  dataSize:16;
    size_t  indDataSize:16; /* SKS pokes this in on loading */
    union TEMPLATE_INDEX_ENTRY_BITS
    {
        size_t  indDataOffset; /* offset of indirect data in block */
        int     dataType;
    }
    bits;
    char    name[12];  /* NB. may be CR terminated */
}
TEMPLATE_INDEX_ENTRY;

#define template__header(b) ( \
    (TEMPLATE_HEADER *) template__block[b] )

#define template__indexentry(b, i) ( \
    ((TEMPLATE_INDEX_ENTRY *) (template__header(b) + 1)) + (i) )

#define template__indextowindow(b, i) ( \
    (WimpWindowWithBitset *) (template__block[b] + template__indexentry(b, i)->dataOffset) )

/*
internal <-> external handle conversion
*/

#define template__itop(b, i) ( \
    (template *) ((((b) << 8) | (i)) ^ 0x4A000000) )
#define template__ptoi(p)    ( \
                 (((int) (p))        ^ 0x4A000000) )

#define template__block_index(i)  ((i) >> 8)
#define template__block_offset(i) ((i) & 0xFF)


#ifdef TEMPLATE_FONT_RESOLVE

static char * template__font_refs;

/*
there may be fonts!
*/

typedef struct TEMPLATE_FONT_ENTRY
{
    int  xPointSize_x16;
    int  yPointSize_x16;
    char fontName[40];  /* NB. may be CR terminated */
}
TEMPLATE_FONT_ENTRY;

#define template__font_data_offset(b) ( \
    template__header(b)->fontDataOffset )

#define template__font_entry(b, i) ( \
    ((TEMPLATE_FONT_ENTRY *) (template__block[b] + template__font_data_offset(b))) + (i) )


typedef struct TEMPLATE_FONT_PAIR
{
    char physfont;
    char refs;
}
TEMPLATE_FONT_PAIR;

#define sizeof_template_font_pair 2

#define template__font_ref(i) ( \
    (TEMPLATE_FONT_PAIR *) (template__font_refs + sizeof_template_font_pair * (i)) )


static void
template__atexit(void)
{
    int i;

    if(template__font_refs)
    {
        for(i = 0; i < 256; ++i)
        {
            TEMPLATE_FONT_PAIR * fpp = template__font_ref(i);

            while(fpp->refs)
            {
                (void) font_LoseFont(fpp->physfont);
                fpp->refs--;
            }
        }
    }
}


#define wimp_IFONTHSHIFT 24
#define wimp_IFONTHMASK  (0xFFU << wimp_IFONTHSHIFT)

static void
template__convert_font_ref(unsigned int b, WimpIconFlagsWithBitset * p_flags_ua)
{
    WimpIconFlagsWithBitset iconflags;
    font                  logfont, physfont;
    TEMPLATE_FONT_ENTRY * fep;
    TEMPLATE_FONT_PAIR *  fpp;

    iconflags.u32 = readval_U32(p_flags_ua);

    logfont = (font) ((U32) (iconflags.u32 & wimp_IFONTHMASK)) >> (wimp_IFONTHSHIFT);

    fpp = template__font_refs ? template__font_ref(logfont) : NULL;

    if(!template__font_refs  ||  (fpp->physfont == 0))
    {
        if(template__font_refs)
        {
            fep = template__font_entry(b, logfont);

            if(wimpt_complain(font_find(fep->fontName,
                                        fep->xPointSize_x16,
                                        fep->yPointSize_x16,
                                        0, 0,
                                        &physfont)))
                physfont = 0;
        }
        else
            physfont = 0;

        if(!physfont)
        {   /* use system font */
            iconflags.bits.indirected = 0;
            iconflags.bits.fg_colour = 7;
            iconflags.bits.bg_colour = 0;
            writeval_U32(p_flags_ua, iconflags.u32);
            return;
        }

        fpp->physfont = physfont;
        fpp->refs++;
    }

    /* make physical font ref in template definition */
    iconflags.u32 = (iconflags.u32 & ~wimp_IFONTHMASK) | ((U32) fpp->physfont << wimp_IFONTHSHIFT);
    writeval_U32(p_flags_ua, iconflags.u32);
}

#endif /* TEMPLATE_FONT_RESOLVE */

/* call this at startup to set which template files to load */

extern void
template_require(
    const unsigned int idx,
    const char * name)
{
    if(idx < TEMPLATE__N_BLOCKS)
        template__require[idx] = name; /* NB not copied */
}

extern void
template_init(void)
{
    if(!template__initialised)
    {
        template_readfile(NULL);

        template__initialised = 1;
    }
}

/* ----------------------------- template_copy -----------------------------
 * Description:   Create a copy of a template.
 *
 * Parameters:    template * templateHandle
 * Returns:       a pointer to a copy of the window definition.
 * Other Info:    Copying also includes fixing up pointers into workspace for
 *                indirected icons/title.  Also includes allocation of this
 *                space.
 *
 */

extern WimpWindowWithBitset *
template_copy_new(
    template * templateHandle)
{
    TEMPLATE_INDEX_ENTRY * ip;
    size_t                 w_size, ind_size;
    const WimpWindowWithBitset * src_w_ua;
    WimpWindowWithBitset * w;
    ptrdiff_t              w_ind_change, ind_ind_change;
    int                    nIcons, icon;
    char *                 indData;
    const char *           src_ind;
    unsigned int           b, iTemplateHandle;

    if(templateHandle == NULL)
        return(NULL);

    iTemplateHandle = template__ptoi(templateHandle);
    b               = template__block_index(iTemplateHandle);
    iTemplateHandle = template__block_offset(iTemplateHandle);

    if(b >= TEMPLATE__N_BLOCKS)
    {
        myassert2x(0, "template_settitle(%u, %u): block index too big",
                   b, iTemplateHandle);
        return(NULL);
    }

    if(iTemplateHandle >= template__nTemplates[b])
    {
        myassert3x(0, "template_settitle(%u, %u): nTemplates[b] = %u",
                   b, iTemplateHandle, template__nTemplates[b]);
        return(NULL);
    }

    ip       = template__indexentry(b, iTemplateHandle);
    src_w_ua = (WimpWindowWithBitset *) (template__block[b] + ip->dataOffset);
    w_size   = ip->dataSize;
    ind_size = ip->indDataSize;
    src_ind  = template__ind_block[b] + ip->bits.indDataOffset;

    tracef7("\n\n\n[template__copy: base &%p index %u, entry &%p, src_w &%p, size %u, extra ind ws &%p, size %u]",
            template__block[b], iTemplateHandle, ip, src_w_ua, w_size, src_ind, ind_size);

    w = (WimpWindowWithBitset *) wlalloc_malloc(w_size + ind_size);
    if(!w)
        return(w);

    tracef3("[template__copy WIN DEFN from &%p to &%p, size %u]", src_w_ua, w, w_size);
    memcpy(w, src_w_ua, w_size);
#if TRACE
    if(memcmp(w, src_w_ua, w_size))
        tracef0("[template__copy WIN DEFN failed]");
#endif

    w->sprite_area = resspr_area(); /* whatever is in operation right now */
    /*reportf("resspr_area %p", resspr_area());*/

    /* all offsets in file are relative to start of object */
    w_ind_change = (char *) w - (char *) 0;

    { /* strap extra indirect data on end of workspace */
    char * newIndData = PtrAddBytes(char *, w, w_size);

    if(ind_size)
    {
        tracef3("[template__copy IND DATA from " PTR_XTFMT " to " PTR_XTFMT ", size %u]", src_ind, newIndData, ind_size);
        memcpy(newIndData, src_ind, ind_size);
#if TRACE
        if(memcmp(newIndData, src_ind, ind_size))
            tracef0("[template__copy IND DATA failed]");
#endif
    }

    ind_ind_change = newIndData - src_ind;
    } /*block*/

    /* relocate title indirection pointer */
    if(w->title_flags.bits.indirected)
    {
        indData = w->title_data.it.buffer;
        w->title_data.it.buffer = indData + (((int) indData < w_size) ? w_ind_change : ind_ind_change);
        tracef1("[template_copy_new: relocated title to " PTR_XTFMT "]", w->title_data.it.buffer);
    }

    nIcons = w->nicons;

    /* relocate icon indirection pointers */
    for(icon = 0; icon < nIcons; ++icon)
    {
        WimpIconBlockWithBitset * p_icon = (WimpIconBlockWithBitset *) (w + 1) + icon; /* yes, this is necessary */
        WimpIconFlagsWithBitset iconflags = p_icon->flags;

        if(iconflags.bits.indirected)
        {
            tracef2("[template__copy: got indirected icon %u, flags " U32_XTFMT "]", icon, iconflags.u32);
            indData = p_icon->data.it.buffer;
            p_icon->data.it.buffer = indData + (((int) indData < w_size) ? w_ind_change : ind_ind_change);
            tracef2("[template_copy_new: relocated icon %u data to " PTR_XTFMT "]", icon, p_icon->data.it.buffer);

            /* only needed for icon with text components
             * Note that the Window Manager sometimes uses
             * -1 as a null string.
            */
            if( iconflags.bits.text  &&
                ((p_icon->data.it.validation != NULL) && ((int) p_icon->data.it.validation != -1)) )
            {
                p_icon->data.it.validation = p_icon->data.it.validation + w_ind_change;
                tracef2("[template_copy_new: relocated icon %u validation string to " PTR_XTFMT "]",
                        icon, p_icon->data.it.validation);
            }
        }
    }

    return(w);
}

extern void
template_copy_dispose(
    WimpWindowWithBitset ** ppww)
{
    wlalloc_dispose((void **) ppww);
}


/*
NB. includes terminator (MUST be forced to CH_NULL)
*/

static size_t
template__datalen(
    char *str)
{
    char *ptr;
    int ch;

    tracef1("[template__datalen(&%p) entered: ", str);

    ptr = str;

    do
        ch = *ptr++;
    while(ch >= 32);

    /* always make CH_NULL terminated */
    *(ptr - 1) = CH_NULL;

    tracef2("'%s', length %u]", str, ptr - str);
    return(ptr - str);
}


/* ----------------------------- template_find -----------------------------
 * Description:   Finds a named template in the template list.
 *
 * Parameters:    const char *name -- the name of the template (as given in FormEd)
 * Returns:       a handle to the found template, 0 if not found
 * Other Info:    none.
 *
 */

extern template *
template_find_new(
    const char * name)
{
    template *             templateHandle = NULL;
    TEMPLATE_INDEX_ENTRY * ip, * first_ip;
    unsigned int           b;

    for(b = 0; (b < TEMPLATE__N_BLOCKS)  &&  !templateHandle; ++b)
    {
        first_ip = template__indexentry(b, 0);
        ip       = first_ip - 1;

        while((++ip)->dataOffset)
#if 0
            if(ip->bits.dataType == 1) /* only know about window definitions --- resolved at load time */
#endif
                if(template__nameeq(name, ip->name))
                {
                    templateHandle = template__itop(b, ip - first_ip);
                    break;
                }
    }

    tracef2("[template_find(%s) yields handle &%p]", name, templateHandle);
    return(templateHandle);
}


static BOOL
template__nameeq(
    const char *s,
    const char *s12)
{
    const char *d;
    int c, c12;
    int b12;

    tracef2("[template__nameeq(%s, %12.12s)]", s, s12);

    d = s;
    do  {
        c   = *s++;
        c12 = *s12++;
        b12 = (c12 < 32); /* s12 finished? */

        if(c == CH_NULL)
            return(b12); /* finished together? */

        if(b12  ||  (c != c12))
            return(FALSE); /* finished s12 before s, or failed match */
    }
    while(s - d < 12);

    return(*s == CH_NULL); /* s12 full 12 chars, has s finished? */
}


/* -------------------------- template_readfile ----------------------------
 * Description:   Reads the template file into a linked list of templates.
 *
 * Parameters:    const char *name -- name of template file
 * Returns:
 * Other Info:    Note that a call is made to resspr_area(), in order to
 *                fix up a window's sprite pointers, so you must have
 *                already called resspr_init.
 *
 */

static void
template__readfile_do(
    unsigned int b,
    const char * filename,
    size_t fileLength)
{
    _kernel_osfile_block fileblk;

    tracef1("[template_readfile: length = %u]", fileLength);
    template__block[b] = wlalloc_malloc(fileLength);
    tracef2("[template_readfile: template__block[%u] now &%p]", b, template__block[b]);

    if(!template__block[b])
    {
        werr(TRUE, msgs_lookup("template3" /*"No room to load Templates"*/));
        return;
    }

    fileblk.load = (int) template__block[b];
    fileblk.exec = 0;

    if(_kernel_osfile(0xFF, filename, &fileblk) == _kernel_ERROR)
        wimpt_noerr((os_error *) _kernel_last_oserror());
}

/*ncr*/
extern BOOL
template_ensure(
    const unsigned int idx)
{
    if(idx >= TEMPLATE__N_BLOCKS)
        return(FALSE);

    if(NULL != template__block[idx])
        return(TRUE);

    if(NULL == template__require[idx])
        return(FALSE);

    char filename__require[256];
    int length__require = res_findname(template__require[idx], filename__require);
    if(length__require > 0)
    {
        template__readfile_do(idx, filename__require, length__require);

        tracef2("[template_ensure: binding template__block[%u] at &%p]", idx, template__block[idx]);
        template__resolve(idx);
    }

    return(NULL != template__block[idx]);
}

extern void
template_readfile(
    const char * filename)
{
    unsigned int b;

    if(NULL != filename)
    {
        /* always load file into first block */
        b = 0;
        if(!template__block[b])
        {
            _kernel_osfile_block fileblk;
            int fileLength;

            if(_kernel_osfile(5, filename, &fileblk) != 1)
                fileLength = -1;
            else
                fileLength = fileblk.start; /* file length */

            reportf("template_readfile(%u:%s): length=%u", strlen(filename), filename, fileLength);

            if(fileLength < (int) sizeof32(TEMPLATE_HEADER))
            {
                werr(TRUE, msgs_lookup("template6" /*"Template file not found"*/));
                return;
            }

            template__readfile_do(b, filename, fileLength);
        }
    }

    /* if there were to be in-memory ones again, we'd have to template__resolve() them here? */

    for(b = 0; b < TEMPLATE__N_BLOCKS; ++b)
        (void) template_ensure(b);
}

static void
template__resolve_icon(
    unsigned int b,
    int pass,
    WimpIconBlockWithBitset * p_icon_ua,
    WimpWindowWithBitset * w_ua,
    size_t * p_indSize /*inout*/,
    size_t * p_newIndDataOffset /*inout*/)
{
    BOOL mutate_flags = FALSE;  /* 06oct96 make for easy design life and RISC OS 2 mutation */
    BOOL nobble_border = FALSE; /* 06oct96 make for easy design life */
    WimpIconFlagsWithBitset iconflags;
    UBF button_type;
    BOOL has_border;

    iconflags.u32 = readval_U32(&p_icon_ua->flags.u32);

    /*tracef3("[template__readfile: looking at icon %u, ptr &%p, flags " U32_XTFMT "]", icon, p_icon_ua, iconflags.u32);*/

    button_type = iconflags.bits.button_type;
    has_border = (0 != iconflags.bits.border);

#ifdef TEMPLATE_FONT_RESOLVE
    if(iconflags.bits.anti_aliased)
        template__convert_font_ref(b, &p_icon_ua->flags);
#else
    assert(!(iconflags.bits.anti_aliased));
#endif

    if(iconflags.bits.indirected)
    {
        char * indData = PtrAddBytes(char *, w_ua, readval_U32(&p_icon_ua->data.it.buffer)); /* NB there will be indirected data (either text or sprite name pointer) */
        int iconLen = template__datalen(indData);
        int reqdLen = readval_U16(&p_icon_ua->data.it.buffer_size);

        if(iconLen != reqdLen)
        {
            tracef1("[template__readfile: icon will need %u bytes]", reqdLen);
            *p_indSize = *p_indSize + reqdLen;
            if(pass == 2)
            {
                /* store address of new copy */
                char * newIndData = template__ind_block[b] + *p_newIndDataOffset;
                memcpy(newIndData, indData, iconLen);
                writeval_U32(&p_icon_ua->data.it.buffer, (U32) newIndData); /* compiler used to barf */
                tracef1("[template__readfile: relocated icon ? data to &%p]", newIndData);
                *p_newIndDataOffset = *p_newIndDataOffset + reqdLen;
            }
        }
        else
            tracef1("[template__readfile: indirected icon ? fits exactly at &%p - no extra allocation]", indData);

        /* 06oct96 - look for some border stuff in the indirected validation string */
        /* NB only on the last pass (else we transform twice) */
        if(iconflags.bits.text)
        {
            if((pass == 2) && has_border)
            {
                S32 indValidOffset = readval_S32(&p_icon_ua->data.it.validation);

                if(indValidOffset > 0)
                {
                    char * indValid = PtrAddBytes(char *, w_ua, indValidOffset);
                    int validLen = template__datalen(indValid); /* forces terminator to CH_NULL so string fns work */
                    char * p;
                    int skip = 0;

                    UNREFERENCED_PARAMETER(validLen);

                    p = indValid;

                    /* search for bo(R)der settings */
                    /* attempt to fix up design border (and button types for RISC OS 3.1 users) */
                    if('R' != *p)
                    {
                        p = strstr(indValid, ";R");
                        skip = 1; /* one more character to skip */
                    }

                    if(NULL != p)
                    {
                        if(wimptx_os_version_query() < RISC_OS_3_5) /* or for testing */
                        {
                            int type;

                            p++;

                            p += skip;

                            switch(type = *p++)
                            {
                            default:  /* normal single pixel border */
#ifndef NDEBUG
                            case '0': /* normal single pixel border */
                            case '1': /* slab out */
                            case '2': /* slab in */
                            case '3': /* ridge */
                            case '4': /* channel */
                            case '7': /* editable field */
#endif
                                break;

                            case '5': /* action button (e.g. Cancel) */
                            case '6': /* default action button (e.g. OK) */
                                {
                                if(*p++ == ',')
                                {
                                    switch(*p++)
                                    {
                                    case '3':
                                        /* mutate into a menu button */
                                        if('6' == type)
                                        {
                                            iconflags.bits.bg_colour = 12; /* default button mutates to fresh cream */

                                            /* reduce the height of this button */
                                            S32 iymin = readval_S32(&p_icon_ua->bbox.ymin);
                                            S32 iymax = readval_S32(&p_icon_ua->bbox.ymax);
                                            iymin += 8;
                                            iymax -= 8;
                                            writeval_S32(&p_icon_ua->bbox.ymin, iymin);
                                            writeval_S32(&p_icon_ua->bbox.ymax, iymax);
                                        }
                                        p[-3] = '1'; /* mutate into R1 */
                                        p[-2] = CH_NULL; /* remove the depressing effect */
                                        iconflags.bits.esg = 1;
                                        iconflags.bits.button_type = 0x09U;
                                        mutate_flags = TRUE;
                                        break;

                                    default:
                                        break;
                                    }
                                }
                                break;
                                }
                            }
                        }
                    }
                    else
                    {   /* no bo(R)der settings */
                        /* remove design-time border */
                        /* NB to retain a border on an icon you must set validstring=" ;R1" */
                        if(wimp_BWRITABLE != button_type)
                            nobble_border = TRUE;
                    }

                    /* search for (S)prite settings (only needed at start) */
                    if('S' == indValid[0])
                    {   /* we have sprite validation section. Nobble standard buttons */
                        if(     0 == strncmp(indValid, "Soptoff,", sizeof("Soptoff,")-1/*CH_NULL*/))
                            nobble_border = TRUE;
                        else if(0 == strncmp(indValid, "Sradiooff,", sizeof("Sradiooff,")-1/*CH_NULL*/))
                            nobble_border = TRUE;
                        else if(0 == strncmp(indValid, "Sup,", sizeof("Sup,")-1/**/))
                            nobble_border = TRUE;
                        else if(0 == strncmp(indValid, "Sdown,", sizeof("Sdown,")-1/*CH_NULL*/))
                            nobble_border = TRUE;
                        else
                            /* otherwise leave well alone - border really required */
                            nobble_border = FALSE;
                    }
                }
                else
                {   /* no validation string at all */
                    /* remove design-time border */
                    nobble_border = TRUE;
                }
            }
        }
    }
    else if(iconflags.bits.text)
    {
        /* simple non-indirected text icon */
        if((pass == 2) && has_border)
        {   /* remove design-time border */
            nobble_border = TRUE;
        }
    }

    if(nobble_border)
    {
        iconflags.bits.border = 0;
        mutate_flags = TRUE;
    }

    if(mutate_flags)
        writeval_U32(&p_icon_ua->flags.u32, iconflags.u32);
}

static void
template__resolve(
    unsigned int b)
{
    unsigned int           pass;
    void *                 spriteArea;
    TEMPLATE_INDEX_ENTRY * ip;
    WimpWindowWithBitset * w_ua;
    size_t                 totalIndSize, indSize;
    size_t                 newIndDataOffset;
    int                    nIcons, icon;

    if(!template__block[b])
        return;

#ifdef TEMPLATE_FONT_RESOLVE
    /* doing fonty stuff? */
    if(template__font_data_offset(b) != -1)
        template_use_fancyfonts();
#endif

    spriteArea = resspr_area();

    pass = 1;

    do  {
        tracef1("\n\n[template_readfile looping over windows: pass %u]", pass);

        totalIndSize = 0;

        newIndDataOffset = 0;

        ip = template__indexentry(b, 0) - 1;

        while((++ip)->dataOffset)
        {
            if(ip->bits.dataType == 1) /* only know about window definitions */
            {
#if TRACE
                if(pass == 1)
                {
                    /* tidy up CR terminated id */
                    char *p = ip->name + sizeof(ip->name);
                    do
                        if(*--p < 32)
                            *p = CH_NULL;
                    while(p > ip->name);
                }
#endif

                if(pass == 2)
                    /* abuse type field to say where indirect data is */
                    ip->bits.indDataOffset = newIndDataOffset;
                else
                    ++template__nTemplates[b];

                /* note that loaded window definitions need NOT be aligned */

                w_ua = (WimpWindowWithBitset *) (template__block[b] + ip->dataOffset);
                tracef2("\n\n[template__readfile: looking at window def &%p, id %12.12s]", w_ua, ip->name);

                indSize = 0;

                { /* relocate title indirection pointer */
                WimpIconFlagsWithBitset iconflags;

                iconflags.u32 = readval_U32(&w_ua->title_flags.u32);

#ifdef TEMPLATE_FONT_RESOLVE
                if(iconflags.bits.anti_aliased)
                    template__convert_font_ref(b, &w_ua->title_flags);
#else
                assert(!(iconflags.bits.anti_aliased));
#endif

                if(iconflags.bits.indirected)
                {
                    char * indData = PtrAddBytes(char *, w_ua, readval_U16(&w_ua->title_data.it.buffer)); /* NB there will be indirected data! */
                    int iconLen = template__datalen(indData);
                    int reqdLen = readval_U16(&w_ua->title_data.it.buffer_size);

                    if(iconLen != reqdLen)
                    {
                        tracef1("[template__readfile: title will need %u bytes]", reqdLen);
                        indSize += reqdLen;
                        if(pass == 2)
                        {
                            /* store address of new copy */
                            char * newIndData = template__ind_block[b] + newIndDataOffset;
                            memcpy(newIndData, indData, iconLen);
                            writeval_U32((P_BYTE) &w_ua->title_data.it.buffer, (U32) newIndData); /* compiler used to barf */
                            tracef1("[template__readfile: relocated title to &%p]", newIndData);
                            newIndDataOffset += reqdLen;
                        }
                    }
                    else
                        tracef0("[template__readfile: indirected title fits exactly - no extra allocation]");
                }
                } /*block*/

                nIcons = readval_U16(&w_ua->nicons);
                tracef1("[template__readfile: window defn has %u icons]", nIcons);

                /* relocate icon indirection pointers and bind sprites */
                for(icon = 0; icon < nIcons; ++icon)
                {
                    WimpIconBlockWithBitset * p_icon_ua = (WimpIconBlockWithBitset *) (w_ua + 1) + icon; /* yes, this is necessary */

                    template__resolve_icon(b, pass, p_icon_ua, w_ua, &indSize, &newIndDataOffset);
                }

                if(pass == 1)
                    totalIndSize += indSize;
                else
                {
                    /* abuse this size field */
                    ip->indDataSize = indSize;
                    tracef3("[template__readfile: poked size field for window %12.12s to %u with %u]", ip->name, ip->indDataSize, indSize);

                    writeval_U32(&w_ua->sprite_area, (U32) spriteArea);
                    tracef1("[template__readfile: Binding sprites to area &%p]", spriteArea);
                }
            }
            else
                *ip->name = CH_NULL; /* kill identifiers of unknown object types */
        }

        if(pass == 1)
        {
            trace_1(TRACE_OUT | TRACE_ANY, "\n\n[template__readfile: need %u bytes indirected workspace]", totalIndSize);

            if(!totalIndSize)
                break;      /* no indirected data - huh! */

            template__ind_block[b] = wlalloc_malloc(totalIndSize);
            if(!template__ind_block[b])
            {
                werr(TRUE, msgs_lookup("template3" /*"No room to load Templates"*/));
                return;
            }
        }
    }
    while(++pass <= 2);
}

/* ---------------------- template_syshandle_unaligned ---------------------
 * Description:   Get a pointer to the underlying window used to create a
 *                template. Note that this may be unaligned!
 *
 * Parameters:    const char *name.
 * Returns:       Pointer to template's underlying window definition (NULL if
 *                template not found).
 * Other Info:    Any changes made to the WimpWindow structure will affect
 *                future windows generated using this template.
 *
 */

extern WimpWindowWithBitset *
template_syshandle_unaligned(
    const char * name)
{
    template *   templateHandle;
    int          iTemplateHandle;
    unsigned int b;

    templateHandle = template_find_new(name);

    if(templateHandle == NULL)
        return(NULL);

    iTemplateHandle = template__ptoi(templateHandle);
    b               = template__block_index(iTemplateHandle);
    iTemplateHandle = template__block_offset(iTemplateHandle);

    /* no block,handle validation needed here */

    return(template__indextowindow(b, template__block_offset(iTemplateHandle)));
}

/*
Set the title of the underlying window definition
*/

extern void
template_settitle(
    template * templateHandle,
    const char * title)
{
    TEMPLATE_INDEX_ENTRY * ip;
    WimpWindowWithBitset * w_ua;
    size_t                 w_size;
    char *                 ptr;
    unsigned int           b, iTemplateHandle;

    iTemplateHandle = template__ptoi(templateHandle);
    b               = template__block_index(iTemplateHandle);
    iTemplateHandle = template__block_offset(iTemplateHandle);

    if(b >= TEMPLATE__N_BLOCKS)
    {
        myassert2x(0, "template_settitle(%u, %u): block index too big",
                   b, iTemplateHandle);
        return;
    }

    if(iTemplateHandle >= template__nTemplates[b])
    {
        myassert3x(0, "template_settitle(%u, %u): nTemplates[b] = %u",
                   b, iTemplateHandle, template__nTemplates[b]);
        return;
    }

    ip     = template__indexentry(b, iTemplateHandle);
    w_ua   = (WimpWindowWithBitset *) (template__block[b] + ip->dataOffset);
    w_size = ip->dataSize;

    /* title data stored relative to start of window defn or as address in indirect block */
    /* NB. not necessarily aligned */
    ptr = (char *) readval_U32(&w_ua->title_data.it.buffer);
    if((int) ptr < w_size)
        ptr += (int) w_ua;

    *ptr = CH_NULL;
    strncat(ptr, title, readval_U16(&w_ua->title_data.it.buffer_size));

    tracef6("[template_settitle(%u) w &%p, size %u -> buffer &%p %s, length %u]",
            iTemplateHandle, w_ua, w_size, ptr, ptr, readval_U16(&w_ua->title_data.it.buffer_size));
}

/* end of cs-templat.c */
