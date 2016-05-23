/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 2001                             */
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
/*  Sun/Solaris audio support                                            */
/*                                                                       */
/*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/filio.h>
#include <sys/audioio.h>
#include "cst_string.h"
#include "cst_wave.h"
#include "cst_audio.h"

static const char *sun_audio_device = "/dev/audio";

cst_audiodev *audio_open_sun(int sps, int channels, cst_audiofmt fmt)
{
    audio_info_t ainfo;
    int fd;
    cst_audiodev *ad;
    char *audio_device;

    if ((fd = open(sun_audio_device,O_WRONLY)) < 0)
    {
        /* the device might be a SunRay, so get the AUDIODEV env var */
        audio_device = getenv("AUDIODEV");
	
	if (audio_device != NULL) {
   	    if ((fd = open(audio_device,O_WRONLY)) < 0) {
	        cst_errmsg("sun_audio: failed to open audio device %s: %s\n",
			   audio_device, strerror(errno));
	    }
	} else {
	    cst_errmsg("sun_audio: failed to open audio device %s: %s\n",
		       sun_audio_device, strerror(errno));
	    cst_error();
	}
    }
    ioctl(fd,AUDIO_GETINFO,&ainfo);

    switch (fmt)
    {
    case CST_AUDIO_LINEAR16:
	ainfo.play.encoding = AUDIO_ENCODING_LINEAR;
	ainfo.play.precision = 16;
	break;
    case CST_AUDIO_LINEAR8:
	ainfo.play.encoding = AUDIO_ENCODING_LINEAR;
	ainfo.play.precision = 8;
	break;
    case CST_AUDIO_MULAW:
	ainfo.play.encoding = AUDIO_ENCODING_ULAW;
	ainfo.play.precision = 8;
	break;
    }

    ainfo.play.channels = 1;
    ainfo.play.sample_rate = sps;

    if (ioctl(fd,AUDIO_SETINFO,&ainfo) == -1)
    {
	cst_errmsg("sun_audio: failed to set audio params: %s\n",
		   strerror(errno));
	close(fd);
	cst_error();
    }

    ad = cst_alloc(cst_audiodev, 1);

    ad->sps = sps;
    ad->real_sps = ainfo.play.sample_rate;

    ad->channels = channels;
    ad->real_channels = ainfo.play.channels;

    ad->fmt = fmt;
    if (ainfo.play.encoding == AUDIO_ENCODING_LINEAR)
    {
	if (ainfo.play.precision == 16)
	    ad->real_fmt = CST_AUDIO_LINEAR16;
	else if (ainfo.play.precision == 8)
	    ad->real_fmt = CST_AUDIO_LINEAR8;
	else
	{
	    cst_errmsg("sun_audio: linear %d bit audio unsupported\n",
		       ainfo.play.precision);
	    close(fd);
	    cst_free(ad);
	    cst_error();
	}
    }
    else if (ainfo.play.encoding == AUDIO_ENCODING_ULAW)
	ad->real_fmt = CST_AUDIO_MULAW;
    else
    {
	    cst_errmsg("sun_audio: unsupported audio format (%d bit/encoding #%d)\n",
		       ainfo.play.precision, ainfo.play.encoding);
	    close(fd);
	    cst_free(ad);
	    cst_error();
    }
    ad->platform_data = (void *)fd;

    return ad;
}

int audio_close_sun(cst_audiodev *ad)
{
    int rv;

    if (ad == NULL)
	    return 0;

    rv = close((int)ad->platform_data);
    cst_free(ad);
    return rv;
}

int audio_write_sun(cst_audiodev *ad, void *samples, int num_bytes)
{
    return write((int)ad->platform_data,samples,num_bytes);
}

int audio_flush_sun(cst_audiodev *ad)
{
    return ioctl((int)ad->platform_data, AUDIO_DRAIN, 0);
}

/* FIXME... */
int audio_drain_sun(cst_audiodev *ad)
{
    return ioctl((int)ad->platform_data, AUDIO_DRAIN, 0);
}

