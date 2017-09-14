(load "misc.scm")
(load "config.scm")


(define (register name field)
  (cons* 'register field name))

(define (register? x)
  (consed-with? 'register x))

(define (field . regs)
  (cond
    ((null? regs)           #f)
    ((register? (car regs)) (cadar regs))
    (else                   (apply field (cdr regs)))))

(define (reg-type . regs)
  (aif (apply field regs)
    (conf 'type it)
    (case (car regs)
      ((irb) (conf 'type b))
      (else  'coreptr))))
      

(define-macro (define-symbols list-name . body)
  `(begin ,@(map (lambda (x)
                   `(define ,x ',x))
                 body)
          (define ,list-name ',body)))


(define-macro (define-registers . body)
  `(begin ,@(map (lambda (pair)
                   (let ((name (car pair)) (field (cadr pair)))
                     `(define ,name (register ',name ,field))))
                 body)))


(define-symbols fields
  a b i f w)

(define-symbols instructions
  dat spl mov djn add jmz sub seq sne slt jmn jmp nop mul mod div ldp stp)

(define-symbols modifiers
  f a b ab ba x i)

(define-symbols addr-modes
  direct immediate bindir bpredec bpostinc aindir apredec apostinc)

(define modes-repr
  (list "$" "#" "@" "<" ">" "*" "{" "}"))

(define-registers
  (raa a) (rab b) (rai i) (raf f) (raw w)
  (rba a) (rbb b) (rbi i) (rbf f) (rbw w)
  (mask w) (fpair w))

(define-symbols misc-regs
  ftmp tmp nextip ip irb pta ptb coresize vectc1c1 vectcc vect00 vect11)
  
(define a-registers
  (list pta raa rab rai raf raw))

(define b-registers
  (list ptb rba rbb rbi rbf rbw))

(define extra-registers
  (list mask nextip))

(define-symbols a-flags
  pta-part ra-swap-ab)

(define-symbols b-flags)
