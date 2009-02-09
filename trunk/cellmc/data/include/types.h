#ifndef TYPES_H
#define TYPES_H
/*
 * This file defines exactly eleven macros related to type-handling
 * depending on the selected precision and platform.
 *
 * These considerably ease printing out results and declaring
 * variables, removing a large number of preprocessor directives for
 * conditional compilation (in already complex code) that would
 * otherwise be necessary.
 *
 * Accordingly, one should NEVER see "float" or "double" in connection
 * with propensities in any CellMC support code, nor "int" in
 * connection with populations, and neither "%d" or "%f" in printf
 * statements connected with them, and neither "4" nor "2" in
 * iterations connected with them.
 *
 * These are:
 *
 *     1. T_NVS : the number of slots in a vector
 *
 * And four macros for types:
 *
 *     2. T_FPV : the floating-point vector type used for propensities
 *     3. T_FPS : the floating-point scalar type used for propensities
 *     4. T_PIV : the integer vector type used for population counts
 *     5. T_PIS : the integer scalar type used for population counts
 *
 * Then, for each of the four types, their corresponding member names
 * in uv_t
 *
 *     6/7/8/9. *_UM  : uv_t member name of all types (at 2/3/4/5)
 *     10/11.   *_FMT : printf format-specifier for the scalar types 
 *                      only (at 3/5)
 */
#include <inttypes.h>

#include "univec.h"
#include "cellmc.h"

#if PREC==CMC_PREC_SINGLE
#   define T_NVS     4			/* Number of vector slots		*/
#   define T_FPV     v4sf		/* Floating-point vector type		*/
#   define T_FPV_UM  sf			/* FP vector member in uv_t		*/
#   define T_FPS     float		/* Floating-point scalar type		*/
#   define T_FPS_UM  f			/* FP scalar array member in uv_t	*/
#   define T_FPS_FMT "%f"		/* FP scalar printf format specifier	*/

#   define T_PIV     v4si		/* Population int vector type		*/
#   define T_PIV_UM  si			/* Population int vector type		*/
#   define T_PIS     int32_t		/* Population int scalar type		*/
#   define T_PIS_UM  i32		/* PI scalar array member in uv_t	*/
#   define T_PIS_FMT "%"PRIi32		/* Printf format for population		*/
#elif PREC==CMC_PREC_DOUBLE
#   define T_NVS     2			/* Number of vector slots		*/
#   define T_FPV     v2df		/* Floating-point vector type		*/
#   define T_FPS     double		/* floating-point scalar type		*/
#   define T_FPS_UM  d			/* FP scalar array member in uv_t	*/
#   define T_FPS_FMT "%f"		/* FP scalar printf format specifier	*/
#   if defined(__IA32__)
#      define T_PIV     v4si		/* Population int vector type		*/
#      define T_PIV_UM  si		/* Population int vector type		*/
#      define T_PIS     int32_t		/* Population int scalar type		*/
#      define T_PIS_UM  i32		/* PI scalar array member in uv_t	*/
#      define T_PIS_FMT "%"PRIi32	/* Printf format for population		*/
#   else
#      define T_PIV     v2di		/* Population int vector type		*/
#      define T_PIV_UM  di		/* Population int vector type		*/
#      define T_PIS     int64_t		/* Population int scalar type		*/
#      define T_PIS_UM  i64		/* PI scalar array member in uv_t	*/
#      define T_PIS_FMT "%"PRIi64	/* Printf format for population		*/
#   endif
#else
#   error No FP precision specified
#endif

#endif /* TYPES_H */
