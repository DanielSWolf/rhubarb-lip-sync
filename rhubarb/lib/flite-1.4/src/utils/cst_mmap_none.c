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
/* cst_mmap_none.c: stubs for systems with no memory-mapped I/O support. */
/*                                                                       */
/*************************************************************************/

#include "cst_file.h"
#include "cst_error.h"
#include "cst_alloc.h"

cst_filemap *cst_mmap_file(const char *path)
{
    cst_dbgmsg("cst_mmap_file: unsupported on this platform");
    cst_error();

    return NULL;
}

int cst_munmap_file(cst_filemap *fmap)
{
    cst_dbgmsg("cst_munmap_file: unsupported on this platform");
    cst_error();

    return -1;
}

cst_filemap *cst_read_whole_file(const char *path)
{
    cst_filemap *fmap;
    cst_file fh;

    if ((fh = cst_fopen(path, CST_OPEN_READ)) < 0) {
	cst_errmsg("cst_read_whole_file: Failed to open file\n");
	return NULL;
    }

    fmap = cst_alloc(cst_filemap, 1);
    fmap->fh = fh;
    fmap->mapsize = cst_filesize(fmap->fh);
    fmap->mem = cst_alloc(char, fmap->mapsize);
    if (cst_fread(fmap->fh, fmap->mem, 1, fmap->mapsize) < fmap->mapsize)
    {
	cst_errmsg("cst_read_whole_file: read() failed\n");
	cst_fclose(fmap->fh);
	cst_free(fmap->mem);
	cst_free(fmap);
	return NULL;
    }

    return fmap;
}

int cst_free_whole_file(cst_filemap *fmap)
{
    if (cst_fclose(fmap->fh) < 0) {
	cst_errmsg("cst_free_whole_file: close() failed\n");
	return -1;
    }
    cst_free(fmap->mem);
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
    fmap->mapsize = cst_filesize(fmap->fh);

    return fmap;
}

int cst_free_part_file(cst_filemap *fmap)
{
    if (cst_fclose(fmap->fh) < 0) {
	cst_errmsg("cst_munmap_file: cst_fclose() failed\n");
	return -1;
    }
    cst_free(fmap);
    return 0;
}
