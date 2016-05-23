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
/*               Date:  November 2001                                    */
/*************************************************************************/
/*                                                                       */
/*  reflection coefficients, algorithms from Festival                    */
/*                                                                       */
/*************************************************************************/

#include "cst_alloc.h"

void lpc2ref(const float *lpc, float *rfc, int order)
{
    /* LPC to reflection coefficients, converted from Festival */
    int i,j;
    float f,ai;
    float *vo,*vx;
    float *vn = cst_alloc(float,order);

    i = order-1;
    rfc[i] = ai = lpc[i];
    f = 1-ai*ai;
    i--;

    for (j=0; j<=i; j++)
        rfc[j] = (lpc[j]+((ai*lpc[i-j])))/f;

    vo=rfc;

    for ( ;i>0; )
    {
        ai=vo[i];
        f = 1-ai*ai;
        i--;
	for (j=0; j<=i; j++)
            vn[j] = (vo[j]+((ai*vo[i-j])))/f;

        rfc[i]=vn[i];

        vx = vn;
        vn = vo;
        vo = vx;
    }

    cst_free(vn);

}

void ref2lpc(const float *rfc, float *lpc, int order)
{
    // Here we use Christopher Longet Higgin's algorithm converted to
    // an equivalent by awb.  Its doesn't have the reverse order or
    // negation requirement.
    float a,b;
    int n,k;

    for (n=0; n < order; n++)
    {
        lpc[n] = rfc[n];
        for (k=0; 2*(k+1) <= n+1; k++)
        {
            a = lpc[k];
            b = lpc[n-(k+1)];
            lpc[k] = a-b*lpc[n];
            lpc[n-(k+1)] = b-a*lpc[n];
        }
    }

}

