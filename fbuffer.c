/*! \file fbuffer.c
  \author Travis Goodspeed
  \brief IMME FrameBuffer
*/

#include "types.h"
#include "fbuffer.h"
#include <cc1110.h>
#include "display.h"

#define FBWIDTH 132
#define FBHEIGHT 64

//It's ugly to grab data like this, consumed 1kB of RAM.
__xdata u8 fbuffer[FBHEIGHT     //eight stripes
		   *FBWIDTH     //132 pixels wide
		   /8           //8 pixels/bit
		   ];

//! Clear the FB.
void fb_blank(){
  u16 i;
  for(i=0;i<FBHEIGHT*FBWIDTH/8;i++)
    fbuffer[i]=0x00;
}


//! Flush the FB to the LCD.
void fb_flush(){
  /* This is specific to the IMME LCD, as it converts from the
     internal bitmap format to the pixmap format of the LCD.
     Replacing this function and altering the buffer size should be
     sufficient to port this module to another platform. */
  
  u16 x, y, r;
  u8 col;
  
  SSN = LOW;
  
  //setNormalReverse(0);
  //erasescreen();
  
  for(y=0;y<FBHEIGHT;y+=8){
    setCursor(y/8,0); //Proper row, leftmost column.
    for(x=0;x<FBWIDTH;x++){
      col=0;
      for(r=0;r<8;r++){
	col=(col>>1)
	  |(fb_getpixel(x,y+r)?0x80:0);
      }
      txData(col);
    }
  }
  
  SSN=HIGH;
}

//! Draw a horizontal line.
void fb_horizline(u16 x, u16 y, u16 xp){
  while(x<xp){
    fb_setpixel(x++,y);
  }
}

//! Write a maskless sprite into the FB.
void fb_bitblt(u8 *sprite, u16 x, u16 y, u8 frame){
  u8 imgwidth=sprite[1];
  u8 height=sprite[2];
  u8 framewidth=sprite[1]/sprite[0];
  u8 framex;
  u8 i,j,p;
  frame%=sprite[0]; //Wrap the frame count around.
  framex=framewidth*frame;
  
  for(i=0;i<framewidth;i++){
    for(j=0;j<height;j++){
      p=sprite[3  //frames, width, height
	       +(framex/8)    //X position of frame.
	       +(i/8)         //X position within frame.
	       +(j*imgwidth/8)//Y position of row
	       ];
      if(p&(1<<(i&7)))
	//Set the right pixel of the framebuffer.
	fb_setpixel(x+i,y+j);
    }
  }
}


//! Set a pixel of the FB.
void fb_setpixel(u16 i, u16 j){
  fbuffer[(j*FBWIDTH+i)/8] |= (1<<(i&7));
}
//! Get a pixel of the FB.
u8 fb_getpixel(u16 i, u16 j){
  return 
    (fbuffer[(j*FBWIDTH+i)/8] & (1<<(i&7)))
    ?1:0;
}
