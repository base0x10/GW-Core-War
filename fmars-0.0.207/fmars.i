%module fmars
%include exception.i
%include typemaps.i
%{
#include "fm_asm.h"
#include "fm_types.h"
#include "fmars.h"
%}

%include "exhaust.h"
%include "fm_types.h"

%exception Warrior
{
   $action
   if (!result)
      SWIG_exception (SWIG_RuntimeError, "fmars: parse error or unsupported instruction")
}

%extend Warrior
{
   Warrior (Mars *mars, char *code)
   {
      warrior_t *exh_war;
      fmars_warrior_t *war;

      if (mars == NULL)
          return NULL;
      if (fm_asm_string (code, NULL, &exh_war,
               mars->nwarriors,
               mars->coresize,
               mars->processes,
               mars->cycles,
               mars->pspace_size,
               mars->maxlength,
               mars->maxlength,
               200)) // round = 200
         return NULL;
      war = fmars_bind_warrior (mars, exh_war->code, exh_war->len, exh_war->start);
      free (exh_war->code);
      free (exh_war);
      if (war == NULL)
         return NULL;
      war->owner = mars;
      return war;
   }

   ~Warrior ()
   {
      if (self)
         free (self->code);
      free (self);
   }
}


%extend Mars
{
   Mars (int coresize = 8000, int processes = 8000, int cycles = 80000, int maxlength = 100)
   {
      fmars_mars_t *m;
      m = fmars_alloc (2, coresize, processes, cycles, 1);
      if (m != NULL)
         m->maxlength = maxlength;
      return m;
   }

   ~Mars ()
   {
      fmars_free (self);
   }

%apply int *OUTPUT {int *OUT_w1wins, int *OUT_w2wins, int *OUT_ties};

   void fightn (Warrior *w1, Warrior *w2, int rounds, int seed, int *OUT_w1wins, int *OUT_w2wins, int *OUT_ties)
   {
      int avail = self->coresize + 1 - 2 * self->maxlength;
      int w1wins = 0, w2wins = 0, ties = 0;

      seed -= self->maxlength;
      if (seed < 0)
         seed += 2147483647;

      for (int i = 0; i < rounds; i++)
      {
         int alive, *death_tab, pos2 = self->maxlength + seed % avail;
         seed = 16807 * (seed % 127773) - 2836 * (seed / 127773);
         if (seed < 0)
            seed += 2147483647;

         fmars_clear_core (self);
         fmars_load_warrior (self, w1, 0, 0, i % 2);
         fmars_load_warrior (self, w2, 1, pos2, 1 - i % 2);
         alive = fmars_sim_multiwarrior (self, &death_tab);

         if (alive == 2)
            ties++;
         else if (death_tab[0] == 0)
            w2wins++;
         else
            w1wins++;
      }

      *OUT_w1wins = w1wins;
      *OUT_w2wins = w2wins;
      *OUT_ties = ties;   
   }

   int fight (Warrior *w1, Warrior *w2, int pos2, int who_starts)
   {
       int alive, *death_tab;
       fmars_clear_core (self);
       fmars_load_warrior (self, w1, 0, 0, who_starts);
       fmars_load_warrior (self, w2, 1, pos2, 1 - who_starts);
       alive = fmars_sim_multiwarrior (self, &death_tab);
       if (alive == 2)
          return 0;
       else if (death_tab[0] == 0)
          return -1;
       else
          return 1;
   }
}



