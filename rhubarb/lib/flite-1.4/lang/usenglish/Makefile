###########################################################################
##                                                                       ##
##                  Language Technologies Institute                      ##
##                     Carnegie Mellon University                        ##
##                        Copyright (c) 2000                             ##
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
##  US English general functions/models                                  ##
##                                                                       ##
###########################################################################
TOP=../..
DIRNAME=lang/usenglish
BUILD_DIRS = 
ALL_DIRS= 
H = usenglish.h us_int_accent_cart.h us_int_tone_cart.h us_durz_cart.h \
    us_ffeatures.h us_phrasing_cart.h us_text.h us_f0.h us_nums_cart.h \
    us_regexes.h us_pos_cart.h
SRCS = us_int_accent_cart.c us_int_tone_cart.c us_f0_model.c \
       us_dur_stats.c us_durz_cart.c us_f0lr.c \
       us_phoneset.c us_ffeatures.c us_phrasing_cart.c \
       us_gpos.c us_text.c us_expand.c \
       us_nums_cart.c us_aswd.c usenglish.c us_pos_cart.c
SCRIPTS = make_us_regexes us_pos.tree
OBJS = $(SRCS:.c=.o)
SCM=
WINDOWS = libflite-usenglish.vcproj
FILES = Makefile $(SCM) $(SRCS) $(H) $(SCRIPTS) $(WINDOWS)
LIBNAME = flite_usenglish
LOCAL_INCLUDES = 

ALL = 

include $(TOP)/config/common_make_rules

regexes:
	./make_us_regexes $(BINDIR) >us_regexes.h

us_pos_cart.c: us_pos.tree
	$(ESTDIR)/../festival/bin/festival -b $(FLITEDIR)/tools/make_cart.scm $(FLITEDIR)/tools/make_pos.scm 

