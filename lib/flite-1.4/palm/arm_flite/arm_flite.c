/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                         Copyright (c) 2004                            */
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
/*               Date:  December 2004                                    */
/*************************************************************************/
/*                                                                       */
/*  get it to use some flite code in the FLOP app                        */
/*                                                                       */
/*************************************************************************/

#include <Standalone.h>
#include <flite.h>
#include "palm_flite.h"

STANDALONE_CODE_RESOURCE_ID(1000);

cst_voice *register_cmu_us_kal(const char *voxdir);

/* Some variables that will be filled in from resources */
extern cst_lexicon cmu_lex;
extern cst_lts_rules cmu_lts_rules;
extern cst_sts_list cmu_us_kal_sts;

#define FLITE_STRING_LENGTH 600

/* Debug error string */
char awb_output[FLITE_STRING_LENGTH] = "no synthesis result";
/* General output string */
char output[FLITE_STRING_LENGTH] = "no synthesis result";

static cst_voice *flite_voice = 0;

static int flop_output(cst_utterance *u,
		       const char *relname,
		       char *output,
		       int max_output)
{
    cst_item *item;
    int count;
    const char * name;
    int ccount = 0, nccount;

    for (count=0,item=relation_head(utt_relation(u,relname)); 
	 item; 
	 item=item_next(item),count++)
    {
	name = item_feat_string(item,"name");
	nccount = ccount + cst_strlen(name) + 1;
	if ((nccount + 1) >= max_output)
	    return count;
	cst_sprintf(output+ccount,"%s ",name);
	ccount = nccount;
    }

    return count;
}

static cst_utterance *find_first_utt(cst_voice *voice, flite_info *fi)
{   /* Find the first utt from fi->start */
    cst_utterance *utt;
    cst_utterance *rutt = NULL;
    cst_tokenstream *ts;
    const char *token;
    cst_item *t;
    cst_relation *tokrel;
    int num_tokens;
    cst_breakfunc breakfunc = default_utt_break;

    if (fi->start >= cst_strlen(fi->text))
	return NULL;
    ts = ts_open_string(&fi->text[fi->start],
	      get_param_string(voice->features,"text_whitespace",NULL),
	      get_param_string(voice->features,"text_singlecharsymbols",NULL),
	      get_param_string(voice->features,"text_prepunctuation",NULL),
   	      get_param_string(voice->features,"text_postpunctuation",NULL));

    if (feat_present(voice->features,"utt_break"))
	breakfunc = val_breakfunc(feat_val(voice->features,"utt_break"));

    num_tokens = 0;
    utt = new_utterance();
    tokrel = utt_relation_create(utt, "Token");
    while (((!ts_eof(ts) || num_tokens > 0)) && ! rutt)
    {
	fi->utt_length = ts->file_pos;
	token = ts_get(ts);

	if ((cst_strlen(token) == 0) ||
	    (num_tokens > 500) ||  /* need an upper bound */
	    (relation_head(tokrel) && 
	     breakfunc(ts,token,tokrel)))
	{
	    rutt = utt;
	}
	else
	{
	    num_tokens++;
	    t = relation_append(tokrel, NULL);
	    item_set_string(t,"name",token);
	    item_set_string(t,"whitespace",ts->whitespace);
	    item_set_string(t,"prepunctuation",ts->prepunctuation);
	    item_set_string(t,"punc",ts->postpunctuation);
	    item_set_int(t,"file_pos",ts->file_pos);
	    item_set_int(t,"line_number",ts->line_number);
	}
    }

    ts_close(ts);
    return utt;
}

static int flite_text_to_text(cst_voice *voice, flite_info *fi)
{   /* return list of words, phones or waveform for given text */
    int count=0;
    cst_utterance *u;
    cst_wave *w;

    u = find_first_utt(voice,fi);
    if (u == NULL)
    {
	cst_sprintf(fi->output,"nothing to synthesize");
	fi->utt_length = 0;
	fi->num_samples = 0;
	fi->samples = 0;
	return 1;  /* nothing to synthesize */
    }

    utt_init(u,voice);
    u = utt_synth_tokens(u);  /* we synth to wave even if not needed */

    if (fi->type == FliteOutputTypePhones)
	count = flop_output(u,"Segment",fi->output,fi->max_output);
    else if (fi->type == FliteOutputTypeWords)
	count = flop_output(u,"Word",fi->output,fi->max_output);
    else if (fi->type == FliteOutputTypeWave)
    {
	w = utt_wave(u);
	fi->num_samples = w->num_samples;
	fi->sample_rate = w->sample_rate;
	fi->samples = w->samples;
	w->samples = 0;       /* set this to null so we don't free it */
	w->num_samples = 0;   /* before we use it back in 68k land    */
	cst_sprintf(fi->output,"playing %d samples from %d for %d",
		    fi->num_samples,
		    fi->start,
		    fi->utt_length);
    }
    else
	cst_sprintf(fi->output,"unknown synthesis option");

    delete_utterance(u);

    return count;
}

int arm_flite_synth_text(flite_info *fi)
{
    /* The main entry point for flite from the m68k world        */
    /* All information is stored within the flite_info structure */
    /* that is already swapped to ARM byte order                 */
    /* Generated information is passed back through fi too       */
    int c=0;

    if (flite_voice == 0)
    {
	/* Here's a secret, flite_init() doesn't actually do anything */
	/* and when linked in, pulls in audio and file i/o in flite.o */
	/* which isn't actually needed by the arm code */

	/* flite_init(); */ /* technically should call it, but I don't */

	/* Ideally we should find voice name from fi and load it */
	flite_voice = register_cmu_us_kal(NULL);
	/* Set up the big data segments */

	cmu_lts_rules.models =     fi->segs[FLITE_CLTS]->arm_mem;
	cmu_lex.data =             fi->segs[FLITE_CLEX]->arm_mem;
	cmu_us_kal_sts.frames =
	         (unsigned short *)fi->segs[FLITE_CLPC]->arm_mem;
	cmu_us_kal_sts.residuals = fi->segs[FLITE_CRES]->arm_mem;
	cmu_us_kal_sts.resoffs = 
	           (unsigned int *)fi->segs[FLITE_CRSI]->arm_mem;

	cst_errjmp = cst_alloc(jmp_buf,1);
    }

    if (setjmp(*cst_errjmp))
    {   /* got thrown an error */
	cst_sprintf(fi->output,"%s",cst_error_msg);
	c = 1;
    }
    else
    {
	c = flite_text_to_text(flite_voice,fi);
    }
	
    return c;

}
