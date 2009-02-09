/**
 * @file   cb_utils.h
 * @brief  Interface to PPU-side control block
 *         manipulation utility functions.
 * @author Emmet Caulfield
 *
 * $Id: cb_utils.h 17 2009-01-19 15:44:44Z emmet $
 */
#ifndef CB_UTILS_H
#define CB_UTILS_H

#include <stdio.h>
#include <stdint.h>
#include <ctrlblk.h>

/**
 * Fills in a control-block.
 */
void cb_fillin( ctrlblk_t* __restrict__ cb, uint32_t n_trjs, uint8_t n_spus, float t_stop);


/**
 * Duplicates a control block exactly, including all the fields that
 * (arguably) it shouldn't: i.e. rng_seed and result_base.
 */
void cb_copy( ctrlblk_t *__restrict__ tgt, ctrlblk_t *__restrict__ src);


/**
 * Dumps a control block to the given stream.
 */
void cb_dump( FILE *fp, ctrlblk_t *__restrict__ cb);


/**
 * Produces a human-readable hash (for filenames)
 */
void cb_hr_hash( ctrlblk_t *__restrict__ cb, char *buf );

#endif /* CB_UTILS_H */
