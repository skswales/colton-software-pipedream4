/* spell.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/******************************************************************************
*
* spelling checker for PipeDream
*
* MRJC May 1988
* March 1989 updated to use sparse lists
* July 1989 updated for moveable indexes
* April 1990 internationalisation
*   >>this requires that spell is compiled<<
*   >>with char defaulting to unsigned    <<
* Sept 1991 full event handling added
*
******************************************************************************/

#include "common/gflags.h"

#include "handlist.h"
#include "file.h"
#include "spell.h"

/* local header file */

#define PRAGMA_PACK_2
#include "coltsoft/pragma.h"

#define NAM_SIZE     100    /* maximum size of names */
#define MAX_DICT       5    /* maximum number of dictionaries */
#define EXT_SIZE     500    /* size of disk extend */
#define DICT_NAMLEN   80    /* length of dictionary name */
#define ESC_CHAR     '|'    /* escape character */
#define MIN_CHAR      32    /* minimum character offset */
#define MAX_CHAR      64    /* maximum character offset */
#define MIN_TOKEN     64    /* minium value of token */
#define MAX_TOKEN    256    /* maximum value of token */
#define MAX_INDEX     96    /* maximum elements in index level */
#define MAX_ENDLEN    15    /* maximum ending length */

#define KEYSTR "[Colton Soft]"

#define SPELL_MAXITEMSIZE 32700
#define SPELL_MAXPOOLSIZE 32700

#define END_MAXITEMSIZE   500
#define END_MAXPOOLSIZE   5000

typedef struct CACHEBLOCK * CACHEP;
typedef struct IXOFDICT * DIXP;
typedef struct ENDSTRUCT * ENDP;
typedef struct I_ENDSTRUCT * I_ENDP;
typedef struct IXSTRUCT * SIXP;
typedef struct TOKWORD * WRDP;

/*
cached block structure
*/

struct CACHEBLOCK
{
    S32  usecount;
    DICT_NUMBER dict_number;
    S32  lettix;
    S32  diskaddress;
    S32  diskspace;

    char data[1];
};

/*
structure of an index to a dictionary
*/

struct IXSTRUCT
{
    union IXSTRUCT_P
    {
        LIST_ITEMNO cacheno;
        S32 disk;
    } p;
    S16  blklen;
    char letflags;
};

/* letter flags */

#define LET_CACHED 0x80
#define LET_LOCKED 0x40
#define LET_ONE    0x20
#define LET_TWO    0x10
#define LET_WRITE     8

/*
structure of the index of dictionaries
*/

typedef struct DICT
{
    FILE_HANDLE file_handle_dict;       /* handle of dictionary file */
    ARRAY_HANDLE dicth;                 /* handle of index memory */
    S32 dictsize;                       /* size of dictionary on disk */
    LIST_BLOCK dict_end_list;           /* list block for ending list */
    char char_offset;                   /* character offset */
    char token_start;                   /* number of first token */
    char man_token_start;               /* manually supplied token start */
    S32 index_offset;                   /* offset of index start */
    S32 data_offset;                    /* offset of data start */
    S32 n_index;                        /* number of index entries */
    char n_index_1;                     /* number of first level elements */
    char n_index_2;                     /* number of second level elements */
    char dict_name[DICT_NAMLEN + 1];    /* dictionary name and copyright */
    char letter_1[MAX_INDEX];           /* mapping of chars for 1st letter */
    char letter_2[MAX_INDEX];           /* mapping of chars for 2nd letter */
    char case_map[256];                 /* case equivalence map */
    char dictflags;                     /* dictionary flags */
    char dict_filename[BUF_MAX_PATHSTRING];/* dictionary filename */
}
DICT, * P_DICT, ** P_P_DICT;

#define P_DICT_NONE _P_DATA_NONE(P_DICT)

/*
tokenised word structure
*/

struct TOKWORD
{
    S32 len;
    S32 lettix;
    S32 tail;

    char body[MAX_WORD];
    char bodyd[MAX_WORD];
    char bodydp[MAX_WORD];

    S32 fail;          /* reason for failure of search */
    S32 findpos;       /* offset from start of data of root */
    S32 matchc;        /* characters at start of root matched */
    S32 match;         /* whole root matched ? */
    S32 matchcp;       /* characters at start of previous root matched */
    S32 matchp;        /* whole of previous root matched ? */
};

/*
ending structure
*/

typedef struct ENDSTRUCT
{
    char len;
    char alpha;
    char pos;
    char ending[MAX_ENDLEN + 1];
}
ENDSTRUCT;

/*
internal ending structure
*/

struct I_ENDSTRUCT
{
    char len;
    char alpha;
    char ending[1];
};

/*
insert codes
*/

#define INS_WORD        1
#define INS_TOKENCUR    2
#define INS_TOKENPREV   3
#define INS_STARTLET    4

#define PRAGMA_PACK
#include "coltsoft/pragma.h"

#if !defined(__CHAR_UNSIGNED__)
#error char must be unsigned
#endif

/*
internal functions
*/

static BOOL
badcharsin(
    _InRef_     P_DICT p_dict,
    _In_z_      const char *str);

_Check_return_
static STATUS
char_ordinal_1(
    _InRef_     P_DICT p_dict,
    _InVal_     S32 ch);

_Check_return_
static STATUS
char_ordinal_2(
    _InRef_     P_DICT p_dict,
    _InVal_     S32 ch);

static S32
def_file_position(
    FILE_HANDLE def_file);

static S32
delreins(
    _InRef_     P_DICT p_dict,
    _In_z_      const char *word,
    WRDP wp);

static S32
endmatch(
    _InRef_     P_DICT p_dict,
    char *word,
    _In_opt_z_  const char *mask,
    S32 updown);

static S32
ensuredict(
    _InRef_     P_DICT p_dict);

static S32
fetchblock(
    _InRef_     P_DICT p_dict,
    S32 lettix);

static S32
freecache(
    S32 lettix);

static S32
get_dict_entry(
    _OutRef_    P_P_DICT p_p_dict);

static S32
initmatch(
    _InRef_     P_DICT p_dict,
  /*_Out_z_*/   char *wordout,
    _In_z_      const char *wordin,
    _In_opt_z_  const char *mask);

#define iswordc_us(p_dict, ch) ( \
    (p_dict)->case_map[(ch)] )

static S32
killcache(
    LIST_ITEMNO cacheno);

static S32
load_dict_def(
    _InoutRef_  P_DICT p_dict);

static S32
load_dict_def_now(
    FILE_HANDLE def_file,
    _InoutRef_  P_DICT p_dict,
    S32 keylen);

static S32
lookupword(
    _InRef_     P_DICT p_dict,
    WRDP wp,
    S32 needpos);

static WRDP
makeindex(
    _InRef_     P_DICT p_dict,
    WRDP wp,
    _In_z_      PC_U8Z word);

static S32
matchword(
    _InRef_     P_DICT p_dict,
    _In_opt_z_  const char *mask,
    char *word);

_Check_return_
static STATUS
nextword(
    _InRef_     P_DICT p_dict,
    char *word);

_Check_return_
static S32
ordinal_char_1(
    _InRef_     P_DICT p_dict,
    _InVal_     S32 ord);

_Check_return_
static S32
ordinal_char_2(
    _InRef_     P_DICT p_dict,
    _InVal_     S32 ord);

_Check_return_
static S32
ordinal_char_3(
    _InRef_     P_DICT p_dict,
    _InVal_     S32 ord);

_Check_return_
static S32
prevword(
    _InRef_     P_DICT p_dict,
    char *word);

static S32
read_def_line(
    FILE_HANDLE def_file,
    char *buffer);

static void
release_dict_entry(
    _InoutRef_  P_DICT p_dict);

static S32
setabval(
    _InRef_     P_DICT p_dict,
    WRDP wp,
    S32 lo_hi);

static void
stuffcache(
    _InRef_     P_DICT p_dict);

static void
tokenise(
    _InRef_     P_DICT p_dict,
    WRDP wp,
    S32 rootlen);

static S32
tolower_us(
    _InRef_     P_DICT p_dict,
    _InVal_     S32 ch);

#define toupper_us(p_dict, ch) ( \
    (p_dict)->case_map[(ch)] \
        ? (p_dict)->case_map[(ch)] \
        : (ch) )

static S32
writeblock(
    CACHEP cp);

static S32
writeindex(
    _InRef_     P_DICT p_dict,
    _InVal_     S32 lettix);

/*
static variables
*/

static P_LIST_BLOCK cachelp = NULL;             /* list of cached blocks */
static DICT dictlist[MAX_DICT];                 /* dictionary table */
static S32 spell_addword_nestf = 0;             /* addword nest level */
static S32 compar_dict;                         /* dictionary compar needs */
static S32 full_event_registered = 0;           /* we've registered interest in full events */
static S32 cache_lock = 0;                      /* ignore full events for a mo (!) */

#define dict_number(p_dict) ( \
    (p_dict) - &dictlist[0] )

_Check_return_
static inline STATUS
dict_validate(
    _OutRef_    P_P_DICT p_p_dict,
    _InVal_     DICT_NUMBER dict_number)
{
    *p_p_dict = P_DICT_NONE;

    if( ((U32) dict_number < (U32) MAX_DICT) && (NULL != (*p_p_dict = &dictlist[dict_number])->file_handle_dict) )
        return(STATUS_OK);

    return(create_error(SPELL_ERR_BADDICT));
}

/*
macro to get index pointer given
either dictionary number or pointer
*/

#define ixpdv(p_dict, ix) array_index_is_valid(&(p_dict)->dicth, ix)
#define ixpdp(p_dict, ix) array_ptr(&(p_dict)->dicth, struct IXSTRUCT, ix)

#define ixv(dict, ix) ixpdv(&dictlist[(dict)], ix)
#define ixp(dict, ix) ixpdp(&dictlist[(dict)], ix)

#define SPACE 32

/******************************************************************************
*
* add a word to a dictionary
*
* --out--
* >0 word added
*  0 word exists
* <0 error
*
******************************************************************************/

_Check_return_
extern STATUS
spell_addword(
    _InVal_     DICT_NUMBER dict_number,
    _In_z_      const char *word)
{
    struct TOKWORD newword;
    S32 res, wordsize, err, rootlen;
    char token_start;
    P_U8 newpos;
    const char *ci;
    CACHEP cp;
    P_LIST_ITEM it;
    SIXP lett;
    P_DICT p_dict;

    status_return(dict_validate(&p_dict, dict_number));

    if(!makeindex(p_dict, &newword, word))
        return(create_error(SPELL_ERR_BADWORD));

    /* check if word exists and get position */
    status_return(res = lookupword(p_dict, &newword, TRUE));

    /* check if dictionary is read only */
    if(p_dict->dictflags & DICT_READONLY)
        return(create_error(SPELL_ERR_READONLY));

    if(res)
        return(STATUS_OK);

    token_start = p_dict->token_start;

    switch(newword.len)
    {
    /* single letter word */
    case 1:
        assert(ixpdv(p_dict, newword.lettix));
        ixpdp(p_dict, newword.lettix)->letflags |= LET_ONE;
        break;

    /* two letter word */
    case 2:
        assert(ixpdv(p_dict, newword.lettix));
        ixpdp(p_dict, newword.lettix)->letflags |= LET_TWO;
        break;

    /* all other words */
    default:
        /* tokenise the word to be inserted */
        rootlen = (newword.fail == INS_STARTLET)
                  ? MAX(newword.matchc, 1)
                  : MAX(newword.matchcp, 1);
        tokenise(p_dict, &newword, rootlen);

        /* check if the root matches the current or previous words */
        rootlen = strlen(newword.body);
        if(newword.fail == INS_WORD)
        {
            if(newword.match)
            {
                if(rootlen == newword.matchc)
                    newword.fail = INS_TOKENCUR;
            }
            else if(newword.matchp)
            {
                if(rootlen == newword.matchcp)
                    newword.fail = INS_TOKENPREV;
                else if(!spell_addword_nestf && (rootlen > newword.matchcp))
                {
                    P_U8 pos;
                    struct TOKWORD delword;

                    delword = newword;
                    strcpy(delword.bodyd, delword.bodydp);

                    assert(ixpdv(p_dict, delword.lettix));
                    cp = (CACHEP) list_gotoitem(cachelp,
                                                ixpdp(p_dict, delword.lettix)->
                                                p.cacheno)->i.inside;

                    pos = cp->data + delword.findpos;

                    /* skip back to start of unit */
                    while(delword.findpos && (*--pos >= token_start))
                        --delword.findpos;

                    spell_addword_nestf = 1;
                    if((res = delreins(p_dict, word, &delword)) < 0)
                        return(res);
                    spell_addword_nestf = 0;
                    if(res)
                        break;
                }
            }
        }

        /* calculate space needed for word */
        switch(newword.fail)
        {
        /* 1 byte for root count,
           n bytes for body,
           1 byte for token */
        case INS_STARTLET:
            wordsize = 1 + rootlen - newword.matchc + 1;
            break;

        case INS_WORD:
            wordsize = 1 + rootlen - newword.matchcp + 1;
            break;

        /* 1 byte for token */
        case INS_TOKENPREV:
        case INS_TOKENCUR:
            wordsize = 1;
            break;
        }

        /* check we have a cache block */
        if((err = fetchblock(p_dict, newword.lettix)) != 0)
            return(err);

        /* add word to cache block */
        err = 0;
        cache_lock = 1;
        for(;;)
        {
            /* loop to get some memory */
            assert(ixpdv(p_dict, newword.lettix));
            lett = ixpdp(p_dict, newword.lettix);

            if((it = list_createitem(cachelp,
                                     lett->p.cacheno,
                                     lett->blklen +
                                     wordsize +
                                     sizeof(struct CACHEBLOCK),
                                     FALSE)) != NULL)
            {
                cp = (CACHEP) it->i.inside;
                break;
            }

            if((err = freecache(newword.lettix)) < 0)
                break;
        }
        cache_lock = 0;

        if(err < 0)
            return(err);

        /* find place to insert new word */
        newpos = cp->data + newword.findpos;
        switch(newword.fail)
        {
        case INS_TOKENCUR:
            /* skip to tokens of current word */
            while(*newpos < token_start)
            {
                ++newpos;
                ++newword.findpos;
            }
            break;

        case INS_TOKENPREV:
        case INS_WORD:
        case INS_STARTLET:
            break;
        }

        /* make space for new word */
        assert(ixpdv(p_dict, newword.lettix));
        lett = ixpdp(p_dict, newword.lettix);
        err = lett->blklen - newword.findpos;
        memmove32(newpos + wordsize,
                  newpos,
                  lett->blklen - newword.findpos);

        lett->blklen += wordsize;

        /* move in word */
        switch(newword.fail)
        {
        case INS_STARTLET:
            *newpos++ = CH_NULL;
            ci = newword.body;
            while(*ci)
                *newpos++ = *ci++;
            *newpos++ = (char) newword.tail;
            *newpos++ = (char) newword.matchc;
            break;

        /* note fall thru */
        case INS_WORD:
            *newpos++ = (char) newword.matchcp;
            ci = newword.body + newword.matchcp;
            while(*ci)
                *newpos++ = *ci++;

        /* note fall thru */
        case INS_TOKENPREV:
        case INS_TOKENCUR:
            *newpos++ = (char) newword.tail;
            break;
        }

        break;
    }

    /* mark that it needs a write */
    assert(ixpdv(p_dict, newword.lettix));
    ixpdp(p_dict, newword.lettix)->letflags |= LET_WRITE;

    return(1);
}

/******************************************************************************
*
* check if the word is in the dictionary
*
******************************************************************************/

_Check_return_
extern STATUS
spell_checkword(
    _InVal_     DICT_NUMBER dict_number,
    _In_z_      const char *word)
{
    struct TOKWORD curword;
    P_DICT p_dict;

    status_return(dict_validate(&p_dict, dict_number));

    if(!makeindex(p_dict, &curword, word))
        return(create_error(SPELL_ERR_BADWORD));

    /* check if word exists and get position */
    return(lookupword(p_dict, &curword, FALSE));
}

/******************************************************************************
*
* close a dictionary
*
******************************************************************************/

_Check_return_
extern STATUS
spell_close(
    _InVal_     DICT_NUMBER dict_number)
{
    P_DICT p_dict;
    FILE_HANDLE file_handle_dict;
    STATUS err;

    trace_0(TRACE_MODULE_SPELL,"spell_close");

    if((U32) dict_number >= (U32) MAX_DICT)
        return(create_error(SPELL_ERR_BADDICT));

    if(NULL == (file_handle_dict = (p_dict = &dictlist[dict_number])->file_handle_dict))
    {
        trace_1(TRACE_OUT | TRACE_MODULE_SPELL, "spell_close called to close non-open dictionary: %d", (int) dict_number);
        return(create_error(SPELL_ERR_CANTCLOSE));
    }

    /* write out any modified part */
    err = ensuredict(p_dict);

    /* make sure no cache blocks left */
    stuffcache(p_dict);

    /* close file on media */
    status_accumulate(err, file_close(&file_handle_dict));

    release_dict_entry(p_dict);

    return(err);
}

/******************************************************************************
*
* create new dictionary
*
******************************************************************************/

_Check_return_
extern STATUS
spell_createdict(
    _In_z_      PCTSTR name,
    _In_z_      PCTSTR def_name)
{
    STATUS err;
    P_DICT p_dict;
    FILE_HANDLE def_file_handle = NULL;
    S32 nbytes, i, res;
    struct IXSTRUCT wix;
    char buffer[255 + 1];

    /* first check to see if it already exists */
    if(file_is_file(name))
        return(create_error(SPELL_ERR_EXISTS));

    /* get a dictionary number */
    status_return(err = get_dict_entry(&p_dict));

    for(;;) /* loop for structure */
    {
        /* try to open definition file (could be definition file or another dictionary) */
        status_break(err = file_open(def_name, file_open_read, &def_file_handle));
        if(NULL == def_file_handle)
        {
            err = create_error(SPELL_ERR_CANTOPENDEFN);
            break;
        }

        /* position to read definition file content */
        trace_0(TRACE_MODULE_SPELL, "spell_createdict about to def_file_position");
        status_break(err = def_file_position(def_file_handle));
        trace_1(TRACE_MODULE_SPELL, "spell_createdict def_file_position returned: %d", err);

        /* create a blank file */
        status_break(err = file_open(name, file_open_write, &p_dict->file_handle_dict));

        trace_0(TRACE_MODULE_SPELL, "spell_createdict about to write KEYSTR");

        /* write out file identifier */
        nbytes = strlen(KEYSTR);
        status_break(err = file_write_bytes(KEYSTR, nbytes, p_dict->file_handle_dict));

        /* copy across definition file content */
        for(;;)
        {
            if(0 == (res = read_def_line(def_file_handle, buffer)))
                break;

            /* stop on error */
            if(res < 0)
            {
                err = res;
                goto error;
            }

            trace_1(TRACE_MODULE_SPELL, "spell_createdict def line: %s", trace_string(buffer));

            if(status_fail(err = file_write_bytes(buffer, res, p_dict->file_handle_dict)))
                goto error;
        }

        trace_1(TRACE_MODULE_SPELL, "spell_createdict after def file: %d", res);

        /* write out definition end byte */
        status_break(err = file_putc(0, p_dict->file_handle_dict));

        /* write out dictionary flag byte */
        status_break(err = file_putc(0, p_dict->file_handle_dict));

        /* close file so far */
        status_break(err = file_close(&p_dict->file_handle_dict));

        p_dict->file_handle_dict = 0;

        /* re-open for update */
        status_break(err = file_open(name, file_open_readwrite, &p_dict->file_handle_dict));
        if(NULL == p_dict->file_handle_dict)
        {
            err = create_error(SPELL_ERR_CANTOPEN);
            break;
        }

        /* process definition from this new file */
        if((res = load_dict_def(p_dict)) < 0)
        {
            err = res;
            break;
        }

        status_break(err = file_seek(p_dict->file_handle_dict, p_dict->index_offset, SEEK_SET));

        /* get a blank structure */
        wix.p.disk   = 0;
        wix.blklen   = 0;
        wix.letflags = 0;

        /* write out blank structures */
        for(i = 0; i < p_dict->n_index; ++i)
        {
            if(status_fail(err = file_write_bytes(&wix, sizeof32(struct IXSTRUCT), p_dict->file_handle_dict)))
            {
                trace_1(TRACE_MODULE_SPELL, "spell_createdict failed to write out index entry %d", i);
                goto error;
            }
        }

        err = file_flush(p_dict->file_handle_dict);

        break; /* out of loop for structure */
    }

error:
    trace_1(TRACE_MODULE_SPELL, "spell_createdict err: %d", err);

    if(p_dict->file_handle_dict)
        status_accumulate(err, file_close(&p_dict->file_handle_dict));

    if(def_file_handle)
        status_consume(file_close(&def_file_handle));

    /* get rid of our entry */
    release_dict_entry(p_dict);

    if(err < 0)
    {
        remove(name);
        return(err);
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* delete a word from a dictionary
*
******************************************************************************/

_Check_return_
extern STATUS
spell_deleteword(
    _InVal_     DICT_NUMBER dict_number,
    _In_z_      const char *word)
{
    struct TOKWORD curword;
    S32 res, delsize, err, tokcount, addroot, blockbefore, i;
    char token_start, char_offset;
    P_U8 sp, ep, datap, endword, ci, co;
    CACHEP cp;
    SIXP lett;
    P_DICT p_dict;

    status_return(dict_validate(&p_dict, dict_number));

    if(!makeindex(p_dict, &curword, word))
        return(create_error(SPELL_ERR_BADWORD));

    /* check if word exists and get position */
    if((res = lookupword(p_dict, &curword, TRUE)) < 0)
        return(res);

    /* check if dictionary is read only */
    if(p_dict->dictflags & DICT_READONLY)
        return(create_error(SPELL_ERR_READONLY));

    if(!res)
        return(create_error(SPELL_ERR_NOTFOUND));

    assert(ixpdv(p_dict, curword.lettix));
    lett = ixpdp(p_dict, curword.lettix);
    lett->letflags |= LET_WRITE;

    /* deal with 1 and 2 letter words */
    if(curword.len == 1)
    {
        lett->letflags &= ~LET_ONE;
        return(0);
    }

    if(curword.len == 2)
    {
        lett->letflags &= ~LET_TWO;
        return(0);
    }

    /* check we have a cache block */
    if((err = fetchblock(p_dict, curword.lettix)) != 0)
        return(err);

    token_start = p_dict->token_start;
    char_offset = p_dict->char_offset;

    /* after succesful find, the pointer points
    at the token of the word found */
    assert(ixpdv(p_dict, curword.lettix));
    lett = ixpdp(p_dict, curword.lettix);
    cp = (CACHEP) list_gotoitem(cachelp, lett->p.cacheno)->i.inside;
    sp = cp->data;

    datap = sp + curword.findpos;
    ep = sp + lett->blklen;

    /* count the tokens */
    while(*datap >= token_start)
        --datap;

    ++datap;
    tokcount = 0;
    while((*datap >= token_start) && (datap < ep))
    {
        ++datap;
        ++tokcount;
    }
    endword = datap;

    /* calculate bytes to delete */
    if(tokcount == 1)
    {
        /* move to beginning of word */
        --datap;
        while(*datap >= char_offset)
            --datap;

        /* last word in the block ? */
        if(endword == ep)
            delsize = endword - datap;
        else
        {
            if(*endword <= *datap)
                delsize = endword - datap;
            else
            {
                /* copy across the extra root required */
                addroot = ((S32) *endword - (S32) *datap) + 1;
                delsize = endword - datap - addroot + 1;
                ci = datap + addroot;
                co = endword + 1;
                for(i = 0; i < addroot; ++i)
                    *--co = *--ci;
            }
        }
    }
    else
    {
        delsize = 1;
        datap = sp + curword.findpos;
    }

    assert(ixpdv(p_dict, curword.lettix));
    lett = ixpdp(p_dict, curword.lettix);
    blockbefore = (datap - sp) + delsize;
    memmove32(datap, datap + delsize, lett->blklen - blockbefore);

    lett->letflags |= LET_WRITE;
    lett->blklen -= delsize;
    return(0);
}

/******************************************************************************
*
* flush a dictionary
*
******************************************************************************/

_Check_return_
extern STATUS
spell_flush(
    _InVal_     DICT_NUMBER dict_number)
{
    P_DICT p_dict;
    FILE_HANDLE file_handle_dict;
    STATUS err;

    trace_0(TRACE_MODULE_SPELL, "spell_flush");

    if((U32) dict_number >= (U32) MAX_DICT)
        return(create_error(SPELL_ERR_BADDICT));

    if(NULL == (file_handle_dict = (p_dict = &dictlist[dict_number])->file_handle_dict))
    {
        trace_1(TRACE_OUT | TRACE_MODULE_SPELL, "spell_flush called to flush non-open dictionary: %d", (int) dict_number);
        return(create_error(SPELL_ERR_CANTWRITE));
    }

    /* write out any modified part */
    err = ensuredict(p_dict);

    /* make sure no cache blocks left */
    stuffcache(p_dict);

    return(err);
}

/******************************************************************************
*
* call back from memory manager to tell us to free some memory
*
******************************************************************************/

static U32
spell_full_events(
    _InVal_     U32 n_bytes)
{
    U32 bytes_freed;

    bytes_freed = 0;

    if(!cache_lock)
    {
        do  {
            bytes_freed += list_packlist(cachelp, n_bytes);

            if(bytes_freed > n_bytes)
                break;
        }
        while(freecache(-1) > 0);
    }

    trace_1(TRACE_MODULE_SPELL, "spell_full_events freed total of %u bytes", bytes_freed);

    return(bytes_freed);
}

/******************************************************************************
*
* report whether a character is part of a valid spellcheck word
*
******************************************************************************/

_Check_return_
extern STATUS
spell_iswordc(
    _InVal_     DICT_NUMBER dict_number,
    S32 ch)
{
    P_DICT p_dict;

    status_return(dict_validate(&p_dict, dict_number));

    return(iswordc_us(p_dict, ch));
}

/******************************************************************************
*
* load a dictionary
*
* all the dictionary is loaded and locked into place
*
******************************************************************************/

_Check_return_
extern STATUS
spell_load(
    _InVal_     DICT_NUMBER dict_number)
{
    S32 err, i;
    P_DICT p_dict;

    status_return(dict_validate(&p_dict, dict_number));

    for(i = 0; i < p_dict->n_index; ++i)
    {
        /* is there a block defined ? */
        assert(ixpdv(p_dict, i));
        if(!ixpdp(p_dict, i)->blklen)
            continue;

        if((err = fetchblock(p_dict, i)) != 0)
            return(err);

        assert(ixpdv(p_dict, i));
        ixpdp(p_dict, i)->letflags |= LET_LOCKED;
    }

    return(0);
}

/******************************************************************************
*
* return the next word in a dictionary
*
* --in--
* wordout must point to a character
* buffer at least MAX_WORD long
*
* --out--
* <0 error
* =0 end of dictionary
* >0 word returned
*
******************************************************************************/

_Check_return_
extern STATUS
spell_nextword(
    _InVal_     DICT_NUMBER dict_number,
    char *wordout,
    _In_z_      const char *wordin,
    _In_opt_z_  const char *mask,
    _InoutRef_  P_S32 brkflg)
{
    STATUS res;
    S32 gotw;
    P_DICT p_dict;

    trace_5(TRACE_MODULE_SPELL, "spell_nextword(%d, &%p, %s, %s, &%p)",
            (int) dict_number, report_ptr_cast(wordout), trace_string(wordin), trace_string(mask), report_ptr_cast(brkflg));

    status_return(dict_validate(&p_dict, dict_number));

    if(badcharsin(p_dict, wordin))
        return(create_error(SPELL_ERR_BADWORD));

    if(mask && badcharsin(p_dict, mask))
        return(create_error(SPELL_ERR_BADWORD));

    /* check for start of dictionary */
    gotw = initmatch(p_dict, wordout, wordin, mask);

    if(gotw)
        if((gotw = spell_checkword(dict_number, wordout)) < 0)
            return(gotw);

    do  {
        if(*brkflg)
            return(create_error(SPELL_ERR_ESCAPE));

        if(!gotw)
        {
            status_return(res = nextword(p_dict, wordout));
        }
        else
        {
            res = 1;
            gotw = 0;
        }
    }
    while(res &&
          matchword(p_dict, mask, wordout) &&
          ((res = !endmatch(p_dict, wordout, mask, 1)) != 0));

    /* return blank word at end */
    if(!res)
        *wordout = CH_NULL;

    trace_2(TRACE_MODULE_SPELL, "spell_nextword yields %s, res = %d", wordout, res);
    return(res);
}

/******************************************************************************
*
* open a dictionary
*
* --out--
* dictionary handle
*
******************************************************************************/

_Check_return_
extern STATUS
spell_opendict(
    _In_z_      PCTSTR name,
    _Out_opt_   char **copy_right)
{
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(struct IXSTRUCT), TRUE);

    STATUS err;
    S32 fpos;
    DICT_NUMBER dict_number;
    P_DICT p_dict;
    S32 i, nmemb;
    SIXP lett, tempix;

#if defined(CHECKING)
    { /* trap chars being signed */
    static char ch;
    ch = (char) '\x80';
    assert(ch > 127);
    } /*block*/
#endif

    if(copy_right)
        *copy_right = NULL;

    /* get a dictionary number */
    if((dict_number = get_dict_entry(&p_dict)) < 0)
        return(dict_number);

    for(;;) /* loop for structure */
    {
        /* look for the file */
        status_break(err = file_open(name, file_open_read, &p_dict->file_handle_dict));
        if(NULL == p_dict->file_handle_dict)
        {
            err = create_error(SPELL_ERR_CANTOPEN);
            break;
        }

        /* register interest in full events */
        if(!full_event_registered)
        {
            status_assert(al_full_event_client_register(spell_full_events));
            full_event_registered = 1;
        }

        /* take copy of name for reopening in setoptions */
        xstrkpy(p_dict->dict_filename, elemof32(p_dict->dict_filename), name);

        /* load dictionary definition */
        status_break(err = load_dict_def(p_dict));

        if(NULL == al_array_alloc(&p_dict->dicth, struct IXSTRUCT, p_dict->n_index, &array_init_block, &err))
            break;

        /* load index */
        tempix = array_base(&p_dict->dicth, struct IXSTRUCT);
        nmemb  = p_dict->n_index;
        status_break(err = file_read(tempix, sizeof(struct IXSTRUCT), nmemb, p_dict->file_handle_dict));

        /* read size of dictionary file */
        status_break(err = file_seek(p_dict->file_handle_dict, 0, SEEK_END));

        if((fpos = file_tell(p_dict->file_handle_dict)) < 0)
        {
            err = fpos;
            break;
        }

        p_dict->dictsize = fpos - p_dict->data_offset;

        /* if dictionary can be updated, re-open for update */
        if(!(p_dict->dictflags & DICT_READONLY))
        {
            file_close(&p_dict->file_handle_dict);

            /* look for the file */
            if(file_open(name, file_open_readwrite, &p_dict->file_handle_dict), p_dict->file_handle_dict == NULL)
            {
                err = create_error(SPELL_ERR_CANTOPEN);
                break;
            }
        }

        break; /* out of loop for structure */
    }

    if(status_fail(err))
    {
        if(p_dict->file_handle_dict)
            status_consume(file_close(&p_dict->file_handle_dict));
        release_dict_entry(p_dict);
        return(err);
    }

    /* return copyright string */
    if(copy_right)
        *copy_right = p_dict->dict_name;

    /* loop over index, masking off unwanted bits */
    assert(ixpdv(p_dict, 0));
    for(i = 0, lett = ixpdp(p_dict, 0); i < p_dict->n_index; ++i, ++lett)
        lett->letflags &= (LET_ONE | LET_TWO);

    trace_1(TRACE_MODULE_SPELL, "spell_open returns: %d", (int) dict_number);

    return(dict_number);
}

/******************************************************************************
*
* pack a dictionary
*
******************************************************************************/

_Check_return_
extern STATUS
spell_pack(
    _InVal_     DICT_NUMBER old_dict_number,
    _InVal_     DICT_NUMBER new_dict_number)
{
    P_DICT olddp;
    P_DICT newdp;
    STATUS err;
    S32 i;
    S32 diskpoint;
    CACHEP cp;
    SIXP lettin;
    SIXP lettout;

    status_return(dict_validate(&olddp, old_dict_number));
    status_return(dict_validate(&newdp, new_dict_number));

    status_return(ensuredict(olddp));

    diskpoint = newdp->data_offset;
    err = 0;

    for(i = 0; i < olddp->n_index; ++i)
    {
        assert(ixpdv(olddp, i));
        lettin  = ixpdp(olddp, i);

        assert(ixpdv(newdp, i));
        lettout = ixpdp(newdp, i);

        /* if no block, copy index entries and continue */
        if(!lettin->blklen)
        {
            *lettout = *lettin;
            lettin->letflags &= LET_ONE | LET_TWO;
            lettout->letflags |= LET_WRITE;
            continue;
        }

        if((err = fetchblock(olddp, i)) != 0)
            break;

        /* re-load index pointers */
        assert(ixpdv(olddp, i));
        lettin  = ixpdp(olddp, i);

        assert(ixpdv(newdp, i));
        lettout = ixpdp(newdp, i);

        /* clear input index flags */
        *lettout = *lettin;
        lettin->letflags &= LET_ONE | LET_TWO;

        cp = (CACHEP) list_gotoitem(cachelp, lettin->p.cacheno)->i.inside;

        /* output index takes over block read from input index */
        lettin->p.disk  = cp->diskaddress;
        cp->diskaddress = diskpoint;
        diskpoint      += lettout->blklen + sizeof(S32);
        cp->diskspace   = lettout->blklen;
        cp->dict_number = new_dict_number;
        cp->lettix = i;
        lettout->letflags |= LET_WRITE;
    }

    newdp->dictflags = olddp->dictflags;

    return(err);
}

/******************************************************************************
*
* return the previous word
*
******************************************************************************/

_Check_return_
extern STATUS
spell_prevword(
    _InVal_     DICT_NUMBER dict_number,
    char *wordout,
    _In_z_      const char *wordin,
    _In_opt_z_  const char *mask,
    _InoutRef_  P_S32 brkflg)
{
    STATUS res;
    P_DICT p_dict;

    trace_5(TRACE_MODULE_SPELL, "spell_prevword(%d, &%p, %s, %s, &%p)",
            (int) dict_number, report_ptr_cast(wordout), trace_string(wordin), trace_string(mask), report_ptr_cast(brkflg));

    status_return(dict_validate(&p_dict, dict_number));

    if(badcharsin(p_dict, wordin))
        return(create_error(SPELL_ERR_BADWORD));

    if(mask && badcharsin(p_dict, mask))
        return(create_error(SPELL_ERR_BADWORD));

    (void) initmatch(p_dict, wordout, wordin, mask);

    do  {
        if(*brkflg)
            return(create_error(SPELL_ERR_ESCAPE));

        status_return(res = prevword(p_dict, wordout));
    }
    while(res &&
          matchword(p_dict, mask, wordout) &&
          ((res = !endmatch(p_dict, wordout, mask, 0)) != 0));

    /* return blank word at start */
    if(!res)
        *wordout = CH_NULL;

    trace_2(TRACE_MODULE_SPELL, "spell_prevword yields %s, res = %d", wordout, res);
    return(res);
}

/******************************************************************************
*
* set dictionary options
*
******************************************************************************/

_Check_return_
extern STATUS
spell_setoptions(
    _InVal_     DICT_NUMBER dict_number,
    _InVal_     U32 optionset,
    _InVal_     U32 optionmask)
{
    P_DICT p_dict;

    status_return(dict_validate(&p_dict, dict_number));

    /* may need to open for update! */
    if(p_dict->dictflags & DICT_READONLY)
    {
        if(status_ok(file_close(&p_dict->file_handle_dict)))
            status_consume(file_open(p_dict->dict_filename, file_open_readwrite, &p_dict->file_handle_dict));
        else
            p_dict->file_handle_dict = NULL;

        if(NULL == p_dict->file_handle_dict)
        {
            /* if failed to open for update, reopen for reading */
            status_return(file_open(p_dict->dict_filename, file_open_read, &p_dict->file_handle_dict));
            return(create_error(SPELL_ERR_CANTWRITE));
        }
    }

    p_dict->dictflags &= (char) optionmask;
    p_dict->dictflags |= (char) optionset;

    status_return(file_seek(p_dict->file_handle_dict, p_dict->index_offset - 1, SEEK_SET));

    status_return(file_putc(p_dict->dictflags & DICT_READONLY, p_dict->file_handle_dict));

    return(STATUS_OK);
}

/******************************************************************************
*
* return statistics about spelling checker
*
******************************************************************************/

extern void
spell_stats(
    _OutRef_    P_S32 cblocks,
    _OutRef_    P_S32 largest,
    _OutRef_    P_S32 totalmem)
{
    LIST_ITEMNO cacheno = 0;
    P_LIST_ITEM it;

    *cblocks = 0;
    *largest = 0;
    *totalmem = 0;

    if((it = list_initseq(cachelp, &cacheno)) != NULL)
    {
        do  {
            CACHEP cp = (CACHEP) it->i.inside;
            S32 blksiz;

            ++(*cblocks);
            assert(ixv(cp->dict_number, cp->lettix));
            blksiz = sizeof(struct CACHEBLOCK) +
                     ixp(cp->dict_number, cp->lettix)->blklen;
            *largest = MAX(*largest, blksiz);
            *totalmem += blksiz;
        }
        while((it = list_nextseq(cachelp, &cacheno)) != NULL);
    }
}

/******************************************************************************
*
* compare two strings using the case and order
* information for a dictionary
*
******************************************************************************/

_Check_return_
static int
strnicmp_us(
    _InoutRef_  P_DICT p_dict,
    _In_        PC_U8 word1,
    _In_        PC_U8 word2,
    _In_        U32 len)
{
    STATUS ch1, ch2;

    for(;;)
    {
        U8 u8_1 = *word1++;
        U8 u8_2 = *word2++;

        u8_1 = (U8) toupper_us(p_dict, u8_1);
        u8_2 = (U8) toupper_us(p_dict, u8_2);

        ch1 = char_ordinal_2(p_dict, u8_1);
        ch2 = char_ordinal_2(p_dict, u8_2);

        if(ch1 != ch2)
            break;

        /* detect when at end of string (NULL or len) */
        if((ch1 < 0) || !(--len))
            return(0);
    }

    return((ch1 > ch2) ? 1 : -1);
}

_Check_return_
extern S32
spell_strnicmp(
    _InVal_     DICT_NUMBER dict_number,
    PC_U8 word1,
    PC_U8 word2,
    _InVal_     U32 len)
{
    P_DICT p_dict;

    status_return(dict_validate(&p_dict, dict_number));

    return(strnicmp_us(p_dict, word1, word2, len));
}

/******************************************************************************
*
* convert a character to lower case
* using the dictionary's mapping
*
******************************************************************************/

_Check_return_
extern STATUS
spell_tolower(
    _InVal_     DICT_NUMBER dict_number,
    _InVal_     S32 ch)
{
    P_DICT p_dict;

    status_return(dict_validate(&p_dict, dict_number));

    return(tolower_us(p_dict, ch));
}

/******************************************************************************
*
* convert a character to upper case
* using the dictionary's mapping
*
******************************************************************************/

_Check_return_
extern STATUS
spell_toupper(
    _InVal_     DICT_NUMBER dict_number,
    _InVal_     S32 ch)
{
    P_DICT p_dict;

    status_return(dict_validate(&p_dict, dict_number));

    return(toupper_us(p_dict, ch));
}

/******************************************************************************
*
* unlock a dictionary
*
******************************************************************************/

_Check_return_
extern STATUS
spell_unlock(
    _InVal_     DICT_NUMBER dict_number)
{
    S32 i, n_index;
    SIXP lett;
    P_DICT p_dict;

    status_return(dict_validate(&p_dict, dict_number));

    assert(ixpdv(p_dict, 0));
    for(i = 0, lett = ixpdp(p_dict, 0), n_index = p_dict->n_index;
        i < n_index;
        ++i, ++lett)
    {
        lett->letflags &= ~LET_LOCKED;
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* say whether a character is valid
* as the first letter of a word
*
* --out--
* =0 character is invalid
*
******************************************************************************/

_Check_return_
extern STATUS
spell_valid_1(
    _InVal_     DICT_NUMBER dict_number,
    _InVal_     S32 ch)
{
    P_DICT p_dict;

    status_return(dict_validate(&p_dict, dict_number));

    return(char_ordinal_1(p_dict, toupper_us(p_dict, ch)) >= 0);
}

/******************************************************************************
*
* ensure string contains only chars valid for wild match
*
******************************************************************************/

static BOOL
badcharsin(
    _InRef_     P_DICT p_dict,
    _In_z_      const char *str)
{
    char ch;

    while((ch = *str++) != CH_NULL)
    {
        if(! (iswordc_us(p_dict, (S32) ch) ||
              (ch == SPELL_WILD_SINGLE) ||
              (ch == SPELL_WILD_MULTIPLE)) )
            return(TRUE);
    }

    return(FALSE);
}

/******************************************************************************
*
* convert a character to an ordinal number
* giving the index for the first character
* position
*
* --out--
* < 0 character not valid in first position
* >=0 ordinal number (offset by char_offset)
*
******************************************************************************/

_Check_return_
static STATUS
char_ordinal_1(
    _InRef_     P_DICT p_dict,
    _InVal_     S32 ch)
{
    if(ch >= 0)
    {
        char i;

        for(i = 0; i < p_dict->n_index_1; ++i)
            if(p_dict->letter_1[i] == (char) ch)
                return(i + p_dict->char_offset);
    }

    return(create_error(SPELL_ERR_BADWORD));
}

/******************************************************************************
*
* convert a character to an ordinal number
* giving the index for the second character
* position
*
* --out--
* < 0 character not valid in 2nd position
* >=0 ordinal number (offset by char_offset)
*
******************************************************************************/

_Check_return_
static STATUS
char_ordinal_2(
    _InRef_     P_DICT p_dict,
    _InVal_     S32 ch)
{
    if(ch >= 0)
    {
        char i;

        for(i = 0; i < p_dict->n_index_2; ++i)
            if(p_dict->letter_2[i] == (char) ch)
                return(i + p_dict->char_offset);
    }

    return(create_error(SPELL_ERR_BADWORD));
}

/******************************************************************************
*
* convert a character to an ordinal number
* giving the index for the third character
* position
*
* --out--
* < 0 character not valid in 3rd position
* >=0 ordinal number (offset by char_offset)
*
******************************************************************************/

_Check_return_
static STATUS
char_ordinal_3(
    _InRef_     P_DICT p_dict,
    _InVal_     S32 ch)
{
    if(ch >= 0)
    {
        char i;

        for(i = 0; i < p_dict->n_index_2; ++i)
            if(p_dict->letter_2[i] == (char) ch)
                return(p_dict->man_token_start ? ch : i + p_dict->char_offset);
    }

    return(create_error(SPELL_ERR_BADWORD));
}

/******************************************************************************
*
* compare two strings for sort routine
*
******************************************************************************/

PROC_QSORT_PROTO(static, compar, PC_SBSTR)
{
    P_PC_SBSTR word_1 = (P_PC_SBSTR) _arg1;
    P_PC_SBSTR word_2 = (P_PC_SBSTR) _arg2;

    /* NB no current_p_docu global register furtling required */

    return(spell_strnicmp(compar_dict, *word_1, *word_2, -1));
}

/******************************************************************************
*
* compare two endings for alphabetic value for sort routine
*
******************************************************************************/

PROC_QSORT_PROTO(static, compar_ending_alpha, ENDSTRUCT)
{
    ENDP end1 = (ENDP) _arg1;
    ENDP end2 = (ENDP) _arg2;

    /* NB no current_p_docu global register furtling required */

    return(strcmp(end1->ending, end2->ending));
}

/******************************************************************************
*
* compare two endings for length value for sort routine
* secondary sort on original order in definition file
*
* (sort on length is in reverse order)
*
******************************************************************************/

PROC_QSORT_PROTO(static, compar_ending_pos, ENDSTRUCT)
{
    ENDP end1 = (ENDP) _arg1;
    ENDP end2 = (ENDP) _arg2;

    /* NB no current_p_docu global register furtling required */

    return(end1->len == end2->len
                ? (end1->pos == end2->pos
                    ? 0
                    : (end1->pos > end2->pos
                        ? 1
                        : -1))
                : (end1->len > end2->len
                    ? -1
                    : 1));
}

/******************************************************************************
*
* take a tokenised and indexed word from
* a word structure, and return the real word
*
******************************************************************************/

static S32
decodeword(
    _InRef_     P_DICT p_dict,
    char *word,
    WRDP wp,
    S32 len)
{
    P_U8 ci;
    char *co;

    *(word + 0) = (char)
                  tolower_us(p_dict,
                                ordinal_char_1(p_dict,
                                               wp->lettix / p_dict->n_index_2 +
                                               p_dict->char_offset));

    if(len == 1)
    {
        *(word + 1) = CH_NULL;
        return(1);
    }

    *(word + 1) = (char)
                  tolower_us(p_dict,
                                ordinal_char_2(p_dict,
                                               wp->lettix % p_dict->n_index_2 +
                                               p_dict->char_offset));

    if(len == 2)
    {
        *(word + 2) = CH_NULL;
        return(2);
    }

    /* decode body */
    ci = wp->bodyd;
    co = word + 2;
    while(*ci)
        *co++ = (char) tolower_us(p_dict, ordinal_char_3(p_dict, *ci++));

    /* decode ending */
    ci = ((I_ENDP) (list_gotoitem(&p_dict->dict_end_list,
                                  (LIST_ITEMNO) wp->tail)->i.inside))->ending;

    while(*ci)
        *co++ = (char) tolower_us(p_dict, ordinal_char_3(p_dict, *ci++));
    *co = CH_NULL;

    return(strlen(word));
}

/******************************************************************************
*
* if the definition file is a dictionary, find out
* and position the file pointer for reading the
* definition file
*
******************************************************************************/

static S32
def_file_position(
    FILE_HANDLE def_file)
{
    char keystr[sizeof(KEYSTR)];
    S32 keylen, nbytes;
    S32 err;

    trace_0(TRACE_MODULE_SPELL, "def_file_position");

    /* position to start of file */
    status_return(file_seek(def_file, 0, SEEK_SET));

    /* read key string to determine if it's a dictionary or a definition file */
    nbytes = strlen(KEYSTR);
    status_return(err = file_read(keystr, sizeof(char), nbytes, def_file));

    if(0 == strncmp(keystr, KEYSTR, strlen(KEYSTR)))
        keylen = strlen(KEYSTR);
    else
        keylen = 0;

    status_return(file_seek(def_file, keylen, SEEK_SET));

    trace_1(TRACE_MODULE_SPELL, "def_file_position keylen: %d", keylen);

    return(0);
}

/******************************************************************************
*
* delete a cache block from the list,
* adjusting cache numbers for the deletion
*
******************************************************************************/

static void
deletecache(
    LIST_ITEMNO cacheno)
{
    LIST_ITEMNO i;

    /* remove cache block */
    trace_2(TRACE_MODULE_SPELL, "deleting cache block: %d, %d items on list",
            cacheno, list_numitem(cachelp));
    list_deleteitems(cachelp, cacheno, (LIST_ITEMNO) 1);

    /* adjust cache numbers below */
    for(i = list_atitem(cachelp); i < list_numitem(cachelp); ++i)
    {
        CACHEP cp = (CACHEP) list_gotoitem(cachelp, i)->i.inside;

        assert(ixv(cp->dict_number, cp->lettix));
        trace_2(TRACE_MODULE_SPELL, "cp: %p, ixp: %p", report_ptr_cast(cp), report_ptr_cast(ixp(cp->dict_number, cp->lettix)));
        ixp(cp->dict_number, cp->lettix)->p.cacheno = i;

        trace_2(TRACE_MODULE_SPELL, "deletecache adjusted: %d, numitems: %d",
                i, list_numitem(cachelp));
    }
}

/******************************************************************************
*
* delete a word unit from the dictionary -
* the root and all the endings, then insert
* the word we were trying to insert but
* couldn't because it was alphabetically in
* the middle of the unit, then re-insert all
* the words in the deleted unit
*
******************************************************************************/

static S32
delreins(
    _InRef_     P_DICT p_dict,
    _In_z_      const char *word,
    WRDP wp)
{
    P_U8 deadwords[MAX_TOKEN - MIN_TOKEN + 1];
    char realword[MAX_WORD + 1];
    S32 wordc = 0, len, i, err;
    char token_start;
    P_U8 datap, ep;
    CACHEP cp, ocp;
    SIXP lett;

    token_start = p_dict->token_start;

    assert(ixpdv(p_dict, wp->lettix));
    lett = ixpdp(p_dict, wp->lettix);
    cp = (CACHEP) list_gotoitem(cachelp, lett->p.cacheno)->i.inside;
    datap = ep = cp->data;

    datap += wp->findpos;
    ep += lett->blklen;

    /* extract each word */
    cache_lock = 1;
    err = 0;
    while((datap < ep) && *datap >= token_start)
    {
        wp->tail = *datap - token_start;
        len = decodeword(p_dict, realword, wp, 0);
        if(NULL == (deadwords[wordc] = al_ptr_alloc_bytes(P_U8, len + 1/*CH_NULL*/, &err)))
            break;
        strcpy(deadwords[wordc++], realword);
        ocp = cp;
        cp = (CACHEP) list_gotoitem(cachelp, lett->p.cacheno)->i.inside;
        /* SKS: memory allocation can move core */
        datap += (P_U8) cp - (P_U8) ocp;
        ep += (P_U8) cp - (P_U8) ocp;
        ++datap;
    }
    cache_lock = 0;

    if(err)
        return(err);

    /* check if any words are out of range */
    for(i = 0; i < wordc; ++i)
        if(spell_strnicmp(dict_number(p_dict), deadwords[i], word, -1) > 0)
            break;

    /* if none out of range, free memory and exit */
    if(i == wordc)
    {
        for(i = 0; i < wordc; ++i)
            al_ptr_free(deadwords[i]);
        return(0);
    }

    /* delete the words */
    for(i = 0; i < wordc; ++i)
    {
        char word_buf[MAX_WORD + 1];

        strcpy(word_buf, deadwords[i]);

        status_return(spell_deleteword(dict_number(p_dict), word_buf));
    }

    /* add the new word */
    if(NULL == (deadwords[wordc] = al_ptr_alloc_bytes(P_U8, strlen(word) + 1/*CH_NULL*/, &err)))
        return(err);
    strcpy(deadwords[wordc++], word);

    /* and put back all the words we deleted */
    compar_dict = dict_number(p_dict);
    qsort((void *) deadwords, wordc, sizeof(deadwords[0]), compar);
    for(i = 0; i < wordc; ++i)
    {
        char word_buf[MAX_WORD + 1];

        strcpy(word_buf, deadwords[i]);
        if((err = spell_addword(dict_number(p_dict), word_buf)) < 0)
            return(err);
        al_ptr_free(deadwords[i]);
    }

    return(1);
}

/******************************************************************************
*
* detect the end of possible matches
*
******************************************************************************/

static S32
endmatch(
    _InRef_     P_DICT p_dict,
    char *word,
    _In_opt_z_  const char *mask,
    S32 updown)
{
    S32 len, res;
    const char *ci = mask;

    if((NULL == mask) || (CH_NULL == mask))
        return(0);

    len = 0;
    while(iswordc_us(p_dict, (S32) *ci))
    {
        ++ci;
        ++len;
    }

    if(!len)
        return(0);

    res = strnicmp_us(p_dict, mask, word, len);

    return(updown ? (res >= 0) ? 0 : 1
                  : (res <= 0) ? 0 : 1);
}

/******************************************************************************
*
* ensure that any modified parts of a dictionary are written out to the disk
*
******************************************************************************/

static S32
ensuredict(
    _InRef_     P_DICT p_dict)
{
    S32 err = 0, allerr = 0;
    S32 i;

    trace_0(TRACE_MODULE_SPELL, "ensuredict");

    /* work down the index and write out anything altered */
    for(i = 0; i < p_dict->n_index; ++i)
    {
        SIXP lett;

        assert(ixpdv(p_dict, i));
        lett = ixpdp(p_dict, i);

        trace_1(TRACE_MODULE_SPELL, "ensure letter: %d", i);
        if(lett->letflags & LET_WRITE)
        {
            if(lett->letflags & LET_CACHED)
                err = killcache(lett->p.cacheno);
            else
            {
                /* mask flags to be written to disk */
                lett->letflags &= LET_ONE | LET_TWO;
                err = writeindex(p_dict, i);
            }

            if(err)
            {
                if(0 == allerr)
                    allerr = err;
            }
            else
            {
                assert(ixpdv(p_dict, i));
                ixpdp(p_dict, i)->letflags &= ~LET_WRITE;
            }
        }
    }

    return(allerr);
}

/******************************************************************************
*
* fetch a block of the dictionary
*
******************************************************************************/

static S32
fetchblock(
    _InRef_     P_DICT p_dict,
    S32 lettix)
{
    CACHEP newblock;
    P_LIST_ITEM it;
    FILE_HANDLE file_handle_dict;
    S32 err, nbytes;
    SIXP lett;

    /* check if it's already cached */
    assert(ixpdv(p_dict, lettix));
    if(ixpdp(p_dict, lettix)->letflags & LET_CACHED)
        return(0);

    trace_3(TRACE_MODULE_SPELL, "fetchblock dict: %d, letter: %d, cachelp: %p",
            dict_number(p_dict), lettix, report_ptr_cast(cachelp));

    /* allocate a list block if we don't have one */
    err = 0;
    if(!cachelp)
    {
        if(NULL != (cachelp = al_ptr_alloc_elem(LIST_BLOCK, 1, &err)))
        {
            list_init(cachelp,
                      SPELL_MAXITEMSIZE,
                      SPELL_MAXPOOLSIZE);
            list_register(cachelp);
            trace_0(TRACE_MODULE_SPELL, "fetchblock has allocated cache list block");
        }
    }

    /* get a space to receive the block */
    if(err >= 0)
    {
        cache_lock = 1;
        for(;;)
        {
            trace_0(TRACE_MODULE_SPELL, "fetchblock doing createitem");
            assert(ixpdv(p_dict, lettix));
            if((it = list_createitem(cachelp,
                                     list_numitem(cachelp),
                                     sizeof(struct CACHEBLOCK) +
                                        ixpdp(p_dict, lettix)->blklen,
                                     FALSE)) != NULL)
                break;

            trace_0(TRACE_MODULE_SPELL, "fetchblock doing freecache");
            if((err = freecache(-1)) < 0)
                break;
        }
        cache_lock = 0;
    }

    if(err < 0)
        return(err);

    newblock = (CACHEP) it->i.inside;
    assert(ixpdv(p_dict, lettix));
    lett = ixpdp(p_dict, lettix);

    /* read the data if there is any */
    if(lett->p.disk)
    {
        /* position for the read */
        trace_0(TRACE_MODULE_SPELL, "fetchblock doing seek");
        file_handle_dict = p_dict->file_handle_dict;
        if(status_fail(err = file_seek(file_handle_dict, lett->p.disk, SEEK_SET)))
        {
            deletecache(list_atitem(cachelp));
            return(err);
        }

        /* read in the block */
        trace_0(TRACE_MODULE_SPELL, "fetchblock doing read");
        nbytes = lett->blklen + sizeof(S32);
        if(status_fail(err = file_read(&newblock->diskspace, sizeof(char), nbytes, file_handle_dict)))
        {
            deletecache(list_atitem(cachelp));
            return(err);
        }
    }
    else
        newblock->diskspace = 0;

    /* save parameters in cacheblock */
    newblock->usecount = 0;
    newblock->lettix = lettix;
    newblock->dict_number = dict_number(p_dict);
    newblock->diskaddress = lett->p.disk;

    /* move index pointer */
    lett->p.cacheno = list_atitem(cachelp);
    lett->letflags |= LET_CACHED;

    return(0);
}

/******************************************************************************
*
* free least used cache block
*
* --out--
* bytes freed
*
******************************************************************************/

static S32
freecache(
    S32 lettix)
{
    LIST_ITEMNO cacheno, minno;
    P_LIST_ITEM it;
    CACHEP cp;
    S32 mincount = 0x7FFFFFFF;
    S32 err, bytes_freed;

    if(!cachelp)
        return(status_nomem());

    minno = -1;
    cacheno = 0;
    if((it = list_initseq(cachelp, &cacheno)) != NULL)
    {
        do  {
            cp = (CACHEP) it->i.inside;
            /* check if block is locked */
            if(lettix != cp->lettix &&
               !(ixp(cp->dict_number, cp->lettix)->letflags & LET_LOCKED))
            {
                if(cp->usecount < mincount)
                {
                    mincount = cp->usecount;
                    minno = cacheno;
                }
            }
        }
        while((it = list_nextseq(cachelp, &cacheno)) != NULL);
    }

    if(minno < 0)
        return(status_nomem());

    cp = (CACHEP) list_gotoitem(cachelp, minno)->i.inside;
    assert(ixv(cp->dict_number, cp->lettix));
    bytes_freed = ixp(cp->dict_number, cp->lettix)->blklen;

    #if TRACE_ALLOWED
    {
    S32 blocks, largest;
    S32 totalmem;

    trace_1(TRACE_MODULE_SPELL, "spell freecache has freed a block of: %d bytes",
            ixp(cp->dict_number, cp->lettix)->blklen);
    spell_stats(&blocks, &largest, &totalmem);
    trace_3(TRACE_MODULE_SPELL, "spell stats: blocks: %d, largest: %d, totalmem: %d",
            blocks, largest, totalmem);
    }
    #endif

    trace_1(TRACE_MODULE_SPELL, "freecache freeing: %d", minno);

    if((err = killcache(minno)) < 0)
        return(err);

    return(bytes_freed);
}

/******************************************************************************
*
* check space in dictionary table
*
* --out--
* <0 if no space, otherwise index
* of free entry
*
******************************************************************************/

static S32
get_dict_entry(
    _OutRef_    P_P_DICT p_p_dict)
{
    DICT_NUMBER dict_number;

    *p_p_dict = P_DICT_NONE;

    for(dict_number = 0; dict_number < MAX_DICT; ++dict_number)
    {
        const P_DICT p_dict = &dictlist[dict_number];

        if(p_dict->file_handle_dict == NULL)
        {
            p_dict->dicth       = 0;
            p_dict->dictsize    = 0;
            p_dict->token_start = p_dict->char_offset = 0;
            p_dict->data_offset = p_dict->index_offset = 0;
            p_dict->n_index     = 0;
            p_dict->n_index_1   = p_dict->n_index_2 = 0;
            p_dict->dictflags   = 0;

            /* initialise endings list */
            list_init(&p_dict->dict_end_list, END_MAXITEMSIZE, END_MAXPOOLSIZE);
            list_register(&p_dict->dict_end_list);

            *p_p_dict = p_dict;
            return(dict_number);
        }
    }

    return(create_error(SPELL_ERR_DICTFULL));
}

/******************************************************************************
*
* initialise variables for match
*
* --out--
* =0 indicates we have no word
* >0 means we have a valid word
*
******************************************************************************/

static S32
initmatch(
    _InRef_     P_DICT p_dict,
  /*_Out_z_*/   char *wordout,
    _In_z_      const char *wordin,
    _In_opt_z_  const char *mask)
{
    const char *ci;
    char *co;

    /* if no wordin, initialise from mask */
    if(!*wordin)
    {
        ci = mask;
        co = wordout;
        do  {
            if( (NULL == ci)                 ||
                (CH_NULL == *ci)             ||
                (SPELL_WILD_MULTIPLE == *ci) ||
                (SPELL_WILD_SINGLE == *ci)   )
            {
                if((NULL == ci) || (ci == mask))
                {
                    *co++ = p_dict->letter_1[0];
                    *co = CH_NULL;

                    assert(ixpdv(p_dict, 0));
                    return((ixpdp(p_dict, 0)->letflags & LET_ONE) ? 1 : 0);
                }
                *co = CH_NULL;
                return(1);
            }
            *co++ = *ci;
        }
        while(*ci++);
    }
    else
        strcpy(wordout, wordin);

    return(0);
}

/******************************************************************************
*
* remove a block from the cache chain,
* writing out to the dictionary if changed
*
******************************************************************************/

static S32
killcache(
    LIST_ITEMNO cacheno)
{
    S32 err, flush_err;
    S32 write;
    CACHEP cp;
    FILE_HANDLE file_handle_dict;
    SIXP lett;

    err = 0;
    cp = (CACHEP) list_gotoitem(cachelp, cacheno)->i.inside;

    trace_1(TRACE_MODULE_SPELL, "killcache cacheno: %d", cacheno);

    /* write out block if altered */
    assert(ixv(cp->dict_number, cp->lettix));
    if((write = (ixp(cp->dict_number, cp->lettix)->letflags & LET_WRITE)) != 0)
        if((err = writeblock(cp)) != 0)
            return(err);

    /* clear flags in index */
    lett = ixp(cp->dict_number, cp->lettix);
    lett->letflags &= LET_ONE | LET_TWO;

    /* save disk address in index */
    lett->p.disk = cp->diskaddress;

    { /* write out index entry */
    const P_DICT p_dict = &dictlist[cp->dict_number];
    file_handle_dict = p_dict->file_handle_dict;
    if(write && (err = writeindex(p_dict, cp->lettix)) != 0)
    {
        /* make the dictionary useless
         * if not properly updated
        */
        trace_1(TRACE_MODULE_SPELL, "write of index block: %d failed", cp->lettix);
        file_clearerror(file_handle_dict);
        status_consume(file_seek(file_handle_dict, 0, SEEK_SET));
        status_consume(file_putc(0, file_handle_dict));
    }
    } /*block*/

    /* throw away the cache block */
    deletecache(cacheno);

    /* flush buffer */
    trace_0(TRACE_MODULE_SPELL, "flushing buffer");
    if((flush_err = file_flush(file_handle_dict)) < 0 && !err)
        err = flush_err;

    trace_0(TRACE_MODULE_SPELL, "buffer flushed");
    return(err);
}

/******************************************************************************
*
* load the dictionary definition
*
* --out--
* <0 error
*
******************************************************************************/

static S32
load_dict_def(
    _InoutRef_  P_DICT p_dict)
{
    char keystr[sizeof(KEYSTR)];
    S32 keylen, nbytes;

    /* position to start of dictionary */
    status_return(file_seek(p_dict->file_handle_dict, 0, SEEK_SET));

    /* read key string to determine dictionary type */
    nbytes = strlen(KEYSTR);
    *keystr = CH_NULL;
    status_return(file_read(keystr, sizeof(char), nbytes, p_dict->file_handle_dict));

    keylen = strlen(KEYSTR);
    if(0 != strncmp(keystr, KEYSTR, keylen))
        return(create_error(SPELL_ERR_BADDICT));

    return(load_dict_def_now(p_dict->file_handle_dict, p_dict, keylen));
}

/******************************************************************************
*
* process a dictionary definition given a handle and key length
*
******************************************************************************/

static S32
load_dict_def_now(
    FILE_HANDLE def_file,
    _InoutRef_  P_DICT p_dict,
    S32 keylen)
{
    ENDSTRUCT token[MAX_TOKEN - MIN_TOKEN];
    char buffer[255 + 1];
    S32 res, i, end;
    char *in, *out;
    ENDP ep;
    S32  filepos;
    S32  err;

    /* position dictionary */
    status_return(err = file_seek(p_dict->file_handle_dict, keylen, SEEK_SET));

    /* read dictionary name */
    if(read_def_line(def_file, buffer) <= 0)
        return(create_error(SPELL_ERR_BADDEFFILE));

    xstrkat(p_dict->dict_name, elemof32(p_dict->dict_name), buffer);

    /* read character offset */
    if(read_def_line(def_file, buffer) <= 0)
        return(create_error(SPELL_ERR_BADDEFFILE));

    p_dict->char_offset = (char) atoi(buffer);

    if((p_dict->char_offset < MIN_CHAR) || (p_dict->char_offset >= MAX_CHAR))
        return(create_error(SPELL_ERR_BADDEFFILE));

    /* read token offset */
    if(read_def_line(def_file, buffer) <= 0)
        return(create_error(SPELL_ERR_BADDEFFILE));

    p_dict->man_token_start = (char) atoi(buffer);

    if(p_dict->man_token_start                      &&
       ((S32) p_dict->man_token_start <  MIN_TOKEN ||
        (S32) p_dict->man_token_start >= MAX_TOKEN) )
        return(create_error(SPELL_ERR_BADDEFFILE));

    /* read first letter list */
    if(((res = read_def_line(def_file, buffer)) <= 0) || (res > MAX_INDEX))
        return(create_error(SPELL_ERR_BADDEFFILE));

    p_dict->n_index_1 = (char) (res - 1);
    in  = buffer;
    out = p_dict->letter_1;
    while(*in)
    {
        if(p_dict->man_token_start &&
           (*in >= p_dict->man_token_start ||
            *in <  p_dict->char_offset) )
            return(create_error(SPELL_ERR_DEFCHARERR));

        *out++ = *in++;
    }

    /* read second letter list */
    if(((res = read_def_line(def_file, buffer)) <= 0) || (res > MAX_INDEX))
        return(create_error(SPELL_ERR_DEFCHARERR));

    p_dict->n_index_2 = (char) (res - 1);
    in  = buffer;
    out = p_dict->letter_2;
    while(*in)
    {
        if(p_dict->man_token_start &&
           (*in >= p_dict->man_token_start ||
            *in <  p_dict->char_offset) )
            return(create_error(SPELL_ERR_DEFCHARERR));

        *out++ = *in++;
    }

    /* initialise case map */
    for(i = 0; i < 256; i++)
        p_dict->case_map[i] = 0;

    /* flag OK characters in case map */
    for(i = 0, in = buffer; *in; ++in)
        p_dict->case_map[*in] = *in;

    /* read case equivalence list */
    if(((res = read_def_line(def_file, buffer)) <= 0) || ((char) (res - 1) > p_dict->n_index_2) )
        return(create_error(SPELL_ERR_DEFCHARERR));

    /* insert case equivalences */
    for(i = 0, in = buffer; *in; ++in, ++i)
        p_dict->case_map[*in] = p_dict->letter_2[i];

    /* set number of index elements */
    p_dict->n_index = (S32) p_dict->n_index_1 * p_dict->n_index_2;

    if(!(p_dict->token_start = p_dict->man_token_start))
        /* set auto token_start */
        p_dict->token_start = p_dict->char_offset + p_dict->n_index_2;
    else
        /* check manual token start is high enough */
        if(p_dict->char_offset + p_dict->n_index_2 > p_dict->man_token_start)
            return(create_error(SPELL_ERR_DEFCHARERR));

    /* insert ending zero - null token */
    token[0].len       = 0;
    token[0].alpha     = 0;
    token[0].pos       = 255;
    token[0].ending[0] = CH_NULL;

    /* read endings list */
    for(end = 1;
        (res = read_def_line(def_file, buffer)) > 0 &&
        end < MAX_TOKEN - MIN_TOKEN;
        ++end)
    {
        char *out;
        S32 i, ch;

        /* read an ending and convert to internal form */
        for(i = 0, in = buffer, ep = &token[end], out = ep->ending;
            i < MAX_ENDLEN;
            ++i)
        {
            U8 u8 = *in++;
            u8 = (U8) toupper_us(p_dict, u8);
            if((ch = char_ordinal_3(p_dict, u8)) <= 0)
                break;
            *out++ = (char) ch;
        }

        if(!i || i == MAX_ENDLEN)
            return(create_error(SPELL_ERR_BADDEFFILE));

        *out++ = CH_NULL;
        ep->len = (char) strlen(ep->ending);
        ep->pos = (char) end;
    }

    /* check we read some endings */
    if(res >= 0 && end == 1)
        res = create_error(SPELL_ERR_BADDEFFILE);
    if(res < 0)
        return(res);

    /* check that tokens and all fit into the space */
    if(p_dict->token_start + end >= MAX_TOKEN)
        return(create_error(SPELL_ERR_BADDEFFILE));

    /* sort into alphabetical order */
    qsort((void *) token,
          end,
          sizeof(token[0]),
          compar_ending_alpha);

    /* add alphabetical numbers */
    for(i = 0, ep = token; i < end; ++i, ++ep)
        ep->alpha = (char) i;

    /* sort into length/original position order */
    qsort((void *) token,
          end,
          sizeof(token[0]),
          compar_ending_pos);

    /* insert into dictionary ending list */
    for(i = 0, ep = token; i < end; ++i, ++ep)
    {
        P_LIST_ITEM it;
        I_ENDP iep;
        S32 bytes;

        /* the ending array already has 1 byte reserved */
        bytes = sizeof(struct I_ENDSTRUCT) + strlen(ep->ending);

        if((it = list_createitem(&p_dict->dict_end_list,
                                 (LIST_ITEMNO) i,
                                 bytes,
                                 FALSE)) == NULL)
            return(status_nomem());

        iep = (I_ENDP) it->i.inside;

        iep->len   = ep->len;
        iep->alpha = ep->alpha;
        strcpy(iep->ending, ep->ending);
    }

    /* use minimum space */
    list_packlist(&p_dict->dict_end_list, (LIST_ITEMNO) -1);

    /* read dictionary flags */
    status_return(res = file_getc(p_dict->file_handle_dict));

    p_dict->dictflags = (char) res;

    /* read position of start of index */
    status_return(filepos = file_tell(p_dict->file_handle_dict));

    /* set offset parameters */
    p_dict->index_offset = filepos;
    p_dict->data_offset  = filepos + p_dict->n_index * sizeof(struct IXSTRUCT);

    return(0);
}

/******************************************************************************
*
* look up a word in the dictionary
*
* --out--
* <0 error
*  0 not found
* >0 found
*
******************************************************************************/

static S32
lookupword(
    _InRef_     P_DICT p_dict,
    WRDP wp,
    S32 needpos)
{
    P_U8 startlim, endlim, midpoint, datap;
    char *co, *ci;
    S32 i, ch, found, bodylen, updown, err;
    char token_start, char_offset;
    CACHEP cp;
    SIXP lett;
    P_LIST_BLOCK dlp;

    if(needpos)
    {
        wp->fail = INS_WORD;
        wp->findpos = wp->matchc = wp->match = wp->matchcp = wp->matchp = 0;
    }

    assert(ixpdv(p_dict, wp->lettix));
    lett = ixpdp(p_dict, wp->lettix);

    /* check one/two letter words */
    switch(wp->len)
    {
    case 1:
        return(lett->letflags & LET_ONE);

    case 2:
        return(lett->letflags & LET_TWO);

    default:
        break;
    }

    /* is there a block defined ? */
    if(!lett->blklen)
        return(0);

    /* fetch the block if not in memory */
    if((err = fetchblock(p_dict, wp->lettix)) != 0)
        return(err);

    /* load some variables */
    token_start = p_dict->token_start;
    char_offset = p_dict->char_offset;
    dlp = &p_dict->dict_end_list;

    /* search the block for the word */
    assert(ixpdv(p_dict, wp->lettix));
    lett = ixpdp(p_dict, wp->lettix);
    cp = (CACHEP) list_gotoitem(cachelp, lett->p.cacheno)->i.inside;
    ++cp->usecount;

    /*
    do a binary search on the third letter
    */

    startlim = datap = cp->data;
    endlim = cp->data + lett->blklen;
    found = updown = 0;

    while(endlim - startlim > 1)
    {
        midpoint = datap = startlim + (endlim - startlim) / 2;

        /* step back to first in block */
        while(*datap)
            --datap;

        ch = (S32) *(datap + 1);
        if(ch == (S32) *wp->body)
        {
            found = 1;
            break;
        }

        if(ch < (S32) *wp->body)
        {
            updown = 1;
            startlim = midpoint;
        }
        else
        {
            updown = 0;
            endlim = midpoint;
        }
    }

    if(!found)
    {
        /* set insert position after this letter
        if we are inserting a higher letter */
        if(needpos)
        {
            if(updown)
            {
                endlim = cp->data + lett->blklen;
                while(datap < endlim)
                {
                    ++datap;
                    if(!*datap)
                        break;
                }
            }

            wp->fail = INS_WORD;
            wp->findpos = datap - (P_U8) cp->data /* fix C6.0 whinge */;
        }
        return(0);
    }

    /* search forward for word */
    endlim = cp->data + lett->blklen;

    *wp->bodyd = CH_NULL;
    while((datap + 1) < endlim)
    {
        /* save previous body */
        if(needpos)
            strcpy(wp->bodydp, wp->bodyd);

        /* take prefix count */
        co = wp->bodyd + *datap++;

        /* build body */
        while(*datap < token_start)
            *co++ = *datap++;

        /* mark end of body */
        bodylen = co - wp->bodyd;
        *co = CH_NULL;

        /* compare bodies and stop search if
        we are past the place */
        for(i = found = 0, ci = wp->bodyd, co = wp->body;
            i < bodylen;
            ++i, ++ci, ++co)
        {
            if(*ci != *co)
            {
                if(*ci > *co)
                    found = 1;
                else
                    found = -1;

                break;
            }
        }

        if(needpos)
        {
            wp->matchcp = wp->matchc;
            wp->matchp = wp->match;
            wp->matchc = i;
            wp->match = (i == bodylen && wp->matchc >= wp->matchcp) ? 1 : 0;
        }

        if(!found)
        {
            /* compare tokens */
            while(((ch = (S32) *datap) >= (S32) token_start) &&
                  datap < endlim)
            {
                ++datap;
                if(0 == strcmp(((I_ENDP) (list_gotoitem(dlp,
                                                    (LIST_ITEMNO)
                                                    ch - token_start)->
                                          i.inside))->ending,
                               wp->body + bodylen))
                {
                    if(needpos)
                    {
                        wp->fail = 0;
                        wp->findpos = datap - (P_U8) cp->data - 1 /* fix C6.0 whinge */;
                    }
                    return(1);
                }
            }
        }

        /* if bodies didn't compare */
        if(found                                   ||
           (needpos && (wp->matchc < wp->matchcp)))
        {
            if(found >= 0)
            {
                /* step back to start of word */
                if(needpos)
                {
                    while(*--datap >= char_offset)
                        ;

                    if(*datap)
                        wp->fail = INS_WORD;
                    else
                        wp->fail = (wp->matchc == 0)
                            ? INS_WORD
                            : INS_STARTLET;

                    wp->findpos = datap - (P_U8) cp->data /* fix C6.0 whinge */;
                }
                return(0);
            }
            else
            {
                /* skip tokens, then move to next body */
                while((*datap >= token_start) && (datap < endlim))
                    ++datap;

                continue;
            }
        }

    }

    /* at end of block */
    if(needpos)
    {
        wp->matchcp = wp->matchc;
        wp->matchp = wp->match;
        wp->matchc = wp->match = 0;
        strcpy(wp->bodydp, wp->bodyd);

        wp->fail = INS_WORD;
        wp->findpos = datap - (P_U8) cp->data /* fix C6.0 whinge */;
    }

    return(0);
}

/******************************************************************************
*
* set up index for a word
* this routine tries to be fast
*
* --in--
* pointer to a word block
*
* --out--
* index set in block,
* pointer to block returned
*
******************************************************************************/

static WRDP
makeindex(
    _InRef_     P_DICT p_dict,
    WRDP wp,
    _In_z_      PC_U8Z word)
{
    STATUS ch;
    U32 len = strlen(word);
    U8 u8;
    char *co;

    /* we can use more than char_offset since two letters
    at the start of every word are stripped by the index */
    if((0 == len) || (len >= (U32) p_dict->char_offset + 2))
        return(NULL);
    wp->len = len;

    /* process first letter */
    u8 = *word++;
    u8 = (U8) toupper_us(p_dict, u8);
    if(status_fail(ch = char_ordinal_1(p_dict, u8)))
        return(NULL);
    wp->lettix = (ch - p_dict->char_offset) * (S32) p_dict->n_index_2;

    /* process second letter */
    if(len > 1)
    {
        u8 = *word++;
        u8 = (U8) toupper_us(p_dict, u8);
        if(status_fail(ch = char_ordinal_2(p_dict, u8)))
            return(NULL);
        wp->lettix += (ch - p_dict->char_offset);
    }

    /* copy across body */
    co = wp->body;
    while(*word)
    {
        u8 = *word++;
        u8 = (U8) toupper_us(p_dict, u8);
        if(status_fail(ch = char_ordinal_3(p_dict, u8)))
            return(NULL);
        *co++ = (char) ch;
    }
    *co++ = CH_NULL;

    /* clear tail index */
    wp->tail = 0;

    return(wp);
}

/******************************************************************************
*
* match two words with possible wildcards
* word1 must contain any wildcards
*
* --out--
* -1 word1 < word2
*  0 word1 = word2
* +1 word1 > word2
*
******************************************************************************/

static S32
matchword(
    _InRef_     P_DICT p_dict,
    _In_opt_z_  const char *mask,
    char *word)
{
    const char *maskp = mask;
    const char *star = mask;
    char *wordp = word;
    char *nextpos = word;

    if((NULL == mask) || (CH_NULL == *mask))
        return(0);

    /* loop1 */
    for(;;)
    {
        ++nextpos;

        /* loop3 */
        for(;;)
        {
            if(*maskp == SPELL_WILD_MULTIPLE)
            {
                nextpos = wordp;
                star = ++maskp;
                break;
            }

            if(toupper_us(p_dict, *maskp) != toupper_us(p_dict, *wordp))
            {
                if(*wordp == CH_NULL)
                    return(1);

                if(*maskp != SPELL_WILD_SINGLE)
                {
                    ++maskp;
                    wordp = nextpos;
                    if(star != mask)
                    {
                        maskp = star;
                        break;
                    }
                    else
                        return( char_ordinal_2(p_dict, toupper_us(p_dict, *maskp)) <
                                char_ordinal_2(p_dict, toupper_us(p_dict, *wordp))
                                 ? -1
                                 :  1);
                }
            }

            if(*maskp++ == CH_NULL)
                return(0);
            ++wordp;
        }
    }
}

/******************************************************************************
*
* return the next word in a dictionary
*
* --out--
* <0 error
* =0 no more words
* >0 length of returned word
*
******************************************************************************/

_Check_return_
static STATUS
nextword(
    _InRef_     P_DICT p_dict,
    char *word)
{
    struct TOKWORD curword;
    S32 err, tokabval, nexthigher, token, curabval, tail, res, n_index;
    P_U8 datap, ep;
    char token_start, *co;
    CACHEP cp;
    SIXP lett;
    P_LIST_BLOCK dlp;

    if(!makeindex(p_dict, &curword, word))
        return(create_error(SPELL_ERR_BADWORD));

    /* check if word exists and get position */
    status_return(res = lookupword(p_dict, &curword, TRUE));

    if(curword.len == 1)
    {
        assert(ixpdv(p_dict, curword.lettix));
        if(ixpdp(p_dict, curword.lettix)->letflags & LET_TWO)
            return(decodeword(p_dict, word, &curword, 2));
    }

    n_index     = p_dict->n_index;
    token_start = p_dict->token_start;
    dlp         = &p_dict->dict_end_list;

    do  {
        assert(ixpdv(p_dict, curword.lettix));
        if(ixpdp(p_dict, curword.lettix)->blklen)
        {
            /* check we have a cache block */
            if((err = fetchblock(p_dict, curword.lettix)) != 0)
                return(err);

            assert(ixpdv(p_dict, curword.lettix));
            lett = ixpdp(p_dict, curword.lettix);
            cp = (CACHEP) list_gotoitem(cachelp,
                                        lett->p.cacheno)->i.inside;
            datap = cp->data + curword.findpos;
            ep = cp->data + lett->blklen;

            /* if we matched previous root */
            if(!curword.match && curword.matchp)
            {
                /* step back to previous unit */
                --datap;

                strcpy(curword.bodyd, curword.bodydp);

                curword.match  = curword.matchp;
                curword.matchc = curword.matchcp;
            }
            /* build a body at the start of a block */
            else if(datap < ep && !*datap)
            {
                /* skip null starter */
                ++datap;
                co = curword.bodyd;
                while(*datap < token_start)
                    *co++ = *datap++;
                *co++ = CH_NULL;
            }

            if(datap < ep)
            {
                tokenise(p_dict, &curword, MAX(curword.matchc, 1));
                if(!curword.match)
                    curabval = -1;
                else
                    curabval = setabval(p_dict, &curword, 1);

                /* ensure we are on the tokens */
                while(*datap >= token_start)
                    --datap;

                while((*datap < token_start) && (datap < ep))
                    ++datap;
            }

            while(datap < ep)
            {
                /* find the next higher token */
                tail = -1;
                nexthigher = MAX_TOKEN;
                while(((token = *datap) >= (S32) token_start) && (datap < ep))
                {
                    ++datap;
                    tokabval = ((I_ENDP) (list_gotoitem(dlp,
                                                        (LIST_ITEMNO)
                                                        token -
                                                        token_start)->
                                                        i.inside))->alpha;
                    if(tokabval > curabval)
                    {
                        if(tokabval < nexthigher)
                        {
                            tail = token - token_start;
                            nexthigher = tokabval;
                        }
                    }
                }

                if((tail >= 0) && (nexthigher > curabval))
                {
                    /* work out the real word from the decoded stuff */
                    curword.tail = tail;
                    return(decodeword(p_dict, word, &curword, 0));
                }

                /* if there were no tokens higher, go onto next root */
                if(++datap >= ep)
                    break;

                co = curword.bodyd + token;

                while((*datap < token_start) && (datap < ep))
                    *co++ = *datap++;

                *co = CH_NULL;
                curabval = -1;
            }
        }

        /* move onto the next letter */
        *curword.bodyd = *curword.bodydp = CH_NULL;
        curword.tail   = curword.matchc = curword.matchcp =
        curword.match  = curword.matchp = curword.findpos = 0;

        /* skip down the index till we find
        an entry with some words in it */
        if(++curword.lettix < n_index)
        {
            assert(ixpdv(p_dict, curword.lettix));
            lett = ixpdp(p_dict, curword.lettix);

            if(lett->letflags & LET_ONE)
                return(decodeword(p_dict, word, &curword, 1));

            if(lett->letflags & LET_TWO)
                return(decodeword(p_dict, word, &curword, 2));
        }
    }
    while(curword.lettix < n_index);

    *word = CH_NULL;
    return(0);
}

/******************************************************************************
*
* given ordinal number for the first character
* position (including char_offset)
* return the upper case character for this ordinal
*
******************************************************************************/

_Check_return_
static S32
ordinal_char_1(
    _InRef_     P_DICT p_dict,
    _InVal_     S32 ord)
{
    return(p_dict->letter_1[ord - p_dict->char_offset]);
}

/******************************************************************************
*
* given ordinal number for the second character
* position (including char_offset)
* return the upper case character for this ordinal
*
******************************************************************************/

_Check_return_
static S32
ordinal_char_2(
    _InRef_     P_DICT p_dict,
    _InVal_     S32 ord)
{
    return(p_dict->letter_2[ord - p_dict->char_offset]);
}

/******************************************************************************
*
* given ordinal number for the third character
* position (including char_offset)
* return the upper case character for this ordinal
*
******************************************************************************/

_Check_return_
static S32
ordinal_char_3(
    _InRef_     P_DICT p_dict,
    _InVal_     S32 ord)
{
    return(p_dict->man_token_start ? ord : p_dict->letter_2[ord - p_dict->char_offset]);
}

/******************************************************************************
*
* return the previous word
*
* --out--
* <0 error
* =0 no more words
* >0 length of word
*
******************************************************************************/

_Check_return_
static STATUS
prevword(
    _InRef_     P_DICT p_dict,
    char *word)
{
    struct TOKWORD curword;
    S32 err, tokabval, nextlower, token, curabval, tail, i, onroot;
    P_U8 sp, ep, datap;
    char token_start, char_offset, *co;
    CACHEP cp;
    SIXP lett;
    P_LIST_BLOCK dlp;

    if(!makeindex(p_dict, &curword, word))
        return(create_error(SPELL_ERR_BADWORD));

    /* check if word exists and get position */
    status_return(err = lookupword(p_dict, &curword, TRUE));

    if(curword.len == 2)
    {
        assert(ixpdv(p_dict, curword.lettix));
        if(ixpdp(p_dict, curword.lettix)->letflags & LET_ONE)
            return(decodeword(p_dict, word, &curword, 1));
    }

    token_start = p_dict->token_start;
    char_offset = p_dict->char_offset;
    dlp         = &p_dict->dict_end_list;

    do  {
        assert(ixpdv(p_dict, curword.lettix));
        if(ixpdp(p_dict, curword.lettix)->blklen)
        {
            /* check we have a cache block */
            if((err = fetchblock(p_dict, curword.lettix)) != 0)
                return(err);

            assert(ixpdv(p_dict, curword.lettix));
            lett = ixpdp(p_dict, curword.lettix);
            cp = (CACHEP) list_gotoitem(cachelp,
                                        lett->p.cacheno)->i.inside;
            sp = cp->data;

            datap = sp + curword.findpos;
            ep = sp + lett->blklen;

            onroot = 0;
            if((datap > sp) && (curword.matchc || curword.matchcp))
            {
                /* if we didn't match this root,
                start from the one before */
                if(!curword.match)
                {
                    if(datap != ep)
                        while(*datap >= char_offset)
                            --datap;
                    if(datap > sp)
                    {
                        --datap;
                        curword.matchc = curword.matchcp;
                        curword.match = curword.matchcp = 0;
                        strcpy(curword.bodyd, curword.bodydp);
                    }
                }

                tokenise(p_dict, &curword, MAX(curword.matchc, 1));
                if(!curword.match)
                    curabval = MAX_TOKEN;
                else
                    curabval = setabval(p_dict, &curword, 0);

                /* move to the start of the root */
                while(*datap >= char_offset)
                    --datap;

                /* build a body at the start of a block */
                if(*datap == CH_NULL)
                {
                    /* skip null starter */
                    ++datap;
                    co = curword.bodyd;
                    while(*datap < token_start)
                        *co++ = *datap++;
                    *co++ = CH_NULL;
                    curword.matchcp = 0;
                }

                /* move on to the tokens */
                while((*datap < token_start) && (datap < ep))
                    ++datap;

                onroot = 1;
            }

            /* move back down the block */
            while((datap > sp) && onroot)
            {
                /* find the next lower token */
                tail = nextlower = -1;
                while(((token = *datap) >= (S32) token_start) && (datap < ep))
                {
                    ++datap;
                    tokabval = ((I_ENDP) (list_gotoitem(dlp,
                                                        (LIST_ITEMNO)
                                                        token -
                                                        token_start)->
                                                        i.inside))->alpha;
                    if(tokabval < curabval)
                    {
                        if(tokabval > nextlower)
                        {
                            tail = token - token_start;
                            nextlower = tokabval;
                        }
                    }
                }

                /* did we find a suitable token ? */
                if((tail >= 0) && (nextlower < curabval))
                {
                    /* work out the real word from the decoded stuff */
                    curword.tail = tail;
                    return(decodeword(p_dict, word, &curword, 0));
                }

                /* if there were no tokens lower,
                go onto previous root */
                --datap;
                while(*datap >= char_offset)
                    --datap;

                /* check for beginning of block,
                or no previous root */
                if((datap == sp) || (!curword.matchcp))
                    break;
                --datap;
                while(*datap >= token_start)
                    --datap;
                ++datap;

                curword.match = 0;
                strcpy(curword.bodyd, curword.bodydp);
                curabval = MAX_TOKEN;
            }
        }

        assert(ixpdv(p_dict, curword.lettix));
        lett = ixpdp(p_dict, curword.lettix);
        if(datap == sp || !lett->blklen)
        {
            char last_ch;
            S32 max_word;

            /* if we are at the start of the block */
            do  {
                /* return the small words if there are some */
                if((curword.len > 2) && (lett->letflags & LET_TWO))
                    return(decodeword(p_dict, word, &curword, 2));
                if((curword.len > 1) && (lett->letflags & LET_ONE))
                    return(decodeword(p_dict, word, &curword, 1));

                if(--curword.lettix < 0)
                    break;

                assert(ixpdv(p_dict, curword.lettix));
                lett = ixpdp(p_dict, curword.lettix);
            }
            while(!lett->blklen);

            /* quit if at beginning */
            if(curword.lettix < 0)
                break;

            /* build previous letter ready for search */
            decodeword(p_dict, word, &curword, 2);
            last_ch  = p_dict->letter_2[p_dict->n_index_2 - 1];
            max_word = p_dict->char_offset + 1;
            for(i = 2, co = word + 2; i < max_word; ++i)
                *co++ = last_ch;
            *co = CH_NULL;

            /* set position to end of block */
            makeindex(p_dict, &curword, word);
        }
        else
        {
            char ch;
            char last_ch;
            S32 max_word;

            /* if we are at the start of a letter in a block */
            co = word + 2;

            /* set word to 'last of previous letter' */
            ch = (char) toupper_us(p_dict, *co);
            *co++ = p_dict->letter_2[((char) char_ordinal_2(p_dict, ch) - p_dict->char_offset) - 1];

            last_ch  = (char) tolower_us(p_dict, p_dict->letter_2[p_dict->n_index_2 - 1]);
            max_word = p_dict->char_offset + 1;

            for(i = 3; i < max_word; ++i)
                *co++ = last_ch;
            *co = CH_NULL;

            makeindex(p_dict, &curword, word);
        }

        if((err = lookupword(p_dict, &curword, TRUE)) < 0)
            return(err);
    }
    while(curword.lettix >= 0);

    *word = CH_NULL;
    return(0);
}

/******************************************************************************
*
* read a line from the definition file
*
* --out--
* <0 error
* =0 EOF
* >0 line length
*
******************************************************************************/

static S32
read_def_line(
    FILE_HANDLE def_file,
    char *buffer)
{
    static S32 last_ch = 0;
    S32 ch, comment, hadcr, hadbar, leadspc, err;
    char *out, *trail;

    comment = hadcr = hadbar = leadspc = 0;
    out = trail = buffer;
    for(;;)
    {
        if(last_ch)
        {
            ch = last_ch;
            last_ch = 0;
        }
        else
        {
            ch = file_getc(def_file);

            if(file_eof(def_file))
                break;
        }

        if((err = file_error(def_file)) != 0)
            return(err);

        /* nulls delimit the definition file
        when stored in the dictionary */
        if(!ch)
            break;

        if(ch == CR || ch == LF)
        {
            hadcr = 1;
            hadbar = comment = 0;
            continue;
        }

        /* stop on non-blank line and CR or LF */
        if(hadcr && trail != buffer)
        {
            /* save last character */
            last_ch = ch;
            break;
        }
        else
            hadcr = 0;

        /* check for comments */
        if(ch == ESC_CHAR)
        {
            if(!hadbar)
            {
                hadbar = 1;
                continue;
            }

            /* bona fide | character */
            hadbar = 0;
        }

        if(hadbar)
            comment = 1;

        if(comment)
            continue;

        /* strip out control characters */
        if(ch < SPACE)
            continue;

        /* strip leading spaces */
        if(ch == SPACE && !leadspc)
            continue;
        else
            leadspc = 1;

        /* finally, save wanted character */
        *out++ = (char) ch;

        if(ch != SPACE)
            trail = out;
    }

    if(trail != buffer)
        *trail++ = CH_NULL;

    return(trail - buffer);
}

/******************************************************************************
*
* give back entry in dictionary table
*
******************************************************************************/

static void
release_dict_entry(
    _InoutRef_  P_DICT p_dict)
{
    list_free(&p_dict->dict_end_list);
    list_deregister(&p_dict->dict_end_list);

    /* free memory used by index */
    al_array_dispose(&p_dict->dicth);

    /* throw away file handle */
    p_dict->file_handle_dict = NULL;
}

/******************************************************************************
*
* set alphabetic value of ending of tokenised word
*
******************************************************************************/

static S32
setabval(
    _InRef_     P_DICT p_dict,
    WRDP wp,
    S32 lo_hi)
{
    char ending[MAX_WORD + 1];
    I_ENDP iep;
    S32 i, abval;
    P_LIST_BLOCK dlp;

    /* find entry for ending */
    dlp = &p_dict->dict_end_list;
    iep = (I_ENDP) list_gotoitem(dlp,
                                 (LIST_ITEMNO)
                                 wp->tail - p_dict->token_start)->i.inside;

    /* if token is whole ending ... */
    if(wp->matchc == (S32) strlen(wp->body))
        return(iep->alpha);

    /* concoct whole ending */
    xstrkpy(ending, elemof32(ending), wp->body + wp->matchc);
    xstrkat(ending, elemof32(ending), iep->ending);

    /* work out the alphabetic value of the non-token ending
    that we have - either the next lower, or next higher value */
    for(i = 0, abval = lo_hi ? 0 : 1000; i < (S32) list_numitem(dlp); ++i)
    {
        iep = (I_ENDP) (list_gotoitem(dlp, (LIST_ITEMNO) i)->i.inside);

        if(lo_hi)
        {
            if(strnicmp_us(p_dict, iep->ending, ending, -1) <= 0 &&
               (S32) iep->alpha > abval)
                abval = (S32) iep->alpha;
        }
        else
        {
            if(strnicmp_us(p_dict, iep->ending, ending, -1) >= 0 &&
               (S32) iep->alpha < abval)
                abval = (S32) iep->alpha;
        }
    }

    return(abval);
}

/******************************************************************************
*
* make sure that there are no cache blocks left for a given dictionary
*
******************************************************************************/

static void
stuffcache(
    _InRef_     P_DICT p_dict)
{
    LIST_ITEMNO i;

    trace_0(TRACE_MODULE_SPELL, "stuffcache");

    /* write out/remove any blocks from this dictionary */
    for(i = 0; i < list_numitem(cachelp); ++i)
    {
        CACHEP cp = (CACHEP) list_gotoitem(cachelp, i)->i.inside;

        if(cp->dict_number == dict_number(p_dict))
        {
            deletecache(i);
            --i;
        }
    }
}

/******************************************************************************
*
* tokenise a word
* this routine tries to be fast
*
* --in--
* pointer to a word block
*
* --out--
* tokenised word in block,
* pointer to block returned
*
******************************************************************************/

static void
tokenise(
    _InRef_     P_DICT p_dict,
    WRDP wp,
    S32 rootlen)
{
    U8 *endbody, token_start;
    S32 i, maxtail;
    P_LIST_BLOCK dlp;

    /* calculate maximum ending length */
    if(wp->len > rootlen + 2)
        maxtail = wp->len - rootlen - 2;
    else
        maxtail = 0;

    /* find a suitable ending */
    endbody = wp->body + wp->len - 2;

    token_start = p_dict->token_start;
    dlp         = &p_dict->dict_end_list;

    for(i = 0; i < (S32) list_numitem(dlp); ++i)
    {
        I_ENDP curend = (I_ENDP) (list_gotoitem(dlp, (LIST_ITEMNO) i)->i.inside);

        if(maxtail < (S32) curend->len)
            continue;

        if(0 == strcmp(curend->ending, endbody - curend->len))
        {
            wp->tail = i + token_start;
            *(endbody - curend->len) = CH_NULL;
            break;
        }
    }
}

/******************************************************************************
*
* convert a character to lower case
* using the dictionary's mapping
*
******************************************************************************/

static S32
tolower_us(
    _InRef_     P_DICT p_dict,
    _InVal_     S32 ch)
{
    if(ch >= SPACE)
    {
        S32 i;

        for(i = SPACE; i < 256; i++)
            if((i != ch) && (p_dict->case_map[i] == (char) ch))
                return(i);
    }

    return(ch);
}

/******************************************************************************
*
* write out an altered block to a dictionary
*
******************************************************************************/

static S32
writeblock(
    CACHEP cp)
{
    const P_DICT p_dict = &dictlist[cp->dict_number];
    SIXP lett;

    trace_2(TRACE_MODULE_SPELL, "writeblock dict: %d, letter: %d", (int) cp->dict_number, cp->lettix);

    assert(ixpdv(p_dict, cp->lettix));
    lett = ixpdp(p_dict, cp->lettix);

    if(lett->blklen)
    {
        const FILE_HANDLE file_handle_dict = p_dict->file_handle_dict;

        /* do we need to extend file ? */
        if((S32) lett->blklen > cp->diskspace)
        {
            status_return(file_seek(file_handle_dict, p_dict->data_offset + p_dict->dictsize + lett->blklen + EXT_SIZE, SEEK_SET));

            status_return(file_putc(0, file_handle_dict));

            cp->diskaddress = p_dict->data_offset + p_dict->dictsize;
            p_dict->dictsize += lett->blklen + EXT_SIZE;
            cp->diskspace += EXT_SIZE;
        }

        /* write out block */
        status_return(file_seek(file_handle_dict, cp->diskaddress, SEEK_SET));

        status_return(file_write_bytes(&cp->diskspace, lett->blklen + sizeof32(S32), file_handle_dict));
    }

    lett->letflags &= ~LET_WRITE;

    return(0);
}

/******************************************************************************
*
* write out the index entry for a letter
*
******************************************************************************/

static S32
writeindex(
    _InRef_     P_DICT p_dict,
    _InVal_     S32 lettix)
{
    status_return(file_seek(p_dict->file_handle_dict, p_dict->index_offset + sizeof32(struct IXSTRUCT) * lettix, SEEK_SET));

    assert(ixpdv(p_dict, lettix));
    status_return(file_write_bytes(ixpdp(p_dict, lettix), sizeof32(struct IXSTRUCT), p_dict->file_handle_dict));

    return(0);
}

/* end of spell.c */
