// functions for garage pwnage and de bruijn sequence generation
// for the im-me and opensesame garage hacking platform
//
// -samy kamkar
// http://samy.pl/opensesame

#include "types.h"
#ifndef LOCAL
#include <cc1110.h>
#endif
#include "display.h"
#include "garages.h"
#include "rf.h"
#include <math.h>
#include <stdio.h>

// we're making these global because
// we have limited stack and we're
// calling some functions recursively
__xdata u8 a[MAXBITS+1]; // largest bit size possible
u16 s, k;
u8 tmpi;
__bit firstTx;

// we could read from garages[] directly but this is faster
u8 codelen;
u8 bits, len;
__bit tri;
u32 b0, b1;
//#define bits g.bits
//#define len  g.len
//#define tri  g.tri
//#define b0   g.b0
//#define b1   g.b1

#ifdef LOCAL
void rftx() { }
void waitForTx() { }
void printl(u8 line, char *str) { }
int main() { db_send(); }
u8 sequence[MAXLEN];
#else
__xdata u8 sequence[MAXLEN];
#endif

// set or clear bit b in array A
void setBit(u8 *A, u16 b, __bit val)
{
	val ?
		(A[b / 8] |=   1 << (b % 8)) :
		(A[b / 8] &= ~(1 << (b % 8)));
}

// test bit b in array A
__bit testBit(u8 *A, u16 b)
{
	return ( ( A[b / 8] & (1 << (b % 8) ) ) != 0 );
}

// binary de bruijn sequence -samyk
void db(u8 t, u8 p)
{
	u8 j;

	if (t > bits)
	{
		if (bits % p == 0)
		{
			for (j = 1; j <= p; j++)
			{
				setBit(sequence, s++, a[j]);

				// once we have 254 bytes (may reduce because running out of space!), we need to tx due to size constraints
				if ((s + 1) * len >= MAXLEN * 8)
					// transmit
					doTx();
			}
		}
	}
	else
	{
		a[t] = a[t-p];

		db(t+1, p);
		for (j = a[t-p] + 1; j <= (tri ? 2 : 1); j++)
		{
			a[t] = j;
			db(t+1, t);
		}
	}
}

// converts our bits to the actual bitstream/modulation the garage expects
void convert_bits()
{
	u8 tlen;
	u16 z;
	u32 tb;

	// pull in last X bits of last transmission
	if (!firstTx)
	{
		// let's keep last bits (10) * len (4) since we may have broken the true code up during this delay
		for (tmpi = 0; tmpi < codelen; tmpi++)
		{
//printf("s=%d cl=%d len=%d realbuf[%d] = rb[%d]\n", s, codelen, len, tmpi,
//			(int)(ceilf((s*len)/8.0)) - codelen + tmpi);
			realbuf[tmpi] = realbuf[(int)(ceilf((s*len)/8.0)) - codelen + tmpi];
			//realbuf[tmpi] = realbuf[((s*len)/8) - codelen + tmpi];
			}
	}
	else
		firstTx = 0;

	// go through each bit and convert to the garage bitstream equivalent
	// eg 0 = 1000, 1 = 1110
	for (z = k / len; z < s; z++)
	{
		tb = testBit(sequence, z) ? b1 : b0;

		tlen = len;
		while (tlen--)
		{
			// there's gotta be a better way to do this but my brain hurts right now
			setBit(realbuf, (7-(k%8))+((k/8)*8), (tb >> tlen) & 1);
			k++;
		}
	}

	// start 10 bits in next time (assuming 10 bit code)
	k = bits * len;
	s = bits;
}

// start the de bruijn sequence using the recursive db() function
void de_bruijn()
{
	// clear a[], s
	for (tmpi = 0; tmpi <= MAXBITS; tmpi++)
		a[tmpi] = 0;
	s = 0;
	firstTx = 1;

	// begin recursive de bruijn
	db(1, 1);

	// a real de bruijn wraps, but since we can't wrap time we have to send the last of the bits to complete the sequence
	for (tmpi = 0; tmpi < bits-1; tmpi++)
		setBit(sequence, s++, 0);

	// transmit anything left over
	doTx();
}

// set FREQ registers in cc111x from a float
void setFreq()
{
#define fnum (u32)(g.hz * ((0x10000 / 1000000.0) / MHZ))
	FREQ2 = fnum >> 16;
	FREQ1 = (fnum >> 8) & 0xFF;
	FREQ0 = fnum & 0xFF;
}

// set baudrate registers
void setBaud()
{
	u8 drate_e = 0;
	u8 drate_m = 0;
	float m = 0;

	for (tmpi = 0; tmpi < 16; tmpi++)
	{

		m = (g.baud * powf(2,28) / (powf(2, tmpi)* (MHZ*1000000.0))-256) + .5;
		if (m < 256)
		{
			drate_e = tmpi;
			drate_m = m;
			break;
		}
	}

	//drate = 1000000.0 * MHZ * (256+drate_m) * powf(2,drate_e) / powf(2,28);

	MDMCFG3 = drate_m;
#ifndef MDMCFG4_DRATE_E
#define MDMCFG4_DRATE_E 0x0F
#endif
	MDMCFG4 &= ~MDMCFG4_DRATE_E;
	MDMCFG4 |= drate_e;
}

// prepare our transmission and keep the end for
// the next transmission as the delay between packets
// may prevent the de bruijn exploit from working
void doTx()
{
	// don't modify realbuf until we're done transmitting
	// previous data since we're using DMA to TX
	waitForTx();

	// modify our realbuf to the real sequence of bits we need to send
	convert_bits();

	// begin transmitting
	rftx();
}

// create a de bruijn sequence according to what we want here
void db_send()
{
	// set power
	// maximum power
	PA_TABLE0 = 0x00;
	if (g.hz <= 400000000)
		PA_TABLE1 = 0xC2;
	else if (g.hz <= 464000000)
		PA_TABLE1 = 0xC0;
	else if (g.hz <= 900000000)
		PA_TABLE1 = 0xC2;
	else
		PA_TABLE1 = 0xC0;

	// set frequency
	setFreq();
	CHANNR = 0x00;

	// maximum channel bandwidth
	setBaud();

	bits    = g.bits;
	len     = g.len;
	tri     = g.tri;
	b0      = g.b0;
	b1      = g.b1;
	codelen = ceilf(bits * (len / 8.0));

	// create and send our de bruijn bitstream
	de_bruijn();
}
