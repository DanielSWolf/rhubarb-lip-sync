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
/*   Error mechanism                                                     */
/*                                                                       */
/*************************************************************************/
#include <stdlib.h>
#include <stdarg.h>
#include "cst_file.h"
#include "cst_error.h"

#ifdef UNDER_CE

#include <winbase.h>
#include <stdlib.h>
#include "cst_alloc.h"

int cst_errmsg(const char *fmt, ...)
{
#ifdef DEBUG
    wchar_t wmsg[256];
    wchar_t *wfmt;
    size_t len;
    va_list args;

    len = mbstowcs(NULL,fmt,0) + 1;
    wfmt = cst_alloc(wchar_t,len);
    mbstowcs(wfmt,fmt,len);

    va_start(args,fmt);
    _vsnwprintf(wmsg,256,wfmt,args);
    va_end(args);

    wmsg[255]=L'\0';
    cst_free(wfmt);
    MessageBoxW(0,wmsg,L"Error",0);
#endif
    return 0;
}
jmp_buf *cst_errjmp = 0;

#elif defined(__palmos__)
#ifdef __ARM_ARCH_4T__
/* We only support throwing errors in ARM land */
jmp_buf *cst_errjmp = 0;
#endif

char cst_error_msg[600];
int cst_errmsg(const char *fmt, ...)
{
    va_list args;
    int count;

    va_start(args,fmt);
    count = cst_vsprintf(cst_error_msg,fmt,args);
    va_end(args);
    return 0;
}
#else

jmp_buf *cst_errjmp = 0;

int cst_errmsg(const char *fmt, ...)
{
    va_list args;
    int rv;

    va_start(args, fmt);
    rv = vfprintf(stderr, fmt, args);
    va_end(args);

    return rv;
}

#endif
