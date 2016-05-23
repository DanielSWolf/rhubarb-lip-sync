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
##  Flite basic include files                                            ##
##                                                                       ##
###########################################################################
TOP=..
DIRNAME=include
BUILD_DIRS = 
ALL_DIRS= 
H = cst_alloc.h  \
	cst_args.h \
	cst_audio.h \
	cst_cart.h \
	cst_clunits.h \
	cst_cg.h \
	cst_diphone.h \
	cst_endian.h \
	cst_error.h \
	cst_features.h \
	cst_file.h \
	cst_hrg.h \
	cst_item.h \
        cst_math.h \
	cst_lexicon.h \
	cst_lts.h \
	cst_lts_rewrites.h \
	cst_phoneset.h \
	cst_regex.h \
	cst_relation.h \
	cst_sigpr.h \
	cst_socket.h \
	cst_string.h \
	cst_sts.h \
	cst_synth.h \
	cst_tokenstream.h \
	cst_track.h \
	cst_units.h \
	cst_utt_utils.h \
	cst_utterance.h \
	cst_val.h \
	cst_val_const.h \
	cst_val_defs.h \
	cst_viterbi.h \
	cst_voice.h \
	cst_wave.h \
	cst_ss.h \
        cst_ffeatures.h \
        cst_wchar.h \
	flite.h

FILES = Makefile $(H)
INSTALL_HEADERS = $(H)

ALL = flite_version.h

include $(TOP)/config/common_make_rules

flite_version.h: $(TOP)/config/project.mak
	@grep "^PROJECT" $(TOP)/config/project.mak | \
	   awk '{printf("#define FLITE_%s \"%s",$$1,$$3); for (i=4; i<=NF; i++) printf(" %s",$$i); printf("\"\n")}' >flite_version.h

