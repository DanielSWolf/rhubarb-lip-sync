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
// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__7848D3BF_1C18_45AB_8164_4BE817F36420__INCLUDED_)
#define AFX_STDAFX_H__7848D3BF_1C18_45AB_8164_4BE817F36420__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__7848D3BF_1C18_45AB_8164_4BE817F36420__INCLUDED)
