/**************************************************************************/
/*                                                                        */
/*             'Hu-Go! Compact disk Definition' handling code             */
/*                                                                        */
/* This is 'copyleft'. Use and abuse it as you want.                      */
/*                                                                        */
/**************************************************************************/
#include "config.h"
#include "iniconfig.h"
#include "utils.h"
#include "hcd.h"
UInt32 HCD_first_track;
UInt32 HCD_last_track;
char HCD_cover_filename[256] = "";
FILE *HCD_iso_FILE = 0;
#ifdef ALLEGRO
PACKFILE *HCD_packed_iso_FILE = 0;
#endif
UInt32 HCD_current_subtitle = 0;
UInt32 HCD_frame_at_beginning_of_track = 0;
UChar HCD_current_played_track = 0;

static int
loc_fill_HCD_info (char *name)
{
  //ZX:
  int current_track;
  char cd_dir[256];
  set_config_file (name);
  init_config();
  HCD_first_track = get_config_int ("main", "first_track", 1);
  HCD_last_track = get_config_int ("main", "last_track", 22);
  current_track = get_config_int ("main", "minimum_bios", -1);

  if (current_track != -1) minimum_bios_hooking = current_track;
  else                     minimum_bios_hooking = 0;

  strcpy(cd_dir, name);
  char* scan_slash = strrchr(cd_dir, '/');
  if (scan_slash) scan_slash[1] = 0;
  else strcpy(cd_dir, "./");

  memset(CD_track, 0, sizeof(CD_track));

  for (current_track = HCD_first_track; current_track <= HCD_last_track; current_track++) {
    char *section_name = (char *) alloca (100);
    char *tmp_buf = (char *) alloca (100);
    sprintf (section_name, "track%d", current_track);
    //----- Init patch section -----
    CD_track[current_track].patch_number = 0;
    CD_track[current_track].patch = NULL;
    //----- find beginning ---------
    strcpy (tmp_buf, get_config_string (section_name, "begin", ""));
    strupr(tmp_buf);
    if (strcasestr (tmp_buf, "MSF"))
    {
      int min = (tmp_buf[4] - '0') * 10 + (tmp_buf[5] - '0');
      int sec = (tmp_buf[7] - '0') * 10 + (tmp_buf[8] - '0');
      int fra = (tmp_buf[10] - '0') * 10 + (tmp_buf[11] - '0');
      CD_track[current_track].beg_lsn = fra + 75 * (sec + 60 * min);
      CD_track[current_track].beg_lsn -= 150;
    }
    else if (strcasestr (tmp_buf, "LSN"))
    {
      CD_track[current_track].beg_lsn = atoi (&tmp_buf[4]) - 150;
    }
    else if (strcasestr (tmp_buf, "LBA"))
    {
      CD_track[current_track].beg_lsn = atoi (&tmp_buf[4]);
    }
    else  CD_track[current_track].beg_lsn = 0;

    //----- determine type ---------
    strcpy (tmp_buf, get_config_string (section_name, "type", "AUDIO"));
    strupr(tmp_buf);
    if (strcmp (tmp_buf, "CODE") == 0)
    {
      //----- track is code ---------
      CD_track[current_track].type = 4;   // Code track
      //----- emulated track will use a regular file ---------
      CD_track[current_track].source_type = HCD_SOURCE_REGULAR_FILE;
      //----- search filename of ISO ---------
      strcpy (tmp_buf, cd_dir);
      strcat (tmp_buf, get_config_string (section_name, "filename", ""));
      strcpy (CD_track[current_track].filename, tmp_buf);
      //----- determine file length of track ---------
      CD_track[current_track].length = file_size (tmp_buf) / 2048;
      //----- search for patch -----------------------
      CD_track[current_track].patch_number = get_config_int (section_name, "patch_number", 0);
      if (CD_track[current_track].patch_number)
      {
        UInt32 i;
        char tmp_str[256];
        CD_track[current_track].patch = (PatchEntry *) malloc (sizeof (PatchEntry)
                   * CD_track[current_track].patch_number);
        for (i = 0; i < CD_track[current_track].patch_number; i++)
        {
          sprintf (tmp_str, "patch%d", i);
          strcpy (tmp_str, get_config_string (section_name, tmp_str, "0XFFFFFFFF,0XFF"));
          sscanf (tmp_str, "%X,%X", &CD_track[current_track].patch[i].offset,
           &CD_track[current_track].patch[i].new_val);
        }
      }
    }
    else if (strcmp (tmp_buf, "CD") == 0)
    {
      int min, sec, fra, control;
      int min_end, sec_end, fra_end, control_end;
      //------- track is extracted from CD ----------------
      CD_track[current_track].source_type = HCD_SOURCE_CD_TRACK;
      //------- initializing the drive --------------------
      strcpy (tmp_buf, get_config_string (section_name, "drive", "0"));
      osd_cd_init (tmp_buf);
      //------- looking for the index of the track to use for emulation ----
      CD_track[current_track].filename[0] =
        get_config_int (section_name, "track", current_track);
      //------- looking for the control byte and deducing type ------
      osd_cd_track_info (CD_track[current_track].filename[0],
               &min, &sec, &fra, &control);
      /* TODO : add support if track is in last position */
      osd_cd_track_info (CD_track[current_track].filename[0] + 1,
               &min_end, &sec_end, &fra_end, &control_end);
      CD_track[current_track].length = Time2Frame (min_end,
                          sec_end,
                          fra_end)
        - Time2Frame (min, sec, fra);
      CD_track[current_track].type = (control & 4);
    }
    else
    {
      //----- track is audio ---------
      CD_track[current_track].type = 0;   // Audio track
      //----- emulated track will use a regular file ---------
      CD_track[current_track].source_type = HCD_SOURCE_REGULAR_FILE;
      //----- search filename ---------
      strcpy (tmp_buf, cd_dir);
      strcat (tmp_buf, get_config_string (section_name, "filename", ""));
      strcpy (CD_track[current_track].filename, tmp_buf);
      //----- search for subtitles -----------------------
      CD_track[current_track].subtitle_synchro_type =
        get_config_int (section_name, "synchro_mode", 0);
      CD_track[current_track].subtitle_number =
        get_config_int (section_name, "subtitle_number", 0);
      if (CD_track[current_track].subtitle_number)
        {
          UInt32 i;
          char tmp_str[256];
          CD_track[current_track].subtitle = (SubtitleEntry *) malloc (sizeof (SubtitleEntry)
                  * CD_track [current_track].subtitle_number);
          for (i = 0; i < CD_track[current_track].subtitle_number; i++)
          {
            sprintf (tmp_str, "subtitle%d", i);
            strcpy (tmp_str, get_config_string (section_name, tmp_str, "0,0,")); 
            sscanf (tmp_str, "%d,%d",
              &CD_track[current_track].subtitle[i].StartTime,
              &CD_track[current_track].subtitle[i].Duration);
            if (strrchr (tmp_str, ','))
            {
              memset (CD_track[current_track].subtitle[i].data, 0, 32);
              strncpy (CD_track[current_track].subtitle[i].data, strrchr (tmp_str, ',') + 1, 31);
            }
         }
       }
       //----- use Tormod's work to guess MP3 length ---------
       if (strcasestr(CD_track[current_track].filename, ".mp3")) {
         CD_track[current_track].length = (int) (MP3_length (CD_track[current_track].filename) * 75.0);
       }

       if (CD_track[current_track].length == 0) CD_track[current_track].length = 30 * 75;   // 30 sec track
     }
     //----- if begin hasn't been specified ---------
     if (!CD_track[current_track].beg_lsn)
     {
       if (current_track == 1) CD_track[current_track].beg_lsn = 0;
       else CD_track[current_track].beg_lsn = CD_track[current_track - 1].beg_lsn +
         CD_track[current_track - 1].length;
     }
     //----- convert beginning in PCE msf format ---------
     {
       int min, sec, fra;
       Frame2Time (CD_track[current_track].beg_lsn + 150, &min, &sec, &fra);
           CD_track[current_track].beg_min = binbcd[min];
           CD_track[current_track].beg_sec = binbcd[sec];
           CD_track[current_track].beg_fra = binbcd[fra];
     }
   }
//restores right file for hugo config
   set_config_file_back();
# if 0 //ZX: FOR_TEST
  for (current_track = HCD_first_track;
       current_track <= HCD_last_track; current_track++)
  {
    Track* a_track = &CD_track[current_track];
    fprintf(stdout, "track %d:\n", current_track);
    fprintf(stdout, "  min=%d sec=%d fra=%d type=%d lsn=%d length=%d src_type=%d\n", 
       a_track->beg_min,
       a_track->beg_sec,
       a_track->beg_fra,
       a_track->type,
       a_track->beg_lsn,
       a_track->length,
       a_track->source_type);
    fprintf(stdout, "  filename=%s\n", CD_track[current_track].filename);
    
  }
# endif
  return 1;
}

static int
loc_fill_TOC_info (char *name)
{
  //ZX:
  char cd_dir[256];
  char buffer[512];
  int  track_id;
  int  track_min;
  int  track_sec;
  int  track_fra;
  int  track_lba;
  char track_type[32];
  int  tok_num = 0;

  strcpy(cd_dir, name);
  char* scan_slash = strrchr(cd_dir, '/');
  if (scan_slash) *scan_slash = 0;
  else strcpy(cd_dir, ".");

  HCD_first_track = 1;
  HCD_last_track =  1;

  memset(CD_track, 0, sizeof(CD_track));

  FILE* fd = fopen(name, "r");
  if (! fd) return 0;

  minimum_bios_hooking = 1;

  while (fgets(buffer, 512, fd)) {
    if (!strncasecmp( buffer, "Track ", 6)) {
      tok_num = sscanf(buffer, "Track %02d %5s %02d:%02d:%02d LBA=%06d", 
                       &track_id, &track_type, &track_min, &track_sec, &track_fra, &track_lba);
      if (tok_num == 6) {
        if (track_id > HCD_last_track) HCD_last_track = track_id;
# if 0
        CD_track[track_id].beg_min = track_min;
        CD_track[track_id].beg_sec = track_sec;
        CD_track[track_id].beg_fra = track_fra;
# endif
        CD_track[track_id].beg_lsn = track_lba - 150;
        //----- convert beginning in PCE msf format ---------
        {
          int min, sec, fra;
          Frame2Time(CD_track[track_id].beg_lsn + 150, &min, &sec, &fra);
          CD_track[track_id].beg_min = binbcd[min];
          CD_track[track_id].beg_sec = binbcd[sec];
          CD_track[track_id].beg_fra = binbcd[fra];
        }

        CD_track[track_id].source_type = HCD_SOURCE_REGULAR_FILE;

        if (! strncasecmp( track_type, "Data", 4)) {
          CD_track[track_id].type = 4; // Code
          sprintf(CD_track[track_id].filename, "%s/%02d.iso", cd_dir, track_id);
          CD_track[track_id].length = file_size (CD_track[track_id].filename) / 2048;
        } else {
          CD_track[track_id].type = 0; // Audio
          sprintf(CD_track[track_id].filename, "%s/%02d.mp3", cd_dir, track_id);
          CD_track[track_id].length = (int)(MP3_length(CD_track[track_id].filename) * 75.0);
        }
      }
    }
  }
  fclose( fd );

# if 0 //ZX: DEBUG
  for (track_id = HCD_first_track;
       track_id <= HCD_last_track; track_id++)
  {
    Track* a_track = &CD_track[track_id];
    fprintf(stdout, "track %d:\n", track_id);
    fprintf(stdout, "  min=%d sec=%d fra=%d type=%d lsn=%d length=%d src_type=%d\n", 
       a_track->beg_min,
       a_track->beg_sec,
       a_track->beg_fra,
       a_track->type,
       a_track->beg_lsn,
       a_track->length,
       a_track->source_type);
    fprintf(stdout, "  filename=%s\n", CD_track[track_id].filename);
    
  }
# endif
  return 1;
}

int
fill_HCD_info (char *name)
{
  HCD_reset();

  if (strcasestr (name, ".HCD")) {
    return loc_fill_HCD_info( name );
  }
  return loc_fill_TOC_info( name );
}

void
HCD_pause_playing ()
{
  if (MP3_playing) MP3_playing = 0;
# if defined(PSP)
  psp_mp3_pause();
# endif
}

void
HCD_mp3_reset()
{
  MP3_playing = 0;
# if defined(PSP)
  psp_mp3_reset();
# endif
}

void
HCD_play_track (UChar track, char repeat)
{
  switch (CD_track[track].subtitle_synchro_type)
    {
    case 0:         // frame synchronisation
      HCD_frame_at_beginning_of_track = (UInt32)frame;
      break;
    case 1:         // timer synchronisation
      HCD_frame_at_beginning_of_track = timer_60;
      break;
    }
  HCD_current_played_track = (UChar)track;
  HCD_current_subtitle = 0;
  if (CD_track[track].source_type == HCD_SOURCE_CD_TRACK)
    {
     osd_cd_play_audio_track((UChar)(CD_track[track].filename[0]));
     }
#if defined(PSP)
  else
  if ((CD_track[track].source_type == HCD_SOURCE_REGULAR_FILE) &&
     (strcasestr (CD_track[track].filename, ".MP3")))
    {            // must play MP3
      psp_cdaudio_play_file (CD_track[track].filename, repeat);
      MP3_playing = 1;
    }
#elif defined(MSDOS) && defined(ALLEGRO)
  else
  if ((CD_track[track].source_type == HCD_SOURCE_REGULAR_FILE) &&
     (strcasestr (CD_track[track].filename, ".MP3")))
    {            // must play MP3
      load_amp (CD_track[track].filename, repeat);
      MP3_playing = 1;
    }
#elif defined (SDL_mixer)
  else if ((CD_track[track].source_type == HCD_SOURCE_REGULAR_FILE)
         && ((strcasestr (CD_track[track].filename, ".mp3"))
         ||(strcasestr (CD_track[track].filename, ".ogg"))
         ||(strcasestr (CD_track[track].filename, ".wav"))))
    {
      Mix_PlayMusic(track,repeat);
      MP3_playing = 1;
    }
#elif defined (NGC)
#endif
};
void
HCD_play_sectors (int begin_sect, int sect_len, char repeat)
{
  int result;
  for (result = nb_max_track; result; result--)
    {
      if (((UInt32)begin_sect >= CD_track[result].beg_lsn) &&
           ((unsigned)begin_sect <= CD_track[result].beg_lsn + CD_track[result].length))
         break;
    }
  if (CD_track[result].source_type == HCD_SOURCE_CD_TRACK)
    {
     UInt32 min_from, sec_from, fra_from;
     UInt32 min_to, sec_to, fra_to;
     UInt32 min_real, sec_real, fra_real, dummy;
     begin_sect -= CD_track[result].beg_lsn;
     /* begin_sect is now relative to the begin of the track to play */
     Frame2Time((unsigned)begin_sect,
                (int*)&min_from,
                (int*)&sec_from,
                (int*)&fra_from);
     sect_len += begin_sect;
     /* sect_len is now also relative to the begin of the track to play */
     Frame2Time((unsigned)sect_len,
                (int*)&min_to,
                (int*)&sec_to,
                (int*)&fra_to);
     osd_cd_track_info((UChar)(CD_track[result].filename[0]),
                       (int*)&min_real,
                       (int*)&sec_real,
                       (int*)&fra_real,
                       (int*)&dummy);
     min_from += min_real;
     sec_from += sec_real;
     fra_from += fra_real;
     min_to += min_real;
     sec_to += sec_real;
     fra_to += fra_real;
     if (fra_to > 75)
       {
        fra_to -= 75;
        sec_to ++;
        }
     if (fra_from > 75)
       {
        fra_from -= 75;
        sec_from ++;
        }
     if (sec_to > 60)
       {
        sec_to -= 60;
        min_to ++;
        }
     if (sec_from > 60)
       {
        sec_from -= 60;
        min_from ++;
        }
      osd_cd_play_audio_range(min_from,
                              sec_from,
                              fra_from,
                              min_to,
                              sec_to,
                              fra_to);
     }
#if defined(PSP)
  else
  if ((CD_track[result].source_type == HCD_SOURCE_REGULAR_FILE) &&
      ((strcasestr (CD_track[result].filename, ".MP3"))))
    {           // must play MP3
      if (-150 < begin_sect - CD_track[result].beg_lsn < 150) {
        psp_cdaudio_play_file (CD_track[result].filename, repeat);
      } else {        /* can't yet easily repeat "inside" a track */
        psp_cdaudio_play_file (CD_track[result].filename, FALSE);
      }
# if 0 //ZX: TO_BE_DONE !
      if (amp_pollsize)
        seek_amp_abs (amp_samprat / amp_pollsize *
            (begin_sect - CD_track[result].beg_lsn) / 75);
# endif
      HCD_frame_at_beginning_of_track =
         frame - (begin_sect - CD_track[result].beg_lsn) / 75.0 * 60.0;
      /* try to estimate the number of cycle that should have elapsed since
         the beginning of the track */
      HCD_current_played_track = result;
      HCD_current_subtitle = 0;
      MP3_playing = 1;
    }

#elif defined(MSDOS) && defined(ALLEGRO)
  else
  if ((CD_track[result].source_type == HCD_SOURCE_REGULAR_FILE) &&
      (strcasestr (CD_track[result].filename, ".MP3")))
    {            // must play MP3
      if (-150 < begin_sect - CD_track[result].beg_lsn < 150)
   load_amp (CD_track[result].filename, repeat);
      else         /* can't yet easily repeat "inside" a track */
   load_amp (CD_track[result].filename, FALSE);
      if (amp_pollsize)
   seek_amp_abs (amp_samprat / amp_pollsize *
            (begin_sect - CD_track[result].beg_lsn) / 75);
      HCD_frame_at_beginning_of_track =
   frame - (begin_sect - CD_track[result].beg_lsn) / 75.0 * 60.0;
      /* try to estimate the number of cycle that should have elapsed since
         the beginning of the track */
      HCD_current_played_track = result;
      HCD_current_subtitle = 0;
      MP3_playing = 1;
    }
#elif defined(SDL_mixer)
  else if ((CD_track[result].source_type == HCD_SOURCE_REGULAR_FILE)
            && (strcasestr (CD_track[result].filename, ".mp3")
            ||strcasestr (CD_track[result].filename, ".ogg"))
            ||strcasestr (CD_track[result].filename, ".wav")) {
      if (-150 < begin_sect - CD_track[result].beg_lsn < 150)
        Mix_PlayMusic(result, repeat);
      else       /* can't yet easily repeat "inside" a track */
        Mix_PlayMusic(result, FALSE);

      Mix_RewindMusic();
      if (Mix_SetMusicPosition((begin_sect - CD_track[result].beg_lsn) / 75)==-1){
        printf("Mix_SetMusicPosition(): %s\n", Mix_GetError());
      }
      HCD_frame_at_beginning_of_track = frame - (begin_sect - CD_track[result].beg_lsn) / 75.0 * 60.0;
      /* try to estimate the number of cycle that should have elapsed since
         the beginning of the track */
      HCD_current_played_track = result;
      HCD_current_subtitle = 0;
      MP3_playing = 1;
  }
#endif
  else if (strcasestr (CD_track[result].filename, ".WAV"))
    {
#ifdef MSDOS
      static SAMPLE *wav_sample;
      wav_sample = load_sample (CD_track[result].filename);
      play_sample (wav_sample, 255, 128, 1000, FALSE);
#endif
    }
}

void
HCD_shutdown ()
{
# if 0 //ZX:
  int current_track;
  for (current_track = (int)HCD_first_track;
       current_track <= (int)HCD_last_track; current_track++)
    {
      if (CD_track[current_track].patch) free (CD_track[current_track].patch);
      if (CD_track[current_track].subtitle) free(CD_track[current_track].subtitle);
    }
# else
  memset(CD_track, 0, sizeof(CD_track));
  HCD_first_track = 0;
  HCD_last_track = 0;
# endif
}
void
HCD_handle_subtitle ()
{
  if (HCD_current_subtitle >=
      CD_track[HCD_current_played_track].subtitle_number)
    return;
  switch (CD_track[HCD_current_played_track].subtitle_synchro_type)
    {
    case 0:
      if (frame - HCD_frame_at_beginning_of_track >=
           (int)(CD_track[HCD_current_played_track].
           subtitle[HCD_current_subtitle].StartTime))
         {
            osd_gfx_set_message (
               CD_track[HCD_current_played_track].subtitle[HCD_current_subtitle].data
            );
            message_delay =
               CD_track[HCD_current_played_track].subtitle[HCD_current_subtitle].Duration;
            HCD_current_subtitle++;
         }
      break;
    case 1:
      if (timer_60 - HCD_frame_at_beginning_of_track >=
     CD_track[HCD_current_played_track].
     subtitle[HCD_current_subtitle].StartTime)
   {
     osd_gfx_set_message (CD_track[HCD_current_played_track].subtitle
                [HCD_current_subtitle].data);
     message_delay =
       CD_track[HCD_current_played_track].
       subtitle[HCD_current_subtitle].Duration;
     HCD_current_subtitle++;
   }
      break;
    }
}
void
read_sector_HCD (unsigned char *p, UInt32 dum)
{
  int result;
  for (result = (int)HCD_last_track; result > 0x01; result--)
    {
      if ((pce_cd_sectoraddy >= CD_track[result].beg_lsn) &&
     (pce_cd_sectoraddy <= CD_track[result].beg_lsn +
      CD_track[result].length))
   break;
    }
  if (CD_track[result].source_type == HCD_SOURCE_REGULAR_FILE)
    HCD_iso_read_sector(p, dum, (UInt32)result);
  else
    if (CD_track[result].source_type == HCD_SOURCE_CD_TRACK)
      HCD_cd_read_sector(p, dum, (UInt32)result);
}

  static int iso_current_file = 0;
  static int iso_current_type = 0;

void
HCD_iso_read_sector(unsigned char *p, UInt32 dum, UInt32 result)
{
#ifndef FINAL_RELEASE
  fprintf (stderr, "Loading sector nø%d.\n", pce_cd_sectoraddy);
  Log ("Loading sector nø%d.\n", pce_cd_sectoraddy);
  Log ("HCD : seek at %d\n",
       (pce_cd_sectoraddy - CD_track[result].beg_lsn) * 2048);
  Log ("Track nø%d begin at %d\n", result, CD_track[result].beg_lsn);
  Log ("Track nø2 begin at %d\n", CD_track[2].beg_lsn);
#endif
label_are_bad:
  if (result == iso_current_file)
  {
    if (iso_current_type == 1)   // non compacted
    {
      fseek (HCD_iso_FILE, (long)((pce_cd_sectoraddy - CD_track[result].beg_lsn)
        * 2048), SEEK_SET);
      long test_read = fread (p, 1, 2048, HCD_iso_FILE);
      if (test_read != 2048) {
          Log("Error when reading sector %d in hcd (%d)", pce_cd_sectoraddy, test_read);
      }
    }
    else
    {
      Log ("Open mode in HCD read function incorrect\n");
    }
  }           // file well opened
  else
  {            // must open file
    if (strcasestr (CD_track[result].filename, ".ISO"))
    {
      if (HCD_iso_FILE) { fclose( HCD_iso_FILE ); HCD_iso_FILE = 0; }
      HCD_iso_FILE = fopen (CD_track[result].filename, "rb");
#ifndef FINAL_RELEASE
      Log ("File tried to open\n");
#endif
      if (!HCD_iso_FILE)
      {
        Log ("ISO file not found : %s\nUsed for track %d\n",
        CD_track[result].filename, result);
        return;
      }
      iso_current_file = result;
      iso_current_type = 1;
    }
    goto label_are_bad;
  }
  {
    int dummy;
    for (dummy = 0; dummy < (int)(CD_track[result].patch_number); dummy++)
      {
   if ((CD_track[result].patch[dummy].offset >> 11) ==
       pce_cd_sectoraddy - CD_track[result].beg_lsn)
     p[CD_track[result].patch[dummy].offset & 2047] =
       CD_track[result].patch[dummy].new_val;
      }
   }
}

void
HCD_reset()
{
  iso_current_file = 0;
  iso_current_type = 0;
  if (HCD_iso_FILE) { fclose( HCD_iso_FILE ); HCD_iso_FILE = 0; }
}

void
HCD_cd_read_sector(unsigned char *p, UInt32 dum, UInt32 result)
{
 UInt32 min, sec, fra, control;
 osd_cd_track_info((UChar)CD_track[result].filename[0],
                   (int*)&min,
                   (int*)&sec,
                   (int*)&fra,
                   (int*)&control);
 osd_cd_read(p, dum - CD_track[result].beg_lsn + Time2HSG((int)min, (int)sec, (int)fra));
}
