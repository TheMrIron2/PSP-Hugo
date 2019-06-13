#include <zlib.h>
#include "hugo.h"

#ifdef HW_RVL
#include "di/di.h"
#endif

struct host_machine host;
struct hugo_options option;

//! Setup the host machine
/*!
 * \param argc The number of argument on command line
 * \param argv The arguments on command line
 * \return zero on success else non zero value
 */
int
initialisation (int argc, char *argv[])
{

#ifdef MSDOS
  _crt0_startup_flags |= _CRT0_FLAG_NO_LFN;
  // Disable long filename to avoid mem waste in select_rom func.
#endif

  memset (&option, 0, sizeof (option));

  option.want_stereo = FALSE;
  option.want_fullscreen = FALSE;
  option.want_fullscreen_aspect = FALSE;
  option.configure_joypads = FALSE;
  option.want_hardware_scaling = FALSE;
  option.want_arcade_card_emulation = TRUE; 
  option.want_supergraphx_emulation = TRUE;
  option.window_size = 1;

#if defined(ENABLE_NETPLAY)
  option.local_input_mapping[0] = 0;
  option.local_input_mapping[1] = 1;
  option.local_input_mapping[2] = -1;
  option.local_input_mapping[3] = -1;
  option.local_input_mapping[4] = -1;
  strcpy(option.server_hostname,"localhost");
#endif

  /*
   * Most sound cards won't like this value but we'll accept whatever SDL
   * gives us.  This frequency will get us really close to the original
   * pc engine sound behaviour.
   */
  option.want_snd_freq = 21963;

  // Initialise paths
  osd_init_paths (argc, argv);

  // Create the log file
  init_log_file ();

  // Init the random seed
  srand ((unsigned int) time (NULL));

#if !defined( NGC) && !defined(PSP)
  // Read configuration in ini file
  parse_INIfile ();

  // Read the command line
  parse_commandline (argc, argv);
#endif

  // Initialise the host machine
  if (!osd_init_machine ())
    return -1;

  // If backup memory name hasn't been overriden on command line, use the default
# if 0 //LUDO:
  if ((bmdefault) && (strcmp (bmdefault, "")))
    snprintf (backup_mem, sizeof (backup_mem), "%s%s", short_exe_name,
        bmdefault);
  else
    snprintf (backup_mem, sizeof (backup_mem), "%sbackup.dat",
        short_exe_name);
# endif

  // In case of crash, try to free audio related ressources if possible
//      atexit (TrashSound);

  // Initialise the input devices
  if (osd_init_input () != 0)
    {
      fprintf (stderr, "Initialization of input system failed\n");
      return (-2);
    }

  return 0;
}


//! Free ressources of the host machine
/*!
 * Deallocate ressources reserved during the initialisation
 */
void
cleanup ()
{

  osd_shutdown_input();

  // Deinitialise the host machine
  osd_shut_machine ();

}

#ifndef NGC
//! Check if a game was asked
/*!
 * \return non zero if a game must be played
 */
int
game_asked ()
{
  return ((CD_emulation == 1) || (strcmp (cart_name, "")));
}
#endif

//! Run an instance of a rom or cd or iso
/*!
 * \return non zero if another game has to be launched
 */
int
play_game (void)
{
  cart_reload = 0;

  if (CartLoad(cart_name)) 
    return 0;

  // Initialise the target machine (pce)
  if (InitPCE () != 0)
    return 0;

  osd_gfx_init();

  osd_snd_init_sound();

#if defined(ENABLE_NETPLAY)
  osd_init_netplay();
#endif
  RunPCE ();

#if defined(ENABLE_NETPLAY)
   osd_shutdown_netplay();
#endif

  osd_snd_trash_sound ();

  osd_gfx_shut_normal_mode();

  TrashPCE ();

  return 0;
}

//LUDO:
int
main_hugo (int argc, char** argv)
{
  int error = initialisation (argc, argv);

  psp_sdl_clear_screen(0);
  psp_sdl_flip();
  psp_sdl_clear_screen(0);
  psp_sdl_flip();

  while (1) { 
    play_game (); 
  }
}

void
main_hugo_emulator_reset()
{
  ResetPCE();
}

int 
main_hugo_load_rom(char* FileName)
{
  if (CartLoad( FileName ))
    return 0;
  
  return ! InitPCE();
}

int 
main_hugo_load_cd(char* FileName)
{
  if (CartLoad( "syscard3.pce" ))
    return 0;

  if (CDLoad(FileName))
    return 0;
  
  return ! InitPCE();
}

int 
main_hugo_load_rom_buffer(char* rom_buffer, size_t rom_size)
{
  if (CartLoadBuffer( rom_buffer, rom_size ))
    return 0;
  
  return ! InitPCE();
}

int
main_hugo_save_state(char* FileName)
{
  gzFile* a_file;

  hard_pce_save();

  if ((a_file = gzopen(FileName, "wb")) != NULL) {
    gzwrite( a_file, hard_pce, sizeof(*hard_pce));
    gzclose(a_file);
    return 0;
  }
  return 1;
}

int
main_hugo_load_state(char* FileName)
{
  gzFile* a_file;

  if ((a_file = gzopen(FileName, "rb")) != NULL) {
    gzread( a_file, hard_pce, sizeof(*hard_pce));
    
    int mmr_index;
    for (mmr_index = 0; mmr_index < 8; mmr_index++)
    {
      bank_set((UChar)mmr_index, mmr[mmr_index]);
    }

    hard_pce_restore();

    gzclose(a_file);
    return 0;
  }
  return 1;
}

void
main_hugo_send_key_event(int joy_num, int key_id, int key_press)
{
  osd_keyboard_event( joy_num, key_id, key_press );
}

void
main_hugo_reset_keyboard()
{
  osd_keyboard_reset();
}
