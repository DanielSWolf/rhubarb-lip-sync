/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                         Copyright (c) 2000                            */
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
/*               Date:  August 2000                                      */
/*************************************************************************/
/*                                                                       */
/*  Waveforms                                                            */
/*                                                                       */
/*************************************************************************/
#include "cst_string.h"
#include "cst_wave.h"
#include "cst_file.h"

void cst_wave_resample(cst_wave *w, int sample_rate)
{
    /* This is here so that it won't necessarily be linked in tight-space */
    /* platforms like PalmOS                                              */

    cst_rateconv *filt;
    int up, down;
    short *in;
    const short *inptr;
    short *outptr;
    int n, insize, outsize;

    /* Sure, we could take the GCD.  In practice, though, this
       gives us what we want and makes things go faster. */
    down = w->sample_rate / 1000;
    up = sample_rate / 1000;

    if (up < 1 || down < 1) {
	cst_errmsg("cst_wave_resample: invalid input/output sample rates (%d, %d)\n",
		   up * 1000, down * 1000);
	cst_error();
    }

    filt = new_rateconv(up, down, w->num_channels);

    inptr = in = w->samples;
    insize = w->num_samples;

    w->num_samples = w->num_samples * up / down + 2048;
    w->samples = cst_alloc(short, w->num_samples * w->num_channels);
    w->sample_rate = sample_rate;

    outptr = w->samples;
    outsize = w->num_samples;

    while ((n = cst_rateconv_in(filt, inptr, insize)) > 0) {
	inptr += n;
	insize -= n;

	while ((n = cst_rateconv_out(filt, outptr, outsize)) > 0) {
	    outptr += n;
	    outsize -= n;
	}
    }
    cst_rateconv_leadout(filt);
    while ((n = cst_rateconv_out(filt, outptr, outsize)) > 0) {
	outptr += n;
	outsize -= n;
    }

    cst_free(in);
    delete_rateconv(filt);

}

int cst_wave_save(cst_wave *w,const char *filename,const char *type)
{
    if (cst_streq(type,"riff"))
	return cst_wave_save_riff(w,filename);
/*    else if (cst_streq(type,"aiff"))
	return cst_wave_save_aiff(w,filename);
    else if (cst_streq(type,"snd"))
    return cst_wave_save_snd(w,filename); */
    else if (cst_streq(type,"raw"))
	return cst_wave_save_raw(w,filename);
    else
    {
	cst_errmsg("cst_wave_save: unsupported wavetype \"%s\"\n",
		   type);
	return -1;
    }
}

int cst_wave_save_raw(cst_wave *w, const char *filename)
{
    cst_file fd;
    int rv;

    if ((fd = cst_fopen(filename,CST_OPEN_WRITE|CST_OPEN_BINARY)) == NULL)
    {
	cst_errmsg("cst_wave_save: can't open file \"%s\"\n",
		   filename);
	return -1;
    }

    rv = cst_wave_save_raw_fd(w, fd);
    cst_fclose(fd);

    return rv;
}

int cst_wave_save_raw_fd(cst_wave *w, cst_file fd)
{
    if (cst_fwrite(fd, cst_wave_samples(w),
		   sizeof(short), cst_wave_num_samples(w)) == cst_wave_num_samples(w))
	return 0;
    else
	return -1;
}


int cst_wave_append_riff(cst_wave *w,const char *filename)
{
    /* Appends to wave in file if it already exists */
    cst_file fd;
    cst_wave_header hdr;
    char info[4];
    int d_int;
    int rv, num_bytes, n, sample_rate;

    if ((fd = cst_fopen(filename,CST_OPEN_WRITE|CST_OPEN_READ|CST_OPEN_BINARY)) == NULL)
    {
	cst_errmsg("cst_wave_append: can't open file \"%s\"\n",
		   filename);
	return -1;
    }
    
    rv = cst_wave_load_riff_header(&hdr,fd);
    if (rv != CST_OK_FORMAT)
    {
	cst_fclose(fd);
	return rv;
    }
    /* We will assume this is one of my riff files so it has *ONE* data part */
    cst_fread(fd,info,1,4);
    cst_fread(fd,&d_int,4,1);
    if (CST_BIG_ENDIAN) d_int = SWAPINT(d_int);
    hdr.num_samples = d_int/sizeof(short);

    cst_fseek(fd,
	      cst_ftell(fd)+(hdr.hsize-16)+
	      (hdr.num_samples*hdr.num_channels*sizeof(short)),
	      CST_SEEK_ABSOLUTE);

    if (CST_BIG_ENDIAN)
    {
	short *xdata = cst_alloc(short,cst_wave_num_channels(w)*
				 cst_wave_num_samples(w));
	memmove(xdata,cst_wave_samples(w),
		sizeof(short)*cst_wave_num_channels(w)*
		cst_wave_num_samples(w));
	swap_bytes_short(xdata,
			 cst_wave_num_channels(w)*
			 cst_wave_num_samples(w));
	n = cst_fwrite(fd,xdata,sizeof(short),
		       cst_wave_num_channels(w)*cst_wave_num_samples(w));
	cst_free(xdata);
    }
    else
    {
	n = cst_fwrite(fd,cst_wave_samples(w),sizeof(short),
		       cst_wave_num_channels(w)*cst_wave_num_samples(w));
    }

    cst_fseek(fd,4,CST_SEEK_ABSOLUTE);
    num_bytes = hdr.num_bytes + (n*sizeof(short));
    if (CST_BIG_ENDIAN) num_bytes = SWAPINT(num_bytes);
    cst_fwrite(fd,&num_bytes,4,1); /* num bytes in whole file */
    cst_fseek(fd,4+4+4+4 +4+2+2 ,CST_SEEK_ABSOLUTE);
    sample_rate = w->sample_rate;
    if (CST_BIG_ENDIAN) sample_rate = SWAPINT(sample_rate);
    cst_fwrite(fd,&sample_rate,4,1); /* sample rate */
    cst_fseek(fd,4+4+4+4+4+2+2+4+4+2+2+4,CST_SEEK_ABSOLUTE); 
    num_bytes = 
	(sizeof(short) * cst_wave_num_channels(w) * cst_wave_num_samples(w)) +
	(sizeof(short) * hdr.num_channels * hdr.num_samples);
    if (CST_BIG_ENDIAN) num_bytes = SWAPINT(num_bytes);
    cst_fwrite(fd,&num_bytes,4,1); /* num bytes in data */
    cst_fclose(fd);

    return rv;
}

int cst_wave_save_riff(cst_wave *w,const char *filename)
{
    cst_file fd;
    int rv;

    if ((fd = cst_fopen(filename,CST_OPEN_WRITE|CST_OPEN_BINARY)) == NULL)
    {
	cst_errmsg("cst_wave_save: can't open file \"%s\"\n",
		   filename);
	return -1;
    }

    rv = cst_wave_save_riff_fd(w, fd);
    cst_fclose(fd);

    return rv;
}

int cst_wave_save_riff_fd(cst_wave *w, cst_file fd)
{
    char *info;
    short d_short;
    int d_int, n;
    int num_bytes;

    info = "RIFF";
    cst_fwrite(fd,info,4,1);
    num_bytes = (cst_wave_num_samples(w)
		 * cst_wave_num_channels(w)
		 * sizeof(short)) + 8 + 16 + 12;
    if (CST_BIG_ENDIAN) num_bytes = SWAPINT(num_bytes);
    cst_fwrite(fd,&num_bytes,4,1); /* num bytes in whole file */
    info = "WAVE";
    cst_fwrite(fd,info,1,4);
    info = "fmt ";
    cst_fwrite(fd,info,1,4);
    num_bytes = 16;                   /* size of header */
    if (CST_BIG_ENDIAN) num_bytes = SWAPINT(num_bytes);
    cst_fwrite(fd,&num_bytes,4,1);        
    d_short = RIFF_FORMAT_PCM;        /* sample type */
    if (CST_BIG_ENDIAN) d_short = SWAPSHORT(d_short);
    cst_fwrite(fd,&d_short,2,1);          
    d_short = cst_wave_num_channels(w); /* number of channels */
    if (CST_BIG_ENDIAN) d_short = SWAPSHORT(d_short);
    cst_fwrite(fd,&d_short,2,1);          
    d_int = cst_wave_sample_rate(w);  /* sample rate */
    if (CST_BIG_ENDIAN) d_int = SWAPINT(d_int);
    cst_fwrite(fd,&d_int,4,1);  
    d_int = (cst_wave_sample_rate(w)
	     * cst_wave_num_channels(w)
	     * sizeof(short));        /* average bytes per second */
    if (CST_BIG_ENDIAN) d_int = SWAPINT(d_int);
    cst_fwrite(fd,&d_int,4,1);
    d_short = (cst_wave_num_channels(w)
	       * sizeof(short));      /* block align */
    if (CST_BIG_ENDIAN) d_short = SWAPSHORT(d_short);
    cst_fwrite(fd,&d_short,2,1);          
    d_short = 2 * 8;                  /* bits per sample */
    if (CST_BIG_ENDIAN) d_short = SWAPSHORT(d_short);
    cst_fwrite(fd,&d_short,2,1);          
    info = "data";
    cst_fwrite(fd,info,1,4);
    d_int = (cst_wave_num_channels(w)
	     * cst_wave_num_samples(w)
	     * sizeof(short));	      /* bytes in data */
    if (CST_BIG_ENDIAN) d_int = SWAPINT(d_int);
    cst_fwrite(fd,&d_int,4,1);  

    if (CST_BIG_ENDIAN)
    {
        short *xdata = cst_alloc(short,cst_wave_num_channels(w)*
				 cst_wave_num_samples(w));
	memmove(xdata,cst_wave_samples(w),
		sizeof(short)*cst_wave_num_channels(w)*
		cst_wave_num_samples(w));
	swap_bytes_short(xdata,
			 cst_wave_num_channels(w)*
			 cst_wave_num_samples(w));
	n = cst_fwrite(fd,xdata,sizeof(short),
		       cst_wave_num_channels(w)*cst_wave_num_samples(w));
	cst_free(xdata);
    }
    else
    {
	n = cst_fwrite(fd,cst_wave_samples(w),sizeof(short),
		       cst_wave_num_channels(w)*cst_wave_num_samples(w));
    }

    if (n != cst_wave_num_channels(w)*cst_wave_num_samples(w))
	return -1;
    else
	return 0;
	
}

int cst_wave_load_raw(cst_wave *w,const char *filename,
		      const char *bo, int sample_rate)
{
    cst_file fd;
    int r;

    if ((fd = cst_fopen(filename,CST_OPEN_READ|CST_OPEN_BINARY)) == NULL)
    {
	cst_errmsg("cst_wave_load: can't open file \"%s\"\n",
		   filename);
	return -1;
    }

    r = cst_wave_load_raw_fd(w, fd, bo, sample_rate);
    
    cst_fclose(fd);
    
    return r;
}

int cst_wave_load_raw_fd(cst_wave *w, cst_file fd,
			 const char *bo, int sample_rate)
{
    long size;

    /* Won't work on pipes, tough luck... */
    size = cst_filesize(fd) / sizeof(short);
    cst_wave_resize(w, size, 1);
    if (cst_fread(fd, w->samples, sizeof(short), size) != size)
	return -1;

    w->sample_rate = sample_rate;
    if (bo) /* if it's NULL we don't care */
	if ((CST_LITTLE_ENDIAN && cst_streq(bo, BYTE_ORDER_BIG))
	    || (CST_BIG_ENDIAN && cst_streq(bo, BYTE_ORDER_LITTLE)))
	    swap_bytes_short(w->samples,w->num_samples);

    return 0;
}

int cst_wave_load_riff(cst_wave *w,const char *filename)
{
    cst_file fd;
    int r;

    if ((fd = cst_fopen(filename,CST_OPEN_READ|CST_OPEN_BINARY)) == NULL)
    {
	cst_errmsg("cst_wave_load: can't open file \"%s\"\n",
		   filename);
	return -1;
    }

    r = cst_wave_load_riff_fd(w,fd);
    
    cst_fclose(fd);
    
    return r;
}

int cst_wave_load_riff_header(cst_wave_header *header,cst_file fd)
{
    char info[4];
    short d_short;
    int d_int;

    if (cst_fread(fd,info,1,4) != 4)
	return CST_WRONG_FORMAT;
    else if (strncmp(info,"RIFF",4) != 0)
	return CST_WRONG_FORMAT;
	
    cst_fread(fd,&d_int,4,1);
    if (CST_BIG_ENDIAN) d_int = SWAPINT(d_int);
    header->num_bytes = d_int;
    
    if ((cst_fread(fd,info,1,4) != 4) ||
	(strncmp(info,"WAVE",4) != 0))
	return CST_ERROR_FORMAT;

    if ((cst_fread(fd,info,1,4) != 4) ||
	(strncmp(info,"fmt ",4) != 0))
	return CST_ERROR_FORMAT;

    cst_fread(fd,&d_int,4,1);
    if (CST_BIG_ENDIAN) d_int = SWAPINT(d_int);
    header->hsize = d_int;
    cst_fread(fd,&d_short,2,1);
    if (CST_BIG_ENDIAN) d_short = SWAPSHORT(d_short);

    if (d_short != RIFF_FORMAT_PCM)
    {
	cst_errmsg("cst_load_wave_riff: unsupported sample format\n");
	return CST_ERROR_FORMAT;
    }
    cst_fread(fd,&d_short,2,1);
    if (CST_BIG_ENDIAN) d_short = SWAPSHORT(d_short);

    header->num_channels = d_short;

    cst_fread(fd,&d_int,4,1);
    if (CST_BIG_ENDIAN) d_int = SWAPINT(d_int);
    header->sample_rate = d_int;
    cst_fread(fd,&d_int,4,1);              /* avg bytes per second */
    cst_fread(fd,&d_short,2,1);            /* block align */
    cst_fread(fd,&d_short,2,1);            /* bits per sample */

    return CST_OK_FORMAT;
}

int cst_wave_load_riff_fd(cst_wave *w,cst_file fd)
{
    cst_wave_header hdr;
    int rv;
    char info[4];
    int d_int, d;
    int data_length;
    int samples;

    rv = cst_wave_load_riff_header(&hdr,fd);
    if (rv != CST_OK_FORMAT)
	return rv;
	
    cst_fseek(fd,cst_ftell(fd)+(hdr.hsize-16),CST_SEEK_ABSOLUTE); /* skip rest of header */

    /* Note there's a bunch of potential random headers */
    while (1)
    {
	if (cst_fread(fd,info,1,4) != 4)
	    return CST_ERROR_FORMAT;
	if (strncmp(info,"data",4) == 0)
	{
	    cst_fread(fd,&d_int,4,1);
	    if (CST_BIG_ENDIAN) d_int = SWAPINT(d_int);
	    samples = d_int/sizeof(short);
	    break;
	}
	else if (strncmp(info,"fact",4) == 0)
	{   
	    cst_fread(fd,&d_int,4,1);
	    if (CST_BIG_ENDIAN) d_int = SWAPINT(d_int);
	    cst_fseek(fd,cst_ftell(fd)+d_int,CST_SEEK_ABSOLUTE);
	}
	else if (strncmp(info,"clm ",4) == 0)
	{   /* another random chunk type -- resample puts this one in */
	    cst_fread(fd,&d_int,4,1);
	    if (CST_BIG_ENDIAN) d_int = SWAPINT(d_int);
	    cst_fseek(fd,cst_ftell(fd)+d_int,CST_SEEK_ABSOLUTE);
	}
	else 
	{
	    cst_errmsg("cst_wave_load_riff: unsupported chunk type \"%*s\"\n",
		       4,info);
	    return CST_ERROR_FORMAT;
	}
    }

    /* Now read the data itself */
    cst_wave_set_sample_rate(w,hdr.sample_rate);     /* sample rate */
    data_length = samples;
    cst_wave_resize(w,samples/hdr.num_channels,hdr.num_channels);

    if ((d = cst_fread(fd,w->samples,sizeof(short),data_length)) != data_length)
    {
	cst_errmsg("cst_wave_load_riff: %d missing samples, resized accordingly\n",
		   data_length-d);
	w->num_samples = d;
    }

    if (CST_BIG_ENDIAN)
	swap_bytes_short(w->samples,w->num_samples);

    return CST_OK_FORMAT;
}
