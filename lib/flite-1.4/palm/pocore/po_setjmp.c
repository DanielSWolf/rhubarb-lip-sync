/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 2005                             */
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
/*               Date:  February 2005                                    */
/*************************************************************************/
/*                                                                       */
/*  Implements setjmp/longjmp, which of course can't be done by calling  */
/*  back to m68k land as that has different registers                    */
/*                                                                       */
/*  From a message from Wolfgang Spraul pno list                         */
/*************************************************************************/
#include <PalmOS.h>
#include <string.h>
#include "PceNativeCall.h" 
#include "CoreTraps.h" 
#include "cst_error.h"
#include "pocore.h"

int setjmp(register jmp_buf env)
{
    asm("stmia a1,{v1-v8,sp,lr};"
	"mov a1,#0;" /* return 0 */
	"bx lr;");
    return 0;
}

void longjmp(register jmp_buf env, register int value)
{
    asm("ldmia a1,{v1-v8,sp,lr};"
	"movs a1,a2;"  /* return value as setjmp() result code */
	"moveq a1,#1;" /* return 1 instead of 0 */
	"bx lr;");
    return;
}




