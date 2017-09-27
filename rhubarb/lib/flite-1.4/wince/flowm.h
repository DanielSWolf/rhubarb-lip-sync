/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                         Copyright (c) 2008                            */
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

#ifndef __FLOWM_H_
#define __FLOWM_H_

/* Following the program structure in Boling's Promgramming in WinCE */
typedef LRESULT (*MsgDispatcher)(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
typedef struct MsgDispatch_struct
{                            
    UINT Code;                                 /* messages */
    MsgDispatcher Fxn;
} MsgDispatch; 

typedef LRESULT (*CMDDispatcher)(HWND h, WORD w, HWND h2, WORD w2);
typedef struct CMDDispatch_struct
{
    UINT Code;
    CMDDispatcher Fxn;
} CMDDispatch;

/* Some globals that serve to communcate between threads */
extern int flowm_play_status;
extern TCHAR fl_filename[257];
extern int flowm_file_pos;
extern int flowm_file_size;
extern int flowm_selected_voice;
extern int flowm_selected_relation;

extern HWND TTSWindow;
#define FL_MAX_MSG_CHARS 300
extern TCHAR fl_tts_msg[FL_MAX_MSG_CHARS];
extern TCHAR fl_fp_msg[FL_MAX_MSG_CHARS];

#define FLOWM_NUM_UTT_POS 200
extern int flowm_utt_pos_pos;
extern int flowm_prev_utt_pos[FLOWM_NUM_UTT_POS];
extern float flowm_duration;

/* The interface to the Flite TTS system */
void flowm_init();
void flowm_terminate();
int flowm_say_text(TCHAR *text);
int flowm_say_file(TCHAR *filename);
int flowm_save_wave(TCHAR *filename); /* save previous waveform */

float flowm_find_file_percentage();
TCHAR *flowm_voice_name(int i);

int InitApp (HINSTANCE h);
HWND InitInstance (HINSTANCE h, LPWSTR p , int i);

#ifdef __MINGW32__
/* These aren't needed in MINGW32 and give compiler warnings */
#undef CALLBACK
#define CALLBACK
#undef WINAPI
#define WINAPI
#endif

void WINAPI SystemIdleTimerReset( void); 

LRESULT CALLBACK MainWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

LRESULT DoCreateMain(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
LRESULT DoNotifyMain (HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
LRESULT DoPaintMain(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
LRESULT DoSizeMain(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT DoCommandMain(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT DoSettingChangeMain(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
LRESULT DoHibernateMain(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
LRESULT DoActivateMain(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
LRESULT DoDestroyMain(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

LPARAM DoMainCommandExit(HWND hwnd,WORD idItem,HWND hwndCtl,WORD wNotifyCode);
LPARAM DoMainCommandAbout(HWND hwnd, WORD idItem, 
                          HWND hwndCtl, WORD wNotifyCode);
LPARAM DoMainCommandPlay(HWND hwnd, WORD idItem,HWND hwndCtl,WORD wNotifyCode);
LPARAM DoMainCommandFile(HWND hwnd, WORD idItem,HWND hwndCtl,WORD wNotifyCode);
BOOL CALLBACK AboutDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK PlayDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK FileDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#define FL_TOOLBAR1 101
#define FL_MAIN 102
#define FL_TOOLMENU 103
#define FL_ABOUTMENU 104
#define FL_RPTLIST 105

#define FLS_TOOLMENUNAME 150
#define FLS_ABOUTMENUNAME 151

#define FL_EXIT    201
#define FL_PLAY    202
#define FL_FILE    203
#define FL_ABOUT   204
#define FL_OPTIONS 205

#define FL_FILENAME   301
#define FL_TEXT       302
#define FL_PUSHPLAY   303
#define FL_PUSHTTS    304
#define FL_PUSHSTOP   305
#define FL_PUSHSELECT 306
#define FL_SYNTHTEXT  307
#define FL_FILEPOS    308
#define FL_PUSHFORWARD 309
#define FL_PUSHBACK   310
#define FL_BENCH      311
#define FL_SCREENOFF  312
#define FL_PUSHEND    313
#define FL_PUSHSAVE   314

#define FL_VOXLIST   401
#define FL_RELLIST   402
#define FL_OUTPUT    403

#define FL_MAX_TEXTLEN 256

#define FLOWM_PLAY 0
#define FLOWM_STOP 1
#define FLOWM_SKIP 2
#define FLOWM_BACKSKIP 3
#define FLOWM_BENCH 4

#endif
