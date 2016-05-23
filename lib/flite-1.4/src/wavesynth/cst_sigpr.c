/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                         Copyright (c) 2001                            */
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
/*               Date:  January 2001                                     */
/*************************************************************************/
/*                                                                       */
/*  Signal processing functions                                          */
/*                                                                       */
/*************************************************************************/

#include "cst_math.h"
#include "cst_hrg.h"
#include "cst_wave.h"
#include "cst_sigpr.h"
#include "cst_sts.h"

cst_wave *lpc_resynth(cst_lpcres *lpcres)
{
    cst_wave *w;
    int i,j,r,o,k;
    int ci,cr;
    float *outbuf, *lpccoefs;
    int pm_size_samps;

    /* Get a new wave to build the signal into */
    w = new_wave();
    cst_wave_resize(w,lpcres->num_samples,1);
    w->sample_rate = lpcres->sample_rate;
    /* outbuf is a circular buffer with past relevant samples in it */
    outbuf = cst_alloc(float,1+lpcres->num_channels);
    /* unpacked lpc coefficients */
    lpccoefs = cst_alloc(float,lpcres->num_channels);

    for (r=0,o=lpcres->num_channels,i=0; i < lpcres->num_frames; i++)
    {
	pm_size_samps = lpcres->sizes[i];

	/* Unpack the LPC coefficients */
	for (k=0; k<lpcres->num_channels; k++)
	{
	    lpccoefs[k] = (float)((((double)lpcres->frames[i][k])/65535.0)*
			   lpcres->lpc_range) + lpcres->lpc_min;
	}
	/* Note we don't zero the lead in from the previous part */
	/* seems like you should but it makes it worse if you do */
/*	memset(outbuf,0,sizeof(float)*(1+lpcres->num_channels)); */

	/* resynthesis the signal */
	for (j=0; j < pm_size_samps; j++,r++)
	{
	    outbuf[o] = (float)cst_ulaw_to_short(lpcres->residual[r]);
	    cr = (o == 0 ? lpcres->num_channels : o-1);
	    for (ci=0; ci < lpcres->num_channels; ci++)
	    {
		outbuf[o] += lpccoefs[ci] * outbuf[cr];
		cr = (cr == 0 ? lpcres->num_channels : cr-1);
	    }
	    w->samples[r] = (short)(outbuf[o]);
	    o = (o == lpcres->num_channels ? 0 : o+1);
	}
    }

    cst_free(outbuf);
    cst_free(lpccoefs);

    return w;

}

cst_wave *lpc_resynth_windows(cst_lpcres *lpcres)
{
    cst_wave *w;
    int i,j,r,o,k;
    int ci,cr;
    float *outbuf, *lpccoefs;
    int pm_size_samps;

    /* Get a new wave to build the signal into */
    w = new_wave();
    cst_wave_resize(w,lpcres->num_samples,1);
    w->sample_rate = lpcres->sample_rate;
    /* outbuf is a circular buffer with past relevant samples in it */
    outbuf = cst_alloc(float,1+lpcres->num_channels);
    /* unpacked lpc coefficients */
    lpccoefs = cst_alloc(float,lpcres->num_channels);

    for (r=0,o=lpcres->num_channels,i=0; i < lpcres->num_frames; i++)
    {
	pm_size_samps = lpcres->sizes[i];

	/* Unpack the LPC coefficients */
	for (k=0; k<lpcres->num_channels; k++)
	{
	    lpccoefs[k] = ((float)(((double)lpcres->frames[i][k])/65535.0)*
			   lpcres->lpc_range) + lpcres->lpc_min;
	}
	memset(outbuf,0,sizeof(float)*(1+lpcres->num_channels)); 

	/* resynthesis the signal */
	for (j=0; j < pm_size_samps; j++,r++)
	{
	    outbuf[o] = (float)cst_ulaw_to_short(lpcres->residual[r]);
	    cr = (o == 0 ? lpcres->num_channels : o-1);
	    for (ci=0; ci < lpcres->num_channels; ci++)
	    {
		outbuf[o] += lpccoefs[ci] * outbuf[cr];
		cr = (cr == 0 ? lpcres->num_channels : cr-1);
	    }
	    w->samples[r] = (short)(outbuf[o]);
	    o = (o == lpcres->num_channels ? 0 : o+1);
	}
    }

    cst_free(outbuf);
    cst_free(lpccoefs);

    return w;

}

const static short ulaw_to_short_table[] =
{
    -32124, -31100, -30076, -29052, -28028, -27004, -25980, -24956,
    -23932, -22908, -21884, -20860, -19836, -18812, -17788, -16764, 
    -15996, -15484, -14972, -14460, -13948, -13436, -12924, -12412,
    -11900, -11388, -10876, -10364, -9852, -9340, -8828, -8316, 
    -7932, -7676, -7420, -7164, -6908, -6652, -6396, -6140,
    -5884, -5628, -5372, -5116, -4860, -4604, -4348, -4092, 
    -3900, -3772, -3644, -3516, -3388, -3260, -3132, -3004,
    -2876, -2748, -2620, -2492, -2364, -2236, -2108, -1980, 
    -1884, -1820, -1756, -1692, -1628, -1564, -1500, -1436,
    -1372, -1308, -1244, -1180, -1116, -1052, -988, -924, 
    -876, -844, -812, -780, -748, -716, -684, -652,
    -620, -588, -556, -524, -492, -460, -428, -396, 
    -372, -356, -340, -324, -308, -292, -276, -260,
    -244, -228, -212, -196, -180, -164, -148, -132, 
    -120, -112, -104, -96, -88, -80, -72, -64,
    -56, -48, -40, -32, -24, -16, -8, 0, 
    32124, 31100, 30076, 29052, 28028, 27004, 25980, 24956,
    23932, 22908, 21884, 20860, 19836, 18812, 17788, 16764, 
    15996, 15484, 14972, 14460, 13948, 13436, 12924, 12412,
    11900, 11388, 10876, 10364, 9852, 9340, 8828, 8316, 
    7932, 7676, 7420, 7164, 6908, 6652, 6396, 6140,
    5884, 5628, 5372, 5116, 4860, 4604, 4348, 4092, 
    3900, 3772, 3644, 3516, 3388, 3260, 3132, 3004,
    2876, 2748, 2620, 2492, 2364, 2236, 2108, 1980, 
    1884, 1820, 1756, 1692, 1628, 1564, 1500, 1436,
    1372, 1308, 1244, 1180, 1116, 1052, 988, 924, 
    876, 844, 812, 780, 748, 716, 684, 652,
    620, 588, 556, 524, 492, 460, 428, 396, 
    372, 356, 340, 324, 308, 292, 276, 260,
    244, 228, 212, 196, 180, 164, 148, 132, 
    120, 112, 104, 96, 88, 80, 72, 64,
    56, 48, 40, 32, 24, 16, 8, 0 };

cst_wave *lpc_resynth_fixedpoint(cst_lpcres *lpcres)
{
    /* The fixed point version, without floats */
    cst_wave *w;
    int i,j,r,o,k;
    int stream_mark;
    int ci,cr;
    int *outbuf, *lpccoefs;
    int pm_size_samps, ilpc_min, ilpc_range;
    int pp = 0;
    int rc = CST_AUDIO_STREAM_CONT;

    /* Get a new wave to build the signal into */
    w = new_wave();
    cst_wave_resize(w,lpcres->num_samples,1);
    w->sample_rate = lpcres->sample_rate;
    /* outbuf is a circular buffer with past relevant samples in it */
    outbuf = cst_alloc(int,1+lpcres->num_channels);
    /* unpacked lpc coefficients */
    lpccoefs = cst_alloc(int,lpcres->num_channels);
    ilpc_min = (int)(lpcres->lpc_min*32768.0);
    /* assume range is never > abs(16) */
    ilpc_range = (int)(lpcres->lpc_range*2048.0);

    stream_mark = 0;
    for (r=0,o=lpcres->num_channels,i=0; 
         (rc == CST_AUDIO_STREAM_CONT) && (i < lpcres->num_frames); 
         i++)
    {
	pm_size_samps = lpcres->sizes[i];

	/* Unpack the LPC coefficients */
	for (k=0; k<lpcres->num_channels; k++)
	    lpccoefs[k]=((lpcres->frames[i][k]/2*ilpc_range)/2048+ilpc_min)/2;

	/* resynthesis the signal */
	for (j=0; j < pm_size_samps; j++,r++)
	{
	    outbuf[o] = (int)ulaw_to_short_table[lpcres->residual[r]];
	    outbuf[o] *= 16384;
	    cr = (o == 0 ? lpcres->num_channels : o-1);
	    for (ci=0; ci < lpcres->num_channels; ci++)
	    {
		outbuf[o] += lpccoefs[ci]*outbuf[cr];
		cr = (cr == 0 ? lpcres->num_channels : cr-1);
	    }
	    outbuf[o] /= 16384;
	    w->samples[r] = (short)outbuf[o];
	    pp = outbuf[o];
	    o = (o == lpcres->num_channels ? 0 : o+1);
	}
        if (lpcres->asi && (r-stream_mark > lpcres->asi->min_buffsize))
        {
             rc = (*lpcres->asi->asc)(w,stream_mark,r-stream_mark,0,
                                 lpcres->asi->userdata);
             stream_mark = r;
        }
    }

    if ((lpcres->asi) && (rc == CST_AUDIO_STREAM_CONT))
        (*lpcres->asi->asc)(w,stream_mark,r-stream_mark,1,lpcres->asi->userdata);

    cst_free(outbuf);
    cst_free(lpccoefs);
    w->num_samples = r;  /* just to be safe */

    return w;

}

cst_wave *lpc_resynth_sfp(cst_lpcres *lpcres)
{
    /* The fixed point spike excited, without floats */
    cst_wave *w;
    int i,j,r,o,k;
    int ci,cr;
    int *outbuf, *lpccoefs;
    int pm_size_samps, ilpc_min, ilpc_range;
    int pp = 0;

    /* Get a new wave to build the signal into */
    w = new_wave();
    cst_wave_resize(w,lpcres->num_samples,1);
    w->sample_rate = lpcres->sample_rate;
    /* outbuf is a circular buffer with past relevant samples in it */
    outbuf = cst_alloc(int,1+lpcres->num_channels);
    /* unpacked lpc coefficients */
    lpccoefs = cst_alloc(int,lpcres->num_channels);
    ilpc_min = (int)(lpcres->lpc_min*32768.0);
    /* assume range is never > abs(16) */
    ilpc_range = (int)(lpcres->lpc_range*2048.0);

    for (r=0,o=lpcres->num_channels,i=0; i < lpcres->num_frames; i++)
    {
	pm_size_samps = lpcres->sizes[i];

	/* Unpack the LPC coefficients */
	for (k=0; k<lpcres->num_channels; k++)
	    lpccoefs[k]=((lpcres->frames[i][k]/2*ilpc_range)/2048+ilpc_min)/2;

	/* resynthesis the signal */
	for (j=0; j < pm_size_samps; j++,r++)
	{
	    outbuf[o] = (int)cst_ulaw_to_short(lpcres->residual[r]);
	    cr = (o == 0 ? lpcres->num_channels : o-1);
	    for (ci=0; ci < lpcres->num_channels; ci++)
	    {
		outbuf[o] += (lpccoefs[ci]*outbuf[cr])/16384;
		cr = (cr == 0 ? lpcres->num_channels : cr-1);
	    }
	    w->samples[r] = (short)outbuf[o];
	    pp = outbuf[o];
	    o = (o == lpcres->num_channels ? 0 : o+1);
	}
    }

    cst_free(outbuf);
    cst_free(lpccoefs);

    return w;

}
