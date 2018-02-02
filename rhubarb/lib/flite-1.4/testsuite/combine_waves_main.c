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
/*               Date:  November 2003                                    */
/*************************************************************************/
/*                                                                       */
/*  Combine multichannel waveforms into a single waveform file           */
/*************************************************************************/
#include <stdio.h>
#include "cst_wave.h"
#include "cst_tokenstream.h"
#include "cst_args.h"

float decode_time(const char *ctime)
{
    /* return number of seconds from time */

    return 0.0;
}

cst_val *get_wavelist(const char *wavelistfile)
{
    cst_val *l = 0;
    cst_tokenstream *ts;
    const char *token;
    int i=0;

    ts = ts_open(wavelistfile);
    if (!ts)
    {
	fprintf(stderr,"combine_waves: can't open \"%s\"\n",wavelistfile);
	return 0;
    }

    while ((token=ts_get(ts)) != 0)
    {
	l = cons_val(string_val(token),l);
	i++;
    }

    if (i%2 != 0)
    {
	fprintf(stderr,"combine_waves: doesn't have matched pairs \"%s\"\n",wavelistfile);
	delete_val(l);
	l = 0;
    }

    ts_close(ts);

    return val_reverse(l);
}


int main(int argc, char **argv)
{
    cst_wave *nw, *all;
    cst_val *files;
    const cst_val *w;
    cst_val *wavelist;
    cst_features *args;
    int i,j;
    float ntime;
    int stime;
    const char *nwfile;

    args = new_features();
    files =
        cst_args(argv,argc,
                 "usage: combine_waves OPTIONS\n"
                 "Combine waves into single waveform\n"
		 "-o <string>  Output waveform\n"
		 "-f <int>     Input sample rate (for raw input)\n"
		 "-itype <string>  Input type, raw or headered\n"
		 "-wavelist <string>  File containing times and wave filenames\n",
                 args);

    wavelist = get_wavelist(get_param_string(args,"-wavelist","-"));

    if (wavelist == 0)
	return -1;

    all = new_wave();
    for (w = wavelist; w; w = val_cdr(w))
    {
	ntime = decode_time(val_string(val_car(w)));
	nwfile = val_string(val_car(val_cdr(w)));

	nw = new_wave();
	if (cst_wave_load_riff(nw,nwfile) != CST_OK_FORMAT)
	{
	    fprintf(stderr,
		    "combine_waves: can't read file or wrong format \"%s\"\n",
		    nwfile);
	    continue;
	}

	stime = ntime * nw->sample_rate;

	cst_wave_resize(all,stime+nw->num_samples,1);
	
	for (i=0,j=stime; i<nw->num_samples; i++,j++)
	{
	    /* this will cause overflows */
	    all->samples[j] += nw->samples[i];
	}
	delete_wave(nw);
    }

    cst_wave_save_riff(all,get_param_string(args,"-o","-"));

    return 0;
}
