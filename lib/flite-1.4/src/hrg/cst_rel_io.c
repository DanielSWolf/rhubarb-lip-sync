/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                      Copyright (c) 1999-2000                          */
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
/*               Date:  September 2000                                   */
/*************************************************************************/
/*                                                                       */
/*  Relations i/o                                                        */
/*                                                                       */
/*************************************************************************/
#include "cst_file.h"
#include "cst_tokenstream.h"
#include "cst_item.h"
#include "cst_relation.h"
#include "cst_utterance.h"

int relation_load(cst_relation *r, const char *filename)
{
    cst_tokenstream *fd;
    cst_item *item;
    const char *token=0;

    if ((fd = ts_open(filename,NULL,";","","")) == 0)
    {
	cst_errmsg("relation_load: can't open file \"%s\" for reading\n",
		   filename);
	return CST_ERROR_FORMAT;
    }

    for ( ; !ts_eof(fd); )
    {
	token = ts_get(fd);
	if (cst_streq("#",token))
	    break;
    }
    if (!cst_streq("#",token))
    {
	cst_errmsg("relation_load: no end of header marker in \"%s\"\n",
		   filename);
	ts_close(fd);
	return CST_ERROR_FORMAT;
    }
	
    while (!ts_eof(fd))
    {
	token = ts_get(fd);
	if (cst_streq(token,""))
	    continue;
	item = relation_append(r,NULL);
	item_set_float(item,"end",(float)cst_atof(token));
	token = ts_get(fd);
	token = ts_get(fd);
	item_set_string(item,"name",token);
    }

    ts_close(fd);
    return CST_OK_FORMAT;
}

int relation_save(cst_relation *r, const char *filename)
{
    cst_file fd;
    cst_item *item;

#ifndef UNDER_CE
    if (cst_streq(filename,"-"))
	fd = stdout;
    else
#endif
	if ((fd = cst_fopen(filename,CST_OPEN_WRITE)) == 0)
    {
	cst_errmsg("relation_save: can't open file \"%s\" for writing\n",
		   filename);
	return CST_ERROR_FORMAT;
    }

    for (item=relation_head(r); item; item=item_next(item))
    {
        if (item_feat_present(item,"end"))
	    cst_fprintf(fd,"%f ",item_feat_float(item,"end"));
	else
	    cst_fprintf(fd,"%f ",0.00);
        if (item_feat_present(item,"name"))
	    cst_fprintf(fd,"%s ",item_feat_string(item,"name"));
	else
	    cst_fprintf(fd,"%s ","_");
	cst_fprintf(fd,"\n");
    }
#ifndef UNDER_CE
    if (fd != stdout)
	cst_fclose(fd);
#endif

    return CST_OK_FORMAT;
}

