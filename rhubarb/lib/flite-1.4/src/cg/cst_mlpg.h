/*  ---------------------------------------------------------------  */
/*      The HMM-Based Speech Synthesis System (HTS): version 1.1.1   */
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
/*  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS   */
/*  ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR          */
/*  PERFORMANCE OF THIS SOFTWARE.                                    */
/*                                                                   */
/*  ---------------------------------------------------------------  */
/*    mlpg.c : speech parameter generation from pdf sequence         */
/*                                                                   */
/*                                    2003/12/26 by Heiga Zen        */
/*  ---------------------------------------------------------------  */

/*********************************************************************/
/*                                                                   */
/*            Nagoya Institute of Technology, Aichi, Japan,          */
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
/*  NAGOYA INSTITUTE OF TECHNOLOGY, CARNEGIE MELLON UNIVERSITY, AND  */
/*  THE CONTRIBUTORS TO THIS WORK DISCLAIM ALL WARRANTIES WITH       */
/*  REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF     */
/*  MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL NAGOYA INSTITUTE  */
/*  OF TECHNOLOGY, CARNEGIE MELLON UNIVERSITY, NOR THE CONTRIBUTORS  */
/*  BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR  */
/*  ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR       */
/*  PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER   */
/*  TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE    */
/*  OR PERFORMANCE OF THIS SOFTWARE.                                 */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/*  ML-Based Parameter Generation                                    */
/*                                    2003/12/26 by Heiga Zen        */
/*                                                                   */
/*  Basic functions are extracted from HTS and                       */
/*   modified by Tomoki Toda (tomoki@ics.nitech.ac.jp)               */
/*  June 2004                                                        */
/*  Integrate as a Voice Conversion module                           */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*          Author :  Tomoki Toda (tomoki@ics.nitech.ac.jp)          */
/*          Date   :  June 2004                                      */
/*                                                                   */
/*-------------------------------------------------------------------*/
/* Integrated into a single C file for festival/flite integration    */
/* May 2008 awb@cs.cmu.edu                                           */
/*-------------------------------------------------------------------*/

#ifndef _MLPG_H
#define _MLPG_H

#include "cst_cg.h"

#define	LENGTH 256
#define	INFTY ((double) 1.0e+38)
#define	INFTY2 ((double) 1.0e+19)
#define	INVINF ((double) 1.0e-38)
#define	INVINF2 ((double) 1.0e-19)

#ifdef M_PI
#define PI		M_PI
#else
#define PI		3.1415926535897932385
#endif

#define	WLEFT 0
#define	WRIGHT 1

typedef struct _DWin {
    int	num;		/* number of static + deltas */
    int **width;	/* width [0..num-1][0(left) 1(right)] */
    double **coef;	/* coefficient [0..num-1][length[0]..length[1]] */
    double **coef_ptrs;	/* keeps the pointers so we can free them */
    int maxw[2];	/* max width [0(left) 1(right)] */
} DWin;

typedef struct _PStreamChol {
    int vSize;		// size of ovserved vector
    int order;		// order of cepstrum
    int T;		// number of frames
    int width;		// width of WSW
    DWin dw;
    double **mseq;	// sequence of mean vector
    double **ivseq;	// sequence of invarsed covariance vector
    double ***ifvseq;	// sequence of invarsed full covariance vector
    double **R;		// WSW[T][range]
    double *r;		// WSM [T]
    double *g;		// g [T]
    double **c;		// parameter c
} PStreamChol;


typedef struct MLPGPARA_STRUCT {
    DVECTOR ov;
    DVECTOR iuv;
    DVECTOR iumv;
    DVECTOR flkv;
    DMATRIX stm;
    DMATRIX dltm;
    DMATRIX pdf;
    DVECTOR detvec;
    DMATRIX wght;
    DMATRIX mean;
    DMATRIX cov;
    LVECTOR clsidxv;
    DVECTOR clsdetv;
    DMATRIX clscov;
    double vdet;
    DVECTOR vm;
    DVECTOR vv;
    DVECTOR var;
} *MLPGPARA;

static MLPGPARA xmlpgpara_init(int dim, int dim2, int dnum, int clsnum);
static void xmlpgparafree(MLPGPARA param);
static double get_like_pdfseq_vit(int dim, int dim2, int dnum, int clsnum,
                                  MLPGPARA param, 
                                  float **model, 
                                  XBOOL dia_flag);
#if 0
static double get_like_gv(long dim2, long dnum, MLPGPARA param);
static void sm_mvav(DMATRIX mat, long hlen);
#endif
static void get_dltmat(DMATRIX mat, DWin *dw, int dno, DMATRIX dmat);


static double *dcalloc(int x, int xoff);
static double **ddcalloc(int x, int y, int xoff, int yoff);

/***********************************/
/* ML using Choleski decomposition */
/***********************************/
/* Diagonal Covariance Version */
static void InitDWin(PStreamChol *pst, const float *dynwin, int fsize);
static void InitPStreamChol(PStreamChol *pst, const float *dynwin, int fsize,
                            int order, int T);
static void mlgparaChol(DMATRIX pdf, PStreamChol *pst, DMATRIX mlgp);
static void mlpgChol(PStreamChol *pst);
static void calc_R_and_r(PStreamChol *pst, const int m);
static void Choleski(PStreamChol *pst);
static void Choleski_forward(PStreamChol *pst);
static void Choleski_backward(PStreamChol *pst, const int m);
#if 0
/* Full Covariance Version */
static void InitPStreamCholFC(PStreamChol *pst, char *dynwinf, char *accwinf,
			      int order, int T);
static void mlgparaCholFC(DMATRIX pdf, PStreamChol *pst, DMATRIX mlgp);
static void mlpgCholFC(PStreamChol *pst);
static void calc_R_and_r_FC(PStreamChol *pst);
static void CholeskiFC(PStreamChol *pst);
static void Choleski_forwardFC(PStreamChol *pst);
static void Choleski_backwardFC(PStreamChol *pst);
#endif

/**********************************/
/* ML Considering Global Variance */
/**********************************/
#if 0
static void varconv(double **c, const int m, const int T, const double var);
static void calc_varstats(double **c, const int m, const int T,
			  double *av, double *var, double *dif);
/* Diagonal Covariance Version */
static void mlgparaGrad(DMATRIX pdf, PStreamChol *pst, DMATRIX mlgp,
			const int max, double th, double e, double alpha,
			DVECTOR vm, DVECTOR vv, XBOOL nrmflag, XBOOL extvflag);
static void mlpgGrad(PStreamChol *pst, const int max, double th, double e,
		     double alpha, DVECTOR vm, DVECTOR vv, XBOOL nrmflag);
static void calc_grad(PStreamChol *pst, const int m);
static void calc_vargrad(PStreamChol *pst, const int m, double alpha, double n,
			 double vm, double vv);
static double get_gauss_dia5(double det,
		     double weight,
		     DVECTOR vec,		// dim
		     DVECTOR meanvec,		// dim
                      DVECTOR invcovvec);		// dim
#endif

static double get_gauss_full(long clsidx,
		      DVECTOR vec,		// [dim]
		      DVECTOR detvec,		// [clsnum]
		      DMATRIX weightmat,	// [clsnum][1]
		      DMATRIX meanvec,		// [clsnum][dim]
		      DMATRIX invcovmat);	// [clsnum * dim][dim]
static double get_gauss_dia(long clsidx,
		     DVECTOR vec,		// [dim]
		     DVECTOR detvec,		// [clsnum]
		     DMATRIX weightmat,		// [clsnum][1]
		     DMATRIX meanmat,		// [clsnum][dim]
		     DMATRIX invcovmat);	// [clsnum][dim]
static double cal_xmcxmc(long clsidx,
		  DVECTOR x,
		  DMATRIX mm,	// [num class][dim]
		  DMATRIX cm);	// [num class * dim][dim]

#if 0
static void get_gv_mlpgpara(MLPGPARA param, char *vmfile, char *vvfile,
			    long dim2, XBOOL msg_flag);
#endif

/* User level function */
cst_track *mlpg(const cst_track *param_track, cst_cg_db *cg_db);

#endif /* _MLPG_H */
