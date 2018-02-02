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
##  FLOP -- FLite On PalmOS                                              ##
##  Author:  Alan W Black (awb@cs.cmu.edu)                               ##
##           November 2004                                               ##
##                                                                       ##
##  A demonstration program to show Flite speaking on a PalmOS device    ##
##                                                                       ##
###########################################################################
TOP = ../..
DIRNAME = palm/flop

RCP = flop.rcp flop.def
H = flop.h
SRCS = flop.c 
PRC = flop.prc
BMPS = flop.bmp flopsmall.bmp
OBJS := $(SRCS:.c=.o)
FILES = Makefile $(H) $(SRCS) $(RCP) $(BMPS)

LOCAL_CLEAN = *.[oa] flop *.bin *.stamp *.ro
FLITE_INCLUDES = -I$(TOP)/include
FLITE_LIBS = -L$(BUILDDIR)/lib -lm68k_flite

FLITE_RO = ../arm_flite/armflite.ro \
           ../arm_flite/clts.ro \
           ../arm_flite/clex.ro \
           ../arm_flite/clpc.ro \
           ../arm_flite/cres.ro \
           ../arm_flite/crsi.ro

include $(TOP)/config/common_make_rules

all: flop.prc

flop.prc: flop flop.ro $(FLITE_RO)
	build-prc -o flop.prc flop.def flop flop.ro $(FLITE_RO)

flop.ro: flop.rcp
	pilrc -ro flop.rcp flop.ro

flop: $(OBJS) $(BUILDDIR)/lib/libm68k_flite.a
	$(M68KCC) $(CFLAGS) -o flop $(OBJS) $(FLITE_LIBS) -lPalmOSGlue

flop.o: flop.c flop.h
	$(M68KCC) $(CFLAGS) $(INCLUDES) -c flop.c
peal.o: peal.c
	$(M68KCC) $(CFLAGS) $(INCLUDES) -c peal.c

