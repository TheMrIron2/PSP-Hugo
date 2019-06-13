#ifndef _HUGO_H_
#define _HUGO_H_

# ifdef __cplusplus
extern "C" {
# endif

#ifndef LINUX_MODE
# define PSP_UNCACHE_PTR(p) ( ((long)(p))|0x40000000 )
# define PSP_NORMAL_PTR(p)  ( ((long)(p))&0xBFFFFFFF )
#else
# define PSP_UNCACHE_PTR(p) (p)
# define PSP_NORMAL_PTR(p)  (p)
#endif


//LUDO:
# define HUGO_RENDER_NORMAL   0
# define HUGO_RENDER_X125     1
# define HUGO_RENDER_X15      2
# define HUGO_RENDER_X175     3
# define HUGO_RENDER_MAX      4
# define HUGO_LAST_RENDER     4

# define MAX_PATH             256
# define HUGO_MAX_SAVE_STATE    5
# define HUGO_MAX_CHEAT        10

#include <psptypes.h>
#include <pspctrl.h>
#include <SDL.h>

#define HUGO_CHEAT_NONE    0
#define HUGO_CHEAT_ENABLE  1
#define HUGO_CHEAT_DISABLE 2

#define HUGO_CHEAT_COMMENT_SIZE 25

  typedef struct Hugo_cheat_t {
    unsigned char  type;
    unsigned short addr;
    unsigned char  value;
    char           comment[HUGO_CHEAT_COMMENT_SIZE];
  } Hugo_cheat_t;

  typedef struct Hugo_save_t {

    SDL_Surface    *surface;
    char            used;
    char            thumb;
    ScePspDateTime  date;

  } Hugo_save_t;

  typedef struct Hugo_t {
 
    Hugo_save_t  hugo_save_state[HUGO_MAX_SAVE_STATE];
    Hugo_cheat_t hugo_cheat[HUGO_MAX_CHEAT];

    int  comment_present;
    char hugo_save_name[MAX_PATH];
    char hugo_home_dir[MAX_PATH];
    int  psp_screenshot_id;
    int  psp_cpu_clock;
    int  psp_cpu_cdclock;
    int  psp_reverse_analog;
    int  psp_irdajoy_type;
    int  psp_irdajoy_debug;
    int  hugo_view_fps;
    int  hugo_current_fps;
    int  hugo_current_clock;
    int  psp_active_joystick;
    int  hugo_cdaudio_enable;
    int  hugo_snd_enable;
    int  hugo_snd_volume;
    int  hugo_snd_freq;
    int  hugo_overclock;
    int  hugo_render_mode;
    int  hugo_vsync;
    int  hugo_delta_y;
    int  hugo_speed_limiter;
    int  psp_skip_max_frame;
    int  psp_skip_cur_frame;
    int  hugo_slow_down_max;
    int  hugo_auto_fire;
    int  hugo_auto_fire_pressed;
    int  hugo_auto_fire_period;

  } Hugo_t;

  extern Hugo_t HUGO;

  extern void main_hugo_send_key_event(int joy_num, int hugo_idx, int key_press);
  extern int  main_hugo_load_state(char *filename);
  extern void main_hugo_force_draw_blit();
  extern int  main_hugo_save_state(char *filename);
  extern int main_hugo_load_rom(char *filename);
  extern void main_hugo_emulator_reset();

  extern void myCtrlPeekBufferPositive( SceCtrlData* pc, int count );
  extern void myCtrlFastPeekBufferPositive( SceCtrlData* pc, int count );
  extern void myPowerSetClockFrequency(int cpu_clock);
  extern int psp_exit_now;

# ifdef __cplusplus
}
# endif

#endif
