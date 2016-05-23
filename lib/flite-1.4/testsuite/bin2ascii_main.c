/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 2003                             */
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
/*               Date:  December 2003                                    */
/*************************************************************************/
/*                                                                       */
/*  Convert binary to ascii files (for tracks)                           */
/*************************************************************************/
#include <stdio.h>
#include "cst_wave.h"
#include "cst_tokenstream.h"
#include "cst_args.h"

int main(int argc, char **argv)
{
    cst_val *files;
    const cst_val *f;
    cst_features *args;
    int i,n;
    const char *datatype;
    FILE *fd;
    int channels;
    int d_int;
    float d_flt;
    double d_dbl;
    int swap = FALSE;

    args = new_features();
    files =
        cst_args(argv,argc,
                 "usage: bin2ascii OPTIONS\n"
                 "Convert a binary file to ascii\n"
		 "-type <string>  int, float or double\n"
		 "-swap           Swap data\n"
		 "-c <int>        Number of channels\n",
                 args);

    channels = get_param_int(args,"-c",12);
    if (feat_present(args,"-swap"))
	swap = TRUE;
    datatype = get_param_string(args,"-type","int");

    for (f=files; f; f=val_cdr(f))
    {
	fd = fopen(val_string(val_car(f)),"rb");
	n = 0;
	while (! feof(fd))
	{
	    for (i=0; i<channels; i++)
	    {
		if (cst_streq(datatype,"int"))
		{
		    n = fread(&d_int,4,1,fd);
		    if (n != 1) break;
		    if (swap) d_int = SWAPINT(d_int);
		    printf("%d ",d_int);
		}
		else if (cst_streq(datatype,"float"))
		{
		    n = fread(&d_flt,4,1,fd);
		    if (n != 1) break;
		    if (swap) swapfloat(&d_flt);
		    printf("%f ",d_flt);
		}
		else if (cst_streq(datatype,"double"))
		{
		    n = fread(&d_dbl,8,1,fd);
		    if (n != 1) break;
		    if (swap) swapdouble(&d_dbl);
		    printf("%g ",d_dbl);
		}
		else
		{
		    fprintf(stderr,"bin2ascii: unknown data type \"%s\"",datatype);
		    return -1;
		}
	    }
	    if (n==0 && i == 0)
		break;
	    printf("\n");
	}
    }

    return 0;
}
