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
/*  Wait from waveforms to connect to a socket and play them streaming   */
/*                                                                       */
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "cst_wave.h"
#include "cst_audio.h"

int play_server_main_usage()
{
    printf("simple audio server\n");
    printf("usage:  play_server [PORTNUMBER]\n");
    printf("Sit and listen on socket number PORTNUMBER and play waveforms\n");
    printf("Send to it, default PORTNUMBER is %d\n",
	   CST_AUDIO_DEFAULT_PORT);
    return 0;
}

int main(int argc, char **argv)
{
    int port;

    port=CST_AUDIO_DEFAULT_PORT;

    if (argc > 1)
    {
	if ((cst_streq("-h",argv[1])) ||
	    (cst_streq("-help",argv[1])) ||
	    (cst_streq("--help",argv[1])))
	{
	    play_server_main_usage();
	    return -1;
	}
	port = atoi(argv[1]);
	if (port <= 0)
	{
	    fprintf(stderr,"invalid port number: %d\n",port);
	    return -1;
	}
    }

    auserver(port);

    return 0;

}

