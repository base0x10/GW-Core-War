(use-modules
  (srfi srfi-1)
  (srfi srfi-8))

(load "basic.scm")
(load "misc.scm")
(load "defs.scm")
(load "config.scm")
(load "asm.scm")

; real-type -> ((star reg) (star reg) ...)

(define (decl-vars regs)

  (define (real-type reg)
    (let ((rt (assoc (reg-type reg) real-types)))
      (list (caddr rt) (cadr rt) reg)))

  (define (group-by-car lst)
    (let loop ((rest (sort lst (lambda (x y) (string> (car x) (car y)))))
               (acc  '()))
      (if (null? rest)
        acc
        (receive (fst snd)
                 (span (lambda (x)
                         (equal? (car x) (caar rest)))
                       rest)
                 (loop snd (cons
                             (cons (caar rest) (map cdr fst))
                             acc))))))

  (apply st
         (map (lambda (lst)
                (st (car lst)
                    " "
                    (apply st (interleave (map (cut apply st <>) (cdr lst))
                                          ","))
                    ";\n"))
              (group-by-car (map real-type regs)))))


(define (cea ma mb . nested-regs+flags)

  (define (need-calc? mode regs)
    (or (member mode '(apredec apostinc bpredec bpostinc))
        (not (null? regs))))

  (let* ((args         (fringe nested-regs+flags))
         (req-a-fields (lset-intersection eq? args a-registers))
         (req-b-fields (lset-intersection eq? args b-registers))
         (req-a-flags  (lset-intersection eq? args a-flags))
         (req-b-flags  (lset-intersection eq? args b-flags))
         (extra-fields (lset-intersection eq? args extra-registers))

         (need-calc-a  (need-calc? ma req-a-fields))
         (need-calc-b  (need-calc? mb req-b-fields))
         (precalc-b    (conf 'cea 'precalc 'ptb)))

    (st (decl-vars (lset-union eq? req-a-fields req-b-fields extra-fields
                                   (filter identity
                                           (list (and need-calc-a pta)
                                                 (and need-calc-b ptb)
                                                 (and need-calc-b
                                                      (not precalc-b)
                                                      (not (eq? immediate mb))
                                                      irb)))))
        
        (when/null (and (need-calc? mb req-b-fields)
                        (not (eq? mb immediate)))
          (if precalc-b
            (set_ ptb (add_ ip (core b ip)))
            (set_ irb (core b ip))))

        (when/null (need-calc? ma req-a-fields)
          (if (eq? ma immediate)
            (set_ pta ip)
            (set_ pta (add_ ip (core a ip))))
          (cea-field ma req-a-fields req-a-flags pta))

        (when/null (need-calc? mb req-b-fields)
          (if (eq? mb immediate)
            (set_ ptb ip)
            (unless/null precalc-b
              (set_ ptb (add_ ip irb))))
          (cea-field mb req-b-fields req-b-flags ptb)))))


(define (set-regs ptr fields flags)
  
  (define (set-single-reg reg)
    (st (if (and (eq? reg raw)
                 (memq ra-swap-ab flags))
          (pshufw->mmx (if (eq? i (car (conf 'insn 'layout))) 176 1)
                           (core w ptr)
                           reg)
          (set_ reg (core (field reg) ptr)))
        (when/null (and (eq? reg raf)
                        (memq ra-swap-ab flags))
          (cond
            ((eq? (conf 'type a) 'short)
             (if (conf 'asm 'rol)
               (roll 16 raf)
               "raf = (raf << 16) | (raf >> 16);"))
            ((eq? (conf 'type f) 'v2si)
             "mmx shuf/swap;")
            (else
             "raf = (raf << 32) | (raf >> 32);")))))
             
  (apply st (map set-single-reg
                 (lset-difference eq? fields '(pta ptb)))))


(define (cea-field mode fields flags p)

  (let* ((a/b            (if (memq mode '(aindir apostinc apredec)) a b))
         (unfold-last    (memq pta-part flags))
         (final-addm-ptr (if unfold-last add-ptr addm-ptr)))

    (case mode
      ((immediate)
       (set-regs ip fields flags))

      ((direct)
       (st (unless/null unfold-last
             (fold-ptr p))
           (set-regs p fields flags)))

      ((aindir bindir)
       (st (fold-ptr p)
           (final-addm-ptr (core a/b p) p)
           (set-regs p fields flags)))
      
      ((apostinc bpostinc)
       (st (fold-ptr p)
           (if (null? fields)
             (incm-core a/b p)
             (if (conf 'postinc 'in 'register)
               (st "{
                       fmars_field_t *ftmp = &" (core a/b p) ";
                       int tmp =" (core a/b p) ";"
                       (final-addm-ptr (core a/b p) p)
                       (set-regs p fields flags)
                       "if (++tmp == coresize)
                           tmp = zero;"
                       (set_ "*ftmp" tmp)
                   "};")
               (st "{
                       fmars_field_t *ftmp = &" (core a/b p) ";"
                       (final-addm-ptr (core a/b p) p)
                       (set-regs p fields flags)
                       (incm "*ftmp")
                   "};")))))
       
       ((apredec bpredec)
        (st (fold-ptr p)
            (if (null? fields)
              (decm-core a/b p)
              (st (if (conf 'predec 'in 'register)
                    (st "{
                            int tmp =" (core a/b p) ";
                            if (--tmp < 0)
                               tmp = coresize1;"
                           (set_ (core a/b p) tmp)
                           (final-addm-ptr tmp p)
                        "};")
                    (st (decm-core a/b p)
                        (final-addm-ptr (core a/b p) p)))
                  (set-regs p fields flags)))))

       (else (error "unsupported addressing mode: " mode)))))


(define (iter-apply-collect collector func lst)
  (apply collector (map (cut apply func <>) lst)))


(define (make-dat insn mod ma mb)
  (st (cea ma mb)
      (if (and (conf 'dat 'shortcut)
               (not (and (eq? mod f) (eq? ma direct) (eq? mb direct))))
        check-die
        real-check-die)))


(define (make-spl insn mod ma mb)
  (when (and (conf 'alter spl)
             (not (conf 'infinite 'queue)))
    (error "alter spl implies infinite queue"))
  (st (cea ma mb pta)
      (push (next ip))
      (if (conf 'alter 'spl)
        (st "*w->tail = pta;"
            (if_ "w->tail - w->head < processes"
                 "w->tail++;"))
        (if_ (if (conf 'rfrenzy22 'hack)
               "w->nprocs < w->maxproc"
               "w->nprocs < processes")
             (st (push pta)
                 "w->nprocs++;")))))


(define (make-nop insn mod ma mb)
  (st (cea ma mb)
      (push (next ip))))


(define (make-jmp insn mod ma mb)
  (st (cea ma mb pta)
      (push pta)))


(define (arith-fields mod combine)
  (cond
    ((memq mod '(a ab b ba))
     (list (if (memq mod '(a ab)) raa rab)
           (if (memq mod '(a ba)) rba rbb)))
    (combine
      (if (eq? (conf 'type a) 'short) (list raw rbw) (list raf rbf)))
    ((eq? mod x)
     (append (arith-fields ab #f) (arith-fields ba #f)))
    (else
     (append (arith-fields a #f) (arith-fields b #f)))))


(define (make-add/sub insn mod ma mb)
  (let* ((mod     (if (eq? mod i) f mod))
         (combine (and (memq mod '(f x)) (conf insn mod 'combine)))
         (fields  (group (arith-fields mod combine) 2))
         (flags   (and combine (eq? mod x) ra-swap-ab))
         (extrfld (and combine mask))
         (arfunc  (if (eq? insn add) addm subm)))

    (st (cea ma mb ptb fields extrfld flags)
        (if combine
          (if (eq? insn add)
            (st (paddw rbw raw)
                (set_ mask raw)
                (pcmpgtw vectc1c1 mask)
                (pand vectcc mask)
                (psubw mask raw)
                (when/null (eq? 'in (car (conf 'insn 'layout)))
                  (psrlq 32 raw))
                (movd->mem raw (core w ptb)))
            (st (psubw raw rbw)
                (set_ mask vect00)
                (pcmpgtw rbw mask)
                (pand vectcc mask)
                (paddw mask rbw)
                (when/null (eq? 'in (car (conf 'insn 'layout)))
                  (psrlq 32 rbw))
                (movd->mem rbw (core w ptb))))
          (iter-apply-collect st
                              (lambda (src dst)
                                (st (arfunc src dst)
                                    (set_ (core (field dst) ptb) dst)))
                              fields))
        (push (next ip)))))


(define (make-mul insn mod ma mb)
  (let ((fields (group (arith-fields (if (eq? mod i) f mod) #f) 2)))
    (st (cea ma mb ptb fields)
        (iter-apply-collect st
                            (lambda (src dst)
                              (set_ (core (field dst) ptb)
                                    (%_ (*_ src dst) coresize)))
                            fields)
        (push (next ip)))))


(define (make-div/mod insn mod ma mb)
  (let ((fields (group (arith-fields (if (eq? mod i) f mod) #f) 2))
        (arfunc (if (eq? insn div) /_ %_)))
    (st (cea ma mb ptb fields)
        (iter-apply-collect st
                            (lambda (src dst)
                              (if_ dst
                                   (set_ (core (field dst) ptb)
                                         (arfunc src dst))))
                            fields)
        (if_ (apply or_ (map not_ (map cadr fields)))
             check-die
             (push (next ip))))))


(define (mov-fields mod)
  (if (and (memq mod '(f x))
           (conf mov mod 'combine))
    (list raf f)
    (case mod
      ((a ab b ba) (list (if (memq mod '(a ab)) raa rab)
                         (if (memq mod '(a ba)) a b)))
      ((i)         (if (conf mov i 'combine)
                     (list raw w)
                     (append (list rai i) (mov-fields f))))
      ((f)         (append (mov-fields a) (mov-fields b)))
      ((x)         (append (mov-fields ab) (mov-fields ba))))))
  

(define (make-mov insn mod ma mb)
  (let ((fields (group (mov-fields mod) 2))
        (flags  (and (eq? mod x) (conf mov x 'combine) ra-swap-ab)))
    (st (cea ma mb ptb fields flags)
        (iter-apply-collect st
                            (lambda (src dst-field)
                              (set_ (core dst-field ptb) src))
                            fields)
        (push (next ip)))))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; passed - pointer to jump to, if the condition (rba || rbb) is passed
;                                            or (rba != 1 || rbb != 1)
; default - pta
; failed - when failed; passed, failed <- (pta, (next ip))
; when no cmov, then 

(define (make-jmn/jmz/djn insn mod ma mb)
  (let* ((mod     (if (memq mod '(x i)) f mod))
         (single  (or (not (eq? mod f)) (conf insn f 'combine)))

         (confmod (and (eq? mod f) mod)) ; empty field for single fields - false value is filtered out
         (complex (conf insn confmod 'complex))
         (partial (conf insn confmod 'partial))
         (cmov    (conf insn confmod 'cmov))
         (negate  (conf insn confmod 'negate))

         (swap    (xor negate (eq? insn jmz)))
         (nextip  (if cmov ip (next ip))) ; for cmov, we will increment ip before if
         (passed  (if swap nextip pta))
         (failed  (if swap pta nextip))

         (one     (aif (and single (eq? mod f) (conf 'djn f 'combine)) it 1))
         (neq_    (if (conf insn confmod 'xor) xor_ !=_))
         (or2_    (if (conf insn confmod 'bitor) bor_ or_))

         (true    (if (eq? insn djn) (cut neq_ <> one) identity)) ; used for negation
         (false   (if (eq? insn djn) (cut ==_ <> one) not_))

         (fields  (cond
                    ((memq mod '(a ba)) rba)
                    ((memq mod '(b ab)) rbb)
                    (single             rbf) ; obviously it's not single, so it must be combined
                    (else               (list rbb rba)))))

    (when (and cmov negate (not complex) (not single) (eq? mod f))
      (error "cannot negate not combined/complex cmov f/x"))
    (when (and complex single (eq? mod f))
      (error "complex conflicts with combine"))
    (when (and complex (not cmov))
      (error "complex implies cmov"))
    (when (and partial (not cmov))
      (error "partial implies cmov"))
    
    (st (cea ma mb pta fields (and partial pta-part) (and (eq? insn djn) ptb))

        (when/null (eq? insn djn)
          (if (and single (not (eq? mod f)))
            (decm-core (field fields) ptb)
            (decm-core-pair ptb)))

        (cond (partial (inc-ptr ip))
              (cmov    (incm-ptr ip))
              (else    null-st))

        ; first if statement
        (if_ (cond
               (single             ((if negate false true) fields))
               ((xor cmov complex) (true rbb))
               (negate             (and_ (false rbb) (false rba)))
               (else               (or2_ (true rbb) (true rba)))) ; standard case
             (if cmov
               (set_ failed passed) ; for || one is enought to jump
               (push passed))
             (unless/null cmov
               (push failed)))

        ; second if statement, generated only for not negated f/x cmov

        (unless/null (or (not cmov) single negate complex)
          (if_ (true rba)
               (set_ failed passed)))

        (when/null partial
          (fold-ptr failed))

        (when/null cmov
          (push failed)))))
        
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;        




; default condition: raa <> rba or rab <> rbb
;                    raa >= rba or rab >= rbb

; seq: if (raa <> rba)
;          failed = passed;
;          inc-ip = ip;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; passed - where to jump if raa<>rba is passed

(define (skip-fields mod combine combine-f)
  (if (and combine (memq mod '(f x)))
    (list raf rbf)
    (case mod
      ((a b ab ba) (list (if (memq mod '(a ab)) raa rab)
                         (if (memq mod '(a ba)) rba rbb)))
      ((i)         (if combine
                     (list raw rbw)
                     (append (skip-fields f combine-f #f) (list rai rbi))))
      ((x)         (append (skip-fields ab #f #f) (skip-fields ba #f #f)))
      ((f)         (append (skip-fields a #f #f) (skip-fields b #f #f))))))


(define (make-seq/sne/slt insn mod ma mb)
  (let* ((mod       (if (and (eq? insn slt) (eq? mod i)) f mod))
         
         (combine   (conf insn mod 'combine))
         (cmov      (conf insn mod 'cmov))
         (negate    (conf insn mod 'negate))
         (fold-once (conf insn mod 'fold-once))

         (neq_    (if (conf insn mod 'xor) xor_ !=_))
         (or2_    (if (conf insn mod 'bitor) bor_ or_))

         ; these are only used for cmov
         (swap      (xor negate (eq? insn sne)))
         (passed    (if swap nextip ip))
         (failed    (if swap ip nextip))
         (cmpfunc   (if (eq? insn slt)
                      (if negate <_ >=_)
                      (if negate ==_ neq_)))

         (extrfld   (and cmov nextip))
         (flags     (and combine (eq? mod x) ra-swap-ab))
         (fields    (group (skip-fields mod combine (conf insn f 'combine))
                           2)))

    ; single + negate + cmov actually makes some sense

    (st (cea ma mb fields flags extrfld)
        (when/null cmov
          (set_ nextip ip)
          ((if fold-once inc-ptr incm-ptr) nextip))

        (if cmov
          (st (iter-apply-collect st
                                  (lambda (left right)
                                    (if_ (cmpfunc left right)
                                         (set_ failed passed))) ; for || one is enough to pass
                                  fields)
              ((if fold-once incm-safe-ptr incm-ptr) failed)
              (push failed))

          (st (if_ (case insn
                     ((seq) (iter-apply-collect and_ ==_ fields))
                     ((sne) (iter-apply-collect or2_ neq_ fields))
                     (else  (iter-apply-collect and_ <_ fields)))
                   ((if fold-once inc-ptr incm-ptr) ip))
              ((if fold-once incm-safe-ptr incm-ptr) ip)
              (push ip))))))


(define next-warrior
  (st "w = w->next;
      ip = *w->head;"
      (if (conf 'infinite 'queue)
        "++w->head;"
        "if (++w->head == queue_end)
            w->head = queue_start;")

      "if (!--cycles)
          goto out;
      goto *" (core i ip) ";"))


(define (make-insn label insn mod ma mb)
  (st label ":
      {"

      (when/null (>= (conf 'debug 'level) 2)
        (debug-disasm-line insn mod ma mb)
        "{")

      ((case insn
        ((dat)         make-dat)
        ((spl)         make-spl)
        ((nop)         make-nop)
        ((jmp)         make-jmp)
        ((add sub)     make-add/sub)
        ((mul)         make-mul)
        ((div mod)     make-div/mod)
        ((mov)         make-mov)
        ((jmn jmz djn) make-jmn/jmz/djn)
        ((seq sne slt) make-seq/sne/slt)
        (else          (error "unsupported instruction: " insn)))
       insn mod ma mb)

      (when/null (>= (conf 'debug 'level) 2)
        "}"
        debug-coresum+queuesum)

      (unless/null (and (zero? (conf 'debug 'level))
                        (conf 'dat 'shortcut)
                        (eq? insn dat)
                        (not (and (eq? mod f)
                                  (eq? ma direct)
                                  (eq? mb direct))))
        next-warrior)

      "}"

      (undef_ pta)
      (undef_ ptb)))
