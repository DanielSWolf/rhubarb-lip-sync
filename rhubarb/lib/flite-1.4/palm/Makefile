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
##                                                                       ##
##  PalmOS support for Flite                                             ##
##                                                                       ##
##  Provides a M68K interface to an ARM compiled Flite shared library    ##
##  and support in the ARM domain for accessing PalmOS system fuctions   ##
##                                                                       ##
##  FLOP: a demonstration program to show Flite speaking on a PalmOS     ##
##        device with PalmOS 5.0 , ARM and audio                         ##
##        Tested on a Treo 600 and a Zire 31                             ##
##                                                                       ##
##  fdlibm is a port of Sun's floating point library but we don't        ##
##  actually need it at present, so removed it from the compilation,     ##
##  but its still in CVS                                                 ##
##                                                                       ##
###########################################################################
TOP = ..
DIRNAME = palm
BUILD_DIRS = pocore arm_flite m68k_flite flop 
#ORPHANED = fdlibm
ALL_DIRS = $(BUILD_DIRS) include $(ORPHANED)
FILES = Makefile 

ALL = 

include $(TOP)/config/common_make_rules



