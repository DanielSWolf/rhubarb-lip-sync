/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 1999                             */
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
/*               Date:  August 2000                                      */
/*************************************************************************/
/*                                                                       */
/*  Tracks (cepstrum, ffts, F0 etc)                                      */
/*                                                                       */
/*************************************************************************/
#include "cst_string.h"
#include "cst_val.h"
#include "cst_track.h"

CST_VAL_REGISTER_TYPE(track,cst_track)

cst_track *new_track()
{
    cst_track *w = cst_alloc(struct cst_track_struct,1);
    return w;
}

void delete_track(cst_track *w)
{
    int i;

    if (w)
    {
	cst_free(w->times);
	for (i=0; i < w->num_frames; i++)
	    cst_free(w->frames[i]);
	cst_free(w->frames);
	cst_free(w);
    }
    return;
}

float track_frame_shift(cst_track *t, int frame)
{
    if (frame == 0)
	return t->times[frame];
    else
	return t->times[frame]-t->times[frame-1];
}

void cst_track_resize(cst_track *t,int num_frames, int num_channels)
{
    float *n_times;
    float **n_frames;
    int i;

    n_times = cst_alloc(float,num_frames);
    memmove(n_times,t->times,
	    (sizeof(float)*((num_frames < t->num_frames) ? 
			    num_frames : t->num_frames)));
    n_frames = cst_alloc(float*,num_frames);
    for (i=0; i<num_frames; i++)
    {
	n_frames[i] = cst_alloc(float,num_channels);
	if (i<t->num_frames)
	{
	    memmove(n_frames[i],
		    t->frames[i],
		    sizeof(float)*((num_channels < t->num_channels) ?
				   num_channels : t->num_channels));
	    cst_free(t->frames[i]);
	}
    }
    for (   ; i<t->num_frames; i++)
	cst_free(t->frames[i]);
    cst_free(t->frames);
    t->frames = n_frames;
    cst_free(t->times);
    t->times = n_times;
    t->num_frames = num_frames;
    t->num_channels = num_channels;
}



