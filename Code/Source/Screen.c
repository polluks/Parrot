/**
    $Id: Screen.c, 0.1, 2020/05/07 15:55:00, betajaen Exp $

    Parrot - Point and Click Adventure Game Player
    ==============================================

    Copyright 2020 Robin Southern http://github.com/betajaen/parrot

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include <Parrot/Parrot.h>
#include <Parrot/Requester.h>
#include <Parrot/Arena.h>
#include <Parrot/String.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

#include "Cursor.inc"

#define PARROT_SCREEN_CLASS 0x44a2d27d 

struct SCREEN_TARGET
{
  struct Screen*       st_Screen;
  struct Window*       st_Window;
  UWORD                st_ReadBuffer;
  UWORD                st_WriteBuffer;
  struct ScreenBuffer* st_Buffers[2];
  struct RastPort      st_RastPorts[2];
};

EXPORT APTR ScreenNew(APTR arena, struct SCREEN_INFO* info)
{
  struct NewScreen      newScreen;
  struct NewWindow      newWindow;
  struct SCREEN_TARGET* screen;

  screen = NULL;
  InitStackVar(struct NewScreen, newScreen);
  InitStackVar(struct NewWindow, newWindow);

  if (info->si_Width < 320)
  {
    info->si_Width = 320;
  }
  
  if (info->si_Height < 240)
  {
    info->si_Height = 240;
  }

  if (info->si_Depth < 2)
  {
    info->si_Depth = 2;
  }

  if (NULL == info->si_Title || '\0' == info->si_Title[0])
  {
    info->si_Title = "Parrot";
  }

  newScreen.LeftEdge = info->si_Left;
  newScreen.TopEdge = info->si_Top;
  newScreen.Width = info->si_Width;
  newScreen.Height = info->si_Height;
  newScreen.Depth = info->si_Depth;
  newScreen.ViewModes = 0;

  if ((info->si_Flags & SIF_IS_HIRES) != 0)
  {
    newScreen.ViewModes |= HIRES;
  }

  if ((info->si_Flags & SIF_IS_LACE) != 0)
  {
    newScreen.ViewModes |= LACE;
  }

  newScreen.Type = CUSTOMSCREEN | SCREENQUIET;

  // if ((info->si_Flags & SIF_IS_PUBLIC) != 0)
  // {
  //   newScreen.Type |= PUBLICSCREEN;
  // }

  newScreen.BlockPen = BLOCKPEN;
  newScreen.DetailPen = DETAILPEN;

  newScreen.Font = NULL;
  newScreen.DefaultTitle = (UBYTE*)info->si_Title;
  newScreen.Gadgets = NULL;
  newScreen.CustomBitMap = NULL;
 
  screen = (struct SCREEN_TARGET*) ObjAlloc(arena, sizeof(struct SCREEN_TARGET), PARROT_SCREEN_CLASS);

  if (NULL == screen)
  {
    RequesterF("Close", "Out of arena memory for a new screen!");
    goto CLEAN_EXIT;
  }

  screen->st_Screen = OpenScreen(&newScreen);

  if (NULL == screen->st_Screen)
  {
    RequesterF("Close", "Could not open screen %ldx%ldx%ld for Parrot", info->si_Width, info->si_Height, info->si_Depth);
    goto CLEAN_EXIT;
  }

  screen->st_ReadBuffer = 0;
  screen->st_WriteBuffer = 1;
  
  screen->st_Buffers[0] = AllocScreenBuffer(screen->st_Screen, NULL, SB_SCREEN_BITMAP);
  InitRastPort(&screen->st_RastPorts[0]);
  screen->st_RastPorts[0].BitMap = screen->st_Buffers[0]->sb_BitMap;
  
  screen->st_Buffers[1] = AllocScreenBuffer(screen->st_Screen, NULL, 0);
  InitRastPort(&screen->st_RastPorts[1]);
  screen->st_RastPorts[1].BitMap = screen->st_Buffers[1]->sb_BitMap;

  newWindow.Width = info->si_Width;
  newWindow.Height = info->si_Height;
  newWindow.Screen = screen->st_Screen;
  newWindow.Type = CUSTOMSCREEN;
  newWindow.Flags = WFLG_BACKDROP | WFLG_BORDERLESS | WFLG_SIMPLE_REFRESH | WFLG_ACTIVATE;
  
  screen->st_Window = OpenWindow(&newWindow);
  
  if (NULL == screen->st_Window)
  {
    RequesterF("Close", "Could not open window for Parrot", info->si_Width, info->si_Height, info->si_Depth);
    goto CLEAN_EXIT;
  }
  
  CLEAN_EXIT:

  return (APTR) screen;
}


EXPORT VOID ScreenDelete(APTR obj)
{
  struct SCREEN_TARGET* screen;

  if (NULL != obj && PARROT_SCREEN_CLASS == ObjGetClass(obj))
  {
    screen = (struct SCREEN_TARGET*) (obj);

    if (NULL != screen->st_Window)
    {
      ClearPointer(screen->st_Window);

      CloseWindow(screen->st_Window);
      screen->st_Window = 0;
    }

    if (NULL != screen->st_Screen)
    {
      if (NULL != screen->st_Buffers[0])
      {
        ChangeScreenBuffer(screen->st_Screen, screen->st_Buffers[0]);
        WaitTOF();
        WaitTOF();
        FreeScreenBuffer(screen->st_Screen, screen->st_Buffers[0]);
      }

      if (NULL != screen->st_Buffers[1])
      {
        ChangeScreenBuffer(screen->st_Screen, screen->st_Buffers[1]);
        WaitTOF();
        WaitTOF();
        FreeScreenBuffer(screen->st_Screen, screen->st_Buffers[1]);
      }

      screen->st_RastPorts[0].BitMap = NULL;
      screen->st_RastPorts[1].BitMap = NULL;

      CloseScreen(screen->st_Screen);
      screen->st_Screen = 0;

    }
  }
}

EXPORT VOID ScreenSetCursor(APTR obj, UBYTE type)
{
  struct SCREEN_TARGET* screen;

  if (NULL != obj && PARROT_SCREEN_CLASS == ObjGetClass(obj))
  {
    screen = (struct SCREEN_TARGET*) (obj);
    UWORD* cursor = NULL;

    switch (type)
    {
      case 0: ClearPointer(screen->st_Window); break;
      case 1: SetPointer(screen->st_Window, Cursor1, 5, 5, -3, -3);   break;
      case 2: SetPointer(screen->st_Window, Cursor2, 13, 13, -6, -6); break;
      case 3: SetPointer(screen->st_Window, Cursor3, 15, 15, -7, -7); break;
      case 4: SetPointer(screen->st_Window, Cursor4, 9, 9, 0, 0); break;
      case 5: SetPointer(screen->st_Window, Cursor5, 9, 11, -3, 0); break;
      case 6: SetPointer(screen->st_Window, Cursor6, 9, 9, -8, 0); break;
      case 7: SetPointer(screen->st_Window, Cursor7, 11, 9, -10, -3); break;
      case 8: SetPointer(screen->st_Window, Cursor8, 9, 9, -8, -8); break;
      case 9: SetPointer(screen->st_Window, Cursor9, 9, 11, -3, -8); break;
      case 10: SetPointer(screen->st_Window, Cursor10, 9, 9, 0, -8); break;
      case 11: SetPointer(screen->st_Window, Cursor11, 11, 9, 0, -3); break;
      case 12: SetPointer(screen->st_Window, Cursor12, 15, 15, -7, -7); break;
      case 13: SetPointer(screen->st_Window, Cursor13, 9, 15, -3, 0); break;
      case 14: SetPointer(screen->st_Window, Cursor14, 9, 15, -3, 0); break;
      case 15: SetPointer(screen->st_Window, Cursor15, 9, 15, -3, 0); break;
      case 16: SetPointer(screen->st_Window, Cursor16, 13, 13, -6, -3); break;
      case 17: SetPointer(screen->st_Window, Cursor17, 15, 13, -6, -3); break;
      case 18: SetPointer(screen->st_Window, Cursor18, 15, 13, -6, -3); break;
      case 19: SetPointer(screen->st_Window, Cursor19, 14, 13, -6, -3); break;
      case 20: SetPointer(screen->st_Window, Cursor20, 16, 13, -7, -3); break;
      case 21: SetPointer(screen->st_Window, Cursor21, 16, 13, -7, -3); break;
      case 22: SetPointer(screen->st_Window, Cursor22, 16, 13, -7, -3); break;
      case 23: SetPointer(screen->st_Window, Cursor23, 15, 13, -7, -3); break;
      case 24: SetPointer(screen->st_Window, Cursor24, 16, 13, -7, -3); break;
      case 25: SetPointer(screen->st_Window, Cursor25, 16, 13, -7, -3); break;
      case 26: SetPointer(screen->st_Window, Cursor26, 16, 13, -7, -3); break;
      case 27: SetPointer(screen->st_Window, Cursor27, 14, 13, -6, -3); break;
      case 28: SetPointer(screen->st_Window, Cursor28, 16, 13, -7, -3); break;
    }
  }
}

EXPORT UBYTE ScreenGetCursor(APTR screen)
{
  return CURSOR_POINT;
}

EXPORT VOID ScreenLoadPaletteTable32(APTR obj, struct PALETTE32_TABLE* paletteTable)
{
  struct SCREEN_TARGET* screen;

  if (NULL != obj && PARROT_SCREEN_CLASS == ObjGetClass(obj))
  {
    screen = (struct SCREEN_TARGET*) (obj);
    LoadRGB32(&screen->st_Screen->ViewPort, &paletteTable->pt_Header);
  }
}

EXPORT VOID ScreenLoadPaletteTable4(APTR obj, struct PALETTE4_TABLE* paletteTable)
{
  struct SCREEN_TARGET* screen;

  if (NULL != obj && PARROT_SCREEN_CLASS == ObjGetClass(obj))
  {
    screen = (struct SCREEN_TARGET*) (obj);
    LoadRGB32(&screen->st_Screen->ViewPort, &paletteTable->pt_Header);
  }
}

EXPORT VOID ScreenClear(APTR obj)
{
  struct SCREEN_TARGET* screen;
  
  if (NULL != obj && PARROT_SCREEN_CLASS == ObjGetClass(obj))
  {
    screen = (struct SCREEN_TARGET*) (obj);

    ClearScreen(&screen->st_RastPorts[screen->st_WriteBuffer]);
  }
}

EXPORT VOID ScreenSwapBuffers(APTR obj)
{
  struct SCREEN_TARGET* screen;

  if (NULL != obj && PARROT_SCREEN_CLASS == ObjGetClass(obj))
  {
    screen = (struct SCREEN_TARGET*) (obj);

    if (ChangeScreenBuffer(screen->st_Screen, screen->st_Buffers[screen->st_WriteBuffer]))
    {
      screen->st_ReadBuffer ^= 1;
      screen->st_WriteBuffer ^= 1;
    }
  }
}
