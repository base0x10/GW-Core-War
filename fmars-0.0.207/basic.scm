(use-modules
  (srfi srfi-1)
  (srfi srfi-13))

(load "misc.scm")
(load "defs.scm")
(load "config.scm")


(define real-types
  '((coreptr . ("*" "fmars_insn_t"))
    (short   . (" " "unsigned short"))
    (int     . (" " "unsigned int"))
    (long    . (" " "unsigned long"))
    (llong   . (" " "unsigned long long"))
    (ptr     . ("*" "void"))
    (v2hi    . (""  "v2si_t"))
    (v4hi    . (""  "v4hi_t"))))


(define (error . msgs)
  (let ((port (current-error-port)))
    (for-each (cut display <> port) (map expr->string msgs))
    (newline port)
    (exit)))


(define-macro (when/null condition . consequence)
  `(if ,condition (st ,@consequence) null-st))


(define-macro (unless/null condition . alternative)
  `(if ,condition null-st (st ,@alternative)))


(define-macro (awhen/null condition . consequence)
  `(aif ,condition (st ,@consequence) null-st))


(define-macro (aunless/null condition . alternative)
  `(aif ,condition null-st (st ,@alternative)))


(define (st . body)
  (string-concatenate (map expr->string body)))


(define null-st "")
(define null-st? string-null?)


(define (expression tag precedence s)
  (cons* 'expression precedence tag s))


(define (expr-format fmt . body)
  (apply format #f fmt (map expr->string body)))


(define (expr->string expr)

  (define expr-tag  caddr)
  (define expr-body cdddr)
  (define (expr-precedence expr)
    (if (consed-with? 'expression expr) (cadr expr) 0))

  (define (no-parentheses? parent child)
    (or (< (expr-precedence child)
           (expr-precedence parent))
        (eq? (expr-tag child)
             (expr-tag parent))))
  
  (cond
    ((consed-with? 'register expr)
     (expr->string (cddr expr)))
    ((consed-with? 'expression expr)
     (string-concatenate
       (interleave (map (lambda (x)
                          (if (no-parentheses? expr x)
                            (st x)
                            (st "(" x ")")))
                        (expr-body expr))
                   (expr-tag expr))))
    (else (any->string expr))))


(define (infix-operator tag precedence)
  (lambda (. body)
    (expression tag precedence body)))


(define %_   (infix-operator "%"  3))
(define *_   (infix-operator "*"  3))
(define /_   (infix-operator "/"  3))
(define add_ (infix-operator "+"  4))
(define >=_  (infix-operator ">=" 6))
(define <_   (infix-operator "<"  6))
;(define xor_ (infix-operator "^"  10)) ; 9
(define bor_ (infix-operator "|"  10))
(define and_ (infix-operator "&&" 11))
(define or_  (infix-operator "||" 12))

; TODO FIXME - assumes sizeof(void*) == sizeof(int)

(define (xor_ a b)
  ((infix-operator "^" 10)
   (if (eq? (reg-type a) 'ptr) (st "(int)" a "") a)
   (if (eq? (reg-type b) 'ptr) (st "(int)" b "") b)))


(define (not_ pred)
  (expression "!" 2 (list null-st pred)))


(define (==_ left right)
  (apply (infix-operator "==" 7)
         (vect-cmp-adjust left right)))


(define (!=_ left right)
  (apply (infix-operator "!=" 7)
         (vect-cmp-adjust left right)))


(define (vect-cmp-adjust left right)

  (define (vect-cmp val fmt)
    (list (expr-format fmt left right) val))

  (case (reg-type left right)
    ((v4hi v2si)
     (vect-cmp 255 "__builtin_ia32_pmovmskb ((v8qi_t) __builtin_ia32_pcmpeqd (~a, ~a))"))
    ((v4sf)
     (vect-cmp 15 "__builtin_ia32_movmskps ((v4sf_t) __builtin_ia32_cmpeqps (~a, ~a))"))
    (else
     (list left right))))


(define (define_ dest src)
  (st "\n#define " dest " " src "\n"))


(define (undef_ dest)
  (st "\n#ifdef " dest "\n#undef " dest "\n#endif\n"))


(define (set_ dest src)
  (cond
;    ((string-prefix? "whole_insn" (expr->string dest))
;     (movq->mem src dest))
;    ((string-prefix? "field_pair" (expr->string dest))
;     (movl->mem src dest))
    (else (st dest "=" src ";"))))


(define (add-ptr src dest)
  (st dest "+=" src ";"))


(define debug-coresum+queuesum
    "printf (\" c %lx p %lx\\n\", coresum (mars), queuesum (mars, w));")


(define real-check-die
  (st "if (" (if (conf 'alter 'spl)
               "w->tail == w->head"
               "!--w->nprocs") ")
      {
      *death_tab++ = w->id;
      if (--alive_cnt < 2)"
      (if (> 2 (conf 'debug 'level))
        "goto out;"
        (st "{"
                 debug-coresum+queuesum
                "goto out;
             }"))
      "   w->prev->next = w->next;
      w->next->prev = w->prev;
      cycles -= cycles / alive_cnt;
      }"))


(define check-die
  (if (zero? (conf 'debug 'level))
    "goto dat_f_direct_direct;"
    real-check-die))


(define (next ptr)
  (cons 'next ptr))


(define (push ptr)
  (if (consed-with? 'next ptr)
    (st (incm-ptr (cdr ptr))
        (push (cdr ptr)))
    (st "*w->tail =" ptr ";"
        (if (conf 'infinite 'queue)
          "++w->tail;"
          "if (++w->tail == queue_end)
              w->tail = queue_start;"))))


(define (inc-ptr ptr)
  (st "++" ptr ";"))


(define (incm-ptr ptr)
  (st "if (++" ptr "== core_end)" ptr "= core;"))

(define (incm-safe-ptr ptr)
  (st (inc-ptr ptr)
      (fold-ptr ptr)))


(define (fold-ptr ptr)
  (if (conf 'fold 'ptr 'cmov)
    (st "{"
        (if (conf 'pointer64bit) "long long " "long ")
        ptr "2 = ("
        (if (conf 'pointer64bit) "long long" "long")
        ")" ptr "+ coresize8;
            if (" ptr ">= core_end)" ptr "= (fmars_insn_t *)" ptr "2;
         };")
    (st "if (" ptr ">= core_end)" ptr "-= coresize;")))


(define (fold-ptr-core-end ptr)
  (st "if (" ptr "== core_end)" ptr "= core;"))


(define (addm-ptr src dest)
  (st (add-ptr src dest)
      (fold-ptr dest)))


(define (addm src dest)
  (st dest "+=" src ";
      if (" dest ">=" coresize ")" dest "-=" coresize ";"))


(define (subm src dest)
  (st dest "-=" src ";
      if (" dest ">= coresize)" dest "+= coresize;"))


(define (incm val)
  (st "if (++" val "== coresize)" val "= 0;"))


(define (decm val)
  (st "if (" val "== 0)" val "= coresize;
       --" val ";"))


(define (decm-core fld ptr)
  (decm (core fld ptr)))


(define (incm-core fld ptr)
  (incm (core fld ptr)))


(define (decm-core-pair ptr)
  (if (conf 'mmx 'decm 'core 'pair)
    (st "{
        v4hi_t fpair, mask;"
        
        (set_ fpair (core w ptr)) ;add -1,-1, mask from fpair
        (paddw vectc1c1 fpair)
        (set_ mask fpair)
        (pcmpgtw vectc1c1 mask)
        (pand vectcc mask)
        (psubw mask fpair)

        (when/null (eq? 'in (car (conf 'insn 'layout)))
          (psrlq 32 fpair))
        (movd->mem fpair (core w ptb))
        "};")
    (st (decm-core a ptr)
        (decm-core b ptr))))


(define (core field index)
  (let ((regval (if (conf 'union)
                  (case field
                    ((a b) (st index "->ab[" (conf 'offset field) "]"))
                    ((f)   (st index "->f["  (conf 'offset field) "]"))
                    ((i)   (st index "->i["  (conf 'offset field) "]"))
                    ((w)   (st index "->w")))
                  (case field
                    ((w) (st "whole_insn (" index ")"))
                    ((f) (st "field_pair (" index ")"))
                    (else (st index "->" field))))))
    (register regval field)))


(define (if_ condition consequence . body)

  (define alternative
    (unless/null (null? body) (car body)))

  (define (brace s)
    (if (or (> (string-count s #\;) 1)
            (string-prefix? "if (" s))
      (st "{" s "}")
      s))

  (st "if (" condition ")"
      (brace consequence)
      (unless/null (null-st? alternative) "else ")
      (brace alternative)))
