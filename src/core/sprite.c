/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "sprite.h"

#include "utils.h"

UChar BGONSwitch = 1;
// do we have to draw background ?

UChar SPONSwitch = 1;
// Do we have to draw sprites ?

UInt32 spr_init_pos[1024];
// cooked initial position of sprite

int ScrollYDiff;
int oldScrollX;
int oldScrollY;
int oldScrollYDiff;

// Actual memory area where the gfx functions are drawing sprites and tiles
UChar SPM_raw[XBUF_WIDTH * XBUF_HEIGHT];
static UChar *SPM = SPM_raw + XBUF_WIDTH * 64 + 32;

int frame = 0;
// number of frame displayed


/*
      Hit Chesk Sprite#0 and others
*/
# if 0 //LUDO:
SInt32
CheckSprites (void)
{
  int i, x0, y0, w0, h0, x, y, w, h;
  SPR *spr;

  spr = (SPR *) SPRAM;
  x0 = spr->x;
  y0 = spr->y;
  w0 = (((spr->atr >> 8) & 1) + 1) * 16;
  h0 = (((spr->atr >> 12) & 3) + 1) * 16;
  spr++;
  for (i = 1; i < 64; i++, spr++)
    {
      x = spr->x;
      y = spr->y;
      w = (((spr->atr >> 8) & 1) + 1) * 16;
      h = (((spr->atr >> 12) & 3) + 1) * 16;
      if ((x < x0 + w0) && (x + w > x0) && (y < y0 + h0) && (y + h > y0))
      return TRUE;
    }
  return FALSE;
}
# endif

/*****************************************************************************

    Function:  plane2pixel

    Description: convert a PCE coded tile into a linear one
    Parameters:int no, the number of the tile to convert
    Return:nothing, but updates VRAM2

*****************************************************************************/
static void
plane2pixel (int no)
{
  UChar  *C  = VRAM + no * 32;
  UInt32 *C2 = ((UInt32 *)VRAM2) + no * 8;

  int len = 8;
  while (len-- > 0) {
      UInt32 L;
      UInt32 M = *C++;
      L =
      ((M & 0x88) >> 3) | ((M & 0x44) << 6) | ((M & 0x22) << 15) |
      ((M & 0x11) << 24);
      M = *C++;
      L |=
      ((M & 0x88) >> 2) | ((M & 0x44) << 7) | ((M & 0x22) << 16) |
      ((M & 0x11) << 25);
      M = C[14];
      L |=
      ((M & 0x88) >> 1) | ((M & 0x44) << 8) | ((M & 0x22) << 17) |
      ((M & 0x11) << 26);
      M = C[15];
      L |=
      ((M & 0x88)) | ((M & 0x44) << 9) | ((M & 0x22) << 18) | ((M & 0x11) << 27);
      *C2++ = L;
    }
}

/*****************************************************************************

    Function:  sp2pixel

    Description:convert a PCE coded sprite into a linear one
    Parameters:int no,the number of the sprite to convert
    Return:nothing but update VRAMS

*****************************************************************************/


static void
sp2pixel (int no)
{
  UChar M;
  UChar *C;
  UInt32 *C2;
  C = &VRAM[no * 128];
  C2 = ((UInt32 *)VRAMS) + no * 32;
  int len = 32;
  while (len-- > 0) {
      UInt32 L;
      M = C[0];
      L =
      ((M & 0x88) >> 3) | ((M & 0x44) << 6) | ((M & 0x22) << 15) |
      ((M & 0x11) << 24);
      M = C[32];
      L |=
      ((M & 0x88) >> 2) | ((M & 0x44) << 7) | ((M & 0x22) << 16) |
      ((M & 0x11) << 25);
      M = C[64];
      L |=
      ((M & 0x88) >> 1) | ((M & 0x44) << 8) | ((M & 0x22) << 17) |
      ((M & 0x11) << 26);
      M = C[96];
      L |=
      ((M & 0x88)) | ((M & 0x44) << 9) | ((M & 0x22) << 18) | ((M & 0x11) <<
                                                 27);
      *C2++ = L;
      C++;
    }
}

#define      FC_W      io.screen_w
#define      FC_H      256

/*****************************************************************************

    Function: RefreshLine

    Description: draw tiles on screen
    Parameters: int Y1,int Y2 (lines to draw between)
    Return: nothing

*****************************************************************************/
void
RefreshLine (int Y1, int Y2)
{
  int X1, XW, Line;
  int x, y, h, offset, Shift;
  UInt16 *PP;

  if (!(ScreenON && BGONSwitch)) return;

  Y2++;
  PP = osd_gfx_buffer + XBUF_WIDTH * Y1;

  y = Y1 + ScrollY - ScrollYDiff;
  offset = y & 7;
  h = 8 - offset;
  if (h > Y2 - Y1)
  h = Y2 - Y1;
  y >>= 3;
  PP -= ScrollX & 7;
  XW = io.screen_w / 8 + 1;
  Shift = ScrollX & 7;

  for (Line = Y1; Line < Y2; y++)
  {
    x = ScrollX / 8;
    y &= io.bg_h - 1;
    UInt16* p_vram = ((UInt16 *)VRAM) + (y * io.bg_w);
    for (X1 = 0; X1 < XW; X1++, x++, PP += 8)
    {
      UChar  *R;
      UInt16 *P;
      UChar  *C;
      UInt32  *C2;
      int no, i;
      x &= io.bg_w - 1;
      no = p_vram[x];

      R = &Pal[(no >> 12) * 16];
      no &= 0x7FF;

# if 0 //LUDO: FOR_TEST
      if (vchange[no]) {
        vchange[no] = 0;
        plane2pixel (no);
      }
# else
      int idx  = no >> 3;
      int mask = 1 << (no & 0x7);
      if (vchange[idx] & mask) {
        vchange[idx] &= ~mask;
        plane2pixel (no);
      }
# endif
      C2 = ((UInt32 *)VRAM2) + (no * 8 + offset);
      C = VRAM + (no * 32 + offset * 2);
      P = PP;
      for (i = 0; i < h; i++, P += XBUF_WIDTH, C += 2, C2++)
      {
              UInt32 L;
              UChar J;
              J = (C[0] | C[1] | C[16] | C[17]);
              if (!J) continue;
              L = *C2;
              if (J & 0x80)
                P[0] = PAL ((L >> 4) & 0xf);
              if (J & 0x40)
                P[1] = PAL ((L >> 12) & 0xf);
              if (J & 0x20)
                P[2] = PAL ((L >> 20) & 0xf);
              if (J & 0x10)
                P[3] = PAL ((L >> 28));
              if (J & 0x08)
                P[4] = PAL ((L) & 0xf);
              if (J & 0x04)
                P[5] = PAL ((L >> 8) & 0xf);
              if (J & 0x02)
                P[6] = PAL ((L >> 16) & 0xf);
              if (J & 0x01)
                P[7] = PAL ((L >> 24) & 0xf);
      }
    }
    Line += h;
    PP += XBUF_WIDTH * h - XW * 8;
    offset = 0;
    h = Y2 - Line;
    if (h > 8) h = 8;
  }
}

#define      V_FLIP      0x8000
#define      H_FLIP      0x0800
#define      SPBG      0x80
#define      CGX      0x100

/*****************************************************************************

    Function: PutSprite

    Description: convert a sprite from VRAM to normal format
    Parameters: UChar *P (the place where to draw i.e. XBuf[...])
                UChar *C (the buffer toward the sprite to draw)
                unsigned long *C2 (the buffer of precalculated sprite)
                UChar *R (address of the palette of this sprite [in PAL] )
                int h (the number of line to draw)
                int inc (the value to increment the sprite buffer)
    Return: nothing

*****************************************************************************/
void
PutSprite (UInt16 * P, UChar * C, UInt32 * C2, UChar * R, int h, int inc)
{
  int i, J;
  UInt32 L;
  for (i = 0; i < h; i++, C += inc, C2 += inc, P += XBUF_WIDTH)
  {
      J = ((UInt16 *) C)[0] | ((UInt16 *) C)[16] | ((UInt16 *) C)[32] | ((UInt16 *)C)[48];
      if (!J) continue;

      L = C2[1];
      if (J & 0x8000)
      P[0] = PAL ((L >> 4) & 0xf);
      if (J & 0x4000)
      P[1] = PAL ((L >> 12) & 0xf);
      if (J & 0x2000)
      P[2] = PAL ((L >> 20) & 0xf);
      if (J & 0x1000)
      P[3] = PAL ((L >> 28));
      if (J & 0x0800)
      P[4] = PAL ((L) & 0xf);
      if (J & 0x0400)
      P[5] = PAL ((L >> 8) & 0xf);
      if (J & 0x0200)
      P[6] = PAL ((L >> 16) & 0xf);
      if (J & 0x0100)
      P[7] = PAL ((L >> 24) & 0xf);

      L = C2[0];
      if (J & 0x80)
      P[8] = PAL ((L >> 4) & 0xf);
      if (J & 0x40)
      P[9] = PAL ((L >> 12) & 0xf);
      if (J & 0x20)
      P[10] = PAL ((L >> 20) & 0xf);
      if (J & 0x10)
      P[11] = PAL ((L >> 28));
      if (J & 0x08)
      P[12] = PAL ((L) & 0xf);
      if (J & 0x04)
      P[13] = PAL ((L >> 8) & 0xf);
      if (J & 0x02)
      P[14] = PAL ((L >> 16) & 0xf);
      if (J & 0x01)
      P[15] = PAL ((L >> 24) & 0xf);
  }
}

void
PutSpriteHandleFull (UChar * P, UChar * C, UInt32 *C2, UChar * R, int h, int inc)
{
  int i, J;
  UInt32 L;
  for (i = 0; i < h; i++, C += inc, C2 += inc, P += XBUF_WIDTH)
    {

      J = (C[0] + (C[1] << 8)) | (C[32] + (C[33] << 8)) | (C[64] + (C[65] << 8)) | (C[96] + (C[97] << 8));
      if (!J) continue;
      if (J == 65535)
      {
        L = C2[1];
        P[0] = PAL ((L >> 4) & 15);
        P[1] = PAL ((L >> 12) & 15);
        P[2] = PAL ((L >> 20) & 15);
        P[3] = PAL ((L >> 28));
        P[4] = PAL ((L) & 15);
        P[5] = PAL ((L >> 8) & 15);
        P[6] = PAL ((L >> 16) & 15);
        P[7] = PAL ((L >> 24) & 15);
        L = C2[0];
        P[8] = PAL ((L >> 4) & 15);
        P[9] = PAL ((L >> 12) & 15);
        P[10] = PAL ((L >> 20) & 15);
        P[11] = PAL ((L >> 28));
        P[12] = PAL ((L) & 15);
        P[13] = PAL ((L >> 8) & 15);
        P[14] = PAL ((L >> 16) & 15);
        P[15] = PAL ((L >> 24) & 15);

        return;
      }

      L = C2[1];            //sp2pixel(C+1);
      if (J & 0x8000)
      P[0] = PAL ((L >> 4) & 15);
      if (J & 0x4000)
      P[1] = PAL ((L >> 12) & 15);
      if (J & 0x2000)
      P[2] = PAL ((L >> 20) & 15);
      if (J & 0x1000)
      P[3] = PAL ((L >> 28));
      if (J & 0x0800)
      P[4] = PAL ((L) & 15);
      if (J & 0x0400)
      P[5] = PAL ((L >> 8) & 15);
      if (J & 0x0200)
      P[6] = PAL ((L >> 16) & 15);
      if (J & 0x0100)
      P[7] = PAL ((L >> 24) & 15);
      L = C2[0];            //sp2pixel(C);
      if (J & 0x80)
      P[8] = PAL ((L >> 4) & 15);
      if (J & 0x40)
      P[9] = PAL ((L >> 12) & 15);
      if (J & 0x20)
      P[10] = PAL ((L >> 20) & 15);
      if (J & 0x10)
      P[11] = PAL ((L >> 28));
      if (J & 0x08)
      P[12] = PAL ((L) & 15);
      if (J & 0x04)
      P[13] = PAL ((L >> 8) & 15);
      if (J & 0x02)
      P[14] = PAL ((L >> 16) & 15);
      if (J & 0x01)
      P[15] = PAL ((L >> 24) & 15);
    }
}


static void
PutSpriteHflip (UInt16 * P, UChar * C, UInt32* C2, UChar * R, int h, int inc)
{
  int i, J;
  UInt32 L;
  for (i = 0; i < h; i++, C += inc, C2 += inc, P += XBUF_WIDTH)
    {
      J =
	(C[0] + (C[1] << 8)) | (C[32] + (C[33] << 8)) | (C[64] +
							 (C[65] << 8)) |
	(C[96] + (C[97] << 8));
      if (!J) continue;
      L = C2[1];		//sp2pixel(C+1);
      if (J & 0x8000)
      P[15] = PAL ((L >> 4) & 15);
      if (J & 0x4000)
      P[14] = PAL ((L >> 12) & 15);
      if (J & 0x2000)
      P[13] = PAL ((L >> 20) & 15);
      if (J & 0x1000)
      P[12] = PAL ((L >> 28));
      if (J & 0x0800)
      P[11] = PAL ((L) & 15);
      if (J & 0x0400)
      P[10] = PAL ((L >> 8) & 15);
      if (J & 0x0200)
      P[9] = PAL ((L >> 16) & 15);
      if (J & 0x0100)
	P[8] = PAL ((L >> 24) & 15);
      L = C2[0];		//sp2pixel(C);
      if (J & 0x80)
      P[7] = PAL ((L >> 4) & 15);
      if (J & 0x40)
      P[6] = PAL ((L >> 12) & 15);
      if (J & 0x20)
      P[5] = PAL ((L >> 20) & 15);
      if (J & 0x10)
      P[4] = PAL ((L >> 28));
      if (J & 0x08)
      P[3] = PAL ((L) & 15);
      if (J & 0x04)
      P[2] = PAL ((L >> 8) & 15);
      if (J & 0x02)
      P[1] = PAL ((L >> 16) & 15);
      if (J & 0x01)
      P[0] = PAL ((L >> 24) & 15);
    }
}

/*****************************************************************************

    Function:  PutSpriteM

    Description: Display a sprite considering priority
    Parameters:
          UChar *P : A Pointer in the buffer where we got to draw the sprite
             UChar *C : A pointer in the video mem where data are available
             unsigned long *C2 : A pointer in the VRAMS mem
             UChar *R      : A pointer to the current palette
             int h : height of the sprite
             int inc : value of the incrementation for the data
             UChar* M :
    Return:

*****************************************************************************/


void
PutSpriteM (UInt16 * P, UChar * C, UInt32 * C2, UChar * R, int h, int inc,
          UChar * M, UChar pr)
{
  int i, J;
  UInt32 L;
  for (i = 0; i < h; i++, C += inc, C2 += inc, P += XBUF_WIDTH, M += XBUF_WIDTH)
    {
      J = (C[0] + (C[1] << 8)) | (C[32] + (C[33] << 8)) | (C[64] + (C[65] << 8)) | (C[96] + (C[97] << 8));
      if (!J) continue;
      L = C2[1];
      if ((J & 0x8000) && M[0] <= pr)
      P[0] = PAL ((L >> 4) & 15);
      if ((J & 0x4000) && M[1] <= pr)
      P[1] = PAL ((L >> 12) & 15);
      if ((J & 0x2000) && M[2] <= pr)
      P[2] = PAL ((L >> 20) & 15);
      if ((J & 0x1000) && M[3] <= pr)
      P[3] = PAL ((L >> 28));
      if ((J & 0x0800) && M[4] <= pr)
      P[4] = PAL ((L) & 15);
      if ((J & 0x0400) && M[5] <= pr)
      P[5] = PAL ((L >> 8) & 15);
      if ((J & 0x0200) && M[6] <= pr)
      P[6] = PAL ((L >> 16) & 15);
      if ((J & 0x0100) && M[7] <= pr)
      P[7] = PAL ((L >> 24) & 15);
      L = C2[0];
      if ((J & 0x80) && M[8] <= pr)
      P[8] = PAL ((L >> 4) & 15);
      if ((J & 0x40) && M[9] <= pr)
      P[9] = PAL ((L >> 12) & 15);
      if ((J & 0x20) && M[10] <= pr)
      P[10] = PAL ((L >> 20) & 15);
      if ((J & 0x10) && M[11] <= pr)
      P[11] = PAL ((L >> 28));
      if ((J & 0x08) && M[12] <= pr)
      P[12] = PAL ((L) & 15);
      if ((J & 0x04) && M[13] <= pr)
      P[13] = PAL ((L >> 8) & 15);
      if ((J & 0x02) && M[14] <= pr)
      P[14] = PAL ((L >> 16) & 15);
      if ((J & 0x01) && M[15] <= pr)
      P[15] = PAL ((L >> 24) & 15);
    }
}

static void
PutSpriteHflipM (UInt16 * P, UChar * C, UInt32 * C2, UChar * R, int h,
             int inc, UChar * M, UChar pr)
{
  int i, J;
  UInt32 L;
  for (i = 0; i < h; i++, C += inc, C2 += inc, P += XBUF_WIDTH, M += XBUF_WIDTH)
    {
      J = (C[0] + (C[1] << 8)) | (C[32] + (C[33] << 8)) | (C[64] + (C[65] << 8)) | (C[96] + (C[97] << 8));
      if (!J) continue;
      L = C2[1];
      if ((J & 0x8000) && M[15] <= pr)
      P[15] = PAL ((L >> 4) & 15);
      if ((J & 0x4000) && M[14] <= pr)
      P[14] = PAL ((L >> 12) & 15);
      if ((J & 0x2000) && M[13] <= pr)
      P[13] = PAL ((L >> 20) & 15);
      if ((J & 0x1000) && M[12] <= pr)
      P[12] = PAL ((L >> 28));
      if ((J & 0x0800) && M[11] <= pr)
      P[11] = PAL ((L) & 15);
      if ((J & 0x0400) && M[10] <= pr)
      P[10] = PAL ((L >> 8) & 15);
      if ((J & 0x0200) && M[9] <= pr)
      P[9] = PAL ((L >> 16) & 15);
      if ((J & 0x0100) && M[8] <= pr)
      P[8] = PAL ((L >> 24) & 15);
      L = C2[0];
      if ((J & 0x80) && M[7] <= pr)
      P[7] = PAL ((L >> 4) & 15);
      if ((J & 0x40) && M[6] <= pr)
      P[6] = PAL ((L >> 12) & 15);
      if ((J & 0x20) && M[5] <= pr)
      P[5] = PAL ((L >> 20) & 15);
      if ((J & 0x10) && M[4] <= pr)
      P[4] = PAL ((L >> 28));
      if ((J & 0x08) && M[3] <= pr)
      P[3] = PAL ((L) & 15);
      if ((J & 0x04) && M[2] <= pr)
      P[2] = PAL ((L >> 8) & 15);
      if ((J & 0x02) && M[1] <= pr)
      P[1] = PAL ((L >> 16) & 15);
      if ((J & 0x01) && M[0] <= pr)
      P[0] = PAL ((L >> 24) & 15);
    }
}

void
PutSpriteMakeMask (UInt16 * P, UChar * C, UInt32 * C2, UChar * R, int h, int inc, UChar * M, UChar pr)
{
  int i;
  UInt16 J;
  UInt32 L;
  for (i = 0; i < h; i++, C += inc, C2 += inc, P += XBUF_WIDTH, M += XBUF_WIDTH)
    {
      J = (C[0] + (C[1] << 8)) | (C[32] + (C[33] << 8)) | (C[64] + (C[65] << 8)) | (C[96] + (C[97] << 8));
      if (!J) continue;
      L = C2[1];
      if (J & 0x8000)
      {
        P[0] = PAL ((L >> 4) & 15);
        M[0] = pr;
      }
      if (J & 0x4000)
      {
        P[1] = PAL ((L >> 12) & 15);
        M[1] = pr;
      }
      if (J & 0x2000)
      {
        P[2] = PAL ((L >> 20) & 15);
        M[2] = pr;
      }
      if (J & 0x1000)
      {
        P[3] = PAL ((L >> 28));
        M[3] = pr;
      }
      if (J & 0x0800)
      {
        P[4] = PAL ((L) & 15);
        M[4] = pr;
      }
      if (J & 0x0400)
      {
        P[5] = PAL ((L >> 8) & 15);
        M[5] = pr;
      }
      if (J & 0x0200)
      {
        P[6] = PAL ((L >> 16) & 15);
        M[6] = pr;
      }
      if (J & 0x0100)
      {
        P[7] = PAL ((L >> 24) & 15);
        M[7] = pr;
      }
      L = C2[0];
      if (J & 0x80)
      {
        P[8] = PAL ((L >> 4) & 15);
        M[8] = pr;
      }
      if (J & 0x40)
      {
        P[9] = PAL ((L >> 12) & 15);
        M[9] = pr;
      }
      if (J & 0x20)
      {
        P[10] = PAL ((L >> 20) & 15);
        M[10] = pr;
      }
      if (J & 0x10)
      {
        P[11] = PAL ((L >> 28));
        M[11] = pr;
      }
      if (J & 0x08)
      {
        P[12] = PAL ((L) & 15);
        M[12] = pr;
      }
      if (J & 0x04)
      {
        P[13] = PAL ((L >> 8) & 15);
        M[13] = pr;
      }
      if (J & 0x02)
      {
        P[14] = PAL ((L >> 16) & 15);
        M[14] = pr;
      }
      if (J & 0x01)
      {
        P[15] = PAL ((L >> 24) & 15);
        M[15] = pr;
      }
    }
}

static void
PutSpriteHflipMakeMask (UInt16 * P, UChar * C, UInt32 * C2, UChar * R,
                  int h, int inc, UChar * M, UChar pr)
{
  int i, J;
  UInt32 L;
  for (i = 0; i < h; i++, C += inc, C2 += inc, P += XBUF_WIDTH, M += XBUF_WIDTH)
    {
      J = (C[0] + (C[1] << 8)) | (C[32] + (C[33] << 8)) | (C[64] + (C[65] << 8)) | (C[96] + (C[97] << 8));
      if (!J) continue;
      L = C2[1];
      if (J & 0x8000)
      {
        P[15] = PAL ((L >> 4) & 15);
        M[15] = pr;
      }
      if (J & 0x4000)
      {
        P[14] = PAL ((L >> 12) & 15);
        M[14] = pr;
      }
      if (J & 0x2000)
      {
        P[13] = PAL ((L >> 20) & 15);
        M[13] = pr;
      }
      if (J & 0x1000)
      {
        P[12] = PAL ((L >> 28));
        M[12] = pr;
      }
      if (J & 0x0800)
      {
        P[11] = PAL ((L) & 15);
        M[11] = pr;
      }
      if (J & 0x0400)
      {
        P[10] = PAL ((L >> 8) & 15);
        M[10] = pr;
      }
      if (J & 0x0200)
      {
        P[9] = PAL ((L >> 16) & 15);
        M[9] = pr;
      }
      if (J & 0x0100)
      {
        P[8] = PAL ((L >> 24) & 15);
        M[8] = pr;
      }
      L = C2[0];
      if (J & 0x80)
      {
        P[7] = PAL ((L >> 4) & 15);
        M[7] = pr;
      }
      if (J & 0x40)
      {
        P[6] = PAL ((L >> 12) & 15);
        M[6] = pr;
      }
      if (J & 0x20)
      {
        P[5] = PAL ((L >> 20) & 15);
        M[5] = pr;
      }
      if (J & 0x10)
      {
        P[4] = PAL ((L >> 28));
        M[4] = pr;
      }
      if (J & 0x08)
      {
        P[3] = PAL ((L) & 15);
        M[3] = pr;
      }
      if (J & 0x04)
      {
        P[2] = PAL ((L >> 8) & 15);
        M[2] = pr;
      }
      if (J & 0x02)
      {
        P[1] = PAL ((L >> 16) & 15);
        M[1] = pr;
      }
      if (J & 0x01)
      {
        P[0] = PAL ((L >> 24) & 15);
        M[0] = pr;
      }
    }
}

/*****************************************************************************

    Function: RefreshSpriteExact

    Description: draw all sprites between two lines, with the normal method
    Parameters: int Y1, int Y2 (the 'ordonee' to draw between), UChar bg (do we draw fg or bg sprites)
    Return: absolutly nothing

*****************************************************************************/

void
RefreshSpriteExact (int Y1, int Y2, UChar bg)
{
  int n;
  SPR *spr;
  static int usespbg = 0;

  /* TEST */
  Y2++;

  spr = (SPR *) SPRAM + 63;

  if (bg == 0)
    usespbg = 0;

  for (n = 0; n < 64; n++, spr--)
    {
      int x, y, no, atr, inc, cgx, cgy;
      UChar *R, *C;
      UInt32 *C2;
      int pos;
      int h, t, i, j;
      int y_sum;
      int spbg;

      atr = spr->atr;
      spbg = (atr >> 7) & 1;
      if (spbg != bg)
      continue;
      y = (spr->y & 1023) - 64;
      x = (spr->x & 1023) - 32;

      no = spr->no & 2047;
      // 4095 is for supergraphx only
      // no = (unsigned int)(spr->no & 4095);

#if defined(GFX_DEBUG)
      /*
         Log("Sprite 0x%02X : X = %d, Y = %d, atr = 0x%04X, no = 0x%03X\n",
         n,
         x,
         y,
         atr,
         (unsigned long)no);
       */
#endif

      cgx = (atr >> 8) & 1;
      cgy = (atr >> 12) & 3;
      cgy |= cgy >> 1;
      no = (no >> 1) & ~(cgy * 2 + cgx);
      if (y >= Y2 || y + (cgy + 1) * 16 < Y1 || x >= FC_W
        || x + (cgx + 1) * 16 < 0)
      continue;

      R = &SPal[(atr & 15) * 16];
      for (i = 0; i < cgy * 2 + cgx + 1; i++)
      {
# if 0 //LUDO: FOR_TEST
        if (vchanges[no + i]) {
            vchanges[no + i] = 0;
            sp2pixel (no + i);
          }
# else
        register int idx  = (no + i) >> 3;
        register int mask = 1 << ((no + i) & 0x7);
        if (vchanges[idx] & mask) {
          vchanges[idx] &= ~mask;
          sp2pixel (no + i);
        }
# endif
        if (!cgx) i++;
      }
      C = VRAM + (no * 128);
      C2 = ((UInt32 *)VRAMS) + (no * 32);
      pos = XBUF_WIDTH * (y + 0) + x;
      inc = 2;
      if (atr & V_FLIP)
      {
        inc = -2;
        C += 15 * 2 + cgy * 256;
        C2 += (15 * 2 + cgy * 64);
      }
      y_sum = 0;

      for (i = 0; i <= cgy; i++)
      {
        t = Y1 - y - y_sum;
        h = 16;
        if (t > 0)
          {
            C += t * inc;
            C2 += (t * inc);
            h -= t;
            pos += t * XBUF_WIDTH;
          }
        if (h > Y2 - y - y_sum)
          h = Y2 - y - y_sum;
        if (spbg == 0)
          {
            usespbg = 1;
            if (atr & H_FLIP)
            {
              for (j = 0; j <= cgx; j++)
                PutSpriteHflipMakeMask (osd_gfx_buffer + pos +
                                  (cgx - j) * 16, C + j * 128,
                                  C2 + j * 32, R, h, inc,
                                  SPM + pos + (cgx - j) * 16, n);
            }
            else
            {
              for (j = 0; j <= cgx; j++)
                PutSpriteMakeMask (osd_gfx_buffer + pos + (j) * 16,
                               C + j * 128, C2 + j * 32, R, h,
                               inc, SPM + pos + j * 16, n);
            }
          }
        else if (usespbg)
          {
            if (atr & H_FLIP)
            {
              for (j = 0; j <= cgx; j++)
                PutSpriteHflipM (osd_gfx_buffer + pos + (cgx - j) * 16,
                             C + j * 128, C2 + j * 32, R, h, inc,
                             SPM + pos + (cgx - j) * 16, n);
            }
            else
            {
              for (j = 0; j <= cgx; j++)
                PutSpriteM (osd_gfx_buffer + pos + (j) * 16, C + j * 128,
                        C2 + j * 32, R, h, inc,
                        SPM + pos + j * 16, n);

            }
          }
        else
          {
            if (atr & H_FLIP)
            {
              for (j = 0; j <= cgx; j++)
                PutSpriteHflip (osd_gfx_buffer + pos + (cgx - j) * 16,
                            C + j * 128, C2 + j * 32, R, h, inc);
            }
            else
            {
              for (j = 0; j <= cgx; j++)
                PutSprite (osd_gfx_buffer + pos + (j) * 16, C + j * 128,
                         C2 + j * 32, R, h, inc);
            }
          }
        pos += h * XBUF_WIDTH;
        C += h * inc + 16 * 7 * inc;
        C2 += (h * inc + 16 * inc);
        y_sum += 16;
      }
    }
  return;
}

char tmp_str[10];
extern int vheight;
extern char *sbuf[];

/*****************************************************************************

    Function: RefreshScreen

    Description: refresh screen
    Parameters: none
    Return: nothing

*****************************************************************************/
void
RefreshScreen()
{
static UChar loc_cnt = 0;

  hugo_render_update();
  psp_update_keys();

  if (++loc_cnt & 0x1) {
    UInt32 c = osd_pal[Pal[0]];
    c = (c << 16) | c;
    UInt32 *p = (UInt32 *)osd_gfx_buffer;
    int height = io.screen_h;
    int width  = io.screen_w >> 1;
    while (height-- > 0) {
      int len = width;
      while (len-- > 0) *p++ = c;
      p += (XBUF_WIDTH >> 1) - width;
    }
    p = (UInt32 *)SPM;
    height = 60 * XBUF_WIDTH;
    while (height-- > 0) *p++ = 0x0;
    //memset (osd_gfx_buffer, Pal[0], 240 * XBUF_WIDTH * 2);
    //memset (SPM, 0, 240 * XBUF_WIDTH);
  }
# if 0
  memset (osd_gfx_buffer + MinLine * WIDTH, Pal[0],
        (MaxLine - MinLine) * WIDTH);
  memset (SPM + MinLine * WIDTH, 0, (MaxLine - MinLine) * WIDTH);
#endif
}
