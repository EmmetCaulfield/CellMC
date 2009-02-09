#ifndef REACTION_STATS_H
#define REACTION_STATS_H

#include <stdio.h>
#include <stdint.h>

#include "types.h"
#include "sim-info.h"

/**
 * A structure for representing statistics about reaction frequencies
 * in a profiling executable produced by CellMC.
 */
typedef struct {
    T_PIS   n_rxns;	/* Number of reactions counted		*/
    double  percent;	/* Percentage of total reactions	*/
    int     orig_posn;	/* Position in original SBML file	*/
} reaction_stats_t;


/**
 * Allocates, initialises, and sorts in descending order by reaction
 * count, an array of reaction_stats_t.
 *
 * The array of reaction_stats_t is initialised from si->results,
 * which is expected to point to an array of reaction counts in the
 * SBML document order of the model from which "this" executable was
 * built, the length of which is known either from the N_RXNS macro or
 * si->n_rxns.
 *
 * This module functions as sim-info helper and uses the helper data
 * and cleanup function pointer members in 'si'.
 */
void rs_init(sim_info_t *si);


/**
 * Frees memory allocated in rs_init(), but functions as a helper and
 * so takes a pointer to the sim_info_t.
 */
void _rs_cleanup(sim_info_t *si);


/**
 * Prints a complete XSL transformation to 'fp' that, when applied to
 * the original SBML model, will reorder the reactions so that
 * document order in a new SBML file corresponds to descending
 * reaction likelihood as determined by the profiling run.
 */
void rs_print_sro_xslt(sim_info_t *rs, FILE *fp);
		  

#endif /* REACTION_STATS_H */
