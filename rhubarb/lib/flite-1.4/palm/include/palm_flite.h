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

#ifndef __PALM_FLITE_H__
#define __PALM_FLITE_H__

#ifdef m68k
/* m68k specific part */
#include <PalmOS.h>
#include <System/SystemMgr.h>
#include <System/FeatureMgr.h>
#include <System/PceNativeCall.h>
#include <PalmOSGlue.h>
#include "cst_endian.h"
#include "peal.h"
#include "elf.h"
/* end of m68k specific part */
#endif
#ifdef __ARM_ARCH_4T__
/* arm specific part */
#include "pealstub.h"
/* end of arm specific part */
#endif

/* For loading in other data segments */
extern char fms_error[600];

typedef struct flite_mem_segment_struct {
    unsigned long int type;  /* DmResType */
    unsigned char *mem;
    unsigned char *arm_mem;  /* byte swaped value of mem */
    unsigned long int id;
    unsigned long int segment_size;
    unsigned long int num_segments;
    unsigned long int num_bytes;
} flite_mem_segment;

#ifdef m68k
/* Can only create and ddestroy these in m68k domain */
flite_mem_segment *fms_load(DmResType type, DmResID baseID);
void fms_unload(flite_mem_segment *fms);
#endif

#define FliteOutputTypeWave   0
#define FliteOutputTypePhones 1
#define FliteOutputTypeWords  2
#define FliteOutputTypeStream 3

/* The five data segments loaded in separately */
#define FLITE_CLTS  0
#define FLITE_CLEX  1
#define FLITE_CLPC  2
#define FLITE_CRES  3
#define FLITE_CRSI  4
#define FLITE_NUM_BIG_SEGMENTS 5

/* The main structure that is passed bewteen the domains */
/* Everything should be 4 bytes to make swaping easier   */
typedef struct flite_info_struct {
    void *m; /* Peal ARM code module */

    /* Input information */
    unsigned long int type;
    char *text;
    unsigned long int start;  /* start position in text */
    unsigned long int utt_length;  /* length chars of most recently synth'd utt */
    
    flite_mem_segment **segs;

    /* Output Information */
    unsigned long int num_samples;
    unsigned long int sample_rate;
    short int *samples;
    unsigned long int PlayPosition;
    unsigned long int WavePosition;

    char *output;
    unsigned long int max_output;

} flite_info;

#ifdef m68k
/* Can only interface to these in m68k domain */
flite_info *flite_init(void);
void flite_end(flite_info *fi);
unsigned long int flite_synth_text(flite_info *fi);
#endif

#endif
