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
/*  FliteCMUKalDiphone.h: instantiation of Flite SAPI for cmu_us_kal     */
/*                                                                       */
/*************************************************************************/

#ifndef __FLITECMUKALDIPHONEOBJ_H_
#define __FLITECMUKALDIPHONEOBJ_H_

#include "resource.h"
#include "FliteTTSEngineObj.h"
#include "flite_sapi_usenglish.h"

extern "C" {
#include "voxdefs.h"
cst_voice *REGISTER_VOX(const char *voxdir);
void UNREGISTER_VOX(cst_voice *vox);
};

class ATL_NO_VTABLE CFliteCMUKalDiphoneObj : 
	public CComCoClass<CFliteCMUKalDiphoneObj, &CLSID_FliteCMUKalDiphoneObj>,
	public CFliteTTSEngineObj
{
public:
	DECLARE_REGISTRY_RESOURCEID(IDR_FLITECMUKALDIPHONEOBJ)

	BEGIN_COM_MAP(CFliteCMUKalDiphoneObj)
		COM_INTERFACE_ENTRY(ISpTTSEngine)
		COM_INTERFACE_ENTRY(ISpObjectWithToken)
	END_COM_MAP()

public:
	CFliteCMUKalDiphoneObj() {
		regfunc = REGISTER_VOX;
		unregfunc = UNREGISTER_VOX;
		phonemefunc = flite_sapi_usenglish_phoneme;
		visemefunc = flite_sapi_usenglish_viseme;
		featurefunc = flite_sapi_usenglish_feature;
		pronouncefunc = flite_sapi_usenglish_pronounce;
	}
	~CFliteCMUKalDiphoneObj() {}
};

#endif //__FLITECMUKALDIPHONEOBJ_H_
