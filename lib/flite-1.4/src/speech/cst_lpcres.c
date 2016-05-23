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
/*               Date:  August 2000                                      */
/*************************************************************************/
/*                                                                       */
/*  Waveforms                                                            */
/*                                                                       */
/*************************************************************************/
#include "cst_string.h"
#include "cst_val.h"
#include "cst_sts.h"

CST_VAL_REGISTER_TYPE(lpcres,cst_lpcres)

cst_lpcres *new_lpcres()
{
    cst_lpcres *l = cst_alloc(struct cst_lpcres_struct,1);
    return l;
}

void delete_lpcres(cst_lpcres *l)
{
    if (l)
    {
	cst_free(l->times);
	cst_free((unsigned short **)l->frames);
	cst_free(l->residual);
	cst_free(l->sizes);
	cst_free(l);
    }
    return;
}

float lpcres_frame_shift(cst_lpcres *t, int frame)
{
    if (frame == 0)
	return (float) t->times[frame];
    else
	return (float) t->times[frame]-t->times[frame-1];
}

void lpcres_resize_frames(cst_lpcres *l,int num_frames)
{
    l->times = cst_alloc(int,num_frames);
    l->frames = cst_alloc(const unsigned short*,num_frames);
    l->sizes = cst_alloc(int,num_frames);
    l->num_frames = num_frames;
}

void lpcres_resize_samples(cst_lpcres *l,int num_samples)
{
    l->residual = cst_alloc(unsigned char,num_samples);
    /* mulaw for 0 is 255 */
    memset(l->residual,255,num_samples);
    l->num_samples = num_samples;

}
