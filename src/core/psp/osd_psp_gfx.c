/****************************************************************************
 * Hu-Go! PSP
 *
 * GFX functions
 ****************************************************************************/

#include "defs.h"
#include "sys_gfx.h"
#include "hard_pce.h"
#include "gfx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <SDL.h>

#include <pspctrl.h>
#include <psptypes.h>
#include <png.h>

# ifndef LINUX_MODE
#include <pspgu.h>
#include <pspdisplay.h>
#include <psprtc.h>
# endif

//! PC Engine rendered screen
unsigned char *screen = NULL;

//! Host machine rendered screen
unsigned char *physical_screen = NULL;

int osd_gfx_init();
int osd_gfx_init_normal_mode();
void osd_gfx_put_image_normal();
void osd_gfx_shut_normal_mode();

UInt16 osd_pal[256];

/*****************************************************************************

    Function: osd_gfx_put_image_normal

    Description: draw the raw computed picture to screen, without any effect
       trying to center it (I bet there is still some work on this, maybe not
                            in this function)
    Parameters: none
    Return: nothing

*****************************************************************************/
void osd_gfx_put_image_normal(void)
{
# if 0
  extern SDL_Surface* blit_surface;
  u16* dst_ptr = blit_surface->pixels;
  u8*  src_ptr = osd_gfx_buffer;

  int len = XBUF_HEIGHT * XBUF_WIDTH;
  while (len-- > 0) {
    *dst_ptr++ =  osd_pal[ *src_ptr++ ];
  }
# endif
  hugo_render_update();
}

/*****************************************************************************

    Function: osd_gfx_set_message

    Description: compute the message that will be displayed to create a sprite
       to blit on screen
    Parameters: char* mess, the message to display
    Return: nothing but set OSD_MESSAGE_SPR

*****************************************************************************/
void osd_gfx_set_message(char* mess)
{
	/*** TODO: Update the screen info ***/
}

/*
 * osd_gfx_init:
 * One time initialization of the main output screen
 */
int osd_gfx_init(void)
{
	SetPalette();
  return 1;
}


/*****************************************************************************

    Function:  osd_gfx_init_normal_mode

    Description: initialize the classic 256*224 video mode for normal video_driver
    Parameters: none
    Return: 0 on error
            1 on success

*****************************************************************************/
int osd_gfx_init_normal_mode()
{
  return 1;  
}

//! Delete the window
void osd_gfx_shut_normal_mode(void)
{
}

/*****************************************************************************

    Function: osd_gfx_set_color

    Description: Change the component of the choosen color
    Parameters: UChar index : index of the color to change
    			UChar r	: new red component of the color
                UChar g : new green component of the color
                UChar b : new blue component of the color
    Return:

*****************************************************************************/
void osd_gfx_set_color(UChar index, UChar r, UChar g, UChar b)
{
  r <<= 2;
  g <<= 2;
  b <<= 2;
  osd_pal[index] = psp_sdl_rgb( r, g, b );
}
