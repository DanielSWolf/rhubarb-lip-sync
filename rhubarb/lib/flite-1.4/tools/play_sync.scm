;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;                                                                     ;;;
;;;                  Language Technologies Institute                    ;;;
;;;                     Carnegie Mellon University                      ;;;
;;;                         Copyright (c) 2000                          ;;;
;;;                        All Rights Reserved.                         ;;;
;;;                                                                     ;;;
;;; Permission is hereby granted, free of charge, to use and distribute ;;;
;;; this software and its documentation without restriction, including  ;;;
;;; without limitation the rights to use, copy, modify, merge, publish, ;;;
;;; distribute, sublicense, and/or sell copies of this work, and to     ;;;
;;; permit persons to whom this work is furnished to do so, subject to  ;;;
;;; the following conditions:                                           ;;;
;;;  1. The code must retain the above copyright notice, this list of   ;;;
;;;     conditions and the following disclaimer.                        ;;;
;;;  2. Any modifications must be clearly marked as such.               ;;;
;;;  3. Original authors' names are not deleted.                        ;;;
;;;  4. The authors' names are not used to endorse or promote products  ;;;
;;;     derived from this software without specific prior written       ;;;
;;;     permission.                                                     ;;;
;;;                                                                     ;;;
;;; CARNEGIE MELLON UNIVERSITY AND THE CONTRIBUTORS TO THIS WORK        ;;;
;;; DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING     ;;;
;;; ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT  ;;;
;;; SHALL CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE     ;;;
;;; FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES   ;;;
;;; WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN  ;;;
;;; AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,         ;;;
;;; ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF      ;;;
;;; THIS SOFTWARE.                                                      ;;;
;;;                                                                     ;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;             Author: Alan W Black (awb@cs.cmu.edu)                   ;;;
;;;               Date: September 2000                                  ;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;                                                                     ;;;
;;; Display as we go synthesis                                          ;;;
;;;                                                                     ;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defvar play_sync_program "/home/awb/projects/flite/testsuite/play_sync")

(define (display_as_we_go file filemode)
  "(display_as_we_go file filemode)
Display the words as we synthesize them."
  (set! tts_hooks
	(list utt.synth utt.play_display))
  (tts_file file filemode)
  (format t "\n\n"))

(define (utt.play_display utt)
  "(utt.play_display utt)
Play and display."
  (let ((tmpwave (make_tmp_filename))
	(tmpword (make_tmp_filename)))
    (utt.save.wave utt tmpwave 'riff)
    (utt.save.play_words utt tmpword)
;    (utt.save.segs utt tmpword)
    (system (format nil "%s %s %s" play_sync_program tmpwave tmpword))
    (delete-file tmpwave)
    (delete-file tmpword)
    utt))

(define (utt.save.play_words utt filename)
"(utt.save.play_words UTT FILE)
  Save words of UTT in a FILE in xlabel format, wityh added __silence__."
  (let ((fd (fopen filename "w")))
    (format fd "#\n")
    (mapcar
     (lambda (w)
       (if (string-equal
	    (item.feat w "R:SylStructure.daughter1.daughter1.R:Segment.p.name")
	    (car (cadr (car (PhoneSet.description '(silences))))))
	   (format fd "%2.4f 100 %s\n" 
		   (item.feat w "R:SylStructure.daughter1.daughter1.R:Segment.p.end")
		   "__silence__"))
       (format fd "%2.4f 100 %s\n" (item.feat w "word_end")
	       (item.name w)))
     (utt.relation.items utt 'Word))
    (fclose fd)
    utt))


(provide 'play_sync)
