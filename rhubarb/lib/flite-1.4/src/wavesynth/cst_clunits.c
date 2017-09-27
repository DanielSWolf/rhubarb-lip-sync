/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                         Copyright (c) 2000                            */
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
/*             Authors:  Alan W Black (awb@cs.cmu.edu)                   */
/*    			 David Huggins-Daines (dhd@cepstral.com)	 */
/*               Date:  April 2001                                       */
/*************************************************************************/
/*                                                                       */
/*  clunits waveform synthesis                                           */
/*                                                                       */
/*************************************************************************/

#include <limits.h>

#include "cst_hrg.h"
#include "cst_utt_utils.h"
#include "cst_viterbi.h"
#include "cst_clunits.h"
#include "cst_units.h"
#include "cst_wave.h"
#include "cst_track.h"
#include "cst_sigpr.h"

#define CLUNITS_DEBUG 0

#ifndef CLUNITS_DEBUG
#define CLUNITS_DEBUG 0
#endif

#if CLUNITS_DEBUG > 0
# define DPRINTF(l,x) if (CLUNITS_DEBUG > (l)) cst_dbgmsg x
#else
# define DPRINTF(l,x)
#endif

#define BIG_BAD_NUMBER 50000 /* big enough to be bad, not big enough
                                to cause overflows */

CST_VAL_REGISTER_TYPE_NODEL(clunit_db,cst_clunit_db)
CST_VAL_REGISTER_TYPE_NODEL(vit_cand,cst_vit_cand)

static cst_utterance *clunits_select(cst_utterance *utt);
static cst_vit_cand *cl_cand(cst_item *i,
			     struct cst_viterbi_struct *vd);
static cst_vit_path *cl_path(cst_vit_path *p,
			     cst_vit_cand *c,
			     cst_viterbi *vd);
static const cst_cart *clunit_get_tree(cst_clunit_db *cludb, const char *name);
static void clunit_set_unit_name(cst_item *s,cst_clunit_db *clunit_db);

typedef int (*cst_distfunc)(const cst_clunit_db *, int, int, const int *, int, int);
static int optimal_couple_frame(cst_clunit_db *cludb, int u0, int u1,
				cst_distfunc dfunc, int bestsofar);
static int optimal_couple(cst_clunit_db *cludb,
			  int u0, int u1,
			  int *u0_move, int *u1_move,
			  cst_distfunc dfunc);
static int frame_distance(const cst_clunit_db *cludb,
			  int a, int b,
			  const int *join_weights,
			  int order,
                          int best);
static int frame_distanceb(const cst_clunit_db *cludb,
			   int a, int b,
			   const int *join_weights,
			   int order,
                           int best);


cst_utterance *clunits_synth(cst_utterance *utt)
{
    /* Basically the same as the diphone code */
    clunits_select(utt);
    join_units(utt);

    return utt;
}

cst_utterance *clunits_dump_units(cst_utterance *utt)
{
    cst_clunit_db *clunit_db;
    cst_item *s, *u;
    int unit_entry;

    clunit_db = val_clunit_db(feat_val(utt->features,"clunit_db"));
    for (s = relation_head(utt_relation(utt,"Segment")); s; s = item_next(s))
    {
	u = item_daughter(s);
	unit_entry = item_feat_int(u,"unit_entry");
	cst_dbgmsg("for %s end %f selected %d %s start move %d end move %d\n",
		   item_name(s),
		   item_feat_float(s,"end"),
		   unit_entry,
		   item_name(u),
		   item_feat_int(u,"unit_start") - clunit_db->units[unit_entry].start,
		   item_feat_int(u,"unit_end") - clunit_db->units[unit_entry].end);
    }

    return utt;
}

static cst_utterance *clunits_select(cst_utterance *utt)
{
    cst_viterbi *vd;
    cst_relation *units,*segs;
    cst_item *s,*u;
    cst_clunit_db *clunit_db;
    int unit_entry;
    
    segs = utt_relation(utt,"Segment");
    vd = new_viterbi(cl_cand,cl_path);
    vd->num_states = -1;
    vd->big_is_good = FALSE;
    feat_set(vd->f,"clunit_db",feat_val(utt->features,"clunit_db"));
    clunit_db = val_clunit_db(feat_val(vd->f,"clunit_db"));
    utt_set_feat(utt,"sts_list",sts_list_val(clunit_db->sts));

    for (s=relation_head(segs); s; s=item_next(s))
	clunit_set_unit_name(s,clunit_db);

    viterbi_initialise(vd,segs);
    viterbi_decode(vd);
    if (!viterbi_result(vd,"selected_unit"))
    {
	cst_errmsg("clunits: can't find path\n");
	cst_error();
    }
    viterbi_copy_feature(vd, "unit_prev_move");
    viterbi_copy_feature(vd, "unit_this_move");
    delete_viterbi(vd);

    /* Construct unit stream with selected units */
    units = utt_relation_create(utt,"Unit");
    for (s=relation_head(segs); s; s=item_next(s))
    {
	u = relation_append(units,NULL);
	item_set_string(u,"name",item_name(s));

	unit_entry = item_feat_int(s,"selected_unit");

	/* Get stuff from unit_db */
	item_set(u,"unit_entry",item_feat(s,"selected_unit"));
	item_set(u,"clunit_name",item_feat(s,"clunit_name"));
#if 0
        printf("awb_debug %s\n",item_feat_string(u,"clunit_name"));
#endif

	/* Use optimal join points if available */
	if (item_feat_present(s, "unit_this_move"))
	    item_set_int(u,"unit_start", item_feat_int(s, "unit_this_move"));
	else
	    item_set_int(u,"unit_start",clunit_db->units[unit_entry].start);

	if (item_next(s) && item_feat_present(item_next(s), "unit_prev_move"))
	    item_set_int(u,"unit_end", item_feat_int(item_next(s), "unit_prev_move"));
	else
	    item_set_int(u,"unit_end",clunit_db->units[unit_entry].end);

	if (item_feat_int(u,"unit_start") > item_feat_int(u, "unit_end"))
	{
            /*	    feat_print(stdout,s->contents->features); */
	    cst_errmsg("start %d end %d\n",
		    item_feat_int(u,"unit_start"), item_feat_int(u, "unit_end"));
	    /* feat_print(stdout,u->contents->features); */
	}

	DPRINTF(0, ("selected %d=%s_%d %d/%d\n",
		    unit_entry, 
		    UNIT_TYPE(clunit_db,unit_entry),
		    UNIT_INDEX(clunit_db,unit_entry),
		    item_feat_int(u,"unit_start"), item_feat_int(u, "unit_end")));

	item_set_int(u,"target_end",
		     (int)(item_feat_float(s,"end")*clunit_db->sts->sample_rate));
    }

    return utt;
}

static cst_vit_cand *cl_cand(cst_item *i,cst_viterbi *vd)
{
    const char *unit_type;
    unsigned short nu;
    int idx;
    int e;
    const cst_val *clist,*c;
    cst_vit_cand *p,*all,*gt,*lc;
    cst_clunit_db *clunit_db;

    clunit_db = val_clunit_db(feat_val(vd->f,"clunit_db"));
    unit_type = item_feat_string(i,"clunit_name");

    /* get tree */
    clist = cart_interpret(i,clunit_get_tree(clunit_db,unit_type));

    all = 0;
    for (c=clist; c; c=val_cdr(c))
    {
	idx = clunit_get_unit_index(clunit_db, unit_type, val_int(val_car(c)));
	p = new_vit_cand();
	p->next = all;
	p->item = i;
	p->score = 0;
	vit_cand_set_int(p,idx);
	all = p;
    }

    if ((clunit_db->extend_selections > 0) && (item_prev(i)))
    {
	lc = val_vit_cand(item_feat(item_prev(i),"clunit_cands"));
	for (e=0; lc && (e < clunit_db->extend_selections); lc=lc->next)
	{
	    nu = clunit_db->units[lc->ival].next;
	    if (nu == CLUNIT_NONE)
		continue;
	    for (gt=all; gt; gt=gt->next)
		if (nu == gt->ival)
		    break;  /* we've got this one already */
	    if ((gt == 0)
		&& (clunit_db->units[nu].type
		    == clunit_db->units[all->ival].type))
	    {
		p = new_vit_cand();
		p->next = all;
		p->item = i;
		p->score = 0;
		vit_cand_set_int(p, nu);
		all = p;
		e++;
	    }
	}
    }
    item_set(i,"clunit_cands",vit_cand_val(all));

    return all;
}

static cst_vit_path *cl_path(cst_vit_path *p,
			     cst_vit_cand *c,
			     cst_viterbi *vd)
{
    int cost;
    cst_vit_path *np;
    cst_clunit_db *cludb;
    cst_distfunc dfunc;
    int u0,u1;
    int u0_move = -1, u1_move = -1;

    np = new_vit_path();
    cludb = val_clunit_db(feat_val(vd->f,"clunit_db"));	
    if (cludb->mcep->sts)
        dfunc = frame_distance;
    else if (cludb->mcep->sts_paged)
        dfunc = frame_distance;
    else
	dfunc = frame_distanceb;

    np->cand = c;
    np->from = p;
    
    if ((p==0) || (p->cand == 0))
	cost = 0;
    else
    {
	u0 = p->cand->ival;
	u1 = c->ival;
	if (cludb->optimal_coupling == 1) {
	    if (np->f == NULL)
		np->f = new_features();
	    cost = optimal_couple(cludb, u0, u1, &u0_move, &u1_move, dfunc);
	    if (u0_move != -1)
		    feat_set(np->f, "unit_prev_move", int_val(u0_move));
	    if (u1_move != -1)
		    feat_set(np->f, "unit_this_move", int_val(u1_move));
	} else if (cludb->optimal_coupling == 2)
	    cost = optimal_couple_frame(cludb, u0, u1, dfunc, INT_MAX);
	else
	    cost = 0;
    }

    cost *= 5; /* magic number ("continuity weight") */
    np->state = c->pos;
    if (p==0)
	np->score = cost + c->score;
    else
	np->score = cost + c->score + p->score;

    return np;
}

static int optimal_couple_frame(cst_clunit_db *cludb, int u0, int u1,
				cst_distfunc dfunc,
                                int bestsofar)
{
    int a,b;

    if (cludb->units[u1].prev == u0)
	return 0; /* Consecutive units win */

    if (cludb->units[u0].next != CLUNIT_NONE)
	a = cludb->units[u0].end;
    else    /* don't want to do this but its all that is left to do */
	a = cludb->units[u0].end-1;  /* if num frames < 1 this is bad */
    b = cludb->units[u1].start;

    return (*dfunc)(cludb, a, b,
		    cludb->join_weights,
		    cludb->mcep->num_channels,bestsofar)
	    + abs(get_frame_size(cludb->sts, a)
		  - get_frame_size(cludb->sts,b)) * cludb->f0_weight;
}

static int optimal_couple(cst_clunit_db *cludb,
			  int u0, int u1,
			  int *u0_move, int *u1_move,
			  cst_distfunc dfunc)
{
    int a,b;
    int u1_p;
    int i, fcount;
    int u0_st, u1_p_st, u0_end, u1_p_end;
    int best_u0, best_u1_p;
    int dist, best_val;

    u1_p = cludb->units[u1].prev;

    if (u1_p == u0)
	return 0;
    if (u1_p == CLUNIT_NONE || cludb->units[u0].phone != cludb->units[u1_p].phone)
	return 10 * optimal_couple_frame(cludb, u0, u1, dfunc,INT_MAX); /* laziness */

    DPRINTF(1,("optimal_coupling %s_%d (%d,%d) %s_%d (%d,%d)\n",
	       UNIT_TYPE(cludb,u0),
	       UNIT_INDEX(cludb,u0),
	       cludb->units[u0].start, cludb->units[u0].end,
	       UNIT_TYPE(cludb,u1),
	       UNIT_INDEX(cludb,u1),
	       cludb->units[u1].start, cludb->units[u1].end));

    u0_end = cludb->units[u0].end - cludb->units[u0].start;
    u1_p_end = cludb->units[u1_p].end - cludb->units[u1_p].start;

    u0_st = u0_end / 3;
    u1_p_st = u1_p_end / 3;

    if ((u0_end - u0_st) < (u1_p_end - u1_p_st))
    {
	fcount = u0_end - u0_st;
	/* u1_p_st = u1_p_end - fcount; */
    }
    else
    {
	fcount = u1_p_end - u1_p_st;
	/* u0_st = u0_end - fcount; */
    }

    DPRINTF(1,("%s == %s, sliding u0:(%d,%d) u1:(%d,%d)\n",
	       UNIT_TYPE(cludb,u0),
	       UNIT_TYPE(cludb,u1_p),
	       u0_st, u0_end, u1_p_st, u1_p_end));

    best_u0 = u0_end;
    best_u1_p = u1_p_end;
    best_val = INT_MAX;

    for (i = 0; i < fcount; ++i) {
	a = cludb->units[u0].start + u0_st + i;
	b = cludb->units[u1_p].start + u1_p_st + i;

	dist = 
	    (*dfunc)(cludb, a, b,
		     cludb->join_weights,
		     cludb->mcep->num_channels,
                     best_val)
	    + abs(get_frame_size(cludb->sts, a)
		  - get_frame_size(cludb->sts,b)) * cludb->f0_weight;

	if (dist < best_val) {
	    best_val = dist;
	    best_u0 = u0_st + i;
	    best_u1_p = u1_p_st + i;
	}
    }

    if (best_val == INT_MAX)
	best_val = BIG_BAD_NUMBER; /* prevent overflows for zero-length units */

    /* u0_move is the new end for u0
       u1_move is the new start for u1

       This works based on the assumption that the STS frames for
       consecutive units in the recordings will also be consecutive.
       The voice compiler MUST preserve this assumption! */

    *u0_move = cludb->units[u0].start + best_u0;
    *u1_move = cludb->units[u1_p].start + best_u1_p;
    DPRINTF(1,("best_u0 %d = %d best_u1 %d = %d best_val %d\n",
	       best_u0, *u0_move, best_u1_p, *u1_move, best_val));

    return 30000 + best_val;
}

static int frame_distance(const cst_clunit_db *cludb,
			  int a, int b,
			  const int *join_weights,
			  int order,
                          int bestsofar)
{
    int r,diff;
    int i;
    const unsigned short *av, *bv;

    bv = get_sts_frame(cludb->mcep, b);
    av = get_sts_frame(cludb->mcep, a);

#if CLUNITS_DEBUG > 2
    cst_dbgmsg("a(%d): ",a);
    for (i = 0; i < order; ++i)
	cst_dbgmsg("%.2f ",
		   (double) av[i] * cludb->mcep->coeff_range / 65536
		   + cludb->mcep->coeff_min);
    cst_dbgmsg("\n");
    cst_dbgmsg("b(%d): ",b);
    for (i = 0; i < order; ++i)
	cst_dbgmsg("%.2f ",
		   (double) bv[i] * cludb->mcep->coeff_range / 65536
		   + cludb->mcep->coeff_min);
    cst_dbgmsg("\n");
#endif

    /* Weighted Manhattan distance */
    for (r = 0, i = 0; i < order; i++)
    {
	diff = av[i]-bv[i];
	r += abs(diff) * join_weights[i] / 65536;
        if (r > bestsofar)
            return r; /* already worse than best */
    }

    return r;
}

static int frame_distanceb(const cst_clunit_db *cludb,
			   int a, int b,
			   const int *join_weights,
			   int order,
                           int bestsofar)
{
    int r,diff;
    int i;
    const unsigned char *av, *bv;

    bv = get_sts_residual_fixed(cludb->mcep, b);
    av = get_sts_residual_fixed(cludb->mcep, a);

#if CLUNITS_DEBUG > 2
    cst_dbgmsg("a(%d): ",a);
    for (i = 0; i < order; ++i)
	cst_dbgmsg("%.2f ",
		   (double) av[i] * cludb->mcep->coeff_range / 256
		   + cludb->mcep->coeff_min);
    cst_dbgmsg("\n");
    cst_dbgmsg("b(%d): ",b);
    for (i = 0; i < order; ++i)
	cst_dbgmsg("%.2f ",
		   (double) bv[i] * cludb->mcep->coeff_range / 256
		   + cludb->mcep->coeff_min);
    cst_dbgmsg("\n");
#endif

    /* Weighted Manhattan distance */
    for (r = 0, i = 0; i < order; i++)
    {
	diff = (av[i]-bv[i]) * 256;
	r += abs(diff) * join_weights[i] / 65536;
        if (r > bestsofar)
            return r; /* already worse than best */
    }

    return r;
}

static int clunit_get_unit_type_index(cst_clunit_db *cludb, const char *name)
{
    int start,end,mid,c;

    start = 0;
    end = cludb->num_types;

    while (start < end) 
    {
	mid = (start+end)/2;
	c = strcmp(cludb->types[mid].name,name);

	if (c == 0)
	    return mid;
	else if (c > 0)
	    end = mid;
	else
	    start = mid + 1;
    }

    cst_errmsg("clunits: unit type \"%s\" not found\n",name);
    return -1;
}

static const cst_cart *clunit_get_tree(cst_clunit_db *cludb, const char *name)
{
    int i;

    i = clunit_get_unit_type_index(cludb, name);
    if (i == -1)
    {
	cst_errmsg("clunits: can't find tree for %s\n",name);
	i = 0; /* "graceful" failure */
    }
    return cludb->trees[i];
}

static void clunit_set_unit_name(cst_item *s,cst_clunit_db *clunit_db)
{
    if (clunit_db->unit_name_func)
    {
	char *cname;
	cname = (clunit_db->unit_name_func)(s);
	item_set_string(s,"clunit_name",cname);
	cst_free(cname);
    }
    else
    {
	/* is just the name by default */
	item_set(s,"clunit_name",item_feat(s,"name"));
    }

}

char *clunits_ldom_phone_word(cst_item *s)
{
    const char *name;
    const char *pname;
    const char *wname;
    const char *silence;
    char *clname;
    char *dname, *p, *q;

    silence = val_string(feat_val(item_utt(s)->features,"silence"));
    name = item_name(s);
    if (cst_streq(name,silence))
    {
	pname = ffeature_string(s,"p.name");
	clname = cst_alloc(char, cst_strlen(silence)+1+cst_strlen(pname)+1);
	cst_sprintf(clname,"%s_%s",silence,pname);
    }
    else
    {
	/* remove single quotes from name */
	wname = ffeature_string(s,"R:SylStructure.parent.parent.name");
	dname = cst_downcase(wname);
	for (q=p=dname; *p != '\0'; p++)
	    if (*p != '\'') *p = *q++;
	*q = '\0';
	clname = cst_alloc(char, cst_strlen(name)+1+cst_strlen(dname)+1);
	cst_sprintf(clname,"%s_%s",name,dname);
	cst_free(dname);
    }
    return clname;
}

int clunit_get_unit_index(cst_clunit_db *cludb,
			  const char *unit_type,
			  int instance)
{
    int i;

    i = clunit_get_unit_type_index(cludb, unit_type);
    if (i == -1)
    {
	/* TODO: fall back to closest possible match */
	cst_errmsg("clunit_get_unit_index: can't find unit type %s, using 0\n",
		   unit_type);
	i = 0;
    }
    if (instance >= cludb->types[i].count)
    {
	cst_errmsg("clunit_get_unit_index: can't find instance %d of %s, using 0\n",
		   instance, unit_type);
	instance = 0;
    }

    return cludb->types[i].start + instance;
}

int clunit_get_unit_index_name(cst_clunit_db *cludb,
			       const char *name)
{
    const char *c;
    char *type;
    int idx, i;

    c = cst_strrchr(name, '_');
    if (c == NULL)
    {
	cst_errmsg("clunit_get_unit_index_name: invalid unit name %s\n", name);
	return -1;
    }
    idx = atoi(c+1);
    type = cst_substr(name, 0, c - name);
    i = clunit_get_unit_index(cludb, type, idx);
    cst_free(type);

    return i;
}
