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
/*  Test of hrg creation and manipulation                                */
/*                                                                       */
/*************************************************************************/
#include <stdio.h>
#include "cst_lexicon.h"

extern cst_lexicon cmu_lex;
void cmu_lex_init();

static void lookup_and_print(cst_lexicon *l,const char *word,const char *feats)
{
    cst_val *p;

    p = lex_lookup(l,word,feats);
    val_print(stdout,p);
    printf("\n");
    delete_val(p);
}

int main(int argc, char **argv)
{
    char *word;
    char *pos = NULL;

    if (argc == 1)
    {
	fprintf(stderr,"usage: lex_lookup WORD [PartOfSpeech]\n");
	return 1;
    }
    else
	word=argv[1];

    if (argc > 2)
	pos=argv[2];

    cmu_lex_init();

    lookup_and_print(&cmu_lex,word,pos);
    
    return 0;
}
