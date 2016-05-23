/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                       Copyright (c) 2000-2008                         */
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
/*               Date:  September 2000                                   */
/*************************************************************************/
/*                                                                       */
/*  Basic user level functions                                           */
/*                                                                       */
/*************************************************************************/

#include "cst_tokenstream.h"
#include "flite.h"

/* This is a global, which isn't ideal, this may change */
/* It is set when flite_set_voice_list() is called which happens in */
/* flite_main() */
cst_val *flite_voice_list = 0;

int flite_init()
{
    cst_regex_init();

    return 0;
}

cst_voice *flite_voice_select(const char *name)
{
    const cst_val *v;
    cst_voice *voice;

    if (flite_voice_list == NULL)
        return NULL;  /* oops, not good */
    if (name == NULL)
        return val_voice(val_car(flite_voice_list));

    for (v=flite_voice_list; v; v=val_cdr(v))
    {
        voice = val_voice(val_car(v));
        if (cst_streq(name,voice->name))
            return voice;
    }

    return flite_voice_select(NULL);

}

int flite_voice_add_lex_addenda(cst_voice *v, const cst_string *lexfile)
{
    /* Add addenda in lexfile to current voice */
    cst_lexicon *lex;
    const cst_val *lex_addenda = NULL;
    cst_val *new_addenda;

    lex = val_lexicon(feat_val(v->features,"lexicon"));
    if (feat_present(v->features, "lex_addenda"))
	lex_addenda = feat_val(v->features, "lex_addenda");

    new_addenda = cst_lex_load_addenda(lex,lexfile);
#if 0
    printf("\naddenda: ");
    val_print(stdout,new_addenda);
    printf("\n");
#endif

    new_addenda = val_append(new_addenda,(cst_val *)lex_addenda);
    if (lex->lex_addenda)
        delete_val(lex->lex_addenda);
    lex->lex_addenda = new_addenda;

    return 0;
}

cst_utterance *flite_do_synth(cst_utterance *u,
                                     cst_voice *voice,
                                     cst_uttfunc synth)
{		       
    utt_init(u, voice);
    if ((*synth)(u) == NULL)
    {
	delete_utterance(u);
	return NULL;
    }
    else
	return u;
}

cst_utterance *flite_synth_text(const char *text, cst_voice *voice)
{
    cst_utterance *u;

    u = new_utterance();
    utt_set_input_text(u,text);
    return flite_do_synth(u, voice, utt_synth);
}

cst_utterance *flite_synth_phones(const char *text, cst_voice *voice)
{
    cst_utterance *u;

    u = new_utterance();
    utt_set_input_text(u,text);
    return flite_do_synth(u, voice, utt_synth_phones);
}

cst_wave *flite_text_to_wave(const char *text, cst_voice *voice)
{
    cst_utterance *u;
    cst_wave *w;

    if ((u = flite_synth_text(text,voice)) == NULL)
	return NULL;

    w = copy_wave(utt_wave(u));
    delete_utterance(u);
    return w;
}

float flite_file_to_speech(const char *filename, 
			   cst_voice *voice,
			   const char *outtype)
{
    cst_utterance *utt;
    cst_tokenstream *ts;
    const char *token;
    cst_item *t;
    cst_relation *tokrel;
    float durs = 0;
    int num_tokens;
    cst_wave *w;
    cst_breakfunc breakfunc = default_utt_break;
    cst_uttfunc utt_user_callback = 0;
    int fp;

    if ((ts = ts_open(filename,
	      get_param_string(voice->features,"text_whitespace",NULL),
	      get_param_string(voice->features,"text_singlecharsymbols",NULL),
	      get_param_string(voice->features,"text_prepunctuation",NULL),
	      get_param_string(voice->features,"text_postpunctuation",NULL)))
	== NULL)
    {
	cst_errmsg("failed to open file \"%s\" for reading\n",
		   filename);
	return 1;
    }
    fp = get_param_int(voice->features,"file_start_position",0);
    if (fp > 0)
        ts_set_stream_pos(ts,fp);

    if (feat_present(voice->features,"utt_break"))
	breakfunc = val_breakfunc(feat_val(voice->features,"utt_break"));

    if (feat_present(voice->features,"utt_user_callback"))
	utt_user_callback = val_uttfunc(feat_val(voice->features,"utt_user_callback"));

    /* If its a file to write to, create and save an empty wave file */
    /* as we are going to incrementally append to it                 */
    if (!cst_streq(outtype,"play") && 
        !cst_streq(outtype,"none") &&
        !cst_streq(outtype,"stream"))
    {
	w = new_wave();
	cst_wave_resize(w,0,1);
	cst_wave_set_sample_rate(w,16000);
	cst_wave_save_riff(w,outtype);  /* an empty wave */
	delete_wave(w);
    }

    num_tokens = 0;
    utt = new_utterance();
    tokrel = utt_relation_create(utt, "Token");
    while (!ts_eof(ts) || num_tokens > 0)
    {
	token = ts_get(ts);
	if ((cst_strlen(token) == 0) ||
	    (num_tokens > 500) ||  /* need an upper bound */
	    (relation_head(tokrel) && 
	     breakfunc(ts,token,tokrel)))
	{
	    /* An end of utt, so synthesize it */
            if (utt_user_callback)
                utt = (utt_user_callback)(utt);

            if (utt)
            {
                utt = flite_do_synth(utt,voice,utt_synth_tokens);
                durs += flite_process_output(utt,outtype,TRUE);
                delete_utterance(utt); utt = NULL;
            }
            else 
                break;

	    if (ts_eof(ts)) break;

	    utt = new_utterance();
	    tokrel = utt_relation_create(utt, "Token");
	    num_tokens = 0;
	}
	num_tokens++;

	t = relation_append(tokrel, NULL);
	item_set_string(t,"name",token);
	item_set_string(t,"whitespace",ts->whitespace);
	item_set_string(t,"prepunctuation",ts->prepunctuation);
	item_set_string(t,"punc",ts->postpunctuation);
        /* Mark it at the beginning of the token */
	item_set_int(t,"file_pos",
                     ts->file_pos-(1+ /* as we are already on the next char */
                                   cst_strlen(token)+
                                   cst_strlen(ts->prepunctuation)+
                                   cst_strlen(ts->postpunctuation)));
	item_set_int(t,"line_number",ts->line_number);
    }

    delete_utterance(utt);
    ts_close(ts);
    return durs;
}

float flite_text_to_speech(const char *text,
			   cst_voice *voice,
			   const char *outtype)
{
    cst_utterance *u;
    float dur;

    u = flite_synth_text(text,voice);
    dur = flite_process_output(u,outtype,FALSE);
    delete_utterance(u);

    return dur;
}

float flite_phones_to_speech(const char *text,
			     cst_voice *voice,
			     const char *outtype)
{
    cst_utterance *u;
    float dur;

    u = flite_synth_phones(text,voice);
    dur = flite_process_output(u,outtype,FALSE);
    delete_utterance(u);

    return dur;
}

float flite_process_output(cst_utterance *u, const char *outtype,
                           int append)
{
    /* Play or save (append) output to output file */
    cst_wave *w;
    float dur;

    if (!u) return 0.0;

    w = utt_wave(u);

    dur = (float)w->num_samples/(float)w->sample_rate;
	     
    if (cst_streq(outtype,"play"))
	play_wave(w);
    else if (cst_streq(outtype,"stream"))
    {
        /* It's already been played so do nothing */
        
    }
    else if (!cst_streq(outtype,"none"))
    {
        if (append)
            cst_wave_append_riff(w,outtype);
        else
            cst_wave_save_riff(w,outtype);
    }

    return dur;
}

int flite_get_param_int(const cst_features *f, const char *name,int def)
{
    return get_param_int(f,name,def);
}
float flite_get_param_float(const cst_features *f, const char *name, float def)
{
    return get_param_float(f,name,def);
}
const char *flite_get_param_string(const cst_features *f, const char *name, const char *def)
{
    return get_param_string(f,name,def);
}
const cst_val *flite_get_param_val(const cst_features *f, const char *name, cst_val *def)
{
    return get_param_val(f,name,def);
}

void flite_feat_set_int(cst_features *f, const char *name, int v)
{
    feat_set_int(f,name,v);
}
void flite_feat_set_float(cst_features *f, const char *name, float v)
{
    feat_set_float(f,name,v);
}
void flite_feat_set_string(cst_features *f, const char *name, const char *v)
{
    feat_set_string(f,name,v);
}
void flite_feat_set(cst_features *f, const char *name,const cst_val *v)
{
    feat_set(f,name,v);
}

int flite_feat_remove(cst_features *f, const char *name)
{
	return feat_remove(f,name);
}

const char *flite_ffeature_string(const cst_item *item,const char *featpath)
{
    return ffeature_string(item,featpath);
}

int flite_ffeature_int(const cst_item *item,const char *featpath)
{
    return ffeature_int(item,featpath);
}
float flite_ffeature_float(const cst_item *item,const char *featpath)
{
    return ffeature_float(item,featpath);
}
const cst_val *flite_ffeature(const cst_item *item,const char *featpath)
{
    return ffeature(item,featpath);
}
cst_item* flite_path_to_item(const cst_item *item,const char *featpath)
{
    return path_to_item(item,featpath);
}

