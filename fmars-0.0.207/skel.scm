(load "basic.scm")
(load "defs.scm")
(load "asm.scm")


(define debug-setup
(st
"static inline unsigned rol (unsigned x, unsigned y)
{
    int a = y & 31;
    int b = 32 - a;
    return (x << a) | (x >> b);
}

static inline int rng (int seed)
{
    seed = 16807 * (seed % 127773) - 2836 * (seed / 127773);
    if (seed < 0)
        seed += 2147483647;
    return seed;
}

#define accumulate(sum, seed, data) \\
    seed = rng (seed); \\
    sum ^= rol ((seed) + (data), data)

static unsigned queuesum (fmars_mars_t *mars, fmars_warr_inf_t *w)
{
    const fmars_queue_item_t *queue_end = mars->queue_mem +
                                          mars->nwarriors * mars->processes +
                                          1;
    unsigned sum = 0x12345678, seed = 0x12345678;
    fmars_queue_item_t *head = w->head;
    while (head != w->tail)
    {
        accumulate (sum, seed, *head - mars->core_mem);
        head++;"
        (unless/null (conf 'infinite 'queue)
          "if (head == queue_end)
              head = mars->queue_mem;")
    "}
return sum;
}

static unsigned cache_size = 0;
static struct
{
    fmars_opcode_t in;
    unsigned num;
} cache[8192];

static unsigned coresum (fmars_mars_t *mars)
{
  unsigned sum = 0x12345678;
  unsigned seed = 0x12345678;
  int i;

  for (i = 0; i < mars->coresize; i++)
  {
      const fmars_opcode_t op = mars->core_mem[i].i;
      unsigned mod, ma, mb, ins = 0, j = 0;

      cache[cache_size].in = op;
      while (cache[ins].in != op)
      ins++;

      if (ins == cache_size)
      {
          while (mars->instructions[j] != op)
              j++;
          cache[cache_size++].num = j;
      }

      ins = cache[ins].num;

      accumulate (sum, seed, (ins >> opPOS) & opMASK);
      accumulate (sum, seed, (ins >> moPOS) & moMASK);
      accumulate (sum, seed, (ins >> maPOS) & mMASK);
      accumulate (sum, seed, (ins >> mbPOS) & mMASK);
      accumulate (sum, seed, mars->core_mem[i].a);
      accumulate (sum, seed, mars->core_mem[i].b);
  }

  return sum;
}"))


(define (debug-disasm-line insn mod ma mb)
  (st "printf (\"%6d %4ld  "
      (symbol->string insn)
      "."
      (string-pad-right (symbol->string mod) 3)
      (list-ref modes-repr (list-index (cut eq? ma <>) addr-modes))
      "%5d , "
      (list-ref modes-repr (list-index (cut eq? mb <>) addr-modes))
      "%5d  |%4ld\", cycles, ip - core, "
      "(" (core a ip) "<= coresize / 2) ?" (core a ip)
                                       ":" (core a ip) "- coresize, "
      "(" (core b ip) "<= coresize / 2) ?" (core b ip)
                                       ":" (core b ip) "- coresize, "
      (if (conf 'alter 'spl) "w->tail - w->head + 1" "w->nprocs")
      ");"))
     

(define debug-coresum
  "printf (\"c %lx\\n\", coresum (mars));")


(define debug-goto-out
  (st "{" debug-coresum+queuesum "goto out; }"))


(define includes/defines
  (st (unless/null (zero? (conf 'debug 'level))
        "#include <stdio.h>
         #include \"insn.h\"\n")
      "#include \"fm_types.h\"
       #define whole_insn(ptr) *(v4hi_t *) ptr\n"
      (if (memq (car (conf 'insn 'layout)) '(a b))
        "#define field_pair(ptr) *(long *) ptr\n"
        "#define field_pair(ptr) *((long *) ptr + 1)\n")))


(define possible-vector-types
  (map (cut apply conf <>) '((type f) (type w))))

(define types
  (st (when/null (or (memq 'v4hi possible-vector-types)
                     (memq 'v2si possible-vector-types))
        "typedef int di_t __attribute__ ((mode (DI)));
         typedef int v2si_t __attribute__ ((mode (V2SI)));
         typedef int v4hi_t __attribute__ ((mode (V4HI)));
         typedef int v8qi_t __attribute__ ((mode (V8QI)));")
      (when/null (memq 'v4sf possible-vector-types)
        "typedef int v4sf_t __attribute__ ((mode (V4SF)));")))


(define declaration
"int fmars_sim_proper (fmars_mars_t *mars, int do_fight)
{")


(define bind-start
   "if (!do_fight)
    {
        int i;
        mars->panic_addr = &&panic;
        for (i = 1; i < 9152; i++)
            mars->instructions[i] = &&panic;")


(define (bind-body insn-hash)
  (string-concatenate
    (map (lambda (key)
           (awhen/null (hash-ref insn-hash key)
             (st "mars->instructions[" key "] = &&" (car it) ";")))
         (iota 9152))))


(define bind-end
       "return -1;
    }")
      

(define sim-init
  (st "
   {
        int processes = mars->processes;
        int proc1 = processes + 1;
        fmars_insn_t *core = mars->core_mem;
        int nwar = mars->nwarriors;
                                                                                    
        fmars_queue_item_t *queue_start = mars->queue_mem;
        fmars_queue_item_t *queue_end = queue_start + nwar * processes + 1"
        (when/null (conf 'alter 'spl)
          "+ nwar")
        ";
                                                                                    
        unsigned int coresize = mars->coresize;
        int coresize8 = -coresize * sizeof (fmars_insn_t);
        int zero = 0;
        int c11 = (1 << 16) + 1;
        fmars_field_t coresize_val = coresize;
        int coresize1 = coresize - 1;
        fmars_insn_t *core_end = core + coresize;
        int cycles = nwar * mars->cycles;
        int alive_cnt = nwar;
        int pspace_size = mars->pspace_size;
                                                                                    
        int *death_tab = mars->starting_seq;
                                                                                    
        int max_alive_proc = nwar * mars->processes;
        fmars_warr_inf_t *warr_tab = mars->warr_tab;"
        
        (when/null (or (memq 'v4hi possible-vector-types)
                       (memq 'v2si possible-vector-types))
          "di_t mm[8];"
          (apply st (map save-mmx-reg
                         '(mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7)
                         (list "mm[0]" "mm[1]" "mm[2]" "mm[3]"
                               "mm[4]" "mm[5]" "mm[6]" "mm[7]"))))
                       

        (when/null (or (conf add f 'combine)
                       (conf add x 'combine)
                       (conf sub f 'combine)
                       (conf sub x 'combine)
                       (conf 'mmx 'decm 'core 'pair))
           "v4hi_t vect11, vect00, vectcc, vectc1c1;

            {
                int v00 = 0;
                int v11 = (1 << 16) + 1;
                int vcc = (coresize << 16) + coresize;
                int vc1 = ((coresize - 1) << 16) + coresize - 1;"

           (movd->mmx 'v00 vect00)
           (movd->mmx 'v11 vect11)
           (movd->mmx 'vcc vectcc)
           (movd->mmx 'vc1 vectc1c1)

           (when/null (eq? 'in (car (conf 'insn 'layout)))
             (psrlq 32 vect00)
             (psrlq 32 vect11)
             (psrlq 32 vectcc)
             (psrlq 32 vectc1c1))

           "}")
           
       "{
            fmars_queue_item_t *pofs = queue_end - 1;
            int i, prev_id, warr_id, next_id;
                                                                                    
            for (i = 0; i < nwar; i++)
            {
                prev_id = mars->starting_seq[(i - 1 + nwar) % nwar];
                warr_id = mars->starting_seq[i];
                next_id = mars->starting_seq[(i + 1) % nwar];
                                                                                    
                warr_tab[warr_id].next = warr_tab + next_id;
                warr_tab[warr_id].prev = warr_tab + prev_id;"

            (if (conf 'alter 'spl)
                "pofs -= processes + 1;"
                "pofs -= processes;")
            
                "*pofs = core + mars->starting_pos[warr_id];
                                                                                    
                warr_tab[warr_id].head = pofs;
                warr_tab[warr_id].tail = pofs + 1;
                warr_tab[warr_id].nprocs = 1;
            }
        }
                                                                                    
        {"
        
        (when/null (conf 'ip 'register)
          "register")
        "   fmars_insn_t *ip "
        (awhen/null (conf 'ip 'register)
          (st "asm (\"" it "\")")) ";

            fmars_warr_inf_t *w;
            {
                w = warr_tab + mars->starting_seq[0];
                ip = *w->head++;
                goto *" (core i ip) ";
            }"))


(define sim-end
  (st " panic:
            return -1;
          
        out:
        "
   (when/null (or (memq 'v4hi possible-vector-types)
                  (memq 'v2si possible-vector-types))
         (apply st (map restore-mmx-reg
                        (list "mm[0]" "mm[1]" "mm[2]" "mm[3]"
                              "mm[4]" "mm[5]" "mm[6]" "mm[7]")
                        '(mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7)))
         emms)
  
       (unless/null (zero? (conf 'debug 'level))
         debug-coresum)

           "return alive_cnt;
        }
    }
}
"))
