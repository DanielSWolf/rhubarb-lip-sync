###########################################################################
##                                                                       ##
##                  Language Technologies Institute                      ##
##                     Carnegie Mellon University                        ##
##                        Copyright (c) 2000-2008                        ##
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
##  Tools for converting Festvox structures to Flite code                ##
##                                                                       ##
###########################################################################
TOP=..
DIRNAME=tools
BUILD_DIRS = 
ALL_DIRS=
SRCS = find_sts_main.c flite_sort_main.c
OBJS = $(SRCS:.c=.o)
TMPLTS = VOICE_ldom.c VOICE_clunits.c VOICE_diphone.c \
         VOICE_cg.c Makefile.flite 
SCRIPTS = build_flite setup_flite huff_table flite_test \
          make_voice_list wfst2c build_lex
SCM=make_lex.scm make_lts.scm make_cart.scm play_sync.scm \
    make_phoneset.scm make_vallist.scm make_clunits.scm \
    make_didb.scm make_didb2.scm make_f0lr.scm make_lts_wfst.scm \
    make_lts_rewrite.scm \
    make_cg.scm

FILES = Makefile $(SCM) $(SRCS) $(SCRIPTS) $(TMPLTS) find_cmimax example.lex
LOCAL_INCLUDES = 

ifeq ($(HOST_PLATFORM),$(TARGET_PLATFORM))
ALL = $(BINDIR)/find_sts $(BINDIR)/flite_sort makescripts
endif	

include $(TOP)/config/common_make_rules

LOCAL_CLEAN = $(BINDIR)/find_sts $(BINDIR)/flite_sort

$(BINDIR)/find_sts: find_sts_main.o $(FLITELIBS) 
	$(CC) $(CFLAGS) -o $@ find_sts_main.o $(FLITELIBFLAGS) $(LDFLAGS)
$(BINDIR)/flite_sort: flite_sort_main.o $(FLITELIBS)
	$(CC) $(CFLAGS) -o $@ flite_sort_main.o $(FLITELIBFLAGS) $(LDFLAGS)
makescripts:
	@ cp -p $(SCRIPTS) $(TOP)/bin

