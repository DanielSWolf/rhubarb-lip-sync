/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 2005                             */
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
/*********************************************************************** */
/*             Author:  Lukas Loehrer ()                                 */
/*               Date:  January 2005                                     */
/*************************************************************************/
/*                                                                       */
/*  Native access to alsa audio devices on Linux                         */
/*  Tested with libasound version 1.0.10                                 */
/*                                                                       */
/*  Added snd_config_update_free_global(); after every close to stop     */
/*  (apparent?) memory leaks                                             */
/*************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <assert.h>
#include <errno.h>

#include "cst_string.h"
#include "cst_wave.h"
#include "cst_audio.h"

#include <alsa/asoundlib.h>


/*static char *pcm_dev_name = "hw:0,0"; */
static char *pcm_dev_name ="default";

static inline void print_pcm_state(snd_pcm_t *handle, char *msg)
{
  fprintf(stderr, "PCM state at %s = %s\n", msg,
		  snd_pcm_state_name(snd_pcm_state(handle)));
}

cst_audiodev *audio_open_alsa(int sps, int channels, cst_audiofmt fmt)
{
  cst_audiodev *ad;
  unsigned 	int real_rate;
  int err;

  /* alsa specific stuff */
  snd_pcm_t *pcm_handle;          
  snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
  snd_pcm_hw_params_t *hwparams;
  snd_pcm_format_t format;
  snd_pcm_access_t access = SND_PCM_ACCESS_RW_INTERLEAVED;

  /* Allocate the snd_pcm_hw_params_t structure on the stack. */
  snd_pcm_hw_params_alloca(&hwparams);

  /* Open pcm device */
  err = snd_pcm_open(&pcm_handle, pcm_dev_name, stream, 0);
  if (err < 0) 
  {
	cst_errmsg("audio_open_alsa: failed to open audio device %s. %s\n",
			   pcm_dev_name, snd_strerror(err));
	return NULL;
  }

  /* Init hwparams with full configuration space */
  err = snd_pcm_hw_params_any(pcm_handle, hwparams);
  if (err < 0) 
  {
	snd_pcm_close(pcm_handle);
        snd_config_update_free_global();
	cst_errmsg("audio_open_alsa: failed to get hardware parameters from audio device. %s\n", snd_strerror(err));
	return NULL;
  }

  /* Set access mode */
  err = snd_pcm_hw_params_set_access(pcm_handle, hwparams, access);
  if (err < 0) 
  {
	snd_pcm_close(pcm_handle);
        snd_config_update_free_global();
	cst_errmsg("audio_open_alsa: failed to set access mode. %s.\n", snd_strerror(err));
	return NULL;
  }

  /* Determine matching alsa sample format */
  /* This could be implemented in a more */
  /* flexible way (byte order conversion). */
  switch (fmt)
  {
  case CST_AUDIO_LINEAR16:
	if (CST_LITTLE_ENDIAN)
	  format = SND_PCM_FORMAT_S16_LE;
	else
	  format = SND_PCM_FORMAT_S16_BE;
	break;
  case CST_AUDIO_LINEAR8:
	format = SND_PCM_FORMAT_U8;
	break;
  case CST_AUDIO_MULAW:
	format = SND_PCM_FORMAT_MU_LAW;
	break;
  default:
	snd_pcm_close(pcm_handle);
        snd_config_update_free_global();
	cst_errmsg("audio_open_alsa: failed to find suitable format.\n");
	return NULL;
	break;
  }

  /* Set samble format */
  err = snd_pcm_hw_params_set_format(pcm_handle, hwparams, format);
  if (err <0) 
  {
	snd_pcm_close(pcm_handle);
        snd_config_update_free_global();
	cst_errmsg("audio_open_alsa: failed to set format. %s.\n", snd_strerror(err));
	return NULL;
  }

  /* Set sample rate near the disired rate */
  real_rate = sps;
  err = snd_pcm_hw_params_set_rate(pcm_handle, hwparams, real_rate, 0);
  if (err < 0)   
  {
	snd_pcm_close(pcm_handle);
        snd_config_update_free_global();
	cst_errmsg("audio_open_alsa: failed to set sample rate near %d. %s.\n", sps, snd_strerror(err));
	return NULL;
  }
  /*FIXME:  This is probably too strict */
  assert(sps == real_rate);

  /* Set number of channels */
  assert(channels >0);
  err = snd_pcm_hw_params_set_channels(pcm_handle, hwparams, channels);
  if (err < 0) 
  {
	snd_pcm_close(pcm_handle);
        snd_config_update_free_global();
	cst_errmsg("audio_open_alsa: failed to set number of channels to %d. %s.\n", channels, snd_strerror(err));
	return NULL;
  }

  /* Commit hardware parameters */
  err = snd_pcm_hw_params(pcm_handle, hwparams);
  if (err < 0) 
  {
	snd_pcm_close(pcm_handle);
        snd_config_update_free_global();
	cst_errmsg("audio_open_alsa: failed to set hw parameters. %s.\n", snd_strerror(err));
	return NULL;
  }

  /* Make sure the device is ready to accept data */
  assert(snd_pcm_state(pcm_handle) == SND_PCM_STATE_PREPARED);

  /* Write hardware parameters to flite audio device data structure */
  ad = cst_alloc(cst_audiodev, 1);
  assert(ad != NULL);
  ad->real_sps = ad->sps = sps;
  ad->real_channels = ad->channels = channels;
  ad->real_fmt = ad->fmt = fmt;
  ad->platform_data = (void *) pcm_handle;

  return ad;
}

int audio_close_alsa(cst_audiodev *ad)
{
  int result;
  snd_pcm_t *pcm_handle;

  if (ad == NULL)
      return 0;

  pcm_handle = (snd_pcm_t *) ad->platform_data;

  snd_pcm_drain(pcm_handle); /* wait for current stuff in buffer to finish */

  result = snd_pcm_close(pcm_handle);
  snd_config_update_free_global();
  if (result < 0)
  {
	cst_errmsg("audio_close_alsa: Error: %s.\n", snd_strerror(result));
  }
  cst_free(ad);
  return result;
}

/* Returns zero if recovery was successful. */
static int recover_from_error(snd_pcm_t *pcm_handle, ssize_t res)
{
  if (res == -EPIPE) /* xrun */
  {
	res = snd_pcm_prepare(pcm_handle);
	if (res < 0) 
	{
	  /* Failed to recover from xrun */
	  cst_errmsg("recover_from_write_error: failed to recover from xrun. %s\n.", snd_strerror(res));
	  return res;
	}
  } 
  else if (res == -ESTRPIPE) /* Suspend */
  {
	while ((res = snd_pcm_resume(pcm_handle)) == -EAGAIN) 
	{
	  snd_pcm_wait(pcm_handle, 1000);
	}
	if (res < 0) 
	{
	  res = snd_pcm_prepare(pcm_handle);
	  if (res <0) 
	  {
		/* Resume failed */
		cst_errmsg("audio_recover_from_write_error: failed to resume after suspend. %s\n.", snd_strerror(res));
		return res;
	  }
	}
  } 
  else if (res < 0) 
  {
	/* Unknown failure */
	cst_errmsg("audio_recover_from_write_error: %s.\n", snd_strerror(res));
	return res;
  }
  return 0;
}

int audio_write_alsa(cst_audiodev *ad, void *samples, int num_bytes)
{
  size_t frame_size;
  ssize_t num_frames, res;
  snd_pcm_t *pcm_handle;
  char *buf = (char *) samples;

  /* Determine frame size in bytes */
  frame_size  = audio_bps(ad->real_fmt) * ad->real_channels;
  /* Require that only complete frames are handed in */
  assert((num_bytes % frame_size) == 0);
  num_frames = num_bytes / frame_size;
  pcm_handle = (snd_pcm_t *) ad->platform_data;

  while (num_frames > 0) 
  {
	res = snd_pcm_writei(pcm_handle, buf, num_frames);
	if (res != num_frames) 
	{
	  if (res == -EAGAIN || (res > 0 && res < num_frames)) 
	  {
		snd_pcm_wait(pcm_handle, 100);
	  }
	  else if (recover_from_error(pcm_handle, res) < 0) 
	  {
		return -1;
	  }
	}

	if (res >0) 
	{
	  num_frames -= res;
	  buf += res * frame_size;
	}
  }
  return num_bytes;
}

int audio_flush_alsa(cst_audiodev *ad)
{
  int result;
  result = snd_pcm_drain((snd_pcm_t *) ad->platform_data);
  if (result < 0)
  {
	cst_errmsg("audio_flush_alsa: Error: %s.\n", snd_strerror(result));
  }
	/* Prepare device for more data */
  result = snd_pcm_prepare((snd_pcm_t *) ad->platform_data);
if (result < 0)
  {
	cst_errmsg("audio_flush_alsa: Error: %s.\n", snd_strerror(result));
  }
  return result;
}

int audio_drain_alsa(cst_audiodev *ad)
{
  int result;
  result = snd_pcm_drop((snd_pcm_t *) ad->platform_data);
  if (result < 0)
  {
	cst_errmsg("audio_drain_alsa: Error: %s.\n", snd_strerror(result));
  }
/* Prepare device for more data */
  result = snd_pcm_prepare((snd_pcm_t *) ad->platform_data);
if (result < 0)
  {
	cst_errmsg("audio_drain_alsa: Error: %s.\n", snd_strerror(result));
  }
  return result;
}
