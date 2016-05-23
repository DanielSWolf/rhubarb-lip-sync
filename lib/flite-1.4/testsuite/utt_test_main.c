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
/*               Date:  April 2000                                       */
/*************************************************************************/
/*                                                                       */
/*  Read some data, build an utt, look up words and extract some feats   */
/*  Actually doing osmething here                                        */
/*                                                                       */
/*                                                                       */
/*************************************************************************/
#include <stdio.h>
#include "cst_hrg.h"
#include "cst_tokenstream.h"
#include "cst_lexicon.h"

extern cst_lexicon cmu_lex;
void cmu_lex_init();

static int bbb_relation_load(cst_relation *r,const char *filename);
static int WordSylSeg(cst_utterance *u);

int main(int argc, char **argv)
{
    cst_utterance *u;
    cst_relation *r;
    cst_item *item=0;
    int i;

    cmu_lex_init();

    u = new_utterance();
    r = utt_relation_create(u,"Word");

    bbb_relation_load(r,"ttt.txt");

    WordSylSeg(u);

    for (i=0,item=item_next(relation_head(utt_relation(u,"Segment"))); 
	 item; item=item_next(item),i++)
    {
	printf("Segment %s %s %s %s\n",
	       ffeature_string(item,"name"),
	       ffeature_string(item,"n.name"),
	       ffeature_string(item,"p.name"),
	       ffeature_string(item,"R:SylStructure.parent.name")
/*	       ffeature_string(item,"R:SylStructure.parent.R:Word.n.name"), */
/*	       item_feat_float(item,"duration")); */
	       );
    }

    delete_utterance(u);

    return 0;
}

static int WordSylSeg(cst_utterance *u)
{
    cst_item *word;
    cst_relation *sylstructure,*seg,*syl;
    cst_val *phones;
    const cst_val *p;
    cst_item *ssword,*segitem;
    
    syl = utt_relation_create(u,"Syllable");
    sylstructure = utt_relation_create(u,"SylStructure");
    seg = utt_relation_create(u,"Segment");
    
    for (word=relation_head(utt_relation(u,"Word")); 
	 word; word=item_next(word))
    {
	printf("word: %s\n",item_feat_string(word,"name"));
	ssword = relation_append(sylstructure,word);
	phones = lex_lookup((cst_lexicon *)&cmu_lex,item_feat_string(word,"name"),0);
	for (p=phones; p; p=val_cdr(p))
	{
	    segitem = relation_append(seg,NULL);
	    item_set(segitem,"name",val_car(p));
	    printf("seg: %s\n",item_feat_string(segitem,"name"));
	    item_add_daughter(ssword,segitem);
	}
	delete_val_list(phones);
    }

    return TRUE;

}

static int bbb_relation_load(cst_relation *r,const char *filename)
{
    const char *token;
    cst_item *item;
    cst_tokenstream *fd;

    fd = ts_open(filename);
    if (fd == 0)
	return 0;

    while (!ts_eof(fd))
    {
	token = ts_get(fd);
	if (cst_streq(token,""))
	    continue;
	item = relation_append(r,NULL);
	item_set_string(item,"name",token);
	item_set_string(item,"whitespace",fd->whitespace);
	item_set_string(item,"prepunctuation",fd->prepunctuation);
	item_set_string(item,"punc",fd->postpunctuation);
	item_set_int(item,"file_pos",fd->file_pos);
	item_set_int(item,"line_number",fd->line_number);
    }
    
    ts_close(fd);

    return 1;
}



