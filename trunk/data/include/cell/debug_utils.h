#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

/** @def DU_PRINTF(L, ...) 
 *
 * A crude macro to print a message iff debugging is enabled.
 */
#ifdef DU_DEBUG
#   ifdef __SPU__
#      define DU_PRINTF(...) { fprintf(stderr, "SPU|%llx> ", speid); fprintf(stderr, __VA_ARGS__); }
#   else
#      define DU_PRINTF(...) { fprintf(stderr, "PPU> "); fprintf(stderr, __VA_ARGS__); }
#   endif
#else
#   define DU_PRINTF(...) ;
#endif


#endif /* DEBUG_UTILS_H */
