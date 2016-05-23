###########################################################################
##                                                                       ##
##                  Language Technologies Institute                      ##
##                     Carnegie Mellon University                        ##
##                         Copyright (c) 1999                            ##
##                        All Rights Reserved.                           ##
##                                                                       ##
##  Permission is hereby granted, free of charge, to use and distribute  ##
##  this software and its documentation without restriction, including   ##
##  without limitation the rights to use, copy, modify, merge, publish,  ##
##  distribute, sublicense, and/or sell copies of this work, and to      ##
##  permit persons to whom this work is furnished to do so, subject to   ##
##  the following conditions:                                            ##
##   1. The code must retain the above copyright notice, this list of    ##
##      conditions and the following disclaimer.                         ##
##   2. Any modifications must be clearly marked as such.                ##
##   3. Original authors' names are not deleted.                         ##
##   4. The authors' names are not used to endorse or promote products   ##
##      derived from this software without specific prior written        ##
##      permission.                                                      ##
##                                                                       ##
##  CARNEGIE MELLON UNIVERSITY AND THE CONTRIBUTORS TO THIS WORK         ##
##  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      ##
##  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   ##
##  SHALL CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE      ##
##  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    ##
##  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   ##
##  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          ##
##  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       ##
##  THIS SOFTWARE.                                                       ##
##                                                                       ##
###########################################################################
##                                                                       ##
##  Documenation                                                         ##
##                                                                       ##
###########################################################################
TOP=..
DIRNAME=doc
BUILD_DIRS = 
ALL_DIRS=
FILES = Makefile flite.texi intro.txt alice stuff.ed

ALL = 

include $(TOP)/config/common_make_rules

doc: flite.html flite.pdf
	cp -p flite.pdf html

flite.html: flite.texi
	@ if [ ! -d html ] ; \
          then mkdir -p html ; fi
	(cd html; texi2html -number -split_chapter ../flite.texi)
	@ if [ -d html/flite ] ; \
	  then mv html/flite/*.html html ; \
               rmdir html/flite; fi
	@ for i in html/*.html ; \
	  do \
	    sed 's/<BODY>/<BODY bgcolor="#ffffff">/' $$i >ttt.html; \
	    mv ttt.html $$i ; \
	  done
	@ cat stuff.ed | ed html/flite_toc.html
	@ (cd html; rm -f index.html; ln flite_toc.html index.html)

flite.pdf:  flite.texi
	@ texi2pdf flite.texi



