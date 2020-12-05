/*
 * IM-Me display functions
 *
 * Copyright 2010 Dave - http://daveshacks.blogspot.com/2010/01/im-me-lcd-interface-hacked.html
 * Copyright 2010 Michael Ossmann
 * Copyright 2015-2020 Samy Kamkar
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

#include "types.h"
#include <cc1110.h>
#include <mcs51/8051.h>
#include "ioCCxx10_bitdef.h"
#include "display.h"
#include "bits.h"
#include "5x7.h"
#include <stdio.h>

void sleepMillis(int ms) {
	int j;
	while (--ms > 0) {
		for (j=0; j<1200;j++); // about 1 millisecond
	};
}

void xtalClock() { // Set system clock source to 26 Mhz
    SLEEP &= ~SLEEP_OSC_PD; // Turn both high speed oscillators on
    while( !(SLEEP & SLEEP_XOSC_S) ); // Wait until xtal oscillator is stable
    CLKCON = (CLKCON & ~(CLKCON_CLKSPD | CLKCON_OSC)) | CLKSPD_DIV_1; // Select xtal osc, 26 MHz
    while (CLKCON & CLKCON_OSC); // Wait for change to take effect
    SLEEP |= SLEEP_OSC_PD; // Turn off the other high speed oscillator (the RC osc)
}

void UART_Init()
{
	SCON = 0x50; /*  configure serial */
	TMOD = 0x20; /*  configure timer */
	TH1  = 0xE6; /*  baud rate 1200 */
	TL1  = 0xE6; /*  baud rate 1200 */
	TR1  = 1;    /*  enable timer */
	TI   = 1;    /*  enable transmitting */
	RI   = 0;    /*  waiting to receive */
}

void setIOPorts() {
	// No need to set PERCFG or P2DIR as default values on reset are fine
	P0SEL |= (BIT5 | BIT3 ); // set SCK and MOSI as peripheral outputs
	P0DIR |= BIT4 | BIT2; // set SSN and A0 as outputs
	P1DIR |= BIT1; // set LCDRst as output
	P2DIR = BIT3 | BIT4; // set LEDs  as outputs
}

// Set a clock rate of approx. 2.5 Mbps for 26 MHz Xtal clock
#define SPI_BAUD_M  170
#define SPI_BAUD_E  16

void configureSPI() {
	U0CSR = 0;  //Set SPI Master operation
	U0BAUD =  SPI_BAUD_M; // set Mantissa
	U0GCR = U0GCR_ORDER | SPI_BAUD_E; // set clock on 1st edge, -ve clock polarity, MSB first, and exponent
}

void tx(unsigned char ch) {
#ifndef SIMULATOR
	U0DBUF = ch;
	while(!(U0CSR & U0CSR_TX_BYTE)); // wait for byte to be transmitted
	U0CSR &= ~U0CSR_TX_BYTE;         // Clear transmit byte status
#endif
}

void txData(unsigned char ch) {
	A0 = HIGH;
	tx(ch);
}

void txCtl(unsigned char ch){
	A0 = LOW;
	tx(ch);
}

void LCDReset(void) {
	LCDRst = LOW; // hold down the RESET line to reset the display
	sleepMillis(1);
	LCDRst = HIGH;
	SSN = LOW;
	/* initialization sequence from sniffing factory firmware */
	txCtl(RESET);
	txCtl(SET_REG_RESISTOR);
	txCtl(VOLUME_MODE_SET);
	txCtl(0x60); /* contrast */
	txCtl(DC_DC_CLOCK_SET);
	txCtl(0x00); /* fOSC (no division) */
	txCtl(POWER_SUPPLY_ON);
	txCtl(ADC_REVERSE);
	txCtl(DISPLAY_ON);
	txCtl(ALL_POINTS_NORMAL);
	SSN = HIGH;
}

/* initiate sleep mode */
void LCDPowerSave() {
	txCtl(STATIC_INDIC_OFF);
	txCtl(DISPLAY_OFF);
	txCtl(ALL_POINTS_ON); // Display all Points on cmd = Power Save when following LCD off
}

void setCursor(unsigned char row, unsigned char col) {
	txCtl(SET_ROW | (row & 0x0f));
	txCtl(SET_COL_LO | (col & 0x0f));
	txCtl(SET_COL_HI | ( (col>>4) & 0x0f));
}

void setDisplayStart(unsigned char start) {
	txCtl(0x40 | (start & 0x3f)); // set Display start address
}

void setNormalReverse(unsigned char normal) {  // 0 = Normal, 1 = Reverse
	txCtl(DISPLAY_NORMAL | (normal & 0x01) );
}

/* print inverted string on screen (dark background) */
void printrl(u8 line, char *str)
{
	reverseTxt = 1;
	printlc(line, 0, str);
	reverseTxt = 0;
}

/* print inverted string on screen (dark background)*/
void printrlc(u8 line, u8 col, char *str)
{
	reverseTxt = 1;
	printlc(line, col, str);
	reverseTxt = 0;
}

/* print string on screen */
void printl(u8 line, char *str)
{
	printlc(line, 0, str);
}

/* print string on screen */
void printlc(u8 line, u8 col, char *str)
{
#ifdef SIMULATOR
	pp(str);
#else
	SSN = LOW;
	setCursor(line, col * 6);
	printf(str);
	SSN = HIGH;
#endif
}

/* print string on screen */
/*
void print(u8 line, char *str, ...)
{
	va_list argptr;
	va_start(argptr, str);
	SSN = LOW;
	setCursor(line, 0);
	vprintf(str, argptr);
	SSN = HIGH;
	va_end(argptr);
}
*/

/* clear all LCD pixels */
void clear() {
	u8 row;
	u8 col;

	SSN = LOW;
	setDisplayStart(0);

	/* normal display mode (not inverted) */
	setNormalReverse(0);

	for (row = 0; row <= 9; row++)
	{
		setCursor(row, 0);
		for (col = 0; col < WIDTH; col++)
			txData(0x00);
	}

	SSN = HIGH;
}

/* sdcc provides printf if we provide this */
PUTCHAR_TYPE putchar(char c)
{
#ifdef SIMULATOR
	while (!TI); /*  wait end of last transmission */
	TI = 0;
	SBUF = c; /*  transmit to serial */
#else
	u8 i;

	c &= 0x7f;

	if (c >= FONT_OFFSET)
	{
		for (i = 0; i < FONT_WIDTH; i++)
			txData(font[c - FONT_OFFSET][i] ^ (reverseTxt ? 0xff : 0));
		txData(reverseTxt ? 0xff : 0x00);
	}
#endif
#ifdef PUTCHAR_INT
  return c;
#endif
}
