/******************************************************************************
 *
 * Hu-Go! PSP
 *  
 ***************************************************************************/

#include <hard_pce.h>
#include <pce.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Hu-Go keys */
#define	JOY_A		    0x01
#define	JOY_B		    0x02
#define	JOY_SELECT	0x04
#define	JOY_RUN	    0x08
#define	JOY_UP		  0x10
#define	JOY_RIGHT	  0x20
#define	JOY_DOWN	  0x40
#define	JOY_LEFT	  0x80

/*****************************************************************
                Generic input handlers 
******************************************************************/
int osd_init_input()
{
  return 0;
}

void osd_shutdown_input(void)
{
}

int osd_keyboard(void)
{
  psp_update_keys();

# if 0 //LUDO: TO_BE_DONE !

  /* switch autofire */
  autofireon ^= 1;

  /* update inputs */
  pad_update();
#ifdef HW_RVL
  wpad_update();
#endif
  return cart_reload;
# else
  return 0;
# endif
}

void
osd_keyboard_reset()
{
  int i;
  /* reset inputs (five inputs max) */
  for (i=0; i<5; i++) io.JOY[i] = 0;
}

void
osd_keyboard_event(int joy_num, int key_id, int key_press)
{
  if (joy_num >= 5) return;
  if (key_id  >= 8) return;

  if (key_press) io.JOY[joy_num] |=  (1 << key_id);
  else           io.JOY[joy_num] &= ~(1 << key_id);
}
