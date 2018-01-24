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
/*  Relations                                                            */
/*                                                                       */
/*************************************************************************/
#include "cst_item.h"
#include "cst_relation.h"
#include "cst_utterance.h"

static const char * const cst_relation_noname = "NoName";

cst_relation *new_relation(const char *name, cst_utterance *u)
{
    cst_relation *r = cst_utt_alloc(u,cst_relation,1);

    r->name = cst_strdup(name);
    r->features = new_features_local(u->ctx);
    r->head = NULL;
    r->utterance = u;

    return r;
}

void delete_relation(cst_relation *r)
{
    cst_item *p, *np;

    if (r != NULL)
    {
	/* This needs to traverse the *all* items */
	for(p=r->head; p; p=np)
	{
	    np = item_next(p);
	    delete_item(p);  /* this *does* go down daughters too */
	}
	delete_features(r->features);
	cst_free(r->name);
	cst_utt_free(r->utterance,r);
    }
}

cst_item *relation_head(cst_relation *r)
{
    return ( r == NULL ? NULL : r->head);
}

cst_item *relation_tail(cst_relation *r)
{
    return (r == NULL ? NULL : r->tail);
}

const char *relation_name(cst_relation *r)
{
    return (r == NULL ? cst_relation_noname : r->name);
}

cst_item *relation_append(cst_relation *r, cst_item *i)
{
    cst_item *ni = new_item_relation(r,i);
    
    if (r->head == NULL)
	r->head = ni;

    ni->p = r->tail;
    if (r->tail)
	r->tail->n = ni;
    r->tail = ni;
    return ni;
}

cst_item *relation_prepend(cst_relation *r, cst_item *i)
{
    cst_item *ni = new_item_relation(r,i);
    
    if (r->tail == NULL)
	r->tail = ni;

    ni->n = r->head;
    if (r->head)
	r->head->p = ni;
    r->head = ni;
    return ni;
}
