(use-modules
  (srfi srfi-1)
  (srfi srfi-11)
  (srfi srfi-13))

(load "srfi-26.scm")

(define (anaphor-let keyword condition body)
  `(let ((it ,condition))
     (,keyword it ,@body)))

(define-macro (awhen condition . body)
  (anaphor-let 'when condition body))

(define-macro (aunless condition . body)
  (anaphor-let 'unless condition body))

(define-macro (aif condition . body)
  (anaphor-let 'if condition body))


(define-macro (unless condition . body)
  `(if (not ,condition) (begin ,@body)))
                                                                                    

(define-macro (when condition . body)
  `(if ,condition (begin ,@body)))


(define (fringe lst)
  (cond
    ((null? lst) lst)
    ((list? lst) (append-map fringe lst))
    (else (list lst))))


(define (consed-with? tag pair)
  (and (pair? pair)
       (equal? (car pair) tag)))


(define (interleave lst sym)
  (let loop ((rest lst) (acc '()))
    (if (null? rest)
      (reverse! (cdr acc))
      (loop (cdr rest) (cons* sym (car rest) acc)))))


;(define (safe-split-at lst cnt)
;  (do ((drop lst (cdr drop))
;       (take '() (cons (car drop) take))
;       (i    cnt (- i 1)))
;    ((or (zero? i) (null? drop))
;     (values (reverse! take) drop))))


(define (safe-split-at lst cnt)
  (let loop ((drop lst) (take '()) (i cnt))
    (if (or (zero? i) (null? drop))
      (cons (reverse! take) drop)
      (loop (cdr drop) (cons (car drop) take) (- i 1)))))


(define (group lst n)
  (let loop ((rest lst) (acc '()))
    (if (null? rest)
      (reverse! acc)
      (let ((take/drop (safe-split-at rest n)))
        (loop (cdr take/drop) (cons (car take/drop) acc))))))

;(define (group lst n)
;  (let loop ((rest lst) (acc '()))
;    (if (null? rest)
;      (reverse! acc)
;      (receive (take drop)
;               (safe-split-at rest n)
;               (loop take (cons drop acc))))))

(define (delete-index idx lst)
  (let loop ((i idx) (rest lst) (acc '()))
    (cond
      ((null? rest) (reverse! acc))
      ((zero? i)    (loop (- i 1) (cdr rest) acc))
      (else         (loop (- i 1) (cdr rest) (cons (car rest) acc))))))


(define (xor a b)
  (if a (not b) b))


(define (any->string x)
  (format #f "~a" x))


(define (string-take-while pred str)
  (let ((idx (string-index str pred)))
    (if idx (string-take str idx) str)))
