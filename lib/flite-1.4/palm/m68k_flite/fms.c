
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
/* flite_mem_segments                                                    */
/*                                                                       */
/* This provides a structure for holding very large data blocks          */
/* that need to be held in multiple resource segments as they are too    */
/* large.  These are read into a continguous block from Feature memory   */
/* It would be nice to just use them directly but that would require     */
/* paging access functions, and ultimately the voices will be on the     */
/* expansion card so will need to be copied anyway.                      */
/*                                                                       */
/* The reource chunks have info in the first resource which are created  */
/* by make_seg_ro                                                        */
/*                                                                       */
/*************************************************************************/

#include <PalmOS.h>
#include "palm_flite.h"

char fms_error[600];

static MemPtr fms_allocate(long int creator,long int id, long int size)
{
    MemPtr mem;
    Err err;

    err = FtrPtrNew(creator,id,size,&mem);
    if (err != 0)
	return NULL;
    err = FtrGet(creator,id,(long int *)&mem);
    if (err == 0)
	return mem;
    else
	return NULL;
}

static Err fms_free(long int creator,long int id)
{
    return FtrPtrFree(creator,id);
}

flite_mem_segment *fms_load(DmResType type, DmResID baseID)
{
    flite_mem_segment *fms;
    MemHandle rs;
    DmResID rid;
    unsigned char *rsh;
    short int i;
    unsigned long int nm;

    StrPrintF(fms_error,"fms ok");

    rs = DmGetResource(type, baseID);
    if (!rs) 
    {
	StrPrintF(fms_error,"fms not found");
	return 0;
    }

    rsh = (unsigned char *)MemHandleLock(rs);

    if (strcmp(rsh,"FLITE ") != 0)
    {   /* Its not one of mine */
	MemHandleUnlock(rs);
	DmReleaseResource(rs);
	StrPrintF(fms_error,"fms FLITE %c%c%c%c resource not found",
		  ((unsigned char *)&type)[0],
		  ((unsigned char *)&type)[1],
		  ((unsigned char *)&type)[2],
		  ((unsigned char *)&type)[3]);
	return 0;
    }

    fms = (flite_mem_segment *)MemPtrNew(sizeof(flite_mem_segment));
    fms->type = type;
    fms->id = StrAToI(&rsh[12]);
    fms->segment_size = StrAToI(&rsh[20]);
    fms->num_segments = StrAToI(&rsh[28]);
    fms->num_bytes = StrAToI(&rsh[36]);

    MemHandleUnlock(rs);
    DmReleaseResource(rs);

    fms->mem = fms_allocate(fms->type,fms->id,fms->num_bytes);
    if (!fms->mem)
    {
	StrPrintF(fms_error,"fms allocate failed");
	fms_unload(fms);
	return 0;
    }

    for (rid=baseID+1,i=0,nm=fms->num_bytes;
	 i<fms->num_segments; 
	 i++,rid++,nm-=fms->segment_size)
    {
	rs = DmGetResource(type, rid);
	if (rs)
	{
	    rsh = (unsigned char *)MemHandleLock(rs);
	    DmWrite(fms->mem,i*fms->segment_size,
		    rsh,
		    (nm > fms->segment_size) ? fms->segment_size : nm);
	    MemHandleUnlock(rs);
	    DmReleaseResource(rs);
	}
	else
	{
	    fms_unload(fms);
	    StrPrintF(fms_error,"fms failed at %d\n",rid);
	    return 0;
	}
    }

    fms->arm_mem = (unsigned char *)SWAPLONG(fms->mem);

    StrPrintF(fms_error,"fms FLITE %c%c%c%c resource ok",
	      ((unsigned char *)&type)[0],
	      ((unsigned char *)&type)[1],
	      ((unsigned char *)&type)[2],
	      ((unsigned char *)&type)[3]);
    return fms;
}

void fms_unload(flite_mem_segment *fms)
{

    if (fms)
    {
	if (fms->mem) fms_free(fms->type,fms->id);
	MemPtrFree(fms);
    }

    return;
}

