###########################################################################
##                                                                       ##
##                  Language Technologies Institute                      ##
##                     Carnegie Mellon University                        ##
##                       Copyright (c) 2008-2009                         ##
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
##  Builds a usable flite library for Windows Mobile (WinCE)             ##
##                                                                       ##
##  FLOWM: flite on windows mobile                                       ##
##         Basic file reader for files                                   ##
##                                                                       ##
##  Note this uses the cegcc compilation suite (Linux/CYGWIN) rather     ##
##  than embedded visual studio.                                         ##
##                http://cegcc.sourceforge.net/                          ##
##                                                                       ##
##  At top level                                                         ##
##      ./configure --target=arm-wince                                   ##
##      make                                                             ##
##  Will generate wince executable in wince/flowm.exe                    ##
##                                                                       ##
###########################################################################
TOP = ..
DIRNAME = wince
BUILD_DIRS = 
ALL_DIRS = $(BUILD_DIRS) 
H = flowm.h
RC = flowm.rc
SRCS = flowm_main.c flowm_flite.c
OBJS = $(SRCS:.c=.o)
IMGS = flowm.bmp flowm.ico
OTHER = _setup.xml
FILES = Makefile flowm.notes $(SRCS) $(H) $(RC) $(IMGS) $(OTHER)
FLOWM_VERSION=0.8

flite_LIBS = flite_$(FL_LANG) flite_$(FL_LEX) flite
flite_LIBS_flags = -L$(LIBDIR) $(flite_LIBS:%=-l%)
flite_LIBS_deps = $(flite_LIBS:%=$(LIBDIR)/lib%.a)

# If you only want one voice, it should be kal -- its fast and small enough
#voice_LIBS_flags = -lflite_cmu_us_kal 
#EXTRA_CC_FLAGS = -Dcmu_us_kal

# OR you can try these specially built voices for WM
# Special 8KH, reduced order (13) voices, seem to be fast enough
# for 400MHz ARM (Tytn II)
voice_LIBS_flags = -Lvoices -lcmu_us_kal -lcmu_us_awb -lcmu_us_rms -lcmu_us_slt
EXTRA_CC_FLAGS = -Dcmu_us_kal -Dcmu_us_awb -Dcmu_us_rms -Dcmu_us_slt

wince_LIBS = -lcommctrl -laygshell
EXTRA_CC_FLAGS += -D_WIN32_IE=0x0500

ALL = flowm.exe

include $(TOP)/config/common_make_rules

flowm.exe: $(OBJS) $(H) flowm.rsc $(FLITELIBS) $(flite_LIBS_deps)
	$(CC) $(CFLAGS) -o $@ $(OBJS) flowm.rsc $(voice_LIBS_flags) $(flite_LIBS_flags) $(LDFLAGS) $(wince_LIBS)
	$(STRIP) flowm.exe
	$(SIZE) flowm.exe

flowm.rsc: flowm.rc flowm.h
	$(WINDRES) $(EXTRA_CC_FLAGS) flowm.rc flowm.rsc

flowm.cab: flowm.exe
	cp -pr ../doc/alice alice.txt
	cp -pr ../doc/intro.txt flintro.txt
	lcab alice.txt flintro.txt flowm.exe _setup.xml flowm.cab

backup:
	tar jcvf flowm_${FLOWM_VERSION}.tar.gz ${FILES}

