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
;;;               Date: January 2001                                    ;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;                                                                     ;;;
;;; Convert the LT models to C                                          ;;;
;;;                                                                     ;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(define (f0lrtoC name start mid end odir)
  "(f0lrtoC name start mid endn odir)
Coverts a f0lr models to a single C fstructure in ODIR/NAME_f0lr.c.  As
there is only one model like this (f2b) this is really only
one off. and the result is included in another file so this doesn't
add the headers defs etc just the const initializations"
  (let 
    ((ofdc (fopen (path-append odir (string-append name "_f0lr.c")) "w")))

    (format ofdc "\n")

    (format ofdc "const us_f0_lr_term f0_lr_terms[] = {\n")
    (while start
     (if (not (and (equal? (car (car start)) (car (car mid)))
		   (equal? (car (car start)) (car (car end)))))
	 (error "don't have the same features"))
     (if (or (not (car (cddr (car start))))
	     (member_string (car (car (cddr (car start))))
			    '("H*" "!H*" "*?" "L-L%" "L-H%" "H-" "!H-")))
	 ;; ignore the terms that can never be 1 as cart doesn't predict them
	 (begin
	   (format ofdc "    { \"%s\", %f, %f, %f, %s },\n"
		   (car (car start))
		   (cadr (car start))
		   (cadr (car mid))
		   (cadr (car end))
		   (cond
		    ((not (car (caddr (car start))))
		     "0")
		    ((string-equal (car (caddr (car start))) "*?")
		     "\"L+H*\"")
		    ((string-equal (car (caddr (car start))) "!H-")
		     "\"H-H%\"")
		    (t 
		     (format nil "\"%s\"" (car (caddr (car start)))))))))
     (set! start (cdr start))
     (set! mid (cdr mid))
     (set! end (cdr end)))
		     
    (format ofdc "    { 0, 0, 0, 0, 0 }\n")
    (format ofdc "};\n")

    (fclose ofdc)))

(provide 'make_f0lr)
