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
#include "cst_wave.h"
#include "cst_track.h"
#include "cst_sigpr.h"


void inv_lpc_filterd(short *sig, float *a, int order, double *res, int size)
{
    int i, j;
    double r;
    for (i = 0; i < order; i++)
    {
	r = sig[i];
	for (j = 1; j < order; j++)
/*	    if (i-j >= 0) */
		r -= a[j] * (double)sig[i - j];
	res[i] = r;
    }
    for (i = order; i < size; i++)
    {
	r = sig[i];
	for (j = 1; j < order; j++)
	    r -= a[j] * (double)sig[i - j];
	res[i] = r;
    }
}

cst_wave *find_residual(cst_wave *sig, cst_track *lpc)
{
    cst_wave *res;
    int i;
    double *resd;
    int size,start,end;

    res = new_wave();
    cst_wave_resize(res,sig->num_samples,1);
    res->sample_rate = sig->sample_rate;
    resd = cst_alloc(double,sig->num_samples);

    start = lpc->num_channels;
    for (i=0; i<lpc->num_frames; i++)
    {
	end = (int)((float)sig->sample_rate * lpc->times[i]);
	size = end - start;
	
	inv_lpc_filterd(&sig->samples[start],lpc->frames[i],lpc->num_channels,
			&resd[start],size);
	
	start = end;
    }

    for (i=0; i<res->num_samples; i++)
	res->samples[i] = resd[i];

    cst_free(resd);

    return res;
}

void lpc_filterd(short *res, float *a, int order, double *sig, int size)
{
    int i, j;
    double r;
    for (i = 0; i < order; i++)
    {
	r = res[i];
	for (j = 1; j < order; j++)
/*	    if (i-j >= 0) */
		r += a[j] * (double)sig[i - j];
	sig[i] = r;
    }
    for (i = order; i < size; i++)
    {
	r = res[i];
	for (j = 1; j < order; j++)
	    r += a[j] * (double)sig[i - j];
	sig[i] = r;
    }
}

cst_wave *reconstruct_wave(cst_wave *res, cst_track *lpc)
{
    cst_wave *sig;
    int i;
    double *sigd;
    int start, end, size;

    sig = new_wave();
    cst_wave_resize(sig,res->num_samples,1);
    sig->sample_rate = res->sample_rate;
    sigd = cst_alloc(double,sig->num_samples);

    start = 0;
    for (i=0; i<lpc->num_frames; i++)
    {
	end = (int)((float)sig->sample_rate * lpc->times[i]);
	size = end - start;
	
	lpc_filterd(&res->samples[start],
		    lpc->frames[i],lpc->num_channels,
		    &sigd[start],size);
	
	start = end;
    }

    for (i=0; i<sig->num_samples; i++)
	sig->samples[i] = sigd[i];

    cst_free(sigd);

    return sig;
}

float lpc_min = -2.709040;
float lpc_range = 2.328840+2.709040;

void lpc_filterde(unsigned char *resulaw, 
		  unsigned short *as, int order, 
		  short *sig,
		  int size)
{
    int i, j;
    float r;
    float *a;
    float *sigf;
    
    a = cst_alloc(float,order);
    for (i=1; i<order; i++)
	a[i] = (((float)as[i]/65535.0)*lpc_range)+lpc_min;
    sigf = cst_alloc(float,size);
    for (i=0; i<size; i++)
	sigf[i] = sig[i];

    for (i = 0; i < order; i++)
    {
	r = cst_ulaw_to_short(resulaw[i]);
	for (j = 1; j < order; j++)
/*	    if (i-j >= 0) */
		r += a[j] * (float)sigf[i - j];
	sigf[i] = r;
    }
    for (i = order; i < size; i++)
    {
	r = cst_ulaw_to_short(resulaw[i]);
	for (j = 1; j < order; j++)
	    r += a[j] * (float)sigf[i - j];
	sigf[i] = r;
    }

    for (i=0; i<size; i++)
	sig[i] = sigf[i];
	
    cst_free(sigf);
/*    cst_free(a); */
}

cst_wave *reconstruct_wavee(cst_wave *res, cst_track *lpc)
{
    cst_wave *sig;
    int i,j;
    short *sigd;
    int start, end, size;
    unsigned short *as;
    unsigned char *resulaw;

    sig = new_wave();
    cst_wave_resize(sig,res->num_samples,1);
    sig->sample_rate = res->sample_rate;
    sigd = cst_alloc(short,sig->num_samples);
    as = cst_alloc(unsigned short,lpc->num_channels);

    resulaw = cst_alloc(unsigned char, res->num_samples);
    for (i=0; i<res->num_samples; i++)
	resulaw[i] = cst_short_to_ulaw(res->samples[i]);
    
    start = 0;
    for (i=0; i<lpc->num_frames; i++)
    {
	end = (int)((float)sig->sample_rate * lpc->times[i]);
	size = end - start;

	for (j=1; j<lpc->num_channels; j++)
	    as[j] = (((lpc->frames[i][j]-lpc_min)/lpc_range)*65535.0);
    
	lpc_filterde(&resulaw[start],
		    as,lpc->num_channels,
		    &sigd[start],size);
	
	start = end;
    }

    for (i=0; i<sig->num_samples; i++)
	sig->samples[i] = sigd[i];

    cst_free(sigd);

    return sig;
}

cst_wave *dumdum(cst_wave *res,cst_track *lpc)
{
    cst_lpcres *lpcres;
    int i,j,s;
    unsigned short *bbb;
    int start,end,size;
    FILE *ofd;

    lpcres = new_lpcres();
    lpcres_resize_frames(lpcres,lpc->num_frames);
    lpcres->num_channels = lpc->num_channels-1;
    start = 0;
    for (i=0; i<lpc->num_frames; i++)
    {
	bbb = cst_alloc(unsigned short,lpc->num_channels-1);
	for (j=1; j<lpc->num_channels; j++)
	    bbb[j-1] = (unsigned short)
		(((lpc->frames[i][j]-lpc_min)/lpc_range)*65535);
	lpcres->frames[i] = bbb;
	end = lpc->times[i]*res->sample_rate;
	lpcres->times[i] = lpc->times[i];
	size = end - start;
	lpcres->sizes[i] = size;
	start = end;
    }
    lpcres_resize_samples(lpcres,res->num_samples);
    lpcres->lpc_min = lpc_min;
    lpcres->lpc_range = lpc_range;
    lpcres->sample_rate = res->sample_rate;
    for (i=0; i<res->num_samples; i++)
	lpcres->residual[i] = cst_short_to_ulaw(res->samples[i]);

    ofd = fopen("lpc1.lpc","w");
    for (s=0,i=0; i<lpcres->num_frames; i++)
    {
	fprintf(ofd,"%d %d %d\n",i,lpcres->times[i],lpcres->sizes[i]);
	for (j=0; j < lpcres->num_channels; j++)
	    fprintf(ofd,"%d ",lpcres->frames[i][j]);
	fprintf(ofd,"\n");
	for (j=0; j < lpcres->sizes[i]; j++,s++)
	    fprintf(ofd,"%d ",lpcres->residual[s]);
	fprintf(ofd,"\n");
    }
    fclose(ofd);
    
    return lpc_resynth(lpcres);
}

int main(int argc, char **argv)
{
    /* need that cool argument parser */
    cst_wave *w,*res,*sig,*w2, *w3;
    cst_track *lpc;
    int i;
    double r;

    if (argc != 3)
    {
	fprintf(stderr,"usage: lpc_test2 LPC WAVEFILE\n");
	return 1;
    }

    lpc = new_track();
    cst_track_load_est(lpc,argv[1]);
    sig = new_wave();
    cst_wave_load_riff(sig,argv[2]);

    res = find_residual(sig,lpc);
    
    w = reconstruct_wave(res,lpc);

    cst_wave_save_riff(res,"res.wav");
    cst_wave_save_riff(w,"new.wav");

    w2 = reconstruct_wavee(res,lpc);
    cst_wave_save_riff(w2,"newe.wav");

    w3 = dumdum(res,lpc);
    cst_wave_save_riff(w3,"newr.wav");

    for (r=0.0,i=0; i<sig->num_samples; i++)
	r += ((float)sig->samples[i]-(float)w->samples[i]) *
	    ((float)sig->samples[i]-(float)w->samples[i]);
    r /= sig->num_samples;
    printf("orig/new %f\n",sqrt(r));
    for (r=0.0,i=0; i<sig->num_samples; i++)
	r += ((float)sig->samples[i]-(float)w2->samples[i]) *
	    ((float)sig->samples[i]-(float)w2->samples[i]);
    r /= sig->num_samples;
    printf("orig/newe %f\n",sqrt(r));
    for (r=0.0,i=0; i<sig->num_samples; i++)
	r += ((float)sig->samples[i]-(float)w3->samples[i]) *
	    ((float)sig->samples[i]-(float)w3->samples[i]);
    r /= sig->num_samples;
    printf("orig/newr %f\n",sqrt(r));
    return 0;
}
