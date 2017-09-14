(use-modules
  (srfi srfi-1))

(load "basic.scm")


(define imm    (cut cons 'imm <>))
(define in     (cut cons* 'in <> <>))
(define in/out (cut cons* 'in/out <> <>))
(define out    (cut cons* 'out <> <>))


(define (arg-wrap arg)
  (st "\""
      (case (car arg)
        ((in/out) "+")
        ((out)    "=")
        ((in)     "")
        (else (error "bad constraint in asm instruction")))
      (case (cadr arg)
        ((mem)     "m")
        ((mmx)     "y")
        ((mem/mmx) "my")
        ((reg/mmx) "ry")
        (else (error "bad constraint in asm instruction")))
      "\" ("
      (cddr arg)
      ")"))


(define (asm insn . args)
  (let* ((no-imm  (remove (lambda (x) (eq? (car x) 'imm)) args))
         (input   (filter (lambda (x) (eq? (car x) 'in)) no-imm))
         (output  (remove (lambda (x) (eq? (car x) 'in)) no-imm))
         (ordered (append output input)))

  (st "asm (\""
      insn
      " "
      (apply st
             (interleave (map (lambda (x)
                                (if (eq? (car x) 'imm)
                                  (st "$" (cdr x))
                                  (st "%" (list-index (cut eq? x <>)
                                                      ordered))))
                              args)
                         ","))
      "\":"
      (apply st (interleave (map arg-wrap output) ","))
      (unless/null (null? input)
        ":"
        (apply st (interleave (map arg-wrap input) ",")))
      ");")))


(define (mmx-arith insn)
  (lambda (src dst)
    (asm insn
         (in     'mem/mmx src)
         (in/out 'mmx dst))))


(define (store insn)
  (lambda (src dst)
    (asm insn
         (in  'reg/mmx src)
         (out 'mem dst))))


(define (roll perm var)
  (asm 'roll
       (imm    perm)
       (in/out 'mem var)))


(define (pshufw->mmx perm src dst)
  (asm 'pshufw
       (imm     perm)
       (in     'mem src)
       (out    'mmx dst)))


(define (psrlq perm var)
  (asm 'psrlq
       (imm    perm)
       (in/out 'mmx var)))

(define (movd->mmx src dst)
  (asm 'movd
       (in  'mem src)
       (out 'mmx dst)))

(define movd->mem (store 'movd))
(define movq->mem (store 'movq))
(define movl->mem (store 'movl))

(define pcmpgtw (mmx-arith 'pcmpgtw))
(define pand    (mmx-arith 'pand))
(define pxor    (mmx-arith 'pxor))
(define paddw   (mmx-arith 'paddw))
(define psubw   (mmx-arith 'psubw))

(define (save-mmx-reg reg mem)
  (format #f "asm volatile (\"movq %%~a, %0\" : \"=m\" (~a));" reg mem))
(define (restore-mmx-reg mem reg)
  (format #f "asm volatile (\"movq %0, %%~a\" : : \"m\" (~a));" reg mem))
(define emms "asm volatile (\"emms\");")
