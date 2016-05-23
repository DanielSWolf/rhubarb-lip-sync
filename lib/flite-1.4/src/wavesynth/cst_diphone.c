/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                         Copyright (c) 2001                            */
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
/*               Date:  January 2001                                     */
/*************************************************************************/
/*                                                                       */
/*  Diphone specific functions                                           */
/*                                                                       */
/*************************************************************************/

#include "cst_math.h"
#include "cst_hrg.h"
#include "cst_utt_utils.h"
#include "cst_wave.h"
#include "cst_track.h"
#include "cst_diphone.h"
#include "cst_units.h"
#include "cst_sigpr.h"

CST_VAL_REGISTER_TYPE_NODEL(diphone_db,cst_diphone_db)

static int get_diphone_entry(cst_diphone_db *udb,const char *name);
static int get_diphone_entry_bsearch(const cst_diphone_entry *entries,
				     int start, int end,
				     const char *key);

cst_utterance* diphone_synth(cst_utterance *utt)
{
    get_diphone_units(utt);
    join_units(utt);
    return utt;
}

cst_utterance *get_diphone_units(cst_utterance *utt)
{
    /* Select units from db with times etc */
    cst_relation *units;
    cst_item *s0, *s1, *u;
    float end0,end1;
    char diphone_name[22];
    cst_diphone_db *udb;
    int unit_entry;

    udb = val_diphone_db(utt_feat_val(utt,"diphone_db"));
    utt_set_feat(utt,"sts_list",sts_list_val(udb->sts));

    units = utt_relation_create(utt,"Unit");

    for (s0=relation_head(utt_relation(utt,"Segment")); 
	 s0 && item_next(s0); s0=s1)
    {
	s1 = item_next(s0);
	cst_sprintf(diphone_name,
		    "%.10s-%.10s",
		    item_name(s0),
		    item_name(s1));

	unit_entry = get_diphone_entry(udb,diphone_name);

	if (unit_entry == -1)
	{
	    cst_errmsg("flite: udb failed to find entry for: %s\n",
		       diphone_name);
	    unit_entry = 0;
	}

	/* first half of diphone */
	u = relation_append(units,NULL);
	item_add_daughter(s0,u);
	item_set_string(u,"name",diphone_name);
	end0 = item_feat_float(s0,"end");
	item_set_int(u,"target_end", (int)(end0*udb->sts->sample_rate));
	item_set_int(u,"unit_entry",unit_entry);
	item_set_int(u,"unit_start",udb->diphones[unit_entry].start_pm);
	item_set_int(u,"unit_end",
		     udb->diphones[unit_entry].start_pm + 
		     udb->diphones[unit_entry].pb_pm);
	/* second half of diphone */
	u = relation_append(units,NULL);
	item_add_daughter(s1,u);
	item_set_string(u,"name",diphone_name);
	end1 = item_feat_float(s1,"end");
	item_set_int(u,"target_end",(int)(((end0+end1)/2.0)*udb->sts->sample_rate));
	item_set_int(u,"unit_entry",unit_entry);
	item_set_int(u,"unit_start",
		     udb->diphones[unit_entry].start_pm + 
		     udb->diphones[unit_entry].pb_pm);
	item_set_int(u,"unit_end",
		     udb->diphones[unit_entry].start_pm + 
		     udb->diphones[unit_entry].pb_pm+
		     udb->diphones[unit_entry].end_pm);
    }
    
    return utt;
}

static int get_diphone_entry(cst_diphone_db *udb, const char *name)
{
    return get_diphone_entry_bsearch(udb->diphones,0,
				     udb->num_entries,
				     name);
}

static int get_diphone_entry_bsearch(const cst_diphone_entry *entries,
				     int start, int end,
				     const char *key)
{
    int mid,c;

    while (start < end) 
    {
	mid = (start+end)/2;
	
	c = strcmp(entries[mid].name,key);

	if (c == 0)
	    return mid;
	else if (c > 0)
	    end = mid;
	else
	    start = mid + 1;
    }

    return -1;  /* can't find it */
}

