/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 2002                             */
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
/*               Date:  August 2002                                      */
/*************************************************************************/
/*                                                                       */
/*  Letter to sound rule for Festival's hand writable rule set           */
/*                                                                       */
/*  Rules are ordered list of                                            */
/*     LC [ A ] RC => B                                                  */
/*                                                                       */
/*************************************************************************/

#include "cst_string.h"
#include "cst_val.h"
#include "cst_lts_rewrites.h"

static int item_match(const cst_val *PATT, const cst_val *THING, 
		      const cst_val *sets)
{
    const cst_val *sss;

    if (cst_streq(val_string(PATT),val_string(THING)))
	return TRUE;

    for (sss=sets; sss; sss=val_cdr(sss))
    {
	if (cst_streq(val_string(val_car(val_car(sss))),val_string(PATT)))
	{   /* Its a set not a letter */
	    if (val_member_string(val_string(THING),val_cdr(val_car(sss))))
	    {
		return TRUE;
	    }
	    else
	    {
		return FALSE;
	    }
	}
    }
    return FALSE;
}

static int context_match(const cst_val *PATTERN, const cst_val *STRING,
			 const cst_val *sets)
{
    int r,s,t;
/*    printf("PATTERN: "); val_print(stdout, PATTERN); printf("\n"); */
/*    printf("STRING: "); val_print(stdout, STRING); printf("\n"); */
    if (!PATTERN)
	r = TRUE;
    else if (!STRING)
	r = FALSE;
    else if (val_cdr(PATTERN) &&
	     (cst_streq("*",val_string(val_car(PATTERN)))))
    {
	r = context_match(val_cdr(val_cdr(PATTERN)),STRING,sets);
	s = context_match(val_cdr(PATTERN),STRING,sets);
	t = item_match(val_car(val_cdr(PATTERN)),val_car(STRING),sets) && 
	    context_match(PATTERN, val_cdr(STRING),sets);
	r = r || s || t;
    }
#if 0
    else if (val_cdr(PATTERN) &&
	     (cst_streq("+",val_string(val_car(PATTERN)))))
	return context_match(val_cdr(PATTERN),STRING,sets) || /* last match */
	    (item_match(val_car(val_cdr(PATTERN)),val_car(STRING),sets) &&
	    context_match(val_cdr(val_cdr(PATTERN)),  /* loop match */
			  val_cdr(STRING),sets));
#endif
    else if (item_match(val_car(PATTERN),val_car(STRING),sets))
	r = context_match(val_cdr(PATTERN),val_cdr(STRING),sets);
    else
	r = FALSE;
/*    printf("R = %s\n",(r ? "TRUE" : "FALSE")); */
    return r;
}

static int rule_matches(const cst_val *LC, const cst_val *RC, 
			const cst_val *RLC, const cst_val *RA, 
			const cst_val *RRC,
			const cst_val *sets)
{
    const cst_val *rc, *ra;

    /* Check [ X ] bit */
    for (rc=RC,ra=RA; ra; ra=val_cdr(ra),rc=val_cdr(rc))
    {
	if (!rc) return FALSE;
	if (!cst_streq(val_string(val_car(ra)),
		       val_string(val_car(rc))))
	    return FALSE;
    }

    /* Check LC bit: LC may have some limited regex stuff  */
    if (context_match(RLC,LC,sets) && context_match(RRC,rc,sets))
	return TRUE;
    else
	return FALSE;
}

static const cst_val *find_rewrite_rule(const cst_val *LC,
					const cst_val *RC, 
					const cst_lts_rewrites *r)
{
    /* Search through rewrite rules to find matching one */
    const cst_val *i, *RLC, *RA, *RRC;
    
    for (i=r->rules; i; i=val_cdr(i))
    {
/*	val_print(stdout, val_car(i));	printf("\n"); */
	RLC = val_car(val_car(i));
	RA = val_car(val_cdr(val_car(i)));
	RRC = val_car(val_cdr(val_cdr(val_car(i))));
	if (rule_matches(LC,RC,RLC,RA,RRC,r->sets))
	    return val_car(i);
    }

#if 0
    fprintf(stderr,"LTS_REWRITES: unable to find a matching rules for:\n");
    fprintf(stderr,"CL: ");
    val_print(stderr,LC);
    fprintf(stderr,"\n");
    fprintf(stderr,"RC: ");
    val_print(stderr,RC);
    fprintf(stderr,"\n");
#endif

    return NULL;
}

cst_val *lts_rewrites_word(const char *word, const cst_lts_rewrites *r)
{
    cst_val *w, *p;
    char x[2];
    int i;

    x[1] = '\0';
    w = cons_val(string_val("#"),NULL);
    for (i=0; word[i]; i++)
    {
	x[0] = word[i];
	w = cons_val(string_val(x),w);
    }
    w = cons_val(string_val("#"),w);

    w = val_reverse(w);

    p = lts_rewrites(w,r);
    
    delete_val(w);

    return p;
}

cst_val *lts_rewrites(const cst_val *itape, const cst_lts_rewrites *r)
{
    /* Returns list of rewritten "letters" to "phones" by r */
    cst_val *LC;
    const cst_val *RC, *i;
    const cst_val *rule;
    cst_val *otape;

    LC = cons_val(val_car(itape),NULL);
    RC = val_cdr(itape);
    otape = NULL;

    while (val_cdr(RC))
    {
	rule = find_rewrite_rule(LC,RC,r);

	if (!rule)
	    break;
/*	val_print(stdout,rule);
	printf("\n"); */

	/* Shift itape head */
	for (i=val_car(val_cdr(rule)); i; i=val_cdr(i))
	{
	    LC = cons_val(val_car(RC),LC);
	    RC = val_cdr(RC);
	}
	
	/* Output things to otape */
	for (i=val_car(val_cdr(val_cdr(val_cdr(rule)))); i; i=val_cdr(i))
	    otape = cons_val(val_car(i),otape);
    }

    delete_val(LC);

    return val_reverse(otape);
}

