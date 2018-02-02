/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 2001                             */
/*                        All Rights Reserved.                           */
/*                                                                       */
/*  Permission is hereby granted, free of charge, to use and distribute  */
/*  this software and its documentation without restriction, including   */
/*  without limitation the rights to use, copy, modify, merge, publish,  */
/*  distribute, sublicense, and/or sell copies of this work, and to      */
/*  permit persons to whom this work is furnished to do so, subject to   */
/*  the following conditions:                                            */
/*   1. The code must retain the above copyright notice, this list of    */
/*      conditions and the following disclaimer.                         */
/*   2. Any modifications must be clearly marked as such.                */
/*   3. Original authors' names are not deleted.                         */
/*   4. The authors' names are not used to endorse or promote products   */
/*      derived from this software without specific prior written        */
/*      permission.                                                      */
/*                                                                       */
/*  CARNEGIE MELLON UNIVERSITY AND THE CONTRIBUTORS TO THIS WORK         */
/*  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      */
/*  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE      */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    */
/*  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   */
/*  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          */
/*  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       */
/*  THIS SOFTWARE.                                                       */
/*                                                                       */
/*************************************************************************/
/*             Author:  Alan W Black (awb@cs.cmu.edu)                    */
/*               Date:  January 1999                                     */
/*************************************************************************/
/*                                                                       */
/*  User defined type registration                                       */
/*                                                                       */
/*  I'd like to make this file be automatically generated                */
/*                                                                       */
/*************************************************************************/
#include "cst_val.h"
#include "cst_string.h"

CST_VAL_REG_TD_TYPE(utterance,cst_utterance,7)
CST_VAL_REG_TD_TYPE(wave,cst_wave,9)
CST_VAL_REG_TD_TYPE(track,cst_track,11)
CST_VAL_REG_TD_TYPE(lpcres,cst_lpcres,13)
CST_VAL_REG_TD_FUNCPTR(uttfunc,cst_uttfunc,15)
CST_VAL_REG_TD_FUNCPTR(ffunc,cst_ffunction,17)
CST_VAL_REG_TD_TYPE_NODEL(relation,cst_relation,19)
CST_VAL_REG_TD_TYPE_NODEL(item,cst_item,21)
CST_VAL_REG_TD_TYPE_NODEL(cart,cst_cart,23)
CST_VAL_REG_TD_TYPE_NODEL(phoneset,cst_phoneset,25)
CST_VAL_REG_TD_TYPE_NODEL(lexicon,cst_lexicon,27)
CST_VAL_REG_TD_TYPE_NODEL(dur_stats,dur_stats,29)
CST_VAL_REG_TD_TYPE_NODEL(diphone_db,cst_diphone_db,31)
CST_VAL_REG_TD_TYPE_NODEL(clunit_db,cst_clunit_db,33)
CST_VAL_REG_TD_TYPE_NODEL(vit_cand,cst_vit_cand,35)
CST_VAL_REG_TD_TYPE_NODEL(sts_list,cst_sts_list,37)
CST_VAL_REG_TD_TYPE_NODEL(userdata,cst_userdata,41)
CST_VAL_REGISTER_TYPE_NODEL(userdata,cst_userdata)
CST_VAL_REG_TD_FUNCPTR(itemfunc,cst_itemfunc,43)
CST_VAL_REG_TD_TYPE(features,cst_features,45)
CST_VAL_REG_TD_FUNCPTR(breakfunc,cst_breakfunc,47)
CST_VAL_REG_TD_TYPE_NODEL(cg_db,cst_cg_db,49)
CST_VAL_REG_TD_TYPE(voice,cst_voice,51)
CST_VAL_REG_TD_TYPE(audio_streaming_info,cst_audio_streaming_info,53)

const cst_val_def cst_val_defs[] = {
    /* These ones are never called */
    { "int"  , NULL },                      /* 1 INT */
    { "float", NULL },                      /* 3 FLOAT */
    { "string", NULL },                     /* 5 STRING */
    /* These are indexed (type/2) at print and delete time */
    { "utterance", val_delete_utterance }, /*  7 utterance */
    { "wave", val_delete_wave },           /*  9 wave */
    { "track", val_delete_track },         /* 11 track */
    { "lpcres", val_delete_lpcres },       /* 13 lpcres */
    { "uttfunc", val_delete_uttfunc },     /* 15 uttfunc */
    { "ffunc", val_delete_ffunc },         /* 17 ffunc */
    { "relation", val_delete_relation },   /* 19 relation */
    { "item", val_delete_item },           /* 21 item */
    { "cart", val_delete_cart },           /* 23 cart */
    { "phoneset", val_delete_phoneset },   /* 25 phoneset */
    { "lexicon", val_delete_lexicon },     /* 27 lexicon */
    { "dur_stats", val_delete_dur_stats }, /* 29 dur_stats */
    { "diphone_db", val_delete_diphone_db }, /* 31 diphone_db */
    { "clunit_db", val_delete_clunit_db }, /* 33 clunit_db */
    { "vit_cand", val_delete_vit_cand },   /* 35 vit_cand */
    { "sts_list", val_delete_sts_list },   /* 37 sts_list */
    { NULL, NULL }, /* 39 (reserved) */
    { "userdata", val_delete_userdata },   /* 41 userdata */
    { "itemfunc", val_delete_itemfunc },   /* 43 itemfunc */
    { "features", val_delete_features },   /* 45 itemfunc */
    { "breakfunc", val_delete_breakfunc }, /* 47 breakfunc */
    { "cg_db", val_delete_cg_db },         /* 49 cg_db */
    { "voice", val_delete_voice },         /* 51 cst_voice */
    { "audio_streaming_info", val_delete_audio_streaming_info }, /* 53 asi */
    { NULL, NULL } /* NULLs at end of list */
};
