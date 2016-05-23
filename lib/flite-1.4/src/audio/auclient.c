/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 2000                             */
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
/*               Date:  October 2000                                     */
/*************************************************************************/
/*                                                                       */
/*  A client/server audio play program                                   */
/*                                                                       */
/*  Server listens to a socket and play anything coming in (streaming)   */
/*  Client just sends (snd) headered waveform to named socket            */
/*                                                                       */
/*  Client side                                                          */
/*                                                                       */
/*************************************************************************/
#include <stdlib.h>
#include "cst_file.h"
#include "cst_socket.h"
#include "cst_string.h"
#include "cst_wave.h"
#include "cst_audio.h"

#ifndef CST_NO_SOCKETS

#include <unistd.h>

int play_wave_client(cst_wave *w,const char *servername,int port,
		     const char *encoding)
{
    int audiofd,q,i,n,r;
    int sample_width;
    unsigned char bytes[CST_AUDIOBUFFSIZE];
    short shorts[CST_AUDIOBUFFSIZE];
    snd_header header;

    if (!w)
	return CST_ERROR_FORMAT;

    if ((audiofd = cst_socket_open(servername,port)) == 0)
	return CST_ERROR_FORMAT;

    header.magic = (unsigned int)0x2e736e64;
    header.hdr_size = sizeof(header);
    if (cst_streq(encoding,"ulaw"))
    {
	sample_width = 1;
	header.encoding = 1; /* ulaw */
    }
    else if (cst_streq(encoding,"uchar"))
    {
	sample_width = 1;
	header.encoding = 2; /* unsigned char */
    }
    else 
    {
	sample_width = 2;
	header.encoding = 3; /* short */
    }
    header.data_size = sample_width * w->num_samples * w->num_channels;
    header.sample_rate = w->sample_rate;
    header.channels = w->num_channels;
    if (CST_LITTLE_ENDIAN)
    {   /* If I'm intel etc swap things, so "network byte order" */
	header.magic = SWAPINT(header.magic);
	header.hdr_size = SWAPINT(header.hdr_size);
	header.data_size = SWAPINT(header.data_size);
	header.encoding = SWAPINT(header.encoding);
	header.sample_rate = SWAPINT(header.sample_rate);
	header.channels = SWAPINT(header.channels);
    }

    if (write(audiofd, &header, sizeof(header)) != sizeof(header))
    {
	cst_errmsg("auclinet: failed to write header to server\n");
	return CST_ERROR_FORMAT;
    }

    for (i=0; i < w->num_samples; i += r)
    {
	if (w->num_samples > i+CST_AUDIOBUFFSIZE)
	    n = CST_AUDIOBUFFSIZE;
	else
	    n = w->num_samples-i;
	if (cst_streq(encoding,"ulaw"))
	{
	    for (q=0; q<n; q++)
		bytes[q] = cst_short_to_ulaw(w->samples[i+q]);
	    r = write(audiofd,bytes,n);
	}
	else 
	{
	    for (q=0; q<n; q++)
		if (CST_LITTLE_ENDIAN)
		    shorts[q] = SWAPSHORT(w->samples[i+q]);
		else
		    shorts[q] = w->samples[i+q];
	    r = write(audiofd,shorts,n*2);
	    r /= 2;
	}
	if (r <= 0)
	    cst_errmsg("failed to write %d samples\n",n);
    }

    cst_socket_close(audiofd);

    return CST_OK_FORMAT;
}

#endif
