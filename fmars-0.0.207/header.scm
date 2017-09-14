(load "config.scm")
(load "misc.scm")
(load "basic.scm")

(define (print . body)
  (for-each display body))

(print
"#ifndef fm_types_h
#define fm_types_h
#ifndef SWIG
"
(when/null (conf 'verbose 'bind 'warrior)
  "#define verbose_bind_warrior
  ")
(when/null (conf 'infinite 'queue)
  "#define inf_queue
  ")
(when/null (conf 'alter 'spl)
  "#define alt_spl
  ")

"
typedef "

  (caddr (assoc (conf 'type 'a) real-types))
  (cadr  (assoc (conf 'type 'a) real-types))

  " fmars_field_t;
typedef "

  (caddr (assoc (conf 'type 'i) real-types))
  (cadr  (assoc (conf 'type 'i) real-types))

  " fmars_opcode_t;"

"typedef " (if (conf 'union) "union" "struct")
"{"

  (if (conf 'union)
    "#define union_core
     unsigned short ab[4];
     void *i[2];
     int f[2];
     int w __attribute__ ((mode (V4HI)));"
    (apply string-append
           (map (lambda (field)
                  (cond
                    ((memq field '(a b)) (format #f "fmars_field_t ~a;" field))
                    ((eq? field 'in)     "fmars_opcode_t i;")
                    ((> field 0)         (format #f "char padding[~a];" field))
                    (else                "dupa")))
                (conf 'insn 'layout))))

"} fmars_insn_t;

typedef fmars_insn_t *fmars_queue_item_t;

typedef struct fmars_warr_inf_t
{
    fmars_queue_item_t *head;
    fmars_queue_item_t *tail;
    struct fmars_warr_inf_t *next;
    struct fmars_warr_inf_t *prev;
    int id;
    int nprocs;"

(when/null (conf 'rfrenzy22 'hack)
  "#define rfrenzy22_hack " (conf 'rfrenzy22 'hack)
  "
  int maxproc;")
    
"    fmars_field_t *pspace;
    int last_result;
} fmars_warr_inf_t;

#endif // SWIG


typedef struct fmars_mars_st
{
#ifdef SWIG
private:
#endif
    int coresize;
    int cycles;
    int processes;
    int nwarriors;
    int pspace_size; // 5 * 4
    int maxlength;

    int *has_pin;
    int *pins;
    int *starting_pos;
    int *starting_seq; // 4 * 4

    fmars_insn_t *core_mem; // 5 * 4
    fmars_opcode_t *instructions;
    fmars_queue_item_t *queue_mem;
    fmars_warr_inf_t *warr_tab;
    fmars_field_t *pspace_mem;

    int (*simulator) (struct fmars_mars_st *, int);

    void *panic_addr;
//    char dummy[4]; // 2 * 4 --- 16 * 4

} Mars;

#ifndef SWIG
typedef Mars fmars_mars_t;
#endif

typedef struct
{
#ifdef SWIG
private:
#endif
    fmars_insn_t *code;
    int len;
    int start;
//    int pin;
    fmars_mars_t *owner;
} Warrior;

typedef Warrior fmars_warrior_t;


#endif
")
