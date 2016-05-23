/*********************************************************************/
/*                                                                   */
/*            Nagoya Institute of Technology, Aichi, Japan,          */
/*       Nara Institute of Science and Technology, Nara, Japan       */
/*                                and                                */
/*             Carnegie Mellon University, Pittsburgh, PA            */
/*                      Copyright (c) 2003-2004                      */
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
/*    2. Any modifications must be clearly marked as such.           */
/*    3. Original authors' names are not deleted.                    */
/*                                                                   */    
/*  NAGOYA INSTITUTE OF TECHNOLOGY, NARA INSTITUTE OF SCIENCE AND    */
/*  TECHNOLOGY, CARNEGIE MELLON UNIVERSITY, AND THE CONTRIBUTORS TO  */
/*  THIS WORK DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,  */
/*  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, */
/*  IN NO EVENT SHALL NAGOYA INSTITUTE OF TECHNOLOGY, NARA           */
/*  INSTITUTE OF SCIENCE AND TECHNOLOGY, CARNEGIE MELLON UNIVERSITY, */
/*  NOR THE CONTRIBUTORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR      */
/*  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM   */
/*  LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,  */
/*  NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN        */
/*  CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.         */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/*          Author :  Tomoki Toda (tomoki@ics.nitech.ac.jp)          */
/*          Date   :  June 2004                                      */
/*                                                                   */
/*          Modified by Alan W Black (awb@cs.cmu.edu) Jan 2006       */
/*          taken from festvox/src/vc/ back into Festival            */
/*          Modified by Alan W Black (awb@cs.cmu.edu) Nov 2007       */
/*          taken from Festival into Flite                           */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  Subroutine for Speech Synthesis                                  */
/*                                                                   */
/*-------------------------------------------------------------------*/

#ifndef __CST_MLSA_H
#define __CST_MLSA_H

#include "cst_audio.h"
#include "cst_wave.h"

/* static void waveampcheck(DVECTOR wav, XBOOL msg_flag); */

#define RANDMAX 32767 
#define   B0         0x00000001
#define   B28        0x10000000
#define   B31        0x80000000
#define   B31_       0x7fffffff
#define   Z          0x00000000

typedef struct _VocoderSetup {
   
   int fprd;
   int iprd;
   int seed;
   int pd;
   unsigned long next;
   Boolean gauss;
   double p1;
   double pc;
   double pj;
   double pade[21];
   double *ppade;
   double *c, *cc, *cinc, *d1;
   double rate;
   
   int sw;
   double r1, r2, s;
   
   int x;
   
   /* for postfiltering */
   int size;
   double *d; 
   double *g;
   double *mc;
   double *cep;
   double *ir;
   int o;
   int irleng;
   
    /* for MIXED EXCITATION */
    int ME_order;
    int ME_num;
    double *hpulse;
    double *hnoise;

    double *xpulsesig;
    double *xnoisesig;

    const double * const *h;  

} VocoderSetup;

static void init_vocoder(double fs, int framel, int m, 
                         VocoderSetup *vs, cst_cg_db *cg_db);
static void vocoder(double p, double *mc, 
                    const cst_track *str, int t,
                    int m, cst_cg_db *cg_db,
                     VocoderSetup *vs, cst_wave *wav, long *pos);
static double mlsadf(double x, double *b, int m, double a, int pd, double *d,
		     VocoderSetup *vs);
static double mlsadf1(double x, double *b, int m, double a, int pd, double *d,
		      VocoderSetup *vs);
static double mlsadf2(double x, double *b, int m, double a, int pd, double *d,
		      VocoderSetup *vs);
static double mlsafir (double x, double *b, int m, double a, double *d);
static double nrandom (VocoderSetup *vs);
static double rnd (unsigned long *next);
static unsigned long srnd (unsigned long seed);
static void mc2b (double *mc, double *b, int m, double a);
static double b2en (double *b, int m, double a, VocoderSetup *vs);
static void b2mc (double *b, double *mc, int m, double a);
static void freqt (double *c1, int m1, double *c2, int m2, double a,
		   VocoderSetup *vs);
static void c2ir (double *c, int nc, double *h, int leng);

static void free_vocoder(VocoderSetup *vs);

#endif /* __CST_MLSA_H */
