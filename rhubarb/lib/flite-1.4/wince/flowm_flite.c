/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                         Copyright (c) 2009                            */
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
/*               Date:  January 2009                                     */
/*************************************************************************/
/*                                                                       */
/*  flowm functions for flite access                                     */
/*                                                                       */
/*************************************************************************/

#include <windows.h>
#include <commctrl.h>                
#include <aygshell.h>

#include "cst_wchar.h"
#include "flite.h"
#include "flowm.h"

/* For debugging its sometimes good to switch off the actual synthesis */
#define DOTTS 1 

static cst_audiodev *fl_ad = 0;

#ifdef DOTTS
cst_voice *register_cmu_us_kal(const char *voxdir);
void unregister_cmu_us_kal(cst_voice *v);
cst_voice *register_cmu_us_awb(const char *voxdir);
void unregister_cmu_us_awb(cst_voice *v);
cst_voice *register_cmu_us_rms(const char *voxdir);
void unregister_cmu_us_rms(cst_voice *v);
cst_voice *register_cmu_us_slt(const char *voxdir);
void unregister_cmu_us_slt(cst_voice *v);
#endif

cst_wave *previous_wave = NULL;

typedef struct VoxDef_struct
{
    TCHAR *name;
    cst_voice *(*rv)(const char *voxdir);  /* register_voice */
    void (*urv)(cst_voice *v);             /* unregister_voice */
    int min_buffsize;                      /* for audio streaming */
    cst_voice *v;
} VoxDef;

VoxDef VoxDefs[] = {
#ifdef cmu_us_kal
    { L"kal", register_cmu_us_kal, unregister_cmu_us_kal, 256, NULL },
#endif
#ifdef cmu_us_awb
    { L"awb", register_cmu_us_awb, unregister_cmu_us_awb, 2000, NULL },
#endif
#ifdef cmu_us_rms
    { L"rms", register_cmu_us_rms, unregister_cmu_us_rms, 2000, NULL },
#endif
#ifdef cmu_us_slt
    { L"slt", register_cmu_us_slt, unregister_cmu_us_slt, 2000, NULL },
#endif
    { NULL, NULL }
};

cst_utterance *flowm_print_relation_callback(cst_utterance *u);
cst_utterance *flowm_utt_callback(cst_utterance *u);
int flowm_audio_stream_chunk(const cst_wave *w, int start, int size, 
                             int last, void *user);

float flowm_find_file_percentage()
{
    if (flowm_file_size <= 0)
        return 0.0;
    else
        return (flowm_file_pos*100.0)/flowm_file_size;
}

TCHAR *flowm_voice_name(int i)
{
    /* In order not to have flite things in flowm_main, we provide an */
    /* interface to the voice list */
    return VoxDefs[i].name;
}

void flowm_init()
{
#ifdef DOTTS
    int i;
    cst_audio_streaming_info *asi;

    flite_init();        /* Initialize flite interface */

    for (i=0; VoxDefs[i].name; i++)
    {
        VoxDefs[i].v = (VoxDefs[i].rv)(NULL); /* register voice */

        /* Set up call back function for low level audio streaming */
        /* This way it plays the waveform as it synthesizes it */
        /* This is necessary for the slower (CG) voices */
        asi = new_audio_streaming_info();
        asi->asc = flowm_audio_stream_chunk;
        asi->min_buffsize = VoxDefs[i].min_buffsize;
        feat_set(VoxDefs[i].v->features,
                 "streaming_info",
                 audio_streaming_info_val(asi));

        /* Set up call back function for sending what tokens are being */
        /* synthesized and for keeping track of the current position in */
        /* the file */
        feat_set(VoxDefs[i].v->features,
                 "utt_user_callback",
                 uttfunc_val(flowm_utt_callback));

        /* For outputing results of a relation (only used in play) */
        feat_set(VoxDefs[i].v->features,
                 "post_synth_hook_func",
                 uttfunc_val(flowm_print_relation_callback));
    }

#endif
    return;
}

void flowm_terminate()
{
#ifdef DOTTS
    int i;

    for (i=0; VoxDefs[i].name; i++)
    {
        (VoxDefs[i].urv)(VoxDefs[i].v); /* unregister voice */
    }
#endif
    if (previous_wave)
    {
        delete_wave(previous_wave);
        previous_wave = NULL;
    }
        
    return;
}

int flowm_save_wave(TCHAR *filename)
{
    /* Save the Last synthesized waveform file to filename */
    char *sfilename;
    int rc;

    if (!previous_wave)
        return -1;

    sfilename = cst_wstr2cstr(filename);
    rc = cst_wave_save_riff(previous_wave,sfilename);
    cst_free(sfilename);

    return rc;
}

#ifdef DOTTS
int flowm_say_text(TCHAR *text)
{
    char *s;
    int ns;
    cst_voice *v;

    if (previous_wave)
    {
        delete_wave(previous_wave);
        previous_wave = NULL;
    }

    s = cst_wstr2cstr(text);               /* text to synthesize */
    v = VoxDefs[flowm_selected_voice].v;   /* voice to synthesize with */

    feat_remove(v->features,"print_info_relation");
    if (flowm_selected_relation == 1)
        feat_set_string(v->features, "print_info_relation", "Word");
    if (flowm_selected_relation == 2)
        feat_set_string(v->features, "print_info_relation", "Segment");

    /* Do the synthesis */
    previous_wave = flite_text_to_wave(s,v);

    ns = cst_wave_num_samples(previous_wave);

    cst_free(s);
    audio_flush(fl_ad);
    audio_close(fl_ad); 
    fl_ad = NULL;

    return ns;
}
#else
int flowm_say_text(TCHAR *text)
{
    MessageBoxW(0,text,L"SayText",0);
    return 0;
}
#endif

cst_utterance *flowm_print_relation_callback(cst_utterance *u)
{
    /* Say the details of a named relation for display */
    char rst[FL_MAX_MSG_CHARS];
    const char *name;
    const char *relname;
    cst_item *item;
    char *space;

    space = "";
    relname = get_param_string(u->features,"print_info_relation", NULL);
    cst_sprintf(rst,"%s: ",relname);

    if (!relname)
    {
        mbstowcs(fl_tts_msg,"",FL_MAX_MSG_CHARS);
        return u;
    }

    for (item=relation_head(utt_relation(u,relname)); 
         item; item=item_next(item))
    {
        name = item_feat_string(item,"name");
        
        if (cst_strlen(name)+1+4 < FL_MAX_MSG_CHARS)
            cst_sprintf(rst,"%s%s%s",rst,space,name);
        else if (cst_strlen(rst)+4 < FL_MAX_MSG_CHARS)
            cst_sprintf(rst,"%s ...",rst);
        else
            break;
        space = " ";
    }
    mbstowcs(fl_tts_msg,rst,FL_MAX_MSG_CHARS);

    return u;
}

cst_utterance *flowm_utt_callback(cst_utterance *u)
{
    char rst[FL_MAX_MSG_CHARS];
    const char *tok;
    cst_item *item;
    char *space;
    int extend_length;
    
    /* In order to stop the synthesizer if the STOP button is pressed */
    /* This stops the synthesis of the next utterance */

    if ((flowm_play_status == FLOWM_PLAY) ||
        (flowm_play_status == FLOWM_SKIP))
    {
        if (TTSWindow)
        {
            rst[0] = '\0';
            space = "";
            for (item=relation_head(utt_relation(u,"Token")); 
                 item; item=item_next(item))
            {
                tok = item_feat_string(item,"name");
                if (cst_streq("",space))
                    /* Only do this on the first token/word */
                    flowm_file_pos = item_feat_int(item,"file_pos");
                extend_length = cst_strlen(rst) + 1 +
                    cst_strlen(item_feat_string(item,"prepunctuation"))+
                    cst_strlen(item_feat_string(item,"punc"));
                if (cst_strlen(tok)+extend_length+4 < FL_MAX_MSG_CHARS)
                    cst_sprintf(rst,"%s%s%s%s%s",rst,space,
                                item_feat_string(item,"prepunctuation"),
                                tok,
                                item_feat_string(item,"punc"));
                else 
                {
                    if (cst_strlen(rst)+4 < FL_MAX_MSG_CHARS)
                        cst_sprintf(rst,"%s ...",rst);
                    break;
                }
                space = " ";
            }

            if (flowm_file_pos > flowm_prev_utt_pos[flowm_utt_pos_pos])
            {
                if ((flowm_utt_pos_pos+1) >= FLOWM_NUM_UTT_POS)
                {
                    /* Filled it up, so move it down */
                    memmove(flowm_prev_utt_pos,&flowm_prev_utt_pos[1],
                            sizeof(int)*(FLOWM_NUM_UTT_POS-10));
                    flowm_utt_pos_pos = (FLOWM_NUM_UTT_POS-10);
                }
                flowm_utt_pos_pos++;
                flowm_prev_utt_pos[flowm_utt_pos_pos] = flowm_file_pos;
            }

            /* Send text to TTSWindow */
            mbstowcs(fl_tts_msg,rst,FL_MAX_MSG_CHARS);
            SetDlgItemText(TTSWindow, FL_SYNTHTEXT, fl_tts_msg);

            /* Update file pos percentage in FilePos window */
            cst_sprintf(rst,"%2.3f",flowm_find_file_percentage());
            mbstowcs(fl_fp_msg,rst,FL_MAX_MSG_CHARS);
            SetDlgItemText(TTSWindow, FL_FILEPOS, fl_fp_msg);

            SystemIdleTimerReset();  /* keep alive while synthesizing */
            if (flowm_play_status == FLOWM_SKIP)
                flowm_play_status = FLOWM_PLAY;
        }
        return u;
    }
    else
    {
        delete_utterance(u);
        return 0;
    }
}

int flowm_audio_stream_chunk(const cst_wave *w, int start, int size, 
                             int last, void *user)
{

    if (fl_ad == NULL)
    {
        fl_ad = audio_open(w->sample_rate,w->num_channels,CST_AUDIO_LINEAR16);
    }

    if (flowm_play_status == FLOWM_PLAY)
    {
        audio_write(fl_ad,&w->samples[start],size*sizeof(short));
        return CST_AUDIO_STREAM_CONT;
    }
    else if (flowm_play_status == FLOWM_BENCH)
    {   /* Do TTS but don't actually play it */
        /* How much have we played */
        flowm_duration += (size*1.0)/w->sample_rate;
        return CST_AUDIO_STREAM_CONT;
    }
    else
    {   /* for STOP, and the SKIPS (if they get here) */
        return CST_AUDIO_STREAM_STOP;
    }
}

#ifdef DOTTS
int flowm_say_file(TCHAR *tfilename)
{
    int rc = 0;
    char *filename;
    cst_voice *v;
    
    if (previous_wave)
    {   /* This is really tidy up from Play -- but might say space */
        delete_wave(previous_wave);
        previous_wave = NULL;
    }

    if (fl_ad)
    {
        MessageBoxW(0,L"audio fd still open",L"SayFile",0);
        audio_close(fl_ad); 
        fl_ad = NULL;
    }

    v = VoxDefs[flowm_selected_voice].v;

    /* Where we want to start from */
    feat_set_int(v->features, "file_start_position", flowm_file_pos);

    /* Only do print_info in play mode */
    feat_remove(v->features,"print_info_relation");

    filename = cst_wstr2cstr(tfilename);
    rc = flite_file_to_speech(filename, v, "stream");
    cst_free(filename);

    audio_flush(fl_ad);
    audio_close(fl_ad); 
    fl_ad = NULL;

    return rc;

}
#else
int flowm_say_file(TCHAR *text)
{
    MessageBoxW(0,text,L"SayFile",0);
    return 0;
}
#endif




    
