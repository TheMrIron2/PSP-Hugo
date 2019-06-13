/*
 *  Copyright (C) 2006 Ludovic Jacomme (ludovic.jacomme@gmail.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>

#include <pspctrl.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspiofilemgr.h>

#include "Hugo.h"
#include "global.h"
#include "psp_sdl.h"
#include "psp_kbd.h"
#include "psp_menu.h"
#include "psp_fmgr.h"
#include "psp_menu_kbd.h"
#include "psp_menu_set.h"
#include "psp_battery.h"
#include "psp_irkeyb.h"

extern SDL_Surface *back_surface;
static int psp_menu_dirty = 1;

# define MENU_SET_SOUND         0
# define MENU_SET_VOLUME        1
# define MENU_SET_FREQ          2
# define MENU_SET_IRDA_MODE     3
# define MENU_SET_CDAUDIO       4
# define MENU_SET_SPEED_LIMIT   5
# define MENU_SET_SKIP_FPS      6
# define MENU_SET_OVERCLOCK     7
# define MENU_SET_VIEW_FPS      8
# define MENU_SET_RENDER        9
# define MENU_SET_DELTA_Y      10
# define MENU_SET_VSYNC        11
# define MENU_SET_CLOCK        12
# define MENU_SET_CDCLOCK      13

# define MENU_SET_LOAD         14
# define MENU_SET_SAVE         15
# define MENU_SET_RESET        16
# define MENU_SET_BACK         17

# define MAX_MENU_SET_ITEM (MENU_SET_BACK + 1)

  static menu_item_t menu_list[] =
  {
    { "Sound enable        :"},
    { "Sound volume boost  :"},
    { "Sound frequency     :"},
    { "IRDA device         :"},
    { "CD audio track      :"},
    { "Speed limiter       :"},
    { "Skip frame          :"},
    { "Overclock           :"},
    { "Display fps         :"},
    { "Render mode         :"},
    { "Delta Y             :"},
    { "Vsync               :"},
    { "Clock frequency     :"},
    { "CD Clock frequency  :"},

    { "Load settings"        },
    { "Save settings"        },
    { "Reset settings"       },
    { "Back to Menu"         }
  };

  static int cur_menu_id = MENU_SET_LOAD;

  static int hugo_cdaudio_enable = 0;
  static int hugo_snd_enable     = 0;
  static int hugo_snd_volume     = 0;
  static int hugo_snd_freq       = 1;
  static int hugo_overclock      = 0;
  static int hugo_render_mode    = 0;
  static int hugo_vsync          = 0;
  static int hugo_view_fps       = 0;
  static int hugo_delta_y        = 0;
  static int hugo_speed_limiter  = 60;
  static int psp_cpu_clock       = 222;
  static int psp_cpu_cdclock     = 300;
  static int hugo_skip_fps       = 0;
  static int psp_irda_mode       = 0;

static void
psp_settings_menu_reset(void);

static void 
psp_display_screen_settings_menu(void)
{
  char buffer[64];
  int menu_id = 0;
  int color   = 0;
  int x       = 0;
  int y       = 0;
  int y_step  = 0;

  //if (psp_menu_dirty) 
  {

    psp_sdl_blit_help();
    psp_menu_dirty = 0;

    psp_sdl_draw_rectangle(10,10,459,249,PSP_MENU_BORDER_COLOR,0);
    psp_sdl_draw_rectangle(11,11,457,247,PSP_MENU_BORDER_COLOR,0);

    psp_sdl_back2_print( 30, 6, " L: Keyboard ", PSP_MENU_NOTE_COLOR);

    psp_display_screen_menu_battery();

    psp_sdl_back2_print( 370, 6, " R: Reset ", PSP_MENU_WARNING_COLOR);

    psp_sdl_back2_print(30, 254, " []: Cancel  O/X: Valid  SELECT: Back ", 
                       PSP_MENU_BORDER_COLOR);

    psp_sdl_back2_print(370, 254, " By Zx-81 ",
                       PSP_MENU_AUTHOR_COLOR);
  }
  
  x      = 20;
  y      = 25;
  y_step = 10;
  
  for (menu_id = 0; menu_id < MAX_MENU_SET_ITEM; menu_id++) {
    color = PSP_MENU_TEXT_COLOR;
    if (cur_menu_id == menu_id) color = PSP_MENU_SEL_COLOR;

    psp_sdl_back2_print(x, y, menu_list[menu_id].title, color);

    if (menu_id == MENU_SET_SOUND) {
      if (hugo_snd_enable) strcpy(buffer,"yes");
      else                 strcpy(buffer,"no ");
      string_fill_with_space(buffer, 4);
      psp_sdl_back2_print(190, y, buffer, color);
    } else
    if (menu_id == MENU_SET_CDAUDIO) {
      if (hugo_cdaudio_enable) strcpy(buffer,"yes");
      else                 strcpy(buffer,"no ");
      string_fill_with_space(buffer, 4);
      psp_sdl_back2_print(190, y, buffer, color);
    } else
    if (menu_id == MENU_SET_FREQ) {
      if (hugo_snd_freq == 0) strcpy(buffer, "22k mono");
      else
      if (hugo_snd_freq == 1) strcpy(buffer, "44k mono");
      else
      if (hugo_snd_freq == 2) strcpy(buffer, "22k stereo");
      else
      if (hugo_snd_freq == 3) strcpy(buffer, "44k stereo");
      string_fill_with_space(buffer, 12);
      psp_sdl_back2_print(190, y, buffer, color);
    } else
    if (menu_id == MENU_SET_VOLUME) {
      sprintf(buffer,"%d", hugo_snd_volume);
      string_fill_with_space(buffer, 7);
      psp_sdl_back2_print(190, y, buffer, color);
    } else
    if (menu_id == MENU_SET_VIEW_FPS) {
      if (hugo_view_fps) strcpy(buffer,"on ");
      else              strcpy(buffer,"off");
      string_fill_with_space(buffer, 4);
      psp_sdl_back2_print(190, y, buffer, color);
    } else
    if (menu_id == MENU_SET_OVERCLOCK) {
      sprintf(buffer,"%d", hugo_overclock);
      string_fill_with_space(buffer, 7);
      psp_sdl_back2_print(190, y, buffer, color);
    } else
    if (menu_id == MENU_SET_SKIP_FPS) {
      sprintf(buffer,"%d", hugo_skip_fps);
      string_fill_with_space(buffer, 4);
      psp_sdl_back2_print(190, y, buffer, color);
    } else
    if (menu_id == MENU_SET_SPEED_LIMIT) {
      if (hugo_speed_limiter == 0) strcpy(buffer,"no");
      else sprintf(buffer, "%d fps", hugo_speed_limiter);
      string_fill_with_space(buffer, 10);
      psp_sdl_back2_print(190, y, buffer, color);
    } else
    if (menu_id == MENU_SET_VSYNC) {
      if (hugo_vsync) strcpy(buffer,"yes");
      else                strcpy(buffer,"no ");
      string_fill_with_space(buffer, 4);
      psp_sdl_back2_print(190, y, buffer, color);
    } else
    if (menu_id == MENU_SET_RENDER) {

      if (hugo_render_mode == HUGO_RENDER_NORMAL) strcpy(buffer , "normal");
      else 
      if (hugo_render_mode == HUGO_RENDER_X125   ) strcpy(buffer, "x1.25");
      else 
      if (hugo_render_mode == HUGO_RENDER_X15    ) strcpy(buffer, "x1.5");
      else 
      if (hugo_render_mode == HUGO_RENDER_X175   ) strcpy(buffer, "x1.75");
      else                                         strcpy(buffer, "max");

      string_fill_with_space(buffer, 13);
      psp_sdl_back2_print(190, y, buffer, color);
    } else
    if (menu_id == MENU_SET_DELTA_Y) {
      sprintf(buffer,"%d", hugo_delta_y);
      string_fill_with_space(buffer, 4);
      psp_sdl_back2_print(190, y, buffer, color);
    } else
    if (menu_id == MENU_SET_CLOCK) {
      sprintf(buffer,"%d", psp_cpu_clock);
      string_fill_with_space(buffer, 4);
      psp_sdl_back2_print(190, y, buffer, color);
    } else
    if (menu_id == MENU_SET_CDCLOCK) {
      sprintf(buffer,"%d", psp_cpu_cdclock);
      string_fill_with_space(buffer, 4);
      psp_sdl_back2_print(190, y, buffer, color);
      y += y_step;
    } else
    if (menu_id == MENU_SET_IRDA_MODE) {
      if (psp_irda_mode == 0) strcpy(buffer, "none");
      else
      if (psp_irda_mode == 1) strcpy(buffer, "keyboard");
      else                    strcpy(buffer, "joystick");
      string_fill_with_space(buffer, 10);
      psp_sdl_back2_print(190, y, buffer, color);
    } else
    if (menu_id == MENU_SET_RESET) {
      y += y_step;
    }

    y += y_step;
  }

  psp_menu_display_save_name();
}


#define MAX_CLOCK_VALUES 5
static int clock_values[MAX_CLOCK_VALUES] = { 133, 222, 266, 300, 333 };

static void
psp_settings_menu_clock(int step)
{
  int index;
  for (index = 0; index < MAX_CLOCK_VALUES; index++) {
    if (psp_cpu_clock == clock_values[index]) break;
  }
  if (step > 0) {
    index++;
    if (index >= MAX_CLOCK_VALUES) index = 0;
    psp_cpu_clock = clock_values[index];

  } else {
    index--;

    if (index < 0) index = MAX_CLOCK_VALUES - 1;
    psp_cpu_clock = clock_values[index];
  }
}

static void
psp_settings_menu_cdclock(int step)
{
  int index;
  for (index = 0; index < MAX_CLOCK_VALUES; index++) {
    if (psp_cpu_cdclock == clock_values[index]) break;
  }
  if (step > 0) {
    index++;
    if (index >= MAX_CLOCK_VALUES) index = 0;
    psp_cpu_cdclock = clock_values[index];

  } else {
    index--;

    if (index < 0) index = MAX_CLOCK_VALUES - 1;
    psp_cpu_cdclock = clock_values[index];
  }
}

static void
psp_settings_menu_render(int step)
{
  if (step > 0) {
    if (hugo_render_mode < HUGO_LAST_RENDER) hugo_render_mode++;
    else                                 hugo_render_mode = 0;
  } else {
    if (hugo_render_mode > 0) hugo_render_mode--;
    else                    hugo_render_mode = HUGO_LAST_RENDER;
  }
}

static void
psp_settings_menu_delta_y(int step)
{
  if (step > 0) {
    if (hugo_delta_y < 10) hugo_delta_y++;
  } else {
    if (hugo_delta_y >  -10) hugo_delta_y--;
  }
}

static void
psp_settings_menu_skip_fps(int step)
{
  if (step > 0) {
    if (hugo_skip_fps < 25) hugo_skip_fps++;
  } else {
    if (hugo_skip_fps > 0) hugo_skip_fps--;
  }
}

static void
psp_settings_menu_limiter(int step)
{
  if (step > 0) {
    if (hugo_speed_limiter < 60) hugo_speed_limiter++;
    else                          hugo_speed_limiter  = 0;
  } else {
    if (hugo_speed_limiter >  0) hugo_speed_limiter--;
    else                          hugo_speed_limiter  = 60;
  }
}

static void
psp_settings_menu_volume(int step)
{
  if (step > 0) {
    if (hugo_snd_volume < 400) hugo_snd_volume+= 10;
    else                       hugo_snd_volume = 0;
  } else {
    if (hugo_snd_volume >  0) hugo_snd_volume -= 10;
    else                      hugo_snd_volume  = 400;
  }
}

static void
psp_settings_menu_freq(int step)
{
  if (step > 0) {
    if (hugo_snd_freq < 3) hugo_snd_freq++;
    else                     hugo_snd_freq = 0;
  } else {
    if (hugo_snd_freq >  0) hugo_snd_freq--;
    else                    hugo_snd_freq  = 3;
  }
}

static void
psp_settings_menu_irda_mode(int step)
{
  if (step > 0) {
    if (psp_irda_mode < 2) psp_irda_mode++;
    else                   psp_irda_mode = 0;
  } else {
    if (psp_irda_mode >  0) psp_irda_mode--;
    else                    psp_irda_mode  = 2;
  }
}

static void
psp_settings_menu_overclock(int step)
{
  if (step > 0) {
    if (hugo_overclock < 50) hugo_overclock += 2;
    else                     hugo_overclock  = 0;
  } else {
    if (hugo_overclock > 0) hugo_overclock -= 2;
    else                    hugo_overclock  = 50;
  }
}

static void
psp_settings_menu_init(void)
{
  hugo_cdaudio_enable = HUGO.hugo_cdaudio_enable;
  hugo_snd_enable     = HUGO.hugo_snd_enable;
  hugo_snd_volume     = HUGO.hugo_snd_volume;
  hugo_snd_freq       = HUGO.hugo_snd_freq;
  hugo_overclock      = HUGO.hugo_overclock;
  hugo_render_mode    = HUGO.hugo_render_mode;
  hugo_vsync          = HUGO.hugo_vsync;
  hugo_speed_limiter  = HUGO.hugo_speed_limiter;
  hugo_view_fps       = HUGO.hugo_view_fps;
  hugo_delta_y        = HUGO.hugo_delta_y;
  hugo_skip_fps       = HUGO.psp_skip_max_frame;
  psp_cpu_clock       = HUGO.psp_cpu_clock;
  psp_cpu_cdclock     = HUGO.psp_cpu_cdclock;
  psp_irda_mode       = psp_irda_get_saved_mode();
}

static void
psp_settings_menu_load(int format)
{
  int ret;

  ret = psp_fmgr_menu(format);
  if (ret ==  1) /* load OK */
  {
    psp_display_screen_settings_menu();
    psp_sdl_back2_print(270,  80, "File loaded !", 
                       PSP_MENU_NOTE_COLOR);
    psp_menu_dirty = 1;
    psp_sdl_flip();
    sleep(1);
    psp_settings_menu_init();
  }
  else 
  if (ret == -1) /* Load Error */
  {
    psp_display_screen_settings_menu();
    psp_sdl_back2_print(270,  80, "Can't load file !", 
                       PSP_MENU_WARNING_COLOR);
    psp_menu_dirty = 1;
    psp_sdl_flip();
    sleep(1);
  }
}

static void
psp_settings_menu_validate()
{
  /* Validate */
  HUGO.hugo_cdaudio_enable= hugo_cdaudio_enable;
  HUGO.hugo_snd_enable    = hugo_snd_enable;
  HUGO.hugo_snd_volume    = hugo_snd_volume;
  HUGO.hugo_snd_freq      = hugo_snd_freq;
  HUGO.hugo_overclock     = hugo_overclock;
  HUGO.hugo_render_mode   = hugo_render_mode;
  HUGO.hugo_vsync         = hugo_vsync;
  HUGO.hugo_speed_limiter = hugo_speed_limiter;
  HUGO.hugo_view_fps      = hugo_view_fps;
  HUGO.hugo_delta_y       = hugo_delta_y;
  HUGO.psp_cpu_clock       = psp_cpu_clock;
  HUGO.psp_cpu_cdclock     = psp_cpu_cdclock;
  HUGO.psp_skip_max_frame  = hugo_skip_fps;
  HUGO.psp_skip_cur_frame  = 0;

  if (psp_irda_set_saved_mode( psp_irda_mode )) {
      
    psp_display_screen_settings_menu();
    psp_sdl_back2_print(270,  80, "IRDA config saved", 
                       PSP_MENU_NOTE_COLOR);
    psp_sdl_back2_print(240,  90, "Emulator restart needed !", 
                       PSP_MENU_NOTE_COLOR);
    psp_menu_dirty = 1;
    psp_sdl_flip();
    sleep(2);
  }

  h6280_set_overclock(HUGO.hugo_overclock);
  osd_psp_set_sound_freq(HUGO.hugo_snd_freq);
  osd_psp_set_sound_volume(HUGO.hugo_snd_volume);
  myPowerSetClockFrequency(HUGO.psp_cpu_clock);
}

static void
psp_settings_menu_save()
{
  int error;

  psp_settings_menu_validate();
  error = hugo_save_settings();

  if (! error) /* save OK */
  {
    psp_display_screen_settings_menu();
    psp_sdl_back2_print(270,  80, "File saved !", 
                       PSP_MENU_NOTE_COLOR);
    psp_menu_dirty = 1;
    psp_sdl_flip();
    sleep(1);
  }
  else 
  {
    psp_display_screen_settings_menu();
    psp_sdl_back2_print(270,  80, "Can't save file !", 
                       PSP_MENU_WARNING_COLOR);
    psp_menu_dirty = 1;
    psp_sdl_flip();
    sleep(1);
  }
}

static void
psp_settings_menu_reset(void)
{
  psp_display_screen_settings_menu();
  psp_sdl_back2_print(270, 80, "Reset Settings !", 
                     PSP_MENU_WARNING_COLOR);
  psp_menu_dirty = 1;
  psp_sdl_flip();
  hugo_default_settings();
  psp_settings_menu_init();
  sleep(1);
}

int 
psp_settings_menu(void)
{
  SceCtrlData c;
  long        new_pad;
  long        old_pad;
  int         last_time;
  int         end_menu;

  psp_kbd_wait_no_button();

  old_pad   = 0;
  last_time = 0;
  end_menu  = 0;

  psp_settings_menu_init();

  psp_menu_dirty = 1;

  while (! end_menu)
  {
    psp_display_screen_settings_menu();
    psp_sdl_flip();

    while (1)
    {
      myCtrlPeekBufferPositive(&c, 1);
      c.Buttons &= PSP_ALL_BUTTON_MASK;

# ifdef USE_PSP_IRKEYB
      psp_irkeyb_set_psp_key(&c);
# endif
      if (c.Buttons) break;
    }

    new_pad = c.Buttons;

    if ((old_pad != new_pad) || ((c.TimeStamp - last_time) > PSP_MENU_MIN_TIME)) {
      last_time = c.TimeStamp;
      old_pad = new_pad;

    } else continue;

    if ((c.Buttons & (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) ==
        (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) {
      /* Exit ! */
      psp_sdl_exit(0);
    } else
    if ((c.Buttons & PSP_CTRL_LTRIGGER) == PSP_CTRL_LTRIGGER) {
      psp_keyboard_menu();
      end_menu = 1;
    } else
    if ((c.Buttons & PSP_CTRL_RTRIGGER) == PSP_CTRL_RTRIGGER) {
      psp_settings_menu_reset();
      end_menu = 1;
    } else
    if ((new_pad & PSP_CTRL_CROSS ) || 
        (new_pad & PSP_CTRL_CIRCLE) || 
        (new_pad & PSP_CTRL_RIGHT ) ||
        (new_pad & PSP_CTRL_LEFT  )) 
    {
      int step;

      if (new_pad & PSP_CTRL_LEFT)  step = -1;
      else 
      if (new_pad & PSP_CTRL_RIGHT) step =  1;
      else                          step =  0;

      switch (cur_menu_id ) 
      {
        case MENU_SET_SOUND      : hugo_snd_enable = ! hugo_snd_enable;
        break;              
        case MENU_SET_CDAUDIO    : hugo_cdaudio_enable = ! hugo_cdaudio_enable;
        break;              
        case MENU_SET_VOLUME     : psp_settings_menu_volume( step );
        break;              
        case MENU_SET_FREQ       : psp_settings_menu_freq( step );
        break;              
        case MENU_SET_OVERCLOCK  : psp_settings_menu_overclock( step );
        break;              
        case MENU_SET_SPEED_LIMIT : psp_settings_menu_limiter( step );
        break;              
        case MENU_SET_SKIP_FPS   : psp_settings_menu_skip_fps( step );
        break;              
        case MENU_SET_VIEW_FPS   : hugo_view_fps = ! hugo_view_fps;
        break;              
        case MENU_SET_DELTA_Y    : psp_settings_menu_delta_y( step );
        break;              
        case MENU_SET_RENDER     : psp_settings_menu_render( step );
        break;              
        case MENU_SET_VSYNC      : hugo_vsync = ! hugo_vsync;
        break;              
        case MENU_SET_CLOCK      : psp_settings_menu_clock( step );
        break;
        case MENU_SET_CDCLOCK    : psp_settings_menu_cdclock( step );
        break;
        case MENU_SET_IRDA_MODE  : psp_settings_menu_irda_mode( step );
        break;
        case MENU_SET_LOAD       : psp_settings_menu_load(FMGR_FORMAT_SET);
                                   psp_menu_dirty = 1;
                                   old_pad = new_pad = 0;
        break;              
        case MENU_SET_SAVE       : psp_settings_menu_save();
                                   psp_menu_dirty = 1;
                                   old_pad = new_pad = 0;
        break;                     
        case MENU_SET_RESET      : psp_settings_menu_reset();
        break;                     
                                   
        case MENU_SET_BACK       : end_menu = 1;
        break;                     
      }

    } else
    if(new_pad & PSP_CTRL_UP) {

      if (cur_menu_id > 0) cur_menu_id--;
      else                 cur_menu_id = MAX_MENU_SET_ITEM-1;

    } else
    if(new_pad & PSP_CTRL_DOWN) {

      if (cur_menu_id < (MAX_MENU_SET_ITEM-1)) cur_menu_id++;
      else                                     cur_menu_id = 0;

    } else  
    if(new_pad & PSP_CTRL_SQUARE) {
      /* Cancel */
      end_menu = -1;
    } else 
    if(new_pad & PSP_CTRL_SELECT) {
      /* Back to HUGO */
      end_menu = 1;
    }
  }
 
  if (end_menu > 0) {
    psp_settings_menu_validate();
  }

  psp_kbd_wait_no_button();

  psp_sdl_clear_screen( PSP_MENU_BLACK_COLOR );
  psp_sdl_flip();
  psp_sdl_clear_screen( PSP_MENU_BLACK_COLOR );
  psp_sdl_flip();

  return 1;
}

