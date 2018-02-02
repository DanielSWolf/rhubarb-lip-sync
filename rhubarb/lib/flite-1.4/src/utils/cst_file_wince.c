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
/*             Author:  David Huggins-Daines <dhd@cepstral.com>          */
/*               Date:  August 2001                                      */
/*************************************************************************/
/*                                                                       */
/*  File I/O wrappers for defective platforms.                           */
/*                                                                       */
/*  fread and fwrite corrected to return count not numbytes              */
/*       awb@cs.cmu.edu 20090124                                         */
/*                                                                       */
/*************************************************************************/

#include <windows.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "cst_file.h"
#include "cst_alloc.h"
#include "cst_error.h"

cst_file cst_fopen(const char *path, int mode)
{
	size_t count = mbstowcs(NULL,path,0)+1;
	wchar_t *wpath = cst_alloc(wchar_t,count);
	cst_file fh;
	long access = GENERIC_READ|GENERIC_WRITE;
        long creation = OPEN_ALWAYS;

	if (((mode & CST_OPEN_READ) && (mode & CST_OPEN_WRITE))
		|| ((mode & CST_OPEN_READ) && (mode & CST_OPEN_APPEND)))
	{
		access = GENERIC_READ|GENERIC_WRITE;
		creation = OPEN_ALWAYS;
	}
	else if (mode & CST_OPEN_READ)
	{
		access = GENERIC_READ;
		creation = OPEN_EXISTING;
	}
	else if (mode & CST_OPEN_WRITE)
	{
		access = GENERIC_WRITE;
		creation = CREATE_ALWAYS; /* FIXME: Does this truncate?  Argh! */
	}
	else if (mode & CST_OPEN_APPEND)
	{
		access = GENERIC_WRITE;
		creation = OPEN_ALWAYS;
	}

	/* Note, we are ignoring CST_FILE_BINARY entirely since there is no
	   CRLF translation done by these APIs (we hope).  This might cause
	   problems for other Windows programs that try to read our output. */
	mbstowcs(wpath,path,count);
	fh = CreateFile(wpath,access,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,
		creation,FILE_ATTRIBUTE_NORMAL,NULL);
	cst_free(wpath);
	if (fh == INVALID_HANDLE_VALUE) {
		long foo = GetLastError();
                (void)foo;
		return NULL;
	}

	if (mode & CST_OPEN_APPEND)
		SetFilePointer(fh,0,NULL,FILE_END);

	return fh;
}

long cst_fwrite(cst_file fh, const void *buf, long size, long count)
{
    unsigned long rv;

	if (!WriteFile(fh,buf,size*count,&rv,NULL))
		return -1;

	return rv/size;
}

long cst_fread(cst_file fh, void *buf, long size, long count)
{
    unsigned long rv;

	if (!ReadFile(fh,buf,size*count,&rv,NULL))
		return -1;
	return rv/size;
}

int cst_fgetc(cst_file fh)
{
	char c;
	if (cst_fread(fh, &c, 1, 1) == 0)
		return EOF;
	return c;
}

long cst_filesize(cst_file fh)
{
	return GetFileSize(fh, NULL);
}

long cst_ftell(cst_file fh)
{
	return SetFilePointer(fh,0,NULL,FILE_CURRENT);
}

long cst_fseek(cst_file fh, long pos, int whence)
{
	int w = 0;

	if (whence == CST_SEEK_ABSOLUTE)
		w = FILE_BEGIN;
	else if (whence == CST_SEEK_RELATIVE)
		w = FILE_CURRENT;
	else if (whence == CST_SEEK_ENDREL)
		w = FILE_END;

	return SetFilePointer(fh,pos,NULL,w);
}

int cst_fprintf(cst_file fh, char *fmt, ...)
{
	va_list args;
	char outbuf[512];
	int count;

	va_start(args,fmt);
	count = vsprintf(outbuf,fmt,args); /* You use WinCE, you lose. */
	va_end(args);
	return cst_fwrite(fh,outbuf,1,count);
}

int cst_sprintf(char *outbuf, const char *fmt, ...)
{
	va_list args;
	int count;

	va_start(args,fmt);
	count = vsprintf(outbuf,fmt,args); /* You use WinCE, you lose. */
	va_end(args);
	return count;
}

int cst_fclose(cst_file fh)
{
	return CloseHandle(fh);
}

cst_filemap *cst_mmap_file(const char *path)
{
	HANDLE ffm;
	cst_filemap *fmap = NULL;
	size_t count = mbstowcs(NULL,path,0) + 1;
	wchar_t *wpath = cst_alloc(wchar_t,count);

	mbstowcs(wpath,path,count);

	ffm = CreateFileForMapping(wpath,GENERIC_READ,FILE_SHARE_READ,NULL,
				   OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	cst_free(wpath);
	if (ffm == INVALID_HANDLE_VALUE) {
		return NULL;
	} else {
		fmap = cst_alloc(cst_filemap,1);
		fmap->fh = CreateFileMapping(ffm,NULL,PAGE_READONLY,0,0,NULL);
		fmap->mapsize = GetFileSize(fmap->fh, NULL);
		fmap->mem = MapViewOfFile(fmap->fh,FILE_MAP_READ,0,0,0);
		if (fmap->fh == NULL || fmap->mem == NULL) {
			cst_free(fmap);
			return NULL;
		}
	}

	return fmap;
}

int cst_munmap_file(cst_filemap *fmap)
{
	UnmapViewOfFile(fmap->mem);
	CloseHandle(fmap->fh);
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
    fmap->mapsize = GetFileSize(fmap->fh, NULL);
    fmap->mem = VirtualAlloc(NULL, fmap->mapsize, MEM_COMMIT|MEM_TOP_DOWN,
			     PAGE_READWRITE);
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
