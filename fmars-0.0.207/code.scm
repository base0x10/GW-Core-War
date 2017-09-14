(use-modules
  (srfi srfi-1)
  (srfi srfi-13)
  (srfi srfi-14)
  (ice-9 rdelim))

(load "defs.scm")
(load "misc.scm")
(load "skel.scm")
(load "insn.scm")


(define (insnum insn mod ma mb)
  (+ (ash insn 9) (ash mod 6) (ash mb 3) ma))


(define (insn-label insn)
  (string-concatenate (map symbol->string (interleave (decode-insn insn) '_))))


(define (decode-insn insn)
  (map list-ref (list instructions modifiers addr-modes addr-modes) insn))


(define (read-insn tokens)
  
  (define (index-of tok tok-lst)
    (list-index (cut equal? tok <>) (map any->string tok-lst)))

  (define (get-indices insn)
    (map index-of insn (list instructions modifiers modes-repr modes-repr)))

  (cond
    ((< (length tokens) 4)         '(#f))
    ((equal? "start" (car tokens)) (read-insn (cdr tokens)))
    ((= (length tokens) 4)         (get-indices tokens))
    (else                          (get-indices (delete-index 3 tokens)))))



(define hash (make-hash-table 1009))
(hash-set! hash 0 (cons (insn-label '(0 0 0 0)) (decode-insn '(0 0 0 0))))


(do ((line (read-line) (read-line)))
  ((eof-object? line))
  (let ((insn (read-insn (string-tokenize
                           (string-downcase line)
                           (char-set-delete char-set:graphic #\.)))))
    (unless (memq #f insn)
;      (format (current-error-port) "decoded instruction ~a (~a)\n" (insn-label insn) (apply insnum insn))
      (hash-set! hash
                 (apply insnum insn)
                 (cons (insn-label insn) (decode-insn insn))))))


(display types)
(unless (zero? (conf 'debug 'level))
  (display debug-setup))
(display declaration)
(display bind-start)
(display (bind-body hash))
(display bind-end)
(display sim-init)
(for-each (lambda (x)
            (awhen (hash-ref hash x)
;              (format (current-error-port) "making insn ~a -> ~a\n" x (car it))
              (display (apply make-insn it))))
          (iota 9152))
(display sim-end)
