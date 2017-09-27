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
;;;                                                                     ;;;
;;; Generate a C compilable lexicon file from a Festival lexicon        ;;;
;;;                                                                     ;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (lextoC name ifile odir)
  "(lextoc name ifile ofile)
Convert a (Festival) compiled lexicon file to a format suitable
For C to compile."
  (let 
    (
     ;; The general tables and indices
     (ofde (fopen (path-append odir (string-append name "_lex_entries.c")) "w"))
     ;; The lexical entries (words and pronunciations) for later compression
     (ofdsd (fopen (path-append odir (string-append name "_lex_data")) "w"))
     ;; number of bytes in data (that may get reset by compression)
     (ofdi (fopen (path-append odir (string-append name "_lex_num_bytes.c")) "w"))
     ;; data file for non-compression case
     (ofddc (fopen (path-append odir (string-append name "_lex_data.c")) "w"))
     (ofddrc (fopen (path-append odir (string-append name "_lex_data_raw.c")) "w"))
     (ifd (fopen ifile "r"))
     (phone_table))
    (format ofde "/*******************************************************/\n")
    (format ofde "/**  generated lexicon entry file from %s    */\n" name)
    (format ofde "/*******************************************************/\n")
    (format ofde "\n")
    (format ofde "#include \"cst_string.h\"\n")
    (format ofde "#include \"cst_lexicon.h\"\n")
    (format ofde "\n")

    (format ofde "const int %s_lex_num_bytes = \n" name)
    (format ofde "#include \"%s_lex_num_bytes.c\"\n" name)
    (format ofde ";\n")
    (format ofde "\n")

    (format ofddc "/*******************************************************/\n")
    (format ofddc "/**  generated lexicon data file from %s    */\n" name)
    (format ofddc "/*******************************************************/\n")
    (format ofddc "\n")
    (format ofddc "const unsigned char %s_lex_data[] = \n" name)
    (format ofddc "{\n")
    (format ofddc "   0,\n")
    (format ofddc "#include \"%s_lex_data_raw.c\"\n" name)
    (format ofddc"};\n")
    (format ofddc "\n")

    (format ofddrc "/**  generated lexicon data file from %s uncompressed   */\n" name)

    (set! phone_table (l2C_dump_entries ifd ofdsd ofddrc ofdi))
    
    ;; Number of entries
    (format ofde "const int %s_lex_num_entries = %d;\n" name lex_num_entries)
    (format ofde "\n")
    
    ;; The phone table (bytes to phone names)
    (format ofde "const char * const %s_lex_phone_table[%d] = \n" 
	    name (+ 1 (length phone_table)))
    (format ofde "{\n")
    (mapcar (lambda (p) (format ofde "    \"%s\",\n" p)) phone_table)
    (format ofde "    NULL\n")
    (format ofde "};\n")
    (format ofde "\n")

    (format ofde "const char * const %s_lex_phones_huff_table[%d] = \n" 
	    name 257)
    (format ofde "{\n")
    (format ofde "    NULL, /* reserved */\n")
    (format ofde "#include \"%s_lex_phones_huff_table.c\"\n" name)
    (format ofde "    NULL\n")
    (format ofde "};\n")
    (format ofde "\n")

    (format ofde "const char * const %s_lex_entries_huff_table[%d] = \n" 
	    name 257)
    (format ofde "{\n")
    (format ofde "    NULL, /* reserved */\n")
    (format ofde "#include \"%s_lex_entries_huff_table.c\"\n" name)
    (format ofde "    NULL\n")
    (format ofde "};\n")

;    ;; The register function -- now hand coded else where
;    (format ofde "\n")
;    (format ofde "void register_lex_%s()\n" name)
;    (format ofde "{\n")
;    (format ofde "   lexicon *lex = new_lexicon();\n")
;    (format ofde "   lex->name = cst_strdup(\"%s\");\n" name)
;    (format ofde "   lex->num_entries = %s_num_entries;\n" name)
;    (format ofde "   lex->entry_index = %s_lex_entry;\n" name)
;    (format ofde "   lex->phones = %s_lex_phones;\n" name)
;    (format ofde "   lex->phone_table = %s_phone_table;\n" name)
;    (format ofde "   lexicon_register(lex);\n")
;    (format ofde "}\n")
    (format ofde "\n")

    (fclose ofde)
    (fclose ofdsd)
    (fclose ofdi)
    (fclose ofddc)
    (fclose ofddrc)
    ))

(define (enoctal x)
  "(enoctal x)
Return number as three digit octal"
  (let (units eights sixtyfours)
    (set! units (% x 8))
    (set! eights (% (/ (- x units) 8) 8))
    (set! sixtyfours (% (/ (- (/ (- x units) 8) eights) 8) 8))
    (format nil "%d%d%d" sixtyfours eights units)))

(define (l2C_dump_entries ifd ofdsd ofddrc ofdi)
  "(l2C_dump_entries ifd ofde)
We dump the entries and prunciation in simple strings to ofdsd which
is used for compression later, and also dump the entries to ofddrc
with their index to ofdi in the format they will be used in.  For
compressions sake we dump the prunciations in reverse (sharing the
null terminator with the previous entry and the entry itself forward.
The index points at the start of the entry (so you have to go backwards to
get the prunciation."
  (let ((phone_table (list "_epsilon_" ))
	(entry)
	(entry_count 0)
	(pos)
	(pcount 0))
;    (if (not (string-equal "MNCL" (readfp ifd)))
;	(error "L2C: input file is not a compiled lexicon\n"))
    (set! pcount (+ 1 pcount)) ;; the initial 0
    (while (not (equal? (set! entry (readfp ifd)) (eof-val)))
     (if (not (car (cdr entry)))
	 (set! pos "0")
	 (set! pos (substring 
		    (string-append (car (cdr entry))) 0 1)))
     (format t "entry: %l\n" entry)
     (if (or (car (cdr entry))     ;; hmm not sure of these conditions
;	     (< (length (car entry)) 5)   ;; awb 28/12/2004
	     (not (equal? (car (cdr (cdr (cdr entry))))
			  (l2C_phonetize (car (cdr (cdr entry)))))))
	 (begin
	   ;; Lexical entry
	   (set! entry_count (+ 1 entry_count))
           (if (consp (car entry))
               (set! simple_entry 
                     (format nil 
                             "%s%s" 
                             pos 
                             (apply string-append (car entry))))
               (set! simple_entry (format nil "%s%s" pos (car entry))))
	   (set! phone_list (l2C_phonetize (car (cdr (cdr entry)))))

	   ;; To the data.c file
	   (format ofddrc "   ")
	   (mapcar 
	    (lambda (p)
	      (format ofddrc "%d," (l2C_phone_index p 0 phone_table))
	      (set! pcount (+ 1 pcount)))
	    (reverse phone_list))
	   (set! pcount (+ 1 pcount))
	   (format ofddrc " 255, /* %s %d */ " simple_entry pcount)
	   (mapcar
	    (lambda (l)
	      (if (string-equal l "'")
		  (format ofddrc "'\\%s'," l) ;; unpacked is "readable"
		  (format ofddrc "'%s'," l))) ;; unpacked is "readable"
	    (symbolexplode simple_entry))
	   (format ofddrc "0,\n") ;; terminator for entry
	   (set! pcount (+ 1 pcount))
	   
	   ;; to the data file for potential other compression
	   (format ofdsd "%s " simple_entry)
	   (mapcar
	    (lambda (p)
	      (format ofdsd "\\%s" 
		      (enoctal (l2C_phone_index p 0 phone_table))))
	    phone_list)
	   (format ofdsd "\n")
	   )
	 (format t "   skipped\n")))
    (format ofdi "    %d\n" pcount)
    (set! lex_num_entries entry_count) ;; shouldn't be a global
    phone_table))

(define (l2C_phone_index p n table)
  (cond
   ((string-equal p (car table))
    n)
   ((not (cdr table))  ;; new p
    (set-cdr! table (list p))
    (+ 1 n))
   (t
    (l2C_phone_index p (+ 1 n) (cdr table)))))

;; Should be a better way to do this
(set! vowels
      '(
	;; radio (CMULEX)
	aa ae ah ao aw ax axr ay eh el em en er ey ih iy ow oy uh uw
	;; mrpa (OALD)
	uh e a o i u ii uu oo aa @@ ai ei oi au ou e@ i@ u@ @
        ;; ogi_worldbet
        i: I E @ u U ^ & > A 3r ei aI >i iU aU oU 
        ))

(define (is_a_vowel p)
;  (if (> (car (cdr festival_version_number)) 4)
;      (string-equal "+" (Param.get (format nil "phoneset.%s.vc" p)))
;      (string-equal "+"
;       (car (cdr 
;            (assoc_string p
;              (car (cdr 
;               (assoc 'phones
;                     (PhoneSet.description '(phones)))))))))
      (member_string p vowels)
;      )
)

(define (l2C_phonetize syls)
  "(l2C_phonetize syls)
Return simple list of atomic phone/stress values"
  (if (consp (car syls))
      ;; syllabified entries
      (apply 
       append
       (mapcar 
	(lambda (syl)
	  (mapcar
	   (lambda (p)
	     (if (is_a_vowel p)
		 (intern (string-append p (car (cdr syl))))
		 p))
	   (car syl)))
	syls))
      ;; Flat entries (from lts_test maybe)
      syls))

(define (L2C_make_phone_index_tree name ifile ofile)
  "(L2C_make_phone_index_tree name ifile ofile)
Build a regular grammar for the words in this lexicon in order
to build a more efficent representation of the pronunciations."
  (let ((ifd (fopen ifile "r"))
	(ofd (fopen ofile "w"))
	entry)
    (if (not (string-equal "MNCL" (readfp ifd)))
	(error "L2C: input file is not a compiled lexicon\n"))
    (format ofd "(TreeLexicon\n")
    (format ofd "  %s\n" name)
    (format ofd "  nil\n")
    (format ofd "  (\n")
    (while (not (equal? (set! entry (readfp ifd)) (eof-val)))
      (format ofd "   ( ")
      (mapcar 
       (lambda (l)
	 (format ofd "%s " l))
       (reverse (symbolexplode (car entry))))
      (if (car (cdr entry))
	  (format ofd "pos_%s " (car (cdr entry)))
	  (format ofd "0 "))
      (mapcar 
       (lambda (l)
	 (format ofd "%s " l))
       (l2C_phonetize (car (cdr (cdr entry)))))
      (format ofd " -> W 1.0)\n"))
    (format ofd "  ))\n")
    ))

(define (remove_predictable_entries infile outfile ltsfile)
  "(remove_predictable_entries infile outfile ltsfile)
Check each entry in infile against the lts rules.  Output those entries that
are predicable and not homographs."
  (let ((fd (fopen infile "r"))
        (ofd (fopen outfile "w"))
        (entry)
        (wordcount 0)
        (correctwords 0))
    ;; Use a new lexicon and not get confused with any installed ont
    (lex.create "g0006")
    (lex.set.compile.file infile)
    (lex.set.phoneset "radio")
    (lex.set.lts.method 'cmu_lts_function)
    (lex.select "g0006")
    (load ltsfile)
    (set! cmu_lts_rules lex_lts_rules)

    (while (not (equal? (set! entry (readfp fd)) (eof-val)))
           (if (consp entry)
               (let ((lts (cmu_lts_function (car entry) (car (cdr entry))))
                     (lexentries (lex.lookup_all (car entry))))
                 (if (or (> (length lexentries) 1)
                         (not (equal? (nth 2 entry) (nth 2 lts))))
                     (begin
                       (format ofd "( \"%s\" %l (" (car entry) (cadr entry))
                       (mapcar
                        (lambda (syl)
                          (mapcar
                           (lambda (seg)
                             (cond
                              ((string-matches seg "[aeiouAEIOU@].*")
                               (format ofd "%s " (string-append seg (cadr syl))))
                              (t
                               (format ofd "%s " seg))))
                           (car syl)))
                        (car (cddr entry)))
                       (format ofd "))\n"))
                     (set! correctwords (+ 1 correctwords)))
                 (set! wordcount (+ 1 wordcount))
                 )
           )
           )
    (format t "From %d words %d correct %d exceptions\n"
            wordcount correctwords (- wordcount correctwords))
    )
)  

(define (utf8entries infile outfile)
  (let (ifd ofd entry)
    (set! ifd (fopen infile "r"))
    (set! ofd (fopen outfile "w"))
    (while (not (equal? (set! entry (readfp ifd)) (eof-val)))
      (mapcar
       (lambda (l) (format ofd "%s " l))
       (utf8explode entry))
      (format ofd "\n"))
    (fclose ifd)
    (fclose ifd)))

(provide 'make_lex)

    

  
