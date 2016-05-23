/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                         Copyright (c) 2001                            */
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
/*               Date:  January 2001                                     */
/*************************************************************************/
/*                                                                       */
/*  For text to phones                                                   */
/*                                                                       */
/*************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "cst_args.h"
#include "flite.h"

#include "../lang/usenglish/usenglish.h"
#include "../lang/cmulex/cmu_lex.h"

static cst_voice *cmu_us_no_wave = NULL;

static cst_utterance *no_wave_synth(cst_utterance *u)
{
    return u;
}

static cst_voice *register_cmu_us_no_wave(const char *voxdir)
{
    cst_voice *v = new_voice();
    cst_lexicon *lex;

    v->name = "no_wave_voice";

    /* Set up basic values for synthesizing with this voice */
    usenglish_init(v);
    feat_set_string(v->features,"name","cmu_us_no_wave");

    /* Lexicon */
    lex = cmu_lex_init();
    feat_set(v->features,"lexicon",lexicon_val(lex));

    /* Intonation */
    feat_set_float(v->features,"int_f0_target_mean",95.0);
    feat_set_float(v->features,"int_f0_target_stddev",11.0);

    feat_set_float(v->features,"duration_stretch",1.1); 

    /* Post lexical rules */
    feat_set(v->features,"postlex_func",uttfunc_val(lex->postlex));

    /* Waveform synthesis: diphone_synth */
    feat_set(v->features,"wave_synth_func",uttfunc_val(&no_wave_synth));

    cmu_us_no_wave = v;

    return cmu_us_no_wave;
}

int main(int argc, char **argv)
{
    cst_val *files;
    cst_features *args=new_features();
    cst_voice *v;
    cst_utterance *u;
    cst_item *s;
    const char *text, *name;

    files =
        cst_args(argv,argc,
                 "usage: t2p \"word word word\"\n"
                 "Convert text to US English phonemes.",
                 args);

    if (files)
	text = val_string(val_car(files));
    else
    {
	fprintf(stderr,"no text specified\n");
	exit(-1);
    }

    
    flite_init();
    v = register_cmu_us_no_wave(NULL);

    u = flite_synth_text(text,v);

    for (s=relation_head(utt_relation(u,"Segment"));
	 s;
	 s = item_next(s))
    {
	name = item_feat_string(s,"name");
	printf("%s",name);
	/* If its a vowel and is stessed output stress value */
	if ((cst_streq("+",ffeature_string(s,"ph_vc"))) &&
	    (cst_streq("1",ffeature_string(s,"R:SylStructure.parent.stress"))))
	    printf("1");
	printf(" ");
    }

    printf("\n");
    
    return 0;
}
