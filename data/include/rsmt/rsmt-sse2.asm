#ifndef RSMT_SSE2_ASM
#define RSMT_SSE2_ASM	
    /*
     * v~d = %[d]
     * x~a = xmm7 
     * y~b = xmm8
     * z~c = %[z]
     */
    asm volatile (
	"movdqa  (%[a]), %%xmm7		\n\t" /* x = *a		   I1   */
	"movdqa  (%[b]), %%xmm8		\n\t" /* y = *b		   I2.1 */
	"psrld   %[SR1], %%xmm8		\n\t" /* y =>> RM_SR1	   I2.2 */
	"psrld   %[SR2],   %[z]		\n\t" /* z = c >> RM_SR2   I3.2 */ 
	"pslld   %[SL1],   %[v]		\n\t" /* v = d << RM_SL1   I4.1 */
	"pxor    %%xmm7,   %[z]		\n\t" /* z ^= x		   I5   */
	"pxor      %[v],   %[z]		\n\t" /* z ^= v		   I6   */
	"pslld   %[SL2], %%xmm7		\n\t" /* x =<< RM_SL2      I7   */
	"pand      %[m], %%xmm8		\n\t" /* y &= mask	   I8   */
	"pxor    %%xmm7,   %[z]		\n\t" /* z ^= x		   I9   */
	"pxor    %%xmm8,   %[z]		\n\t" /* z ^= y		   IA   */
	: /* outputs */ [z] "=x" (z)
	: /* inputs */  [a] "r" (a), [b] "r" (b), "[z]"(c), [v]"x"(d), 
	  [SR1]"i"(RM_SR1), [SR2]"i"(RM_SR2), [SL1]"i"(RM_SL1), 
	  [SL2]"i"(RM_SL2), [m]"x"(mask)
	: /* clobbered */ "%xmm7", "%xmm8"
    );

#endif /* RSMT_SSE2_ASM */
