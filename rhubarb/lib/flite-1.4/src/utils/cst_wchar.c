/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 2008                             */
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
/*               Date:  June 2008                                        */
/*************************************************************************/
/*                                                                       */
/*   wchar_t functions                                                     */
/*                                                                       */
/*************************************************************************/
#include "cst_wchar.h"
#include "cst_alloc.h"
#include "cst_string.h"

wchar_t *cst_cstr2wstr(const char *s)
{
    /* Actually this is naive and is really only for ASCII wchar_ts */
    int i,l;
    wchar_t *w;
    
    l=cst_strlen(s);
    w = cst_alloc(wchar_t,l+1);

    for (i=0; i<l; i++)
    {
        w[i] = (wchar_t)s[i];
    }
    w[i]=(wchar_t)'\0';
    
    return w;
}

char *cst_wstr2cstr(const wchar_t *w)
{
    int i,l;
    char *s;

    l=cst_wstrlen(w);
    s = cst_alloc(char,l+1);
    
    for (i=0; i<l; i++)
    {
        s[i] = (char)w[i];
    }
    s[i]=(wchar_t)'\0';
    
    return s;
}

