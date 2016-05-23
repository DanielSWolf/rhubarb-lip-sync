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
/* cst_mmap_posix.c: memory-mapped file I/O support for POSIX systems    */
/*                                                                       */
/*************************************************************************/

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "cst_file.h"
#include "cst_error.h"
#include "cst_alloc.h"

#ifdef __QNXNTO__
#include <sys/syspage.h>
#define getpagesize() (SYSPAGE_ENTRY( system_private )->pagesize)
#else
#define MAP_NOSYNCFILE 0
#endif

cst_filemap *cst_mmap_file(const char *path)
{
    cst_filemap *fmap = NULL;
    size_t pgsize;
    struct stat buf;
    int fd;

    pgsize = getpagesize();

    if ((fd = open(path, O_RDONLY)) < 0) {
	perror("cst_mmap_file: Failed to open file");
	return NULL;
    }
    if ((fstat(fd, &buf)) < 0) {
	perror("cst_mmap_file: fstat() failed");
	return NULL;
    }
    fmap = cst_alloc(cst_filemap, 1);
    fmap->fd = fd;
    fmap->mapsize = (buf.st_size + pgsize - 1) / pgsize * pgsize;
    if ((fmap->mem = mmap(0, fmap->mapsize, PROT_READ,
			  MAP_SHARED | MAP_NOSYNCFILE, fd, 0)) == (caddr_t)-1) {
	perror("cst_mmap_file: mmap() failed");
	cst_free(fmap);
	return NULL;
    }

    return fmap;
}

int cst_munmap_file(cst_filemap *fmap)
{
    if (munmap(fmap->mem, fmap->mapsize) < 0) {
	perror("cst_munmap_file: munmap() failed");
	return -1;
    }
    if (close(fmap->fd) < 0) {
	perror("cst_munmap_file: close() failed");
	return -1;
    }
    cst_free(fmap);
    return 0;
}

cst_filemap *cst_read_whole_file(const char *path)
{
    cst_filemap *fmap;
    struct stat buf;
    int fd;

    if ((fd = open(path, O_RDONLY)) < 0) {
	perror("cst_read_whole_file: Failed to open file");
	return NULL;
    }
    if ((fstat(fd, &buf)) < 0) {
	perror("cst_read_whole_file: fstat() failed");
	return NULL;
    }

    fmap = cst_alloc(cst_filemap, 1);
    fmap->fd = fd;
    fmap->mapsize = buf.st_size;
    fmap->mem = cst_alloc(char, fmap->mapsize);
    if (read(fmap->fd, fmap->mem, fmap->mapsize) < fmap->mapsize)
    {
	perror("cst_read_whole_fiel: read() failed");
	close(fmap->fd);
	cst_free(fmap->mem);
	cst_free(fmap);
	return NULL;
    }

    return fmap;
}

int cst_free_whole_file(cst_filemap *fmap)
{
    if (close(fmap->fd) < 0) {
	perror("cst_free_whole_file: close() failed");
	return -1;
    }
    cst_free(fmap->mem);
    cst_free(fmap);
    return 0;
}

cst_filemap *cst_read_part_file(const char *path)
{
    cst_filemap *fmap;
    struct stat buf;
    cst_file fh;

    if ((fh = cst_fopen(path, CST_OPEN_READ)) == NULL) {
	perror("cst_read_part_file: Failed to open file");
	return NULL;
    }
    if ((fstat(fileno(fh), &buf)) < 0) {
	perror("cst_read_part_file: fstat() failed");
	return NULL;
    }

    fmap = cst_alloc(cst_filemap, 1);
    fmap->fh = fh;
    fmap->mapsize = buf.st_size;

    return fmap;
}

int cst_free_part_file(cst_filemap *fmap)
{
    if (cst_fclose(fmap->fh) < 0) {
	perror("cst_munmap_file: cst_fclose() failed");
	return -1;
    }
    cst_free(fmap);
    return 0;
}
