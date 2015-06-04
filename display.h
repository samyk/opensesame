/*
 * IM-Me display functions
 *
 * Copyright 2010 Dave
 * http://daveshacks.blogspot.com/2010/01/im-me-lcd-interface-hacked.html
 *
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

#include "types.h"

static volatile __bit reverseTxt = 0;

#define LOW 0
#define HIGH 1

#define WIDTH  132
#define HEIGHT 65

#define DISPLAY_ON        0xaf
#define DISPLAY_OFF       0xae

#define SET_ROW           0xb0
#define SET_COL_LO        0x00
#define SET_COL_HI        0x10
#define SET_START_LINE    0x40

#define ADC_NORMAL        0xa0
#define ADC_REVERSE       0xa1
#define DISPLAY_NORMAL    0xa6
#define DISPLAY_REVERSE   0xa7
#define ALL_POINTS_NORMAL 0xa4
#define ALL_POINTS_ON     0xa5
#define BIAS_RATIO_9      0xa2
#define BIAS_RATIO_7      0xa3
#define READ_MODIFY_WRITE 0xe0
#define END_RMW           0xee
#define RESET             0xe2
//missing common output mode select
#define POWER_SUPPLY_OFF  0x28
#define POWER_SUPPLY_ON   0x2f
#define SET_REG_RESISTOR  0x24
#define VOLUME_MODE_SET   0x81
#define STATIC_INDIC_OFF  0xac
#define STATIC_INDIC_ON   0xad
//missing booster ratio select
#define NOP               0xe3
#define OUTPUT_STATUS_SEL 0xc0
#define TEST_RESET        0xf0
#define OSC_FREQ_314      0xe4
#define OSC_FREQ_263      0xe5
#define NORMAL_DISPLAY    0x82
#define PARTIAL_DISPLAY   0x83
#define DC_DC_CLOCK_SET   0xe6

void sleepMillis(int ms);

void xtalClock();

// IO Port Definitions:
#define A0 P0_2
#define SSN P0_4
#define LCDRst P1_1
#define LED_RED  P2_3
#define LED_GREEN P2_4
// plus SPI ports driven from USART0 are:
// MOSI P0_3
// SCK P0_5

void setIOPorts();

void configureSPI();

void tx(unsigned char ch);

void txData(unsigned char ch);

void txCtl(unsigned char ch);

void LCDReset(void);

void LCDPowerSave();

void setCursor(unsigned char row, unsigned char col);

void setDisplayStart(unsigned char start);

void setNormalReverse(unsigned char normal);

void printrlc(u8 line, u8 col, char *str);
void printrl(u8 line, char *str);
void printlc(u8 line, u8 col, char *str);
void printl(u8 line, char *str);
void print(u8 line, char *str, ...);

void clear();
void UART_Init();

#ifndef LOCAL
void putchar(char c);
#endif
