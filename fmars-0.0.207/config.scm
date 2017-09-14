(use-modules
  (srfi srfi-1)
  (srfi srfi-13)
  (ice-9 rdelim)
  (ice-9 regex))

(load "misc.scm")


(define (create-config-hash filename)
  
  (define (get-value str)
    (cond
      ((equal? str "true")  #t)
      ((equal? str "false") #f)
      (else                 (read (open-input-string str)))))

  (define (insert hash key val)
    (let ((nval (cond
                  ((null? val)        #f)
                  ((= (length val) 1) (get-value (car val)))
                  (else               (map get-value val)))))
      (hash-set! hash (sort key string<) nval)))

  (let ((cfg-hash (make-hash-table 101))
        (cfg-file (open-input-file filename)))
    (do ((line (read-line cfg-file) (read-line cfg-file)))
      ((eof-object? line) cfg-hash)
      (let ((sm (string-match "(.+)=(.+)"
                              (string-downcase
                                (string-trim-both
                                  (string-take-while #\; line))))))
        (if sm (insert cfg-hash
                       (string-tokenize (match:substring sm 1))
                       (string-tokenize (match:substring sm 2))))))))


(define ch (create-config-hash "fmars.cfg"))

(define (conf . key)
  (hash-ref ch (sort (map symbol->string (filter symbol? key)) string<)))

