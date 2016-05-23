/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 2001                             */
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
/*  Unix sort is different in non-obvious ways so we use the actual      */
/*  strcmp function that will be used to index the units to do the sort  */
/*                                                                       */
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "cst_val.h"
#include "cst_tokenstream.h"

int flite_strcmp(const void *a, const void *b)
{
    const cst_val **v1;
    const cst_val **v2;

    v1 = (const cst_val **)a;
    v2 = (const cst_val **)b;
    return strcmp(val_string(val_car(*v1)),val_string(val_car(*v2)));
}

int main(int argc, char **argv)
{
    cst_tokenstream *ts;
    cst_val *f,*g;
    const cst_val *ff;
    const char *token;
    int s,i;
    const cst_val **ll;

    ts = ts_open("-",NULL,"()","","");

    f = NULL;
    s = 0;
    while (!ts_eof(ts))
    {
	g = NULL;	
	for(token = ts_get(ts);
	    !cst_streq(token,")");
	    token = ts_get(ts))
	{
	    g = cons_val(string_val(token),g);
	    if (ts_eof(ts))
		break;
	}
	if (!ts_eof(ts))
	{
	    g = cons_val(string_val(")"),g);
	    f = cons_val(val_reverse(g),f);
	    s++;
	}
    }

    ll = cst_alloc(const cst_val *,s);
    for (i=0,ff=f; ff; ff=val_cdr(ff),i+=1)
	ll[i] = val_car(ff);

    qsort(ll,s,sizeof(cst_val *),flite_strcmp);

    for (i=0; i < s; i++)
    {
	for (ff=ll[i]; ff; ff=val_cdr(ff))
	    printf("%s ",val_string(val_car(ff)));
	printf("\n");
    }

    return 0;
}
