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
/*   This is Zen's MLSA filter as ported by Toda to festvox vc        */
/*   and back ported into hts/festival so we can do MLSA filtering   */
/*   If I took more time I could probably make this use the same as  */
/*   as the other code in this directory -- awb@cs.cmu.edu 03JAN06   */
/*  ---------------------------------------------------------------  */
/*   and then ported into Flite (November 2007 awb@cs.cmu.edu)       */

/*********************************************************************/
/*                                                                   */
/*  vector (etc) code common to mlpg and mlsa                        */
/*-------------------------------------------------------------------*/

#include "cst_alloc.h"
#include "cst_string.h"
#include "cst_math.h"
#include "cst_vc.h"

/* from vector.cc */

LVECTOR xlvalloc(long length)
{
    LVECTOR x;

    length = MAX(length, 0);
    x = cst_alloc(struct LVECTOR_STRUCT,1);
    x->data = cst_alloc(long,MAX(length, 1));
    x->imag = NULL;
    x->length = length;

    return x;
}

void xlvfree(LVECTOR x)
{
    if (x != NULL) {
	if (x->data != NULL) {
	    cst_free(x->data);
	}
	if (x->imag != NULL) {
	    cst_free(x->imag);
	}
	cst_free(x);
    }

    return;
}

DVECTOR xdvalloc(long length)
{
    DVECTOR x;

    length = MAX(length, 0);
    x = cst_alloc(struct DVECTOR_STRUCT,1);
    x->data = cst_alloc(double,MAX(length, 1));
    x->imag = NULL;
    x->length = length;

    return x;
}

void xdvfree(DVECTOR x)
{
    if (x != NULL) {
	if (x->data != NULL) {
	    cst_free(x->data);
	}
	if (x->imag != NULL) {
	    cst_free(x->imag);
	}
	cst_free(x);
    }

    return;
}

void dvialloc(DVECTOR x)
{
    if (x->imag != NULL) {
	cst_free(x->imag);
    }
    x->imag = cst_alloc(double,x->length);

    return;
}

DVECTOR xdvcut(DVECTOR x, long offset, long length)
{
    long k;
    long pos;
    DVECTOR y;
    
    y = xdvalloc(length);
    if (x->imag != NULL) {
	dvialloc(y);
    }

    for (k = 0; k < y->length; k++) {
	pos = k + offset;
	if (pos >= 0 && pos < x->length) {
	    y->data[k] = x->data[pos];
	    if (y->imag != NULL) {
		y->imag[k] = x->imag[pos];
	    }
	} else {
	    y->data[k] = 0.0;
	    if (y->imag != NULL) {
		y->imag[k] = 0.0;
	    }
	}
    }

    return y;
}

DMATRIX xdmalloc(long row, long col)
{
    DMATRIX matrix;
    int i;

    matrix = cst_alloc(struct DMATRIX_STRUCT,1);
    matrix->data = cst_alloc(double *,row);
    for (i=0; i<row; i++)
        matrix->data[i] = cst_alloc(double,col);
    matrix->imag = NULL;
    matrix->row = row;
    matrix->col = col;

    return matrix;
}

void xdmfree(DMATRIX matrix)
{
    int i;

    if (matrix != NULL) {
	if (matrix->data != NULL) {
            for (i=0; i<matrix->row; i++)
                cst_free(matrix->data[i]);
            cst_free(matrix->data);
	}
	if (matrix->imag != NULL) {
            for (i=0; i<matrix->row; i++)
                cst_free(matrix->imag[i]);
            cst_free(matrix->imag);
	}
	cst_free(matrix);
    }

    return;
}

DVECTOR xdvinit(double j, double incr, double n)
{
    long k;
    long num;
    DVECTOR x;

    if ((incr > 0.0 && j > n) || (incr < 0.0 && j < n)) {
	x = xdvnull();
	return x;
    }
    if (incr == 0.0) {
	num = (long)n;
	if (num <= 0) {
	    x = xdvnull();
	    return x;
	}
    } else {
	num = LABS((long)((n - j) / incr)) + 1;
    }
    
    /* memory allocate */
    x = xdvalloc(num);

    /* initailize data */
    for (k = 0; k < x->length; k++) {
	x->data[k] = j + (k * incr);
    }

    return x;
}

/* from voperate.cc */
double dvmax(DVECTOR x, long *index)
{
    long k;
    long ind;
    double max;

    ind = 0;
    max = x->data[ind];
    for (k = 1; k < x->length; k++) {
	if (max < x->data[k]) {
	    ind = k;
	    max = x->data[k];
	}
    }

    if (index != NULL) {
	*index = ind;
    }

    return max;
}

double dvmin(DVECTOR x, long *index)
{
    long k;
    long ind;
    double min;

    ind = 0;
    min = x->data[ind];
    for (k = 1; k < x->length; k++) {
	if (min > x->data[k]) {
	    ind = k;
	    min = x->data[k];
	}
    }

    if (index != NULL) {
	*index = ind;
    }

    return min;
}

double dvsum(DVECTOR x)
{
    long k;
    double sum;

    for (k = 0, sum = 0.0; k < x->length; k++) {
	sum += x->data[k];
    }

    return sum;
}
