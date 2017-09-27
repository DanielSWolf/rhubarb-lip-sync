/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 2004                             */
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
/*               Date:  December 2004                                    */
/*************************************************************************/
/*                                                                       */
/*  PalmOS Callbacks to System Functions                                 */
/*                                                                       */
/*************************************************************************/
#include <PalmOS.h>
#include <PceNativeCall.h>
#include <CoreTraps.h>
#include <StringMgr.h>
#include "pocore.h"
#include "stdarg.h"

Int16
StrVPrintF( Char* s, const Char* format, _Palm_va_list args )
{
    Int16 r;
    unsigned char stack[12];
    unsigned void *arm_args = (void *)args;
    unsigned char *m68k_args;
    int m68k_args_size = 0;
    int m68k_p, arm_p;

    *(int *)&stack[0] = SWAPINT(s);
    *(int *)&stack[4] = SWAPINT(format);

    m68k_args = cst_alloc(unsigned char,24);
    m68k_args_size = 24;
    m68k_p = 0;
    arm_p = 0;

    for (i=0; format[i]; i++)
    {
	if (format[i] == '%')
	{
	    i++;
	    if (format[i] == '%')
		continue;
	    else if ((format[i] == 's') ||
		     (format[i] == 'l') ||
		     (format[i] == 'f'))
	    {   /* a four byte item */
		*(int *)&stack[m68k_p] = SWAPINT(arm_args[arm_p]);
		m68k_p += 4; arm_p++;
	    }
	    else if ((format[i] == '.'))
	    {   /* a two item item -- assumes a string */
		*(unsigned short *)&stack[m68k_p] = SWAPSHORT(arm_args[arm_p]);
		m68k_p += 2; arm_p++;
		*(int *)&stack[m68k_p] = SWAPINT(arm_args[arm_p]);
		m68k_p += 4; arm_p++;
	    }
	    else if ((format[i] == 'c'))
	    {   /* a one byte item */
		*(unsigned char *)&stack[m68k_p] = arm_args[arm_p]; /* wrong */
		m68k_p += 1; arm_p++;
		p += 1;
	    }
	    else if ((format[i] == 'd'))
	    {   /* a two byte item */
		*(unsigned short *)&stack[m68k_p] = SWAPSHORT(arm_args[arm_p]);
		m68k_p += 2; arm_p++;
	    }
	}
    }

    /* Need to unpack the arm args and make the m68k args */
    *(int *)&stack[8] = SWAPINT(args);

    r = (Int16)(*gCall68KFuncP)(gEmulStateP,
				  PceNativeTrapNo(sysTrapStrVPrintF),
				  stack,
				  12 | kPceNativeWantA0);

    cst_free(m68k_args);
    return r;
}





