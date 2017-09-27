/*************************************************************************/
/*                                                                       */
/*                           Cepstral, LLC                               */
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
/*  CEPSTRAL, LLC AND THE CONTRIBUTORS TO THIS WORK DISCLAIM ALL         */
/*  WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED       */
/*  WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL         */
/*  CEPSTRAL, LLC NOR THE CONTRIBUTORS BE LIABLE FOR ANY SPECIAL,        */
/*  INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER          */
/*  RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION    */
/*  OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR  */
/*  IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.          */
/*                                                                       */
/*************************************************************************/
/*             Author:  David Huggins-Daines (dhd@cepstral.com)          */
/*               Date:  October 2001                                     */
/*************************************************************************/
/*                                                                       */
/*  au_wince.c: audio support for Windows CE and Win32                   */
/*  (IHNJ, IJLS "wince")                                                 */
/*                                                                       */
/*************************************************************************/

#include <windows.h>
#include <mmsystem.h>

#include "cst_audio.h"
#include "cst_alloc.h"

typedef struct au_wince_pdata_struct {
	HWAVEOUT wo;
	HANDLE bevt;
	HANDLE wevt;
	LONG bcnt;
  int in_reset;
  void **fq;
  int fqlen;
  int fqmaxlen;
} au_wince_pdata;

void add_to_free_queue(cst_audiodev *ad, void *datum)
{
    au_wince_pdata *pd = ad->platform_data;

    if (pd->fqlen == pd->fqmaxlen && !(pd->fqmaxlen % 32))
    {
        pd->fqmaxlen += 32;
        pd->fq = (void **)realloc(pd->fq, pd->fqmaxlen * sizeof(void *));
        if (!pd->fq)
        {
            cst_errmsg("Out of memory\n");
            cst_error();
        }
    }
    pd->fq[pd->fqlen++] = datum;
}

static void finish_header(HWAVEOUT drvr, WAVEHDR *hdr)
{
    if (waveOutUnprepareHeader(drvr,hdr,sizeof(*hdr))
        != MMSYSERR_NOERROR)
    {
        cst_errmsg("Failed to unprepare header %p\n", hdr);
        cst_error();
    }
    cst_free(hdr->lpData);
    cst_free(hdr);
}

void CALLBACK sndbuf_done(HWAVEOUT drvr, UINT msg,
			  DWORD udata, DWORD param1, DWORD param2)
{
    WAVEHDR *hdr = (WAVEHDR *)param1;
    cst_audiodev *ad = (cst_audiodev *)udata;
    au_wince_pdata *pd = ad->platform_data;

    if (msg == MM_WOM_DONE && hdr && (hdr->dwFlags & WHDR_DONE)) {
        LONG c;

        c = InterlockedDecrement(&pd->bcnt);
        if (c == 0)
            SetEvent(pd->bevt);
        if (c == 7)
            SetEvent(pd->wevt);
        if (pd->in_reset) add_to_free_queue(ad, hdr);
    }
}

cst_audiodev *audio_open_wince(int sps, int channels, int fmt)
{
    cst_audiodev *ad;
    au_wince_pdata *pd;
    HWAVEOUT wo;
    WAVEFORMATEX wfx;
    MMRESULT err;

    ad = cst_alloc(cst_audiodev,1);
    ad->sps = ad->real_sps = sps;
    ad->channels = ad->real_channels = channels;
    ad->fmt = ad->real_fmt = fmt;

    memset(&wfx,0,sizeof(wfx));
    wfx.nChannels = channels;
    wfx.nSamplesPerSec = sps;

    switch (fmt)
    {
    case CST_AUDIO_LINEAR16:
        wfx.wFormatTag = WAVE_FORMAT_PCM;
        wfx.wBitsPerSample = 16;
        break;
    case CST_AUDIO_LINEAR8:
        wfx.wFormatTag = WAVE_FORMAT_PCM;
        wfx.wBitsPerSample = 8;
        break;
    default:
        cst_errmsg("audio_open_wince: unsupported format %d\n", fmt);
        cst_free(ad);
        cst_error();
    }
    wfx.nBlockAlign = wfx.nChannels*wfx.wBitsPerSample/8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec*wfx.nBlockAlign;
    err = waveOutOpen(&wo,WAVE_MAPPER,&wfx,
                      (DWORD)sndbuf_done,(DWORD)ad,
                      CALLBACK_FUNCTION);
    if (err != MMSYSERR_NOERROR)
    {
        cst_errmsg("Failed to open output device: %x\n", err);
        cst_free(ad);
        cst_error();
    }

    pd = cst_alloc(au_wince_pdata,1);
    pd->wo = wo;
    pd->bevt = CreateEvent(NULL,FALSE,FALSE,NULL);
    pd->wevt = CreateEvent(NULL,FALSE,FALSE,NULL);
    pd->bcnt = 0;
    ad->platform_data = pd;
    return ad;
}

static void free_queue_empty(cst_audiodev *ad)
{
    au_wince_pdata *pd = ad->platform_data;

    while (pd->fqlen)
    {
        finish_header(pd->wo, pd->fq[--pd->fqlen]);
    }
}

int audio_close_wince(cst_audiodev *ad)
{
    au_wince_pdata *pd = ad->platform_data;
    MMRESULT err;

    if (ad)
    {
        /* Okay, I actually think this isn't a race, because
           bcnt is only ever decremented asynchronously, and
           the asynchronous callback can't be interrupted.  So
           the only issue is whether it hits zero between the
           time we test it and the time we start waiting, and
           in this case, the event will get set anyway. */
        if (pd->bcnt > 0)
            WaitForSingleObject(pd->bevt, INFINITE);
        pd->in_reset = 1;
        err = waveOutReset(pd->wo);
        if (err != MMSYSERR_NOERROR)
        {
            cst_errmsg("Failed to reset output device: %x\n", err);
            cst_error();
        }
        pd->in_reset = 0;
        free_queue_empty(ad);
        err = waveOutClose(pd->wo);
        if (err != MMSYSERR_NOERROR)
        {
            cst_errmsg("Failed to close output device: %x\n", err);
            cst_error();
        }
        cst_free(pd);
        cst_free(ad);
    }
    return 0;
}

int audio_write_wince(cst_audiodev *ad, void *samples, int num_bytes)
{
    au_wince_pdata *pd = ad->platform_data;
    WAVEHDR *hdr;
    MMRESULT err;

    if (num_bytes == 0)
        return 0;

    hdr = cst_alloc(WAVEHDR,1);
    hdr->lpData = cst_alloc(char,num_bytes);
    memcpy(hdr->lpData,samples,num_bytes);
    hdr->dwBufferLength = num_bytes;

    err = waveOutPrepareHeader(pd->wo, hdr, sizeof(*hdr));
    if (err != MMSYSERR_NOERROR)
    {
        cst_errmsg("Failed to prepare header %p: %x\n", hdr, err);
        cst_error();
    }

    if (InterlockedIncrement(&pd->bcnt) == 8)
        WaitForSingleObject(pd->wevt, INFINITE);
    err = waveOutWrite(pd->wo, hdr, sizeof(*hdr));
    if (err != MMSYSERR_NOERROR)
    {
        cst_errmsg("Failed to write header %p: %x\n", hdr, err);
        cst_error();
    }
    return num_bytes;
}

int audio_flush_wince(cst_audiodev *ad)
{
    au_wince_pdata *pd = ad->platform_data;

    if (pd->bcnt > 0)
        WaitForSingleObject(pd->bevt, INFINITE);
    return 0;
}

int audio_drain_wince(cst_audiodev *ad)
{
    au_wince_pdata *pd = ad->platform_data;

    pd->in_reset = 1;
    waveOutReset(pd->wo);
    pd->in_reset = 0;
    free_queue_empty(ad);
    if (pd->bcnt > 0)
        WaitForSingleObject(pd->bevt, INFINITE);
    return 0;
}
