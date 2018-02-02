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
/* Functions shared between mlpg and mlsa                            */
/*-------------------------------------------------------------------*/

#ifndef __CST_VC_H
#define __CST_VC_H

typedef struct LVECTOR_STRUCT {
    long length;
    long *data;
    long *imag;
} *LVECTOR;

typedef struct DVECTOR_STRUCT {
    long length;
    double *data;
    double *imag;
} *DVECTOR;

typedef struct DMATRIX_STRUCT {
    long row;
    long col;
    double **data;
    double **imag;
} *DMATRIX;

#define XBOOL int
#define XTRUE 1
#define XFALSE 0

#define NODATA NULL

#define FABS(x) ((x) >= 0.0 ? (x) : -(x))
#define LABS(x) ((x) >= 0 ? (x) : -(x))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define xdvnull() xdvalloc(0)

#define xdvnums(length, value) xdvinit((double)(value), 0.0, (double)(length))
#define xdvzeros(length) xdvnums(length, 0.0)

LVECTOR xlvalloc(long length);
void xlvfree(LVECTOR x);
DVECTOR xdvalloc(long length);
DVECTOR xdvcut(DVECTOR x, long offset, long length);
void xdvfree(DVECTOR vector);
double dvmax(DVECTOR x, long *index);
double dvmin(DVECTOR x, long *index);
DMATRIX xdmalloc(long row, long col);
void xdmfree(DMATRIX matrix);
DVECTOR xdvinit(double j, double incr, double n);

double dvsum(DVECTOR x);

#define RANDMAX 32767 
#define   B0         0x00000001
#define   B28        0x10000000
#define   B31        0x80000000
#define   B31_       0x7fffffff
#define   Z          0x00000000

typedef enum {MFALSE, MTRUE} Boolean;

#endif /* __CST_VC_H */
