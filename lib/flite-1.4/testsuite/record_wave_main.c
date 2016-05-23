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
/*               Date:  July 2001                                        */
/*************************************************************************/
/*                                                                       */
/*  Record a waveform and save to give file                              */
/*                                                                       */
/*  overly specific hack (for linux) first                               */
/*                                                                       */
/*************************************************************************/
#include <unistd.h> 
#include <signal.h> 
#include <stdio.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <fcntl.h>
#include "cst_wave.h"
#include "cst_audio.h"

int desired_rate = 16000;
float desired_time = -1;
int still_record = 1;

static const char * const vw_audio_device = "/dev/dsp";
static int audio_set_sample_rate_vw(int afd,int sample_rate)
{
    int fmt;
    int sfmts;
    int stereo=0;
    int sstereo=0;
    int osample_rate;
    int channels=1;

    ioctl(afd,SNDCTL_DSP_RESET,0);
    sstereo = stereo;
    ioctl(afd,SNDCTL_DSP_STEREO,&sstereo);
    /* Some devices don't do mono even when you ask them nicely */
    if (sstereo != stereo)
	osample_rate = sample_rate / 2;
    else
	osample_rate = sample_rate;
    ioctl(afd,SNDCTL_DSP_SPEED,&osample_rate);
    ioctl(afd,SNDCTL_DSP_CHANNELS,&channels);
    ioctl(afd,SNDCTL_DSP_GETFMTS,&sfmts);

    if (sfmts == AFMT_U8)
	fmt = AFMT_U8;         // its really an 8 bit only device
    else if (CST_LITTLE_ENDIAN)
	fmt = AFMT_S16_LE;  
    else
	fmt = AFMT_S16_BE;  
    
    ioctl(afd,SNDCTL_DSP_SETFMT,&fmt);

    if (fmt == AFMT_U8)
	return -1;
    else
	return 0;
}


void sigint_handler(int a)
{
    still_record = 0;
}

int main(int argc, char **argv)
{
    /* need that cool argument parser */
    cst_wave *w;
    int r, n, d, i;
    int desired_samples;
    char * ofile="record.wav";

    if (argc == 1)
    {
	fprintf(stderr,"usage: record_wave -f FREQ -t TIME WAVEFILE\n");
	return 1;
    }

    for (i=1; i<argc; i++)
    {
	if ((i+1 < argc) && (cst_streq(argv[i],"-f")))
	{
	    desired_rate = atoi(argv[i+1]);
	    i++;
	}
	else if ((i+1 < argc) && (cst_streq(argv[i],"-t")))
	{
	    desired_time = atof(argv[i+1]);
	    i++;
	}
	else
	    ofile = argv[i];
    }

    r = open(vw_audio_device,O_RDONLY);
    audio_set_sample_rate_vw(r,desired_rate);

    signal(SIGINT,sigint_handler);

    w = new_wave();  
    w->sample_rate = desired_rate;
    if (desired_time > 0)
	desired_samples = desired_time * desired_rate;
    else
	desired_samples = 5 * desired_rate;

    cst_wave_resize(w,desired_samples,1);

    d = 256; n = d;
    for (i=0;
	 still_record && ((desired_time < 0) || (i < desired_samples));
	 i+=n)
    {
	if (desired_time < 0)
	{
	    if (i+n > w->num_samples)
		cst_wave_resize(w,w->num_samples*1.25,1);
	}
	else if (i+n > w->num_samples)
	    d = w->num_samples - i;

	n = read(r,&w->samples[i],d*sizeof(short));
	n /= 2;
    }

    cst_wave_resize(w,i,1);
    cst_wave_save(w,ofile,"riff");

    if (desired_time < 0)
	printf("wrote %d samples %f to %s\n",
	       i,(float)i/(float)desired_rate,ofile);

    return 0;
}
