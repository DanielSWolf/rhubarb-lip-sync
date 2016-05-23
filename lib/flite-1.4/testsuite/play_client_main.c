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
/*  Read and play a waveform by sending it to a server                   */
/*                                                                       */
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "cst_wave.h"
#include "cst_audio.h"

int play_client_main_usage()
{
    printf("simple audio server: client program\n");
    printf("usage:  play_client [-s SERVERNAME] [-p PORTNUMBER] [-e ENCODING ] WAVEFORM0 ...\n");
    printf("Send waveforms to a named audio sever\n");
    printf("Default server is \"localhost\" default port is %d\n",
	   CST_AUDIO_DEFAULT_PORT);
    printf("Default encoding is short, but you can choose ulaw too\n");
    printf("If encoding is ulaw, transfer is as 8 bit mulaw (i.e. quicker)\n");

    return 0;
}

int main(int argc, char **argv)
{
    cst_wave *w;
    int port;
    char *server;
    char *encoding; 
    int i,iw;

    port = CST_AUDIO_DEFAULT_PORT;
    server = CST_AUDIO_DEFAULT_SERVER;
    encoding = CST_AUDIO_DEFAULT_ENCODING;

    if (argc == 1)
    {
	play_client_main_usage();
	return 1;
    }

    if ((cst_streq("-h",argv[1])) ||
	(cst_streq("-help",argv[1])) ||
	(cst_streq("--help",argv[1])))
    {
	play_client_main_usage();
	return 1;
    }
    iw = 1;

    if (cst_streq("-s",argv[iw]))
    {
	if (argc < iw+1)
	{
	    fprintf(stderr,"ERROR: no servername given\n");
	    play_client_main_usage();
	    return 1;
	}
	server = argv[iw+1];
	iw+=2;
    }

    if (cst_streq("-p",argv[iw]))
    {
	if (argc < iw+1)
	{
	    fprintf(stderr,"ERROR: no port given\n");
	    play_client_main_usage();
	    return 1;
	}
	port = atoi(argv[iw+1]);
	iw+=2;
    }

    if (cst_streq("-e",argv[iw]))
    {
	if (argc < iw+1)
	{
	    fprintf(stderr,"ERROR: no encoding given\n");
	    play_client_main_usage();
	    return 1;
	}
	encoding = argv[iw+1];
	iw+=2;
    }

    for (i=iw; i<argc; i++)
    {
	w = new_wave();
	if (cst_wave_load_riff(w,argv[i]) != CST_OK_FORMAT)
	{
	    fprintf(stderr,
		    "play_wave: can't read file or wrong format \"%s\"\n",
		    argv[i]);
	    continue;
	}
	play_wave_client(w,server,port,encoding);
	delete_wave(w);
    }

    return 0;
}
