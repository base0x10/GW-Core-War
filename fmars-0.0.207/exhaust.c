/* exhaust.c: crippled pmars-like redcode simulator
 * $Id: exhaust.c,v 1.13 2002/05/13 08:12:09 rowan Exp $
 */

/* This file is part of `exhaust', a memory array redcode simulator.
 * Copyright (C) 2002 M Joonas Pihlaja
 * 
 * This file falls under the GNU Public License, version 2.  See the
 * file COPYING for details.  */

//#define DEBUGX

#define VERSION 1
#define REVISION 9

/* Note pmars incompatibility: Minimum warrior separation (minsep) is
   handled wrong by pmars (prior to 0.8.6) when minsep < MAXLENGTH.
   This affects running warriors in small cores since the other
   warriors might be loaded outside of core (it might even crash).
*/
#include <stdio.h>
#ifdef SYSV
#include <strings.h>
#else
#include <string.h>
#endif
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "exhaust.h"            /* types */

#include "fm_asm.h"             /* assembler proto */
#include "fmars.h"

#define MAX_WARRIORS 10

static unsigned int NWarriors = 0;
static warrior_t *war;
static fmars_warrior_t *fmars_warriors[MAX_WARRIORS];

static char *WarriorNames[MAX_WARRIORS];
static char buf[65536];

                                /* File names of warriors. */

static field_t Positions[MAX_WARRIORS];
static field_t StartPositions[MAX_WARRIORS];

                                /* Starting position of each warrior. */


static int Deaths[MAX_WARRIORS];

static u32_t Results[MAX_WARRIORS][MAX_WARRIORS + 1];

                                /* Results[war_id][j] is the number of
                                   rounds in which warrior war_id
                                   survives until the end with exactly
                                   j-1 other warriors.  For j=0, then
                                   it is the number of rounds where
                                   the warrior died. */

                                /* p-spaces of warriors. */


/* Globals to communicate options from readargs() to main() */
int OPT_cycles = 80000;         /* cycles until tie */
unsigned int OPT_rounds = 1;    /* number of rounds to fight */
unsigned int OPT_coresize = 8000;       /* default core size */
unsigned int OPT_minsep = 0;    /* minimum distance between warriors */
unsigned int OPT_maxlen = 100;
int OPT_processes = 8000;       /* default max. procs./warrior */
int OPT_F = 0;                  /* initial position of warrior 2 */
int OPT_k = 0;                  /* nothing (`koth' format flag) */
int OPT_b = 0;                  /* nothing (we are always `brief') */
int OPT_m = 0;                  /* multi-warrior output format. */
int OPT_A = 0;                  /* Output assembled warriors */

char *prog_name;
char errmsg[256];

/*---------------------------------------------------------------
 * Utilities
 */

#define min(x,y) ((x)<(y) ? (x) : (y))

/* xbasename(): custom basename(1); returns the filename from a path
 */
static char *xbasename (s)
     char *s;
{
   char *b;

   b = strrchr (s, '/');
   return b ? 1 + b : s;
}

void panic (char *msg)
{
   fprintf (stderr, "%s: ", prog_name);
   fprintf (stderr, "%s\n", msg);
   exit (1);
}


/* pmars/rng.c random number generator
 */
s32_t rng (seed)
     s32_t seed;
{
   register s32_t temp = seed;

   temp = 16807 * (temp % 127773) - 2836 * (temp / 127773);
   if (temp < 0)
      temp += 2147483647;
   return temp;
}

void usage ()
{
   printf ("%s v%d.%d\n", prog_name, VERSION, REVISION);
   printf ("usage: %s [opts] warriors...\n", prog_name);
   printf ("opts: -r <rounds>, -c <cycles>, -F <pos>, -s <coresize>,\n"
           "      -p <maxprocs>, -d <minsep>, -l <maxlen> -A, -bk\n");
#ifdef rfrenzy22_hack
   printf ("special rf22 hacked version: maxproc(warrior1)=%d\n", rfrenzy22_hack);
#endif
   exit (1);
}

/*---------------------------------------------------------------
 * Warrior initialisations.
 */

void import_warriors (fmars_mars_t *mars)
{
   unsigned int i;
   int err, rd;

   for (i = 0; i < NWarriors; i++)
   {
      rd = read (open (WarriorNames[i], O_RDONLY), buf, sizeof (buf));
      if ((unsigned) rd >= sizeof (buf))
      {
         fprintf (stderr, "bufsize = %d, read = %d\n", sizeof (buf), rd);
         panic ("static buffer too small");
      }
      buf[rd] = 0;
      err =
         fm_asm_string (buf, NULL, &war, NWarriors, OPT_coresize, OPT_processes,
                        OPT_cycles, OPT_coresize / 16, OPT_maxlen, OPT_minsep,
                        OPT_rounds);
      if (err)
      {
         fprintf (stderr, "%d - ", err);
         panic ("assembler error");
      }
      if (OPT_A)
         fm_disasm_war (war, OPT_coresize);
      else
      {
         fmars_warriors[i] =
            fmars_bind_warrior (mars, war->code, war->len, war->start);
         if (!fmars_warriors[i])
            panic ("failed to convert warrior (unsupported insn?)");
      }
      free (war->code);
      free (war);
   }
#ifdef DEBUGX
   printf ("warriors imported\n");
#endif
}

void check_sanity ()
{
   u32_t space_used;
   unsigned int i;

   /* Make sure each warrior has some code. */
   for (i = 0; i < NWarriors; i++)
   {
      if (fmars_warriors[i]->len == 0)
      {
         sprintf (errmsg, "warrior %d has no code\n", i);
         panic (errmsg);
      }
   }
   
   /* Make sure there is some minimum sepation. */
   if (OPT_minsep == 0)
   {
      OPT_minsep = min (OPT_coresize / NWarriors, OPT_maxlen);
   }

   /* Make sure minsep dominates the lengths of all warriors. */
   for (i = 0; i < NWarriors; i++)
   {
      if (OPT_minsep < fmars_warriors[i]->len)
      {
         panic ("minimum separation must be >= longest warrior\n");
      }
   }

   /* Make sure there is space for all warriors to be loaded. */
   space_used = NWarriors * OPT_minsep;
   if (space_used > OPT_coresize)
   {
      panic ("warriors too large to fit into core\n");
   }
}


/*---------------------------------------------------------------
 * Warrior positioning algorithms
 *
 * These are pMARS compatible.  Warrior 0 is always positioned at 0.
 * posit() and npos() are transcribed from pmars/pos.c.  */

#define RETRIES1 20             /* how many times to try generating one
                                 * position */
#define RETRIES2 4              /* how many times to start backtracking */
int posit (s32_t * seed)
{
   unsigned int pos = 1, i;
   unsigned int retries1 = RETRIES1, retries2 = RETRIES2;
   int diff;

   do
   {
      /* generate */
      *seed = rng (*seed);
      Positions[pos] =
         (*seed % (OPT_coresize - 2 * OPT_minsep + 1)) + OPT_minsep;

      /* test for overlap */
      for (i = 1; i < pos; ++i)
      {
         /* calculate positive difference */
         diff = (int) Positions[pos] - Positions[i];
         if (diff < 0)
            diff = -diff;
         if ((unsigned int) diff < OPT_minsep)
            break;              /* overlap! */
      }

      if (i == pos)             /* no overlap, generate next number */
         ++pos;
      else
      {                         /* overlap */
         if (!retries2)
            return 1;           /* exceeded attempts, fail */
         if (!retries1)
         {                      /* backtrack: generate new sequence starting */
            pos = i;            /* at arbitrary position (last conflict) */
            --retries2;
            retries1 = RETRIES1;
         }
         else                   /* generate new current number (pos not
                                 * incremented) */
            --retries1;
      }
   }
   while (pos < NWarriors);
   return 0;
}

void npos (s32_t * seed)
{
   unsigned int i, j;
   unsigned int temp;
   unsigned int room = OPT_coresize - OPT_minsep * NWarriors + 1;

   /* Choose NWarriors-1 positions from the available room. */
   for (i = 1; i < NWarriors; i++)
   {
      *seed = rng (*seed);
      temp = *seed % room;
      for (j = i - 1; j > 0; j--)
      {
         if (temp > Positions[j])
            break;
         Positions[j + 1] = Positions[j];
      }
      Positions[j + 1] = temp;
   }

   /* Separate the positions by OPT_minsep cells. */
   temp = OPT_minsep;
   for (i = 1; i < NWarriors; i++)
   {
      Positions[i] += temp;
      temp += OPT_minsep;
   }

   /* Random permutation of positions. */
   for (i = 1; i < NWarriors; i++)
   {
      *seed = rng (*seed);
      j = *seed % (NWarriors - i) + i;
      temp = Positions[j];
      Positions[j] = Positions[i];
      Positions[i] = temp;
   }
}

s32_t compute_positions (s32_t seed)
{
   u32_t avail = OPT_coresize + 1 - NWarriors * OPT_minsep;

   Positions[0] = 0;

   /* Case single warrior. */
   if (NWarriors == 1)
      return seed;

   /* Case one on one. */
   if (NWarriors == 2)
   {
      Positions[1] = OPT_minsep + seed % avail;
#ifdef DEBUGX
      printf ("warrior2 loaded at: %d\n", Positions[1]);
      fflush (stdout);
#endif
      seed = rng (seed);
      return seed;
   }

   if (NWarriors > 2)
   {
      if (posit (&seed))
      {
         npos (&seed);
      }
   }
   return seed;
}


/*---------------------------------------------------------------
 * Misc.
 */

void load_warriors (fmars_mars_t *mars, int round)
{
   unsigned int i;

   for (i = 0; i < NWarriors; i++)
      fmars_load_warrior (mars, fmars_warriors[i], i, Positions[i],
                          (NWarriors + round - i) % NWarriors);

#ifdef DEBUGX
   printf ("warriors loaded\n");
   fflush (stdout);
#endif

}

void clear_results ()
{
   unsigned int i, j;

   for (i = 0; i < NWarriors; i++)
   {
      for (j = 0; j <= NWarriors; j++)
      {
         Results[i][j] = 0;
      }
   }
}

void accumulate_results (fmars_mars_t *mars, int nalive, int *death_tab,
                         int round)
{

   /* Fetch the results of the last round from p-space location 0
      that has been updated by the simulator. */
/*    for (i=0; i<NWarriors; i++) {
	    unsigned int result;
	    result = pspace_get(PSpaces[i], 0);
	    Results[i][result]++;
	}*/
   unsigned int i;

/* Fetch the results of the last round from p-space location 0
   that has been updated by the simulator. */
   for (i = 0; i < mars->nwarriors; i++)
   {
      unsigned int result;

      result = fmars_get_last_result (mars, i); // mars->war_tab[i].last_result;
      /* printf("%d ", result); */
      Results[i][result]++;
   }

}


void output_results ()
{
   unsigned int i;
   unsigned int j;

   if (NWarriors == 2 && !OPT_m)
   {
      printf ("%ld %ld\n", Results[0][1], Results[0][2]);
      printf ("%ld %ld\n", Results[1][1], Results[1][2]);
   }
   else
   {
      for (i = 0; i < NWarriors; i++)
      {
         for (j = 1; j <= NWarriors; j++)
         {
            printf ("%ld ", Results[i][j]);
         }
         printf ("%ld\n", Results[i][0]);
      }
   }
}


/*---------------------------------------------------------------
 * Command line arguments.
 */

/*
 * parse options
 */
void readargs (argc, argv)
     int argc;
     char **argv;
{
   int n;
   char c;
   int cix;
   int tmp;

   n = 1;
   while (n < argc)
   {
      cix = 0;
      c = argv[n][cix++];
      if (c == '-' && argv[n][1])
      {
         do
         {
            c = argv[n][cix++];
            if (c)
               switch (c)
               {

                  case 'k':
                     OPT_k = 1;
                     break;
                  case 'b':
                     OPT_b = 1;
                     break;
                  case 'm':
                     OPT_m = 1;
                     break;
                  case 'A':
                     OPT_A = 1;
                     break;

                  case 'F':
                     if (n == argc - 1 || !isdigit (argv[n + 1][0]))
                        panic ("bad argument for option -F\n");
                     c = 0;
                     OPT_F = atoi (argv[++n]);
                     break;

                  case 's':
                     if (n == argc - 1 || !isdigit (argv[n + 1][0]))
                        panic ("bad argument for option -s\n");
                     c = 0;
                     OPT_coresize = atoi (argv[++n]);
                     if (OPT_coresize <= 0)
                        panic ("core size must be > 0\n");
                     break;

                  case 'd':
                     if (n == argc - 1 || !isdigit (argv[n + 1][0]))
                        panic ("bad argument for option -d\n");
                     c = 0;
                     OPT_minsep = atoi (argv[++n]);
                     if ((int) OPT_minsep <= 0)
                        panic ("minimum warrior separation must be > 0\n");
                     break;
                     
                  case 'l':
                     if (n == argc - 1 || !isdigit (argv[n + 1][0]))
                        panic ("bad argument for option -l\n");
                     c = 0;
                     OPT_maxlen = atoi (argv[++n]);
                     if ((int) OPT_maxlen <= 0)
                        panic ("maximum warrior length must be > 0\n");
                     break;

                  case 'p':
                     if (n == argc - 1 || !isdigit (argv[n + 1][0]))
                        panic ("bad argument for option -p\n");
                     c = 0;
                     OPT_processes = atoi (argv[++n]);
                     if (OPT_processes <= 0)
                        panic ("max processes must be > 0\n");
                     break;

                  case 'r':
                     if (n == argc - 1 || !isdigit (argv[n + 1][0]))
                        panic ("bad argument for option -r\n");
                     c = 0;
                     tmp = atoi (argv[++n]);
                     if (tmp < 0)
                        panic ("can't do a negative number of rounds!\n");
                     OPT_rounds = tmp;
                     break;
                  case 'c':
                     if (n == argc - 1 || !isdigit (argv[n + 1][0]))
                        panic ("bad argument for option -c\n");
                     c = 0;
                     OPT_cycles = atoi (argv[++n]);
                     if (OPT_cycles <= 0)
                        panic ("cycles must be > 0\n");
                     break;
                  default:
                     sprintf (errmsg, "unknown option '%c'\n", c);
                     panic (errmsg);
               }
         }
         while (c);

      }
      else                      /* it's a file name */
      {
         if (NWarriors == MAX_WARRIORS)
         {
            panic ("too many warriors\n");
         }
         WarriorNames[NWarriors++] = argv[n];
      }
      n++;
   }

   if (NWarriors == 0)
      usage ();
}



/*-------------------------------------------------------------------------
 * Main
 */


int main (int argc, char **argv)
{
   fmars_mars_t *mars;
   int fmars_startpos[32];
   unsigned int n;              /* round counter */
   s32_t seed;                  /* rnd seed. */

   prog_name = xbasename (argv[0]);
   readargs (argc, argv);

#ifdef DEBUGX
   printf ("allocating buffers... ");
   fflush (stdout);
#endif



   mars =
      fmars_alloc (NWarriors, OPT_coresize, OPT_processes, OPT_cycles,
                   OPT_coresize / 16);
#ifdef DEBUGX
   printf ("done\n");
   fflush (stdout);

#endif


   import_warriors (mars);
   if (OPT_A)
      return 0;
   check_sanity ();
   clear_results ();

   seed = OPT_F ? OPT_F - OPT_minsep : rng (time (0) * 0x1d872b41);

/*
     * Allocate simulator buffers and initialise p-spaces.
     */
/*  if (! sim_alloc_bufs( NWarriors, OPT_coresize, OPT_processes, OPT_cycles))
	panic("can't allocate memory.\n");*/

/*  save_pspaces();
    amalgamate_pspaces();	* Share P-spaces with equal PINs */


   /*
    * Fight OPT_rounds rounds.
    */
#ifdef DEBUGX
   printf ("ready..\n");
   fflush (stdout);
#endif

   for (n = 0; n < OPT_rounds; n++)
   {
      int nalive, q;

      fmars_clear_core (mars);

#ifdef DEBUGX
      printf ("core cleared\n");
      fflush (stdout);
#endif

      seed = compute_positions (seed);

      load_warriors (mars, n);

      nalive = fmars_sim_multiwarrior (mars, 0);
      if (nalive < 0)
         panic ("simulator panic!\n");
      else
      {
#ifdef DEBUGX
         printf ("simulation finished sucessfully\n");
         fflush (stdout);
#endif
      }
      accumulate_results (mars, nalive, Deaths, n);
#ifdef DEBUGX
      printf ("results accumulated\n");
      fflush (stdout);
#endif
   }
   fmars_free (mars);
   {
      int i;

      for (i = 0; i < NWarriors; i++)
         fmars_free_warrior (fmars_warriors[i]);
   }


   output_results ();
   return 0;
}
