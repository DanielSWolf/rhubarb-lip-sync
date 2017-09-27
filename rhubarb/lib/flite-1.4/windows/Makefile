###########################################################################
##                                                                       ##
##                  Language Technologies Institute                      ##
##                     Carnegie Mellon University                        ##
##                         Copyright (c) 2008                            ##
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
##  Windows Mobile support for Flite                                     ##
##                                                                       ##
##  Builds a usable flite, language and voice .dlls with mingw           ##
##                                                                       ##
##     http://www.mingw.org                                              ##
##                                                                       ##
##  Also an example command line program                                 ## 
##                                                                       ##
##  At top level                                                         ##
##      ./configure --target=i386-mingw32                                ##
##      make                                                             ##
##                                                                       ##
###########################################################################
TOP = ..
DIRNAME = windows
BUILD_DIRS = 
ALL_DIRS = $(BUILD_DIRS) 
H = 
RC = 
SRCS = 
OBJS = $(SRCS:.c=.o)
WINDOWS = fliteDll.cpp stdafx.h
FILES = Makefile $(SRCS) $(H) $(RC) $(WINDOWS)

flite_LIBS = flite_$(FL_VOX) flite_$(FL_LANG) flite_$(FL_LEX) 
flite_LIBS_flags = -L$(LIBDIR) $(flite_LIBS:%=-l%)
flite_LIBS_deps = $(flite_LIBS:%=$(LIBDIR)/lib%.a)

LOCAL_CLEAN = flite.dll flite.lib flite.def

ALL = 

include $(TOP)/config/common_make_rules

flite.def: $(LIBDIR)/libflite.a
	echo "LIBRARY FLITE.DLL" >flite.def
	echo "EXPORTS" >>flite.def
	$(NM) $(LIBDIR)/libflite.a | \
	grep "^0" | awk '{if ($$2 == "T") print $$3}' | sed 's/_//' >>flite.def

flite.dll: $(LIBDIR)/libflite.a
	rm -rf flitedll
	mkdir flitedll
	(cd flitedll && ar x ../$(LIBDIR)/libflite.a)
	$(DLLWRAP) -def flite.def flitedll/*.o --output-lib flite.lib -o flite.dll
	rm -rf flitedll



