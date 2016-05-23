/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                         Copyright (c) 2008                            */
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
/*               Date:  May 2008                                         */
/*************************************************************************/
/*                                                                       */
/*  Removed the Cepstral copyrighted version (which was also free) and   */
/*  reverted to the flite-1.0-beta version, the other code had support   */
/*  for things not in flite and the original is sufficient               */
/*                                                                       */
/*************************************************************************/

#include "cst_math.h"
#include "cst_hrg.h"
#include "cst_wave.h"
#include "cst_sigpr.h"
#include "cst_sts.h"

CST_VAL_REGISTER_TYPE_NODEL(sts_list,cst_sts_list)

cst_sts_list *new_sts_list()
{
    cst_sts_list *l = cst_alloc(struct cst_sts_list_struct,1);
    return l;
}

void delete_sts_list(cst_sts_list *l)
{
    if (l)
    {
	/* sub data is always const so can't free it */
	cst_free(l);
    }
    return;
}

int get_unit_size(const cst_sts_list *s,int start, int end)
{
    /* returns size (in samples) of unit */
    int i,size;

    for (i=start,size=0; i<end; i++)
	size += get_frame_size(s, i);

    return size;
}

int get_frame_size(const cst_sts_list *sts_list, int frame)
{
    if (sts_list->sts) 
	return sts_list->sts[frame].size;
    else if (sts_list->sts_paged)
	return sts_list->sts_paged[frame].res_size;
    else 
    {
	/* This assumes that the voice compiler has generated an extra
           offset at the end of the array. */
	return sts_list->resoffs[frame+1] - sts_list->resoffs[frame];
    } 
}

const unsigned short * get_sts_frame(const cst_sts_list *sts_list, int frame)
{
    if (sts_list->sts)
        return sts_list->sts[frame].frame;
    else if (sts_list->sts_paged)
        return &sts_list->sts_paged[frame].frame_page[sts_list->num_channels * sts_list->sts_paged[frame].frame_offset];
    else
        return sts_list->frames + (frame * sts_list->num_channels);
}

const unsigned char * get_sts_residual(const cst_sts_list *sts_list, int frame)
{
    if (sts_list->sts)
        return sts_list->sts[frame].residual;
    else if (sts_list->sts_paged)
        return &sts_list->sts_paged[frame].res_page[sts_list->sts_paged[frame].res_offset];
    else 
        return sts_list->residuals + sts_list->resoffs[frame];
}

const unsigned char *get_sts_residual_fixed(const cst_sts_list *sts_list, int frame)
{
    /* Actually for mceps */
    if (sts_list->sts)
	return sts_list->sts[frame].residual;
    else if (sts_list->sts_paged)
        return &sts_list->sts_paged[frame].res_page[sts_list->sts_paged[frame].res_offset];
    else
	return 
            (const unsigned char *)sts_list->residuals
	    + (frame * sts_list->num_channels);

}

