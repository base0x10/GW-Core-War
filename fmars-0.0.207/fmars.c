#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "fm_types.h"
#include "fmars.h"
#include "exhaust.h"
#include "insn.h"

#ifdef verbose_bind_warrior
#include "fm_asm.h"
#endif


/* FIXME: clean up pspace allocation + core_mem (... * sizeof (...) + 7)
   TODO: rewrite pspace code
   FIXME: clean up clear_core ()
   
   TODO: get rid of memset () and string.h
   TODO: make sure that after allocation alligment is perfect
   
   from athlon optimization guide:
   double *p = (double *) malloc (sizeof (double) * n + 7L);
   double *q = (double *) ((((long) (p)) + 7L) & (-8L)); */

int fmars_sim_proper (fmars_mars_t *mars, int do_fight);

static inline void reset_pspaces (fmars_mars_t *mars)
{
   fmars_field_t *pspace = mars->pspace_mem;
   int i;

   for (i = 0; i < mars->nwarriors; i++)
   {
      mars->warr_tab[i].pspace = pspace;
      pspace += mars->pspace_size;
      mars->has_pin[i] = 0;
   }
}

#if 0
fmars_mars_t *fmars_alloc (int nwarriors, int coresize, int processes,
                           int cycles, int pspace_size)
{
   fmars_mars_t *mars;


   if (pspace_size <= 0)
      pspace_size = 1;
   if (processes <= 0)
      processes = 1;
   if (cycles <= 0)
      cycles = 1;
   if (nwarriors <= 0)
      nwarriors = 1;
   if (coresize <= 0)
      coresize = 1;
   
   mars =
      (fmars_mars_t *) calloc (sizeof (fmars_mars_t) +
                               nwarriors * sizeof (fmars_warr_inf_t) +
                               coresize * sizeof (fmars_insn_t) +
                               (nwarriors * processes +
#ifdef inf_queue
                               cycles + 16 +
#ifdef alt_spl
                               nwarriors + 16 +
#endif
#endif
                                16) * sizeof (fmars_queue_item_t) +
                               9152 * sizeof (fmars_opcode_t) +
                               4 * nwarriors * sizeof (int) +
                               nwarriors * pspace_size *
                               sizeof (fmars_field_t) + 16 + 16, 1);
   if (mars != NULL)
   {
      int i;
      fmars_insn_t *core;

      mars->nwarriors = nwarriors;
      mars->coresize = coresize;
      mars->processes = processes;
      mars->cycles = cycles;
      mars->pspace_size = pspace_size;

      mars->warr_tab = (fmars_warr_inf_t *) (mars + 1);

//        mars->core_mem = (fmars_insn_t *) (mars->warr_tab + nwarriors);

      core = (fmars_insn_t *) (mars->warr_tab + nwarriors);
      core = (fmars_insn_t *) ((int) core + 16 - (int) core % 16);
      mars->core_mem = core;

      mars->instructions =
         (fmars_opcode_t *) (mars->core_mem + coresize);
      mars->starting_seq = (int *) (mars->instructions + 9152);
      mars->starting_pos = (int *) (mars->starting_seq + nwarriors);
      mars->has_pin = (int *) (mars->starting_pos + nwarriors);
      mars->pins = (int *) (mars->has_pin + nwarriors);
      mars->pspace_mem = (fmars_field_t *) (mars->pins + nwarriors);
      mars->queue_mem = (fmars_queue_item_t *) (mars->pspace_mem + nwarriors * pspace_size);

      mars->simulator = fmars_sim_proper;

      for (i = 0; i < nwarriors; i++)
         mars->warr_tab[i].id = i;

      reset_pspaces (mars);
      mars->simulator (mars, 0);
   }
   return mars;
}
#endif

void fmars_free (fmars_mars_t *mars)
{
   if (mars)
   {
      free (mars->warr_tab);
      free (mars->core_mem);
      free (mars->queue_mem);
      free (mars->instructions);
      free (mars->pspace_mem);
      free (mars->starting_seq);
      free (mars->starting_pos);
      free (mars->has_pin);
      free (mars->pins);
   }
   free (mars);
}

#if 1
fmars_mars_t *fmars_alloc (int nwarriors, int coresize, int processes,
                           int cycles, int pspace_size)
{
   fmars_mars_t *mars;
   int i;

   if (pspace_size <= 0) pspace_size = 1;
   if (processes <= 0)   processes = 1;
   if (cycles <= 0)      cycles = 1;
   if (nwarriors <= 0)   nwarriors = 1;
   if (coresize <= 0)    coresize = 1;
   
   mars = (fmars_mars_t *) malloc (sizeof (fmars_mars_t));
   if (!mars)
      return NULL;
   mars->warr_tab = (fmars_warr_inf_t *) malloc (nwarriors * sizeof (fmars_warr_inf_t));
   mars->core_mem = (fmars_insn_t *) malloc (coresize * sizeof (fmars_insn_t));
   mars->queue_mem = (fmars_queue_item_t *) malloc (sizeof (fmars_queue_item_t) *
                                               (nwarriors * processes
#ifdef inf_queue
                                                + cycles
#ifdef alt_spl
                                                + nwarriors
#endif
#endif
                                                + 1));
   mars->instructions = (fmars_opcode_t *) malloc (9152 * sizeof (fmars_opcode_t));
   mars->pspace_mem = (fmars_field_t *) malloc (nwarriors * pspace_size * sizeof (fmars_field_t));
   mars->starting_seq = (int *) malloc (nwarriors * sizeof (int));
   mars->starting_pos = (int *) malloc (nwarriors * sizeof (int));
   mars->has_pin = (int *) malloc (nwarriors * sizeof (int));
   mars->pins = (int *) malloc (nwarriors * sizeof (int));

   if (!mars->warr_tab || !mars->core_mem || !mars->queue_mem || !mars->instructions
         || !mars->pspace_mem || !mars->starting_seq || !mars->starting_pos
         || !mars->has_pin || !mars->pins)
      return fmars_free (mars), NULL;
   
   mars->nwarriors = nwarriors;
   mars->coresize = coresize;
   mars->processes = processes;
   mars->cycles = cycles;
   mars->pspace_size = pspace_size;
   mars->simulator = fmars_sim_proper;

   for (i = 0; i < nwarriors; i++)
   {
      mars->warr_tab[i].id = i;
#ifdef rfrenzy22_hack
      if (i == 0)
         mars->warr_tab[i].maxproc = rfrenzy22_hack;
      else
         mars->warr_tab[i].maxproc = processes;
#endif
   }

   
   
   reset_pspaces (mars);
   mars->simulator (mars, 0);

   return mars;
}
#endif

int fmars_sim_multiwarrior (fmars_mars_t *mars, int **death_tab)
{
   int alive_count, i;

   alive_count = mars->simulator (mars, 1);
   if (alive_count >= 0)
   {
      for (i = 0; i < mars->nwarriors; i++)
         mars->warr_tab[i].last_result = alive_count;
      for (i = 0; i < mars->nwarriors - alive_count; i++)
         mars->warr_tab[mars->starting_seq[i]].last_result = 0;
      if (death_tab != NULL)
         *death_tab = mars->starting_seq;
   }

   return alive_count;
}

void fmars_free_warrior (fmars_warrior_t *warrior)
{
   if (warrior)
      free (warrior->code);
   free (warrior);
}

fmars_warrior_t *fmars_bind_warrior (fmars_mars_t *mars, exhaust_insn_t * code,
                                     int len, int start)
{
   fmars_warrior_t *warrior;

#if 0
   warrior =
      (fmars_warrior_t *) malloc (sizeof (fmars_warrior_t) +
                                  len * sizeof (fmars_insn_t) + 16 + 16);
#endif
   warrior = (fmars_warrior_t *) malloc (sizeof (fmars_warrior_t));
   if (!warrior)
      return NULL;
   warrior->code = (fmars_insn_t *) malloc (len * sizeof (fmars_insn_t));
   if (!warrior->code)
      return free (warrior), NULL;

   if (warrior != NULL)
   {
      int i;
      unsigned ioffs;
      fmars_insn_t *w_code;

//        warrior->code = (fmars_insn_t *) (warrior + 1);
#if 0
      w_code = (fmars_insn_t *) (warrior + 1);
      w_code = (fmars_insn_t *) ((int) w_code + 16 - (int) w_code % 16);
      warrior->code = w_code;
#endif

      warrior->len = len;
      warrior->start = start;

      for (i = 0; i < len; i++)
      {
#ifdef union_core
         warrior->code[i].ab[0] = code[i].a;
         warrior->code[i].ab[1] = code[i].b;
         ioffs = code[i].i & iMASK;
         if (ioffs >= 9152 || mars->instructions[ioffs] == mars->panic_addr)
         {
#ifdef verbose_bind_warrior
            char buf[128];
            fm_disasm_line (buf, sizeof (buf), code[i], mars->coresize);
            printf ("fm_bind_warr: unsupported insn: %s\n", buf);
#endif
            fmars_free_warrior (warrior);
            return NULL;
         }
         warrior->code[i].i[1] = mars->instructions[ioffs]; /* & iMASK from insn.h */
#else
         warrior->code[i].a = code[i].a;
         warrior->code[i].b = code[i].b;
         ioffs = code[i].i & iMASK;
         if (ioffs >= 9152 || mars->instructions[ioffs] == mars->panic_addr)
         {
#ifdef verbose_bind_warrior
            char buf[128];
            fm_disasm_line (buf, sizeof (buf), code[i], mars->coresize);
            printf ("fm_bind_warr: unsupported insn: %s\n", buf);
#endif
            fmars_free_warrior (warrior);
            return NULL;
         }
         warrior->code[i].i = mars->instructions[ioffs];    /* & iMASK from insn.h */
#endif
      }
   }

   return warrior;
}

void fmars_set_pin (fmars_mars_t *mars, int warr_id, int pin)
{
   int i;

   for (i = 0; i < mars->nwarriors; i++)
      if (mars->has_pin[i] && mars->pins[i] == pin)
         mars->warr_tab[i].pspace = mars->warr_tab[warr_id].pspace;

   mars->has_pin[warr_id] = 1;
   mars->pins[warr_id] = pin;
}

void fmars_clear_pspaces (fmars_mars_t *mars)
{
   int i;

   reset_pspaces (mars);
   for (i = 0; i < mars->nwarriors; i++)
      mars->warr_tab[i].last_result = mars->coresize - 1;
   memset (mars->pspace_mem, 0,
           mars->nwarriors * mars->pspace_size * sizeof (fmars_field_t));
}

void fmars_load_warrior (fmars_mars_t *mars, fmars_warrior_t *warrior,
                         int warr_id, int position, int start_seq)
{
   int i, k, len = warrior->len;
   int coresize = mars->coresize;

   if (position < 0 || position >= mars->coresize)
   {
      position = position % mars->coresize;
      if (position < 0)
         position += mars->coresize;
   }
   if (warr_id < 0 || warr_id >= mars->nwarriors)
      printf ("fmars_load_warrior: wrong warr_id\n");
   if (start_seq < 0 || start_seq >= mars->nwarriors)
      printf ("fmars_load_warrior: wrong start_seq\n");

   mars->starting_pos[warr_id] = (warrior->start + position) % coresize;
   mars->starting_seq[start_seq] = warr_id;

   for (i = 0; i < len; i++)
   {
      k = position + i;
      if (k >= coresize)
         k -= coresize;
      mars->core_mem[k] = warrior->code[i];
   }
}

void fmars_clear_core (fmars_mars_t *mars)
{
   const unsigned coresize = mars->coresize;
   fmars_insn_t dat0;
   unsigned i;

#ifdef union_core
   dat0.ab[0] = 0;
   dat0.ab[1] = 0;
   dat0.i[1] = mars->instructions[0];
#else
   dat0.a = 0;
   dat0.b = 0;
   dat0.i = mars->instructions[0];
#endif

   for (i = 0; i < coresize; i++)
      mars->core_mem[i] = dat0;
}
