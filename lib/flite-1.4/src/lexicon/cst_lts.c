/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 1999                             */
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
/*               Date:  December 1999                                    */
/*************************************************************************/
/*                                                                       */
/*  Letter to sound rule support                                         */
/*                                                                       */
/*************************************************************************/

#include "cst_features.h"
#include "cst_lts.h"
#include "cst_endian.h"

static cst_lts_phone apply_model(cst_lts_letter *vals,
				 cst_lts_addr start,
				 const cst_lts_model *model);

cst_lts_rules *new_lts_rules()
{
    cst_lts_rules *lt = cst_alloc(cst_lts_rules,1);
    lt->name = 0;
    lt->letter_index = 0;
    lt->models = 0;
    lt->phone_table = 0;
    lt->context_window_size = 0;
    lt->context_extra_feats = 0;
    lt->letter_table = 0;
    return lt;
}

cst_val *lts_apply_val(const cst_val *wlist,const char *feats,const cst_lts_rules *r)
{
    /* for symbol to symbol mapping */
    const cst_val *v;
    cst_val *p;
    char *word;
    int i,j;

    word = cst_alloc(char,val_length(wlist)+1);

    for (v=wlist,i=0; v; v=val_cdr(v),i++)
    {
	for (j=0; r->letter_table[j]; j++)
	    if (cst_streq(val_string(val_car(v)),r->letter_table[j]))
	    {
		word[i] = j;
		break;
	    }
        if (!r->letter_table[j])
        {
#if 0
            printf("awb_debug unknown letter >%s<\n",val_string(val_car(v)));
#endif
            i--;  /* can't find this letter so skip it */
        }
    }

    p = lts_apply(word,feats,r);
    cst_free(word);

    return p;
}

cst_val *lts_apply(const char *word,const char *feats,const cst_lts_rules *r)
{
    int pos, index, i;
    cst_val *phones=0;
    cst_lts_letter *fval_buff;
    cst_lts_letter *full_buff;
    cst_lts_phone phone;
    char *left, *right, *p;
    char hash;
    char zeros[8];
    
    /* For feature vals for each letter */
    fval_buff = cst_alloc(cst_lts_letter,
			  (r->context_window_size*2)+
			   r->context_extra_feats);
    /* Buffer with added contexts */
    full_buff = cst_alloc(cst_lts_letter,
			  (r->context_window_size*2)+
			  cst_strlen(word)+1); /* TBD assumes single POS feat */
    if (r->letter_table)
    {
	for (i=0; i<8; i++) zeros[i] = 2;
	cst_sprintf((char *)full_buff,
                    "%.*s%c%s%c%.*s",
		    r->context_window_size-1, zeros,
		    1,
		    word,
		    1,
		    r->context_window_size-1, zeros);
	hash = 1;
    }
    else
    {
	/* Assumes l_letter is a char and context < 8 */
	cst_sprintf((char *)full_buff,
                    "%.*s#%s#%.*s",
		    r->context_window_size-1, "00000000",
		    word,
		    r->context_window_size-1, "00000000");
	hash = '#';
    }

    /* Do the prediction backwards so we don't need to reverse the answer */
    for (pos = r->context_window_size + cst_strlen(word) - 1;
	 full_buff[pos] != hash;
	 pos--)
    {
	/* Fill the features buffer for the predictor */
	cst_sprintf((char *)fval_buff,
                    "%.*s%.*s%s",
		    r->context_window_size,
		    full_buff+pos-r->context_window_size,
		    r->context_window_size,
		    full_buff+pos+1,
		    feats);
	if ((!r->letter_table
	     && ((full_buff[pos] < 'a') || (full_buff[pos] > 'z'))))
	{   
#ifdef EXCESSIVELY_CHATTY
	    cst_errmsg("lts:skipping unknown char \"%c\"\n",
		       full_buff[pos]);
#endif
	    continue;
	}
	if (r->letter_table)
	    index = full_buff[pos] - 3;
	else
	    index = (full_buff[pos]-'a')%26;
	phone = apply_model(fval_buff,
			    r->letter_index[index],
			    r->models);
	/* delete epsilons and split dual-phones */
	if (cst_streq("epsilon",r->phone_table[phone]))
	    continue;
	else if ((p=strchr(r->phone_table[phone],'-')) != NULL)
	{
	    left = cst_substr(r->phone_table[phone],0,
			      cst_strlen(r->phone_table[phone])-cst_strlen(p));
	    right = cst_substr(r->phone_table[phone],
			       (cst_strlen(r->phone_table[phone])-cst_strlen(p))+1,
			       (cst_strlen(p)-1));
	    phones = cons_val(string_val(left),
			      cons_val(string_val(right),phones));
	    cst_free(left);
	    cst_free(right);
	}
	else
	    phones = cons_val(string_val(r->phone_table[phone]),phones);
    }

    cst_free(full_buff);
    cst_free(fval_buff);
    
    return phones;
}

static void cst_lts_get_state(cst_lts_rule *state,
			      const cst_lts_model *model,
			      unsigned short n,
			      int rule_size)
{   /* As some OS's require a more elaborate access than a simple lookup */
    memmove(state,&model[n*rule_size],rule_size);
}

static cst_lts_phone apply_model(cst_lts_letter *vals,cst_lts_addr start, 
				 const cst_lts_model *model)
{
    /* because some machines (ipaq/mips) can't deal with addrs not on     */
    /* word boundaries we use a static and copy the rule values each time */
    /* so we know its properly aligned                                    */
    /* Hmm this still might be wrong on some machines that align the      */
    /* structure cst_lts_rules differently                                */
    cst_lts_rule state;
    unsigned short nstate;
    static const int sizeof_cst_lts_rule = 6;

    cst_lts_get_state(&state,model,start,sizeof_cst_lts_rule);
    for ( ;
	 state.feat != CST_LTS_EOR;
	)
    {
/*	printf("%s %c %c %d\n",vals,vals[state.feat],state.val,
	(vals[state.feat] == state.val) ? 1 : 0); */
	if (vals[state.feat] == state.val)
	    nstate = state.qtrue;
	else
	    nstate = state.qfalse;
	/* This should really happen at compilation time */
	if (CST_BIG_ENDIAN)
	    nstate = SWAPSHORT(nstate);

	cst_lts_get_state(&state,model,nstate,sizeof_cst_lts_rule);
    }

    return (cst_lts_phone)state.val;
}
