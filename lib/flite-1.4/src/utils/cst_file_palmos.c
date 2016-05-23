/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
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
/*             Author:  Alan W Black <awb@cs.cmu.edu>                    */
/*               Date:  July 2004                                        */
/*************************************************************************/
/*                                                                       */
/*  File I/O wrappers for PalmOS platforms.                              */
/*                                                                       */
/*  flop/armflite currently (Jan 05) only uses cst_sprintf               */
/*                                                                       */
/*************************************************************************/

#include "cst_file.h"
#include "cst_error.h"
#include "cst_alloc.h"

#define	 _STDIO_PALM_C_
#include <PalmOS.h>
#include <StdIOPalm.h>
#include <System/FileStream.h>

#include <string.h>
#include <stdarg.h>

cst_file cst_fopen(const char *path, int mode)
{
    /* This is kind of hacky. */
    if ((mode & CST_OPEN_WRITE) && (mode & CST_OPEN_READ))
	    return FileOpen(0, path, 0, 0, fileModeReadWrite, NULL);
    else if ((mode & CST_OPEN_APPEND) && (mode & CST_OPEN_READ))
	    return FileOpen(0, path, 0, 0, 
			    fileModeAppend | fileModeReadWrite, NULL);
    else if (mode & CST_OPEN_WRITE)
	    return FileOpen(0, path, 0, 0, fileModeReadWrite, NULL);
    else if (mode & CST_OPEN_APPEND)
	    return FileOpen(0, path, 0, 0, fileModeAppend, NULL);
    else if (mode & CST_OPEN_READ)
	    return FileOpen(0, path, 0, 0, fileModeReadOnly, NULL);
    else
	    return FileOpen(0, path, 0, 0, fileModeReadOnly, NULL);
}

long cst_fwrite(cst_file fh, const void *buf, long size, long count)
{
    return FileWrite(fh, buf, size, count, NULL);
}

long cst_fread(cst_file fh, void *buf, long size, long count)
{
    return FileRead(fh,buf,size,count,NULL);
}

long cst_filesize(cst_file fh)
{
    long epos=0;

    FileTell(fh,&epos,NULL);

    return epos;
}

int cst_fgetc(cst_file fh)
{
    return fgetc(fh);
}

long cst_ftell(cst_file fh)
{
    return FileTell(fh,NULL,NULL);
}

long cst_fseek(cst_file fh, long pos, int whence)
{
    FileOriginEnum w = fileOriginBeginning;

    if (whence == CST_SEEK_ABSOLUTE)
	w = fileOriginBeginning;
    else if (whence == CST_SEEK_RELATIVE)
	w = fileOriginCurrent;
    else if (whence == CST_SEEK_ENDREL)
	w = fileOriginEnd;

    return FileSeek(fh, pos, w);
}

int cst_fclose(cst_file fh)
{
    return FileClose(fh);
}

static int po_itoa(char *s, int i, const char* modifier)
{    /* print ascii representation of i to s with any modifier (e.g. "03") */
    char n[20];  /* numbers cannot be bigger than 20 places */
    int ni, si;
    int q;

    /* modifier currently ignored */

    /* construct the ascii backwards */
    if (i == 0)
    {
	n[0] = '0';
	ni=1;
    }
    else
	for (ni=0,q=(i < 0 ? -i : i); q > 0; q/=10,ni++)
	    n[ni] = q%10 + '0';
    if (i < 0)
    {
	n[ni] = '-';
	ni++;
    }

    /* pop it off and onto the string */
    for (si=0; ni>0; si++,ni--)
	s[si] = n[ni-1];

    return si;
}

static int po_ftoa(char *s, float f, const char* modifier)
{
    char n[20];  /* numbers cannot be bigger than 30 places */
    char p[20];
    int ni, si, pi;
    int q,i,d;
    float pp;

    /* modifier currently ignored */

    /* construct the ascii backwards */
    if (f == 0.0)
    {
	n[0] = '0';
	n[1] = '.';
	n[2] = '0';
	ni=3;
	p[0] = '\0';
    }
    else
    {
	/* above zero part */
	i = (f < 0 ? (int)-f : (int)f);
	for (ni=0,q=i; q > 0; q/=10,ni++)
	    n[ni] = q%10 + '0';
	/* point */
	p[0] = '.';
	/* below zero part */
	pp = (f < 0 ? -f : f) - (float)i;
	if (pp == 0)
	{
	    p[1] = '0'; pi = 2;
	}
	else
	    for (pi=1,pp*=10.0; (pp > 0) && (pi < 4); pp*=10.0,pi++)
	    {
		d = (int)pp;
		p[pi] = d + '0';
		pp -= d;
	    }
	p[pi] = '\0';
    }

    /* sign */
    if (f < 0)
    {
	n[ni] = '-';
	ni++;
    }

    /* pop it off and onto the string */
    for (si=0; ni>0; si++,ni--)
	s[si] = n[ni-1];
    for (pi=0; p[pi]; si++,pi++)
	s[si] = p[pi];

    return si;
}

int cst_vsprintf(char *s, const char *fmt, va_list args)
{
    /* A simple sprintf that caters for what flite uses */
    int sp=0,fp=0;
    char *sa;
    char ca;
    int ia;
    float fa;

    for (sp=0,fp=0; fmt[fp]; fp++)
    {
	if (fmt[fp] == '%')
	{
	    if (fmt[fp+1] == '%')
	    {
		s[sp] = '%';
		sp++; fp++;
	    }
	    else if (fmt[fp+1] == 's')
	    {
		sa = va_arg(args, char *);
		memmove(s+sp,sa,cst_strlen(sa));
		sp+=cst_strlen(sa); fp++;
	    }
	    else if (fmt[fp+1] == '.')
	    {   
		if (fmt[fp+2] == '*')
		{   /* When the length is specified on the stack */
		    ia = va_arg(args, int);
		    sa = va_arg(args, char *);
		    memmove(s+sp,sa,ia);
		    sp+=ia; fp+=3;
		}
		else
		{   /* When the length is specified in the format */
		    fp+=2;   /* skip over the field width */
		    while ((strchr("0123456789",fmt[fp]) != 0) &&
			   fmt[fp])
			fp++;
		    sa = va_arg(args, char *);
		    memmove(s+sp,sa,cst_strlen(sa));
		    sp+=cst_strlen(sa);
		}
	    }
	    else if (fmt[fp+1] == 'c')
	    {
		ca = (unsigned char)va_arg(args, unsigned int);
		s[sp] = ca;
		sp++; fp++;
	    }
	    else if (fmt[fp+1] == 'd')
	    {
		ia = va_arg(args, int);
		sp += po_itoa(s+sp,ia,"");
		fp++;
	    }
	    else if (fmt[fp+1] == 'f')
	    {
		fa = (float)va_arg(args, double);
		sp += po_ftoa(s+sp,fa,"");
		fp++;
	    }
	}
	else
	{
	    s[sp] = fmt[fp];
	    sp++;
	}
    }
    va_end(args);
    s[sp] = '\0';
	    
    return sp;
}

int cst_fprintf(cst_file fh, char *fmt, ...)
{
    va_list args;
    char outbuf[512];
    int count;

    va_start(args,fmt);
    count = cst_vsprintf(outbuf,fmt,args);
    va_end(args);
    return cst_fwrite(fh,outbuf,1,count);
}

int cst_sprintf(char *s, const char *fmt, ...)
{
    va_list args;
    int count;

    va_start(args,fmt);
    count = cst_vsprintf(s,fmt,args);
    va_end(args);

    return count;
}
