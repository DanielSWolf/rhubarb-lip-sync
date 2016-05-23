/*  ---------------------------------------------------------------  */
/*      The HMM-Based Speech Synthesis System (HTS): version 1.1b    */
/*                        HTS Working Group                          */
/*                                                                   */
/*                   Department of Computer Science                  */
/*                   Nagoya Institute of Technology                  */
/*                                and                                */
/*    Interdisciplinary Graduate School of Science and Engineering   */
/*                   Tokyo Institute of Technology                   */
/*                      Copyright (c) 2001-2003                      */
/*                        All Rights Reserved.                       */
/*                                                                   */
/*  Permission is hereby granted, free of charge, to use and         */
/*  distribute this software and its documentation without           */
/*  restriction, including without limitation the rights to use,     */
/*  copy, modify, merge, publish, distribute, sublicense, and/or     */
/*  sell copies of this work, and to permit persons to whom this     */
/*  work is furnished to do so, subject to the following conditions: */
/*                                                                   */
/*    1. The code must retain the above copyright notice, this list  */
/*       of conditions and the following disclaimer.                 */
/*                                                                   */
/*    2. Any modifications must be clearly marked as such.           */
/*                                                                   */    
/*  NAGOYA INSTITUTE OF TECHNOLOGY, TOKYO INSITITUTE OF TECHNOLOGY,  */
/*  HTS WORKING GROUP, AND THE CONTRIBUTORS TO THIS WORK DISCLAIM    */
/*  ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL       */
/*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL NAGOYA INSTITUTE OF TECHNOLOGY, TOKYO INSITITUTE OF        */
/*  TECHNOLOGY, HTS WORKING GROUP, NOR THE CONTRIBUTORS BE LIABLE    */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY        */
/*  DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,  */
/*  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTUOUS   */
/*  ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR          */
/*  PERFORMANCE OF THIS SOFTWARE.                                    */
/*                                                                   */
/*  ---------------------------------------------------------------  */
/*   This is Zen's MLSA filter as ported by Toda to festvox vc       */
/*   and back ported into hts/festival so we can do MLSA filtering   */
/*   If I took more time I could probably make this use the same as  */
/*   as the other code in this directory -- awb@cs.cmu.edu 03JAN06   */
/*  ---------------------------------------------------------------  */
/*   and then ported into Flite (November 2007 awb@cs.cmu.edu)       */
/*     with some speed uptimizations                                 */

/*********************************************************************/
/*                                                                   */
/*  Mel-cepstral vocoder (pulse/noise excitation & MLSA filter)      */
/*                                    2003/12/26 by Heiga Zen        */
/*                                                                   */
/*  Extracted from HTS and slightly modified                         */
/*   by Tomoki Toda (tomoki@ics.nitech.ac.jp)                        */
/*  June 2004                                                        */
/*  Integrate as a Voice Conversion module                           */
/*                                                                   */
/*-------------------------------------------------------------------*/

#include "cst_alloc.h"
#include "cst_string.h"
#include "cst_math.h"
#include "cst_track.h"
#include "cst_wave.h"
#include "cst_audio.h"

#ifdef UNDER_CE
#define SPEED_HACK
/* This is one of those other things you shouldn't do, but it makes
   CG voices in flowm fast enough on my phone */
#define double float
#endif

#include "cst_vc.h"
#include "cst_cg.h"
#include "cst_mlsa.h"


static cst_wave *synthesis_body(const cst_track *params, 
                                const cst_track *str, 
                                double fs, double framem,
                                cst_cg_db *cg_db,
                                cst_audio_streaming_info *asi);

cst_wave *mlsa_resynthesis(const cst_track *params, 
                           const cst_track *str, 
                           cst_cg_db *cg_db,
                           cst_audio_streaming_info *asi)
{
    /* Resynthesizes a wave from given track */
    cst_wave *wave = 0;
    int sr = 16000;
    double shift;

    if (params->num_frames > 1)
        shift = 1000.0*(params->times[1]-params->times[0]);
    else
        shift = 5.0;

    wave = synthesis_body(params,str,sr,shift,cg_db,asi);

    return wave;
}

static cst_wave *synthesis_body(const cst_track *params, /* f0 + mcep */
                                const cst_track *str,
                                double fs,	/* sampling frequency (Hz) */
                                double framem,	/* FFT length */
                                cst_cg_db *cg_db,
                                cst_audio_streaming_info *asi)
{
    long t, pos;
    int framel, i;
    double f0;
    VocoderSetup vs;
    cst_wave *wave = 0;
    double *mcep;
    int stream_mark;
    int rc = CST_AUDIO_STREAM_CONT;
    int num_mcep;

    num_mcep = params->num_channels-1;
    framel = (int)(framem * fs / 1000.0);
    init_vocoder(fs, framel, num_mcep, &vs, cg_db);

    if (str != NULL)
        vs.gauss = MFALSE;

    /* synthesize waveforms by MLSA filter */
    wave = new_wave();
    cst_wave_resize(wave,params->num_frames * (framel + 2),1);
#ifdef SPEED_HACK
    /* This is a SPECTACULAR hack -- basically resample the original */
    /* files to 8KHz but label them as 16KHz and do the full build */
    /* then after synthesis get them to be played at 8KHz -- it works */
    wave->sample_rate = 8000; 
#else
    wave->sample_rate = fs; 
#endif
    mcep = cst_alloc(double,num_mcep+1);

    for (t = 0, stream_mark = pos = 0; 
         (rc == CST_AUDIO_STREAM_CONT) && (t < params->num_frames); 
         t++) 
    {
        f0 = (double)params->frames[t][0];
        for (i=1; i<num_mcep+1; i++)
            mcep[i-1] = params->frames[t][i];
        mcep[i-1] = 0;

        vocoder(f0, mcep, str, t, num_mcep, cg_db, &vs, wave, &pos);

        if (asi && (pos-stream_mark > asi->min_buffsize))
        {
            rc=(*asi->asc)(wave,stream_mark,pos-stream_mark,0,asi->userdata);
            stream_mark = pos;
        }
    }
    wave->num_samples = pos;

    if (asi && (rc == CST_AUDIO_STREAM_CONT))
    {   /* drain the last part of the waveform */
        (*asi->asc)(wave,stream_mark,pos-stream_mark,1,asi->userdata);
    }

    /* memory free */
    cst_free(mcep);
    free_vocoder(&vs);

    return wave;
}

static void init_vocoder(double fs, int framel, int m, 
                         VocoderSetup *vs, cst_cg_db *cg_db)
{
    /* initialize global parameter */
    vs->fprd = framel;
    vs->iprd = 1;
    vs->seed = 1;
#ifdef SPEED_HACK
    /* This makes it about 25% faster and sounds basically the same */
    vs->pd   = 4;
#else
    vs->pd   = 5;
#endif

    vs->next =1;
    vs->gauss = MTRUE;

    /* Pade' approximants */
    vs->pade[ 0]=1.0;
    vs->pade[ 1]=1.0; vs->pade[ 2]=0.0;
    vs->pade[ 3]=1.0; vs->pade[ 4]=0.0;      vs->pade[ 5]=0.0;
    vs->pade[ 6]=1.0; vs->pade[ 7]=0.0;      vs->pade[ 8]=0.0;      vs->pade[ 9]=0.0;
    vs->pade[10]=1.0; vs->pade[11]=0.4999273; vs->pade[12]=0.1067005; vs->pade[13]=0.01170221; vs->pade[14]=0.0005656279;
    vs->pade[15]=1.0; vs->pade[16]=0.4999391; vs->pade[17]=0.1107098; vs->pade[18]=0.01369984; vs->pade[19]=0.0009564853;
    vs->pade[20]=0.00003041721;

    vs->rate = fs;
    vs->c = cst_alloc(double,3 * (m + 1) + 3 * (vs->pd + 1) + vs->pd * (m + 2));
   
    vs->p1 = -1;
    vs->sw = 0;
    vs->x  = 0x55555555;
   
    /* for postfiltering */
    vs->mc = NULL;
    vs->o  = 0;
    vs->d  = NULL;
    vs->irleng= 64;
   
    // for MIXED EXCITATION
    vs->ME_order = cg_db->ME_order;
    vs->ME_num = cg_db->ME_num;
    vs->hpulse = cst_alloc(double,vs->ME_order);
    vs->hnoise = cst_alloc(double,vs->ME_order);
    vs->xpulsesig = cst_alloc(double,vs->ME_order);
    vs->xnoisesig = cst_alloc(double,vs->ME_order);
    vs->h = cg_db->me_h;

    return;
}

static double plus_or_minus_one()
{
    /* Randomly return 1 or -1 */
    /* not sure rand() is portable */
    if (rand() > RAND_MAX/2.0)
        return 1.0;
    else
        return -1.0;
}

static void vocoder(double p, double *mc, 
                    const cst_track *str, int t,
                    int m, cst_cg_db *cg_db,
                    VocoderSetup *vs, cst_wave *wav, long *pos)
{
    double inc, x, e1, e2;
    int i, j, k; 
    double xpulse, xnoise;
    double fxpulse, fxnoise;
    float gain=1.0;
    
    if (cg_db->gain != 0.0)
        gain = cg_db->gain;
   
    if (str != NULL)     /* MIXED-EXCITATION */
    {
        /* Copy in str's and build hpulse and hnoise for this frame */
        for (i=0; i<vs->ME_order; i++)
        {
            vs->hpulse[i] = vs->hnoise[i] = 0.0;
            for (j=0; j<vs->ME_num; j++)
            {
                vs->hpulse[i] += str->frames[t][j] * vs->h[j][i];
                vs->hnoise[i] += (1 - str->frames[t][j]) * vs->h[j][i];
            }
        }
    }

    if (p != 0.0)
	p = vs->rate / p;  /* f0 -> pitch */
   
    if (vs->p1 < 0) {
	if (vs->gauss & (vs->seed != 1)) 
	    vs->next = srnd((unsigned)vs->seed);
         
	vs->p1   = p;
	vs->pc   = vs->p1;
	vs->cc   = vs->c + m + 1;
	vs->cinc = vs->cc + m + 1;
	vs->d1   = vs->cinc + m + 1;

	mc2b(mc, vs->c, m, cg_db->mlsa_alpha);
      
	if (cg_db->mlsa_beta > 0.0 && m > 1) {
	    e1 = b2en(vs->c, m, cg_db->mlsa_alpha, vs);
	    vs->c[1] -= cg_db->mlsa_beta * cg_db->mlsa_alpha * mc[2];
	    for (k=2;k<=m;k++)
		vs->c[k] *= (1.0 + cg_db->mlsa_beta);
	    e2 = b2en(vs->c, m, cg_db->mlsa_alpha, vs);
	    vs->c[0] += log(e1/e2)/2;
	}

	return;
    }

    mc2b(mc, vs->cc, m, cg_db->mlsa_alpha); 
    if (cg_db->mlsa_beta>0.0 && m > 1) {
	e1 = b2en(vs->cc, m, cg_db->mlsa_alpha, vs);
	vs->cc[1] -= cg_db->mlsa_beta * cg_db->mlsa_alpha * mc[2];
	for (k = 2; k <= m; k++)
	    vs->cc[k] *= (1.0 + cg_db->mlsa_beta);
	e2 = b2en(vs->cc, m, cg_db->mlsa_alpha, vs);
	vs->cc[0] += log(e1 / e2) / 2.0;
    }

    for (k=0; k<=m; k++)
	vs->cinc[k] = (vs->cc[k] - vs->c[k]) *
	    (double)vs->iprd / (double)vs->fprd;

    if (vs->p1!=0.0 && p!=0.0) {
	inc = (p - vs->p1) * (double)vs->iprd / (double)vs->fprd;
    } else {
	inc = 0.0;
	vs->pc = p;
	vs->p1 = 0.0;
    }

    for (j = vs->fprd, i = (vs->iprd + 1) / 2; j--;) {
	if (vs->p1 == 0.0) {
	    if (vs->gauss)
		x = (double) nrandom(vs);
	    else
		x = plus_or_minus_one();
            if (str != NULL)             /* MIXED EXCITATION */
            {
                xnoise = x;
                xpulse = 0.0;
            }
	} else {
	    if ((vs->pc += 1.0) >= vs->p1) {
		x = sqrt (vs->p1);
		vs->pc = vs->pc - vs->p1;
	    } else 
                x = 0.0;

            if (str != NULL)  /* MIXED EXCITATION */
            {
                xpulse = x;
                xnoise = plus_or_minus_one();
            }
	}

        /* MIXED EXCITATION */
        /* The real work -- apply shaping filters to pulse and noise */
        if (str != NULL)
        {
            fxpulse = fxnoise = 0.0;
            for (k=vs->ME_order-1; k>0; k--)
            {
                fxpulse += vs->hpulse[k] * vs->xpulsesig[k];
                fxnoise += vs->hnoise[k] * vs->xnoisesig[k];

                vs->xpulsesig[k] = vs->xpulsesig[k-1];
                vs->xnoisesig[k] = vs->xnoisesig[k-1];
            }
            fxpulse += vs->hpulse[0] * xpulse;
            fxnoise += vs->hnoise[0] * xnoise;
            vs->xpulsesig[0] = xpulse;
            vs->xnoisesig[0] = xnoise;

            x = fxpulse + fxnoise; /* excitation is pulse plus noise */
        }

#ifdef SPEED_HACK
        /* 8KHz voices are too quiet */
	x *= exp(vs->c[0])*2.0;
#else
	x *= exp(vs->c[0])*gain;
#endif

	x = mlsadf(x, vs->c, m, cg_db->mlsa_alpha, vs->pd, vs->d1, vs);

        wav->samples[*pos] = (short)x;
	*pos += 1;

	if (!--i) {
	    vs->p1 += inc;
	    for (k = 0; k <= m; k++) vs->c[k] += vs->cinc[k];
	    i = vs->iprd;
	}
    }
   
    vs->p1 = p;
    memmove(vs->c,vs->cc,sizeof(double)*(m+1));
   
    return;
}

static double mlsadf(double x, double *b, int m, double a, int pd, double *d, VocoderSetup *vs)
{

   vs->ppade = &(vs->pade[pd*(pd+1)/2]);
    
   x = mlsadf1 (x, b, m, a, pd, d, vs);
   x = mlsadf2 (x, b, m, a, pd, &d[2*(pd+1)], vs);

   return(x);
}

static double mlsadf1(double x, double *b, int m, double a, int pd, double *d, VocoderSetup *vs)
{
   double v, out = 0.0, *pt, aa;
   register int i;

   aa = 1 - a*a;
   pt = &d[pd+1];

   for (i=pd; i>=1; i--) {
      d[i] = aa*pt[i-1] + a*d[i];
      pt[i] = d[i] * b[1];
      v = pt[i] * vs->ppade[i];
      x += (1 & i) ? v : -v;
      out += v;
   }

   pt[0] = x;
   out += x;

   return(out);
}

static double mlsadf2 (double x, double *b, int m, double a, int pd, double *d, VocoderSetup *vs)
{
   double v, out = 0.0, *pt, aa;
   register int i;
    
   aa = 1 - a*a;
   pt = &d[pd * (m+2)];

   for (i=pd; i>=1; i--) {
       pt[i] = mlsafir (pt[i-1], b, m, a, &d[(i-1)*(m+2)]);

       v = pt[i] * vs->ppade[i];

       x  += (1&i) ? v : -v;
       out += v;
   }
    
   pt[0] = x;
   out  += x;

   return(out);
}

static double mlsafir (double x, double *b, int m, double a, double *d)
{  
   double y = 0.0;
   double aa;
   register int i;

   aa = 1 - a*a;

   d[0] = x;
   d[1] = aa*d[0] + a*d[1];
   for (i=2; i<= m; i++) {
      d[i] = d[i] + a*(d[i+1]-d[i-1]);
      y += d[i]*b[i];
   }

   for (i=m+1; i>1; i--) 
      d[i] = d[i-1];

   return(y);
}

static double nrandom (VocoderSetup *vs)
{
   if (vs->sw == 0) {
      vs->sw = 1;
      do {
         vs->r1 = 2.0 * rnd(&vs->next) - 1.0;
         vs->r2 = 2.0 * rnd(&vs->next) - 1.0;
         vs->s  = vs->r1 * vs->r1 + vs->r2 * vs->r2;
      } while (vs->s > 1 || vs->s == 0);

      vs->s = sqrt (-2 * log(vs->s) / vs->s);
      
      return(vs->r1*vs->s);
   }
   else {
      vs->sw = 0;
      
      return (vs->r2*vs->s);
   }
}

static double rnd (unsigned long *next)
{
   double r;

   *next = *next * 1103515245L + 12345;
   r = (*next / 65536L) % 32768L;

   return(r/RANDMAX); 
}

static unsigned long srnd ( unsigned long seed )
{
   return(seed);
}

/* mc2b : transform mel-cepstrum to MLSA digital fillter coefficients */
static void mc2b (double *mc, double *b, int m, double a)
{
   b[m] = mc[m];
    
   for (m--; m>=0; m--)
      b[m] = mc[m] - a * b[m+1];
   
   return;
}


static double b2en (double *b, int m, double a, VocoderSetup *vs)
{
   double en;
   int k;
   
   if (vs->o<m) {
      if (vs->mc != NULL)
          cst_free(vs->mc);
    
      vs->mc = cst_alloc(double,(m + 1) + 2 * vs->irleng);
      vs->cep = vs->mc + m+1;
      vs->ir  = vs->cep + vs->irleng;
   }

   b2mc(b, vs->mc, m, a);
   freqt(vs->mc, m, vs->cep, vs->irleng-1, -a, vs);
   c2ir(vs->cep, vs->irleng, vs->ir, vs->irleng);
   en = 0.0;
   
   for (k=0;k<vs->irleng;k++)
      en += vs->ir[k] * vs->ir[k];

   return(en);
}


/* b2bc : transform MLSA digital filter coefficients to mel-cepstrum */
static void b2mc (double *b, double *mc, int m, double a)
{
  double d, o;
        
  d = mc[m] = b[m];
  for (m--; m>=0; m--) {
    o = b[m] + a * d;
    d = b[m];
    mc[m] = o;
  }
  
  return;
}

/* freqt : frequency transformation */
static void freqt (double *c1, int m1, double *c2, int m2, double a, VocoderSetup *vs)
{
   register int i, j;
   double b;
    
   if (vs->d==NULL) {
      vs->size = m2;
      vs->d    = cst_alloc(double,vs->size + vs->size + 2);
      vs->g    = vs->d+vs->size+1;
   }

   if (m2>vs->size) {
       cst_free(vs->d);
      vs->size = m2;
      vs->d    = cst_alloc(double,vs->size + vs->size + 2);
      vs->g    = vs->d+vs->size+1;
   }
    
   b = 1-a*a;
   for (i=0; i<m2+1; i++)
      vs->g[i] = 0.0;

   for (i=-m1; i<=0; i++) {
      if (0 <= m2)
         vs->g[0] = c1[-i]+a*(vs->d[0]=vs->g[0]);
      if (1 <= m2)
         vs->g[1] = b*vs->d[0]+a*(vs->d[1]=vs->g[1]);
      for (j=2; j<=m2; j++)
         vs->g[j] = vs->d[j-1]+a*((vs->d[j]=vs->g[j])-vs->g[j-1]);
   }

   memmove(c2,vs->g,sizeof(double)*(m2+1));
   
   return;
}

/* c2ir : The minimum phase impulse response is evaluated from the minimum phase cepstrum */
static void c2ir (double *c, int nc, double *h, int leng)
{
   register int n, k, upl;
   double  d;

   h[0] = exp(c[0]);
   for (n=1; n<leng; n++) {
      d = 0;
      upl = (n>=nc) ? nc-1 : n;
      for (k=1; k<=upl; k++)
         d += k*c[k]*h[n-k];
      h[n] = d/n;
   }
   
   return;
}

static void free_vocoder(VocoderSetup *vs)
{

    cst_free(vs->c);
    cst_free(vs->mc);
    cst_free(vs->d);
 
    vs->c = NULL;
    vs->mc = NULL;
    vs->d = NULL;
    vs->ppade = NULL;
    vs->cc = NULL;
    vs->cinc = NULL;
    vs->d1 = NULL;
    vs->g = NULL;
    vs->cep = NULL;
    vs->ir = NULL;

    cst_free(vs->hpulse);
    cst_free(vs->hnoise);
    cst_free(vs->xpulsesig);
    cst_free(vs->xnoisesig);

   
    return;
}

