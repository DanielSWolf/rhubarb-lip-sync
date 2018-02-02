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
/*  Items (and Item Contents)                                            */
/*                                                                       */
/*************************************************************************/
#include "cst_alloc.h"
#include "cst_item.h"
#include "cst_relation.h"
#include "cst_utterance.h"

/* Define functions for using items, rels and utts as vals */
CST_VAL_REGISTER_TYPE_NODEL(relation,cst_relation)
CST_VAL_REGISTER_TYPE_NODEL(item,cst_item)
CST_VAL_REGISTER_TYPE(utterance,cst_utterance)
CST_VAL_REGISTER_FUNCPTR(itemfunc,cst_itemfunc)

cst_item *new_item_relation(cst_relation *r,cst_item *i)
{
    cst_item *ni;

    ni = cst_utt_alloc(r->utterance, cst_item, 1);
    ni->contents = 0;
    ni->n = ni->p = ni->u = ni->d = 0;
    ni->relation = r;
    item_contents_set(ni,i);
    return ni;
}

void item_contents_set(cst_item *current, cst_item *i)
{
    cst_item_contents *c = 0;
    cst_item *nn_item;
    if (i == 0)
	c = new_item_contents(current);
    else
	c = i->contents;
    if (c != current->contents)
    {
	item_unref_contents(current);
	current->contents = c;
	/* If this contents is already in this relation         */
	/* empty the other reference                            */
	if (feat_present(current->contents->relations,current->relation->name))
	{   /* oops this is already in this relation */
	    nn_item = val_item(feat_val(current->contents->relations,
					current->relation->name));
	    feat_set(nn_item->contents->relations,
		     current->relation->name,
		     item_val(nn_item));
	       
	}
	/* Add back reference */
	feat_set(current->contents->relations,
		 current->relation->name,
		 item_val(current));
    }
}

void delete_item(cst_item *item)
{
    cst_item *ds, *nds;

    if (item->n != NULL) 
    { 
	item->n->p = item->p;
	item->n->u = item->u;  /* in trees if this is first daughter */
    }
    if (item->p != NULL) item->p->n = item->n;
    if (item->u != NULL) item->u->d = item->n; /* when first daughter */
    
    if (item->relation)
    {
	if (item->relation->head == item)
	    item->relation->head = item->n;
	if (item->relation->tail == item)
	    item->relation->tail = item->p;
    }

    /* Delete all the daughters of item */
    for (ds = item->d; ds; ds=nds)
    {
	nds = ds->n;
	delete_item(ds);
    }
    
    item_unref_contents(item);
    cst_utt_free(item->relation->utterance, item);
}

void item_unref_contents(cst_item *item)
{
    /* unreference this item from contents, and delete contents */
    /* if no one else is referencing it                         */

    if (item && item->contents)
    {
	feat_remove(item->contents->relations,item->relation->name);
	if (feat_length(item->contents->relations) == 0)
	{
	    delete_features(item->contents->relations);
	    delete_features(item->contents->features);
	    cst_utt_free(item->relation->utterance,item->contents);
	}
	item->contents = NULL;
    }
}

cst_item_contents *new_item_contents(cst_item *i)
{
    cst_item_contents *ic;

    ic = cst_utt_alloc(i->relation->utterance,cst_item_contents,1);
    ic->features = new_features_local(i->relation->utterance->ctx);
    ic->relations = new_features_local(i->relation->utterance->ctx);

    return ic;
}

cst_item *item_as(const cst_item *i,const char *rname)
{
    /* return i as relation rname or null */
    const cst_val *v;

    if (i == NULL)
	return NULL;
    else 
    {
        v = feat_val(i->contents->relations,rname);
        if (v != NULL)
            return val_item(v);
        else
            return NULL;
    }
}

/********************************************************************/
/*    List relation related functions                               */
/********************************************************************/

cst_item *item_next(const cst_item *i)
{
    if (i == NULL)
	return NULL;
    else
	return i->n;
}

cst_item *item_prev(const cst_item *i)
{
    if (i == NULL)
	return NULL;
    else
	return i->p;
}

cst_item *item_append(cst_item *current, cst_item *ni)
{
    cst_item *rni = 0;

    if (ni && (ni->relation == current->relation))
    {
	/* got to delete it first as an item can't be in a relation twice */
    }
    else
	rni = new_item_relation(current->relation,ni);

    rni->n = current->n;
    if (current->n != NULL)
	current->n->p = rni;
    rni->p = current;
    current->n = rni;

    if (current->relation->tail == current)
	current->relation->tail = rni;

    return rni;
}

cst_item *item_prepend(cst_item *current, cst_item *ni)
{
    cst_item *rni = 0;

    if (ni && (ni->relation == current->relation))
    {
	/* got to delete it first as an item can't be in a relation twice */
    }
    else
	rni = new_item_relation(current->relation,ni);

    rni->p = current->p;
    if (current->p != NULL)
	current->p->n = rni;
    rni->n = current;
    current->p = rni;

    if (current->u)  /* in a tree */
    {
	current->u->d = rni;
	rni->u = current->u;
	current->u = NULL;
    }

    if (current->relation->head == current)
	current->relation->head = rni;

    return rni;
}

/********************************************************************/
/*    Tree relation related functions                               */
/********************************************************************/
cst_item *item_parent(const cst_item *i)
{
    const cst_item *n;

    for (n=i; item_prev(n); n=item_prev(n));
    if (n == NULL)
	return NULL;
    else 
	return n->u;
}

cst_item *item_daughter(const cst_item *i)
{
    if (i == NULL)
	return NULL;
    else 
	return i->d;
}

cst_item *item_nth_daughter(const cst_item *i,int n)
{
    int d;
    cst_item *p;

    for (d=0,p=item_daughter(i); p && (d < n); p=item_next(p),d++);
    return p;
}


cst_item *item_last_daughter(const cst_item *i)
{
    cst_item *p;

    for (p=item_daughter(i); item_next(p); p=item_next(p));
    return p;
}

cst_item *item_add_daughter(cst_item *i,cst_item *nd)
{
    cst_item *p,*rnd;

    p = item_last_daughter(i);

    if (p)
	rnd=item_append(p,nd);
    else
    {   /* first new daughter */
	if (nd && (nd->relation == i->relation))
	{
	    /* got to delete it first as nd can't be in a relation twice */
	    cst_errmsg("item_add_daughter: already in relation\n");
	    return 0;
	}
	else
	    rnd = new_item_relation(i->relation,nd);

	rnd->u = i;
	i->d = rnd;
    }

    return rnd;
}

/********************************************************************/
/*    Feature functions                                             */
/********************************************************************/

int item_feat_present(const cst_item *i,const char *name)
{
    return feat_present(item_feats(i),name);
}

int item_feat_remove(const cst_item *i,const char *name)
{
    return feat_remove(item_feats(i),name);
}

cst_features *item_feats(const cst_item *i)
{
    return (i ? i->contents->features : NULL);
}

const cst_val *item_feat(const cst_item *i,const char *name)
{
    return feat_val(item_feats(i),name);
}

int item_feat_int(const cst_item *i,const char *name)
{
    return feat_int(item_feats(i),name);
}

float item_feat_float(const cst_item *i,const char *name)
{
    return feat_float(item_feats(i),name);
}

const char *item_feat_string(const cst_item *i,const char *name)
{
    return feat_string(item_feats(i),name);
}

void item_set(const cst_item *i,const char *name,const cst_val *val)
{
    feat_set(item_feats(i),name,val);
}

void item_set_int(const cst_item *i,const char *name,int val)
{
    feat_set_int(item_feats(i),name,val);
}

void item_set_float(const cst_item *i,const char *name,float val)
{
    feat_set_float(item_feats(i),name,val);
}

void item_set_string(const cst_item *i,const char *name,const char *val)
{
    feat_set_string(item_feats(i),name,val);
}

cst_utterance *item_utt(const cst_item *i)
{
    if (i && i->relation)
	return i->relation->utterance;
    else
	return NULL;
}

int item_equal(const cst_item *a, const cst_item *b)
{
    if ((a == b) ||
	(a && b && (a->contents == b->contents)))
	return TRUE;
    else
	return FALSE;
}
