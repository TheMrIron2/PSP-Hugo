/*
 *  Copyright (C) 2009 Ludovic Jacomme (ludovic.jacomme@gmail.com)
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

#include <pspctrl.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <SDL.h>

#include "Hugo.h"
#include "global.h"
#include "psp_kbd.h"
#include "psp_menu.h"
#include "psp_sdl.h"
#include "psp_danzeff.h"
# ifdef USE_PSP_IRKEYB
#include "psp_irkeyb.h"
# endif
# ifdef USE_IRDA_JOY
#include "psp_irda.h"
# endif
#include "psp_battery.h"

# define KBD_MIN_ANALOG_TIME  150000
# define KBD_MIN_START_TIME   200000
# define KBD_MAX_EVENT_TIME   300000
# define KBD_MIN_PENDING_TIME 300000
# define KBD_MIN_HOTKEY_TIME  600000
# define KBD_MIN_DANZEFF_TIME 150000
# define KBD_MIN_COMMAND_TIME 100000
# define KBD_MIN_BATTCHECK_TIME 90000000 
# define KBD_MIN_AUTOFIRE_TIME   1000000

# ifdef USE_IRDA_JOY
  static irda_joy_t   loc_irda_joy_event;
# endif
 static SceCtrlData    loc_button_data;
 static unsigned int   loc_last_event_time = 0;
 static unsigned int   loc_last_hotkey_time = 0;
#ifdef USE_PSP_IRKEYB
 static unsigned int   loc_last_irkbd_event_time = 0;
#endif
 static unsigned int   loc_last_analog_time = 0;
 static long           first_time_stamp = -1;
 static long           first_time_batt_stamp = -1;
 static long           first_time_auto_stamp = -1;
 static char           loc_button_press[ KBD_MAX_BUTTONS ]; 
 static char           loc_button_release[ KBD_MAX_BUTTONS ]; 

 unsigned int   kbd_button_mask[ KBD_MAX_BUTTONS ] =
 {
   PSP_CTRL_UP         , /*  KBD_UP         */
   PSP_CTRL_RIGHT      , /*  KBD_RIGHT      */
   PSP_CTRL_DOWN       , /*  KBD_DOWN       */
   PSP_CTRL_LEFT       , /*  KBD_LEFT       */
   PSP_CTRL_TRIANGLE   , /*  KBD_TRIANGLE   */
   PSP_CTRL_CIRCLE     , /*  KBD_CIRCLE     */
   PSP_CTRL_CROSS      , /*  KBD_CROSS      */
   PSP_CTRL_SQUARE     , /*  KBD_SQUARE     */
   PSP_CTRL_SELECT     , /*  KBD_SELECT     */
   PSP_CTRL_START      , /*  KBD_START      */
   PSP_CTRL_HOME       , /*  KBD_HOME       */
   PSP_CTRL_HOLD       , /*  KBD_HOLD       */
   PSP_CTRL_LTRIGGER   , /*  KBD_LTRIGGER   */
   PSP_CTRL_RTRIGGER   , /*  KBD_RTRIGGER   */
 };

  char kbd_button_name[ KBD_ALL_BUTTONS ][10] =
  {
    "UP",
    "RIGHT",
    "DOWN",
    "LEFT",
    "TRIANGLE",
    "CIRCLE",
    "CROSS",
    "SQUARE",
    "SELECT",
    "START",
    "HOME",
    "HOLD",
    "LTRIGGER",
    "RTRIGGER",
    "JOY_UP",
    "JOY_RIGHT",
    "JOY_DOWN",
    "JOY_LEFT"
 };

 static char kbd_button_name_L[ KBD_ALL_BUTTONS ][20] =
 {
   "L_UP",
   "L_RIGHT",
   "L_DOWN",
   "L_LEFT",
   "L_TRIANGLE",
   "L_CIRCLE",
   "L_CROSS",
   "L_SQUARE",
   "L_SELECT",
   "L_START",
   "L_HOME",
   "L_HOLD",
   "L_LTRIGGER",
   "L_RTRIGGER",
   "L_JOY_UP",
   "L_JOY_RIGHT",
   "L_JOY_DOWN",
   "L_JOY_LEFT"
 };
 
  static char kbd_button_name_R[ KBD_ALL_BUTTONS ][20] =
 {
   "R_UP",
   "R_RIGHT",
   "R_DOWN",
   "R_LEFT",
   "R_TRIANGLE",
   "R_CIRCLE",
   "R_CROSS",
   "R_SQUARE",
   "R_SELECT",
   "R_START",
   "R_HOME",
   "R_HOLD",
   "R_LTRIGGER",
   "R_RTRIGGER",
   "R_JOY_UP",
   "R_JOY_RIGHT",
   "R_JOY_DOWN",
   "R_JOY_LEFT"
 };
 
  struct hugo_key_trans psp_hugo_key_to_name[HUGO_MAX_KEY]=
  {
    { HUGO_JOY1_A,      "J1 A"      },
    { HUGO_JOY1_B,      "J1 B"      },
    { HUGO_JOY1_SELECT, "J1 SELECT" },
    { HUGO_JOY1_RUN,    "J1 RUN"    },
    { HUGO_JOY1_UP,     "J1 UP"     },
    { HUGO_JOY1_RIGHT,  "J1 RIGHT"  },
    { HUGO_JOY1_DOWN,   "J1 DOWN"   },
    { HUGO_JOY1_LEFT,   "J1 LEFT"   },

    { HUGO_JOY2_A,      "J2 A"      },
    { HUGO_JOY2_B,      "J2 B"      },
    { HUGO_JOY2_SELECT, "J2 SELECT" },
    { HUGO_JOY2_RUN,    "J2 RUN"    },
    { HUGO_JOY2_UP,     "J2 UP"     },
    { HUGO_JOY2_RIGHT,  "J2 RIGHT"  },
    { HUGO_JOY2_DOWN,   "J2 DOWN"   },
    { HUGO_JOY2_LEFT,   "J2 LEFT"   },

    { HUGOC_FPS,      "C_FPS"      },
    { HUGOC_JOY,      "C_JOY"      },
    { HUGOC_RENDER,   "C_RENDER"   },
    { HUGOC_LOAD,     "C_LOAD"     },
    { HUGOC_SAVE,     "C_SAVE"     },
    { HUGOC_RESET,    "C_RESET"    },
    { HUGOC_AUTOFIRE, "C_AUTOFIRE" },
    { HUGOC_INCFIRE,  "C_INCFIRE"  },
    { HUGOC_DECFIRE,  "C_DECFIRE"  },
    { HUGOC_INCDELTA, "C_INCDELTA" },
    { HUGOC_DECDELTA, "C_DECDELTA" },
    { HUGOC_SCREEN,   "C_SCREEN"   }
  };

 static int loc_default_mapping[ KBD_ALL_BUTTONS ] = {
   HUGO_JOY1_UP          , /*  KBD_UP         */
   HUGO_JOY1_RIGHT       , /*  KBD_RIGHT      */
   HUGO_JOY1_DOWN        , /*  KBD_DOWN       */
   HUGO_JOY1_LEFT        , /*  KBD_LEFT       */
   HUGO_JOY1_RUN         , /*  KBD_TRIANGLE   */
   HUGO_JOY1_A           , /*  KBD_CIRCLE     */
   HUGO_JOY1_B           , /*  KBD_CROSS      */
   HUGO_JOY1_SELECT      , /*  KBD_SQUARE     */
   -1                   , /*  KBD_SELECT     */
   -1                   , /*  KBD_START      */
   -1                   , /*  KBD_HOME       */
   -1                   , /*  KBD_HOLD       */
   KBD_LTRIGGER_MAPPING , /*  KBD_LTRIGGER   */
   KBD_RTRIGGER_MAPPING , /*  KBD_RTRIGGER   */
   HUGO_JOY1_UP          , /*  KBD_JOY_UP     */
   HUGO_JOY1_RIGHT       , /*  KBD_JOY_RIGHT  */
   HUGO_JOY1_DOWN        , /*  KBD_JOY_DOWN   */
   HUGO_JOY1_LEFT          /*  KBD_JOY_LEFT   */
  };

 static int loc_default_mapping_L[ KBD_ALL_BUTTONS ] = {
   HUGOC_INCDELTA      , /*  KBD_UP         */
   HUGOC_RENDER        , /*  KBD_RIGHT      */
   HUGOC_DECDELTA      , /*  KBD_DOWN       */
   HUGOC_RENDER        , /*  KBD_LEFT       */
   HUGOC_LOAD          , /*  KBD_TRIANGLE   */
   HUGOC_JOY           , /*  KBD_CIRCLE     */
   HUGOC_SAVE          , /*  KBD_CROSS      */
   HUGOC_FPS           ,  /*  KBD_SQUARE     */
   -1                  , /*  KBD_SELECT     */
   -1                  , /*  KBD_START      */
   -1                  , /*  KBD_HOME       */
   -1                    , /*  KBD_HOLD       */
   KBD_LTRIGGER_MAPPING  , /*  KBD_LTRIGGER   */
   KBD_RTRIGGER_MAPPING  , /*  KBD_RTRIGGER   */
   HUGO_JOY1_UP           , /*  KBD_JOY_UP     */
   HUGO_JOY1_RIGHT        , /*  KBD_JOY_RIGHT  */
   HUGO_JOY1_DOWN         , /*  KBD_JOY_DOWN   */
   HUGO_JOY1_LEFT           /*  KBD_JOY_LEFT   */
  };

 static int loc_default_mapping_R[ KBD_ALL_BUTTONS ] = {
   HUGO_JOY1_UP            , /*  KBD_UP         */
   HUGOC_INCFIRE          , /*  KBD_RIGHT      */
   HUGO_JOY1_DOWN          , /*  KBD_DOWN       */
   HUGOC_DECFIRE          , /*  KBD_LEFT       */
   HUGO_JOY1_RUN         , /*  KBD_TRIANGLE   */
   HUGO_JOY1_A           , /*  KBD_CIRCLE     */
   HUGOC_AUTOFIRE         , /*  KBD_CROSS      */
   HUGO_JOY1_SELECT      , /*  KBD_SQUARE     */
   -1                    , /*  KBD_SELECT     */
   -1                    , /*  KBD_START      */
   -1                    , /*  KBD_HOME       */
   -1                    , /*  KBD_HOLD       */
   KBD_LTRIGGER_MAPPING  , /*  KBD_LTRIGGER   */
   KBD_RTRIGGER_MAPPING  , /*  KBD_RTRIGGER   */
   HUGO_JOY1_UP           , /*  KBD_JOY_UP     */
   HUGO_JOY1_RIGHT        , /*  KBD_JOY_RIGHT  */
   HUGO_JOY1_DOWN         , /*  KBD_JOY_DOWN   */
   HUGO_JOY1_LEFT           /*  KBD_JOY_LEFT   */
  };

 int psp_kbd_mapping[ KBD_ALL_BUTTONS ];
 int psp_kbd_mapping_L[ KBD_ALL_BUTTONS ];
 int psp_kbd_mapping_R[ KBD_ALL_BUTTONS ];
 int psp_kbd_presses[ KBD_ALL_BUTTONS ];
 int kbd_ltrigger_mapping_active;
 int kbd_rtrigger_mapping_active;

 static int danzeff_hugo_key     = 0;
 static int danzeff_hugo_pending = 0;
 static int danzeff_mode        = 0;

#ifdef USE_PSP_IRKEYB
 static int irkeyb_hugo_key      = 0;
 static int irkeyb_hugo_pending  = 0;
# endif

# define KBD_MAX_ENTRIES   20

  int kbd_layout[KBD_MAX_ENTRIES][2] = {
    /* Key            Ascii */
    { HUGO_JOY1_A     ,    -1          },
    { HUGO_JOY1_B     ,    -1          },
    { HUGO_JOY1_SELECT,    DANZEFF_DEL },
    { HUGO_JOY1_RUN   ,    ' ' },
    { HUGO_JOY1_UP    ,    -1 },
    { HUGO_JOY1_RIGHT ,    -1 },
    { HUGO_JOY1_DOWN  ,    -1 },
    { HUGO_JOY1_LEFT  ,    -1 },
    { HUGOC_FPS      ,     -1 },
    { HUGOC_JOY      ,     -1 },
    { HUGOC_RENDER   ,     -1 },
    { HUGOC_LOAD     ,     -1 },
    { HUGOC_SAVE     ,     -1 },
    { HUGOC_RESET    ,     -1 },
    { HUGOC_AUTOFIRE ,     -1 },
    { HUGOC_INCFIRE  ,     -1 },
    { HUGOC_DECFIRE  ,     -1 },
    { HUGOC_SCREEN   ,     -1 },
  };

# define PADDLE_MIN_RES     10000
# define PADDLE_STEP_RES    10000
# define PADDLE_MAX_RES   1000000

# define PADDLE_MIDDLE_RES 500000

int
hugo_key_event(int hugo_idx, int key_press)
{
  if (HUGO.psp_active_joystick) {
    if ((hugo_idx >= HUGO_JOY1_A   ) && 
        (hugo_idx <= HUGO_JOY1_LEFT)) {
      /* Convert Joystick Player 1 -> Player 2 ? */
      hugo_idx = hugo_idx - HUGO_JOY1_A + HUGO_JOY2_A;
    }
  }
  if ((hugo_idx >= HUGOC_FPS) &&
      (hugo_idx <= HUGOC_SCREEN)) {

    if (key_press) {
      SceCtrlData c;
      sceCtrlPeekBufferPositive(&c, 1);
      if ((c.TimeStamp - loc_last_hotkey_time) > KBD_MIN_HOTKEY_TIME) 
      {
        loc_last_hotkey_time = c.TimeStamp;
        hugo_treat_command_key(hugo_idx);
      }
    }

  } else
  if ((hugo_idx >=         0) && 
      (hugo_idx < HUGO_MAX_KEY)) {

    if (hugo_idx <= HUGO_JOY1_LEFT) {
      main_hugo_send_key_event(0, hugo_idx, key_press);
    } else
    if (hugo_idx <= HUGO_JOY2_LEFT) {
      main_hugo_send_key_event(1, hugo_idx, key_press);
    }
  }

  return 0;
}


int 
hugo_kbd_reset()
{
  memset(loc_button_press  , 0, sizeof(loc_button_press));
  memset(loc_button_release, 0, sizeof(loc_button_release));

  main_hugo_reset_keyboard();

  return 0;
}

int
hugo_get_key_from_ascii(int key_ascii)
{
  int index;
  for (index = 0; index < KBD_MAX_ENTRIES; index++) {
   if (kbd_layout[index][1] == key_ascii) return kbd_layout[index][0];
  }
  return -1;
}

void
psp_kbd_default_settings()
{
  memcpy(psp_kbd_mapping, loc_default_mapping, sizeof(loc_default_mapping));
  memcpy(psp_kbd_mapping_L, loc_default_mapping_L, sizeof(loc_default_mapping_L));
  memcpy(psp_kbd_mapping_R, loc_default_mapping_R, sizeof(loc_default_mapping_R));
}

int
psp_kbd_reset_hotkeys(void)
{
  int index;
  int key_id;
  for (index = 0; index < KBD_ALL_BUTTONS; index++) {
    key_id = loc_default_mapping[index];
    if ((key_id >= HUGOC_FPS) && (key_id <= HUGOC_SCREEN)) {
      psp_kbd_mapping[index] = key_id;
    }
    key_id = loc_default_mapping_L[index];
    if ((key_id >= HUGOC_FPS) && (key_id <= HUGOC_SCREEN)) {
      psp_kbd_mapping_L[index] = key_id;
    }
    key_id = loc_default_mapping_R[index];
    if ((key_id >= HUGOC_FPS) && (key_id <= HUGOC_SCREEN)) {
      psp_kbd_mapping_R[index] = key_id;
    }
  }
  return 0;
}

int
psp_kbd_load_mapping(char *kbd_filename)
{
  FILE    *KbdFile;
  int      error = 0;
  
  KbdFile = fopen(kbd_filename, "r");
  error   = 1;

  if (KbdFile != (FILE*)0) {
    psp_kbd_load_mapping_file(KbdFile);
    error = 0;
    fclose(KbdFile);
  }

  kbd_ltrigger_mapping_active = 0;
  kbd_rtrigger_mapping_active = 0;
    
  return error;
}

int
psp_kbd_load_mapping_file(FILE *KbdFile)
{
  char     Buffer[512];
  char    *Scan;
  int      tmp_mapping[KBD_ALL_BUTTONS];
  int      tmp_mapping_L[KBD_ALL_BUTTONS];
  int      tmp_mapping_R[KBD_ALL_BUTTONS];
  int      hugo_key_id = 0;
  int      kbd_id = 0;

  memcpy(tmp_mapping, loc_default_mapping, sizeof(loc_default_mapping));
  memcpy(tmp_mapping_L, loc_default_mapping_L, sizeof(loc_default_mapping_L));
  memcpy(tmp_mapping_R, loc_default_mapping_R, sizeof(loc_default_mapping_R));

  while (fgets(Buffer,512,KbdFile) != (char *)0) {
      
      Scan = strchr(Buffer,'\n');
      if (Scan) *Scan = '\0';
      /* For this #@$% of windows ! */
      Scan = strchr(Buffer,'\r');
      if (Scan) *Scan = '\0';
      if (Buffer[0] == '#') continue;

      Scan = strchr(Buffer,'=');
      if (! Scan) continue;
    
      *Scan = '\0';
      hugo_key_id = atoi(Scan + 1);

      for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++) {
        if (!strcasecmp(Buffer, kbd_button_name[kbd_id])) {
          tmp_mapping[kbd_id] = hugo_key_id;
          //break;
        }
      }
      for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++) {
        if (!strcasecmp(Buffer,kbd_button_name_L[kbd_id])) {
          tmp_mapping_L[kbd_id] = hugo_key_id;
          //break;
        }
      }
      for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++) {
        if (!strcasecmp(Buffer,kbd_button_name_R[kbd_id])) {
          tmp_mapping_R[kbd_id] = hugo_key_id;
          //break;
        }
      }
  }

  memcpy(psp_kbd_mapping, tmp_mapping, sizeof(psp_kbd_mapping));
  memcpy(psp_kbd_mapping_L, tmp_mapping_L, sizeof(psp_kbd_mapping_L));
  memcpy(psp_kbd_mapping_R, tmp_mapping_R, sizeof(psp_kbd_mapping_R));
  
  return 0;
}

int
psp_kbd_save_mapping(char *kbd_filename)
{
  FILE    *KbdFile;
  int      kbd_id = 0;
  int      error = 0;

  KbdFile = fopen(kbd_filename, "w");
  error   = 1;

  if (KbdFile != (FILE*)0) {

    for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++)
    {
      fprintf(KbdFile, "%s=%d\n", kbd_button_name[kbd_id], psp_kbd_mapping[kbd_id]);
    }
    for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++)
    {
      fprintf(KbdFile, "%s=%d\n", kbd_button_name_L[kbd_id], psp_kbd_mapping_L[kbd_id]);
    }
    for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++)
    {
      fprintf(KbdFile, "%s=%d\n", kbd_button_name_R[kbd_id], psp_kbd_mapping_R[kbd_id]);
    }
    error = 0;
    fclose(KbdFile);
  }

  return error;
}

int 
psp_kbd_is_danzeff_mode()
{
  return danzeff_mode;
}

int
psp_kbd_enter_danzeff()
{
  unsigned int danzeff_key = 0;
  int          hugo_key    = 0;
  int          key_code    = HUGO_JOY1_SELECT;
  SceCtrlData  c;

  if (! danzeff_mode) {
    psp_init_keyboard();
    danzeff_mode = 1;
  }

  sceCtrlPeekBufferPositive(&c, 1);
  c.Buttons &= PSP_ALL_BUTTON_MASK;

  if (danzeff_hugo_pending) 
  {
    if ((c.TimeStamp - loc_last_event_time) > KBD_MIN_PENDING_TIME) {
      loc_last_event_time = c.TimeStamp;
      danzeff_hugo_pending = 0;
      hugo_key_event(danzeff_hugo_key, 0);
    }

    return 0;
  }

  if ((c.TimeStamp - loc_last_event_time) > KBD_MIN_DANZEFF_TIME) {
    loc_last_event_time = c.TimeStamp;
  
    sceCtrlPeekBufferPositive(&c, 1);
    c.Buttons &= PSP_ALL_BUTTON_MASK;
# ifdef USE_PSP_IRKEYB
    psp_irkeyb_set_psp_key(&c);
# endif
    danzeff_key = danzeff_readInput(c);
  }

  if (danzeff_key > DANZEFF_START) {
    hugo_key = hugo_get_key_from_ascii(danzeff_key);

    if (hugo_key != -1) {
      danzeff_hugo_key     = hugo_key;
      danzeff_hugo_pending = 10;
      hugo_key_event(danzeff_hugo_key, 1);
    }

    return 1;

  } else if (danzeff_key == DANZEFF_START) {
    danzeff_mode          = 0;
    danzeff_hugo_pending = 0;
    danzeff_hugo_key     = 0;

    psp_kbd_wait_no_button();

  } else if (danzeff_key == DANZEFF_SELECT) {
    danzeff_mode          = 0;
    danzeff_hugo_pending = 0;
    danzeff_hugo_key     = 0;
    psp_main_menu();
    psp_init_keyboard();

    psp_kbd_wait_no_button();
  }

  return 0;
}

#ifdef USE_PSP_IRKEYB
int
psp_kbd_enter_irkeyb()
{
  int hugo_key   = 0;
  int psp_irkeyb = PSP_IRKEYB_EMPTY;

  SceCtrlData  c;
  sceCtrlPeekBufferPositive(&c, 1);

  if (irkeyb_hugo_pending) 
  {
    if ((c.TimeStamp - loc_last_irkbd_event_time) > KBD_MIN_PENDING_TIME) {
      loc_last_irkbd_event_time = c.TimeStamp;
      irkeyb_hugo_pending = 0;
      hugo_key_event(irkeyb_hugo_key, 0);
    }
    return 0;
  }

  psp_irkeyb = psp_irkeyb_read_key();
  if (psp_irkeyb != PSP_IRKEYB_EMPTY) {

    if (psp_irkeyb == PSP_IRKEYB_UP) {
      hugo_key = HUGO_JOY1_UP;
    } else
    if (psp_irkeyb == PSP_IRKEYB_DOWN) {
      hugo_key = HUGO_JOY1_DOWN;
    } else
    if (psp_irkeyb == PSP_IRKEYB_LEFT) {
      hugo_key = HUGO_JOY1_LEFT;
    } else
    if (psp_irkeyb == PSP_IRKEYB_RIGHT) {
      hugo_key = HUGO_JOY1_RIGHT;
    } else {
      hugo_key = hugo_get_key_from_ascii(psp_irkeyb);
    }
    if (hugo_key != -1) {
      irkeyb_hugo_key     = hugo_key;
      irkeyb_hugo_pending = 1;
      hugo_key_event(hugo_key, 1);
    }
    return 1;
  }
  return 0;
}
# endif

int
hugo_decode_key(int psp_b, int button_pressed)
{
  int wake = 0;
  int reverse_analog = HUGO.psp_reverse_analog;

  if (reverse_analog) {
    if ((psp_b >= KBD_JOY_UP  ) &&
        (psp_b <= KBD_JOY_LEFT)) {
      psp_b = psp_b - KBD_JOY_UP + KBD_UP;
    } else
    if ((psp_b >= KBD_UP  ) &&
        (psp_b <= KBD_LEFT)) {
      psp_b = psp_b - KBD_UP + KBD_JOY_UP;
    }
  }

  if (psp_b == KBD_START) {
     if (button_pressed) psp_kbd_enter_danzeff();
  } else
  if (psp_b == KBD_SELECT) {
    if (button_pressed) {
      psp_main_menu();
      psp_init_keyboard();
    }
  } else {
 
    if (psp_kbd_mapping[psp_b] >= 0) {
      wake = 1;
      if (button_pressed) {
        // Determine which buton to press first (ie which mapping is currently active)
        if (kbd_ltrigger_mapping_active) {
          // Use ltrigger mapping
          psp_kbd_presses[psp_b] = psp_kbd_mapping_L[psp_b];
          hugo_key_event(psp_kbd_presses[psp_b], button_pressed);
        } else
        if (kbd_rtrigger_mapping_active) {
          // Use rtrigger mapping
          psp_kbd_presses[psp_b] = psp_kbd_mapping_R[psp_b];
          hugo_key_event(psp_kbd_presses[psp_b], button_pressed);
        } else {
          // Use standard mapping
          psp_kbd_presses[psp_b] = psp_kbd_mapping[psp_b];
          hugo_key_event(psp_kbd_presses[psp_b], button_pressed);
        }
      } else {
          // Determine which button to release (ie what was pressed before)
          hugo_key_event(psp_kbd_presses[psp_b], button_pressed);
      }

    } else {
      if (psp_kbd_mapping[psp_b] == KBD_LTRIGGER_MAPPING) {
        kbd_ltrigger_mapping_active = button_pressed;
        kbd_rtrigger_mapping_active = 0;
      } else
      if (psp_kbd_mapping[psp_b] == KBD_RTRIGGER_MAPPING) {
        kbd_rtrigger_mapping_active = button_pressed;
        kbd_ltrigger_mapping_active = 0;
      }
    }
  }
  return 0;
}

# define ANALOG_THRESHOLD 60

void 
kbd_get_analog_direction(int Analog_x, int Analog_y, int *x, int *y)
{
  int DeltaX = 255;
  int DeltaY = 255;
  int DirX   = 0;
  int DirY   = 0;

  *x = 0;
  *y = 0;

  if (Analog_x <=        ANALOG_THRESHOLD)  { DeltaX = Analog_x; DirX = -1; }
  else 
  if (Analog_x >= (255 - ANALOG_THRESHOLD)) { DeltaX = 255 - Analog_x; DirX = 1; }

  if (Analog_y <=        ANALOG_THRESHOLD)  { DeltaY = Analog_y; DirY = -1; }
  else 
  if (Analog_y >= (255 - ANALOG_THRESHOLD)) { DeltaY = 255 - Analog_y; DirY = 1; }

  *x = DirX;
  *y = DirY;
}

void
kbd_change_auto_fire(int auto_fire)
{
  HUGO.hugo_auto_fire = auto_fire;
  if (HUGO.hugo_auto_fire_pressed) {
    if (HUGO.psp_active_joystick) {
      hugo_key_event(HUGO_JOY2_B, 0);
    } else {
      hugo_key_event(HUGO_JOY1_B, 0);
    }
    HUGO.hugo_auto_fire_pressed = 0;
  }
}


static int 
kbd_reset_button_status(void)
{
  int b = 0;
  /* Reset Button status */
  for (b = 0; b < KBD_MAX_BUTTONS; b++) {
    loc_button_press[b]   = 0;
    loc_button_release[b] = 0;
  }
  psp_init_keyboard();
  return 0;
}

int
kbd_scan_keyboard(void)
{
  SceCtrlData c;
  long        delta_stamp;
  int         event;
  int         b;

  int new_Lx;
  int new_Ly;
  int old_Lx;
  int old_Ly;

  event = 0;
  myCtrlFastPeekBufferPositive( &c, 1 );
  c.Buttons &= PSP_ALL_BUTTON_MASK;

# ifdef USE_PSP_IRKEYB
  psp_irkeyb_set_psp_key(&c);
# endif
# ifdef USE_IRDA_JOY
  if (HUGO.psp_irdajoy_type != IRDAJOY_TYPE_NONE) {
    psp_irda_get_joy_event( &loc_irda_joy_event );
    psp_irda_joy_convert( &loc_irda_joy_event, &c );
  }
# endif

  if ((c.Buttons & (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) ==
      (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) {
    /* Exit ! */
    psp_sdl_exit(0);
  }

  delta_stamp = c.TimeStamp - first_time_stamp;
  if ((delta_stamp < 0) || (delta_stamp > KBD_MIN_BATTCHECK_TIME)) {
    first_time_stamp = c.TimeStamp;
    if (psp_is_low_battery()) {
      psp_main_menu();
      psp_init_keyboard();
      return 0;
    }
  }

  if (HUGO.hugo_auto_fire) {
    delta_stamp = c.TimeStamp - first_time_auto_stamp;
    if ((delta_stamp < 0) || 
        (delta_stamp > (KBD_MIN_AUTOFIRE_TIME / (1 + HUGO.hugo_auto_fire_period)))) {
      first_time_auto_stamp = c.TimeStamp;

      if (HUGO.psp_active_joystick) {
        hugo_key_event(HUGO_JOY2_B, HUGO.hugo_auto_fire_pressed);
      } else {
        hugo_key_event(HUGO_JOY1_B, HUGO.hugo_auto_fire_pressed);
      }
      HUGO.hugo_auto_fire_pressed = ! HUGO.hugo_auto_fire_pressed;
    }

  } else
  if (delta_stamp < KBD_MAX_EVENT_TIME) {
    return 0;
  }

  /* Check Analog Device */
  kbd_get_analog_direction(loc_button_data.Lx,loc_button_data.Ly,&old_Lx,&old_Ly);
  kbd_get_analog_direction( c.Lx, c.Ly, &new_Lx, &new_Ly);

  /* Analog device has moved */
  if (new_Lx > 0) {
    if (old_Lx  > 0) hugo_decode_key(KBD_JOY_LEFT , 0);
    hugo_decode_key(KBD_JOY_RIGHT, 1);
  } else 
  if (new_Lx < 0) {
    if (old_Lx  < 0) hugo_decode_key(KBD_JOY_RIGHT, 0);
    hugo_decode_key(KBD_JOY_LEFT , 1);
  } else {
    if (old_Lx  > 0) hugo_decode_key(KBD_JOY_LEFT , 0);
    else
    if (old_Lx  < 0) hugo_decode_key(KBD_JOY_RIGHT, 0);
    else {
      hugo_decode_key(KBD_JOY_LEFT  , 0);
      hugo_decode_key(KBD_JOY_RIGHT , 0);
    }
  }

  if (new_Ly < 0) {
    if (old_Ly  > 0) hugo_decode_key(KBD_JOY_DOWN , 0);
    hugo_decode_key(KBD_JOY_UP   , 1);

  } else 
  if (new_Ly > 0) {
    if (old_Ly  < 0) hugo_decode_key(KBD_JOY_UP   , 0);
    hugo_decode_key(KBD_JOY_DOWN , 1);

  } else {
    if (old_Ly  > 0) hugo_decode_key(KBD_JOY_DOWN , 0);
    else
    if (old_Ly  < 0) hugo_decode_key(KBD_JOY_UP   , 0);
    else {
      hugo_decode_key(KBD_JOY_DOWN , 0);
      hugo_decode_key(KBD_JOY_UP   , 0);
    }
  }

  for (b = 0; b < KBD_MAX_BUTTONS; b++) 
  {
    if (c.Buttons & kbd_button_mask[b]) {
# if 0 // GAME MODE !
      if (!(loc_button_data.Buttons & kbd_button_mask[b])) 
# endif
      {
        loc_button_press[b] = 1;
        event = 1;
      }
    } else {
      if (loc_button_data.Buttons & kbd_button_mask[b]) {
        loc_button_release[b] = 1;
        loc_button_press[b] = 0;
        event = 1;
      }
    }
  }
  memcpy(&loc_button_data,&c,sizeof(SceCtrlData));

  return event;
}

void
kbd_wait_start(void)
{
  while (1)
  {
    SceCtrlData c;
    sceCtrlReadBufferPositive(&c, 1);
    c.Buttons &= PSP_ALL_BUTTON_MASK;
    if (c.Buttons & PSP_CTRL_START) break;
  }
  psp_kbd_wait_no_button();
}

void
psp_init_keyboard(void)
{
  hugo_kbd_reset();
  kbd_ltrigger_mapping_active = 0;
  kbd_rtrigger_mapping_active = 0;
}

void
psp_kbd_wait_no_button(void)
{
  SceCtrlData c;

  do {
   myCtrlPeekBufferPositive(&c, 1);
   c.Buttons &= PSP_ALL_BUTTON_MASK;

  } while (c.Buttons != 0);
} 

void
psp_kbd_wait_button(void)
{
  SceCtrlData c;

  do {
   sceCtrlReadBufferPositive(&c, 1);
   c.Buttons &= PSP_ALL_BUTTON_MASK;
  } while (c.Buttons == 0);
} 

int
psp_update_keys(void)
{
  int         b;

  static char first_time = 1;
  static int release_pending = 0;

  if (first_time) {

    SceCtrlData c;
    sceCtrlPeekBufferPositive(&c, 1);
    c.Buttons &= PSP_ALL_BUTTON_MASK;

    if (first_time_stamp == -1) first_time_stamp = c.TimeStamp;

    first_time      = 0;
    release_pending = 0;

    for (b = 0; b < KBD_MAX_BUTTONS; b++) {
      loc_button_release[b] = 0;
      loc_button_press[b] = 0;
    }
    sceCtrlPeekBufferPositive(&loc_button_data, 1);
    loc_button_data.Buttons &= PSP_ALL_BUTTON_MASK;

    psp_main_menu();
    psp_init_keyboard();

    return 0;
  }

  hugo_apply_cheats();

  if (danzeff_mode) {
    psp_kbd_enter_danzeff();
    return 0;
  }

# ifdef USE_PSP_IRKEYB
  if (psp_kbd_enter_irkeyb()) {
    return 1;
  }
# endif

  if (release_pending)
  {
    release_pending = 0;
    for (b = 0; b < KBD_MAX_BUTTONS; b++) {
      if (loc_button_release[b]) {
        loc_button_release[b] = 0;
        loc_button_press[b] = 0;
        hugo_decode_key(b, 0);
      }
    }
  }

  kbd_scan_keyboard();

  /* check press event */
  for (b = 0; b < KBD_MAX_BUTTONS; b++) {
    if (loc_button_press[b]) {
      loc_button_press[b] = 0;
      release_pending     = 0;
      hugo_decode_key(b, 1);
    }
  }
  /* check release event */
  for (b = 0; b < KBD_MAX_BUTTONS; b++) {
    if (loc_button_release[b]) {
      release_pending = 1;
      break;
    }
  }

  return 0;
}
