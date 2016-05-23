/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 2004                             */
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
/*               Date:  July 2004                                        */
/*************************************************************************/
/*                                                                       */
/*  Audio support for PalmOS                                             */
/*                                                                       */
/*************************************************************************/

#include "cst_file.h"
#include <stdlib.h>
#include <sys/types.h>
#include "cst_string.h"
#include "cst_wave.h"
#include "cst_audio.h"

#include <System/SoundMgr.h>

cst_audiodev *audio_open_palmos(int sps, int channels, cst_audiofmt fmt)
{
    cst_audiodev *ad;
    int afmt;
    int sfmts;
    int afd;
    int frag;

    ad = cst_alloc(cst_audiodev, 1);
    ad->sps = sps;
    ad->channels = channels;
    ad->fmt = fmt;
    ad->platform_data = NULL;

    if (ad->channels == 0)
	ad->channels = 1;

    ad->real_sps = ad->sps;

    ad->real_channels = ad->channels;

#if 0
    if (fmt == CST_AUDIO_LINEAR8 && (sfmts & AFMT_U8))
    {
	ad->real_fmt = CST_AUDIO_LINEAR8;
	afmt = AFMT_U8;
    }
    else if (fmt == CST_AUDIO_MULAW && (sfmts & AFMT_MU_LAW))
    {
	ad->real_fmt = CST_AUDIO_MULAW;
	afmt = AFMT_MU_LAW;
    }
    else if (CST_LITTLE_ENDIAN)
    {
	if (sfmts & AFMT_S16_LE)
	{
	    ad->real_fmt = CST_AUDIO_LINEAR16;
	    afmt = AFMT_S16_LE;
	}
	else if (sfmts & AFMT_S16_BE) /* not likely */
	{
	    ad->byteswap = 1;
	    ad->real_fmt = CST_AUDIO_LINEAR16;
	    afmt = AFMT_S16_BE;
	}
	else if (sfmts & AFMT_U8)
	{
	    afmt = AFMT_U8;
	    ad->real_fmt = CST_AUDIO_LINEAR8;
	}
	else
	{
	    cst_free(ad);
	    close(afd);
	    return NULL;
	}
    }
    else
    {
	if (sfmts & AFMT_S16_BE)
	{
	    ad->real_fmt = CST_AUDIO_LINEAR16;
	    afmt = AFMT_S16_BE;
	}
	else if (sfmts & AFMT_S16_LE) /* likely */
	{
	    ad->real_fmt = CST_AUDIO_LINEAR16;
	    ad->byteswap = 1;
	    afmt = AFMT_S16_LE;
	}
	else if (sfmts & AFMT_U8)
	{
	    ad->real_fmt = CST_AUDIO_LINEAR8;
	    afmt = AFMT_U8;
	}
	else
	{
	    cst_free(ad);
	    close(afd);
	    return NULL;
	}
    }
#endif

    return ad;
}

int audio_close_palmos(cst_audiodev *ad)
{
    int rv=0;

    if (ad == NULL)
	return 0;

    cst_free(ad);
    return rv;
}

int audio_write_palmos(cst_audiodev *ad, void *samples, int num_bytes)
{
/*    return write((int)ad->platform_data,samples,num_bytes); */
	return 0;
}

int audio_flush_palmos(cst_audiodev *ad)
{
/*    return ioctl((int)ad->platform_data, SNDCTL_DSP_SYNC, NULL); */
	return 0;
}

int audio_drain_palmos(cst_audiodev *ad)
{
/*    return ioctl((int)ad->platform_data, SNDCTL_DSP_RESET, NULL); */
	return 0;
}
