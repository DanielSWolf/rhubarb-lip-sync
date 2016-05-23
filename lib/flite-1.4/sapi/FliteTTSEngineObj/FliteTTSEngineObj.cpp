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
/*  FliteTTSEngineObj.cpp: implementation of SAPI interface to Flite     */
/*                                                                       */
/*************************************************************************/

#include "FliteTTSEngineObj.h"

extern "C" {
#include "flite.h"
#include "cst_tokenstream.h"
};

static cst_val *sapi_tokentowords(cst_item *i);

STDMETHODIMP
CFliteTTSEngineObj::SetObjectToken(ISpObjectToken* pToken)
{
	USES_CONVERSION;
	CSpDynamicString voxdir;
	char *avoxdir;
	HRESULT hr;
	const cst_val *ttwv;

	if (!SUCCEEDED(hr = SpGenericSetObjectToken(pToken, vox_token)))
		return hr;
	if (!SUCCEEDED(vox_token->GetStringValue(L"voxdir", &voxdir)))
		avoxdir = NULL; /* It isn't always necessary */
	else
		avoxdir = W2A(voxdir);

	if (curr_vox)
		(*unregfunc)(curr_vox);
	if ((curr_vox = (*regfunc)(avoxdir)) == NULL)
		return E_INVALIDARG; /* or something */

	if ((ttwv = feat_val(curr_vox->features, "tokentowords_func"))) {
		feat_set(curr_vox->features, "old_tokentowords_func", ttwv);
		feat_set(curr_vox->features, "tokentowords_func",
			 itemfunc_val(sapi_tokentowords));
	}

	return hr;
}

STDMETHODIMP
CFliteTTSEngineObj::GetOutputFormat(const GUID * pTargetFormatId,
				    const WAVEFORMATEX * pTargetWaveFormatEx,
				    GUID * pDesiredFormatId, 
				    WAVEFORMATEX ** ppCoMemDesiredWaveFormatEx)
{
	WAVEFORMATEX *wfx;

	if (curr_vox == NULL)
		return SpConvertStreamFormatEnum(SPSF_16kHz16BitMono,
						 pDesiredFormatId,
						 ppCoMemDesiredWaveFormatEx);

	/* Just return the default format for the voice, since SAPI
           can do all the necessary conversion.  (our resampling
           algorithm is a lot better than SAPI's, though...) */
	if ((wfx = (WAVEFORMATEX *)CoTaskMemAlloc(sizeof(*wfx))) == NULL)
		return E_OUTOFMEMORY;
	memset(wfx, 0, sizeof(*wfx));
	wfx->nChannels = 1;
	wfx->nSamplesPerSec = get_param_int(curr_vox->features, "sample_rate", 16000);
	wfx->wFormatTag = WAVE_FORMAT_PCM;
	wfx->wBitsPerSample = 16;
	wfx->nBlockAlign = wfx->nChannels*wfx->wBitsPerSample/8;
	wfx->nAvgBytesPerSec = wfx->nSamplesPerSec*wfx->nBlockAlign;

	*pDesiredFormatId = SPDFID_WaveFormatEx;
	*ppCoMemDesiredWaveFormatEx = wfx;

	return S_OK;
}

int
CFliteTTSEngineObj::phoneme_len(cst_item *s)
{
    return (short) ((item_feat_float(s, "end")
                     - ffeature_float(s, "p.end")) * 1000);
}

int
CFliteTTSEngineObj::viseme_len(cst_item *s)
{
	int len, vis, v;

	if (!visemefunc)
		return 0;

	vis = (*visemefunc)(s);
	len = phoneme_len(s);
	for (s = item_next(s); s; s = item_next(s)) {
		v = (*visemefunc)(s);
		if (v != vis)
			break;
		len += phoneme_len(s);
	}

	return len;
}

int
CFliteTTSEngineObj::next_viseme(cst_item *s)
{
	int vis, v;

	if (!visemefunc)
		return 0;

	vis = (*visemefunc)(s);
	for (s = item_next(s); s; s = item_next(s)) {
		v = (*visemefunc)(s);
		if (v != vis)
			break;
	}

	return v;
}

void
CFliteTTSEngineObj::send_item_events()
{
	USES_CONVERSION;
	cst_item *i;
	int sps;

	sps = get_param_int(curr_vox->features, "sample_rate", 16000);
	for (i = relation_head(utt_relation(curr_utt, "Segment"));
	     i; i = item_next(i))
	{
		int offset;
		SPEVENT evt;
		cst_item *token;
		const cst_val *bmark;

		offset = (int) (ffeature_float(i, "p.end") * sps * sizeof(short));
		token = path_to_item(i, "R:SylStructure.parent.parent.R:Token.parent");

		/* Word boundaries */
		if (token
		    && item_parent(item_as(i, "SylStructure"))
		    && item_prev(item_as(i, "SylStructure")) == NULL
		    && item_prev(item_parent(item_as(i, "SylStructure"))) == NULL
		    && (token !=
			path_to_item(i, "R:SylStructure.parent.parent.p."
				     "R:Token.parent")))
		{
			cst_item *prev;

			if ((prev = item_prev(token))
			    && (bmark = item_feat(prev, "bookmark")))
			{
				SPEVENT fooevt; /* SpClearEvent will segfault... */
				fooevt.eEventId = SPEI_TTS_BOOKMARK;
				fooevt.elParamType = SPET_LPARAM_IS_STRING;
				fooevt.ullAudioStreamOffset = bcount + offset;
				fooevt.wParam = atol(val_string(bmark));
				fooevt.lParam = (LPARAM) A2W(val_string(bmark));
				site->AddEvents(&fooevt, 1);
			}
			SpClearEvent(&evt);
			evt.eEventId = SPEI_WORD_BOUNDARY;
			evt.elParamType = SPET_LPARAM_IS_UNDEFINED;
			evt.ullAudioStreamOffset = bcount + offset;
			evt.wParam = item_feat_int(token, "token_length");
			evt.lParam = item_feat_int(token, "token_pos");
			site->AddEvents(&evt, 1);
		}

		/* Visemes */
		if (visemefunc 
		    && ((*visemefunc)(i)
			!= (*visemefunc)(item_prev(i)))) {
			SpClearEvent(&evt);
			evt.eEventId = SPEI_VISEME;
			evt.elParamType = SPET_LPARAM_IS_UNDEFINED;
			evt.ullAudioStreamOffset = bcount + offset;
			evt.wParam = MAKEWPARAM(next_viseme(i), viseme_len(i));
			evt.lParam = MAKELPARAM((*visemefunc)(i), (*featurefunc)(i));
			site->AddEvents(&evt, 1);
		}

		/* Bookmarks:

		   If the previous token is a bookmark and this is the
		   first segment in the first word of the current
		   token, then send an event. (we did that already)

		   If this is the last segment in the utterance
		   (probably a pause) and the last token in the
		   utterance is a bookmark, then send an event. */

		if (item_next(i) == NULL
		    && (bmark = item_feat(relation_tail(utt_relation(curr_utt, "Token")),
					  "bookmark")))
		{
			SPEVENT fooevt; /* SpClearEvent will segfault... */
			fooevt.eEventId = SPEI_TTS_BOOKMARK;
			fooevt.elParamType = SPET_LPARAM_IS_STRING;
			fooevt.ullAudioStreamOffset = bcount + offset;
			fooevt.wParam = atol(val_string(bmark));
			fooevt.lParam = (LPARAM) A2W(val_string(bmark));
			site->AddEvents(&fooevt, 1);
		}

		/* Phonemes */
		if (phonemefunc) {
			SpClearEvent(&evt);
			evt.eEventId = SPEI_PHONEME;
			evt.elParamType = SPET_LPARAM_IS_UNDEFINED;
			evt.ullAudioStreamOffset = bcount + offset;
			evt.wParam = MAKEWPARAM((*phonemefunc)(item_next(i)),
						phoneme_len(i));
			evt.lParam = MAKELPARAM((*phonemefunc)(i),
						(*featurefunc)(i));
			site->AddEvents(&evt, 1);
		}
	}
}

void
CFliteTTSEngineObj::send_sentence_event(int fpos)
{
	SPEVENT evt;

	SpClearEvent(&evt);
	evt.eEventId = SPEI_SENTENCE_BOUNDARY;
	evt.elParamType = SPET_LPARAM_IS_UNDEFINED;
	evt.ullAudioStreamOffset = bcount;
	evt.lParam = sentence_start;
	evt.wParam = fpos - sentence_start;
	site->AddEvents(&evt, 1);

	sentence_start = fpos;
}

void
CFliteTTSEngineObj::synth_one_utt()
{
	cst_wave *wav;
	ULONG b;

	if (!(curr_utt && tok_rel && relation_head(tok_rel)))
		return;

	feat_copy_into(curr_vox->features, curr_utt->features);
	if (curr_vox->utt_init)
		curr_vox->utt_init(curr_utt, curr_vox);
	utt_synth_tokens(curr_utt);
	wav = utt_wave(curr_utt);
	send_item_events();
	if (volume != 100)
		cst_wave_rescale(wav, volume * 65536 / 100);
	site->Write(wav->samples,
		    wav->num_samples * sizeof(*wav->samples),
		    &b);
	bcount += wav->num_samples * sizeof(*wav->samples);
	delete_utterance(curr_utt);
	curr_utt = NULL;
	tok_rel = NULL;
}

void
CFliteTTSEngineObj::start_new_utt()
{
	curr_utt = new_utterance();
	tok_rel = utt_relation_create(curr_utt, "Token");
}

/* snarfed from flite.c */
static int
utt_break(cst_tokenstream *ts, const char *token, cst_relation *tokens)
{
    /* This is English (and some other latin based languages) */
    /* so it shouldn't be here                                */
    const char *postpunct;
    const char *ltoken;
    cst_item *tail = relation_tail(tokens);

    if (!item_feat_present(tail, "punc"))
	    return FALSE;
	
    ltoken = item_name(tail);
    postpunct = item_feat_string(tail, "punc");

    if (strchr(ts->whitespace,'\n') != cst_strrchr(ts->whitespace,'\n'))
	 /* contains two new lines */
	 return TRUE;
    else if (strchr(postpunct,':') ||
	     strchr(postpunct,'?') ||
	     strchr(postpunct,'!'))
	return TRUE;
    else if (strchr(postpunct,'.') &&
	     (strlen(ts->whitespace) > 1) &&
	     strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZ",token[0]))
	return TRUE;
    else if (strchr(postpunct,'.') &&
	     /* next word starts with a capital */
	     strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZ",token[0]) &&
	     !strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZ",ltoken[strlen(ltoken)-1]))
	return TRUE;
    else
	return FALSE;
}

static cst_val *
sapi_tokentowords(cst_item *i)
{
	const cst_val *ttwv;

	if (item_feat_present(i, "phones"))
		return default_tokentowords(i);
	if (item_feat_present(i, "bookmark"))
		return NULL;

	ttwv = utt_feat_val(item_utt(i), "old_tokentowords_func");
	if (ttwv)
		return (*val_itemfunc(ttwv))(i);
	else
		return default_tokentowords(i);
}

static cst_tokenstream *
my_ts_open_string(cst_utterance *utt, const char *text)
{
	cst_tokenstream *ts;

	ts = ts_open_string(text);
	ts->whitespacesymbols = 
		get_param_string(utt->features, "text_whitespace",
				 ts->whitespacesymbols);
	ts->singlecharsymbols = 
		get_param_string(utt->features, "text_singlecharsymbols",
				 ts->singlecharsymbols);
	ts->prepunctuationsymbols = 
		get_param_string(utt->features, "text_prepunctuation",
				 ts->prepunctuationsymbols);
	ts->postpunctuationsymbols = 
		get_param_string(utt->features,"text_pospunctuation",
				 ts->postpunctuationsymbols);

	return ts;
}

static cst_item *
append_new_token(cst_tokenstream *ts, const char *token, cst_relation *rel)
{
	cst_item *t;

	t = relation_append(rel, NULL);
	item_set_string(t, "name", token);
	if (ts) {
		item_set_string(t, "whitespace", ts->whitespace);
		item_set_string(t, "prepunctuation", ts->prepunctuation);
		item_set_string(t, "punc", ts->postpunctuation);
	}
	return t;
}

/* Each step is prev * (10**(1/10)) */
static const double sapi_ratetab_foo[21] = {
	3,
	2.68787537952229,
	2.40822468528069,
	2.15766927997459,
	1.93318204493176,
	1.73205080756888,
	1.55184557391536,
	1.39038917031591,
	1.24573093961552,
	1.11612317403391,
	1,
	0.895958459840763,
	0.802741561760231,
	0.719223093324865,
	0.644394014977255,
	0.577350269189626,
	0.517281857971787,
	0.46346305677197,
	0.415243646538506,
	0.372041058011302,
	0.333333333333333
};
static const double *sapi_ratetab = sapi_ratetab_foo + 10;

/* Each step is prev * (2**(1/24)) */
static const double sapi_pitchtab_foo[21] = {
	1.33333333333333,
	1.29537592153814,
	1.25849908357559,
	1.22267205760623,
	1.18786495752045,
	1.15404874800819,
	1.12119522033829,
	1.0892769688274,
	1.0582673679788,
	1.02814055027196,
	1.0,
	0.970435455228124,
	0.942809041582063,
	0.915969098305392,
	0.889893236113355,
	0.864559703100672,
	0.839947366596581,
	0.816035695536436,
	0.792804743335147,
	0.770235131248181,
	0.75
};
static const double *sapi_pitchtab = sapi_pitchtab_foo + 10;

static double
convert_sapi_rate(int r)
{
	if (r < -10)
		r = -10;
	else if (r > 10)
		r = 10;
	return sapi_ratetab[r];
}

static double
convert_sapi_pitch(int p)
{
	if (p == -24)
		return 0.5;
	else if (p == 24)
		return 2.0;
	else {
		if (p < -10)
			p = -10;
		else if (p > 10)
			p = 10;
		return sapi_pitchtab[p];
	}
}

static void
set_local_prosody(cst_item *t, const SPVSTATE *st)
{
	int r, p;

	if (st->EmphAdj) /* This doesn't really work, though */
		item_set_int(t, "EMPH", st->EmphAdj);
	if ((r = st->RateAdj))
		item_set_float(t, "local_duration_stretch",
			       convert_sapi_rate(r));
	if ((p = st->PitchAdj.MiddleAdj))
		item_set_float(t, "local_f0_shift",
			       convert_sapi_pitch(p));
	/* We could do something with RangeAdj, but SAPI apparently
           doesn't use it yet and the documentation doesn't say what
           the value passed in it would mean, anyway. */
}

void
CFliteTTSEngineObj::speak_frag()
{
	cst_tokenstream *ts;
	const char *token;
	cst_item *t;
	char *text;
	size_t len;

	len = wcstombs(NULL, curr_frag->pTextStart, curr_frag->ulTextLen);
	text = cst_alloc(char, len+1);
	wcstombs(text, curr_frag->pTextStart, curr_frag->ulTextLen);
	ts = my_ts_open_string(curr_utt, text);
	cst_free(text);

	while (!ts_eof(ts)) {
		token = ts_get(ts);
		if (relation_head(tok_rel) && utt_break(ts, token, tok_rel)) {
			get_actions_and_do_them();
			if (aborted)
				goto pod_bay_doors;
			if (sentence_skip == 0) {
				send_sentence_event(ts->token_pos
						    + curr_frag->ulTextSrcOffset);
				synth_one_utt();
				start_new_utt();
			} else {
				--sentence_skip;
				site->CompleteSkip(1);
				start_new_utt();
			}
		}

		t = append_new_token(ts, token, tok_rel);
		item_set_int(t, "token_pos",
			     ts->token_pos + curr_frag->ulTextSrcOffset);
		item_set_int(t, "token_length", strlen(token));
		set_local_prosody(t, &curr_frag->State);
	}
pod_bay_doors: /* Open the POD bay doors, KAL... */
	delete_tokenstream(ts);
}

void
CFliteTTSEngineObj::spell_frag()
{
	int i;

	for (i = 0; i < curr_frag->ulTextLen; ++i) {
		cst_item *t;
		char *c;
		int len;

		if (!iswspace(curr_frag->pTextStart[i])) {
			len = wcstombs(NULL, curr_frag->pTextStart + i, 1);
			c = cst_alloc(char, len + 1);
			wcstombs(c, curr_frag->pTextStart + i, 1);

			t = append_new_token(NULL, c, tok_rel);
			item_set_int(t, "token_pos", curr_frag->ulTextSrcOffset + i);
			item_set_int(t, "token_length", 1);
			set_local_prosody(t, &curr_frag->State);
			cst_free(c);
		}
	}
}

void
CFliteTTSEngineObj::pronounce_frag()
{
	cst_val *phones;
	cst_item *t;
	
	if (!pronouncefunc
	    || (phones = (*pronouncefunc)(curr_frag->State.pPhoneIds)) == NULL)
		return;

	t = append_new_token(NULL, "", tok_rel);
	item_set_int(t, "token_pos", curr_frag->ulTextSrcOffset);
	item_set_int(t, "token_length", curr_frag->ulTextLen);
	set_local_prosody(t, &curr_frag->State);
	item_set(t, "phones", phones);
}

void
CFliteTTSEngineObj::silence_frag()
{
	int sps, silence;
	short *buf;
	ULONG b;

	/* Force an utterance break */
	synth_one_utt();
	start_new_utt();

	sps = get_param_int(curr_vox->features, "sample_rate", 16000);
	silence = sps * curr_frag->State.SilenceMSecs / 1000;
	buf = cst_alloc(short, 512);
	while (silence > 0) {
		b = ((silence < 512) ? silence : 512) * sizeof(*buf);
		site->Write(buf, b, &b);
		bcount += b;
		silence -= (b / sizeof(*buf));
	}
}

void
CFliteTTSEngineObj::set_bookmark()
{
	size_t len;
	char *bookmark;
	cst_item *t;

	len = wcstombs(NULL, curr_frag->pTextStart, curr_frag->ulTextLen);
	bookmark = cst_alloc(char, len+1);
	wcstombs(bookmark, curr_frag->pTextStart, curr_frag->ulTextLen);

	t = relation_append(tok_rel, NULL);
	item_set_string(t, "name", "");
	item_set_string(t, "bookmark", bookmark);
	item_set_int(t, "token_pos", curr_frag->ulTextSrcOffset);
	item_set_int(t, "token_length", curr_frag->ulTextLen);
	cst_free(bookmark);
}

void
CFliteTTSEngineObj::get_actions_and_do_them()
{
	DWORD actions;

	actions = site->GetActions();
	if (actions & SPVES_ABORT) {
		aborted = 1;
		return;
	}
	if (actions & SPVES_SKIP) {
		SPVSKIPTYPE stype;
		long count;

		site->GetSkipInfo(&stype, &count);
		if (stype == SPVST_SENTENCE) {
			sentence_skip += count;
		}
	}
	if (actions & SPVES_RATE) {
		long adj;

		site->GetRate(&adj);
		utt_set_feat_float(curr_utt, "duration_stretch",
				   convert_sapi_rate(adj));
		feat_set_float(curr_vox->features, "duration_stretch",
			       convert_sapi_rate(adj));
	}
	if (actions & SPVES_VOLUME) {
		USHORT adj;

		site->GetVolume(&adj);
		volume = adj;
	}
}

STDMETHODIMP
CFliteTTSEngineObj::Speak(DWORD dwSpeakFlags,
			  REFGUID rguidFormatId,
			  const WAVEFORMATEX * pWaveFormatEx,
			  const SPVTEXTFRAG* pTextFragList,
			  ISpTTSEngineSite* pOutputSite)
{
	if (curr_vox == NULL)
		return E_POINTER;

	start_new_utt();
	bcount = 0;
	sentence_start = 0;
	sentence_skip = 0;
	aborted = 0;
	volume = 100;
	site = pOutputSite;

	for (curr_frag = pTextFragList; curr_frag && !aborted;
	     curr_frag = curr_frag->pNext) {
		get_actions_and_do_them();

		switch (curr_frag->State.eAction) {
		case SPVA_Speak:
			speak_frag();
			break;
		case SPVA_SpellOut:
			spell_frag();
			break;
		case SPVA_Pronounce:
			pronounce_frag();
			break;
		case SPVA_Silence:
			silence_frag();
			break;
		case SPVA_Bookmark:
			set_bookmark();
			break;
		default:
			break;
		}
	}

	if (tok_rel && relation_tail(tok_rel))
		send_sentence_event(item_feat_int(relation_tail(tok_rel), "token_pos")
				    + item_feat_int(relation_tail(tok_rel),
						    "token_length"));
	synth_one_utt();

	delete_utterance(curr_utt);
	curr_utt = NULL;
	tok_rel = NULL;
	site = NULL;
	
	return S_OK;
}
