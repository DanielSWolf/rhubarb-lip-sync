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
/*               Date:  October 2001                                     */
/*************************************************************************/
/*                                                                       */
/* cst_mmap_win32.c: memory-mapped file I/O support for Win32 systems    */
/*                                                                       */
/*************************************************************************/

#include <windows.h>

#ifndef MEM_TOP_DOWN
#define MEM_TOP_DOWN 0
#endif

#include "cst_file.h"
#include "cst_error.h"
#include "cst_alloc.h"

cst_filemap *cst_mmap_file(const char *path)
{
	HANDLE ffm;
	cst_filemap *fmap = NULL;

	ffm = CreateFile(path,GENERIC_READ,FILE_SHARE_READ,NULL,
			 OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (ffm == INVALID_HANDLE_VALUE) {
		return NULL;
	} else {
		fmap = cst_alloc(cst_filemap,1);
		fmap->h = CreateFileMapping(ffm,NULL,PAGE_READONLY,0,0,NULL);
		fmap->mapsize = GetFileSize(fmap->h, NULL);
		fmap->mem = MapViewOfFile(fmap->h,FILE_MAP_READ,0,0,0);
		if (fmap->h == NULL || fmap->mem == NULL) {
			CloseHandle(ffm);
			cst_free(fmap);
			return NULL;
		}
	}

	return fmap;
}

int cst_munmap_file(cst_filemap *fmap)
{
	UnmapViewOfFile(fmap->mem);
	CloseHandle(fmap->h);
	cst_free(fmap);
	return 0;
}

cst_filemap *cst_read_whole_file(const char *path)
{
    cst_filemap *fmap;
    cst_file fh;

    if ((fh = cst_fopen(path, CST_OPEN_READ)) == NULL) {
	cst_errmsg("cst_read_whole_file: Failed to open file\n");
	return NULL;
    }

    fmap = cst_alloc(cst_filemap, 1);
    fmap->fh = fh;
    cst_fseek(fmap->fh, 0, CST_SEEK_ENDREL);
    fmap->mapsize = cst_ftell(fmap->fh);
    fmap->mem = VirtualAlloc(NULL, fmap->mapsize, MEM_COMMIT|MEM_TOP_DOWN,
			     PAGE_READWRITE);
    cst_fseek(fmap->fh, 0, CST_SEEK_ABSOLUTE);
    cst_fread(fmap->fh, fmap->mem, 1, fmap->mapsize);

    return fmap;
}

int cst_free_whole_file(cst_filemap *fmap)
{
    if (cst_fclose(fmap->fh) < 0) {
	cst_errmsg("cst_read_whole_file: cst_fclose() failed\n");
	return -1;
    }
    VirtualFree(fmap->mem, fmap->mapsize, MEM_DECOMMIT);
    cst_free(fmap);
    return 0;
}

cst_filemap *cst_read_part_file(const char *path)
{
    cst_filemap *fmap;
    cst_file fh;

    if ((fh = cst_fopen(path, CST_OPEN_READ)) == NULL) {
	cst_errmsg("cst_read_part_file: Failed to open file\n");
	return NULL;
    }

    fmap = cst_alloc(cst_filemap, 1);
    fmap->fh = fh;

    return fmap;
}

int cst_free_part_file(cst_filemap *fmap)
{
    if (cst_fclose(fmap->fh) < 0) {
	cst_errmsg("cst_read_part_file: cst_fclose() failed\n");
	return -1;
    }
    cst_free(fmap);
    return 0;
}
