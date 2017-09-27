###########################################################################
##                                                                       ##
##                  Language Technologies Institute                      ##
##                     Carnegie Mellon University                        ##
##                         Copyright (c) 2004                            ##
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
##  Author:  Alan W Black (awb@cs.cmu.edu)                               ##
##           November 2004                                               ##
##                                                                       ##
##  In order to call PalmOS System functions from ARM you need to make a ##
##  M68K system call.  This contains the basic glue code to do this      ##
##                                                                       ##
##  Also maps some posix functions names to PalmOS names so we don't     ##
##  need to change the core code                                         ##
##                                                                       ##
##  Done as lots of different files so you only end up linking in the    ##
##  you use                                                              ##
##                                                                       ##
##  Inspired by the output from gen_pace.pl Eric House (fixin@peak.org)  ##
##  and the PalmOS docs, but done by hand.                               ##
##                                                                       ##
###########################################################################
TOP = ../..
DIRNAME = palm/pocore

SRCS = po_alloc.c \
       po_MemPtrNew.c po_MemChunkFree.c \
       po_FileTell.c po_FileOpen.c po_FileWrite.c \
       po_FileSeek.c po_FileReadLow.c po_FileClose.c \
       po_atof.c po_sio.c \
       po_core.c po_setjmp.c
OTHERS = po_StrPrintF.c po_StrVPrintF.c \

OBJS := $(SRCS:.c=.o)
FILES = Makefile $(H) $(SRCS) $(OTHERS)

LIBNAME = pocore

ALL = 

include $(TOP)/config/common_make_rules

