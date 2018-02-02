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
/*               Date:  January 2000                                     */
/*************************************************************************/
/*                                                                       */
/*  Various regex tests                                                  */
/*                                                                       */
/*************************************************************************/
#include <stdio.h>
#include "cst_regex.h"

char *rtests[] = {
    "1",
    " \n ",
    "hello",
    "Hello",
    "1and2",
    "oneandtwo",
    "-1.34",
    "235",
    "034",
    "1,234,235",
    "1,2345",
    NULL };

int main(int argc, char **argv)
{
    int i;
    cst_regex *commaint;

    commaint = new_cst_regex("[0-9][0-9]?[0-9]?,\\([0-9][0-9][0-9],\\)*[0-9][0-9][0-9]\\(\\.[0-9]+\\)?");

    for (i=0; rtests[i]; i++)
    {
	printf("\"%s\"\n",rtests[i]);
	printf(" %.8s %c\n",
	       "white",(cst_regex_match(cst_rx_white,rtests[i]) ? 't' : 'f'));
	printf(" %.8s %c\n",
	       "alpha",(cst_regex_match(cst_rx_alpha,rtests[i]) ? 't' : 'f'));
	printf(" %.8s %c\n",
	       "upper",
	       (cst_regex_match(cst_rx_uppercase,rtests[i]) ? 't' : 'f'));
	printf(" %.8s %c\n",
	       "lower",
	       (cst_regex_match(cst_rx_lowercase,rtests[i]) ? 't' : 'f'));
	printf(" %.8s %c\n",
	       "alphanum",
	       (cst_regex_match(cst_rx_alphanum,rtests[i]) ? 't' : 'f'));
	printf(" %.8s %c\n",
	       "ident",
	       (cst_regex_match(cst_rx_identifier,rtests[i]) ? 't' : 'f'));
	printf(" %.8s %c\n",
	       "int",
	       (cst_regex_match(cst_rx_int,rtests[i]) ? 't' : 'f'));
	printf(" %.8s %c\n",
	       "double",
	       (cst_regex_match(cst_rx_double,rtests[i]) ? 't' : 'f'));
	printf(" %.8s %c\n",
	       "commaint",
	       (cst_regex_match(commaint,rtests[i]) ? 't' : 'f'));
    }
    
    delete_cst_regex(commaint);

    return 0;
}
