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
/*                   Copyright (c) 2003-2004,2008                    */
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
/*  TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE    */
/*  OR PERFORMANCE OF THIS SOFTWARE.                                 */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/*          Author :  Tomoki Toda (tomoki@ics.nitech.ac.jp)          */
/*          Date   :  June 2004                                      */
/*                                                                   */
/*  Modified as a single file for inclusion in festival/flite        */
/*  May 2008 awb@cs.cmu.edu                                          */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  ML-Based Parameter Generation                                    */
/*                                                                   */
/*-------------------------------------------------------------------*/

#include "cst_alloc.h"
#include "cst_string.h"
#include "cst_math.h"
#include "cst_track.h"
#include "cst_wave.h"
#include "cst_vc.h"
#include "cst_mlpg.h"

#define mlpg_alloc(X,Y) (cst_alloc(Y,X))
#define mlpg_free cst_free

static MLPGPARA xmlpgpara_init(int dim, int dim2, int dnum, 
                               int clsnum)
{
    MLPGPARA param;
    
    // memory allocation
    param = mlpg_alloc(1,struct MLPGPARA_STRUCT);
    param->ov = xdvalloc(dim);
    param->iuv = NODATA;
    param->iumv = NODATA;
    param->flkv = xdvalloc(dnum);
    param->stm = NODATA;
    param->dltm = xdmalloc(dnum, dim2);
    param->pdf = NODATA;
    param->detvec = NODATA;
    param->wght = xdmalloc(clsnum, 1);
    param->mean = xdmalloc(clsnum, dim);
    param->cov = NODATA;
    param->clsidxv = NODATA;
    /* dia_flag */
	param->clsdetv = xdvalloc(1);
	param->clscov = xdmalloc(1, dim);

    param->vdet = 1.0;
    param->vm = NODATA;
    param->vv = NODATA;
    param->var = NODATA;

    return param;
}

static void xmlpgparafree(MLPGPARA param)
{
    if (param != NODATA) {
	if (param->ov != NODATA) xdvfree(param->ov);
	if (param->iuv != NODATA) xdvfree(param->iuv);
	if (param->iumv != NODATA) xdvfree(param->iumv);
	if (param->flkv != NODATA) xdvfree(param->flkv);
	if (param->stm != NODATA) xdmfree(param->stm);
	if (param->dltm != NODATA) xdmfree(param->dltm);
	if (param->pdf != NODATA) xdmfree(param->pdf);
	if (param->detvec != NODATA) xdvfree(param->detvec);
	if (param->wght != NODATA) xdmfree(param->wght);
	if (param->mean != NODATA) xdmfree(param->mean);
	if (param->cov != NODATA) xdmfree(param->cov);
	if (param->clsidxv != NODATA) xlvfree(param->clsidxv);
	if (param->clsdetv != NODATA) xdvfree(param->clsdetv);
	if (param->clscov != NODATA) xdmfree(param->clscov);
	if (param->vm != NODATA) xdvfree(param->vm);
	if (param->vv != NODATA) xdvfree(param->vv);
	if (param->var != NODATA) xdvfree(param->var);
	mlpg_free(param);
    }

    return;
}

static double get_like_pdfseq_vit(int dim, int dim2, int dnum, int clsnum,
                                  MLPGPARA param, 
                                  float **model, 
                                  XBOOL dia_flag)
{
    long d, c, k, l, j;
    double sumgauss;
    double like = 0.0;

    for (d = 0, like = 0.0; d < dnum; d++) {
	// read weight and mean sequences
        param->wght->data[0][0] = 0.9; /* FIXME weights */
        for (j=0; j<dim; j++)
            param->mean->data[0][j] = model[d][(j+1)*2];

	// observation vector
	for (k = 0; k < dim2; k++) {
	    param->ov->data[k] = param->stm->data[d][k];
	    param->ov->data[k + dim2] = param->dltm->data[d][k];
	}

	// mixture index
        c = d;
	param->clsdetv->data[0] = param->detvec->data[c];

	// calculating likelihood
	if (dia_flag == XTRUE) {
	    for (k = 0; k < param->clscov->col; k++)
		param->clscov->data[0][k] = param->cov->data[c][k];
	    sumgauss = get_gauss_dia(0, param->ov, param->clsdetv,
				     param->wght, param->mean, param->clscov);
	} else {
	    for (k = 0; k < param->clscov->row; k++)
		for (l = 0; l < param->clscov->col; l++)
		    param->clscov->data[k][l] =
			param->cov->data[k + param->clscov->row * c][l];
	    sumgauss = get_gauss_full(0, param->ov, param->clsdetv,
				      param->wght, param->mean, param->clscov);
	}
	if (sumgauss <= 0.0) param->flkv->data[d] = -1.0 * INFTY2;
	else param->flkv->data[d] = log(sumgauss);
	like += param->flkv->data[d];

	// estimating U', U'*M
	if (dia_flag == XTRUE) {
	    // PDF [U'*M U']
	    for (k = 0; k < dim; k++) {
		param->pdf->data[d][k] =
		    param->clscov->data[0][k] * param->mean->data[0][k];
		param->pdf->data[d][k + dim] = param->clscov->data[0][k];
	    }
	} else {
	    // PDF [U'*M U']
	    for (k = 0; k < dim; k++) {
		param->pdf->data[d][k] = 0.0;
		for (l = 0; l < dim; l++) {
		    param->pdf->data[d][k * dim + dim + l] =
			param->clscov->data[k][l];
		    param->pdf->data[d][k] +=
			param->clscov->data[k][l] * param->mean->data[0][l];
		}
	    }
	}
    }

    like /= (double)dnum;

    return like;
}

#if 0
static double get_like_gv(long dim2, long dnum, MLPGPARA param)
{
    long k;
    double av = 0.0, dif = 0.0;
    double vlike = -INFTY;

    if (param->vm != NODATA && param->vv != NODATA) {
	for (k = 0; k < dim2; k++)
	    calc_varstats(param->stm->data, k, dnum, &av,
			  &(param->var->data[k]), &dif);
	vlike = log(get_gauss_dia5(param->vdet, 1.0, param->var,
                                   param->vm, param->vv));
    }

    return vlike;
}

static void sm_mvav(DMATRIX mat, long hlen)
{
    long k, l, m, p;
    double d, sd;
    DVECTOR vec = NODATA;
    DVECTOR win = NODATA;

    vec = xdvalloc(mat->row);

    // smoothing window
    win = xdvalloc(hlen * 2 + 1);
    for (k = 0, d = 1.0, sd = 0.0; k < hlen; k++, d += 1.0) {
	win->data[k] = d;	win->data[win->length - k - 1] = d;
	sd += d + d;
    }
    win->data[k] = d;	sd += d;
    for (k = 0; k < win->length; k++) win->data[k] /= sd;

    for (l = 0; l < mat->col; l++) {
	for (k = 0; k < mat->row; k++) {
	    for (m = 0, vec->data[k] = 0.0; m < win->length; m++) {
		p = k - hlen + m;
		if (p >= 0 && p < mat->row)
		    vec->data[k] += mat->data[p][l] * win->data[m];
	    }
	}
	for (k = 0; k < mat->row; k++) mat->data[k][l] = vec->data[k];
    }

    xdvfree(win);
    xdvfree(vec);

    return;
}
#endif

static void get_dltmat(DMATRIX mat, DWin *dw, int dno, DMATRIX dmat)
{
    int i, j, k, tmpnum;

    tmpnum = (int)mat->row - dw->width[dno][WRIGHT];
    for (k = dw->width[dno][WRIGHT]; k < tmpnum; k++)	// time index
	for (i = 0; i < (int)mat->col; i++)	// dimension index
	    for (j = dw->width[dno][WLEFT], dmat->data[k][i] = 0.0;
		 j <= dw->width[dno][WRIGHT]; j++)
		dmat->data[k][i] += mat->data[k + j][i] * dw->coef[dno][j];

    for (i = 0; i < (int)mat->col; i++) {		// dimension index
	for (k = 0; k < dw->width[dno][WRIGHT]; k++)		// time index
	    for (j = dw->width[dno][WLEFT], dmat->data[k][i] = 0.0;
		 j <= dw->width[dno][WRIGHT]; j++)
		if (k + j >= 0)
		    dmat->data[k][i] += mat->data[k + j][i] * dw->coef[dno][j];
		else
		    dmat->data[k][i] += (2.0 * mat->data[0][i] - mat->data[-k - j][i]) * dw->coef[dno][j];
	for (k = tmpnum; k < (int)mat->row; k++)	// time index
	    for (j = dw->width[dno][WLEFT], dmat->data[k][i] = 0.0;
		 j <= dw->width[dno][WRIGHT]; j++)
		if (k + j < (int)mat->row)
		    dmat->data[k][i] += mat->data[k + j][i] * dw->coef[dno][j];
		else
		    dmat->data[k][i] += (2.0 * mat->data[mat->row - 1][i] - mat->data[mat->row - k - j + mat->row - 2][i]) * dw->coef[dno][j];
    }

    return;
}


static double *dcalloc(int x, int xoff)
{
    double *ptr;

    ptr = mlpg_alloc(x,double);
    /* ptr += xoff; */ /* Just not going to allow this */
    return(ptr);
}

static double **ddcalloc(int x, int y, int xoff, int yoff)
{
    double **ptr;
    register int i;

    ptr = mlpg_alloc(x,double *);
    for (i = 0; i < x; i++) ptr[i] = dcalloc(y, yoff);
    /* ptr += xoff; */ /* Just not going to allow this */
    return(ptr);
}

/////////////////////////////////////
// ML using Choleski decomposition //
/////////////////////////////////////
static void InitDWin(PStreamChol *pst, const float *dynwin, int fsize)
{
    int i,j;
    int leng;

    pst->dw.num = 1;	// only static
    if (dynwin) {
	pst->dw.num = 2;	// static + dyn
    }
    // memory allocation
    pst->dw.width = mlpg_alloc(pst->dw.num,int *);
    for (i = 0; i < pst->dw.num; i++)
        pst->dw.width[i] = mlpg_alloc(2,int);

    pst->dw.coef = mlpg_alloc(pst->dw.num, double *);
    pst->dw.coef_ptrs = mlpg_alloc(pst->dw.num, double *);
    // window for static parameter	WLEFT = 0, WRIGHT = 1
    pst->dw.width[0][WLEFT] = pst->dw.width[0][WRIGHT] = 0;
    pst->dw.coef_ptrs[0] = mlpg_alloc(1,double);
    pst->dw.coef[0] = pst->dw.coef_ptrs[0];
    pst->dw.coef[0][0] = 1.0;

    // set delta coefficients
    for (i = 1; i < pst->dw.num; i++) {
        pst->dw.coef_ptrs[i] = mlpg_alloc(fsize, double);
	pst->dw.coef[i] = pst->dw.coef_ptrs[i];
        for (j=0; j<fsize; j++) /* FIXME make dynwin doubles for memmove */
            pst->dw.coef[i][j] = (double)dynwin[j];
	// set pointer
	leng = fsize / 2;			// L (fsize = 2 * L + 1)
	pst->dw.coef[i] += leng;		// [L] -> [0]	center
	pst->dw.width[i][WLEFT] = -leng;	// -L		left
	pst->dw.width[i][WRIGHT] = leng;	//  L		right
	if (fsize % 2 == 0) pst->dw.width[i][WRIGHT]--;
    }

    pst->dw.maxw[WLEFT] = pst->dw.maxw[WRIGHT] = 0;
    for (i = 0; i < pst->dw.num; i++) {
	if (pst->dw.maxw[WLEFT] > pst->dw.width[i][WLEFT])
	    pst->dw.maxw[WLEFT] = pst->dw.width[i][WLEFT];
	if (pst->dw.maxw[WRIGHT] < pst->dw.width[i][WRIGHT])
	    pst->dw.maxw[WRIGHT] = pst->dw.width[i][WRIGHT];
    }

    return;
}

static void InitPStreamChol(PStreamChol *pst, const float *dynwin, int fsize,
                            int order, int T)
{
    // order of cepstrum
    pst->order = order;

    // windows for dynamic feature
    InitDWin(pst, dynwin, fsize);

    // dimension of observed vector
    pst->vSize = (pst->order + 1) * pst->dw.num;	// odim = dim * (1--3)

    // memory allocation
    pst->T = T;					// number of frames
    pst->width = pst->dw.maxw[WRIGHT] * 2 + 1;	// width of R
    pst->mseq = ddcalloc(T, pst->vSize, 0, 0);	// [T][odim] 
    pst->ivseq = ddcalloc(T, pst->vSize, 0, 0);	// [T][odim]
    pst->R = ddcalloc(T, pst->width, 0, 0);	// [T][width]
    pst->r = dcalloc(T, 0);			// [T]
    pst->g = dcalloc(T, 0);			// [T]
    pst->c = ddcalloc(T, pst->order + 1, 0, 0);	// [T][dim]

    return;
}

static void mlgparaChol(DMATRIX pdf, PStreamChol *pst, DMATRIX mlgp)
{
    int t, d;

    // error check
    if (pst->vSize * 2 != pdf->col || pst->order + 1 != mlgp->col) {
	cst_errmsg("Error mlgparaChol: Different dimension\n");
        cst_error();
    }

    // mseq: U^{-1}*M,	ifvseq: U^{-1}
    for (t = 0; t < pst->T; t++) {
	for (d = 0; d < pst->vSize; d++) {
	    pst->mseq[t][d] = pdf->data[t][d];
	    pst->ivseq[t][d] = pdf->data[t][pst->vSize + d];
	}
    } 

    // ML parameter generation
    mlpgChol(pst);

    // extracting parameters
    for (t = 0; t < pst->T; t++)
	for (d = 0; d <= pst->order; d++)
	    mlgp->data[t][d] = pst->c[t][d];

    return;
}

// generate parameter sequence from pdf sequence using Choleski decomposition
static void mlpgChol(PStreamChol *pst)
{
   register int m;

   // generating parameter in each dimension
   for (m = 0; m <= pst->order; m++) {
       calc_R_and_r(pst, m);
       Choleski(pst);
       Choleski_forward(pst);
       Choleski_backward(pst, m);
   }
   
   return;
}

//------ parameter generation fuctions
// calc_R_and_r: calculate R = W'U^{-1}W and r = W'U^{-1}M
static void calc_R_and_r(PStreamChol *pst, const int m)
{
    register int i, j, k, l, n;
    double   wu;
   
    for (i = 0; i < pst->T; i++) {
	pst->r[i] = pst->mseq[i][m];
	pst->R[i][0] = pst->ivseq[i][m];
      
	for (j = 1; j < pst->width; j++) pst->R[i][j] = 0.0;
      
	for (j = 1; j < pst->dw.num; j++) {
	    for (k = pst->dw.width[j][0]; k <= pst->dw.width[j][1]; k++) {
		n = i + k;
		if (n >= 0 && n < pst->T && pst->dw.coef[j][-k] != 0.0) {
		    l = j * (pst->order + 1) + m;
		    pst->r[i] += pst->dw.coef[j][-k] * pst->mseq[n][l]; 
		    wu = pst->dw.coef[j][-k] * pst->ivseq[n][l];
            
		    for (l = 0; l < pst->width; l++) {
			n = l-k;
			if (n <= pst->dw.width[j][1] && i + l < pst->T &&
			    pst->dw.coef[j][n] != 0.0)
			    pst->R[i][l] += wu * pst->dw.coef[j][n];
		    }
		}
	    }
	}
    }

    return;
}

// Choleski: Choleski factorization of Matrix R
static void Choleski(PStreamChol *pst)
{
    register int t, j, k;

    pst->R[0][0] = sqrt(pst->R[0][0]);

    for (j = 1; j < pst->width; j++) pst->R[0][j] /= pst->R[0][0];

    for (t = 1; t < pst->T; t++) {
	for (j = 1; j < pst->width; j++)
	    if (t - j >= 0)
		pst->R[t][0] -= pst->R[t - j][j] * pst->R[t - j][j];
         
	pst->R[t][0] = sqrt(pst->R[t][0]);
         
	for (j = 1; j < pst->width; j++) {
	    for (k = 0; k < pst->dw.maxw[WRIGHT]; k++)
		if (j != pst->width - 1)
		    pst->R[t][j] -=
			pst->R[t - k - 1][j - k] * pst->R[t - k - 1][j + 1];
            
	    pst->R[t][j] /= pst->R[t][0];
	}
    }
   
    return;
}

// Choleski_forward: forward substitution to solve linear equations
static void Choleski_forward(PStreamChol *pst)
{
    register int t, j;
    double hold;
   
    pst->g[0] = pst->r[0] / pst->R[0][0];

    for (t=1; t < pst->T; t++) {
	hold = 0.0;
	for (j = 1; j < pst->width; j++)
	    if (t - j >= 0 && pst->R[t - j][j] != 0.0)
		hold += pst->R[t - j][j] * pst->g[t - j];
	pst->g[t] = (pst->r[t] - hold) / pst->R[t][0];
    }
   
    return;
}

// Choleski_backward: backward substitution to solve linear equations
static void Choleski_backward(PStreamChol *pst, const int m)
{
    register int t, j;
    double hold;
   
    pst->c[pst->T - 1][m] = pst->g[pst->T - 1] / pst->R[pst->T - 1][0];

    for (t = pst->T - 2; t >= 0; t--) {
	hold = 0.0;
	for (j = 1; j < pst->width; j++)
	    if (t + j < pst->T && pst->R[t][j] != 0.0)
		hold += pst->R[t][j] * pst->c[t + j][m];
	pst->c[t][m] = (pst->g[t] - hold) / pst->R[t][0];
   }
   
   return;
}

////////////////////////////////////
// ML Considering Global Variance //
////////////////////////////////////
#if 0
static void varconv(double **c, const int m, const int T, const double var)
{
    register int n;
    double sd, osd;
    double oav = 0.0, ovar = 0.0, odif = 0.0;

    calc_varstats(c, m, T, &oav, &ovar, &odif);
    osd = sqrt(ovar);	sd = sqrt(var);
    for (n = 0; n < T; n++)
	c[n][m] = (c[n][m] - oav) / osd * sd + oav;

    return;
}

static void calc_varstats(double **c, const int m, const int T,
		   double *av, double *var, double *dif)
{
    register int i;
    register double d;

    *av = 0.0;
    *var = 0.0;
    *dif = 0.0;
    for (i = 0; i < T; i++) *av += c[i][m];
    *av /= (double)T;
    for (i = 0; i < T; i++) {
	d = c[i][m] - *av;
	*var += d * d;	*dif += d;
    }
    *var /= (double)T;

    return;
}

// Diagonal Covariance Version
static void mlgparaGrad(DMATRIX pdf, PStreamChol *pst, DMATRIX mlgp, const int max,
		 double th, double e, double alpha, DVECTOR vm, DVECTOR vv,
		 XBOOL nrmflag, XBOOL extvflag)
{
    int t, d;

    // error check
    if (pst->vSize * 2 != pdf->col || pst->order + 1 != mlgp->col) {
	cst_errmsg("Error mlgparaChol: Different dimension\n");
        cst_error();
    }

    // mseq: U^{-1}*M,	ifvseq: U^{-1}
    for (t = 0; t < pst->T; t++) {
	for (d = 0; d < pst->vSize; d++) {
	    pst->mseq[t][d] = pdf->data[t][d];
	    pst->ivseq[t][d] = pdf->data[t][pst->vSize + d];
	}
    } 

    // ML parameter generation
    mlpgChol(pst);

    // extend variance
    if (extvflag == XTRUE)
	for (d = 0; d <= pst->order; d++)
	    varconv(pst->c, d, pst->T, vm->data[d]);

    // estimating parameters
    mlpgGrad(pst, max, th, e, alpha, vm, vv, nrmflag);

    // extracting parameters
    for (t = 0; t < pst->T; t++)
	for (d = 0; d <= pst->order; d++)
	    mlgp->data[t][d] = pst->c[t][d];

    return;
}

// generate parameter sequence from pdf sequence using gradient
static void mlpgGrad(PStreamChol *pst, const int max, double th, double e,
	      double alpha, DVECTOR vm, DVECTOR vv, XBOOL nrmflag)
{
   register int m, i, t;
   double diff, n, dth;

   if (nrmflag == XTRUE)
       n = (double)(pst->T * pst->vSize) / (double)(vm->length);
   else n = 1.0;

   // generating parameter in each dimension
   for (m = 0; m <= pst->order; m++) {
       calc_R_and_r(pst, m);
       dth = th * sqrt(vm->data[m]);
       for (i = 0; i < max; i++) {
	   calc_grad(pst, m);
	   if (vm != NODATA && vv != NODATA)
	       calc_vargrad(pst, m, alpha, n, vm->data[m], vv->data[m]);
	   for (t = 0, diff = 0.0; t < pst->T; t++) {
	       diff += pst->g[t] * pst->g[t];
	       pst->c[t][m] += e * pst->g[t];
	   }
	   diff = sqrt(diff / (double)pst->T);
	   if (diff < dth || diff == 0.0) break;
       }
   }
   
   return;
}

// calc_grad: calculate -RX + r = -W'U^{-1}W * X + W'U^{-1}M
void calc_grad(PStreamChol *pst, const int m)
{
    register int i, j;

    for (i = 0; i < pst->T; i++) {
	pst->g[i] = pst->r[i] - pst->c[i][m] * pst->R[i][0];
	for (j = 1; j < pst->width; j++) {
	    if (i + j < pst->T) pst->g[i] -= pst->c[i + j][m] * pst->R[i][j];
	    if (i - j >= 0) pst->g[i] -= pst->c[i - j][m] * pst->R[i - j][j];
	}
    }

    return;
}

static void calc_vargrad(PStreamChol *pst, const int m, double alpha, double n,
		  double vm, double vv)
{
    register int i;
    double vg, w1, w2;
    double av = 0.0, var = 0.0, dif = 0.0;

    if (alpha > 1.0 || alpha < 0.0) {
	w1 = 1.0;		w2 = 1.0;
    } else {
	w1 = alpha;	w2 = 1.0 - alpha;
    }

    calc_varstats(pst->c, m, pst->T, &av, &var, &dif);

    for (i = 0; i < pst->T; i++) {
	vg = -(var - vm) * (pst->c[i][m] - av) * vv * 2.0 / (double)pst->T;
	pst->g[i] = w1 * pst->g[i] / n + w2 * vg;
    }

    return;
}
#endif

// diagonal covariance
static DVECTOR xget_detvec_diamat2inv(DMATRIX covmat)	// [num class][dim]
{
    long dim, clsnum;
    long i, j;
    double det;
    DVECTOR detvec = NODATA;

    clsnum = covmat->row;
    dim = covmat->col;
    // memory allocation
    detvec = xdvalloc(clsnum);
    for (i = 0; i < clsnum; i++) {
	for (j = 0, det = 1.0; j < dim; j++) {
	    det *= covmat->data[i][j];
	    if (det > 0.0) {
		covmat->data[i][j] = 1.0 / covmat->data[i][j];
	    } else {
		cst_errmsg("error:(class %ld) determinant <= 0, det = %f\n", i, det);
                xdvfree(detvec);
		return NODATA;
	    }
	}
	detvec->data[i] = det;
    }

    return detvec;
}

static double get_gauss_full(long clsidx,
		      DVECTOR vec,		// [dim]
		      DVECTOR detvec,		// [clsnum]
		      DMATRIX weightmat,	// [clsnum][1]
		      DMATRIX meanvec,		// [clsnum][dim]
		      DMATRIX invcovmat)	// [clsnum * dim][dim]
{
    double gauss;

    if (detvec->data[clsidx] <= 0.0) {
	cst_errmsg("#error: det <= 0.0\n");
        cst_error();
    }

    gauss = weightmat->data[clsidx][0]
	/ sqrt(pow(2.0 * PI, (double)vec->length) * detvec->data[clsidx])
	* exp(-1.0 * cal_xmcxmc(clsidx, vec, meanvec, invcovmat) / 2.0);
    
    return gauss;
}

static double cal_xmcxmc(long clsidx,
		  DVECTOR x,
		  DMATRIX mm,	// [num class][dim]
		  DMATRIX cm)	// [num class * dim][dim]
{
    long clsnum, k, l, b, dim;
    double *vec = NULL;
    double td, d;

    dim = x->length;
    clsnum = mm->row;
    b = clsidx * dim;
    if (mm->col != dim || cm->col != dim || clsnum * dim != cm->row) {
	cst_errmsg("Error cal_xmcxmc: different dimension\n");
        cst_error();
    }

    // memory allocation
    vec = mlpg_alloc((int)dim, double);
    for (k = 0; k < dim; k++) vec[k] = x->data[k] - mm->data[clsidx][k];
    for (k = 0, d = 0.0; k < dim; k++) {
	for (l = 0, td = 0.0; l < dim; l++) td += vec[l] * cm->data[l + b][k];
	d += td * vec[k];
    }
    // memory free
    mlpg_free(vec); vec = NULL;

    return d;
}

#if 0
// diagonal covariance
static double get_gauss_dia5(double det,
		     double weight,
		     DVECTOR vec,		// dim
		     DVECTOR meanvec,		// dim
		     DVECTOR invcovvec)		// dim
{
    double gauss, sb;
    long k;

    if (det <= 0.0) {
	cst_errmsg("#error: det <= 0.0\n");
        cst_error();
    }

    for (k = 0, gauss = 0.0; k < vec->length; k++) {
	sb = vec->data[k] - meanvec->data[k];
	gauss += sb * invcovvec->data[k] * sb;
    }

    gauss = weight / sqrt(pow(2.0 * PI, (double)vec->length) * det)
	* exp(-gauss / 2.0);

    return gauss;
}
#endif

static double get_gauss_dia(long clsidx,
		     DVECTOR vec,		// [dim]
		     DVECTOR detvec,		// [clsnum]
		     DMATRIX weightmat,		// [clsnum][1]
		     DMATRIX meanmat,		// [clsnum][dim]
		     DMATRIX invcovmat)		// [clsnum][dim]
{
    double gauss, sb;
    long k;

    if (detvec->data[clsidx] <= 0.0) {
	cst_errmsg("#error: det <= 0.0\n");
        cst_error();
    }

    for (k = 0, gauss = 0.0; k < vec->length; k++) {
	sb = vec->data[k] - meanmat->data[clsidx][k];
	gauss += sb * invcovmat->data[clsidx][k] * sb;
    }

    gauss = weightmat->data[clsidx][0]
	/ sqrt(pow(2.0 * PI, (double)vec->length) * detvec->data[clsidx])
	* exp(-gauss / 2.0);

    return gauss;
}

static void pst_free(PStreamChol *pst)
{
    int i;

    for (i=0; i<pst->dw.num; i++)
        mlpg_free(pst->dw.width[i]);
    mlpg_free(pst->dw.width); pst->dw.width = NULL;
    for (i=0; i<pst->dw.num; i++)
        mlpg_free(pst->dw.coef_ptrs[i]);
    mlpg_free(pst->dw.coef); pst->dw.coef = NULL;
    mlpg_free(pst->dw.coef_ptrs); pst->dw.coef_ptrs = NULL;

    for (i=0; i<pst->T; i++)
        mlpg_free(pst->mseq[i]);
    mlpg_free(pst->mseq);
    for (i=0; i<pst->T; i++)
        mlpg_free(pst->ivseq[i]);
    mlpg_free(pst->ivseq);
    for (i=0; i<pst->T; i++)
        mlpg_free(pst->R[i]);
    mlpg_free(pst->R);
    mlpg_free(pst->r);
    mlpg_free(pst->g);
    for (i=0; i<pst->T; i++)
        mlpg_free(pst->c[i]);
    mlpg_free(pst->c);

    return;
}

cst_track *mlpg(const cst_track *param_track, cst_cg_db *cg_db)
{
    /* Generate an (mcep) track using Maximum Likelihood Parameter Generation */
    MLPGPARA param = NODATA;
    cst_track *out;
    int dim, dim_st;
    float like;
    int i,j;
    int nframes;
    PStreamChol pst;

    nframes = param_track->num_frames;
    dim = (param_track->num_channels/2)-1;
    dim_st = dim/2; /* dim2 in original code */
    out = new_track();
    cst_track_resize(out,nframes,dim_st+1);

    param = xmlpgpara_init(dim,dim_st,nframes,nframes);

    // mixture-index sequence
    param->clsidxv = xlvalloc(nframes);
    for (i=0; i<nframes; i++)
        param->clsidxv->data[i] = i;

    // initial static feature sequence
    param->stm = xdmalloc(nframes,dim_st);
    for (i=0; i<nframes; i++)
    {
        for (j=0; j<dim_st; j++)
            param->stm->data[i][j] = param_track->frames[i][(j+1)*2];
    }

    /* Load cluster means */
    for (i=0; i<nframes; i++)
        for (j=0; j<dim_st; j++)
            param->mean->data[i][j] = param_track->frames[i][(j+1)*2];
    
    /* GMM parameters diagonal covariance */
    InitPStreamChol(&pst, cg_db->dynwin, cg_db->dynwinsize, dim_st-1, nframes);
    param->pdf = xdmalloc(nframes,dim*2);
    param->cov = xdmalloc(nframes,dim);
    for (i=0; i<nframes; i++)
        for (j=0; j<dim; j++)
            param->cov->data[i][j] = 
                param_track->frames[i][(j+1)*2+1] *
                param_track->frames[i][(j+1)*2+1];
    param->detvec = xget_detvec_diamat2inv(param->cov);

    /* global variance parameters */
    /* TBD get_gv_mlpgpara(param, vmfile, vvfile, dim2, msg_flag); */

    get_dltmat(param->stm, &pst.dw, 1, param->dltm);

    like = get_like_pdfseq_vit(dim, dim_st, nframes, nframes, param,
			       param_track->frames, XTRUE);

    /* vlike = get_like_gv(dim2, dnum, param); */

    mlgparaChol(param->pdf, &pst, param->stm);

    /* Put the answer back into the output track */
    for (i=0; i<nframes; i++)
    {
        out->times[i] = param_track->times[i];
        out->frames[i][0] = param_track->frames[i][0]; /* F0 */
        for (j=0; j<dim_st; j++)
            out->frames[i][j+1] = param->stm->data[i][j];
    }

    // memory free
    xmlpgparafree(param);
    pst_free(&pst);

    return out;
}

