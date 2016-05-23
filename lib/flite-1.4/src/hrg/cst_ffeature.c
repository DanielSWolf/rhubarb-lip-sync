/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                       Copyright (c) 1999-2007                         */
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
/*               Date:  April 2000                                       */
/*************************************************************************/
/*                                                                       */
/*  Item features and paths                                              */
/*                                                                       */
/*************************************************************************/
#include "cst_alloc.h"
#include "cst_item.h"
#include "cst_relation.h"
#include "cst_utterance.h"
#include "cst_tokenstream.h"

CST_VAL_REGISTER_FUNCPTR(ffunc,cst_ffunction)

DEF_STATIC_CONST_VAL_STRING(ffeature_default_val,"0");

static const void *internal_ff(const cst_item *item,
			       const char *featpath,int type);

const char *ffeature_string(const cst_item *item,const char *featpath)
{
    return val_string(ffeature(item,featpath));
}
int ffeature_int(const cst_item *item,const char *featpath)
{
    return val_int(ffeature(item,featpath));
}
float ffeature_float(const cst_item *item,const char *featpath)
{
    return val_float(ffeature(item,featpath));
}

cst_item* path_to_item(const cst_item *item,const char *featpath)
{
    return (cst_item *)internal_ff(item,featpath,1);
}

const cst_val *ffeature(const cst_item *item,const char *featpath)
{
    return (cst_val *)internal_ff(item,featpath,0);
}

static const void *internal_ff(const cst_item *item,
			       const char *featpath,int type)
{
    const char *tk, *relation;
    cst_utterance *utt;
    const cst_item *pitem;
    void *void_v;
    const cst_val *ff;
    cst_ffunction ffunc;
    char tokenstring[200]; /* we don't seem to have featpaths longer than 72 */
    char *tokens[100];
    int i,j;

    /* This used to use cst_tokenstream but that was too slow */
    for (i=0; i<199 && featpath[i]; i++)
        tokenstring[i] = featpath[i];
    tokenstring[i]='\0';
    tokens[0] = tokenstring;
    for (i=0,j=1; tokenstring[i]; i++)
    {
        if (strchr(":.",tokenstring[i]))
        {
            tokenstring[i] = '\0';
            tokens[j] = &tokenstring[i+1];
            j++;
        }
    }
    tokens[j] = NULL;
    j=0;
    for (tk = tokens[j], pitem=item;
	 pitem && 
	     (((type == 0) && tokens[j+1]) ||
	      ((type == 1) && tk));
	 j++, tk = tokens[j])
    {
	if (cst_streq(tk,"n"))
	    pitem = item_next(pitem);
	else if (cst_streq(tk,"p"))
	    pitem = item_prev(pitem);
	else if (cst_streq(tk,"pp"))
	{
	    if (item_prev(pitem))
		pitem = item_prev(item_prev(pitem));
	    else
		pitem = NULL;
	}
	else if (cst_streq(tk,"nn"))
	{
	    if (item_next(pitem))
		pitem = item_next(item_next(pitem));
	    else
		pitem = NULL;
	}
	else if (cst_streq(tk,"parent"))
	    pitem = item_parent(pitem);
	else if ((cst_streq(tk,"daughter")) ||
		 (cst_streq(tk,"daughter1")))
	    pitem = item_daughter(pitem);
	else if (cst_streq(tk,"daughtern"))
	    pitem = item_last_daughter(pitem);
	else if (cst_streq(tk,"R"))
	{
	    /* A relation move */
            j++;
	    relation = tokens[j];
	    pitem = item_as(pitem,relation);
	}
	else
	{
	    cst_errmsg("ffeature: unknown directive \"%s\" ignored\n",tk);
	    return NULL;
	}
    }

    if (type == 0)
    {
	if (pitem && (utt = item_utt(pitem)))
	    ff = feat_val(utt->ffunctions,tk);
	else
	    ff = NULL;
	void_v = NULL;
	if (!ff)
	    void_v = (void *)item_feat(pitem,tk);
	else if (pitem)
	{
	    ffunc = val_ffunc(ff);
	    void_v = (void *)(*ffunc)(pitem);
	}
	if (void_v == NULL)
        {
#if 0
            if (pitem)
                printf("awb_debug didn't find %s in %s\n",tk,
                       get_param_string(pitem->contents->features,"name","noname"));
            else
            {
                if (cst_streq("gpos",tk))
                    printf("awb_debug2\n");
                printf("awb_debug didn't find %s %s\n",tk,featpath);
            }
#endif
	    void_v = (void *)&ffeature_default_val;
        }
    }
    else
	void_v = (void *)pitem;

    return void_v;
}

void ff_register(cst_features *ffunctions, const char *name, cst_ffunction f)
{
    /* Register features functions */

    if (feat_present(ffunctions, name))
	cst_errmsg("warning: ffunction %s redefined\n", name);
    feat_set(ffunctions, name, ffunc_val(f));
}

void ff_unregister(cst_features *ffunctions, const char *name)
{
    feat_remove(ffunctions, name);
}
