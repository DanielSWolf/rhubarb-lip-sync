/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                      Copyright (c) 2004-2005                          */
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
/*               Date:  November 2004                                    */
/*************************************************************************/
/*                                                                       */
/*  FLOP -- FLite On PalmOS                                              */
/*                                                                       */
/*  A demonstration program to show Flite speaking on a PalmOS device    */
/*                                                                       */
/*  I'd like to acknowledge help from the tutorials in                   */
/*     "Palm OS Programming" by Neil Rhodes and Julie McKeehan           */
/*     O'Reilly 2002.                                                    */
/*  and http://mobile.eric-poncet.com/palm/tutorial/                     */
/*                                                                       */
/*************************************************************************/

#include <PalmOS.h>
#include <PalmTypes.h>
#include <PalmCompatibility.h>
#include <System/SystemPublic.h>
#include <System/SoundMgr.h>
#include <UI/UIPublic.h>

#include "palm_flite.h"
#include "flop.h"

typedef struct PlayInfo_struct {
    unsigned long int active;
    unsigned long int p;
    short *samples;
    unsigned long int num_samples;
    unsigned long int sample_rate;
} PlayInfo;

#define ROM_VERSION_REQUIRED_FOR_FLOP  0x04000000 /* 5.0 or later */

static Boolean FlopStop = false;
static Boolean FlopPlay = false;
static UInt32 FlopOutputType = FliteOutputTypeWave;
static char *FlopOutputTypes[4] =
{
    "Wave",
    "Phones",
    "Words",
    0
};

/* static char *TextBuffer = "4.3% liked Henry I on May 5."; */
static char *TextBuffer = "Flite is a small portable run-time synthesizer.  It can run under Unix, Windows, Windows C.E. and Palm O.S.";
static char *FileNameBuffer = "*scratch*";
flite_info *flite = 0;
static char output[FlopMaxOutputChars];
static char input[FlopMaxOutputChars];

SndStreamRef playstream;
PlayInfo playdata;

static Err StopPlayStream(void)
{
    if (playdata.samples)  
    {   /* Wait for anything currently playing to end */
	SndStreamStop(playstream);
	/* free up waveform */
	MemPtrFree(playdata.samples);
	playdata.num_samples = 0;
	playdata.samples = 0;
	playdata.active = false;
	playdata.p = 0;
    }
    
    return errNone;
}

static Err DrainPlayStream(void)
{
    if (playdata.samples)  
    {   /* Wait for anything currently playing to end */
	while (playdata.active)
	    SndStreamStart(playstream);
	StopPlayStream();
    }

    return errNone;
}

static Err PlayStreamCallBack(void *vplaydata, SndStreamRef stream,
			      void *voutbuffer, UInt32 count)
{
    /* Play count samples to outbuffer */
    unsigned long int i;
    PlayInfo *playdata = (PlayInfo *)vplaydata;
    short *outbuffer = (short *)voutbuffer;

    for (i=0; i<count; i++)
    {
	if (playdata->p < playdata->num_samples)
	{
	    outbuffer[i] = playdata->samples[playdata->p];
	    playdata->p++;
	}
	else
	{
	    playdata->active = false;
	    playdata->p = 0;
	    outbuffer[i] = 0;
	}
    }

    return errNone;
}

static Err SetupPlayStream(flite_info *flite)
{
    /* setup playdata */
    playdata.p = 0;
    playdata.samples = flite->samples;
    playdata.num_samples = flite->num_samples;
    playdata.sample_rate = flite->sample_rate;
    flite->samples = 0;
    flite->num_samples = 0;
    playdata.active = true;

    /* create play stream */
    SndStreamCreate(&playstream,
		    sndOutput,
		    playdata.sample_rate,
		    sndInt16Little,  /* arm shorts */
		    sndMono,
		    PlayStreamCallBack,
		    &playdata,
		    1024,
		    false);

    return errNone;
}

static void SetField(UInt16 formID, UInt16 fieldID, const char *str)
{
    FormPtr frm;
    FieldPtr fld;
    UInt16 obj;
    CharPtr p;
    VoidHand h;
	
    frm = FrmGetFormPtr(formID);
    obj = FrmGetObjectIndex(frm, fieldID);
    fld = (FieldPtr)FrmGetObjectPtr(frm, obj);
    h = (VoidHand)FldGetTextHandle(fld);
    if (h == NULL)
    {
	h = MemHandleNew (FldGetMaxChars(fld)+1);
	ErrFatalDisplayIf(!h, "No Memory");
    }
	
    p = (CharPtr)MemHandleLock(h);
    StrCopy(p, str);
    MemHandleUnlock(h);
	
    FldSetTextHandle(fld, (Handle)h);
}

static void FlopSpeakCmd(UInt16 formID, UInt16 ifieldID, UInt16 ofieldID)
{
    FormPtr frm;
    FieldPtr fld;
    UInt16 obj;
    CharPtr p;
    VoidHand h;
    UInt32 processor_type;
    const char *ch=NULL;
    
    frm = FrmGetFormPtr(formID);
    obj = FrmGetObjectIndex(frm, ifieldID);
    fld = (FieldPtr)FrmGetObjectPtr(frm, obj);
    h = (VoidHand)FldGetTextHandle(fld);
    p = (CharPtr)MemHandleLock(h);

    /* Check we are on an ARM */
    FtrGet(sysFileCSystem, sysFtrNumProcessorID, &processor_type);
    if (processor_type == sysFtrNumProcessorx86)
	ch = "Flite doesn't run on the emulator";
    else if (!sysFtrNumProcessorIsARM(processor_type))
	ch = "Flite requires an ARM processor";
    else
    {   /* We're on an ARM -- so setup flite */
	if (!flite) 
	{
	    flite = flite_init();
	    if (flite == 0)
		ch = "Flite failed to initialize";
	    else
	    {
		flite->PlayPosition = 0;
		flite->output = output;
		flite->max_output = FlopMaxOutputChars;
		ch = flite->output;
	    }
	}

	if (FlopPlay == false)
	{
	    FlopPlay = true;
	    StrPrintF(input,"%s",p);
	    if (flite->utt_length == 0)
		flite->start = 0;
	    /* toggle play/stop button to play */
	}
	else
	{
	    FlopPlay = false;
	    flite->start = flite->PlayPosition;
	    /* toggle play/stop button to stop */
	}
    }

    if (ch)
    {
	/* Update the output field with any new information */
	SetField(FlopForm, FlopOutput, ch);
	FrmDrawForm(FrmGetFormPtr(FlopForm));
    }

    MemHandleUnlock(h);
}

static Boolean FlopFormHandleEvent(EventPtr event)
{
    Boolean handled = false;
    UInt16 value;
    FieldPtr field;
    FormPtr form;
    int fieldid;
    UInt16 *curval;
    static UInt16 curinval = 0;
    static UInt16 curoutval = 0;

    form = FrmGetFormPtr(FlopForm);

    switch (event->eType)
    {
    case frmOpenEvent:
	SetField(FlopForm, FlopText , TextBuffer);
	SetField(FlopForm, FlopFileName , FileNameBuffer);
	FrmDrawForm(form);
	handled = true;
	break;
    case sclRepeatEvent:

	value = event->data.sclRepeat.newValue;
	if (event->data.sclRepeat.scrollBarID == FlopInScrollBar)
	{   /* its the input field scrollbar */
	    fieldid = FlopText;
	    curval = &curinval;
	}
	else
	{   /* its the output field scrollbar */
	    fieldid = FlopOutput;
	    curval = &curoutval;
	}

	field = 
	    (FieldPtr)FrmGetObjectPtr(form,FrmGetObjectIndex(form,fieldid));

	if (value > *curval)
	    FldScrollField(field,value-*curval, winDown);
	else
	    FldScrollField(field, *curval-value, winUp);
	*curval = value;

	break;
    case ctlSelectEvent:
	switch (event->data.ctlSelect.controlID)
	{
	case FlopSpeak:
	    FlopSpeakCmd(FlopForm,FlopText,FlopOutput);
	    FrmDrawForm(form);
	    handled = true;
	    break;
	case FlopShutup:
	    StopPlayStream();
	    handled = true;
	    break;
	case FlopExit:
	    FlopStop = true;
	    handled = true;
	    break;
	default:
	    break;
	}
	break;
    case popSelectEvent:
	FlopOutputType = event->data.popSelect.selection;
	CtlSetLabel(event->data.popSelect.controlP,
		    FlopOutputTypes[FlopOutputType]);
	handled = true;
	break;
    case menuEvent:
	MenuEraseStatus(NULL);
	switch (event->data.menu.itemID)
	{
	case FlopOptionsAboutCmd:
	    (void)FrmAlert(FlopAboutAlert);
	    handled = true;
	    break;
	default:
	    break;
	}
	break;

    default:
	break;
    }

    return handled;
}

static Boolean AppHandleEvent(EventPtr event)
{
    FormPtr frm;
    Int32 formId;
    Boolean handled;

    handled = false;

    if (event->eType == frmLoadEvent)
    {
	formId = event->data.frmLoad.formID;
	frm = FrmInitForm(formId);
	FrmSetActiveForm(frm);
	switch (formId)
	{
	case FlopForm:
	    FrmSetEventHandler(frm, FlopFormHandleEvent);
	    handled = true;
	    break;
				
	default:
	    break;
	}
    }

    return handled;
}

static void AppEventLoop(void)
{
    Err error;
    EventType event;

    do 
    {
	/* we should wait for 100ms only so we can server the audio */
	EvtGetEvent(&event, evtWaitForever);
    
	if (SysHandleEvent(&event)) continue;
	if (MenuHandleEvent(0, &event, &error)) continue;
	if (AppHandleEvent(&event)) continue;

	FrmDispatchEvent(&event);

    	/* Serve playing */
	if (FlopPlay == true)
	{
	    if (!flite->samples)
	    {   /* Got stuff to play and we're not currently playing */
		flite->type = FlopOutputType; /* words, phones, wave, stream */
		flite->text = input;               /* text to be synthesized */
		flite_synth_text(flite);                 /* do the synthesis */
		flite->WavePosition = flite->start;
		flite->start += flite->utt_length;        /* update position */
		
		/* highlight the area being spoken */

		if (flite->type != FliteOutputTypeWave)
		    FlopPlay = false;  /* we don't do async play on words/ph */

	    }

	    if (playdata.samples && !playdata.active)
		/* stop and tidy up anything that's finished playing */
		StopPlayStream();
	    else if (flite->samples && !playdata.active)
	    {   /* create a new stream and start it playing */
		flite->PlayPosition = flite->WavePosition;
		SetupPlayStream(flite);
		SndStreamStart(playstream);
	    }
	    else if (!playdata.active)
		/* go into stop mode as there is nothing more to play */
		FlopPlay = false;   /* nothing more to play */
	}
	else
	{
	    if (flite && flite->output)
		StrPrintF(flite->output,"stopped %d",flite->PlayPosition);
	    StopPlayStream();
	    /* flush any other waveform waiting to play */
	    if (flite && flite->samples)
	    {
	    	MemPtrFree(flite->samples);
		flite->num_samples = 0;
		flite->samples = 0;
	    }
	}
	if (flite && flite->output)
	{   /* should be within if above, but for debugging ... */
	    /* Update the output field with any new information */
	    SetField(FlopForm, FlopOutput, flite->output);
	    FrmDrawForm(FrmGetFormPtr(FlopForm));
	}
    } 
    while ((FlopStop==false) && (event.eType != appStopEvent));

    return;
}

static Err AppStart(void)
{
    FlopStop = false;
    return errNone;
}

static void AppStop(void)
{
    if (flite)
    {
	StopPlayStream();
	flite_end(flite);
    }

    FrmCloseAllForms();
    return;
}

UInt32 PilotMain(UInt16 launchCode, 
		 MemPtr launchParameters, 
		 UInt16 launchFlags)
{
    Err error;
    UInt32 romVersion;

    FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
    if (romVersion < ROM_VERSION_REQUIRED_FOR_FLOP)
    {
	FrmAlert(RomIncompatibleAlert);
	return (sysErrRomIncompatible);
    }

    switch(launchCode)
    {
    case sysAppLaunchCmdNormalLaunch:

	error = AppStart();
	if (error) return error;

	FrmGotoForm(FlopForm);
	AppEventLoop();
	AppStop();
	break;
	
    default:
	break;
    }

    return errNone;
}

