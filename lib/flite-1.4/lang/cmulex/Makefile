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
##  CMULEX                                                               ##
##                                                                       ##
###########################################################################
TOP=../..
DIRNAME=lang/cmulex
BUILD_DIRS = 
ALL_DIRS= 
H = cmu_lts_model.h cmu_lex.h
SRCS = cmu_lts_rules.c cmu_lts_model.c \
       cmu_lex.c cmu_lex_entries.c cmu_lex_data.c cmu_postlex.c

LEX_DATA_INCLUDES = cmu_lex_data_raw.c cmu_lex_num_bytes.c \
     cmu_lex_phones_huff_table.c cmu_lex_entries_huff_table.c \

OBJS = $(SRCS:.c=.o)
SCRIPTS=make_cmulex  

SCM=
WINDOWS = libflite-cmulex.vcproj
FILES = Makefile $(SCM) $(SRCS) $(H) $(SCRIPTS) $(LEX_DATA_INCLUDES) $(WINDOWS)
LIBNAME = flite_cmulex
LOCAL_INCLUDES = $(LEXDEFS)

ALL = 

include $(TOP)/config/common_make_rules

