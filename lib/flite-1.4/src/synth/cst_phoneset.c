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
/*               Date:  December 2000                                    */
/*************************************************************************/
/*                                                                       */
/*    Voice definition                                                   */
/*                                                                       */
/*************************************************************************/
#include "cst_val.h"
#include "cst_utterance.h"
#include "cst_item.h"
#include "cst_phoneset.h"

CST_VAL_REGISTER_TYPE_NODEL(phoneset,cst_phoneset)

cst_phoneset *new_phoneset()
{
    /* These aren't going to be supported dynamically */
    cst_phoneset *v = cst_alloc(struct cst_phoneset_struct,1);

    return v;
}

void delete_phoneset(cst_phoneset *v)
{
    if (v)
    {
	cst_free(v);
    }
}

int phone_id(const cst_phoneset *ps,const char* phonename)
{
    int i;

    for (i=0; i< ps->num_phones; i++)
	if (cst_streq(ps->phonenames[i],phonename))
	    return i;
    /* Wonder if I should print an error here or not */

    return 0;
}

int phone_feat_id(const cst_phoneset *ps,const char* featname)
{
    int i;

    for (i=0; ps->featnames[i]; i++)
	if (cst_streq(ps->featnames[i],featname))
	    return i;

    /* Wonder if I should print an error here or not */
    return 0;
}

const cst_val *phone_feature(const cst_phoneset *ps,
			     const char* phonename,
			     const char *featname)
{
    return ps->featvals[ps->fvtable[phone_id(ps,phonename)]
          		           [phone_feat_id(ps,featname)]];
}

const char *phone_feature_string(const cst_phoneset *ps,
				 const char* phonename,
				 const char *featname)
{
    return val_string(phone_feature(ps,phonename,featname));
}


const cst_phoneset *item_phoneset(const cst_item *p)
{
    return val_phoneset(feat_val(item_utt(p)->features,"phoneset"));
}


