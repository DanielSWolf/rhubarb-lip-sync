/**********
 * Copyright (c) 2004 Greg Parker.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GREG PARKER ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **********/

#include "pealstub.h"
#include <stdint.h>
#include <PceNativeCall.h>

// Emulator state for calls back into 68K (set by PealArmStub)
const void *gEmulStateP = 0;
Call68KFuncType *gCall68KFuncP = 0;


// Must match struct PealArgs in m68k/peal.c
typedef struct {
    void *fn;
    void *arg;
    void *got;
} PealArgs;


// reads unaligned and byte-swaps
#define read32(a) \
     ( ((((unsigned char *)(a))[0]) << 24) | \
       ((((unsigned char *)(a))[1]) << 16) | \
       ((((unsigned char *)(a))[2]) <<  8) | \
       ((((unsigned char *)(a))[3]) <<  0) )

// interfacearm attribute means PealArmStub can be called from 
// non-interworking ARM code like PceNativeCall if PealArmStub is 
// compiled as Thumb code.

uint32_t PealArmStub(const void *emulStateP, PealArgs *args, Call68KFuncType *call68KFuncP)
{
    uint32_t (*fn)(void *);
    void *arg;
    register void *got asm("r10");
    const void *oldEmulStateP;
    Call68KFuncType *oldCall68KFuncP;
    uint32_t result;

    // Unpack args from PealCall() and store GOT in R10
    fn = (uint32_t(*)(void *)) read32(&args->fn);
    arg = (void *) read32(&args->arg);
    got = (void *) read32(&args->got);

    // Save old PceNativeCall values and set new ones
    oldEmulStateP = gEmulStateP; gEmulStateP = emulStateP;
    oldCall68KFuncP = gCall68KFuncP; gCall68KFuncP = call68KFuncP;

    // Call the function
    result = (*fn)(arg);

    // Restore old PceNativeCall values
    gEmulStateP = oldEmulStateP;
    gCall68KFuncP = oldCall68KFuncP;

    return result;
}
