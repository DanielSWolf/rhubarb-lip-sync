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
/*             Author:  Alan W Black (awb@cs.cmu.edu)                    */
/*               Date:  November 2001                                    */
/*************************************************************************/
/*                                                                       */
/*  Argument/usage command parser (like that in EST)                     */
/*                                                                       */
/*************************************************************************/
#include "cst_tokenstream.h"
#include "cst_features.h"

static void parse_description(const char *description, cst_features *f);
static void parse_usage(const char *progname,
			const char *s1, const char *s2,
			const char *description);

cst_val *cst_args(char **argv, int argc,
		  const char *description,
		  cst_features *args)
{
    /* parses the given arguments wrt the description */
    cst_features *op_types = new_features();
    cst_val *files = NULL;
    int i;
    const char *type;
 
    parse_description(description,op_types);

    for (i=1; i<argc; i++)
    {
	if (argv[i][0] == '-')
	{
	    if ((!feat_present(op_types,argv[i])) ||
		(cst_streq("-h",argv[i])) ||
		(cst_streq("-?",argv[i])) ||
		(cst_streq("-help",argv[i])))
		parse_usage(argv[0],"","",description);
	    else
	    {
		type = feat_string(op_types,argv[i]);
		if (cst_streq("<binary>",type))
		    feat_set_string(args,argv[i],"true");
		else
		{
		    if (i+1 == argc)
			parse_usage(argv[0],
				    "missing argument for ",argv[i],
				    description);
		    if (cst_streq("<int>",type))
			feat_set_int(args,argv[i],atoi(argv[i+1]));
		    else if (cst_streq("<float>",type))
			feat_set_float(args,argv[i],atof(argv[i+1]));
		    else if (cst_streq("<string>",type))
			feat_set_string(args,argv[i],argv[i+1]);
		    else
			parse_usage(argv[0],
				    "unknown arg type ",type,
				    description);
		    i++;
		}
	    }
	}
	else
	    files = cons_val(string_val(argv[i]),files);
    }
    delete_features(op_types);

    return val_reverse(files);
}

static void parse_usage(const char *progname,
			const char *s1, const char *s2,
			const char *description)
{
    cst_errmsg("%s: %s %s\n", progname,s1,s1);
    cst_errmsg("%s\n",description);
    exit(0);
}

static void parse_description(const char *description, cst_features *f)
{
    /* parse the description into something more usable */
    cst_tokenstream *ts;
    const char *arg;
    char *op;

    ts = ts_open_string(description,
			" \t\r\n", /* whitespace */
			"{}[]|",   /* singlecharsymbols */
			"",        /* prepunctuation */
			"");       /* postpunctuation */
    while (!ts_eof(ts))
    {
	op = cst_strdup(ts_get(ts));
	if ((op[0] == '-') && (cst_strchr(ts->whitespace,'\n') != 0))
	{    /* got an option */
	    arg = ts_get(ts);
	    if (arg[0] == '<')
		feat_set_string(f,op,arg);
	    else
		feat_set_string(f,op,"<binary>");
	}
        else
            cst_free(op);
    }

    ts_close(ts);

}
