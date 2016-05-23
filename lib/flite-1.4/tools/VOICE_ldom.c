/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                      Copyright (c) 1999-2000                          */
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
/*               Date:  September 2002                                   */
/*************************************************************************/
/*                                                                       */
/*  A simple ldom voice defintion	    		                 */
/*                                                                       */
/*************************************************************************/

#include <string.h>
#include "flite.h"
#include "cst_clunits.h"
#include "usenglish.h"
#include "cmulex.h"

static char *__VOICENAME___unit_name(cst_item *s);

extern cst_clunit_db __VOICENAME___db;
cst_voice *__VOICENAME___clunits = NULL;

cst_voice *register___VOICENAME__(const char *voxdir)
{
    cst_voice *v = new_voice();

    v->name = "__NICKNAME__";

    /* Sets up language specific parameters in the __VOICENAME__. */
    usenglish_init(v);

    /* Things that weren't filled in already. */
    feat_set_string(v->features,"name","__VOICENAME__");

    /* Lexicon */
    cmu_lex_init();
    feat_set(v->features,"lexicon",lexicon_val(&cmu_lex));

    /* Waveform synthesis */
    feat_set(v->features,"wave_synth_func",uttfunc_val(&clunits_synth));
    feat_set(v->features,"clunit_db",clunit_db_val(&__VOICENAME___db));
    feat_set_int(v->features,"sample_rate",__VOICENAME___db.sts->sample_rate);
    feat_set_string(v->features,"join_type","simple_join");
    feat_set_string(v->features,"resynth_type","fixed");

    /* Unit selection */
    __VOICENAME___db.unit_name_func = __VOICENAME___unit_name;

    __VOICENAME___clunits = v;

    return __VOICENAME___clunits;
}

void unregister___VOICENAME__(cst_voice *vox)
{
    if (vox != __VOICENAME___clunits)
	return;
    delete_voice(vox);
    __VOICENAME___clunits = NULL;
}

static char *__VOICENAME___unit_name(cst_item *s)
{
    return clunits_ldom_phone_word(s);
}
