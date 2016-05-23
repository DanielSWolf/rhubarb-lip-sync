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
/*  A synth test program                                                 */
/*                                                                       */
/*************************************************************************/

#include <stdio.h>
#include <string.h>
#include "flite.h"

cst_voice *register_cmu_us_kal();

int main(int argc, char **argv)
{
    cst_voice *v;
    cst_wave *w;
    cst_utterance *u;
    int i;
    float durs;
    int *fff;

/*    putenv("MALLOC_TRACE=mallfile");
      mtrace(); */

    if (argc != 3)
    {
	fprintf(stderr,"usage: TEXT WAVEFILE\n");
	return 1;
    }

    flite_init();

    v = register_cmu_us_kal();
    durs = 0.0;

    for (i=0; i<2; i++)
    {
	u = flite_synth_text(argv[1],v);
	w = utt_wave(u);
	durs += (float)w->num_samples/(float)w->sample_rate;
	
	if (cst_streq(argv[2],"play"))
	    play_wave(w);
	else if (!cst_streq(argv[2],"none"))
	    cst_wave_save_riff(w,argv[2]);
	delete_utterance(u);
    }
    printf("%f seconds of speech synthesized\n",durs);
/*    muntrace(); */

    return 0;
}
