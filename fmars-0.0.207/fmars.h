#ifndef fmars_h
#define fmars_h

#include "fm_types.h"
#include "exhaust.h"

#define exhaust_insn_t insn_t

#ifdef __cplusplus
extern "C" {
#endif

/*
    fmars_alloc (nwarriors, coresize, processes, cycles, pspace_size)

    Creates new MARS with given parameters. Returns a pointer to mars,
    or NULL if failed to allocate memory. The memory should be freed
    with free () when no longer needed.
*/

void fmars_free (fmars_mars_t *mars);

fmars_mars_t *fmars_alloc (int nwarriors, int coresize, int processes,
			   int cycles, int pspace_size);



/*
    fmars_bind_warrior (*mars, *code, len, start)

    code - pointer to the first instruction of a warrior in exhaust format
    len - length of the warrior
    start - starting position of the warrior

    Converts a warrior in exhaust format to fmars internal format. The
    representation is tightly bound to the mars passed in the first argument,
    and cannot be reused in different instances of fmars.
    
    Returns a pointer to warrior, or NULL if failed to allocate memory.
    The memory should be freed with free () when no longer needed.
*/

void fmars_free_warrior (fmars_warrior_t *warrior);

fmars_warrior_t *fmars_bind_warrior (fmars_mars_t *mars, exhaust_insn_t *code,
				     int len, int start);



/*
    fmars_clear_pspaces (*mars)

    Resets PINs of all warriors and clears pspaces.
*/

void fmars_clear_pspaces (fmars_mars_t *mars);



/*
    fmars_set_pin (*mars, warr_id, pin)

    Sets PIN of a warrior with the given ID to pin and shares its
    pspace if necessary.
*/

void fmars_set_pin (fmars_mars_t *mars, int warr_id, int pin);



/*
    fmars_clear_core (*mars)

    Clears the core.
*/

void fmars_clear_core (fmars_mars_t *mars);



/*
    fmars_load_warrior (*mars, *warrior, warr_id, position, start_seq)

    warr_id - a number unique to each warrior, ranging from 0 to (nwar - 1);
	      if you're using pspace, it shouldn't change in subseqent rounds
    position - a position to load the first instruction of the warrior
    start_seq - position of the warrior in starting sequence, ranging from
		0 to (nwar - 1); warrior with lower value starts first

    Example: to load two warriors into random positions, with every warrior
    starting every other round:

	fmars_load_warrior (mars, w1, 0, 0, round % 2);
	fmars_load_warrior (mars, w2, 1, 100 + random () % 7801,
			    1 - round % 2);
*/

void fmars_load_warrior (fmars_mars_t *mars, fmars_warrior_t *warrior,
			 int warr_id, int position, int start_seq);



/*
    fmars_sim_multiwarrior (*mars, *death_tab);

    death_tab - if not NULL, the variable it points to will be set to point to
		a static buffer, containing IDs of dead warriors in order in
		which they were killed
		
    Returns number of warriors alive at the end of the round, or -1 in case
    of simulator panic.

    Example:

	int *death_tab;
	alive_cnt = fmars_sim_multiwarrior (mars, &death_tab);
	if (alive_cnt == 1)
	    printf ("warrior %d lost\n", death_tab[0]);
*/

int fmars_sim_multiwarrior (fmars_mars_t *mars, int **death_tab);



/*
    fmars_get_last_result (*mars, warr_id)

    Returns result of last round for the warrior with given ID, that is:
    0, if the warrior was killed, or number of warriors alive at the
    end of the round.
*/

static inline int fmars_get_last_result (fmars_mars_t *mars, int warr_id)
{
    return mars->warr_tab[warr_id].last_result;
}
#ifdef __cplusplus
}
#endif

#endif
