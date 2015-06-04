/*! \file fbuffer.h
  \author Travis Goodspeed
  \brief IMME FrameBuffer
*/

#include "types.h"

//! Set a pixel of the FB.
void fb_setpixel(u16 i, u16 j);
//! Get a pixel of the FB.
u8 fb_getpixel(u16 i, u16 j);

//! Flush the FB to the LCD.
void fb_flush();

//! Write a maskless sprite into the FB.
void fb_bitblt(u8 *sprite, u16 x, u16 y, u8 frame);

//! Clear the FB.
void fb_blank();

//! Draw a horizontal line.
void fb_horizline(u16 x, u16 y, u16 x);
