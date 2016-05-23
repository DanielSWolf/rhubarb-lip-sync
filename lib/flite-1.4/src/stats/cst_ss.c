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
/*  sufficient statitsics                                                */
/*                                                                       */
/*************************************************************************/

#include "cst_alloc.h"
#include "cst_ss.h"
#include "cst_math.h"

cst_ss *new_ss()
{
    cst_ss *ss = cst_alloc(cst_ss,1);
    return ss;
}

void delete_ss(cst_ss *ss)
{
    cst_free(ss);
}

void ss_reset(cst_ss *ss)
{
    ss->num_samples = 0;
    ss->sum = 0;
    ss->sumx = 0;
}

double ss_mean(cst_ss *ss)
{
    if (ss->num_samples > 0)
	return ss->sum/ss->num_samples;
    else
	return 0;
}

double ss_variance(cst_ss *ss)
{
    if (ss->num_samples > 1)
	return ((ss->num_samples*ss->sumx)-(ss->sum*ss->sum))/
	    (ss->num_samples*(ss->num_samples-1));
    else
	return 0;
}

double ss_stddev(cst_ss *ss)
{
    return sqrt(ss_variance(ss));
}

void ss_cummulate(cst_ss *ss,double a)
{
    ss->sum += a;
    ss->sumx += a*a;
    ss->num_samples++;
}

void ss_cummulate_n(cst_ss *ss,double a, double count)
{
    ss->sum += a*count;
    ss->sumx += (a*a)*count;
    ss->num_samples += count;
}

