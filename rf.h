#include "types.h"
#include <math.h>

#ifdef CC1110
#define MHZ 26
#elif CC1111
#define MHZ 48
#else
#error Please define CC1110 or CC1111 so clock can be determined.
#endif

#ifdef LOCAL
int PA_TABLE1, PA_TABLE0, FREQ0, FREQ1, FREQ2, CHANNR, MDMCFG3, MDMCFG4;
u8 realbuf[MAXLEN+1];
#else
extern __xdata u8 realbuf[MAXLEN+1];
#endif

void rftx();
void waitForTx();
void db_send();
void setBit(u8 *A, u16 b, __bit val);
__bit testBit(u8 *A, u16 b);
void db(u8 t, u8 p);
void convert_bits();
void de_bruijn();
void setFreq();
void setBaud();
void doTx();
