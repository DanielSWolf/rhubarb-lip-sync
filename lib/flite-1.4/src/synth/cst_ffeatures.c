/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                         Copyright (c) 2007                            */
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
/*  Some language independent features                                   */
/*                                                                       */

#include "cst_hrg.h"
#include "cst_phoneset.h"
#include "cst_regex.h"

static const cst_val *word_break(const cst_item *word);
static const cst_val *word_punc(const cst_item *word);
static const cst_val *word_numsyls(const cst_item *word);
static const cst_val *ssyl_in(const cst_item *syl);
static const cst_val *syl_in(const cst_item *syl);
static const cst_val *syl_out(const cst_item *syl);
static const cst_val *syl_break(const cst_item *syl);
static const cst_val *syl_codasize(const cst_item *syl);
static const cst_val *syl_onsetsize(const cst_item *syl);
static const cst_val *accented(const cst_item *p);

DEF_STATIC_CONST_VAL_STRING(val_string_onset,"onset");
DEF_STATIC_CONST_VAL_STRING(val_string_coda,"coda");
DEF_STATIC_CONST_VAL_STRING(val_string_initial,"initial");
DEF_STATIC_CONST_VAL_STRING(val_string_single,"single");
DEF_STATIC_CONST_VAL_STRING(val_string_final,"final");
DEF_STATIC_CONST_VAL_STRING(val_string_mid,"mid");
DEF_STATIC_CONST_VAL_STRING(val_string_empty,"");

const cst_val *ph_vc(const cst_item *p)
{
    return phone_feature(item_phoneset(p),item_name(p),"vc");
}
const cst_val *ph_vlng(const cst_item *p)
{
    return phone_feature(item_phoneset(p),item_name(p),"vlng");
}
const cst_val *ph_vheight(const cst_item *p)
{
   return phone_feature(item_phoneset(p),item_name(p),"vheight");
}
const cst_val *ph_vrnd(const cst_item *p)
{
    return phone_feature(item_phoneset(p),item_name(p),"vrnd");
}
const cst_val *ph_vfront(const cst_item *p)
{
    return phone_feature(item_phoneset(p),item_name(p),"vfront");
}
const cst_val *ph_ctype(const cst_item *p)
{
    return phone_feature(item_phoneset(p),item_name(p),"ctype");
}
const cst_val *ph_cplace(const cst_item *p)
{
    return phone_feature(item_phoneset(p),item_name(p),"cplace");
}
const cst_val *ph_cvox(const cst_item *p)
{
    return phone_feature(item_phoneset(p),item_name(p),"cvox");
}

const cst_val *cg_duration(const cst_item *p)
{
    /* Note this constructs float vals, these will be freed when the */
    /* cart cache is freed, so this should only be used in carts     */
    if (!p)
        return float_val(0.0);
    else if (!item_prev(p))
        return item_feat(p,"end");
    else
        return float_val(item_feat_float(p,"end")
			 - item_feat_float(item_prev(p),"end"));
}

DEF_STATIC_CONST_VAL_STRING(val_string_pos_b,"b");
DEF_STATIC_CONST_VAL_STRING(val_string_pos_m,"m");
DEF_STATIC_CONST_VAL_STRING(val_string_pos_e,"e");

const cst_val *cg_state_pos(const cst_item *p)
{
    const char *name;
    name = item_feat_string(p,"name");
    if (!cst_streq(name,ffeature_string(p,"p.name")))
        return (cst_val *)&val_string_pos_b;
    if (cst_streq(name,ffeature_string(p,"n.name")))
        return (cst_val *)&val_string_pos_m;
    else
        return (cst_val *)&val_string_pos_e;
}

const cst_val *cg_state_place(const cst_item *p)
{
    float start, end;
    int this;
    start = (float)ffeature_int(p,"R:mcep_link.parent.daughter1.frame_number");
    end = (float)ffeature_int(p,"R:mcep_link.parent.daughtern.frame_number");
    this = item_feat_int(p,"frame_number");
    if ((end-start) == 0.0)
        return float_val(0.0);
    else
        return float_val((this-start)/(end-start));
}

const cst_val *cg_state_index(const cst_item *p)
{
    float start;
    int this;
    start = (float)ffeature_int(p,"R:mcep_link.parent.daughter1.frame_number");
    this = item_feat_int(p,"frame_number");
    return float_val(this-start);
}

const cst_val *cg_state_rindex(const cst_item *p)
{
    float end;
    int this;
    end = (float)ffeature_int(p,"R:mcep_link.parent.daughtern.frame_number");
    this = item_feat_int(p,"frame_number");
    return float_val(end-this);
}

const cst_val *cg_phone_place(const cst_item *p)
{
    float start, end;
    int this;
    start = (float)ffeature_int(p,"R:mcep_link.parent.R:segstate.parent.daughter1.R:mcep_link.daughter1.frame_number");
    end = (float)ffeature_int(p,"R:mcep_link.parent.R:segstate.parent.daughtern.R:mcep_link.daughtern.frame_number");
    this = item_feat_int(p,"frame_number");
    if ((end-start) == 0.0)
        return float_val(0.0);
    else
        return float_val((this-start)/(end-start));
}

const cst_val *cg_phone_index(const cst_item *p)
{
    float start;
    int this;
    start = (float)ffeature_int(p,"R:mcep_link.parent.R:segstate.parent.daughter1.R:mcep_link.daughter1.frame_number");
    this = item_feat_int(p,"frame_number");
    return float_val(this-start);
}

const cst_val *cg_phone_rindex(const cst_item *p)
{
    float end;
    int this;
    end = (float)ffeature_int(p,"R:mcep_link.parent.R:segstate.parent.daughtern.R:mcep_link.daughtern.frame_number");
    this = item_feat_int(p,"frame_number");
    return float_val(end-this);
}

const cst_val *cg_is_pau(const cst_item *p)
{
    if (p && cst_streq("pau",item_feat_string(p,"name")))
        return &val_int_1;
    else
        return &val_int_0;
}

const cst_val *cg_find_phrase_number(const cst_item *p)
{
    const cst_item *v;
    int x = 0;

    for (v=item_prev(p); v; v=item_prev(v))
        x++;

    return val_int_n(x);
}

const cst_val *cg_position_in_phrasep(const cst_item *p)
{
    float pstart, pend, phrasenumber;
    float x;
    #define CG_FRAME_SHIFT 0.005 

    pstart = ffeature_float(p,"R:mcep_link.parent.R:segstate.parent.R:SylStructure.parent.parent.R:Phrase.parent.daughter1.R:SylStructure.daughter1.daughter1.R:Segment.p.end");
    pend = ffeature_float(p,"R:mcep_link.parent.R:segstate.parent.R:SylStructure.parent.parent.R:Phrase.parent.daughtern.R:SylStructure.daughtern.daughtern.R:Segment.end");
    phrasenumber = ffeature_float(p,"R:mcep_link.parent.R:segstate.parent.R:SylStructure.parent.parent.R:Phrase.parent.lisp_cg_find_phrase_number");
    if ((pend - pstart) == 0.0)
        return float_val(-1.0);
    else
    {
        x = phrasenumber +
            ((CG_FRAME_SHIFT*item_feat_float(p,"frame_number"))-pstart) /
            (pend - pstart);
        return float_val(x);
    }
}

static const cst_val *accented(const cst_item *syl)
{
    if ((item_feat_present(syl,"accent")) ||
	(item_feat_present(syl,"endtone")))
	return VAL_STRING_1;
    else
	return VAL_STRING_0;
}

static const cst_val *seg_coda_ctype(const cst_item *seg, const char *ctype)
{
    const cst_item *s;
    const cst_phoneset *ps = item_phoneset(seg);
    
    for (s=item_last_daughter(item_parent(item_as(seg,"SylStructure")));
	 s;
	 s=item_prev(s))
    {
	if (cst_streq("+",phone_feature_string(ps,item_feat_string(s,"name"),
					       "vc")))
	    return VAL_STRING_0;
	if (cst_streq(ctype,phone_feature_string(ps,item_feat_string(s,"name"),
					       "ctype")))
	    return VAL_STRING_1;
    }

    return VAL_STRING_0;
}

static const cst_val *seg_onset_ctype(const cst_item *seg, const char *ctype)
{
    const cst_item *s;
    const cst_phoneset *ps = item_phoneset(seg);
    
    for (s=item_daughter(item_parent(item_as(seg,"SylStructure")));
	 s;
	 s=item_next(s))
    {
	if (cst_streq("+",phone_feature_string(ps,item_feat_string(s,"name"),
					       "vc")))
	    return VAL_STRING_0;
	if (cst_streq(ctype,phone_feature_string(ps,item_feat_string(s,"name"),
						 "ctype")))
	    return VAL_STRING_1;
    }

    return VAL_STRING_0;
}

static const cst_val *seg_coda_fric(const cst_item *seg)
{
    return seg_coda_ctype(seg,"f");
}

static const cst_val *seg_onset_fric(const cst_item *seg)
{
    return seg_onset_ctype(seg,"f");
}

static const cst_val *seg_coda_stop(const cst_item *seg)
{
    return seg_coda_ctype(seg,"s");
}

static const cst_val *seg_onset_stop(const cst_item *seg)
{
    return seg_onset_ctype(seg,"s");
}

static const cst_val *seg_coda_nasal(const cst_item *seg)
{
    return seg_coda_ctype(seg,"n");
}

static const cst_val *seg_onset_nasal(const cst_item *seg)
{
    return seg_onset_ctype(seg,"n");
}

static const cst_val *seg_coda_glide(const cst_item *seg)
{
    if (seg_coda_ctype(seg,"r") == VAL_STRING_0)
	    return seg_coda_ctype(seg,"l");
    return VAL_STRING_1;
}

static const cst_val *seg_onset_glide(const cst_item *seg)
{
    if (seg_onset_ctype(seg,"r") == VAL_STRING_0)
	    return seg_onset_ctype(seg,"l");
    return VAL_STRING_1;
}

static const cst_val *seg_onsetcoda(const cst_item *seg)
{
    const cst_item *s;
    const cst_phoneset *ps = item_phoneset(seg);

    if (!seg) return VAL_STRING_0;
    for (s=item_next(item_as(seg,"SylStructure"));
	 s;
	 s=item_next(s))
    {
	if (cst_streq("+",phone_feature_string(ps,item_feat_string(s,"name"),
					       "vc")))
	    return (cst_val *)&val_string_onset;
    }
    return (cst_val *)&val_string_coda;
}

static const cst_val *pos_in_syl(const cst_item *seg)
{
    const cst_item *s;
    int c;
    
    for (c=-1,s=item_as(seg,"SylStructure");
	 s;
	 s=item_prev(s),c++);

    return val_string_n(c);
}

static const cst_val *position_type(const cst_item *syl)
{
    const cst_item *s = item_as(syl,"SylStructure");

    if (s == 0)
	return (cst_val *)&val_string_single;
    else if (item_next(s) == 0)
    {
	if (item_prev(s) == 0)
	    return (cst_val *)&val_string_single;
	else
	    return (cst_val *)&val_string_final;
    }
    else if (item_prev(s) == 0)
	return (cst_val *)&val_string_initial;
    else
	return (cst_val *)&val_string_mid;
}

static const cst_val *sub_phrases(const cst_item *syl)
{
    const cst_item *s;
    int c;
    
    for (c=0,s=path_to_item(syl,"R:SylStructure.parent.R:Phrase.parent.p");
	 s && (c < CST_CONST_INT_MAX); 
	 s=item_prev(s),c++);

    return val_string_n(c);
}

static const cst_val *last_accent(const cst_item *syl)
{
    const cst_item *s;
    int c;
    
    for (c=0,s=item_as(syl,"Syllable");
	 s && (c < CST_CONST_INT_MAX); 
	 s=item_prev(s),c++)
    {
	if (val_int(accented(s)))
	    return val_string_n(c);
    }

    return val_string_n(c);
}

static const cst_val *next_accent(const cst_item *syl)
{
    const cst_item *s;
    int c;

    s=item_as(syl,"Syllable");
    if (s)
        s = item_next(s);
    else
        return val_string_n(0);
    for (c=0;
	 s && (c < CST_CONST_INT_MAX); 
	 s=item_next(s),c++)
    {
	if (val_int(accented(s)))
	    return val_string_n(c);
    }

    return val_string_n(c);
}

static const cst_val *syl_final(const cst_item *seg)
{   /* last segment in a syllable */
    const cst_item *s = item_as(seg,"SylStructure");

    if (!s || (item_next(s) == NULL))
	return VAL_STRING_1;
    else
	return VAL_STRING_0;
}

static const cst_val *word_punc(const cst_item *word)
{
    cst_item *ww;
    const cst_val *v;

    ww = item_as(word,"Token");

    if ((ww != NULL) && (item_next(ww) != 0))
	v = &val_string_empty;
    else
	v = ffeature(item_parent(ww),"punc");

/*    printf("word_punc word %s punc %s\n",
	   item_feat_string(ww,"name"),
	   val_string(v)); */

    return v;

}

static const cst_val *word_break(const cst_item *word)
{
    cst_item *ww,*pp;
    const char *pname;

    ww = item_as(word,"Phrase");

    if ((ww == NULL) || (item_next(ww) != 0))
	return VAL_STRING_1;
    else
    {
	pp = item_parent(ww);
	pname = val_string(item_feat(pp,"name"));
	if (cst_streq("BB",pname))
	    return VAL_STRING_4;
	else if (cst_streq("B",pname))
	    return VAL_STRING_3;
	else 
	    return VAL_STRING_1;
    }
}

static const cst_val *word_numsyls(const cst_item *word)
{
    cst_item *d;
    int c;
    
    for (c=0,d=item_daughter(item_as(word,"SylStructure"));
	 d;
	 d=item_next(d),c++);

    return val_int_n(c);
}

static const cst_val *ssyl_in(const cst_item *syl)
{
    /* Number of stressed syllables since last major break */
    const cst_item *ss,*p,*fs;
    int c;

    ss = item_as(syl,"Syllable");

    fs = path_to_item(syl,"R:SylStructure.parent.R:Phrase.parent.daughter.R:SylStructure.daughter");

    /* This should actually include the first syllable, but Festival's
       doesn't. */
    for (c=0, p=item_prev(ss); 
	 p && (!item_equal(p,fs)) && (c < CST_CONST_INT_MAX);
	 p=item_prev(p))
    {
	if (cst_streq("1",item_feat_string(p,"stress")))
	    c++;
    }
    
    return val_string_n(c);  /* its used randomly as int and float */
}

static const cst_val *ssyl_out(const cst_item *syl)
{
    /* Number of stressed syllables until last major break */
    const cst_item *ss,*p,*fs;
    int c;

    ss = item_as(syl,"Syllable");

    fs = path_to_item(syl,"R:SylStructure.parent.R:Phrase.parent.daughtern.R:SylStructure.daughtern");

    for (c=0, p=item_next(ss); 
	 p && (c < CST_CONST_INT_MAX); 
	 p=item_next(p))
    {
	if (cst_streq("1",item_feat_string(p,"stress")))
	    c++;
	if (item_equal(p,fs))
	    break;
    }
    
    return val_string_n(c);  /* its used randomly as int and float */
}

static const cst_val *syl_in(const cst_item *syl)
{
    /* Number of syllables since last major break */
    const cst_item *ss,*p,*fs;
    int c;

    ss = item_as(syl,"Syllable");

    fs = path_to_item(syl,"R:SylStructure.parent.R:Phrase.parent.daughter.R:SylStructure.daughter");

    for (c=0, p=ss; 
	 p && (c < CST_CONST_INT_MAX); 
	 p=item_prev(p),c++)
	if (item_equal(p,fs))
	    break;
    return val_string_n(c);
}

static const cst_val *syl_out(const cst_item *syl)
{
    /* Number of syllables until next major break */
    cst_item *ss,*p;
    const cst_item *fs;
    int c;

    ss = item_as(syl,"Syllable");

    fs = path_to_item(syl,"R:SylStructure.parent.R:Phrase.parent.daughtern.R:SylStructure.daughtern");

    for (c=0, p=ss; 
	 p && (c < CST_CONST_INT_MAX); 
	 p=item_next(p),c++)
	if (item_equal(p,fs))
	    break;
    return val_string_n(c);
}

static const cst_val *syl_break(const cst_item *syl)
{
    /* Break level after this syllable */
    cst_item *ss;

    ss = item_as(syl,"SylStructure");

    if (ss == NULL)
	return VAL_STRING_1;  /* hmm, no sylstructure */
    else if (item_next(ss) != NULL)
	return VAL_STRING_0;  /* word internal */
    else if (item_parent(ss) == NULL)  /* no parent */
	return VAL_STRING_1;
    else
	return word_break(item_parent(ss));
}

static const cst_val *cg_break(const cst_item *syl)
{
    /* phrase prediction is so different between flite and festival */
    /* we go with this more robust technique */
    cst_item *ss;

    ss = item_as(syl,"SylStructure");

    if (ss == NULL)
	return VAL_INT_0;  /* hmm, no sylstructure */
    else if (item_next(ss) != NULL)
	return VAL_INT_0;  /* word internal */
    else if (path_to_item(ss,"R:SylStructure.parent.R:Word.n") == NULL)
        return VAL_INT_4;  /* utterance final */
    else if (path_to_item(ss,"R:SylStructure.parent.R:Phrase.n") == NULL)
        return VAL_INT_3;  /* phrase final */
    else
	return VAL_INT_1;  /* word final */
}

static const cst_val *syl_codasize(const cst_item *syl)
{
    cst_item *d;
    int c;

    for (c=1,d=item_last_daughter(item_as(syl,"SylStructure"));
	 d;
	 d=item_prev(d),c++)
    {
	if (cst_streq("+",val_string(ph_vc(d))))
	    break;
    }

    return val_string_n(c);
}

static const cst_val *syl_onsetsize(const cst_item *syl)
{
    cst_item *d;
    int c;
    
    for (c=0,d=item_daughter(item_as(syl,"SylStructure"));
	 d;
	 d=item_next(d),c++)
    {
	if (cst_streq("+",val_string(ph_vc(d))))
	    break;
    }

    return val_string_n(c);
}

static const cst_val *asyl_in(const cst_item *syl)
{
    /* Number of accented syllables since last major break */
    const cst_item *ss,*p,*fs;
    int c;

    ss = item_as(syl,"Syllable");

    fs = path_to_item(syl,"R:SylStructure.parent.R:Phrase.parent.daughter.R:SylStructure.daughter");

    for (c=0, p=ss; 
	 p && (c < CST_CONST_INT_MAX); 
	 p=item_prev(p))
    {
	if (val_int(accented(p)) == 1)
	    c++;
	if (item_equal(p,fs))
	    break;
    }
    
    return val_string_n(c);
}

static const cst_val *asyl_out(const cst_item *syl)
{
    /* Number of accented syllables until next major break */
    cst_item *ss,*p;
    const cst_item *fs;
    int c;

    ss = item_as(syl,"Syllable");

    fs = path_to_item(syl,"R:SylStructure.parent.R:Phrase.parent.daughtern.R:SylStructure.daughtern");

    for (c=0, p=ss; 
	 p && (c < CST_CONST_INT_MAX); 
	 p=item_next(p))
    {
	if (val_int(accented(p)) == 1)
	    c++;
	if (item_equal(p,fs))
	    break;
    }
    return val_string_n(c);
}

static const cst_val *segment_duration(const cst_item *seg)
{
    const cst_item *s = item_as(seg,"Segment");

    if (!s)
	return VAL_STRING_0;
    else if (item_prev(s) == NULL)
	return item_feat(s,"end");
    else
	/* It should be okay to construct this as it will get
           dereferenced when the CART interpreter frees its feature
           cache. */
	return float_val(item_feat_float(s,"end")
			 - item_feat_float(item_prev(s),"end"));
}


void basic_ff_register(cst_features *ffunctions)
{
    ff_register(ffunctions, "ph_vc",ph_vc);
    ff_register(ffunctions, "ph_vlng",ph_vlng);
    ff_register(ffunctions, "ph_vheight",ph_vheight);
    ff_register(ffunctions, "ph_vfront",ph_vfront);
    ff_register(ffunctions, "ph_vrnd",ph_vrnd);
    ff_register(ffunctions, "ph_ctype",ph_ctype);
    ff_register(ffunctions, "ph_cplace",ph_cplace);
    ff_register(ffunctions, "ph_cvox",ph_cvox);

    ff_register(ffunctions, "lisp_cg_duration", cg_duration);
    ff_register(ffunctions, "lisp_cg_state_pos", cg_state_pos);
    ff_register(ffunctions, "lisp_cg_state_place", cg_state_place);
    ff_register(ffunctions, "lisp_cg_state_index", cg_state_index);
    ff_register(ffunctions, "lisp_cg_state_rindex", cg_state_rindex);
    ff_register(ffunctions, "lisp_cg_phone_place", cg_phone_place);
    ff_register(ffunctions, "lisp_cg_phone_index", cg_phone_index);
    ff_register(ffunctions, "lisp_cg_phone_rindex", cg_phone_rindex);

    ff_register(ffunctions, "lisp_cg_position_in_phrasep", cg_position_in_phrasep);
    ff_register(ffunctions, "lisp_cg_find_phrase_number", cg_find_phrase_number);
    ff_register(ffunctions, "lisp_is_pau", cg_is_pau);

    ff_register(ffunctions, "word_numsyls",word_numsyls);
    ff_register(ffunctions, "word_break",word_break);
    ff_register(ffunctions, "word_punc",word_punc);
    ff_register(ffunctions, "ssyl_in",ssyl_in);
    ff_register(ffunctions, "ssyl_out",ssyl_out);
    ff_register(ffunctions, "syl_in",syl_in);
    ff_register(ffunctions, "syl_out",syl_out);
    ff_register(ffunctions, "syl_break",syl_break);
    ff_register(ffunctions, "lisp_cg_break",cg_break);
    ff_register(ffunctions, "old_syl_break",syl_break);
    ff_register(ffunctions, "syl_onsetsize",syl_onsetsize);
    ff_register(ffunctions, "syl_codasize",syl_codasize);
    ff_register(ffunctions, "accented",accented);
    ff_register(ffunctions, "asyl_in",asyl_in);
    ff_register(ffunctions, "asyl_out",asyl_out);
    ff_register(ffunctions, "lisp_coda_fric",seg_coda_fric);
    ff_register(ffunctions, "lisp_onset_fric",seg_onset_fric);
    ff_register(ffunctions, "lisp_coda_stop",seg_coda_stop);
    ff_register(ffunctions, "lisp_onset_stop",seg_onset_stop);
    ff_register(ffunctions, "lisp_coda_nasal",seg_coda_nasal);
    ff_register(ffunctions, "lisp_onset_nasal",seg_onset_nasal);
    ff_register(ffunctions, "lisp_coda_glide",seg_coda_glide);
    ff_register(ffunctions, "lisp_onset_glide",seg_onset_glide);
    ff_register(ffunctions, "seg_onsetcoda",seg_onsetcoda);
    ff_register(ffunctions, "pos_in_syl",pos_in_syl);
    ff_register(ffunctions, "position_type",position_type);
    ff_register(ffunctions, "sub_phrases",sub_phrases);
    ff_register(ffunctions, "last_accent",last_accent);
    ff_register(ffunctions, "next_accent",next_accent);
    ff_register(ffunctions, "syl_final",syl_final);
    ff_register(ffunctions, "segment_duration",segment_duration);

}
