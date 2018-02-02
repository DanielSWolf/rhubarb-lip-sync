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
/*  Resynthesize from lpc coefficients plus a residual                   */
/*                                                                       */
/*************************************************************************/
#include <stdio.h>
#include "cst_wave.h"
#include "cst_track.h"
#include "cst_sigpr.h"

int main(int argc, char **argv)
{
    /* need that cool argument parser */
    cst_wave *w,*res;
    cst_track *lpc;
    cst_lpcres *lpcres;
    int i,j;
    float lpcmin = -2.709040;
    float lpcrange = 2.328840+2.709040;
    unsigned short *bbb;

    if (argc != 4)
    {
	fprintf(stderr,"usage: lpc_test LPC RESIDUAL WAVEFILE\n");
	return 1;
    }

    lpc = new_track();
    cst_track_load_est(lpc,argv[1]);
    res = new_wave();
    cst_wave_load_riff(res,argv[2]);

    lpcres = new_lpcres();
    lpcres_resize_frames(lpcres,lpc->num_frames);
    lpcres->num_channels = lpc->num_channels-1;
    for (i=0; i<lpc->num_frames; i++)
    {
	bbb = cst_alloc(unsigned short,lpc->num_channels-1);
	for (j=1; j<lpc->num_channels; j++)
	    bbb[j-1] = (unsigned short)
		(((lpc->frames[i][j]-lpcmin)/lpcrange)*65535);
	lpcres->frames[i] = bbb;
	lpcres->sizes[i] = track_frame_shift(lpc,i)*res->sample_rate;
    }
    lpcres_resize_samples(lpcres,res->num_samples);
    lpcres->lpc_min = lpcmin;
    lpcres->lpc_range = lpcrange;
    lpcres->sample_rate = res->sample_rate;
    for (i=0; i<res->num_samples; i++)
	lpcres->residual[i] = cst_short_to_ulaw(res->samples[i]);
    
    w = lpc_resynth(lpcres);

    cst_wave_save_riff(w,argv[3]);

    return 0;
}
