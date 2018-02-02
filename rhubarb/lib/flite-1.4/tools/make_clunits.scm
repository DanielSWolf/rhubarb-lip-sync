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
;;;               Date: April 2001                                      ;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;                                                                     ;;;
;;; Convert festvox voice to flite                                      ;;;
;;;                                                                     ;;;
;;;   clunits: catalogue, carts and param                               ;;;
;;;                                                                     ;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(defvar lpc_min -2.709040)
(defvar lpc_max 2.328840)
(defvar mcep_min -5.404620)
(defvar mcep_max 4.540220)

(define (clunits_convert name clcatfnfileordered clcatfnunitordered 
			 cltreesfn festvoxdir odir)
  "(clunits_convert name clcatfn clcatfnordered cltreesfn festvoxdir odir)
Convert a festvox clunits (processed) voice into a C file."
   (clunitstoC clcatfnfileordered clcatfnunitordered name 
	       (path-append festvoxdir "sts")
	       (path-append festvoxdir "mcep")
	       odir)

   (set! ofd (fopen (path-append odir (string-append name "_clunits.c")) "a"))

   (clunits_make_carts cltreesfn clcatfnunitordered name odir ofd)

   (format ofd "\n\n")
   (format ofd "static const int %s_join_weights[] = \n" name)
   (format ofd " { 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768,\n")
   (format ofd "   32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768 };\n\n")

   (format ofd "extern const cst_cart * const %s_carts[];\n" name )
   (format ofd "extern cst_sts_list %s_sts, %s_mcep;\n\n" name name )

   (format ofd "cst_clunit_db %s_db = {\n" name)
   (format ofd "  \"%s\",\n\n" name)
   (format ofd "  %s_unit_types,\n" name)
   (format ofd "  %s_carts,\n" name)
   (format ofd "  %s_units,\n\n" name)

   (format ofd "  %s_num_unit_types,\n" name)
   (format ofd "  %s_num_units,\n\n" name)

   (format ofd "  &%s_sts,\n\n" name)

   (format ofd "  &%s_mcep,\n\n" name)

   (format ofd "  %s_join_weights,\n\n" name)
   (format ofd "  1, /* optimal coupling */\n")
   (format ofd "  5, /* extend selections */\n")
   (format ofd "  100, /* f0 weight */\n")
   (format ofd "  0  /* unit_name function */\n")
   
   (format ofd "};\n")

   (fclose ofd)

   ;; Duration model
   (clunits_convert_durmodel  
    (format nil "%s/festvox/%s_durdata.scm" festvoxdir name)
    name odir)
)

(define (unit_type u)
   (apply
    string-append
    (reverse
     (symbolexplode 
      (string-after 
       (apply
        string-append
        (reverse (symbolexplode u)))
       "_")))))

(define (unit_occur u)
   (apply
    string-append
    (reverse
     (symbolexplode 
      (string-before
       (apply
        string-append
        (reverse (symbolexplode u)))
       "_")))))

(define (sort_clentries entries clcatfnunitorder)
  (let ((neworder nil))
    (mapcar
     (lambda (unit)
       (set! neworder 
             (cons (assoc (car unit) entries)
                   neworder)))
     (load clcatfnunitorder t))
    (reverse neworder))
  )

(define (clunitstoC clcatfnfileordered clcatfnunitordered 
		    name stsdir mcepdir odir)
  "(clunitstoC clcatfnfileordered clcatfnunitordered name mcepdir stsdir odir)
Convert a clunits catalogue and its sts representations into a
compilable single C file."
  (let 
    ((clindex (load clcatfnfileordered t))
     (lofdidx (fopen (path-append odir (string-append name "_lpc.c")) "w"))
     (lofdh (fopen (path-append odir (string-append name "_lpc.h")) "w"))
     (cofdidx (fopen (path-append odir (string-append name "_clunits.c")) "w"))
     (cofdh (fopen (path-append odir (string-append name "_clunits.h")) "w"))
     (mofdidx (fopen (path-append odir (string-append name "_mcep.c")) "w"))
     (mofdh (fopen (path-append odir (string-append name "_mcep.h")) "w")))

    (format lofdidx "/*****************************************************/\n")
    (format lofdidx "/**  Autogenerated Clunits index for %s    */\n" name)
    (format lofdidx "/*****************************************************/\n")
    (format lofdidx "\n")
    (format lofdidx "#include \"cst_clunits.h\"\n")
    (format lofdidx "#include \"%s_lpc.h\"\n" name)
    (format mofdidx "/*****************************************************/\n")
    (format mofdidx "/**  Autogenerated Clunits index for %s    */\n" name)
    (format mofdidx "/*****************************************************/\n")
    (format mofdidx "\n")
    (format mofdidx "#include \"%s_mcep.h\"\n" name)
    (format mofdidx "#include \"cst_clunits.h\"\n")

    (set! pm_pos 0)
    (set! sample_pos 0)
    (set! times nil)
    (set! clunits_entries nil)
    (set! done_files nil)
    (set! num_unit_entries (length clindex))
    (set! residual_sizes nil)

    (set! lpc_info nil)
    (set! mcep_info nil)

    (set! n 500)
    (set! f 0)
    (while clindex
     (if (equal? n 500)
	 (begin
	   (if (> f 0)
	       (begin 
                 (format lofdbitlpc "0};\n\n")
                 (fclose lofdbitlpc) 
                 (format lofdbitres "0};\n\n")
                 (fclose lofdbitres) 
                 (format mofdbitmcep "0};\n\n")
                 (fclose mofdbitmcep)
                 ))
	   (set! lofdbitlpc
		 (fopen (format nil "%s/%s_lpc_%03d.c" odir name f) "w"))
           
           (format lofdbitlpc 
                   "const unsigned short %s_sts_lpc_page_%d[] = { \n" 
                   name f )
           (format lofdh "extern const unsigned short %s_sts_lpc_page_%d[];\n" 
                   name f )

	   (set! lofdbitres
		 (fopen (format nil "%s/%s_res_%03d.c" odir name f) "w"))
           (format lofdbitres 
                   "const unsigned char %s_sts_res_page_%d[] = { \n" 
                   name f )
           (format lofdh "extern const unsigned char %s_sts_res_page_%d[];\n" 
                   name f )

	   (set! mofdbitmcep 
		 (fopen (format nil "%s/%s_mcep_%03d.c" odir name f) "w"))
           (format mofdbitmcep
                   "const unsigned short %s_sts_mcep_page_%d[] = {\n" 
                   name f )
           (format mofdh "extern const unsigned short %s_sts_mcep_page_%d[];\n" 
                   name f )
	   (set! n 0)
           (set! lpc_page f)
           (set! mcep_page f)
           (set! lpc_page_pos 0)
           (set! res_page_pos 0)
           (set! mcep_page_pos 0)
	   (set! f (+ 1 f))))
     (set! n (+ 1 n))
     (set! pms (find_pm_pos 
		name
		(car clindex)
		stsdir
		mcepdir
		lofdbitlpc
		lofdbitres
		mofdbitmcep
		))

     ;; Output unit_entry for this unit
     (set! clunits_entries
	   (cons
	    (list 
	     (nth 0 (car clindex))
	     (nth 2 pms) ; start_pm
	     (nth 3 pms) ; phone_boundary_pm
	     (nth 4 pms) ; end_pm
	     (nth 5 (car clindex))
	     (nth 6 (car clindex))
	     )
	    clunits_entries))
     (set! clindex (cdr clindex)))

    (set! clunits_entries (sort_clentries clunits_entries clcatfnunitordered))

    (format lofdidx "\n\n")
    (format mofdidx "\n\n")

    (begin ;; Close of last pages
      (format lofdbitlpc "0};\n\n")
      (fclose lofdbitlpc) 
      (format lofdbitres "0};\n\n")
      (fclose lofdbitres) 
      (format mofdbitmcep "0};\n\n")
      (fclose mofdbitmcep)
      )

    (format lofdidx "const cst_sts_paged %s_sts_paged_vals[] = { \n" name)
    (set! i 0)
    (mapcar
     (lambda (info)
       (format lofdidx "  {%d,%d,%d,%s_sts_lpc_page_%d,%s_sts_res_page_%d},\n"
               (nth 1 info) (nth 2 info) (nth 3 info)
               name (nth 0 info)
               name (nth 0 info))
       (set! i (+ 1 i)))
     (reverse lpc_info))
    (format lofdidx "   { 0, 0, 0, 0, 0 }};\n\n")
    (format lofdidx "const cst_sts_list %s_sts = {\n" name)
    (format lofdidx "  NULL, %s_sts_paged_vals,\n" name)
    (format lofdidx "  NULL, NULL, NULL,\n")
    (format lofdidx "  %d,\n" i)
    (format lofdidx "  %d,\n" lpc_order)
    (format lofdidx "  %d,\n" sample_rate)
    (format lofdidx "  %f,\n" lpc_min)
    (format lofdidx "  %f\n" lpc_range)
    (format lofdidx "};\n\n")

    (format mofdidx "const cst_sts_paged %s_mcep_paged_vals[] = { \n" name)
    (set! i 0)
    (mapcar
     (lambda (info)
       (format mofdidx "  { %d, 0, 0, %s_sts_mcep_page_%d, 0 }, \n"
               (nth 1 info) 
	       name (nth 0 info))
       (set! i (+ 1 i)))
       (reverse mcep_info))
    (format mofdidx "   { 0, 0, 0 }};\n\n")

    (format mofdidx "const cst_sts_list %s_mcep = {\n" name)
    (format mofdidx "  NULL, %s_mcep_paged_vals,\n" name)
    (format mofdidx "  NULL, NULL, NULL,\n")
    (format mofdidx "  %d,\n" i)
    (format mofdidx "  %d,\n" mcep_order)
    (format mofdidx "  %d,\n" sample_rate)
    (format mofdidx "  %f,\n" mcep_min)
    (format mofdidx "  %f\n" (- mcep_max mcep_min))
    (format mofdidx "};\n\n")

    (format cofdidx "/*****************************************************/\n")
    (format cofdidx "/**  Autogenerated Clunits index for %s    */\n" name)
    (format cofdidx "/*****************************************************/\n")
    (format cofdidx "\n")
    (format cofdidx "#include \"cst_clunits.h\"\n")
    (format cofdidx "#include \"%s_clunits.h\"\n" name)
    (format cofdidx "#include \"%s_cltrees.h\"\n" name)

    (format cofdidx "\n\n")
    (set! unitbase_count 0)
    (set! unitbases nil)
    (mapcar
     (lambda (p)
       (if (and (not (string-matches (car p) ".*#.*"))
		(not (member_string (string-before (car p) "_") unitbases)))
	   (begin
	     (format cofdidx "#define unitbase_%s %d\n" 
		     (clunits_normal_phone_name 
                      (string-before (car p) "_"))
                     unitbase_count)
	     (set! unitbases (cons (string-before (car p) "_") unitbases))
	     (set! unitbase_count (+ 1 unitbase_count)))))
     clunits_entries)
    (format cofdidx "\n\n")

    (format cofdidx "const cst_clunit %s_units[] = { \n" name)
    (set! num_entries 0)
    (set! this_ut "")
    (set! this_ut_count 0)
    (mapcar
     (lambda (e)
       (if (not (string-equal this_ut (unit_type (nth 0 e))))
	   (begin
	     (if (> this_ut_count 0)
		 (format cofdh "#define unit_%s_num %d\n"
			 (clunits_normal_phone_name this_ut) 
                         this_ut_count))
	     (format cofdh "#define unit_%s_start %d\n"
		     (clunits_normal_phone_name (unit_type (nth 0 e)))
                     num_entries)
	     (set! this_ut (unit_type (nth 0 e)))
	     (set! this_ut_count 0)
	     ))
       (format cofdh "#define unit_%s %d\n" 
               (clunits_normal_phone_name (nth 0 e)) num_entries)
       (set! num_entries (+ 1 num_entries))
       (set! this_ut_count (+ 1 this_ut_count))
       (format cofdidx "   { /* %s */ unit_type_%s, unitbase_%s, %d,%d, %s, %s },\n"
               (nth 0 e)
               (clunits_normal_phone_name (unit_type (nth 0 e)))
	       (clunits_normal_phone_name (string-before (nth 0 e) "_"))
	       (nth 1 e) ; start_pm
	       (nth 3 e) ; end_pm
	       (clunits_normal_phone_name (nth 4 e)) ; prev
	       (clunits_normal_phone_name (nth 5 e)) ; next
	       ))
     clunits_entries)
    (format cofdidx "   { 0,0,0,0 } };\n\n")
    (format cofdidx "#define %s_num_units %d\n" name num_entries)
    (if (> this_ut_count 0)
	(format cofdh "#define unit_%s_num %d\n"
		this_ut this_ut_count))
    (fclose lofdh)
    (fclose lofdidx)
    (fclose mofdh)
    (fclose mofdidx)
    (fclose cofdh)
    (fclose cofdidx)
    ))

(defvar sts_coeffs_fname nil)

(define (find_pm_pos name entry stsdir mcepdir lofdlpc lofdres mofdmcep)
  "(find_pm_pos entry lpddir)
Diphone dics give times in seconds here we want them as indexes.  This
function converts the lpc to ascii and finds the pitch marks that
go with this unit.  These are written to ofdsts with ulaw residual
as short term signal."
  (let ((start_time (nth 2 entry))
	(phoneboundary_time (nth 3 entry))
	(end_time (nth 4 entry))
	start_pm pb_pm end_pm
	(ltime 0))
    (format t "%l\n" entry)
    (if (not (string-equal (cadr entry) sts_coeffs_fname))
        (begin
          ;; Only load when when we have a new filename
          (set! sts_coeffs
                (load (format nil "%s/%s.sts" stsdir (cadr entry)) t))
          (set! mcep_coeffs
                (load_ascii_track
                 (format nil "%s/%s.mcep" mcepdir (cadr entry))
                 (nth 2 entry) ;; from this start time
                 ))
          (set! sts_info (car sts_coeffs))
          (set! sts_coeffs (cdr sts_coeffs))
          (set! sts_coeffs_fname (cadr entry))
          ))
    (set! ltime 0)
    (set! size_to_now 0)
    ;; Flip through the sts's and mceps to find the right one --
    (while (and sts_coeffs (cdr sts_coeffs)
	    (> (absdiff start_time (car (car sts_coeffs)))
	      (absdiff start_time (car (cadr sts_coeffs)))))
     (set! ltime (car (car sts_coeffs)))
     (set! mcep_coeffs (cdr mcep_coeffs))
     (set! sts_coeffs (cdr sts_coeffs)))
    (set! sample_rate (nth 2 sts_info))
    (set! lpc_order (nth 1 sts_info))
    (set! lpc_min (nth 3 sts_info))
    (set! lpc_range (nth 4 sts_info))
    (set! start_pm pm_pos)
    (while (and sts_coeffs (cdr sts_coeffs)
                mcep_coeffs (cdr mcep_coeffs)
	    (> (absdiff phoneboundary_time (car (car sts_coeffs)))
	       (absdiff phoneboundary_time (car (cadr sts_coeffs)))))
     (output_mcep name (car mcep_coeffs) mofdmcep)
     (output_sts name (car sts_coeffs) (nth 1 entry) lofdlpc lofdres)
     (set! mcep_coeffs (cdr mcep_coeffs))
     (set! sts_coeffs (cdr sts_coeffs)))
    (set! pb_pm pm_pos)
    (while (and sts_coeffs (cdr sts_coeffs)
                mcep_coeffs (cdr mcep_coeffs)
	    (> (absdiff end_time (car (car sts_coeffs)))
	       (absdiff end_time (car (cadr sts_coeffs)))))
     (output_mcep name (car mcep_coeffs) mofdmcep)
     (output_sts name (car sts_coeffs) (nth 1 entry) lofdlpc lofdres)
     (set! mcep_coeffs (cdr mcep_coeffs))
     (set! sts_coeffs (cdr sts_coeffs)))
    (set! end_pm pm_pos)

    (list 
     (car entry)
     (cadr entry)
     start_pm
     pb_pm
     end_pm)))

(define (output_sts name frame fname ofdlpc ofdres)
  "(output_sts frame residual ofd)
Ouput this LPC frame."
  (let ((time (nth 0 frame))
	(coeffs (nth 1 frame))
	(size (nth 2 frame))
	(r (nth 3 frame)))
    (set! times (cons time times))

    (set! l_n lpc_page_pos)
    (while coeffs
     (format ofdlpc " %d," (car coeffs))
     (set! coeffs (cdr coeffs)))
    (format ofdlpc "\n")
    (set! lpc_page_pos (+ 1 lpc_page_pos)) ;; we know the frame size

    (set! r_n res_page_pos)
    (while r
     (format ofdres " %d," (car r))
     (set! res_page_pos (+ 1 res_page_pos))
     (set! r (cdr r)))
    (format ofdres "\n")
    
    (set! lpc_info
          (cons
           (list lpc_page l_n size r_n)
           lpc_info))
    (set! pm_pos (+ 1 pm_pos))
))

(define (lpccoeff_norm c)
  (* (/ (- c lpc_min) (- lpc_max lpc_min))
     65535))

(define (mcepcoeff_norm c)
  (* (/ (- c mcep_min) (- mcep_max mcep_min))
     65535))

(define (output_mcep name frame ofd)
  "(output_mcep frame duration residual ofd)
Ouput this MCEP frame."
  (let ()
    (set! mcep_order (- (length frame) 3))

    (set! frame (cddr frame)) ;; skip the "1"
    (set! frame (cdr frame)) ;; skip the energy
    (set! m_n mcep_page_pos)
    (while frame
     (format ofd " %d," (mcepcoeff_norm (car frame)))
     (set! frame (cdr frame)))
    (format ofd "\n")
    (set! mcep_page_pos (+ 1 mcep_page_pos))

    (set! mcep_info
          (cons
           (list mcep_page m_n)
           mcep_info))

))

(define (load_ascii_track trackfilename starttime)
   "(load_ascii_track trackfilename)
Coverts trackfilename to simple ascii representation."
   (let ((tmpfile (make_tmp_filename))
	 (nicestarttime (if (> starttime 0.100)
			    (- starttime 0.100)
			    starttime))
	 b)
     (system (format nil "$ESTDIR/bin/ch_track -otype est -start %f %s | 
                        awk '{if ($1 == \"EST_Header_End\")
                                 header=1;
                              else if (header == 1)
                                 printf(\"( %%s )\\n\",$0)}'>%s" 
		     nicestarttime trackfilename tmpfile))
     (set! b (load tmpfile t))
     (delete-file tmpfile)
     b))


(define (absdiff a b)
  (let ((d (- a b )))
    (if (< d 0)
	(* -1 d)
	d)))

(define (carttoC_extract_answer_list ofdh tree)
  "(carttoC_extract_answer_list tree)
Get list of answers from leaf node."
;  (carttoC_val_table ofdh 
;		     (car (last (car tree)))
;		     'none)
;  (format t "%l\n" (car tree))
  (cellstovals "cl" (mapcar car (caar tree)) ofdh)
  (format nil "cl_%04d" cells_count))

(define (sort_cltrees trees clcatfn)
  (let ((neworder nil) (ut nil))
    (mapcar
     (lambda (unit)
       (set! ut (unit_type (car unit)))
       (if (not (assoc_string ut neworder))
	   (set! neworder (cons (assoc_string ut trees) neworder))))
     (load clcatfn t))
    (reverse neworder)))

(define (clunits_make_carts cartfn clcatfn name odir cofd)
 "(define clunits_make_carts cartfn name)
Output clunit selection carts into odir/name_carts.c"
 (let (ofd ofdh)
 ;; Set up to dump full list of things at leafs
 (set! carttoC_extract_answer carttoC_extract_answer_list)
 (load cartfn)

 (set! ofd (fopen (format nil "%s/%s_cltrees.c" odir name) "w"))
 (set! ofdh (fopen (format nil "%s/%s_cltrees.h" odir name) "w"))
 (format ofd "\n")
 (format ofd "#include \"cst_string.h\"\n")
 (format ofd "#include \"cst_cart.h\"\n")
 (format ofd "#include \"cst_regex.h\"\n")
 (format ofd "#include \"%s_cltrees.h\"\n" name)

 (set! val_table nil)

 (set! clunits_selection_trees (sort_cltrees clunits_selection_trees clcatfn))

 (mapcar
  (lambda (cart)
    (set! current_node -1)
    (set! feat_nums nil)
    (do_carttoC ofd ofdh 
		(format nil "%s_%s" name 
                        (clunits_normal_phone_name (car cart)))
		(cadr cart)))
  clunits_selection_trees)
 
 (format ofd "\n\n")
 (format ofd "const cst_cart * const %s_carts[] = {\n" name)
 (mapcar
  (lambda (cart)
    (format ofd " &%s_%s_cart,\n" name 
            (clunits_normal_phone_name (car cart)))
    )
  clunits_selection_trees)
 (format ofd " 0 };\n")

 (format cofd "\n\n")
 (format cofd "#define %s_num_unit_types %d\n" 
	 name (length clunits_selection_trees))

 (format cofd "\n\n")
 (format cofd "const cst_clunit_type %s_unit_types[] = {\n" name)
 (set! n 0)
 (mapcar
  (lambda (cart)
    (format ofdh "#define unit_type_%s %d\n" 
	    (clunits_normal_phone_name (car cart)) n)
    (format cofd "  { \"%s\", unit_%s_start, unit_%s_num},\n" 
	    (car cart) 
            (clunits_normal_phone_name (car cart) )
            (clunits_normal_phone_name (car cart)) )
    (set! n (+ 1 n))
    )
  clunits_selection_trees)
 (format cofd "  { NULL, CLUNIT_NONE, CLUNIT_NONE } };\n")

 (fclose ofd)
 (fclose ofdh)

 )
)

(define (clunits_convert_durmodel durmodelfn name odir)

  (set! durmodel (load durmodelfn t))
  (set! phonedurs (cdr (cadr (car (cddr (car durmodel))))))
  (set! zdurtree (cadr (car (cddr (cadr durmodel)))))

  (set! carttoC_extract_answer basic_carttoC_extract_answer)

  (set! dfd (fopen (path-append odir (string-append name "_cl_durmodel.c")) "w"))
  (set! dfdh (fopen (path-append odir (string-append name "_cl_durmodel.h")) "w"))
  (format dfd "/*****************************************************/\n")
  (format dfd "/**  Autogenerated durmodel_cl for %s    */\n" name)
  (format dfd "/*****************************************************/\n")

  (format dfd "#include \"cst_synth.h\"\n")
  (format dfd "#include \"cst_string.h\"\n")
  (format dfd "#include \"cst_cart.h\"\n")
  (format dfd "#include \"%s_cl_durmodel.h\"\n\n" name)

  (mapcar
   (lambda (s)
     (format dfd "static const dur_stat dur_state_%s = { \"%s\", %f, %f };\n"
             (clunits_normal_phone_name (car s)) 
             (car s) (car (cdr s)) (car (cddr s)))
     )
   phonedurs)
  (format dfd "\n")

  (format dfd "const dur_stat * const %s_dur_stats[] = {\n" name)
  (mapcar
   (lambda (s)
     (format dfd "   &dur_state_%s,\n" (clunits_normal_phone_name (car s))))
   phonedurs)  
  (format dfd "   NULL\n};\n")

  (set! val_table nil)
  (set! current_node -1)
  (set! feat_nums nil)
  (do_carttoC dfd dfdh 
              (format nil "%s_%s" name "dur")
              zdurtree)

  (fclose dfd)
  (fclose dfdh)
)

(define (clunits_normal_phone_name x)
  ;; Some phonenames aren't valid C labels
  (cond
   ((string-matches x ".*@.*" x) 
    (intern
     (string-append
      (string-before x "@")
      "atsign"
      (string-after x "@"))))
   ((string-matches x ".*:.*")
    (intern
     (string-append
      (string-before x ":")
      "sc"
      (string-after x ":"))))
   (t x)))

(provide 'make_clunits)

