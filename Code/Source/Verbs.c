/**
    $Id: Verbs.c, 1.2 2020/05/11 15:49:00, betajaen Exp $

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
#include <Parrot/String.h>
#include <Parrot/Graphics.h>

STATIC CHAR CaptionText[65] = { 0 };
STATIC UWORD CaptionTextLength = 0;

VOID PlayExit(struct UNPACKED_ROOM* room, struct VERBS* verbs, struct EXIT* exit)
{

  WORD pxlen;
  CaptionTextLength = 0;

  if (verbs->vb_Selected == VERB_NONE || verbs->vb_Selected == VERB_WALK)
  {

    if (exit->ex_Name[0] == 0)
    {
      CaptionTextLength = StrCopy(CaptionText, sizeof(CaptionText), "Walk to exit");
    }
    else
    {
      CaptionTextLength = StrFormat(CaptionText, sizeof(CaptionText), "Walk to %s", exit->ex_Name)-1;
    }
  }
  
  GfxSetAPen(1, 0);
  GfxSetBPen(1, 1);
  GfxRectFill(1, 0, 0, 319, 11);

  if (CaptionTextLength)
  {
    pxlen = GfxTextLength(1, CaptionText, CaptionTextLength);
    pxlen >>= 1;

    GfxSetAPen(1, 1);
    GfxSetBPen(1, 0);
    GfxMove(1, 160 - pxlen, 10);
    GfxText(1, CaptionText, CaptionTextLength);
  }
}

VOID PlayActivator(struct UNPACKED_ROOM* room, struct VERBS* verbs, struct ENTITY* exit)
{

  WORD pxlen;
  CaptionTextLength = 0;

  if (verbs->vb_Selected == VERB_NONE || verbs->vb_Selected == VERB_WALK)
  {

    if (exit->en_Name[0] == 0)
    {
      CaptionTextLength = StrCopy(CaptionText, sizeof(CaptionText), "Walk");
    }
    else
    {
      CaptionTextLength = StrFormat(CaptionText, sizeof(CaptionText), "Walk to %s", exit->en_Name) - 1;
    }
  }

  GfxSetAPen(1, 0);
  GfxSetBPen(1, 1);
  GfxRectFill(1, 0, 0, 319, 11);

  if (CaptionTextLength)
  {
    pxlen = GfxTextLength(1, CaptionText, CaptionTextLength);
    pxlen >>= 1;

    GfxSetAPen(1, 1);
    GfxSetBPen(1, 0);
    GfxMove(1, 160 - pxlen, 10);
    GfxText(1, CaptionText, CaptionTextLength);
  }
}



VOID PlayCaption(struct UNPACKED_ROOM* room)
{
  struct VERBS* verbs;
  verbs = &room->ur_Verbs;

  room->ur_UpdateFlags &= ~UFLG_CAPTION;

  if (room->ur_HoverEntity != NULL)
  {
    if (room->ur_HoverEntity->en_Type == ET_EXIT)
    {
      PlayExit(room, &room->ur_Verbs, (struct EXIT*) room->ur_HoverEntity);
    }
    else if (room->ur_HoverEntity->en_Type == ET_ACTIVATOR)
    {
      PlayActivator(room, &room->ur_Verbs, (struct ENTITY*) room->ur_HoverEntity);
    }
  }
  else
  {
    GfxSetAPen(1, 0);
    GfxSetBPen(1, 0);
    GfxRectFill(1, 0, 0, 319, 11);
  }

  GfxSubmit(1);
}
