/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                         Copyright (c) 2008                            */
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
/*               Date:  May 2008                                         */
/*************************************************************************/
/*                                                                       */
/*  To avoid an initialization phase where regexes need to be set up we  */
/*  precompile them into statics that can be used directly --            */
/*  (dhd championed this technique, and this is a reimplementation of    */
/*  his compilation technique this)                                      */
/*                                                                       */
/*************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "flite.h"

static void compregex_usage()
{
    printf("compregex: compile regexes into C structures\n");
    printf("usage: compregex name regex\n"
           "  Compiles regex into a C structure called name\n");
    exit(0);
}

static void regex_to_C(const char *name, const cst_regex *rgx)
{
    int i;

    printf("static const unsigned char %s_rxprog[] = {\n   ",name);
    for (i=0; i<rgx->regsize; i++)
    {
        printf("%d, ", (unsigned char)rgx->program[i]);
        if (i%16 == 15)
            printf("\n   ");
    }
    printf("\n};\n");
    printf("static const cst_regex %s_rx = {\n   ",name);
    printf("%d, ",rgx->regstart);
    printf("%d, ",rgx->reganch);
    if (rgx->regmust == NULL)
        printf("NULL, ");
    else
        printf("%s_rxprog + %d, ", name, rgx->regmust - rgx->program);
    printf("%d, ",rgx->regmlen);
    printf("%d,\n   ",rgx->regsize);
    printf("(char *)%s_rxprog\n",name);
    printf("};\n");

    printf("const cst_regex * const %s = &%s_rx;\n\n",name, name);
}

int main(int argc, char **argv)
{
    cst_regex *rgx;

    if (argc != 3)
        compregex_usage();

    rgx = new_cst_regex(argv[2]);
    regex_to_C(argv[1],rgx);
    delete_cst_regex(rgx);
    
    return 0;
}
