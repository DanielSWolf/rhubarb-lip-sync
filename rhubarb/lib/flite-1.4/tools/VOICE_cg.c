/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                      Copyright (c) 1999-2007                          */
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
/*               Date:  November 2007                                    */
/*************************************************************************/
/*                                                                       */
/*  A clustergen voice defintion			                 */
/*                                                                       */
/*************************************************************************/

#include <string.h>
#include "flite.h"
#include "cst_cg.h"

#include "__FLITELANG__.h"
#include "__FLITELEX__.h"

extern cst_cg_db __VOICENAME___cg_db;
cst_voice *__VOICENAME___cg = NULL;

cst_voice *register___VOICENAME__(const char *voxdir)
{
    cst_voice *vox;
    cst_lexicon *lex;

    if (__VOICENAME___cg)
        return __VOICENAME___cg;  /* Already registered */

    vox = new_voice();
    vox->name = "__NICKNAME__";

    /* Sets up language specific parameters in the __VOICENAME__. */
    __FLITELANG___init(vox);

    /* Things that weren't filled in already. */
    flite_feat_set_string(vox->features,"name","__VOICENAME__");

    /* Lexicon */
    lex = __FLITELEX___init();
    flite_feat_set(vox->features,"lexicon",lexicon_val(lex));
    flite_feat_set(vox->features,"postlex_func",uttfunc_val(lex->postlex));

    /* No standard segment durations are needed as its done at the */
    /* HMM state level */
    flite_feat_set_string(vox->features,"no_segment_duration_model","1");
    flite_feat_set_string(vox->features,"no_f0_target_model","1");

    /* Waveform synthesis */
    flite_feat_set(vox->features,"wave_synth_func",uttfunc_val(&cg_synth));
    flite_feat_set(vox->features,"cg_db",cg_db_val(&__VOICENAME___cg_db));
    flite_feat_set_int(vox->features,"sample_rate",__VOICENAME___cg_db.sample_rate);

    __VOICENAME___cg = vox;

    return __VOICENAME___cg;
}

void unregister___VOICENAME__(cst_voice *vox)
{
    if (vox != __VOICENAME___cg)
	return;
    delete_voice(vox);
    __VOICENAME___cg = NULL;
}

