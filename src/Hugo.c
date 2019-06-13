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
#include <string.h>
#include <sys/stat.h>
#include <psppower.h>
#include <SDL/SDL.h>
#include <pspctrl.h>
#include <psptypes.h>
#include <png.h>

#include "global.h"
#include "Hugo.h"
#include "miniunz.h"
#include "psp_sdl.h"
#include "psp_kbd.h"
#include "psp_menu.h"
#include "psp_fmgr.h"

#include <pspctrl.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspiofilemgr.h>


#include "hard_pce.h"

  Hugo_t HUGO;

  int psp_screenshot_mode = 0;
  int psp_irda_mode = 0;
  int psp_exit_now  = 0;


/* Resolution: 256x212, 320x256 and 512x256  */

static inline void
loc_split_rect( SDL_Rect* r )
{
  if (r->x < 0) r->x = 0;
  if (r->y < 0) r->y = 0;
  if (r->w > 480) r->w = 480;
  if (r->h > 272) r->h = 272;
}


static void
loc_put_image_normal()
{
  SDL_Rect srcRect;
  SDL_Rect dstRect;

  srcRect.w = io.screen_w;
  srcRect.h = io.screen_h;
  srcRect.x = 0;
  srcRect.y = 0;

  dstRect.w = io.screen_w;
  dstRect.h = io.screen_h;
  dstRect.x = (480 - dstRect.w) / 2;
  dstRect.y = (272 - dstRect.h) / 2;

  loc_split_rect( &dstRect );
  psp_sdl_gu_stretch(&srcRect, &dstRect);
}

static void
loc_put_image_x125()
{
  SDL_Rect srcRect;
  SDL_Rect dstRect;

  srcRect.w = io.screen_w;
  srcRect.h = io.screen_h;
  srcRect.x = 0;
  srcRect.y = 0;

  /* 1.25 */
  dstRect.w = io.screen_w + (io.screen_w >> 2);
  dstRect.h = 272;
  dstRect.x = (480 - dstRect.w) / 2;
  dstRect.y = 0;

  loc_split_rect( &dstRect );
  psp_sdl_gu_stretch(&srcRect, &dstRect);
}

static void
loc_put_image_x15()
{
  SDL_Rect srcRect;
  SDL_Rect dstRect;

  srcRect.x = 0;
  srcRect.y = 10 + HUGO.hugo_delta_y;
  srcRect.w = io.screen_w;
  srcRect.h = 220;

  dstRect.w = io.screen_w + (io.screen_w >> 1);
  dstRect.h = 272;
  dstRect.x = (480 - dstRect.w) / 2;
  dstRect.y = 0;

  loc_split_rect( &dstRect );
  psp_sdl_gu_stretch(&srcRect, &dstRect);
}

static void
loc_put_image_x175()
{
  SDL_Rect srcRect;
  SDL_Rect dstRect;

  srcRect.x = 0;
  srcRect.y = 10 + HUGO.hugo_delta_y;
  srcRect.w = io.screen_w;
  srcRect.h = 220;

  dstRect.w = (io.screen_w << 1) - (io.screen_w >> 2);
  dstRect.h = 272;
  dstRect.x = (480 - dstRect.w) / 2;
  dstRect.y = 0;

  loc_split_rect( &dstRect );
  psp_sdl_gu_stretch(&srcRect, &dstRect);
}

static void
loc_put_image_max()
{
  SDL_Rect srcRect;
  SDL_Rect dstRect;

  srcRect.x = 0;
  srcRect.y = 0;
  srcRect.w = io.screen_w;
  srcRect.h = io.screen_h;
  dstRect.x = 0;
  dstRect.y = 0;
  dstRect.w = 480;
  dstRect.h = 272;

  loc_split_rect( &dstRect );
  psp_sdl_gu_stretch(&srcRect, &dstRect);
}

void
psp_hugo_wait_vsync()
{
# ifndef LINUX_MODE
  static int loc_pv = 0;
  int cv = sceDisplayGetVcount();
  if (loc_pv == cv) {
    sceDisplayWaitVblankCB();
  }
  loc_pv = sceDisplayGetVcount();
# endif
}

void
hugo_synchronize(void)
{
  static u32 nextclock = 1;

  if (nextclock) {
    u32 curclock;
    do {
     curclock = SDL_GetTicks();
    } while (curclock < nextclock);
    nextclock = curclock + (u32)( 1000 / HUGO.hugo_speed_limiter);
  }
}

void
hugo_update_fps()
{
  static u32 next_sec_clock = 0;
  static u32 cur_num_frame = 0;
  cur_num_frame++;
  u32 curclock = SDL_GetTicks();
  if (curclock > next_sec_clock) {
    next_sec_clock = curclock + 1000;
    HUGO.hugo_current_fps = cur_num_frame;
    cur_num_frame = 0;
  }
}


//Graphic rendering 
void
hugo_render_update()
{
  if (HUGO.psp_skip_cur_frame <= 0) {

    HUGO.psp_skip_cur_frame = HUGO.psp_skip_max_frame;

    if (HUGO.hugo_render_mode == HUGO_RENDER_NORMAL) loc_put_image_normal();
    else
    if (HUGO.hugo_render_mode == HUGO_RENDER_X125) loc_put_image_x125();
    else
    if (HUGO.hugo_render_mode == HUGO_RENDER_X15 ) loc_put_image_x15();
    else
    if (HUGO.hugo_render_mode == HUGO_RENDER_X175) loc_put_image_x175();
    else
    if (HUGO.hugo_render_mode == HUGO_RENDER_MAX   ) loc_put_image_max();

    if (HUGO.hugo_speed_limiter) {
      hugo_synchronize();
    }

    if (HUGO.hugo_view_fps) {
      hugo_update_fps();
    }

    if (psp_kbd_is_danzeff_mode()) {

      sceDisplayWaitVblankStart();

      danzeff_moveTo(-165, -50);
      danzeff_render();
    }

# ifdef USE_IRDA_JOY
    if (HUGO.psp_irdajoy_debug) {
      psp_sdl_fill_print(0, 10, psp_irda_get_debug_string(), 0xffffff, 0 );
    }
# endif
    if (HUGO.hugo_view_fps) {
      char buffer[32];
      sprintf(buffer, "%03d %3d", HUGO.hugo_current_clock, (int)HUGO.hugo_current_fps );
      psp_sdl_fill_print(0, 0, buffer, 0xffffff, 0 );
    }

    if (HUGO.hugo_vsync) {
      psp_hugo_wait_vsync();
    }
    psp_sdl_flip();

    if (psp_screenshot_mode) {
      psp_screenshot_mode--;
      if (psp_screenshot_mode <= 0) {
        psp_audio_pause();
        psp_sdl_save_screenshot();
        psp_audio_resume();
        psp_screenshot_mode = 0;
      }
    }

  } else if (HUGO.psp_skip_max_frame) {
    HUGO.psp_skip_cur_frame--;

    if (HUGO.hugo_view_fps) {
      hugo_update_fps();
    }
  }
}

void
hugo_update_save_name(char *Name)
{
  char        TmpFileName[MAX_PATH];
# ifdef LINUX_MODE
  struct stat aStat;
# else
  SceIoStat   aStat;
# endif
  int         index;
  char       *SaveName;
  char       *Scan1;
  char       *Scan2;

  SaveName = strrchr(Name,'/');
  if (SaveName != (char *)0) SaveName++;
  else                       SaveName = Name;

  if (!strncasecmp(SaveName, "sav_", 4)) {
    Scan1 = SaveName + 4;
    Scan2 = strrchr(Scan1, '_');
    if (Scan2 && (Scan2[1] >= '0') && (Scan2[1] <= '5')) {
      strncpy(HUGO.hugo_save_name, Scan1, MAX_PATH);
      HUGO.hugo_save_name[Scan2 - Scan1] = '\0';
    } else {
      strncpy(HUGO.hugo_save_name, SaveName, MAX_PATH);
    }
  } else {
    strncpy(HUGO.hugo_save_name, SaveName, MAX_PATH);
  }

  if (HUGO.hugo_save_name[0] == '\0') {
    strcpy(HUGO.hugo_save_name,"default");
  }

  for (index = 0; index < HUGO_MAX_SAVE_STATE; index++) {
    HUGO.hugo_save_state[index].used  = 0;
    memset(&HUGO.hugo_save_state[index].date, 0, sizeof(ScePspDateTime));
    HUGO.hugo_save_state[index].thumb = 0;

    snprintf(TmpFileName, MAX_PATH, "%s/save/sav_%s_%d.stz", HUGO.hugo_home_dir, HUGO.hugo_save_name, index);
# ifdef LINUX_MODE
    if (! stat(TmpFileName, &aStat)) 
# else
    if (! sceIoGetstat(TmpFileName, &aStat))
# endif
    {
      HUGO.hugo_save_state[index].used = 1;
      HUGO.hugo_save_state[index].date = aStat.st_mtime;
      snprintf(TmpFileName, MAX_PATH, "%s/save/sav_%s_%d.png", HUGO.hugo_home_dir, HUGO.hugo_save_name, index);
# ifdef LINUX_MODE
      if (! stat(TmpFileName, &aStat)) 
# else
      if (! sceIoGetstat(TmpFileName, &aStat))
# endif
      {
        if (psp_sdl_load_thumb_png(HUGO.hugo_save_state[index].surface, TmpFileName)) {
          HUGO.hugo_save_state[index].thumb = 1;
        }
      }
    }
  }

  HUGO.comment_present = 0;
  snprintf(TmpFileName, MAX_PATH, "%s/txt/%s.txt", HUGO.hugo_home_dir, HUGO.hugo_save_name);
# ifdef LINUX_MODE
  if (! stat(TmpFileName, &aStat)) 
# else
  if (! sceIoGetstat(TmpFileName, &aStat))
# endif
  {
    HUGO.comment_present = 1;
  }
}

void
reset_save_name()
{
  hugo_update_save_name("");
}

typedef struct thumb_list {
  struct thumb_list *next;
  char              *name;
  char              *thumb;
} thumb_list;

static thumb_list* loc_head_thumb = 0;

static void
loc_del_thumb_list()
{
  while (loc_head_thumb != 0) {
    thumb_list *del_elem = loc_head_thumb;
    loc_head_thumb = loc_head_thumb->next;
    if (del_elem->name) free( del_elem->name );
    if (del_elem->thumb) free( del_elem->thumb );
    free(del_elem);
  }
}

static void
loc_add_thumb_list(char* filename)
{
  thumb_list *new_elem;
  char tmp_filename[MAX_PATH];

  strcpy(tmp_filename, filename);
  char* save_name = tmp_filename;

  /* .png extention */
  char* Scan = strrchr(save_name, '.');
  if ((! Scan) || (strcasecmp(Scan, ".png"))) return;
  *Scan = 0;

  if (strncasecmp(save_name, "sav_", 4)) return;
  save_name += 4;

  Scan = strrchr(save_name, '_');
  if (! Scan) return;
  *Scan = 0;

  /* only one png for a given save name */
  new_elem = loc_head_thumb;
  while (new_elem != 0) {
    if (! strcasecmp(new_elem->name, save_name)) return;
    new_elem = new_elem->next;
  }

  new_elem = (thumb_list *)malloc( sizeof( thumb_list ) );
  new_elem->next = loc_head_thumb;
  loc_head_thumb = new_elem;
  new_elem->name  = strdup( save_name );
  new_elem->thumb = strdup( filename );
}

void
load_thumb_list()
{
# ifndef LINUX_MODE
  char SaveDirName[MAX_PATH];
  struct SceIoDirent a_dirent;
  int fd = 0;

  loc_del_thumb_list();

  snprintf(SaveDirName, MAX_PATH, "%s/save", HUGO.hugo_home_dir);
  memset(&a_dirent, 0, sizeof(a_dirent));

  fd = sceIoDopen(SaveDirName);
  if (fd < 0) return;

  while (sceIoDread(fd, &a_dirent) > 0) {
    if(a_dirent.d_name[0] == '.') continue;
    if(! FIO_S_ISDIR(a_dirent.d_stat.st_mode)) 
    {
      loc_add_thumb_list( a_dirent.d_name );
    }
  }
  sceIoDclose(fd);
# else
  char SaveDirName[MAX_PATH];
  DIR* fd = 0;

  loc_del_thumb_list();

  snprintf(SaveDirName, MAX_PATH, "%s/save", HUGO.hugo_home_dir);

  fd = opendir(SaveDirName);
  if (!fd) return;

  struct dirent *a_dirent;
  while ((a_dirent = readdir(fd)) != 0) {
    if(a_dirent->d_name[0] == '.') continue;
    if (a_dirent->d_type != DT_DIR) 
    {
      loc_add_thumb_list( a_dirent->d_name );
    }
  }
  closedir(fd);
# endif
}

int
load_thumb_if_exists(char *Name)
{
  char        FileName[MAX_PATH];
  char        ThumbFileName[MAX_PATH];
# ifdef LINUX_MODE
  struct stat aStat;
# else
  SceIoStat   aStat;
# endif
  char       *SaveName;
  char       *Scan;

  strcpy(FileName, Name);
  SaveName = strrchr(FileName,'/');
  if (SaveName != (char *)0) SaveName++;
  else                       SaveName = FileName;

  Scan = strrchr(SaveName,'.');
  if (Scan) *Scan = '\0';

  if (!SaveName[0]) return 0;

  thumb_list *scan_list = loc_head_thumb;
  while (scan_list != 0) {
    if (! strcasecmp( SaveName, scan_list->name)) {
      snprintf(ThumbFileName, MAX_PATH, "%s/save/%s", HUGO.hugo_home_dir, scan_list->thumb);
# ifdef LINUX_MODE
      if (! stat(ThumbFileName, &aStat)) 
# else
      if (! sceIoGetstat(ThumbFileName, &aStat))
# endif
      {
        if (psp_sdl_load_thumb_png(save_surface, ThumbFileName)) {
          return 1;
        }
      }
    }
    scan_list = scan_list->next;
  }
  return 0;
}

typedef struct comment_list {
  struct comment_list *next;
  char              *name;
  char              *filename;
} comment_list;

static comment_list* loc_head_comment = 0;

static void
loc_del_comment_list()
{
  while (loc_head_comment != 0) {
    comment_list *del_elem = loc_head_comment;
    loc_head_comment = loc_head_comment->next;
    if (del_elem->name) free( del_elem->name );
    if (del_elem->filename) free( del_elem->filename );
    free(del_elem);
  }
}

static void
loc_add_comment_list(char* filename)
{
  comment_list *new_elem;
  char  tmp_filename[MAX_PATH];

  strcpy(tmp_filename, filename);
  char* save_name = tmp_filename;

  /* .png extention */
  char* Scan = strrchr(save_name, '.');
  if ((! Scan) || (strcasecmp(Scan, ".txt"))) return;
  *Scan = 0;

  /* only one txt for a given save name */
  new_elem = loc_head_comment;
  while (new_elem != 0) {
    if (! strcasecmp(new_elem->name, save_name)) return;
    new_elem = new_elem->next;
  }

  new_elem = (comment_list *)malloc( sizeof( comment_list ) );
  new_elem->next = loc_head_comment;
  loc_head_comment = new_elem;
  new_elem->name  = strdup( save_name );
  new_elem->filename = strdup( filename );
}

void
load_comment_list()
{
# ifndef LINUX_MODE
  char SaveDirName[MAX_PATH];
  struct SceIoDirent a_dirent;
  int fd = 0;

  loc_del_comment_list();

  snprintf(SaveDirName, MAX_PATH, "%s/txt", HUGO.hugo_home_dir);
  memset(&a_dirent, 0, sizeof(a_dirent));

  fd = sceIoDopen(SaveDirName);
  if (fd < 0) return;

  while (sceIoDread(fd, &a_dirent) > 0) {
    if(a_dirent.d_name[0] == '.') continue;
    if(! FIO_S_ISDIR(a_dirent.d_stat.st_mode)) 
    {
      loc_add_comment_list( a_dirent.d_name );
    }
  }
  sceIoDclose(fd);
# else
  char SaveDirName[MAX_PATH];
  DIR* fd = 0;

  loc_del_comment_list();

  snprintf(SaveDirName, MAX_PATH, "%s/txt", HUGO.hugo_home_dir);

  fd = opendir(SaveDirName);
  if (!fd) return;

  struct dirent *a_dirent;
  while ((a_dirent = readdir(fd)) != 0) {
    if(a_dirent->d_name[0] == '.') continue;
    if (a_dirent->d_type != DT_DIR) 
    {
      loc_add_comment_list( a_dirent->d_name );
    }
  }
  closedir(fd);
# endif
}

char*
load_comment_if_exists(char *Name)
{
static char loc_comment_buffer[128];

  char        FileName[MAX_PATH];
  char        TmpFileName[MAX_PATH];
  FILE       *a_file;
  char       *SaveName;
  char       *Scan;

  loc_comment_buffer[0] = 0;

  strcpy(FileName, Name);
  SaveName = strrchr(FileName,'/');
  if (SaveName != (char *)0) SaveName++;
  else                       SaveName = FileName;

  Scan = strrchr(SaveName,'.');
  if (Scan) *Scan = '\0';

  if (!SaveName[0]) return 0;

  comment_list *scan_list = loc_head_comment;
  while (scan_list != 0) {
    if (! strcasecmp( SaveName, scan_list->name)) {
      snprintf(TmpFileName, MAX_PATH, "%s/txt/%s", HUGO.hugo_home_dir, scan_list->filename);
      a_file = fopen(TmpFileName, "r");
      if (a_file) {
        char* a_scan = 0;
        loc_comment_buffer[0] = 0;
        if (fgets(loc_comment_buffer, 60, a_file) != 0) {
          a_scan = strchr(loc_comment_buffer, '\n');
          if (a_scan) *a_scan = '\0';
          /* For this #@$% of windows ! */
          a_scan = strchr(loc_comment_buffer,'\r');
          if (a_scan) *a_scan = '\0';
          fclose(a_file);
          return loc_comment_buffer;
        }
        fclose(a_file);
        return 0;
      }
    }
    scan_list = scan_list->next;
  }
  return 0;
}

void
hugo_kbd_load(void)
{
  char        TmpFileName[MAX_PATH + 1];
  struct stat aStat;

  snprintf(TmpFileName, MAX_PATH, "%s/kbd/%s.kbd", HUGO.hugo_home_dir, HUGO.hugo_save_name );
  if (! stat(TmpFileName, &aStat)) {
    psp_kbd_load_mapping(TmpFileName);
  }
}

int
hugo_kbd_save(void)
{
  char TmpFileName[MAX_PATH + 1];
  snprintf(TmpFileName, MAX_PATH, "%s/kbd/%s.kbd", HUGO.hugo_home_dir, HUGO.hugo_save_name );
  return( psp_kbd_save_mapping(TmpFileName) );
}

void
hugo_joy_load(void)
{
  char        TmpFileName[MAX_PATH + 1];
  struct stat aStat;

  snprintf(TmpFileName, MAX_PATH, "%s/joy/%s.joy", HUGO.hugo_home_dir, HUGO.hugo_save_name );
  if (! stat(TmpFileName, &aStat)) {
    psp_joy_load_settings(TmpFileName);
  }
}

int
hugo_joy_save(void)
{
  char TmpFileName[MAX_PATH + 1];
  snprintf(TmpFileName, MAX_PATH, "%s/joy/%s.joy", HUGO.hugo_home_dir, HUGO.hugo_save_name );
  return( psp_joy_save_settings(TmpFileName) );
}

void
hugo_emulator_reset(void)
{
  main_hugo_emulator_reset();
}

void
hugo_default_settings()
{
  HUGO.hugo_cdaudio_enable    = 1;
  HUGO.hugo_snd_enable        = 1;
  HUGO.hugo_snd_volume        = 100;
  HUGO.hugo_snd_freq          = 3; /* stereo 44k */
  HUGO.hugo_render_mode       = HUGO_RENDER_X125;
  HUGO.hugo_vsync             = 0;
  HUGO.hugo_overclock         = 16;
  HUGO.hugo_speed_limiter     = 60;
  HUGO.psp_cpu_clock          = 266;
  HUGO.psp_cpu_cdclock        = 300;
  HUGO.psp_screenshot_id      = 0;
  HUGO.hugo_view_fps          = 0;
  HUGO.hugo_delta_y           = 0;

  myPowerSetClockFrequency(HUGO.psp_cpu_clock);
  h6280_set_overclock(HUGO.hugo_overclock);
  osd_psp_set_sound_freq(HUGO.hugo_snd_freq);
}

static int
loc_hugo_save_settings(char *chFileName)
{
  FILE* FileDesc;
  int   error = 0;

  FileDesc = fopen(chFileName, "w");
  if (FileDesc != (FILE *)0 ) {

    fprintf(FileDesc, "psp_cpu_clock=%d\n"      , HUGO.psp_cpu_clock);
    fprintf(FileDesc, "psp_cpu_cdclock=%d\n"    , HUGO.psp_cpu_cdclock);
    fprintf(FileDesc, "psp_skip_max_frame=%d\n" , HUGO.psp_skip_max_frame);
    fprintf(FileDesc, "hugo_view_fps=%d\n"     , HUGO.hugo_view_fps);
    fprintf(FileDesc, "hugo_delta_y=%d\n"      , HUGO.hugo_delta_y);
    fprintf(FileDesc, "hugo_snd_enable=%d\n"   , HUGO.hugo_snd_enable);
    fprintf(FileDesc, "hugo_cdaudio_enable=%d\n", HUGO.hugo_cdaudio_enable);
    fprintf(FileDesc, "hugo_snd_freq=%d\n"   , HUGO.hugo_snd_freq);
    fprintf(FileDesc, "hugo_overclock=%d\n"    , HUGO.hugo_overclock);
    fprintf(FileDesc, "hugo_snd_volume=%d\n"   , HUGO.hugo_snd_volume);
    fprintf(FileDesc, "hugo_render_mode=%d\n"  , HUGO.hugo_render_mode);
    fprintf(FileDesc, "hugo_vsync=%d\n"        , HUGO.hugo_vsync);
    fprintf(FileDesc, "hugo_speed_limiter=%d\n", HUGO.hugo_speed_limiter);

    fclose(FileDesc);

  } else {
    error = 1;
  }

  return error;
}

int
hugo_save_settings(void)
{
  char  FileName[MAX_PATH+1];
  int   error;

  error = 1;

  snprintf(FileName, MAX_PATH, "%s/set/%s.set", HUGO.hugo_home_dir, HUGO.hugo_save_name);
  error = loc_hugo_save_settings(FileName);

  return error;
}

static int
loc_hugo_load_settings(char *chFileName)
{
  char  Buffer[512];
  char *Scan;
  unsigned int Value;
  FILE* FileDesc;

  FileDesc = fopen(chFileName, "r");
  if (FileDesc == (FILE *)0 ) return 0;

  while (fgets(Buffer,512, FileDesc) != (char *)0) {

    Scan = strchr(Buffer,'\n');
    if (Scan) *Scan = '\0';
    /* For this #@$% of windows ! */
    Scan = strchr(Buffer,'\r');
    if (Scan) *Scan = '\0';
    if (Buffer[0] == '#') continue;

    Scan = strchr(Buffer,'=');
    if (! Scan) continue;

    *Scan = '\0';
    Value = atoi(Scan+1);

    if (!strcasecmp(Buffer,"psp_cpu_clock"))      HUGO.psp_cpu_clock = Value;
    else
    if (!strcasecmp(Buffer,"psp_cpu_cdclock"))    HUGO.psp_cpu_cdclock = Value;
    else
    if (!strcasecmp(Buffer,"hugo_view_fps"))     HUGO.hugo_view_fps = Value;
    else
    if (!strcasecmp(Buffer,"hugo_delta_y"))     HUGO.hugo_delta_y = Value;
    else
    if (!strcasecmp(Buffer,"psp_skip_max_frame")) HUGO.psp_skip_max_frame = Value;
    else
    if (!strcasecmp(Buffer,"hugo_cdaudio_enable"))   HUGO.hugo_cdaudio_enable = Value;
    else
    if (!strcasecmp(Buffer,"hugo_snd_enable"))   HUGO.hugo_snd_enable = Value;
    else
    if (!strcasecmp(Buffer,"hugo_snd_freq"))   HUGO.hugo_snd_freq = Value;
    else
    if (!strcasecmp(Buffer,"hugo_overclock"))   HUGO.hugo_overclock = Value;
    else
    if (!strcasecmp(Buffer,"hugo_snd_volume"))   HUGO.hugo_snd_volume = Value;
    else
    if (!strcasecmp(Buffer,"hugo_render_mode"))  HUGO.hugo_render_mode = Value;
    else
    if (!strcasecmp(Buffer,"hugo_vsync"))  HUGO.hugo_vsync = Value;
    else
    if (!strcasecmp(Buffer,"hugo_speed_limiter"))  HUGO.hugo_speed_limiter = Value;
  }

  fclose(FileDesc);

  myPowerSetClockFrequency(HUGO.psp_cpu_clock);
  h6280_set_overclock(HUGO.hugo_overclock);
  osd_psp_set_sound_freq(HUGO.hugo_snd_freq);
  osd_psp_set_sound_volume(HUGO.hugo_snd_volume);

  return 0;
}

int
hugo_load_settings()
{
  char  FileName[MAX_PATH+1];
  int   error;

  error = 1;

  snprintf(FileName, MAX_PATH, "%s/set/%s.set", HUGO.hugo_home_dir, HUGO.hugo_save_name);
  error = loc_hugo_load_settings(FileName);

  return error;
}

int
hugo_load_file_settings(char *FileName)
{
  return loc_hugo_load_settings(FileName);
}

static int
loc_hugo_save_cheat(char *chFileName)
{
  FILE* FileDesc;
  int   cheat_num;
  int   error = 0;

  FileDesc = fopen(chFileName, "w");
  if (FileDesc != (FILE *)0 ) {

    for (cheat_num = 0; cheat_num < HUGO_MAX_CHEAT; cheat_num++) {
      Hugo_cheat_t* a_cheat = &HUGO.hugo_cheat[cheat_num];
      if (a_cheat->type != HUGO_CHEAT_NONE) {
        fprintf(FileDesc, "%d,%x,%x,%s\n", 
                a_cheat->type, a_cheat->addr, a_cheat->value, a_cheat->comment);
      }
    }
    fclose(FileDesc);

  } else {
    error = 1;
  }

  return error;
}

int
hugo_save_cheat(void)
{
  char  FileName[MAX_PATH+1];
  int   error;

  error = 1;

  snprintf(FileName, MAX_PATH, "%s/cht/%s.cht", HUGO.hugo_home_dir, HUGO.hugo_save_name);
  error = loc_hugo_save_cheat(FileName);

  return error;
}

static int
loc_hugo_load_cheat(char *chFileName)
{
  char  Buffer[512];
  char *Scan;
  char *Field;
  unsigned int  cheat_addr;
  unsigned int  cheat_value;
  unsigned int  cheat_type;
  char         *cheat_comment;
  int           cheat_num;
  FILE* FileDesc;

  memset(HUGO.hugo_cheat, 0, sizeof(HUGO.hugo_cheat));
  cheat_num = 0;

  FileDesc = fopen(chFileName, "r");
  if (FileDesc == (FILE *)0 ) return 0;

  while (fgets(Buffer,512, FileDesc) != (char *)0) {

    Scan = strchr(Buffer,'\n');
    if (Scan) *Scan = '\0';
    /* For this #@$% of windows ! */
    Scan = strchr(Buffer,'\r');
    if (Scan) *Scan = '\0';
    if (Buffer[0] == '#') continue;

    /* %d, %x, %x, %s */
    Field = Buffer;
    Scan = strchr(Field, ',');
    if (! Scan) continue;
    *Scan = 0;
    if (sscanf(Field, "%d", &cheat_type) != 1) continue;
    Field = Scan + 1;
    Scan = strchr(Field, ',');
    if (! Scan) continue;
    *Scan = 0;
    if (sscanf(Field, "%x", &cheat_addr) != 1) continue;
    Field = Scan + 1;
    Scan = strchr(Field, ',');
    if (! Scan) continue;
    *Scan = 0;
    if (sscanf(Field, "%x", &cheat_value) != 1) continue;
    Field = Scan + 1;
    cheat_comment = Field;

    if (cheat_type <= HUGO_CHEAT_NONE) continue;

    Hugo_cheat_t* a_cheat = &HUGO.hugo_cheat[cheat_num];

    a_cheat->type  = cheat_type;
    a_cheat->addr  = cheat_addr;
    a_cheat->value = cheat_value;
    strncpy(a_cheat->comment, cheat_comment, sizeof(a_cheat->comment));
    a_cheat->comment[sizeof(a_cheat->comment)-1] = 0;

    if (++cheat_num >= HUGO_MAX_CHEAT) break;
  }
  fclose(FileDesc);

  return 0;
}

int
hugo_load_cheat()
{
  char  FileName[MAX_PATH+1];
  int   error;

  error = 1;

  snprintf(FileName, MAX_PATH, "%s/cht/%s.cht", HUGO.hugo_home_dir, HUGO.hugo_save_name);
  error = loc_hugo_load_cheat(FileName);

  return error;
}

int
hugo_load_file_cheat(char *FileName)
{
  return loc_hugo_load_cheat(FileName);
}

void
hugo_apply_cheats()
{
  int cheat_num;
  for (cheat_num = 0; cheat_num < HUGO_MAX_CHEAT; cheat_num++) {
    Hugo_cheat_t* a_cheat = &HUGO.hugo_cheat[cheat_num];
    if (a_cheat->type == HUGO_CHEAT_ENABLE) {
      writeindexedram(a_cheat->addr, a_cheat->value);
    }
  }
}

void
hugo_audio_pause(void)
{
  if (HUGO.hugo_snd_enable) {
# ifdef PSP_AUDIO
    psp_audio_pause();
# else
    SDL_PauseAudio(1);
# endif
  }
}

void
hugo_audio_resume(void)
{
  if (HUGO.hugo_snd_enable) {
# ifdef PSP_AUDIO
    psp_audio_resume();
# else
    SDL_PauseAudio(0);
# endif
  }
}

int 
loc_hugo_save_state(char *filename)
{
  return main_hugo_save_state(filename);
}

int
hugo_load_rom(char *FileName, int zip_format)
{
  char   SaveName[MAX_PATH+1];
  char*  ExtractName;
  char*  scan;
  int    format;
  int    error;
  size_t unzipped_size;

  error = 1;

  if (zip_format) {

    ExtractName = find_possible_filename_in_zip( FileName, "pce.bin");
    if (ExtractName) {
      strncpy(SaveName, FileName, MAX_PATH);
      scan = strrchr(SaveName,'.');
      if (scan) *scan = '\0';
      hugo_update_save_name(SaveName);
      const char* rom_buffer = extract_file_in_memory ( FileName, ExtractName, &unzipped_size);
      if (rom_buffer) {
        error = ! main_hugo_load_rom_buffer( rom_buffer, unzipped_size );
      }
    }

  } else {
    strncpy(SaveName,FileName,MAX_PATH);
    scan = strrchr(SaveName,'.');
    if (scan) *scan = '\0';
    hugo_update_save_name(SaveName);
    error = ! main_hugo_load_rom(FileName);
  }

  if (! error ) {
    hugo_emulator_reset();
    hugo_kbd_load();
    hugo_joy_load();
    hugo_load_cheat();
    hugo_load_settings();
  }

  return error;
}

int
hugo_load_cd(char *FileName)
{
  char   SaveName[MAX_PATH+1];
  int    error;
  char  *scan;

  error = 1;

  strncpy(SaveName,FileName,MAX_PATH);
  scan = strrchr(SaveName,'.');
  if (scan) *scan = '\0';
  hugo_update_save_name(SaveName);
  error = ! main_hugo_load_cd(FileName);

  if (! error ) {
    hugo_emulator_reset();
    hugo_kbd_load();
    hugo_joy_load();
    hugo_load_cheat();
    hugo_load_settings();
  }

  return error;
}

int
hugo_snapshot_save_slot(int save_id)
{
  char      FileName[MAX_PATH+1];
# ifdef LINUX_MODE
  struct stat aStat;
# else
  SceIoStat   aStat;
# endif
  int       error;

  error = 1;

  if (save_id < HUGO_MAX_SAVE_STATE) {
    snprintf(FileName, MAX_PATH, "%s/save/sav_%s_%d.stz", HUGO.hugo_home_dir, HUGO.hugo_save_name, save_id);
    error = loc_hugo_save_state(FileName);
    if (! error) {
# ifdef LINUX_MODE
      if (! stat(FileName, &aStat)) 
# else
      if (! sceIoGetstat(FileName, &aStat))
# endif
      {
        HUGO.hugo_save_state[save_id].used  = 1;
        HUGO.hugo_save_state[save_id].thumb = 0;
        HUGO.hugo_save_state[save_id].date  = aStat.st_mtime;
        snprintf(FileName, MAX_PATH, "%s/save/sav_%s_%d.png", HUGO.hugo_home_dir, HUGO.hugo_save_name, save_id);
        if (psp_sdl_save_thumb_png(HUGO.hugo_save_state[save_id].surface, FileName)) {
          HUGO.hugo_save_state[save_id].thumb = 1;
        }
      }
    }
  }

  return error;
}

int
hugo_snapshot_load_slot(int load_id)
{
  char  FileName[MAX_PATH+1];
  int   error;

  error = 1;

  if (load_id < HUGO_MAX_SAVE_STATE) {
    snprintf(FileName, MAX_PATH, "%s/save/sav_%s_%d.stz", HUGO.hugo_home_dir, HUGO.hugo_save_name, load_id);
    error = main_hugo_load_state(FileName);
  }
  return error;
}

int
hugo_snapshot_del_slot(int save_id)
{
  char  FileName[MAX_PATH+1];
# ifdef LINUX_MODE
  struct stat aStat;
# else
  SceIoStat   aStat;
# endif
  int   error;

  error = 1;

  if (save_id < HUGO_MAX_SAVE_STATE) {
    snprintf(FileName, MAX_PATH, "%s/save/sav_%s_%d.stz", HUGO.hugo_home_dir, HUGO.hugo_save_name, save_id);
    error = remove(FileName);
    if (! error) {
      HUGO.hugo_save_state[save_id].used  = 0;
      HUGO.hugo_save_state[save_id].thumb = 0;
      memset(&HUGO.hugo_save_state[save_id].date, 0, sizeof(ScePspDateTime));

      /* We keep always thumbnail with id 0, to have something to display in the file requester */ 
      if (save_id != 0) {
        snprintf(FileName, MAX_PATH, "%s/save/sav_%s_%d.png", HUGO.hugo_home_dir, HUGO.hugo_save_name, save_id);
# ifdef LINUX_MODE
        if (! stat(FileName, &aStat)) 
# else
        if (! sceIoGetstat(FileName, &aStat))
# endif
        {
          remove(FileName);
        }
      }
    }
  }

  return error;
}

void
hugo_treat_command_key(int hugo_idx)
{
  int new_render;

  psp_audio_pause();

  switch (hugo_idx) 
  {
    case HUGOC_FPS: HUGO.hugo_view_fps = ! HUGO.hugo_view_fps;
    break;
    case HUGOC_JOY: HUGO.psp_reverse_analog = ! HUGO.psp_reverse_analog;
    break;
    case HUGOC_RENDER: 
      psp_sdl_black_screen();
      new_render = HUGO.hugo_render_mode + 1;
      if (new_render > HUGO_LAST_RENDER) new_render = 0;
      HUGO.hugo_render_mode = new_render;
    break;
    case HUGOC_LOAD: 
       psp_main_menu_load_current();
    break;
    case HUGOC_SAVE: 
       psp_main_menu_save_current(); 
    break;
    case HUGOC_RESET: 
       psp_sdl_black_screen();
       main_hugo_emulator_reset(); 
       reset_save_name();
    break;
    case HUGOC_AUTOFIRE: 
       kbd_change_auto_fire(! HUGO.hugo_auto_fire);
    break;
    case HUGOC_DECFIRE: 
      if (HUGO.hugo_auto_fire_period > 0) HUGO.hugo_auto_fire_period--;
    break;
    case HUGOC_INCFIRE: 
      if (HUGO.hugo_auto_fire_period < 19) HUGO.hugo_auto_fire_period++;
    break;
    case HUGOC_DECDELTA:
      if (HUGO.hugo_delta_y > -10) HUGO.hugo_delta_y--;
    break;
    case HUGOC_INCDELTA: 
      if (HUGO.hugo_delta_y <  10) HUGO.hugo_delta_y++;
    break;
    case HUGOC_SCREEN: psp_screenshot_mode = 10;
    break;
  }
  psp_audio_resume();
}

void
myCtrlPeekBufferPositive( SceCtrlData* pc, int count )
{
  static long last_time = 0L;
  if (psp_exit_now) psp_sdl_exit(0);
  sceCtrlPeekBufferPositive( pc, count );
  /* too fast ? */
  if ((pc->TimeStamp - last_time) < 16000) {
    sceDisplayWaitVblankStart();
  }
  last_time = pc->TimeStamp;
}

void
myPowerSetClockFrequency(int cpu_clock)
{
  if (HUGO.hugo_current_clock != cpu_clock) {
    scePowerSetClockFrequency(cpu_clock, cpu_clock, cpu_clock/2);
    HUGO.hugo_current_clock = cpu_clock;
  }
}

void
myCtrlFastPeekBufferPositive( SceCtrlData* pc, int count )
{
  if (psp_exit_now) psp_sdl_exit(0);
  sceCtrlPeekBufferPositive( pc, count );
}

void
psp_global_initialize()
{
  memset(&HUGO, 0, sizeof(Hugo_t));
  getcwd(HUGO.hugo_home_dir,MAX_PATH);

  hugo_default_settings();
  psp_joy_default_settings();
  psp_kbd_default_settings();

  psp_sdl_init();

  hugo_update_save_name("");
  hugo_load_settings();
  hugo_kbd_load();
  hugo_joy_load();
  hugo_load_cheat();

  myPowerSetClockFrequency(HUGO.psp_cpu_clock);
}

extern int main_hugo(int argc, char* argv[]);

int
SDL_main(int argc,char **argv)
{
  psp_global_initialize();

  main_hugo(argc, argv);

  return(0);
}
