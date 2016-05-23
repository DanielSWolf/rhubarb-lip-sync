/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 2000                             */
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
/*               Date:  August 2000                                      */
/*************************************************************************/
/*                                                                       */
/*  A Viterbi decoder                                                    */
/*                                                                       */
/*************************************************************************/

#include <limits.h>
#include "cst_viterbi.h"

static void vit_point_init_path_array(cst_vit_point *n,int num_states);
static void vit_point_init_dynamic_path_array(cst_vit_point *n,
					      cst_vit_cand *c);
static void vit_add_paths(cst_viterbi *vd,
			  cst_vit_point *point,
			  cst_vit_path *path);
static void vit_add_path(cst_viterbi *vd,cst_vit_point *p, cst_vit_path *np);
static cst_vit_path *find_best_path(cst_viterbi *vd);
static int betterthan(cst_viterbi *v,int a, int b);

cst_vit_cand *new_vit_cand()
{
    cst_vit_cand *c = cst_alloc(struct cst_vit_cand_struct,1);

    return c;
}

void vit_cand_set(cst_vit_cand *vc, cst_val *val)
{
	if (vc->val)
		delete_val(vc->val);
	vc->val = val;
	val_inc_refcount(vc->val);
}

void vit_cand_set_int(cst_vit_cand *vc, int ival)
{
	vc->ival = ival;
	vit_cand_set(vc, int_val(ival));
}

void delete_vit_cand(cst_vit_cand *vc)
{
    if (vc)
    {
	delete_val(vc->val);
	delete_vit_cand(vc->next);
	cst_free(vc);
    }
}

cst_vit_path *new_vit_path()
{
    cst_vit_path *p = cst_alloc(struct cst_vit_path_struct,1);

    return p;
}

void delete_vit_path(cst_vit_path *vp)
{
    if (vp)
    {
	if (vp->f)
	    delete_features(vp->f);
	delete_vit_path(vp->next);
	cst_free(vp);
    }
}

cst_vit_point *new_vit_point()
{
    cst_vit_point *p = cst_alloc(struct cst_vit_point_struct,1);

    return p;
}

void delete_vit_point(cst_vit_point *vp)
{
    int i;

    if (vp)
    {
	if (vp->paths)
	    delete_vit_path(vp->paths);
	if (vp->num_states != 0)
	{
	    for (i=0; i<vp->num_states; i++)
		if (vp->state_paths[i])
		    delete_vit_path(vp->state_paths[i]);
	    cst_free(vp->state_paths);
	}
	delete_vit_cand(vp->cands);
	delete_vit_point(vp->next);
	cst_free(vp);
    }
}

cst_viterbi *new_viterbi(cst_vit_cand_f_t *cand_func, 
			 cst_vit_path_f_t *path_func)
{
    cst_viterbi *v = cst_alloc(struct cst_viterbi_struct,1);
    
    v->cand_func = cand_func;
    v->path_func = path_func;
    v->f = new_features();
    return v;
}

void delete_viterbi(cst_viterbi *vd)
{
    if (vd)
    {
	delete_vit_point(vd->timeline);
	delete_features(vd->f);
	cst_free(vd);
    }

    return;
}

void viterbi_initialise(cst_viterbi *vd,cst_relation *r)
{
    cst_item *i;
    cst_vit_point *last = 0;
    cst_vit_point *n = 0;

    /* Construct the timeline with points for each item in relation */
    /* initiallising the state tables at each point                 */
    for (i=relation_head(r); TRUE ; i=item_next(i))
    {
	n = new_vit_point();
	n->item = i;
	if (vd->num_states > 0) /* a state based viterbi search */
	    vit_point_init_path_array(n,vd->num_states);
	if (last)
	    last->next = n;
	else
	    vd->timeline = n;
	last = n;
	/* we need an extra one (even if there are no items) */
	/* so we go one past end of the relation             */
	if (i == 0)  
	{
	    vd->last_point = n;
	    break;
	}
    }

    if (vd->num_states == 0)  /* its a general beam search */
	vd->timeline->paths = new_vit_path();
    if (vd->num_states == -1) /* Dynamic number of states (# cands) */
	vit_point_init_path_array(vd->timeline,1);
    
}

static void vit_point_init_path_array(cst_vit_point *n,int num_states)
{
    n->num_states = num_states;
    n->state_paths = cst_alloc(cst_vit_path*,num_states);
}

static void vit_point_init_dynamic_path_array(cst_vit_point *n,
					      cst_vit_cand *c)
{
    int i;
    cst_vit_cand *cc;

    for (cc=c,i=0; cc; i++,cc=cc->next)
	cc->pos = i;

    vit_point_init_path_array(n,i);
}

void viterbi_decode(cst_viterbi *vd)
{
    cst_vit_point *p;
    cst_vit_path *np;
    cst_vit_cand *c;
    int i;

    /* For each time point */
    for (p=vd->timeline; p->next != NULL; p=p->next)
    {
	/* Find the candidates */
	p->cands = (*vd->cand_func)(p->item,vd);  
	if (vd->num_states != 0)  /* true viterbi */
	{
	    if (vd->num_states == -1)  /* dynamic number of states (# cands) */
		vit_point_init_dynamic_path_array(p->next,p->cands);
	    for (i=0; i<p->num_states; i++)
	    {

		if (((p == vd->timeline) && i==0) ||
		    (p->state_paths[i] != 0))
		    for (c=p->cands; c; c=c->next)
		    {
			np = (*vd->path_func)(p->state_paths[i],c,vd);
			vit_add_paths(vd,p->next,np);
		    }
	    }
		
	}
	else                      /* general beam search */
	{
	    cst_errmsg("viterbi, general beam search not implemented\n");
/*
	    for (t=p->paths; t; t=t->next)
	    {
		for (c=p->cands; c; c=c->next)
		{
		    np = (vd->path_func)(t,c,vd);
		    add_path(p->next,np);
		}
	    }
*/
	}
    }
}

static void vit_add_paths(cst_viterbi *vd,
			  cst_vit_point *point,
			  cst_vit_path *path)
{
    /* Add a list of paths at point */
    cst_vit_path *p, *next_p;

    for (p=path; p; p=next_p)
    {
	next_p = p->next; /* as p could be deleted is not required */
	vit_add_path(vd,point,p);
    }
}

static void vit_add_path(cst_viterbi *vd,cst_vit_point *p, cst_vit_path *np)
{
    if (p->state_paths[np->state] == 0)
    {   /* we don't have one yet so this is best */
	p->state_paths[np->state] = np;
    }
    else if (betterthan(vd,np->score,p->state_paths[np->state]->score))
    {   /* its better than what we have already */
	delete_vit_path(p->state_paths[np->state]);
	p->state_paths[np->state] = np;
    }
    else
	delete_vit_path(np);
}

int viterbi_result(cst_viterbi *vd,const char *n)
{
    /* Find best path through the decoder, adding field to item named n */
    /* with choising value                                              */
    cst_vit_path *p;

    if ((vd->timeline == 0) || (vd->timeline->next == 0))
	return TRUE;  /* it has succeeded in the null case */
    p = find_best_path(vd);
    if (p == NULL)    /* bummer */
	return FALSE;
    
    for (; p; p=p->from)
	if (p->cand)
	{
	    item_set(p->cand->item,n,p->cand->val);
	}
    return TRUE;
}

void viterbi_copy_feature(cst_viterbi *vd,const char *featname)
{
    /* copy a feature from the best path to related item */
    cst_vit_path *p;

    p = find_best_path(vd);
    if (p == NULL)    /* nothing to copy, emtpy stream or no solution */
	return;
    
    for (; p; p=p->from)
	if (p->cand && feat_present(p->f, featname))
	    item_set(p->cand->item,featname,feat_val(p->f,featname));
    return;
}

static cst_vit_path *find_best_path(cst_viterbi *vd)
{
    cst_vit_point *t;
    int best,worst;
    cst_vit_path *best_p=NULL;
    int i;

    if (vd->big_is_good)
	worst = -INT_MAX;
    else
	worst = INT_MAX;
    best = worst;  /* though we should be able to find something better */

    t = vd->last_point;

    if (vd->num_states != 0)
    {
	for (i=0; i<t->num_states; i++)
	{
	    if ((t->state_paths[i] != NULL) &&
		(betterthan(vd,t->state_paths[i]->score, best)))
	    {
		best = t->state_paths[i]->score;
		best_p = t->state_paths[i];
	    }
	}
    }

    return best_p;
}

static int betterthan(cst_viterbi *v,int a, int b)
{
    /* better may be most big or most small */
    /* for probabalities big is good */
    
    if (v->big_is_good)
	return (a > b);
    else
	return (a < b);
}
