/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 2008                             */
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
/*               Date:  November 2008                                    */
/*************************************************************************/
/*                                                                       */
/*  Support for audio streaming.                                         */
/*                                                                       */
/*  When switched on you get called with cst_wave with a start and size  */
/*  at least as big as the requested buffer size (except for last chunk  */
/*                                                                       */
/*************************************************************************/

#include "cst_string.h"
#include "cst_wave.h"
#include "cst_audio.h"

CST_VAL_REGISTER_TYPE(audio_streaming_info,cst_audio_streaming_info)

cst_audio_streaming_info *new_audio_streaming_info(void)
{
    cst_audio_streaming_info *asi = 
        cst_alloc(struct cst_audio_streaming_info_struct,1);

    asi->min_buffsize = 256;
    asi->asc = NULL;
    asi->userdata = NULL;

    return asi;
}

void delete_audio_streaming_info(cst_audio_streaming_info *asi)
{
    if (asi)
        cst_free(asi);
    return;
}

int audio_stream_chunk(const cst_wave *w, int start, int size, 
                       int last, void *user)
{
    /* Called with new samples from start for size samples */
    /* last is true if this is the last segment. */
    /* This is really just and example that you can copy for you streaming */
    /* function */
    /* This particular example is *not* thread safe */
    int n;
    static cst_audiodev *ad = 0;

    if (start == 0)
        ad = audio_open(w->sample_rate,w->num_channels,CST_AUDIO_LINEAR16);

    n = audio_write(ad,&w->samples[start],size*sizeof(short));

    if (last == 1)
    {
        audio_close(ad);
        ad = NULL;
    }

    /* if you want to stop return CST_AUDIO_STREAM_STOP */
    return CST_AUDIO_STREAM_CONT;
}

