///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                           Cepstral, LLC                               //
//                        Copyright (c) 2001                             //
//                        All Rights Reserved.                           //
//                                                                       //
//  Permission is hereby granted, free of charge, to use and distribute  //
//  this software and its documentation without restriction, including   //
//  without limitation the rights to use, copy, modify, merge, publish,  //
//  distribute, sublicense, and/or sell copies of this work, and to      //
//  permit persons to whom this work is furnished to do so, subject to   //
//  the following conditions:                                            //
//   1. The code must retain the above copyright notice, this list of    //
//      conditions and the following disclaimer.                         //
//   2. Any modifications must be clearly marked as such.                //
//   3. Original authors' names are not deleted.                         //
//   4. The authors' names are not used to endorse or promote products   //
//      derived from this software without specific prior written        //
//      permission.                                                      //
//                                                                       //
//  CEPSTRAL, LLC AND THE CONTRIBUTORS TO THIS WORK DISCLAIM ALL         //
//  WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED       //
//  WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL         //
//  CEPSTRAL, LLC NOR THE CONTRIBUTORS BE LIABLE FOR ANY SPECIAL,        //
//  INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER          //
//  RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION    //
//  OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR  //
//  IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.          //
//                                                                       //
///////////////////////////////////////////////////////////////////////////
//             Author:  David Huggins-Daines (dhd@cepstral.com)          //
//               Date:  November 2001                                    //
///////////////////////////////////////////////////////////////////////////

// FliteCMUKalDiphone.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f FliteCMUKalDiphoneps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "FliteCMUKalDiphone.h"

#include "FliteCMUKalDiphone_i.c"
#include "FliteCMUKalDiphoneObj.h"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_FliteCMUKalDiphoneObj, CFliteCMUKalDiphoneObj)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_FLITECMUKALDIPHONELib);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}


