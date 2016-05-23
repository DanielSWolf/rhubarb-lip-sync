/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                         Copyright (c) 2004                            */
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
/*               Date:  December 2004                                    */
/*************************************************************************/
/*                                                                       */
/*  Glue code for m68k calling Flite API functions                       */
/*                                                                       */
/*************************************************************************/

#include <PalmOS.h>
#include "palm_flite.h"

unsigned long int Large_Alloc(unsigned long int size)
{
    /* Call to MemGluePtrNew() for large allocs */
    void *p;
    p = MemGluePtrNew(size);
    MemSet(p,size,0);
    return (unsigned long int)p;
}

flite_info *flite_init(void)
{
    flite_info *fi=0;
    PealModule *m;
    void **Callback_arm;

    /* Load the main arm_flite code */
    m = PealLoadFromResources('armc',1000);
    if (m == 0) return 0;

    /* Set up the call back for getting large memory allocs */
    Callback_arm = (void **)PealLookupSymbol(m,"Large_Alloc_Callback_m68k");
    *Callback_arm = (void *)SWAPLONG(&Large_Alloc);

    fi = MemPtrNew(sizeof(flite_info));
    MemSet(fi,sizeof(flite_info),0);
    fi->m = m;

    fi->segs = MemPtrNew(FLITE_NUM_BIG_SEGMENTS*sizeof(flite_mem_segment *));
    fi->segs[FLITE_CLTS] = fms_load('clts',1000);
    if (!fi->segs[FLITE_CLTS]) { flite_end(fi); return 0; }
    fi->segs[FLITE_CLEX] = fms_load('clex',1000);
    if (!fi->segs[FLITE_CLEX]) { flite_end(fi); return 0; }
    fi->segs[FLITE_CLPC] = fms_load('clpc',1000);
    if (!fi->segs[FLITE_CLPC]) { flite_end(fi); return 0; }
    fi->segs[FLITE_CRES] = fms_load('cres',1000);
    if (!fi->segs[FLITE_CRES]) { flite_end(fi); return 0; }
    fi->segs[FLITE_CRSI] = fms_load('crsi',1000);
    if (!fi->segs[FLITE_CRSI]) { flite_end(fi); return 0; }

    fi->num_samples = 0;
    fi->samples = 0;

    return fi;
}

void flite_end(flite_info *fi)
{
    if (!fi) return;

    if (fi->m) PealUnload(fi->m);
    fms_unload(fi->segs[FLITE_CLTS]);
    fms_unload(fi->segs[FLITE_CLEX]);
    fms_unload(fi->segs[FLITE_CLPC]);
    fms_unload(fi->segs[FLITE_CRES]);
    fms_unload(fi->segs[FLITE_CRSI]);

    /* MemPtrFree(fi->samples); */

    MemPtrFree(fi->segs);
    MemPtrFree(fi);

    return;
}

unsigned long int flite_synth_text(flite_info *fi)
{
    void *flite_synth_text_arm;
    unsigned long int *ifi;
    unsigned long int result=0;
    int i;
    void *arg_1;

    /* Swap the data for the arm world */
    for (i=0; i<FLITE_NUM_BIG_SEGMENTS; i++)
	fi->segs[i] = (flite_mem_segment *)SWAPLONG(fi->segs[i]);
    ifi = (unsigned long int *)fi;
    for (i=1; i<(sizeof(flite_info)/4); i++)
	ifi[i] = SWAPLONG(ifi[i]);

    arg_1 = (void *)fi;

    flite_synth_text_arm = PealLookupSymbol(fi->m, "arm_flite_synth_text");
    result = (unsigned long int)PealCall(fi->m, flite_synth_text_arm, arg_1);

    /* Swap it back for the m68k world */
    for (i=1; i<(sizeof(flite_info)/4); i++)
	ifi[i] = SWAPLONG(ifi[i]);
    for (i=0; i<FLITE_NUM_BIG_SEGMENTS; i++)
	fi->segs[i] = (flite_mem_segment *)SWAPLONG(fi->segs[i]);

    return result;
}


