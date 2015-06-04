/*
 * Copyright 2010 Michael Ossmann
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <cc1110.h>
#include "ioCCxx10_bitdef.h"
#include "types.h"
#include "bits.h"

/* prepare an interrupt for the power button so it will wake us up */
void setup_pm_interrupt() {
	/* clear the interrupt flags */
	P1IFG &= ~BIT6;
	P1IF = 0;

	/* enable interrupt on power button */
	P1IEN = BIT6;

	/* enable interrupts on the port */
	IEN2 |= IEN2_P1IE;

	/* produce interrupts on falling edge */
	PICTL |= PICTL_P1ICON;

	/* enable interrupts globally */
	EA = 1;
}

/* power button interrupt service routine */
void port1_isr() __interrupt (P1INT_VECTOR) {
	/* clear the interrupt flags */
	P1IFG &= ~BIT6;
	P1IF = 0;

	/* clear sleep mode bits */
	SLEEP &= ~SLEEP_MODE;
}

/*
 * All this DMA and clock nonsense is based on the Errata Note (swrz022b) which
 * describes a workaround for "Part May Hang in Power Mode."  Timing is
 * critical here.  Do not edit this function without reading the Errata Note.
 */

void sleep() {
	volatile u8 desc_high = DMA0CFGH;
	volatile u8 desc_low = DMA0CFGL;
	//__xdata u8 dma_buf[7] = {0x07,0x07,0x07,0x07,0x07,0x07,0x04};
	static __code u8 dma_buf[7] = {0x07,0x07,0x07,0x07,0x07,0x07,0x04};
	static __xdata u8 dma_desc[8] = {0x00,0x00,0xDF,0xBE,0x00,0x07,0x20,0x42};

	/* switch to HS RCOSC */
	SLEEP &= ~SLEEP_OSC_PD;
	while (!(SLEEP & SLEEP_HFRC_S));
	CLKCON = (CLKCON & ~CLKCON_CLKSPD) | CLKCON_OSC | CLKSPD_DIV_2;
	while (!(CLKCON & CLKCON_OSC));
	SLEEP |= SLEEP_OSC_PD;

	setup_pm_interrupt();

	/* store descriptors and abort any transfers */
	desc_high = DMA0CFGH;
	desc_low = DMA0CFGL;
	DMAARM |= (DMAARM_ABORT | DMAARM0);

	/* DMA prep */
	dma_desc[0] = (u16)&dma_buf >> 8;
	dma_desc[1] = (u16)&dma_buf;
	DMA0CFGH = (u16)&dma_desc >> 8;
	DMA0CFGL = (u16)&dma_desc;
	DMAARM = DMAARM0;

	/*
	 * Any interrupts not intended to wake from sleep should be
	 * disabled by this point.
	 */

	/* disable flash cache */
	MEMCTR |= MEMCTR_CACHD;

	/* select sleep mode PM3 and power down XOSC */
	SLEEP |= (SLEEP_MODE_PM3 | SLEEP_OSC_PD);

	__asm
   	nop
   	nop
   	nop
	__endasm;

	if (SLEEP & SLEEP_MODE_PM3) {
		__asm
		mov 0xD7,#0x01 /* DMAREQ */
		nop
		orl 0x87,#0x01 /* last instruction before sleep */
		nop            /* first instruction after wake */
		__endasm;
	}

	/* enable flash cache */
	MEMCTR &= ~MEMCTR_CACHD;

	/* restore DMA */
	DMA0CFGH = desc_high;
	DMA0CFGL = desc_low;
	DMAARM = DMAARM0;

	/* make sure HS RCOSC is stable */
	while (!(SLEEP & SLEEP_HFRC_S));
}
