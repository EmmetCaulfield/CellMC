#ifndef SUPPORT_H
#define SUPPORT_H

#include <stdio.h>

#if defined(__SPU__)
#   include "cell/spu/arch-support.h"
#else
#   include "sim-info.h"
#   include "error-macros.h"
#   if defined(__SSE2__)
#      include "ia32/arch-support.h"
#   elif defined(__PPU__)
//#      include "cell/ppu/arch-support.h"
#   else
#      error Unknown architecture
#   endif
#endif

#endif /* SUPPORT_H */
