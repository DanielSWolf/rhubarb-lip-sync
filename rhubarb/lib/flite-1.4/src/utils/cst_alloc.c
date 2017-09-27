/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 1999                             */
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
/*               Date:  July 1999                                        */
/*************************************************************************/
/*                                                                       */
/*  Basic wraparounds for malloc and free                                */
/*                                                                       */
/*************************************************************************/
#include "cst_file.h"
#include "cst_alloc.h"
#include "cst_error.h"

#ifdef UNDER_CE
#include <windows.h>
#endif /* UNDER_CE */

/* define this if you want to trace memory usage */
/* #define CST_DEBUG_MALLOC */
/* #define CST_DEBUG_MALLOC_TRACE */
#ifdef CST_DEBUG_MALLOC
int cst_allocated = 0;
int cst_freed = 0;
int cst_alloc_max = 0;
int cst_alloc_imax = 0;
int cst_alloc_num_calls = 0;
int cst_alloc_out = 0;
#ifdef CST_DEBUG_MALLOC_TRACE
/* This is a crude memory tracer to find leaks */
int cst_alloc_ckpt = -1;
/* You have to know how big this should be to use it, its for debuging only */
#define NUM_CHUNKS 10000000
void *cst_alloc_cunks[NUM_CHUNKS];
#endif
#endif

void *cst_safe_alloc(int size)
{
    /* returns pointer to memory all set 0 */
    void *p = NULL;
    if (size < 0)
    {
	cst_errmsg("alloc: asked for negative size %d\n", size);
	cst_error();
    }
    else if (size == 0)  /* some mallocs return NULL for this */
	size++;

#ifdef CST_DEBUG_MALLOC
    if (size > cst_alloc_imax)
    {
	cst_alloc_imax = size;
    }
    cst_allocated += size;
    cst_alloc_out += size;
    size += 2 * sizeof(int);
#endif

#ifdef UNDER_CE
    p = (void *)LocalAlloc(LPTR, size);
#else
    p = (void *)calloc(size,1);
#endif

#ifdef CST_DEBUG_MALLOC
#ifdef CST_DEBUG_MALLOC_TRACE
    if (cst_alloc_num_calls == cst_alloc_ckpt)
	cst_dbgmsg("cst_malloc: %d\n", cst_alloc_ckpt);
    cst_alloc_cunks[cst_alloc_num_calls] = p;
#endif
    cst_alloc_num_calls++;
    *(int *)p = 1314;
    p = (int *)p + 1;
    *(int *)p = size - (2 * sizeof(int));
    if ((cst_allocated - cst_freed) > cst_alloc_max)
	cst_alloc_max = cst_allocated - cst_freed;
    p = (int *)p + 1;
#endif

    if (p == NULL)
    {
	cst_errmsg("alloc: can't alloc %d bytes\n", size);
	cst_error();
    }

    return p;
}

void *cst_safe_calloc(int size)
{
    return cst_safe_alloc(size);
}

void *cst_safe_realloc(void *p,int size)
{
    void *np=0;

#ifdef CST_DEBUG_MALLOC
    cst_free(p);
    return cst_safe_alloc(size);
#endif

    if (size == 0)
	size++;  /* as some mallocs do strange things with 0 */

    if (p == NULL)
	np = cst_safe_alloc(size);
    else
#ifdef UNDER_CE
	np = LocalReAlloc((HLOCAL)p, size, LMEM_MOVEABLE|LMEM_ZEROINIT);
#else
	np = realloc(p,size);
#endif

    if (np == NULL)
    {
	cst_errmsg("CST_REALLOC failed for %d bytes\n",size);
	cst_error();
    }

    return np;
}

void cst_free(void *p)
{
    if (p != NULL)
    {
#ifdef CST_DEBUG_MALLOC
	if (*((int *)p - 2) != 1314)
	{
	    cst_dbgmsg("CST_MALLOC_DEBUG freeing non-malloc memory\n");
	    return;
	}
	if (*((int *)p - 1) <= 0)
	{
	    cst_dbgmsg("CST_MALLOC_DEBUG re-freeing memory\n");
	    return;
	}
	cst_freed += *((int *)p - 1);
	cst_alloc_out -= *((int *)p - 1);
	*((int *)p - 1) = 0; /* mark it as freed */
	p = (int *)p - 2;
#endif
#ifndef CST_DEBUG_MALLOC_TRACE
#ifdef UNDER_CE
	if (LocalFree(p) != NULL)
	{
	    cst_errmsg("LocalFree(%p) failed with code %x\n",
		       p, GetLastError());
	    cst_error();
	}
#else
	free(p);
#endif
#endif
    }
}

#ifdef CST_DEBUG_MALLOC_TRACE

void cst_find_unfreed()
{
    int i, t;

    
    for (i = 0, t = 0;
	 i < NUM_CHUNKS
	     && i < cst_alloc_num_calls
	     && cst_alloc_cunks[i];
	 i++)
    {
	if (((int *)cst_alloc_cunks[i])[1] != 0) 
	{
            cst_dbgmsg("unfreed at %d\n", i);
	    t++;
	}
    }
    cst_dbgmsg("total unfreed %d\n", t);
}

void cst_alloc_debug_summary()
{
    cst_find_unfreed();
    printf("allocated %d freed %d max %d imax %d calls %d out %d\n",
	   cst_allocated, cst_freed, cst_alloc_max, 
	   cst_alloc_imax, cst_alloc_num_calls, cst_alloc_out);
}
#endif

#ifdef UNDER_CE
cst_alloc_context new_alloc_context(int size)
{
    HANDLE h;

    h = HeapCreate(0, size, 0);
    return (cst_alloc_context) h;
}

void delete_alloc_context(cst_alloc_context ctx)
{
    HANDLE h;

    h = (HANDLE)ctx;
    HeapDestroy(h);
}

void *cst_local_alloc(cst_alloc_context ctx, int size)
{
    HANDLE h;

    h = (HANDLE)ctx;
    if (h)
	return HeapAlloc(h, HEAP_ZERO_MEMORY, size);
    else
	return LocalAlloc(LPTR, size);
}

void cst_local_free(cst_alloc_context ctx, void *p)
{
    HANDLE h;

    h = (HANDLE)ctx;
    if (h)
	HeapFree(h, 0, p);
    else
	LocalFree(p);
}
#endif


