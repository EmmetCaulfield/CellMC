/**
 * @file    summblk.h
 * @brief   Model-independent summary-block definition common to 
 *          SPU and PPU code.
 * @author  Emmet Caulfield
 *
 * $Id$
 */
#ifndef SUMMBLK_H
#define SUMMBLK_H

#include <inttypes.h>	/* For exact integer types */


/*
 * We don't need this to be a particular size because we send it in an
 * extra result block
 */
typedef struct {
    /* PPU to SPU */
    uint64_t nr_abs;		/* Absolute number of reactions.				*/
    uint64_t nr_con;		/* Number of reactions contributing to computed trajectories	*/
} summblk_t;


#endif /* SUMMBLK_H */
