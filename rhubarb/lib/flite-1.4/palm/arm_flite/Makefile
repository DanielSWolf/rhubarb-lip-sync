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
###########################################################################
##                                                                       ##
##  Flite interface code and building of Flite code and data segments    ##
##  for ARM.  The main code is in armflite but large data structures     ##
##  are in separate .ro files                                            ##
##                                                                       ##
###########################################################################
TOP = ../..
DIRNAME = palm/arm_flite

SRCS = arm_flite.c pealstub.c 
OBJS := $(SRCS:.c=.o)
SCRIPTS=make_seg_ro
FILES = Makefile $(H) $(SRCS) $(SCRIPTS)
LOCAL_CLEAN = *.o *.ro *.bin

FLITE_LIBS = -lflite_cmu_us_kal -lflite_cmulex -lflite_usenglish -lflite
FLITE_LIB_FILES = $(BUILDDIR)/lib/libflite_cmu_us_kal.a \
                  $(BUILDDIR)/lib/libflite_cmulex.a \
                  $(BUILDDIR)/lib/libflite_usenglish.a \
                  $(BUILDDIR)/lib/libflite.a
PO_LIB_FILES = $(BUILDDIR)/lib/libpocore.a
PO_LIBS = -lpocore

PO_FLITE_LIBS = -L$(BUILDDIR)/lib $(FLITE_LIB_FILES) $(PO_LIBS)

ALL = armflite.ro clts.ro clex.ro clpc.ro cres.ro clpc.ro crsi.ro

include $(TOP)/config/common_make_rules

armflite.bin: $(OBJS) $(FLITE_LIB_FILES) $(PO_LIB_FILES)
	$(CC) -fPIC -Os -march=armv4t  -Wl,--split-by-file=64000 -msingle-pic-base -Wl,--emit-relocs -nostartfiles $(OBJS) -o armflite.bin $(PO_FLITE_LIBS)
#	$(CC) -fPIC -Os -march=armv4t  -Wl,--split-by-reloc=1 -msingle-pic-base -Wl,--emit-relocs -nostartfiles $(OBJS) -o armflite.bin $(PO_FLITE_LIBS)

armflite.ro: armflite.bin
	peal-postlink -v -s 1000 -t armc -o armflite.ro armflite.bin
#	peal-postlink -v -o armflite.unsplit armflite.bin

clts.ro: $(BUILDDIR)/obj/lang/cmulex/cmu_lts_model.o
	./make_seg_ro $(BUILDDIR)/obj/lang/cmulex/cmu_lts_model.o clts 1000

clex.ro:  $(BUILDDIR)/obj/lang/cmulex/cmu_lex_data.o
	./make_seg_ro $(BUILDDIR)/obj/lang/cmulex/cmu_lex_data.o clex 1000

clpc.ro:  $(BUILDDIR)/obj/lang/cmu_us_kal/cmu_us_kal_lpc.o
	./make_seg_ro $(BUILDDIR)/obj/lang/cmu_us_kal/cmu_us_kal_lpc.o clpc 1000

cres.ro:  $(BUILDDIR)/obj/lang/cmu_us_kal/cmu_us_kal_res.o
	./make_seg_ro $(BUILDDIR)/obj/lang/cmu_us_kal/cmu_us_kal_res.o cres 1000

crsi.ro:  $(BUILDDIR)/obj/lang/cmu_us_kal/cmu_us_kal_residx.o
	./make_seg_ro $(BUILDDIR)/obj/lang/cmu_us_kal/cmu_us_kal_residx.o crsi 1000


