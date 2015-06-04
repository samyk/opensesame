/* locally via gcc */
#ifdef __GNUC__ // testing locally
#define LOCAL
#define __xdata static
#define __idata static
#define __far static
#define __data static
#define __near static
#define __code static
#define __bit char
#include <stdio.h>
#define pp(...) printf(__VA_ARGS__)

/* simulator or real CC111x */
#else

#ifdef SIMULATOR
#define pp(...) printf(__VA_ARGS__)
#else
#define pp(...) ((void)0)
#endif

#endif

#define CC1110

#define true 1
#define false 0
#define  u8 unsigned char
#define u16 unsigned int
#define u32 unsigned long int
#define  s8 char
#define s16 int
#define s32 long int
