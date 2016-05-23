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
/*  Test of hrg creation and manipulation                                */
/*                                                                       */
/*************************************************************************/
#include <stdio.h>
#include "cst_hrg.h"

int main(int argc, char **argv)
{
    cst_utterance *u;
    cst_relation *r;
    cst_item *item=0;
    int i;

    u = new_utterance();
    r = utt_relation_create(u,"Segment");

    for (i=0; i<10; i++)
    {
	char buff[20];
	sprintf(buff,"seg_%03d",i);
	if (i==0)
	    item = relation_append(r,NULL);
	else
	    item = item_append(item,NULL);
	item_set_string(item,"name",buff);
	item_set_float(item,"duration",i*0.20);
    }

    for (i=0,item=relation_head(utt_relation(u,"Segment")); 
	 item; item=item_next(item),i++)
    {
	printf("Segment %d %s %f\n",
	       i,
	       item_feat_string(item,"name"),
	       item_feat_float(item,"duration"));
    }

    delete_utterance(u);

    return 0;
}
