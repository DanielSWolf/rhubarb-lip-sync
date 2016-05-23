/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                      Copyright (c) 2008-2009                          */
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
/*  Flowm: Flite on Windows Mobile (pronounced flume)                    */
/*         0.5                                                           */
/*                                                                       */
/*  A simple file reader                                                 */
/*                                                                       */
/*  Thanks to Programming Microsoft Windows CE by Douglas Boling which   */
/*  taught me a lot about how to program in WinCE                        */
/*                                                                       */
/*************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>                
#include <aygshell.h>

/* Apparently from pwingdi.h -- but I don't have that .h file */
/* For DisplayOff */
#define SETPOWERMANAGEMENT   6147
#define VideoPowerOff 4
typedef struct _VIDEO_POWER_MANAGEMENT {
    ULONG Length;
    ULONG DPMSVersion;
    ULONG PowerState;
} VIDEO_POWER_MANAGEMENT;

#include "flowm.h"

#define AppName L"Flown"
HINSTANCE hInst;

int flowm_play_status = FLOWM_STOP;
TCHAR fl_filename[257];
int flowm_file_pos = 0;
int flowm_file_size = 0;
/* To hold position of previous utterances for backskipping */
int flowm_utt_pos_pos=0;
int flowm_prev_utt_pos[FLOWM_NUM_UTT_POS];
int flowm_selected_voice = 0;
int flowm_selected_relation = 0;
float flowm_duration = 0.0;

#define FL_BENCH_TEXT L"A whole joy was reaping, but they've gone south, you should fetch azure mike."

HWND hmbar = 0;
HWND TTSWindow = 0;
TCHAR fl_tts_msg[FL_MAX_MSG_CHARS];
TCHAR fl_fp_msg[FL_MAX_MSG_CHARS];
SHACTIVATEINFO sai;

HANDLE tts_thread = INVALID_HANDLE_VALUE;

const MsgDispatch MainDispatchers[] = 
{
    { WM_CREATE, DoCreateMain },
    { WM_SIZE, DoSizeMain },
    { WM_COMMAND, DoCommandMain },
    { WM_NOTIFY, DoNotifyMain },
    { WM_SETTINGCHANGE, DoSettingChangeMain },
    { WM_HIBERNATE, DoHibernateMain },
    { WM_ACTIVATE, DoActivateMain },
    { WM_DESTROY, DoDestroyMain },
    { 0, NULL }
};

const CMDDispatch MainCommandItems[] = {
    { FL_EXIT, DoMainCommandExit },
    { FL_PLAY, DoMainCommandPlay },
    { FL_FILE, DoMainCommandFile },
    { FL_ABOUT, DoMainCommandAbout },
    { 0, NULL }
};

/*********************************************************************/
/*********************************************************************/

LRESULT CALLBACK MainWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    int i;

    for (i=0; MainDispatchers[i].Code != 0; i++)
    {
        if (msg == MainDispatchers[i].Code)
            return (MainDispatchers[i].Fxn)(hwnd,msg,wParam,lParam);
    }
    return DefWindowProc(hwnd,msg,wParam,lParam);
}

LRESULT DoCreateMain(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    SHMENUBARINFO mbi;

    ZeroMemory(&mbi,sizeof(mbi));
    mbi.cbSize = sizeof(SHMENUBARINFO);
    mbi.hwndParent = hwnd;
    mbi.nToolBarId = FL_TOOLBAR1;
    mbi.hInstRes = hInst;

    SHCreateMenuBar(&mbi);
    hmbar=mbi.hwndMB;

    if (!hmbar)
    {
        MessageBox(hwnd, L"Couldn't create toolbar",AppName, MB_OK);
        DestroyWindow(hwnd);
        return 0;
    }

    fl_filename[0] = '\0';

    return 0;
}

LRESULT DoSizeMain(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

LRESULT DoCommandMain(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WORD idItem, wNotifyCode;
    HWND hwndCtl;
    int i;
    
    idItem = (WORD)LOWORD(wParam);
    wNotifyCode = (WORD)HIWORD(wParam);
    hwndCtl = (HWND)lParam;

    for (i=0; MainCommandItems[i].Code; i++)
    {
        if (idItem == MainCommandItems[i].Code)
            return MainCommandItems[i].Fxn(hwnd,idItem,hwndCtl,wNotifyCode);
    }

    return 0;
}

LRESULT DoNotifyMain (HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    return 0;
}

LRESULT DoSettingChangeMain(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    SHHandleWMSettingChange(hwnd, wParam, lParam, &sai);
    return 0;
}

LRESULT DoHibernateMain(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    return 0;
}

LRESULT DoActivateMain(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    SHHandleWMActivate(hwnd, wParam, lParam, &sai, 0);
    return 0;
}

LRESULT DoDestroyMain(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    PostQuitMessage(0);
    return 0;
}

/* Command handler routines */
LPARAM DoMainCommandExit(HWND hwnd, WORD idItem, HWND hwndCtl, WORD wNotifyCode)
{
    SendMessage(hwnd, WM_CLOSE, 0, 0);
    return 0;
}

LPARAM DoMainCommandAbout(HWND hwnd, WORD idItem, 
                          HWND hwndCtl, WORD wNotifyCode)
{
    DialogBox(hInst, L"aboutbox", hwnd, AboutDlgProc);
    return 0;
}

BOOL CALLBACK AboutDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    SHINITDLGINFO idi;

    switch (msg)
    {
    case WM_INITDIALOG:
        idi.dwMask = SHIDIM_FLAGS;
        idi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIZEDLGFULLSCREEN |
            SHIDIF_SIPDOWN;
        idi.hDlg = hwnd;
        SHInitDialog (&idi);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            EndDialog(hwnd,0);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

LPARAM DoMainCommandPlay(HWND hwnd, WORD idItem, 
                          HWND hwndCtl, WORD wNotifyCode)
{
    DialogBox(hInst, L"playbox", hwnd, PlayDlgProc);
    return 0;
}

BOOL CALLBACK PlayDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    OPENFILENAME sf;
    const LPTSTR pszSaveFilter = L"Waveforms (*.wav)\0*.wav\0\0";
    SHINITDLGINFO idi;
    TCHAR text[FL_MAX_TEXTLEN];
    TCHAR szFilePos[FL_MAX_TEXTLEN];
    TCHAR *x;
    int i, looptimes;
    SYSTEMTIME StartTime, EndTime;
    float elapsedtime;

    switch (msg)
    {
    case WM_INITDIALOG:
        idi.dwMask = SHIDIM_FLAGS;
        idi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIZEDLGFULLSCREEN;
        idi.hDlg = hwnd;
        SHInitDialog (&idi);

        for (i=0; flowm_voice_name(i); i++)
        {   /* Add regestered voices to the voice dropdown */
            SendDlgItemMessage(hwnd, FL_VOXLIST, CB_ADDSTRING, 
                               0, (LPARAM)flowm_voice_name(i));
        }
        /* Position cursor at start, selecting first voice */
        SendDlgItemMessage(hwnd, FL_VOXLIST, CB_SETCURSEL, 0, 0);

        /* Add Relation types to the selection box */
        SendDlgItemMessage(hwnd,FL_RELLIST,CB_ADDSTRING,0,(LPARAM)L"Token");
        SendDlgItemMessage(hwnd,FL_RELLIST,CB_ADDSTRING,0,(LPARAM)L"Word");
        SendDlgItemMessage(hwnd,FL_RELLIST,CB_ADDSTRING,0,(LPARAM)L"Segment");
        SendDlgItemMessage(hwnd,FL_RELLIST,CB_SETCURSEL,0,0);

        TTSWindow = 0;  /* just in case there is a race condition */

        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case FL_PUSHPLAY:
            /* Synthesis the text with selection voice */
            /* Output items in selected relation at end */
            GetDlgItemText(hwnd, FL_TEXT, text, FL_MAX_TEXTLEN);

            flowm_selected_voice = 
                ComboBox_GetCurSel(GetDlgItem(hwnd,FL_VOXLIST));
            flowm_selected_relation = 
                ComboBox_GetCurSel(GetDlgItem(hwnd,FL_RELLIST));
            flowm_play_status = FLOWM_PLAY;
            flowm_say_text(text);
            flowm_play_status = FLOWM_STOP;

            if (flowm_selected_relation > 0)
                SetDlgItemText(hwnd, FL_OUTPUT, fl_tts_msg);

            return TRUE;
        case FL_PUSHSAVE:
            /* Save the last synthesized waveform */
            ZeroMemory(&sf,sizeof(sf));
            sf.lStructSize = sizeof(sf);
            sf.lpstrFile = fl_filename;
            sf.nMaxFile = 256;
            sf.lpstrFilter = pszSaveFilter;
            sf.Flags = 0;
            sf.lpstrTitle = L"Flowm Save";

            if (GetSaveFileName(&sf) != 0)
            {
                if (flowm_save_wave(fl_filename) == -1)
                    wsprintf(fl_tts_msg,L"Wave NOT saved in %s",fl_filename);
                else
                    wsprintf(fl_tts_msg,L"Wave saved in %s",fl_filename);
                MessageBox(hwnd, fl_tts_msg, AppName, MB_OK);
            }
            fl_filename[0] = '\0';

            return TRUE;
        case FL_BENCH:
            /* Bench marking, find out the speed of synthesis on device */
            if (flowm_play_status == FLOWM_BENCH)
            {
                flowm_play_status = FLOWM_STOP;
                return TRUE;
            }
            flowm_play_status = FLOWM_BENCH;
            /* Find out what voice we want */
            flowm_selected_voice = 
                ComboBox_GetCurSel(GetDlgItem(hwnd,FL_VOXLIST));
            flowm_selected_relation = 0;

            flowm_duration = 0.0;
            GetSystemTime(&StartTime);
            GetDlgItemText(hwnd, FL_TEXT, szFilePos, FL_MAX_TEXTLEN);
            looptimes = wcstod(szFilePos,&x);
            if (looptimes < 5)
                looptimes = 5;
            wsprintf(fl_tts_msg,L"Bench marking: loop = %d ...",looptimes);
            SetDlgItemText(hwnd, FL_OUTPUT, fl_tts_msg);
            SetDlgItemText(hwnd, FL_TEXT,FL_BENCH_TEXT);
            for (i=0; i<looptimes; i++)
                flowm_say_text(FL_BENCH_TEXT);
            GetSystemTime(&EndTime);
            /* This will fail over noon (?) and midnight */
            elapsedtime = 
                (EndTime.wHour*60.0*60.0+EndTime.wMinute*60.0+
                 EndTime.wSecond+EndTime.wMilliseconds/1000.0) -
                (StartTime.wHour*60.0*60.0+StartTime.wMinute*60.0+
                 StartTime.wSecond+ StartTime.wMilliseconds/1000.0);
            wsprintf(fl_tts_msg,L"%2.1f speech; %2.1f seconds; %2.2f rate;",
                     flowm_duration,
                     elapsedtime,
                     flowm_duration/elapsedtime);
            SetDlgItemText(hwnd, FL_OUTPUT, fl_tts_msg);
            
            flowm_play_status = FLOWM_STOP;
            return TRUE;
        case IDOK:
        case IDCANCEL:
            flowm_play_status = FLOWM_STOP;
            EndDialog(hwnd,0);
            return TRUE;
        }
    break;
    }
    return FALSE;
}

LPARAM DoMainCommandFile(HWND hwnd, WORD idItem, 
                          HWND hwndCtl, WORD wNotifyCode)
{
    DialogBox(hInst, L"filebox", hwnd, FileDlgProc);
    return 0;
}

DWORD WINAPI tts_thread_function(PVOID pArg)
{

    while (flowm_play_status != FLOWM_STOP)
    {
        flowm_say_file(fl_filename);
        if (flowm_play_status == FLOWM_BACKSKIP)
            flowm_play_status = FLOWM_PLAY; /* restart at new place */
        else if (flowm_play_status != FLOWM_STOP)
        {
            /* If we finish the file without an explicit stop, reset */
            /* the file position to the start of the file */
            flowm_file_pos = 0;  
            if (TTSWindow) SetDlgItemText(TTSWindow, FL_FILEPOS, L"0");
            flowm_play_status = FLOWM_STOP;
        }
    }

    return 0x15;
}

static int flowm_find_filesize(TCHAR *filename)
{
    HANDLE fp;
    int fs = 0;

    fp = CreateFile(filename, GENERIC_READ,
                    FILE_SHARE_READ, NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
    if (!fp)
        return -1;

    fs = GetFileSize(fp,NULL);
    CloseHandle(fp);

    return fs;
}

void DisplayOff()
{   /* Switch the screen off now */
    HDC hdc;
    VIDEO_POWER_MANAGEMENT vpm;

    hdc = GetDC(NULL);
    vpm.PowerState = VideoPowerOff;
    vpm.Length = sizeof(VIDEO_POWER_MANAGEMENT);

    ExtEscape(hdc,
              SETPOWERMANAGEMENT,
              sizeof(VIDEO_POWER_MANAGEMENT),
              (LPCSTR)&vpm,
              0,
              NULL);

    return;
}

BOOL CALLBACK FileDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    SHINITDLGINFO idi;
    OPENFILENAME of;
    const LPTSTR pszOpenFilter = L"Text Documents (*.txt)\0*.txt\0\0";
    int nParameter = 5;
    DWORD dwThreadID = 0;
    TCHAR szFilePos[FL_MAX_TEXTLEN];
    TCHAR *x;
    int i;
    float nfilepos;
    HWND hwndvox;

    switch (msg)
    {
    case WM_INITDIALOG:
        idi.dwMask = SHIDIM_FLAGS;
        idi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIZEDLGFULLSCREEN;
        idi.hDlg = hwnd;
        SHInitDialog (&idi);

        if (fl_filename[0] != '\0')
        {
            SetDlgItemText(hwnd, FL_FILENAME, fl_filename);
            flowm_file_pos = 0;
            SetDlgItemText(hwnd, FL_FILEPOS, L"0");
        }

        for (i=0; flowm_voice_name(i); i++)
        {   /* Add regestered voices to the voice dropdown */
            SendDlgItemMessage(hwnd, FL_VOXLIST, CB_ADDSTRING, 
                               0, (LPARAM)flowm_voice_name(i));
        }
        /* Position cursor at start, selecting first voice */
        SendDlgItemMessage(hwnd, FL_VOXLIST, CB_SETCURSEL, 0, 0);

        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case FL_PUSHSELECT:
            /* Find a file to synthesize */
            fl_filename[0] = '\0';
            ZeroMemory(&of,sizeof(of));
            of.lStructSize = sizeof(of);
            of.lpstrFile = fl_filename;
            of.nMaxFile = 256;
            of.lpstrFilter = pszOpenFilter;
            of.Flags = 0;
            of.lpstrTitle = L"Flowm Open";

            if (GetOpenFileName(&of) != 0)
            {
                flowm_play_status = FLOWM_STOP;
                SetDlgItemText(hwnd, FL_FILENAME, fl_filename);

                flowm_file_pos = 0;
                flowm_file_size = flowm_find_filesize(fl_filename);
                flowm_utt_pos_pos = 0;
                flowm_prev_utt_pos[flowm_utt_pos_pos] = 0;
                SetDlgItemText(hwnd, FL_FILEPOS, L"0");
                SetDlgItemText(hwnd, FL_SYNTHTEXT, L"");
                if (flowm_file_size == -1)
                {
                    wsprintf(fl_fp_msg,L"File not found: %s",fl_filename);
                    MessageBox(hwnd, fl_fp_msg,AppName, MB_OK);
                    fl_filename[0] = '\0';
                }
            }
            return TRUE;
        case FL_PUSHTTS:
            /* Synthesize the selected file from the specified position */
            /* with the selected voice */
            if ((flowm_play_status != FLOWM_PLAY) &&
                (fl_filename[0] != '\0'))
            {
                flowm_play_status = FLOWM_PLAY;
                TTSWindow = hwnd;
                /* Find out what voice we want */
                hwndvox = GetDlgItem(hwnd,FL_VOXLIST);
                flowm_selected_voice = ComboBox_GetCurSel(hwndvox);

                /* Update file position, in case user changed it */
                GetDlgItemText(hwnd, FL_FILEPOS, szFilePos, FL_MAX_TEXTLEN);
                nfilepos = wcstod(szFilePos,&x);
                /* Be a little careful we don't reset file position */
                /* when we have an accuracy problem: filepos is an int */
                /* but the display field is the % */
                if (nfilepos > 100) nfilepos = 100.0;
                if (nfilepos < 0) nfilepos = 0.0;
                if (((nfilepos - flowm_find_file_percentage()) > 0.001) ||
                    ((nfilepos - flowm_find_file_percentage()) < -0.001))
                    flowm_file_pos = (int)((nfilepos*flowm_file_size)/100.0);
                
                /* Do the synthesis in a new thread, so we can still process */
                /* button pushes */
                tts_thread = CreateThread(NULL, 0, tts_thread_function, 
                                          (PVOID)nParameter, 0, &dwThreadID);
                CloseHandle(tts_thread);
            }
            return TRUE;
        case FL_PUSHBACK:
            /* Move back to previous utterance */
            if (flowm_utt_pos_pos > 0)
            {   /* Jump back to beginning of previous utt (we know about) */
                flowm_utt_pos_pos--;
                flowm_file_pos = flowm_prev_utt_pos[flowm_utt_pos_pos];
            }
            else if (flowm_file_pos > 1)
                /* Not got any previous -- we never payed that bit get */
                /* so just jump back 100 chars */
                flowm_file_pos -= 100;
            else /* At start of file already */
                flowm_file_pos = 0;
            wsprintf(fl_fp_msg,L"%2.3f",flowm_find_file_percentage());
            SetDlgItemText(TTSWindow, FL_FILEPOS, fl_fp_msg);
            flowm_play_status = FLOWM_BACKSKIP;
            return TRUE;
        case FL_PUSHFORWARD:
            /* Skip forward to next utterances now */
            if ((flowm_play_status == FLOWM_PLAY) ||
                (flowm_play_status == FLOWM_SKIP))
                flowm_play_status = FLOWM_SKIP; /* already playing/skipping */
            else
            {   /* No playing so just jump forward 100 chars */
                flowm_file_pos += 100;
                wsprintf(fl_fp_msg,L"%2.3f",flowm_find_file_percentage());
                SetDlgItemText(TTSWindow, FL_FILEPOS, fl_fp_msg);
            }
            return TRUE;
        case FL_PUSHSTOP:
            flowm_play_status = FLOWM_STOP;

            return TRUE;
        case FL_SCREENOFF:
            /* turn off the screen */
            DisplayOff();
            return TRUE;  
        case IDOK:
            if (flowm_play_status == FLOWM_STOP)
            {   /* Only end if we are in stop mode */
                TTSWindow = 0;  /* just in case there is a race condition */
                EndDialog(hwnd,0);
            }
            return TRUE;  /* you need to press end to end */
        case FL_PUSHEND:
        case IDCANCEL:
            flowm_play_status = FLOWM_STOP;
            TTSWindow = 0;  /* just in case there is a race condition */
            EndDialog(hwnd,0);
            return TRUE;
        }
    break;
    }
    return FALSE;
}

int WinMain(HINSTANCE hInstance,
            HINSTANCE hPrevInstance,
            LPWSTR lpCmdLine,
            int nCmdShow)
{
    MSG msg;
    HWND hwndMain=0;
    int rc;

    rc = InitApp(hInstance);
    if (rc) return rc;

    hwndMain=InitInstance(hInstance,lpCmdLine, nCmdShow);
    if (hwndMain == 0) return 0x10;

    flowm_init();  /* Initialize the TTS system */

    while (GetMessage(&msg,NULL,0,0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    flowm_terminate(); /* Close the TTS systems */

    return msg.wParam;
}

int InitApp(HINSTANCE hInstance)
{
    WNDCLASS wc;
    HWND hWnd;

    /* If already running, return */
    hWnd = FindWindow(AppName,NULL);
    if (hWnd)
    {
        SetForegroundWindow((HWND)(((DWORD)hWnd) | 0x01));
        return -1;
    }

    wc.style = CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc = MainWndProc;  /* main callback function */
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); /* Default cursor */
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  NULL;                  /* Menu name */
    wc.lpszClassName = AppName;             /* Window class name */

    if (RegisterClass(&wc) == 0)
        return 1;
    else
        return 0;
}

HWND InitInstance(HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    HWND hwnd;

    hInst = hInstance;

    hwnd = CreateWindow(AppName,
                        AppName,
                        WS_VISIBLE,
                        CW_USEDEFAULT,  /* x position */
                        CW_USEDEFAULT,  /* y position */
                        CW_USEDEFAULT,  /* Initial width */
                        CW_USEDEFAULT,  /* Initial height */
                        NULL,           /* Parent */
                        NULL,           /* Menu */
                        hInstance,
                        NULL);          /* extra params */
    if (!IsWindow(hwnd))
        return 0;  /* failed to create a window */
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    return hwnd;
}

    
