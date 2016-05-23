;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;                                                                     ;;;
;;;                  Language Technologies Institute                    ;;;
;;;                     Carnegie Mellon University                      ;;;
;;;                         Copyright (c) 1999                          ;;;
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
;;;               Date: December 1999                                   ;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;; Convert the LTS cart trees to regular grammars and build wfsts
;;;
;;; Technique described in the Flite paper
;;;  http://www.cs.cmu.edu/~awb/papers/ISCA01/flite/flite.html
;;;
;;;
;;; call as:
;;;  festival $FLITEDIR/tools/make_lts_wfst.scm cmulex_lts_rules.scm \
;;;     '(lts_to_rg_to_wfst cmulex_lts_rules ".")'
;;;
;;; will make a bunch of *.tree.wfst files in "."
;;; use make_lts.scm:(ltsregextoC "cmulex" "." ".")
;;; to convert them to scheme
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(define (lts_to_rg_to_wfst lts_rules odir)
  (mapcar	
   (lambda (a)
     (format t "%s\n" (car a))
     (savergtree (car (cdr a)) 
		 (format nil "%s/%s.tree.rg" odir (car a)))
     (system 
      (format nil 
	      "$ESTDIR/bin/wfst_build -heap 10000000 -type rg -detmin -o %s/%s.tree.wfst %s/%s.tree.rg"
	      odir (car a)
	      odir (car a)))
     t)
   lts_rules)
  t)

(define (torg tree)
  "(torg tree)
Convert a CART tree to a regular grammar.  This will only
reasonably work for classification trees, not regression
trees."
  (set! torg_state 1)
  (torgrule tree (intern (format nil "s%d" torg_state))))

(define (ensymbolize l)
  (let ((ss "_"))
    (mapcar
     (lambda (a)
       (set! ss (string-append ss a "_")))
     l)
    ss))

(define (cart_tree_node_count tree)
  "(tree_node_count tree)
Count the number nodes (questions and leafs) in the given CART tree."
  (cond
   ((cdr tree)
    (+ 1
       (cart_tree_node_count (car (cdr tree)))
       (cart_tree_node_count (car (cdr (cdr tree))))))
   (t
    1)))

(define (torgrule tree state)
  (cond
   ((cdr tree)
    (let ((leftstate (intern (format nil "s%d" (+ 1 torg_state))))
	  (rightstate (intern (format nil "s%d" (+ 2 torg_state)))))
      (set! torg_state (+ 2 torg_state))
      (append
       (list
	(list state '-> (ensymbolize (car tree)) leftstate)
	(list state '-> (ensymbolize (cons 'not (car tree))) rightstate))
       (torgrule (car (cdr tree)) leftstate)
       (torgrule (car (cdr (cdr tree))) rightstate))))
   (t
    (let ((ss (car (last (car tree)))))
      (list
       (list state '-> 
	     (if (string-equal "_epsilon_" ss)
		 'epsilon
		 ss)))))))

(define (torg2 tree)
  (torgrule2 tree nil))

(define (torgrule2 tree p)
  (cond
   ((cdr tree)
    (append
     (torgrule2 (car (cdr tree)) (cons (ensymbolize (car tree)) p))
     (torgrule2 (car (cdr (cdr tree)))
		(cons (ensymbolize (cons 'not (car tree))) p))))
   (t
    (let ((ss (car (last (car tree)))))
      (list
       (cons 'S (cons '-> 
		      (reverse 
		       (mapcar (lambda (a) a)
			       (cons
				(if (string-equal "_epsilon_" ss)
				    'epsilon
				    ss)
				p))))))))))
	      

(define (savergtree tree fname)
  (let ((fd (fopen fname "w")))
;    (format fd "(RegularGrammar\n")
;    (format fd " name\n")
;    (format fd " ()\n")
;    (format fd "(\n")
;    (mapcar
;     (lambda (a) (format fd "%l\n" a))
;     (torg2 tree))
;    (format fd "))\n")
    (pprintf
     (list
      'RegularGrammar
      'name
      ()
      (torg tree))
     fd)
    (fclose fd)))
