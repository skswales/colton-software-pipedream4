/* spell.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

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
#define BUF_SIZE    1024    /* size of buffer on stream */
#define EXT_SIZE     500    /* size of disk extend */
#define DICT_NAMLEN   80    /* length of dictionary name */
#define ESC_CHAR     '|'    /* escape character */
#define MIN_CHAR      32    /* minimum character offset */
#define MAX_CHAR      64    /* maximum character offset */
#define MIN_TOKEN     64    /* minium value of token */
#define MAX_TOKEN    256    /* maximum value of token */
#define MAX_INDEX     96    /* maximum elements in index level */
#define MAX_ENDLEN    15    /* maximum ending length */

#define KEYSTR      "[Colton Soft]"
#define OLD_KEYSTR  "Integrale"

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
    S32  dict;
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

struct IXOFDICT
{
    FILE_HANDLE dicthandle;             /* handle of dictionary file */
    ARRAY_HANDLE dicth;                      /* handle of index memory */
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
};

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

#if WINDOWS
#ifndef _CHAR_UNSIGNED
#error chars must be unsigned: use -J
#endif
#endif

/*
internal functions
*/

static S32
badcharsin(
    S32 dict,
    char *str);

static S32
char_ordinal_1(
    S32 dict,
    S32 ch);

static S32
char_ordinal_2(
    S32 dict,
    S32 ch);

static S32
def_file_position(
    FILE_HANDLE def_file);

static S32
delreins(
    S32 dict,
    char *word,
    WRDP wp);

static S32
endmatch(
    S32 dict,
    char *word,
         char *mask,
    S32 updown);

static S32
ensuredict(
    S32 dict);

static S32
fetchblock(
    S32 dict,
    S32 lettix);

static S32
freecache(
    S32 lettix);

static S32
get_dict_entry(void);

static S32
initmatch(
    S32 dict,
    char *wordout,
    char *wordin,
    char *mask);

static S32
killcache(
    LIST_ITEMNO cacheno);

static S32
load_dict_def(
    S32 dict,
    P_U8 def_name);

static S32
load_dict_def_now(
    FILE_HANDLE def_file,
    S32 dict,
    S32 keylen);

static S32
lookupword(
    S32 dict,
    WRDP wp,
    S32 needpos);

static WRDP
makeindex(
    S32 dict,
    WRDP wp,
    char *word);

static S32
matchword(
    S32 dict,
    char *mask,
    char *word);

static S32
nextword(
    S32 dict,
    char *word);

static S32
ordinal_char_1(
    S32 dict,
    S32 ord);

static S32
ordinal_char_2(
    S32 dict,
    S32 ord);

static S32
ordinal_char_3(
    S32 dict,
    S32 ord);

static S32
prevword(
    S32 dict,
    char *word);

static S32
read_def_line(
    FILE_HANDLE def_file,
    char *buffer);

static void
release_dict_entry(
    S32 dict);

static S32
setabval(
    S32 dict,
    WRDP wp,
    S32 lo_hi);

static void
stuffcache(
    S32 dict);

static void
tokenise(
    S32 dict,
    WRDP wp,
    S32 rootlen);

static S32
writeblock(
    CACHEP cp);

static S32
writeindex(
    S32 dict,
    S32 lettix);

/*
static variables
*/

static P_LIST_BLOCK cachelp = NULL;             /* list of cached blocks */
static struct IXOFDICT dictlist[MAX_DICT];      /* dictionary table */
static S32 spell_addword_nestf = 0;             /* addword nest level */
static S32 compar_dict;                         /* dictionary compar needs */
static S32 full_event_registered = 0;           /* we've registered interest in full events */
static S32 cache_lock = 0;                      /* ignore full events for a mo (!) */

/*
macro to get index pointer given
either dictionary number or pointer
*/

#define ixp(dict, ix) array_ptr(&dictlist[(dict)].dicth, struct IXSTRUCT, ix)
#define ixpdp(dp, ix) array_ptr(&(dp)->dicth, struct IXSTRUCT, ix)

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

extern S32
spell_addword(
    S32 dict,
    char *word)
{
    struct TOKWORD newword;
    S32 res, wordsize, err, rootlen;
    char token_start;
    P_U8 newpos, temppos;
    char *ci;
    CACHEP cp;
    P_LIST_ITEM it;
    SIXP lett;
    DIXP dp;

    if( (dict < 0)  ||  (dict >= MAX_DICT)  ||
        ((dp = &dictlist[dict])->dicthandle == NULL) )
            return(create_error(SPELL_ERR_BADDICT));

    if(!makeindex(dict, &newword, word))
        return(create_error(SPELL_ERR_BADWORD));

    /* check if word exists and get position */
    if((res = lookupword(dict, &newword, TRUE)) < 0)
        return(res);

    /* check if dictionary is read only */
    if(dp->dictflags & DICT_READONLY)
        return(create_error(SPELL_ERR_READONLY));

    if(res)
        return(0);

    token_start = dp->token_start;

    switch(newword.len)
    {
    /* single letter word */
    case 1:
        ixp(dict, newword.lettix)->letflags |= LET_ONE;
        break;

    /* two letter word */
    case 2:
        ixp(dict, newword.lettix)->letflags |= LET_TWO;
        break;

    /* all other words */
    default:
        /* tokenise the word to be inserted */
        rootlen = (newword.fail == INS_STARTLET)
                  ? MAX(newword.matchc, 1)
                  : MAX(newword.matchcp, 1);
        tokenise(dict, &newword, rootlen);

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

                    cp = (CACHEP) list_gotoitem(cachelp,
                                                ixp(dict, delword.lettix)->
                                                p.cacheno)->i.inside;

                    pos = cp->data + delword.findpos;

                    /* skip back to start of unit */
                    while(delword.findpos && (*--pos >= token_start))
                        --delword.findpos;

                    spell_addword_nestf = 1;
                    if((res = delreins(dict, word, &delword)) < 0)
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
        if((err = fetchblock(dict, newword.lettix)) != 0)
            return(err);

        /* add word to cache block */
        err = 0;
        cache_lock = 1;
        for(;;)
        {
            /* loop to get some memory */
            lett = ixp(dict, newword.lettix);

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
        temppos = newpos;
        lett = ixp(dict, newword.lettix);
        err = lett->blklen - newword.findpos;
        memmove32(newpos + wordsize,
                  newpos,
                  lett->blklen - newword.findpos);

        lett->blklen += wordsize;

        /* move in word */
        switch(newword.fail)
        {
        case INS_STARTLET:
            *newpos++ = '\0';
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
    ixp(dict, newword.lettix)->letflags |= LET_WRITE;

    return(1);
}

/******************************************************************************
*
* check if the word is in the dictionary
*
******************************************************************************/

extern S32
spell_checkword(
    S32 dict,
    char *word)
{
    struct TOKWORD curword;

    if( (dict < 0)  ||  (dict >= MAX_DICT)  ||
        (dictlist[dict].dicthandle == NULL) )
            return(create_error(SPELL_ERR_BADDICT));

    if(!makeindex(dict, &curword, word))
        return(create_error(SPELL_ERR_BADWORD));

    /* check if word exists and get position */
    return(lookupword(dict, &curword, FALSE));
}

/******************************************************************************
*
* close a dictionary
*
******************************************************************************/

extern S32
spell_close(
    S32 dict)
{
    FILE_HANDLE dicthand;
    S32 err, close_err;

    trace_0(TRACE_MODULE_SPELL,"spell_close");

    if((dict < 0)  ||  (dict >= MAX_DICT))
        return(create_error(SPELL_ERR_BADDICT));

    if((dicthand = dictlist[dict].dicthandle) == NULL)
    {
        trace_1(TRACE_OUT | TRACE_MODULE_SPELL, "spell_close called to close non-open dictionary: %d", dict);
        return(create_error(SPELL_ERR_CANTCLOSE));
    }

    /* write out any modified part */
    err = ensuredict(dict);

    /* make sure no cache blocks left */
    stuffcache(dict);

    /* close file on media */
    if((close_err = file_close(&dicthand)) < 0)
        err = err ? err : close_err;

    release_dict_entry(dict);

    return(err);
}

/******************************************************************************
*
* create new dictionary
*
******************************************************************************/

extern S32
spell_createdict(
    char *name,
    char *def_name)
{
    S32  err;
    S32 dict, nbytes, i, res;
    FILE_HANDLE newdict, def_file;
    DIXP dp;
    struct IXSTRUCT wix;
    char buffer[255 + 1];

    /* get a dictionary number */
    if((dict = get_dict_entry()) < 0)
        return(dict);

    dp = &dictlist[dict];

    /* check to see if it exists */
    if(file_open(name, file_open_read, &newdict), newdict != NULL)
    {
        file_close(&newdict);
        release_dict_entry(dict);
        return(create_error(SPELL_ERR_EXISTS));
    }

    /* dummy loop for structure */
    do  {
        err = 0;
        newdict = def_file = NULL;

        /* try to open definition file */
        if(file_open(def_name, file_open_read, &def_file), def_file == NULL)
        {
            err = create_error(SPELL_ERR_CANTOPENDEFN);
            break;
        }

        /* create a blank file */
        if(file_open(name, file_open_write, &newdict), newdict == NULL)
        {
            err = create_error(SPELL_ERR_CANTOPEN);
            break;
        }

        /* get file buffers */
        file_buffer(newdict, NULL, BUF_SIZE);
        file_buffer(def_file, NULL, BUF_SIZE);

        trace_0(TRACE_MODULE_SPELL, "spell_createdict about to write KEYSTR");

        /* write out file identifier */
        nbytes = strlen(KEYSTR);
        if((err = file_write(KEYSTR, sizeof(char), nbytes, newdict)) < 0)
            break;

        trace_0(TRACE_MODULE_SPELL, "spell_createdict about to def_file_position");

        /* position definition file */
        if((err = def_file_position(def_file)) < 0)
            break;

        trace_1(TRACE_MODULE_SPELL, "spell_createdict def_file_position returned: %d", err);

        /* copy across definition file */
        while((res = read_def_line(def_file, buffer)) > 0)
        {
            trace_1(TRACE_MODULE_SPELL, "spell_createdict def line: %s", trace_string(buffer));

            if((err = file_write(buffer, sizeof(char), res, newdict)) < 0)
                goto error;
        }

        trace_1(TRACE_MODULE_SPELL, "spell_createdict after def file: %d", res);

        /* stop on error */
        if(res < 0)
        {
            err = res;
            break;
        }

        /* write out definition end byte */
        if((err = file_putc(0, newdict)) < 0)
            break;

        /* write out dictionary flag byte */
        if((err = file_putc(0, newdict)) < 0)
            break;

        /* close file so far */
        if((err = file_close(&newdict)) < 0)
            break;

        newdict = 0;

        trace_1(TRACE_MODULE_SPELL, "spell_createdict err: %d", err);

        /* re-open for update */
        if(file_open(name, file_open_readwrite, &newdict), newdict == NULL)
        {
            err = create_error(SPELL_ERR_CANTOPEN);
            break;
        }

        file_buffer(newdict, NULL, BUF_SIZE);

        /* process definition file */
        if((res = load_dict_def(dict, NULL)) < 0)
        {
            err = res;
            break;
        }

        /* get a blank structure */
        wix.p.disk   = 0;
        wix.blklen   = 0;
        wix.letflags = 0;

        if((err = file_seek(newdict, dp->index_offset, SEEK_SET)) < 0)
            break;

        /* write out blank structures */
        for(i = 0; i < dp->n_index; ++i)
        {
            if((err = file_write(&wix,
                                 sizeof(struct IXSTRUCT),
                                 1,
                                 newdict)) < 0)
            {
                trace_1(TRACE_MODULE_SPELL, "spell_createdict failed to write out index entry %d", i);
                goto error;
            }
        }
    }
    while(FALSE);

error:

    if(err >= 0)
        err = file_flush(newdict);

    /* get rid of our entry */
    release_dict_entry(dict);

    if(err < 0)
    {
        if(newdict)
            file_close(&newdict);

        if(def_file)
            file_close(&def_file);

        remove(name);
        return(err);
    }

    file_close(&def_file);

    return(file_close(&newdict));
}

/******************************************************************************
*
* delete a word from a dictionary
*
******************************************************************************/

extern S32
spell_deleteword(
    S32 dict,
    char *word)
{
    struct TOKWORD curword;
    S32 res, delsize, err, tokcount, addroot, blockbefore, i;
    char token_start, char_offset;
    P_U8 sp, ep, datap, endword, ci, co;
    CACHEP cp;
    SIXP lett;
    DIXP dp;

    if( (dict < 0)  ||  (dict >= MAX_DICT)  ||
        ((dp = &dictlist[dict])->dicthandle == NULL) )
            return(create_error(SPELL_ERR_BADDICT));

    if(!makeindex(dict, &curword, word))
        return(create_error(SPELL_ERR_BADWORD));

    /* check if word exists and get position */
    if((res = lookupword(dict, &curword, TRUE)) < 0)
        return(res);

    /* check if dictionary is read only */
    if(dp->dictflags & DICT_READONLY)
        return(create_error(SPELL_ERR_READONLY));

    if(!res)
        return(create_error(SPELL_ERR_NOTFOUND));

    lett = ixp(dict, curword.lettix);
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
    if((err = fetchblock(dict, curword.lettix)) != 0)
        return(err);

    token_start = dp->token_start;
    char_offset = dp->char_offset;

    /* after succesful find, the pointer points
    at the token of the word found */
    lett = ixp(dict, curword.lettix);
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

    lett = ixp(dict, curword.lettix);
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

extern S32
spell_flush(
    S32 dict)
{
    FILE_HANDLE dicthand;
    S32 err;

    trace_0(TRACE_MODULE_SPELL, "spell_flush");

    if((dict < 0)  ||  (dict >= MAX_DICT))
        return(create_error(SPELL_ERR_BADDICT));

    if((dicthand = dictlist[dict].dicthandle) == NULL)
    {
        trace_1(TRACE_OUT | TRACE_MODULE_SPELL, "spell_flush called to flush non-open dictionary: %d", dict);
        return(create_error(SPELL_ERR_CANTWRITE));
    }

    /* write out any modified part */
    err = ensuredict(dict);

    /* make sure no cache blocks left */
    stuffcache(dict);

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

extern S32
spell_iswordc(
    S32 dict,
    S32 ch)
{
    DIXP dp;

    if( (dict < 0)  ||  (dict >= MAX_DICT)  ||
        ((dp = &dictlist[dict])->dicthandle == NULL) )
            return(create_error(SPELL_ERR_BADDICT));

    return(dp->case_map[ch]);
}

/******************************************************************************
*
* load a dictionary
*
* all the dictionary is loaded and locked into place
*
******************************************************************************/

extern S32
spell_load(
    S32 dict)
{
    S32 err, i;
    DIXP dp;

    if( (dict < 0)  ||  (dict >= MAX_DICT)  ||
        ((dp = &dictlist[dict])->dicthandle == NULL) )
            return(create_error(SPELL_ERR_BADDICT));

    for(i = 0; i < dp->n_index; ++i)
    {
        /* is there a block defined ? */
        if(!ixpdp(dp, i)->blklen)
            continue;

        if((err = fetchblock(dict, i)) != 0)
            return(err);

        ixpdp(dp, i)->letflags |= LET_LOCKED;
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

extern S32
spell_nextword(
    S32 dict,
    char *wordout,
    char *wordin,
    char *mask,
    P_S32 brkflg)
{
    S32 res, gotw;

    trace_5(TRACE_MODULE_SPELL, "spell_nextword(%d, &%p, %s, %s, &%p)",
            dict, report_ptr_cast(wordout), trace_string(wordin), trace_string(mask), report_ptr_cast(brkflg));

    if((dict < 0)  ||  (dict >= MAX_DICT)  ||
       (dictlist[dict].dicthandle == NULL))
            return(create_error(SPELL_ERR_BADDICT));

    if(badcharsin(dict, wordin))
        return(create_error(SPELL_ERR_BADWORD));
    if(mask && badcharsin(dict, mask))
        return(create_error(SPELL_ERR_BADWORD));

    /* check for start of dictionary */
    gotw = initmatch(dict, wordout, wordin, mask);

    if(gotw && (gotw = spell_checkword(dict, wordout)) < 0)
        return(gotw);

    do
        {
        if(brkflg && *brkflg)
            return(create_error(SPELL_ERR_ESCAPE));

        if(!gotw)
        {
            if((res = nextword(dict, wordout)) < 0)
                return(res);
        }
        else
        {
            res = 1;
            gotw = 0;
        }
    }
    while(res &&
          matchword(dict, mask, wordout) &&
          ((res = !endmatch(dict, wordout, mask, 1)) != 0));

    /* return blank word at end */
    if(!res)
        *wordout = '\0';

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

extern S32
spell_opendict(
    P_U8 name,
    P_U8 def_name,
    char **copy_right)
{
    S32  fpos, err;
    S32 dict, i, nmemb;
    DIXP dp;
    SIXP lett, tempix;

    #ifndef NDEBUG
    {
    /* trap chars being signed */
    char ch = '\x80';
    assert(ch > 127);
    }
    #endif

    /* register interest in full events */
    if(!full_event_registered)
    {
        status_assert(al_full_event_client_register(spell_full_events));
        full_event_registered = 1;
    }

    /* get a dictionary number */
    if((dict = get_dict_entry()) < 0)
        return(dict);

    /* dummy loop for structure */
    do  {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(struct IXSTRUCT), TRUE);
        /* save dictionary parameters */
        dp = &dictlist[dict];

        /* look for the file */
        if(file_open(name, file_open_read, &dp->dicthandle), dp->dicthandle == NULL)
        {
            err = create_error(SPELL_ERR_CANTOPEN);
            break;
        }

        /* take copy of name for reopening in setoptions */
        xstrkpy(dp->dict_filename, elemof32(dp->dict_filename), name);

        file_buffer(dp->dicthandle, NULL, BUF_SIZE);

        /* load dictionary definition */
        if((err = load_dict_def(dict, def_name)) < 0)
            break;

        if(NULL == al_array_alloc(&dp->dicth, struct IXSTRUCT, dp->n_index, &array_init_block, &err))
            break;

        /* load index */
        tempix = array_base(&dp->dicth, struct IXSTRUCT);
        nmemb  = dp->n_index;
        if((err = file_read(tempix,
                            sizeof(struct IXSTRUCT),
                            nmemb,
                            dp->dicthandle)) < 0)
            break;

        /* read size of dictionary file */
        if((err = file_seek(dp->dicthandle, 0, SEEK_END)) < 0)
            break;

        if((fpos = file_tell(dp->dicthandle)) < 0)
        {
            err = fpos;
            break;
        }

        dp->dictsize = fpos - dp->data_offset;

        /* if dictionary can be updated, re-open for update */
        if(!(dp->dictflags & DICT_READONLY))
        {
            file_close(&dp->dicthandle);

            /* look for the file */
            if(file_open(name, file_open_readwrite, &dp->dicthandle), dp->dicthandle == NULL)
            {
                err = create_error(SPELL_ERR_CANTOPEN);
                break;
            }

            file_buffer(dp->dicthandle, 0, BUF_SIZE);
        }
    }
    while(FALSE);

    if(err < 0)
    {
        if(dp->dicthandle)
            file_close(&dp->dicthandle);
        release_dict_entry(dict);
        return(err);
    }

    /* loop over index, masking off unwanted bits */
    for(i = 0, lett = ixpdp(dp, 0); i < dp->n_index; ++i, ++lett)
        lett->letflags &= (LET_ONE | LET_TWO);

    /* return copyright string */
    if(copy_right)
        *copy_right = dp->dict_name;

    trace_1(TRACE_MODULE_SPELL, "spell_open returns: %d", dict);

    return(dict);
}

/******************************************************************************
*
* pack a dictionary
*
******************************************************************************/

extern S32
spell_pack(
    S32 olddict,
    S32 newdict)
{
    S32 err, i;
    S32  diskpoint;
    CACHEP cp;
    DIXP olddp, newdp;
    SIXP lettin, lettout;

    if((err = ensuredict(olddict)) < 0)
        return(err);

    olddp = &dictlist[olddict];
    newdp = &dictlist[newdict];
    diskpoint = newdp->data_offset;
    err = 0;

    for(i = 0; i < olddp->n_index; ++i)
    {
        lettin  = ixpdp(olddp, i);
        lettout = ixpdp(newdp, i);

        /* if no block, copy index entries and continue */
        if(!lettin->blklen)
        {
            *lettout = *lettin;
            lettin->letflags &= LET_ONE | LET_TWO;
            lettout->letflags |= LET_WRITE;
            continue;
        }

        if((err = fetchblock(olddict, i)) != 0)
            break;

        /* re-load index pointers */
        lettin  = ixpdp(olddp, i);
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
        cp->dict = newdict;
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

extern S32
spell_prevword(
    S32 dict,
    char *wordout,
    char *wordin,
    char *mask,
    P_S32 brkflg)
{
    S32 res;

    trace_5(TRACE_MODULE_SPELL, "spell_prevword(%d, &%p, %s, %s, &%p)",
            dict, report_ptr_cast(wordout), trace_string(wordin), trace_string(mask), report_ptr_cast(brkflg));

    if( (dict < 0)  ||  (dict >= MAX_DICT)  ||
        (dictlist[dict].dicthandle == NULL) )
            return(create_error(SPELL_ERR_BADDICT));

    if(badcharsin(dict, wordin))
        return(create_error(SPELL_ERR_BADWORD));
    if(mask && badcharsin(dict, mask))
        return(create_error(SPELL_ERR_BADWORD));

    initmatch(dict, wordout, wordin, mask);

    do  {
        if(brkflg && *brkflg)
            return(create_error(SPELL_ERR_ESCAPE));

        if((res = prevword(dict, wordout)) < 0)
            return(res);
    }
    while(res &&
          matchword(dict, mask, wordout) &&
          ((res = !endmatch(dict, wordout, mask, 0)) != 0));

    /* return blank word at start */
    if(!res)
        *wordout = '\0';

    trace_2(TRACE_MODULE_SPELL, "spell_prevword yields %s, res = %d", wordout, res);
    return(res);
}

/******************************************************************************
*
* set dictionary options
*
******************************************************************************/

extern S32
spell_setoptions(
    S32 dict,
    S32 optionset,
    S32 optionmask)
{
    DIXP dp;
    S32  err;

    if( (dict < 0)  ||  (dict >= MAX_DICT)  ||
        ((dp = &dictlist[dict])->dicthandle == NULL) )
            return(create_error(SPELL_ERR_BADDICT));

    /* may need to open for update! */
    if(dp->dictflags & DICT_READONLY)
    {
        if(file_close(&dp->dicthandle))
            dp->dicthandle = NULL;
        else
            file_open(dp->dict_filename, file_open_readwrite, &dp->dicthandle);

        if(!dp->dicthandle)
        {
            /* if failed to open for update, reopen for reading */
            file_open(dp->dict_filename, file_open_read, &dp->dicthandle);
            return(create_error(SPELL_ERR_CANTWRITE));
        }
    }

    dp->dictflags &= (char) optionmask;
    dp->dictflags |= (char) optionset;

    if((err = file_seek(dp->dicthandle, dp->index_offset - 1, SEEK_SET)) < 0)
        return(err);

    if((err = file_putc(dp->dictflags & DICT_READONLY, dp->dicthandle)) < 0)
        return(err);

    return(0);
}

/******************************************************************************
*
* return statistics about spelling checker
*
******************************************************************************/

extern void
spell_stats(
    P_S32 cblocks,
    P_S32 largest,
    P_S32 totalmem)
{
    LIST_ITEMNO cacheno;
    P_LIST_ITEM it;
    S32 blksiz;

    *cblocks = *largest = 0;
    *totalmem = 0;

    cacheno = 0;
    if((it = list_initseq(cachelp, &cacheno)) != NULL)
    {
        do  {
            CACHEP cp = (CACHEP) it->i.inside;

            ++(*cblocks);
            blksiz = sizeof(struct CACHEBLOCK) +
                     ixp(cp->dict, cp->lettix)->blklen;
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

extern S32
spell_strnicmp(
    S32 dict,
    PC_U8 word1,
    PC_U8 word2,
    S32 len)
{
    S32 ch1, ch2;

    if( (dict < 0)  ||  (dict >= MAX_DICT)  ||
        (dictlist[dict].dicthandle == NULL) )
            return(create_error(SPELL_ERR_BADDICT));

    while((ch1 = char_ordinal_2(dict, spell_toupper(dict, *word1++))) ==
          (ch2 = char_ordinal_2(dict, spell_toupper(dict, *word2++))))
        {
        /* detect when at end of string (NULL or len) */
        if(ch1 < 0 || !(--len))
            return(0);
        }

    return(ch1 > ch2 ? 1 : -1);
}

/******************************************************************************
*
* convert a character to lower case
* using the dictionary's mapping
*
******************************************************************************/

extern S32
spell_tolower(
    S32 dict,
    S32 ch)
{
    S32 i;
    DIXP dp;

    if( (dict < 0)  ||  (dict >= MAX_DICT)  ||
        ((dp = &dictlist[dict])->dicthandle == NULL) )
            return(create_error(SPELL_ERR_BADDICT));

    if(ch >= SPACE)
        for(i = SPACE; i < 256; i++)
            if(i != ch && dp->case_map[i] == (char) ch)
                return(i);

    return(ch);
}

/******************************************************************************
*
* convert a character to upper case
* using the dictionary's mapping
*
******************************************************************************/

extern S32
spell_toupper(
    S32 dict,
    S32 ch)
{
    S32 upper_ch;
    DIXP dp;

    if( (dict < 0)  ||  (dict >= MAX_DICT)  ||
        ((dp = &dictlist[dict])->dicthandle == NULL) )
            return(create_error(SPELL_ERR_BADDICT));

    upper_ch = dp->case_map[ch];

    if(!upper_ch)
        upper_ch = ch;

    return(upper_ch);
}

/******************************************************************************
*
* unlock a dictionary
*
******************************************************************************/

extern S32
spell_unlock(
    S32 dict)
{
    S32 i, n_index;
    SIXP lett;
    DIXP dp;

    if( (dict < 0)  ||  (dict >= MAX_DICT)  ||
        ((dp = &dictlist[dict])->dicthandle == NULL) )
            return(create_error(SPELL_ERR_BADDICT));

    for(i = 0, lett = ixp(dict, 0), n_index = dp->n_index;
        i < n_index;
        ++i, ++lett)
        lett->letflags &= ~LET_LOCKED;

    return(0);
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

extern S32
spell_valid_1(
    S32 dict,
    S32 ch)
{
    if( (dict < 0)  ||  (dict >= MAX_DICT)  ||
        (dictlist[dict].dicthandle == NULL) )
            return(create_error(SPELL_ERR_BADDICT));

    return(char_ordinal_1(dict, spell_toupper(dict, ch)) >= 0);
}

/******************************************************************************
*
* ensure string contains only chars valid for wild match
*
******************************************************************************/

static S32
badcharsin(
    S32 dict,
    char *str)
{
    char ch;

    while((ch = *str++) != 0)
        if(!(spell_iswordc(dict, (S32) ch) ||
             (ch == SPELL_WILD_SINGLE)      ||
             (ch == SPELL_WILD_MULTIPLE)))
            return(TRUE);

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

static S32
char_ordinal_1(
    S32 dict,
    S32 ch)
{
    DIXP dp;
    char i;

    if(ch >= 0)
    {
        dp = &dictlist[dict];

        for(i = 0; i < dp->n_index_1; ++i)
            if(dp->letter_1[i] == (char) ch)
                return(i + dp->char_offset);
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

static S32
char_ordinal_2(
    S32 dict,
    S32 ch)
{
    DIXP dp;
    char i;

    if(ch >= 0)
    {
        dp = &dictlist[dict];

        for(i = 0; i < dp->n_index_2; ++i)
            if(dp->letter_2[i] == (char) ch)
                return(i + dp->char_offset);
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

static S32
char_ordinal_3(
    S32 dict,
    S32 ch)
{
    DIXP dp;
    char i;

    if(ch >= 0)
    {
        dp = &dictlist[dict];

        for(i = 0; i < dp->n_index_2; ++i)
            if(dp->letter_2[i] == (char) ch)
                return(dp->man_token_start ? ch : i + dp->char_offset);
    }

    return(create_error(SPELL_ERR_BADWORD));
}

/******************************************************************************
*
* compare two strings for sort routine
*
******************************************************************************/

PROC_QSORT_PROTO(static, compar, PC_L1STR)
{
    P_PC_L1STR word_1 = (P_PC_L1STR) arg1;
    P_PC_L1STR word_2 = (P_PC_L1STR) arg2;

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
    ENDP end1 = (ENDP) arg1;
    ENDP end2 = (ENDP) arg2;

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
    ENDP end1 = (ENDP) arg1;
    ENDP end2 = (ENDP) arg2;

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
    S32 dict,
    char *word,
    WRDP wp,
    S32 len)
{
    P_U8 ci;
    char *co;
    DIXP dp = &dictlist[dict];

    *(word + 0) = (char)
                  spell_tolower(dict,
                                ordinal_char_1(dict,
                                               wp->lettix / dp->n_index_2 +
                                               dp->char_offset));

    if(len == 1)
    {
        *(word + 1) = '\0';
        return(1);
    }

    *(word + 1) = (char)
                  spell_tolower(dict,
                                ordinal_char_2(dict,
                                               wp->lettix % dp->n_index_2 +
                                               dp->char_offset));

    if(len == 2)
    {
        *(word + 2) = '\0';
        return(2);
    }

    /* decode body */
    ci = wp->bodyd;
    co = word + 2;
    while(*ci)
        *co++ = (char) spell_tolower(dict, ordinal_char_3(dict, *ci++));

    /* decode ending */
    ci = ((I_ENDP) (list_gotoitem(&dp->dict_end_list,
                                  (LIST_ITEMNO) wp->tail)->i.inside))->ending;

    while(*ci)
        *co++ = (char) spell_tolower(dict, ordinal_char_3(dict, *ci++));
    *co = '\0';

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
    char keystr[MAX(sizeof(OLD_KEYSTR), sizeof(KEYSTR))];
    S32 keylen, nbytes;
    S32 err;

    trace_0(TRACE_MODULE_SPELL, "def_file_position");

    /* position to start of file */
    if((err = file_seek(def_file, 0, SEEK_SET)) < 0)
        return(err);

    /* read key string to determine if it's a dictionary */
    nbytes = MAX(strlen(OLD_KEYSTR), strlen(KEYSTR));
    if((err = file_read(keystr,
                        sizeof(char),
                        nbytes,
                        def_file)) < 0)
        return(err);

    /* is it old dictionary format ? */
    if(0 == strncmp(keystr, OLD_KEYSTR, strlen(OLD_KEYSTR)))
        return(create_error(SPELL_ERR_BADDEFFILE));

    trace_0(TRACE_MODULE_SPELL, "def_file_position: is not old format dictionary");

    if(0 == strncmp(keystr, KEYSTR, strlen(KEYSTR)))
        keylen = strlen(KEYSTR);
    else
        keylen = 0;

    if((err = file_seek(def_file, keylen, SEEK_SET)) < 0)
        return(err);

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
        trace_2(TRACE_MODULE_SPELL, "cp: %p, ixp: %p", report_ptr_cast(cp), report_ptr_cast(ixp(cp->dict, cp->lettix)));
        ixp(cp->dict, cp->lettix)->p.cacheno = i;

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
    S32 dict,
    char *word,
    WRDP wp)
{
    P_U8 deadwords[MAX_TOKEN - MIN_TOKEN + 1];
    char realword[MAX_WORD + 1];
    S32 wordc = 0, len, i, err;
    char token_start;
    P_U8 datap, ep;
    CACHEP cp, ocp;
    SIXP lett;

    token_start = dictlist[dict].token_start;
    lett = ixp(dict, wp->lettix);
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
        len = decodeword(dict, realword, wp, 0);
        if(NULL == (deadwords[wordc] = al_ptr_alloc_bytes(P_U8, len + 1/*NULLCH*/, &err)))
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
        if(spell_strnicmp(dict, deadwords[i], word, -1) > 0)
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
        if((err = spell_deleteword(dict, word_buf)) < 0)
            return(err);
    }

    /* add the new word */
    if(NULL == (deadwords[wordc] = al_ptr_alloc_bytes(P_U8, strlen(word) + 1/*NULLCH*/, &err)))
        return(err);
    strcpy(deadwords[wordc++], word);

    /* and put back all the words we deleted */
    compar_dict = dict;
    qsort((void *) deadwords, wordc, sizeof(deadwords[0]), compar);
    for(i = 0; i < wordc; ++i)
    {
        char word_buf[MAX_WORD + 1];

        strcpy(word_buf, deadwords[i]);
        if((err = spell_addword(dict, word_buf)) < 0)
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
    S32 dict,
    char *word,
    char *mask,
    S32 updown)
{
    S32 len, res;
    char *ci;

    if(!mask || !*mask)
        return(0);

    len = 0;
    ci = mask;
    while(spell_iswordc(dict, (S32) *ci))
    {
        ++ci;
        ++len;
    }

    if(!len)
        return(0);

    res = spell_strnicmp(dict, mask, word, len);

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
    S32 dict)
{
    S32 err, allerr, i;
    DIXP dp;

    trace_0(TRACE_MODULE_SPELL, "ensuredict");

    /* work down the index and write out anything altered */
    for(i = 0, err = allerr = 0, dp = &dictlist[dict];
        i < dp->n_index;
        ++i)
    {
        SIXP lett = ixpdp(dp, i);

        trace_1(TRACE_MODULE_SPELL, "ensure letter: %d", i);
        if(lett->letflags & LET_WRITE)
        {
            if(lett->letflags & LET_CACHED)
                err = killcache(lett->p.cacheno);
            else
            {
                /* mask flags to be written to disk */
                lett->letflags &= LET_ONE | LET_TWO;
                err = writeindex(dict, i);
            }

            if(err)
                allerr = allerr ? allerr : err;
            else
                ixpdp(dp, i)->letflags &= ~LET_WRITE;
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
    S32 dict,
    S32 lettix)
{
    CACHEP newblock;
    P_LIST_ITEM it;
    FILE_HANDLE dicthand;
    S32 err, nbytes;
    DIXP dp;
    SIXP lett;

    dp = &dictlist[dict];

    /* check if it's already cached */
    if(ixpdp(dp, lettix)->letflags & LET_CACHED)
        return(0);

    trace_3(TRACE_MODULE_SPELL, "fetchblock dict: %d, letter: %d, cachelp: %p",
            dict, lettix, report_ptr_cast(cachelp));

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
            if((it = list_createitem(cachelp,
                                     list_numitem(cachelp),
                                     sizeof(struct CACHEBLOCK) +
                                        ixpdp(dp, lettix)->blklen,
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
    lett = ixpdp(dp, lettix);

    /* read the data if there is any */
    if(lett->p.disk)
    {
        /* position for the read */
        trace_0(TRACE_MODULE_SPELL, "fetchblock doing seek");
        dicthand = dictlist[dict].dicthandle;
        if((err = file_seek(dicthand, lett->p.disk, SEEK_SET)) < 0)
        {
            deletecache(list_atitem(cachelp));
            return(err);
        }

        /* read in the block */
        trace_0(TRACE_MODULE_SPELL, "fetchblock doing read");
        nbytes = lett->blklen + sizeof(S32);
        if((err = file_read(&newblock->diskspace,
                            sizeof(char),
                            nbytes,
                            dicthand)) < 0)
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
    newblock->dict = dict;
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
               !(ixp(cp->dict, cp->lettix)->letflags & LET_LOCKED))
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
    bytes_freed = ixp(cp->dict, cp->lettix)->blklen;

    #if TRACE_ALLOWED
    {
    S32 blocks, largest;
    S32 totalmem;

    trace_1(TRACE_MODULE_SPELL, "spell freecache has freed a block of: %d bytes",
            ixp(cp->dict, cp->lettix)->blklen);
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
get_dict_entry(void)
{
    S32 i;

    for(i = 0; i < MAX_DICT; ++i)
        if(dictlist[i].dicthandle == NULL)
        {
            DIXP dp = &dictlist[i];

            dp->dicth          = 0;
            dp->dictsize       = 0;
            dp->token_start    = dp->char_offset = 0;
            dp->data_offset    = dp->index_offset = 0;
            dp->n_index        = 0;
            dp->n_index_1      = dp->n_index_2 = 0;
            dp->dictflags      = 0;

            /* initialise endings list */
            list_init(&dp->dict_end_list, END_MAXITEMSIZE, END_MAXPOOLSIZE);
            list_register(&dp->dict_end_list);

            return(i);
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
    S32 dict,
    char *wordout,
    char *wordin,
    char *mask)
{
    char *ci, *co;

    /* if no wordin, initialise from mask */
    if(!*wordin)
    {
        ci = mask;
        co = wordout;
        do  {
            if(!ci                          ||
               !*ci                         ||
               (*ci == SPELL_WILD_MULTIPLE) ||
               (*ci == SPELL_WILD_SINGLE))
            {
                if(!ci || (ci == mask))
                {
                    *co++ = dictlist[dict].letter_1[0];
                    *co = '\0';

                    return((ixp(dict, 0)->letflags & LET_ONE) ? 1 : 0);
                }
                *co = '\0';
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
    FILE_HANDLE dicthand;
    SIXP lett;

    err = 0;
    cp = (CACHEP) list_gotoitem(cachelp, cacheno)->i.inside;

    trace_1(TRACE_MODULE_SPELL, "killcache cacheno: %d", cacheno);

    /* write out block if altered */
    if((write = (ixp(cp->dict, cp->lettix)->letflags & LET_WRITE)) != 0)
        if((err = writeblock(cp)) != 0)
            return(err);

    /* clear flags in index */
    lett = ixp(cp->dict, cp->lettix);
    lett->letflags &= LET_ONE | LET_TWO;

    /* save disk address in index */
    lett->p.disk = cp->diskaddress;

    /* write out index entry */
    dicthand = dictlist[cp->dict].dicthandle;
    if(write && (err = writeindex(cp->dict, cp->lettix)) != 0)
    {
        /* make the dictionary useless
         * if not properly updated
        */
        trace_1(TRACE_MODULE_SPELL, "write of index block: %d failed", cp->lettix);
        file_clearerror(dicthand);
        (void) file_seek(dicthand, 0, SEEK_SET);
        (void) file_putc(0, dicthand);
    }

    /* throw away the cache block */
    deletecache(cacheno);

    /* flush buffer */
    trace_0(TRACE_MODULE_SPELL, "flushing buffer");
    if((flush_err = file_flush(dicthand)) < 0 && !err)
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
    S32 dict,
    P_U8 def_name)
{
    DIXP dp = &dictlist[dict];
    char keystr[MAX(sizeof(OLD_KEYSTR), sizeof(KEYSTR))];
    S32 keylen, res, nbytes;
    FILE_HANDLE def_file;
    S32  err;

    /* position to start of dictionary */
    if((err = file_seek(dp->dicthandle, 0, SEEK_SET)) < 0)
        return(err);

    /* read key string to determine dictionary type */
    nbytes = MAX(strlen(OLD_KEYSTR), strlen(KEYSTR));
    if((err = file_read(keystr,
                        sizeof(char),
                        nbytes,
                        dp->dicthandle)) < 0)
        return(err);

    /* is it old dictionary format ? */
    if(0 == strncmp(keystr, OLD_KEYSTR, strlen(OLD_KEYSTR)))
    {
        /* try to open definition file */
        if(file_open(def_name, file_open_read, &def_file), def_file == NULL)
            return(create_error(SPELL_ERR_BADDEFFILE));

        file_buffer(def_file, NULL, BUF_SIZE);
        keylen = strlen(OLD_KEYSTR);
    }
    else if(0 == strncmp(keystr, KEYSTR, strlen(KEYSTR)))
    {
        keylen = strlen(KEYSTR);
        def_file = dp->dicthandle;
    }
    else
        return(create_error(SPELL_ERR_BADDICT));

    res = load_dict_def_now(def_file, dict, keylen);

    /* close definition file if separate */
    if(def_file != dp->dicthandle)
    {
        if((err = file_close(&def_file)) < 0 && res >= 0)
            res = err;
    }

    return(res);
}

/******************************************************************************
*
* process a dictionary definition given a handle and key length
*
******************************************************************************/

static S32
load_dict_def_now(
    FILE_HANDLE def_file,
    S32 dict,
    S32 keylen)
{
    DIXP dp = &dictlist[dict];
    ENDSTRUCT token[MAX_TOKEN - MIN_TOKEN];
    char buffer[255 + 1];
    S32 res, i, end;
    char *in, *out;
    ENDP ep;
    S32  filepos;
    S32  err;

    /* position dictionary */
    if((err = file_seek(dp->dicthandle, keylen, SEEK_SET)) < 0)
        return(err);

    /* read dictionary name */
    if(read_def_line(def_file, buffer) <= 0)
        return(create_error(SPELL_ERR_BADDEFFILE));

    xstrkat(dp->dict_name, elemof32(dp->dict_name), buffer);

    /* read character offset */
    if(read_def_line(def_file, buffer) <= 0)
        return(create_error(SPELL_ERR_BADDEFFILE));

    dp->char_offset = (char) atoi(buffer);

    if(dp->char_offset < MIN_CHAR || dp->char_offset >= MAX_CHAR)
        return(create_error(SPELL_ERR_BADDEFFILE));

    /* read token offset */
    if(read_def_line(def_file, buffer) <= 0)
        return(create_error(SPELL_ERR_BADDEFFILE));

    dp->man_token_start = (char) atoi(buffer);

    if(dp->man_token_start                      &&
       ((S32) dp->man_token_start <  MIN_TOKEN ||
        (S32) dp->man_token_start >= MAX_TOKEN))
        return(create_error(SPELL_ERR_BADDEFFILE));

    /* read first letter list */
    if((res = read_def_line(def_file, buffer)) <= 0 || res > MAX_INDEX)
        return(create_error(SPELL_ERR_BADDEFFILE));

    dp->n_index_1 = (char) (res - 1);
    in            = buffer;
    out           = dp->letter_1;
    while(*in)
    {
        if(dp->man_token_start &&
           (*in >= dp->man_token_start ||
            *in <  dp->char_offset))
            return(create_error(SPELL_ERR_DEFCHARERR));

        *out++ = *in++;
    }

    /* read second letter list */
    if((res = read_def_line(def_file, buffer)) <= 0 || res > MAX_INDEX)
        return(create_error(SPELL_ERR_DEFCHARERR));

    dp->n_index_2 = (char) (res - 1);
    in            = buffer;
    out           = dp->letter_2;
    while(*in)
    {
        if(dp->man_token_start &&
           (*in >= dp->man_token_start ||
            *in <  dp->char_offset))
            return(create_error(SPELL_ERR_DEFCHARERR));

        *out++ = *in++;
    }

    /* initialise case map */
    for(i = 0; i < 256; i++)
        dp->case_map[i] = 0;

    /* flag OK characters in case map */
    for(i = 0, in = buffer; *in; ++in)
        dp->case_map[*in] = *in;

    /* read case equivalence list */
    if((res = read_def_line(def_file, buffer)) <= 0 ||
       (char) (res - 1) > dp->n_index_2)
        return(create_error(SPELL_ERR_DEFCHARERR));

    /* insert case equivalences */
    for(i = 0, in = buffer; *in; ++in, ++i)
        dp->case_map[*in] = dp->letter_2[i];

    /* set number of index elements */
    dp->n_index = (S32) dp->n_index_1 * dp->n_index_2;

    if(!(dp->token_start = dp->man_token_start))
        /* set auto token_start */
        dp->token_start = dp->char_offset + dp->n_index_2;
    else
        /* check manual token start is high enough */
        if(dp->char_offset + dp->n_index_2 > dp->man_token_start)
            return(create_error(SPELL_ERR_DEFCHARERR));

    /* insert ending zero - null token */
    token[0].len       = 0;
    token[0].alpha     = 0;
    token[0].pos       = 255;
    token[0].ending[0] = '\0';

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
            if((ch = char_ordinal_3(dict, spell_toupper(dict, *in++))) <= 0)
                break;
            *out++ = (char) ch;
        }

        if(!i || i == MAX_ENDLEN)
            return(create_error(SPELL_ERR_BADDEFFILE));

        *out++ = '\0';
        ep->len = (char) strlen(ep->ending);
        ep->pos = (char) end;
    }

    /* check we read some endings */
    if(res >= 0 && end == 1)
        res = create_error(SPELL_ERR_BADDEFFILE);
    if(res < 0)
        return(res);

    /* check that tokens and all fit into the space */
    if(dp->token_start + end >= MAX_TOKEN)
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

        if((it = list_createitem(&dp->dict_end_list,
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
    list_packlist(&dp->dict_end_list, (LIST_ITEMNO) -1);

    /* read dictionary flags */
    if((res = file_getc(dp->dicthandle)) < 0)
        return(res);

    dp->dictflags = (char) res;

    /* read position of start of index */
    if((filepos = file_tell(dp->dicthandle)) < 0)
        return(filepos);

    /* set offset parameters */
    dp->index_offset = filepos;
    dp->data_offset  = filepos + dp->n_index * sizeof(struct IXSTRUCT);

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
    S32 dict,
    WRDP wp,
    S32 needpos)
{
    P_U8 startlim, endlim, midpoint, datap;
    char *co, *ci;
    S32 i, ch, found, bodylen, updown, err;
    char token_start, char_offset;
    CACHEP cp;
    SIXP lett;
    DIXP dp;
    P_LIST_BLOCK dlp;

    if(needpos)
    {
        wp->fail = INS_WORD;
        wp->findpos = wp->matchc = wp->match = wp->matchcp = wp->matchp = 0;
    }

    lett = ixp(dict, wp->lettix);

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
    if((err = fetchblock(dict, wp->lettix)) != 0)
        return(err);

    /* load some variables */
    dp = &dictlist[dict];
    token_start = dp->token_start;
    char_offset = dp->char_offset;
    dlp = &dp->dict_end_list;

    /* search the block for the word */
    lett = ixp(dict, wp->lettix);
    cp = (CACHEP) list_gotoitem(cachelp, lett->p.cacheno)->i.inside;
    ++cp->usecount;

    /*
    do a binary search on the third letter
    */

    startlim = cp->data;
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

    *wp->bodyd = '\0';
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
        *co = '\0';

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
    S32 dict,
    WRDP wp,
    char *word)
{
    S32 ch, len;
    char *co;
    DIXP dp = &dictlist[dict];

    /* check length */
    len = strlen(word);

    /* we can use more than char_offset since two letters
    at the start of every word are stripped by the index */
    if(!len || len >= dp->char_offset + 2)
        return(NULL);
    wp->len = len;

    /* process first letter */
    if((ch = char_ordinal_1(dict, spell_toupper(dict, *word++))) < 0)
        return(NULL);
    wp->lettix = (ch - dp->char_offset) * (S32) dp->n_index_2;

    /* process second letter */
    if(len > 1)
    {
        if((ch = char_ordinal_2(dict, spell_toupper(dict, *word++))) < 0)
            return(NULL);

        wp->lettix += ch - dp->char_offset;
    }

    /* copy across body */
    co = wp->body;
    while(*word)
    {
        if((ch = char_ordinal_3(dict, spell_toupper(dict, *word++))) < 0)
            return(NULL);
        *co++ = (char) ch;
    }
    *co++ = '\0';

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
    S32 dict,
    char *mask,
    char *word)
{
    char *maskp, *wordp, *star, *nextpos;

    if(!mask || (*mask == '\0'))
        return(0);

    maskp = star = mask;
    wordp = nextpos = word;

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

            if(spell_toupper(dict, *maskp) != spell_toupper(dict, *wordp))
            {
                if(*wordp == '\0')
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
                        return(char_ordinal_2(dict,
                                              spell_toupper(dict, *maskp)) <
                               char_ordinal_2(dict,
                                              spell_toupper(dict, *wordp))
                                 ? -1
                                 :  1);
                }
            }

            if(*maskp++ == '\0')
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

static S32
nextword(
    S32 dict,
    char *word)
{
    struct TOKWORD curword;
    S32 err, tokabval, nexthigher, token, curabval, tail, res,
         n_index;
    P_U8 datap, ep;
    char token_start, *co;
    CACHEP cp;
    SIXP lett;
    P_LIST_BLOCK dlp;

    if(!makeindex(dict, &curword, word))
        return(create_error(SPELL_ERR_BADWORD));

    /* check if word exists and get position */
    if((res = lookupword(dict, &curword, TRUE)) < 0)
        return(res);

    if((curword.len == 1) && (ixp(dict, curword.lettix)->letflags & LET_TWO))
        return(decodeword(dict, word, &curword, 2));

    n_index     = dictlist[dict].n_index;
    token_start = dictlist[dict].token_start;
    dlp         = &dictlist[dict].dict_end_list;

    do  {
        if(ixp(dict, curword.lettix)->blklen)
        {
            /* check we have a cache block */
            if((err = fetchblock(dict, curword.lettix)) != 0)
                return(err);

            lett = ixp(dict, curword.lettix);
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
                *co++ = '\0';
            }

            if(datap < ep)
            {
                tokenise(dict, &curword, MAX(curword.matchc, 1));
                if(!curword.match)
                    curabval = -1;
                else
                    curabval = setabval(dict, &curword, 1);

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
                    return(decodeword(dict, word, &curword, 0));
                }

                /* if there were no tokens higher, go onto next root */
                if(++datap >= ep)
                    break;

                co = curword.bodyd + token;

                while((*datap < token_start) && (datap < ep))
                    *co++ = *datap++;

                *co = '\0';
                curabval = -1;
            }
        }

        /* move onto the next letter */
        *curword.bodyd = *curword.bodydp = '\0';
        curword.tail   = curword.matchc = curword.matchcp =
        curword.match  = curword.matchp = curword.findpos = 0;

        /* skip down the index till we find
        an entry with some words in it */
        if(++curword.lettix < n_index)
        {
            lett = ixp(dict, curword.lettix);
            if(lett->letflags & LET_ONE)
                return(decodeword(dict, word, &curword, 1));

            if(lett->letflags & LET_TWO)
                return(decodeword(dict, word, &curword, 2));
        }
    }
    while(curword.lettix < n_index);

    *word = '\0';
    return(0);
}

/******************************************************************************
*
* given ordinal number for the first character
* position (including char_offset)
* return the upper case character for this ordinal
*
******************************************************************************/

static S32
ordinal_char_1(
    S32 dict,
    S32 ord)
{
    DIXP dp = &dictlist[dict];

    return(dp->letter_1[ord - dp->char_offset]);
}

/******************************************************************************
*
* given ordinal number for the second character
* position (including char_offset)
* return the upper case character for this ordinal
*
******************************************************************************/

static S32
ordinal_char_2(
    S32 dict,
    S32 ord)
{
    DIXP dp = &dictlist[dict];

    return(dp->letter_2[ord - dp->char_offset]);
}

/******************************************************************************
*
* given ordinal number for the third character
* position (including char_offset)
* return the upper case character for this ordinal
*
******************************************************************************/

static S32
ordinal_char_3(
    S32 dict,
    S32 ord)
{
    DIXP dp = &dictlist[dict];

    return(dp->man_token_start ? ord : dp->letter_2[ord - dp->char_offset]);
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

static S32
prevword(
    S32 dict,
    char *word)
{
    struct TOKWORD curword;
    S32 err, tokabval, nextlower, token, curabval, tail, i, onroot;
    P_U8 sp, ep, datap;
    char token_start, char_offset, *co;
    CACHEP cp;
    SIXP lett;
    DIXP dp;
    P_LIST_BLOCK dlp;

    if(!makeindex(dict, &curword, word))
        return(create_error(SPELL_ERR_BADWORD));

    /* check if word exists and get position */
    if((err = lookupword(dict, &curword, TRUE)) < 0)
        return(err);

    if((curword.len == 2) && (ixp(dict, curword.lettix)->letflags & LET_ONE))
        return(decodeword(dict, word, &curword, 1));

    dp = &dictlist[dict];
    token_start = dp->token_start;
    char_offset = dp->char_offset;
    dlp         = &dp->dict_end_list;

    do  {
        if(ixp(dict, curword.lettix)->blklen)
        {
            /* check we have a cache block */
            if((err = fetchblock(dict, curword.lettix)) != 0)
                return(err);

            lett = ixp(dict, curword.lettix);
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

                tokenise(dict, &curword, MAX(curword.matchc, 1));
                if(!curword.match)
                    curabval = MAX_TOKEN;
                else
                    curabval = setabval(dict, &curword, 0);

                /* move to the start of the root */
                while(*datap >= char_offset)
                    --datap;

                /* build a body at the start of a block */
                if(*datap == '\0')
                {
                    /* skip null starter */
                    ++datap;
                    co = curword.bodyd;
                    while(*datap < token_start)
                        *co++ = *datap++;
                    *co++ = '\0';
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
                    return(decodeword(dict, word, &curword, 0));
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

        lett = ixp(dict, curword.lettix);
        if(datap == sp || !lett->blklen)
        {
            char last_ch;
            S32 max_word;

            /* if we are at the start of the block */
            do  {
                /* return the small words if there are some */
                if((curword.len > 2) && (lett->letflags & LET_TWO))
                    return(decodeword(dict, word, &curword, 2));
                if((curword.len > 1) && (lett->letflags & LET_ONE))
                    return(decodeword(dict, word, &curword, 1));

                if(--curword.lettix < 0)
                    break;

                lett = ixp(dict, curword.lettix);
            }
            while(!lett->blklen);

            /* quit if at beginning */
            if(curword.lettix < 0)
                break;

            /* build previous letter ready for search */
            decodeword(dict, word, &curword, 2);
            last_ch  = dp->letter_2[dp->n_index_2 - 1];
            max_word = dp->char_offset + 1;
            for(i = 2, co = word + 2; i < max_word; ++i)
                *co++ = last_ch;
            *co = '\0';

            /* set position to end of block */
            makeindex(dict, &curword, word);
        }
        else
        {
            char ch;
            char last_ch;
            S32 max_word;

            /* if we are at the start of a letter in a block */
            co = word + 2;

            /* set word to 'last of previous letter' */
            ch = (char) spell_toupper(dict, *co);
            *co++ = dp->letter_2[char_ordinal_2(dict, ch) - dp->char_offset - 1];

            last_ch  = (char) spell_tolower(dict, dp->letter_2[dp->n_index_2 - 1]);
            max_word = dp->char_offset + 1;

            for(i = 3; i < max_word; ++i)
                *co++ = last_ch;
            *co = '\0';

            makeindex(dict, &curword, word);
        }

        if((err = lookupword(dict, &curword, TRUE)) < 0)
            return(err);
    }
    while(curword.lettix >= 0);

    *word = '\0';
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
        *trail++ = '\0';

    return(trail - buffer);
}

/******************************************************************************
*
* give back entry in dictionary table
*
******************************************************************************/

static void
release_dict_entry(
    S32 dict)
{
    DIXP dp = &dictlist[dict];

    list_free(&dp->dict_end_list);
    list_deregister(&dp->dict_end_list);

    /* free memory used by index */
    al_array_dispose(&dp->dicth);

    /* throw away file handle */
    dp->dicthandle = NULL;
}

/******************************************************************************
*
* set alphabetic value of ending of tokenised word
*
******************************************************************************/

static S32
setabval(
    S32 dict,
    WRDP wp,
    S32 lo_hi)
{
    DIXP dp = &dictlist[dict];
    char ending[MAX_WORD + 1];
    I_ENDP iep;
    S32 i, abval;
    P_LIST_BLOCK dlp;

    /* find entry for ending */
    dlp = &dp->dict_end_list;
    iep = (I_ENDP) list_gotoitem(dlp,
                                 (LIST_ITEMNO)
                                 wp->tail - dp->token_start)->i.inside;

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
            if(spell_strnicmp(dict, iep->ending, ending, -1) <= 0 &&
               (S32) iep->alpha > abval)
                abval = (S32) iep->alpha;
        }
        else
        {
            if(spell_strnicmp(dict, iep->ending, ending, -1) >= 0 &&
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
    S32 dict)
{
    LIST_ITEMNO i;

    trace_0(TRACE_MODULE_SPELL, "stuffcache");

    /* write out/remove any blocks
    from this dictionary */
    for(i = 0; i < list_numitem(cachelp); ++i)
    {
        CACHEP cp = (CACHEP) list_gotoitem(cachelp, i)->i.inside;

        if(cp->dict == dict)
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
    S32 dict,
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

    token_start = dictlist[dict].token_start;
    dlp         = &dictlist[dict].dict_end_list;

    for(i = 0; i < (S32) list_numitem(dlp); ++i)
    {
        I_ENDP curend = (I_ENDP) (list_gotoitem(dlp, (LIST_ITEMNO) i)->i.inside);

        if(maxtail < (S32) curend->len)
            continue;

        if(0 == strcmp(curend->ending, endbody - curend->len))
        {
            wp->tail = i + token_start;
            *(endbody - curend->len) = '\0';
            break;
        }
    }
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
    FILE_HANDLE dicthand;
    DIXP dp;
    SIXP lett;

    trace_2(TRACE_MODULE_SPELL, "writeblock dict: %d, letter: %d", cp->dict, cp->lettix);

    lett = ixp(cp->dict, cp->lettix);

    if(lett->blklen)
    {
        S32 err;

        dp = &dictlist[cp->dict];
        dicthand = dp->dicthandle;

        /* do we need to extend file ? */
        if((S32) lett->blklen > cp->diskspace)
        {
            if((err = file_seek(dicthand,
                                dp->data_offset + dp->dictsize + lett->blklen + EXT_SIZE,
                                SEEK_SET)) < 0)
                return(err);

            if((err = file_putc(0, dicthand)) < 0)
                return(err);

            cp->diskaddress = dp->data_offset + dp->dictsize;
            dp->dictsize   += lett->blklen + EXT_SIZE;
            cp->diskspace  += EXT_SIZE;
        }

        /* write out block */
        if((err = file_seek(dicthand, cp->diskaddress, SEEK_SET)) < 0)
            return(err);

        if((err = file_write(&cp->diskspace,
                             lett->blklen + sizeof(S32),
                             1,
                             dicthand)) < 0)
            return(err);
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
    S32 dict,
    S32 lettix)
{
    DIXP dp = &dictlist[dict];
    S32 err;

    /* update index entry for letter */
    if((err = file_seek(dp->dicthandle,
                        dp->index_offset + sizeof(struct IXSTRUCT) * lettix,
                        SEEK_SET)) < 0)
        return(err);

    if((err = file_write(ixpdp(dp, lettix),
                         sizeof(struct IXSTRUCT),
                         1,
                         dp->dicthandle)) < 0)
        return(err);

    return(0);
}

/* end of spell.c */
