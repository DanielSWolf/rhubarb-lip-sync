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
/*  Regexes, this is just a front end to Henry Spencer's regex code      */
/*  Includes a mapping of fsf format regex's to hs format (escaping)     */
/*                                                                       */
/*************************************************************************/
#include "cst_alloc.h"
#include "cst_regex.h"

#include "cst_regex_defs.h"

/* For access by const models */
const cst_regex *const cst_regex_table[] = {
	&cst_rx_dotted_abbrev_rx
};

static char *regularize(const char *unregex,int match);

void cst_regex_init()
{
    /* no need to initialize regexes anymore, they are pre-compiled */

    return;
}

int cst_regex_match(const cst_regex *r, const char *str)
{
    cst_regstate *s;

    if (r == NULL) return 0;
    s = hs_regexec(r, str);
    if (s) {
	cst_free(s);
	return 1;
    } else
	return 0;
}

cst_regstate *cst_regex_match_return(const cst_regex *r, const char *str)
{
    if (r == NULL) 
        return 0;

    return hs_regexec(r, str);
}

cst_regex *new_cst_regex(const char *str)
{
    cst_regex *r;
    char *reg_str = regularize(str,1);

    r = hs_regcomp(reg_str);
    cst_free(reg_str);

    return r;
}

void delete_cst_regex(cst_regex *r)
{
    if (r)
	hs_regdelete(r);

    return;
}

/* These define the different escape conventions for the FSF's */
/* regexp code and Henry Spencer's */

static const char *fsf_magic="^$*+?[].\\";
static const char *fsf_magic_backslashed="()|<>";
static const char *spencer_magic="^$*+?[].()|\\\n";
static const char *spencer_magic_backslashed="<>";

/* Adaptation of rjc's mapping of fsf format to henry spencer's format */
/* of escape sequences, as taken from EST_Regex.cc in EST              */
static char *regularize(const char *unregex,int match)
{
    char *reg = cst_alloc(char, cst_strlen(unregex)*2+3);
    char *r=reg;
    const char *e;
    int magic=0,last_was_bs=0;
    const char * in_brackets=NULL;
    const char *ex = (unregex?unregex:"");

    if (match && *ex != '^')
	*(r++) = '^';

    for(e=ex; *e ; e++)
    {
	if (*e == '\\' && !last_was_bs)
	{
	    last_was_bs=1;
	    continue;
	}

	magic=strchr((last_was_bs?fsf_magic_backslashed:fsf_magic), *e)!=NULL;

	if (in_brackets)
	{
	    *(r++) = *e;
	    if (*e  == ']' && (e-in_brackets)>1)
		in_brackets=0;
	}
	else if (magic)
	{
	    if (strchr(spencer_magic_backslashed, *e))
		*(r++) = '\\';

	    *(r++) = *e;
	    if (*e  == '[')
		in_brackets=e;
	}
	else 
	{
	    if (strchr(spencer_magic, *e))
		*(r++) = '\\';

	    *(r++) = *e;
	}
	last_was_bs=0;
    }
  
    if (match && (e==ex || *(e-1) != '$'))
    {
	if (last_was_bs)
	    *(r++) = '\\';
	*(r++) = '$';
    }

    *r='\0';

    return reg;
}
