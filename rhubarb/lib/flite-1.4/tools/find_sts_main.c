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
/*               Date:  June 2001                                        */
/*************************************************************************/
/*                                                                       */
/*  residuals etc                                                        */
/*                                                                       */
/*************************************************************************/
#include <stdio.h>
#include <math.h>
#include <string.h>

/* To allow some normally const fields to manipulated during building */
#define const

#include "cst_wave.h"
#include "cst_track.h"
#include "cst_sigpr.h"

float lpc_min;
float lpc_range;

void inv_lpc_filterd(short *sig, float *a, int order, double *res, int size)
{
    /* Note this address bytes before sig, up to order shorts back */
    int i, j;
    double r;

    for (i = 0; i < size; i++)
    {
	r = sig[i];
	for (j = 1; j < order; j++)
	    r -= a[j] * (double)sig[i - j];
	res[i] = r;
    }
}

cst_sts *find_sts(cst_wave *sig, cst_track *lpc)
{
    cst_sts *sts;
    int i,j;
    double *resd;
    int size,start,end;
    short *sigplus;

    sts = cst_alloc(cst_sts,lpc->num_frames);
    sigplus = cst_alloc(short,sig->num_samples+lpc->num_channels);
    memset(sigplus,0,sizeof(short)*lpc->num_channels);
    memmove(&sigplus[lpc->num_channels],
	    sig->samples,
	    sizeof(short)*sig->num_samples);
    /* EST LPC Windows are centered around the point */
    /* so offset things by a half period */
    start = (int)((float)sig->sample_rate * lpc->times[0]/2);
    for (i=0; i<lpc->num_frames; i++)
    {
	if (i+1 == lpc->num_frames)
	    end = (int)((float)sig->sample_rate * lpc->times[i]);
	else
	    end = (int)((float)sig->sample_rate *
			(lpc->times[i]+lpc->times[i+1]))/2;
	size = end - start;
	if (size == 0)
	    printf("frame size at %f is 0\n",lpc->times[i]);
	resd = cst_alloc(double,size);
	
	inv_lpc_filterd(&sigplus[start+lpc->num_channels],
			lpc->frames[i],lpc->num_channels,
			resd,
			size);
	sts[i].size = size;
	sts[i].frame = cst_alloc(unsigned short,lpc->num_channels-1);
	for (j=1; j < lpc->num_channels; j++)
	    sts[i].frame[j-1] = (unsigned short)
		(((lpc->frames[i][j]-lpc_min)/lpc_range)*65535);
	sts[i].residual = cst_alloc(unsigned char,size);
	for (j=0; j < size; j++)
	    sts[i].residual[j] = cst_short_to_ulaw((short)resd[j]);
	start = end;
    }

    cst_free(sigplus);
    return sts;
}

cst_wave *reconstruct_wave(cst_wave *sig, cst_sts *sts, cst_track *lpc)
{
    cst_lpcres *lpcres;
    int i,j,r;
    int start;
    int num_samples;
/*    FILE *ofd; */

    for (num_samples = 0, i=0; i < lpc->num_frames; i++)
	num_samples += sts[i].size;

    lpcres = new_lpcres();
    lpcres_resize_frames(lpcres,lpc->num_frames);
    lpcres->num_channels = lpc->num_channels-1;
    start = (int)((float)sig->sample_rate * lpc->times[0]/2);
    num_samples += start;
    for (i=0; i<lpc->num_frames; i++)
    {
	lpcres->frames[i] = sts[i].frame;
	lpcres->sizes[i] = sts[i].size;
    }
    lpcres_resize_samples(lpcres,num_samples);
    lpcres->lpc_min = lpc_min;
    lpcres->lpc_range = lpc_range;
    lpcres->sample_rate = sig->sample_rate;
    for (r=start,i=0; i<lpc->num_frames; i++)
	for (j=0; j<sts[i].size; j++,r++)
	    lpcres->residual[r] = sts[i].residual[j];

#if 0
    /* Debug dump */
    ofd = fopen("lpc_resid.lpc","w");
    for (s=0,i=0; i<lpcres->num_frames; i++)
    {
	fprintf(ofd,"%d %d %d\n",i,0,lpcres->sizes[i]);
	for (j=0; j < lpcres->num_channels; j++)
	    fprintf(ofd,"%d ",lpcres->frames[i][j]);
	fprintf(ofd,"\n");
	for (j=0; j < lpcres->sizes[i]; j++,s++)
	    fprintf(ofd,"%d ",lpcres->residual[s]);
	fprintf(ofd,"\n");
    }
    fclose(ofd);
    ofd = fopen("lpc_resid.res","w");
    for (i=0; i < r; i++)
	fprintf(ofd,"%d\n",cst_ulaw_to_short(lpcres->residual[i]));
    fclose(ofd);
#endif

    return lpc_resynth_fixedpoint(lpcres);
}

void compare_waves(cst_wave *a, cst_wave *b)
{
    int i;
    double r;

    if (a->num_samples != b->num_samples)
    {
	if (a->num_samples > b->num_samples)
	{
	    compare_waves(b,a);
	    return;
	}
    }
    
    for (r=0.0,i=0; i<a->num_samples; i++)
	r += ((float)a->samples[i]-(float)b->samples[i]) *
	    ((float)a->samples[i]-(float)b->samples[i]);
    r /= a->num_samples;
    printf("a/b diff %f\n",sqrt(r));
}

void save_sts(cst_sts *sts, cst_track *lpc, cst_wave *sig, const char *fn)
{
    FILE *fd;
    int i,j,m;

    if ((fd=fopen(fn,"w"))== NULL)
    {
	fprintf(stderr,"can't open for writing file: %s\n", fn);
	exit(-1);
    }

    fprintf(fd,"( %d %d %d %f %f)\n", lpc->num_frames, 
	    lpc->num_channels-1, sig->sample_rate,
	    lpc_min, lpc_range);
    for (m=i=0; i<lpc->num_frames; i++)
    {
	fprintf(fd,"( %f (",lpc->times[i]);
	for (j=1; j < lpc->num_channels; j++)
	    fprintf(fd," %d",sts[i].frame[j-1]);
	fprintf(fd," ) %d ( ", sts[i].size);
	for (j=0; j < sts[i].size; j++)
	    fprintf(fd," %d",sts[i].residual[j]);
	fprintf(fd," ))\n");
    }
    
    fclose(fd);
}

int main(int argc, char **argv)
{
    cst_track *lpc;
    cst_wave *sig, *sig2;
    cst_sts *sts;

    if (argc != 6)
    {
	fprintf(stderr,"usage: find_sts lpc_min lpc_range LPC WAVEFILE STS\n");
	return 1;
    }

    lpc_min = atof(argv[1]);
    lpc_range = atof(argv[2]);

    lpc = new_track();
    cst_track_load_est(lpc,argv[3]);
    sig = new_wave();
    if (cst_wave_load_riff(sig,argv[4]) == CST_WRONG_FORMAT)
    {
	fprintf(stderr,
		"cannot load waveform, format unrecognized, from \"%s\"\n",
		argv[4]);
	exit(-1);
    }

    sts = find_sts(sig,lpc);

    /* See if it worked */
    sig2 = reconstruct_wave(sig,sts,lpc);

    compare_waves(sig,sig2);
    cst_wave_save_riff(sig2,"sig2.wav");

    save_sts(sts,lpc,sig,argv[5]);

    return 0;
}
