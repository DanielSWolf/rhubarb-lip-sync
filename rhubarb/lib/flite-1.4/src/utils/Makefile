###########################################################################
##                                                                       ##
##                  Language Technologies Institute                      ##
##                     Carnegie Mellon University                        ##
##                        Copyright (c) 1999                             ##
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
##    C Speech Tools library: basic functionality for system access      ##
##                                                                       ##
###########################################################################
TOP=../..
DIRNAME=src/utils
BUILD_DIRS = 
ALL_DIRS= 
H = 
include $(TOP)/config/config
SRCS = cst_alloc.c cst_error.c cst_string.c cst_wchar.c \
       cst_tokenstream.c cst_val.c cst_features.c \
       cst_endian.c cst_socket.c cst_val_const.c \
       cst_val_user.c cst_args.c 
OBJS := $(SRCS:.c=.o) \
        $(MMAPTYPE:%=cst_mmap_%.o) \
        $(STDIOTYPE:%=cst_file_%.o)
FILES = Makefile $(H) $(SRCS) \
	cst_mmap_posix.c cst_mmap_win32.c cst_mmap_none.c \
	cst_file_stdio.c cst_file_wince.c cst_file_palmos.c
LIBNAME = flite

LOCAL_INCLUDES = $(SOCKETS)

ALL = 

include $(TOP)/config/common_make_rules

