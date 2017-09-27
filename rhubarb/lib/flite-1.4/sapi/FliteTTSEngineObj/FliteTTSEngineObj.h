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
/*               Date:  December 2001                                    */
/*************************************************************************/
/*                                                                       */
/*  FliteTTSEngineObj.h: implementation of SAPI interface to Flite       */
/*                                                                       */
/*************************************************************************/

#ifndef __FLITETTSENGINEOBJ_H_
#define __FLITETTSENGINEOBJ_H_

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <spddkhlp.h>
#include <spcollec.h>
#include "flite.h"

class ATL_NO_VTABLE CFliteTTSEngineObj :
	public CComObjectRootEx<CComMultiThreadModel>,
	public ISpTTSEngine,
	public ISpObjectWithToken
{
public:
	CFliteTTSEngineObj() {
		flite_init();
		curr_utt = NULL;
		curr_vox = NULL;
	}
	~CFliteTTSEngineObj() {
		if (curr_utt) delete_utterance(curr_utt);
		if (unregfunc && curr_vox) (*unregfunc)(curr_vox);
	}

/* ISpObjectWithToken methods */
public:
	STDMETHODIMP SetObjectToken(ISpObjectToken * pToken);
	STDMETHODIMP GetObjectToken(ISpObjectToken** ppToken)
		{ return SpGenericGetObjectToken(ppToken, vox_token); }

/* ISpTTSEngine methods */
public:
	STDMETHOD(Speak)(DWORD dwSpeakFlags,
			 REFGUID rguidFormatId, const WAVEFORMATEX * pWaveFormatEx,
			 const SPVTEXTFRAG* pTextFragList, ISpTTSEngineSite* pOutputSite);
	STDMETHOD(GetOutputFormat)(const GUID * pTargetFormatId,
				   const WAVEFORMATEX * pTargetWaveFormatEx,
				   GUID * pDesiredFormatId, 
				   WAVEFORMATEX ** ppCoMemDesiredWaveFormatEx);

/* Implementation stuff */
protected:
	/* These get set by a subclass's constructor.  That's not the
           proper C++ way to do this, but I do not care. */
	cst_voice *(*regfunc)(const char *);
	void (*unregfunc)(cst_voice *);
	int (*phonemefunc)(cst_item *);
	int (*visemefunc)(cst_item *);
	int (*featurefunc)(cst_item *);
	cst_val *(*pronouncefunc)(SPPHONEID *);

	/* SAPI's use of the term "token" is quite unfortunate. */
	CComPtr<ISpObjectToken> vox_token;
	cst_voice *curr_vox;

	/* Synthesis state variables */
	cst_utterance *curr_utt;
	cst_relation *tok_rel;
	const SPVTEXTFRAG *curr_frag;
	ISpTTSEngineSite *site;
	ULONGLONG bcount;
	int sentence_start;
	int sentence_skip;
	int aborted;
	int volume;

private:
	/* These have no earthly business being in a header file, but
           they need access to the instance data. */
	void start_new_utt();
	void synth_one_utt();
	void send_item_events();
	void send_sentence_event(int fpos);
	void get_actions_and_do_them();

	int next_viseme(cst_item *s);
	int viseme_len(cst_item *s);
	int phoneme_len(cst_item *s);

	void speak_frag();
	void spell_frag();
	void pronounce_frag();
	void silence_frag();
	void set_bookmark();
};

#endif //__FLITETTSENGINEOBJ_H_
