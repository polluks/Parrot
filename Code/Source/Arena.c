/**
    $Id: Arena.c, 1.2 2020/05/11 10:48:00, betajaen Exp $

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
#include <Parrot/String.h>

#include <proto/exec.h>
#include <proto/iffparse.h>

struct ARENA
{
  ULONG   ah_Size;
  ULONG   ah_Used;
};

struct ARENA* ArenaGame = NULL;
struct ARENA* ArenaChapter = NULL;
struct ARENA* ArenaRoom = NULL;

EXPORT struct ARENA* ArenaOpen(ULONG size, ULONG requirements)
{
  struct ARENA* arena;

  arena = NULL;

  size = (size + 3) & ~0x03;
  arena = (struct ARENA*) AllocVec(size + sizeof(struct ARENA), requirements | MEMF_CLEAR);

  if (NULL == arena)
  {
    PARROT_ERR(
      "Cannot Allocate Arena Memory!\n"
      "Reason: AllocVec returned NULL"
      PARROT_ERR_INT("size")
      PARROT_ERR_HEX("requirements"),
      size,
      requirements
    );
    goto CLEAN_EXIT;
  }

  arena->ah_Size = size;
  arena->ah_Used = 0;
  
CLEAN_EXIT:

  return arena;
}

EXPORT VOID ArenaClose(struct ARENA* arena)
{
  if (NULL == arena)
  {
    PARROT_ERR(
      "Cannot Deallocate Arena Memory!\n"
      "Reason: Arena pointer is NULL"
      PARROT_ERR_PTR("arena"),
      arena
    );
    goto CLEAN_EXIT;
  }

  arena->ah_Size = 0;

  FreeVec(arena);

CLEAN_EXIT:

}


VOID ExitArenaNow()
{
  if (ArenaRoom != NULL)
  {
    FreeVec(ArenaRoom);
  }

  if (ArenaChapter != NULL)
  {
    FreeVec(ArenaChapter);
  }

  if (ArenaGame != NULL)
  {
    FreeVec(ArenaGame);
  }
}

EXPORT BOOL ArenaRollback(struct ARENA* arena)
{
  if (NULL == arena)
    return FALSE;

  arena->ah_Used = 0;

  return TRUE;
}

EXPORT ULONG ArenaSpace(struct ARENA* arena)
{
  if (NULL == arena)
    return FALSE;

  return arena->ah_Size - arena->ah_Used;
}

EXPORT ULONG ArenaSize(struct ARENA* arena)
{
  if (NULL == arena)
    return FALSE;

  return arena->ah_Size;
}


EXPORT APTR NewObject(struct ARENA* arena, ULONG size, BOOL zeroFill)
{
  APTR result;
  CHAR strType;

  result = NULL;

  if (NULL == arena)
  {
    PARROT_ERR(
      "Cannot Allocate Memory!\n"
      "Reason: Arena given is NULL"
      PARROT_ERR_INT("ARENA::ah_Used")
      PARROT_ERR_INT("ARENA::ah_Size"),
      arena->ah_Used,
      arena->ah_Size
    );
    goto CLEAN_EXIT;
  }

  size = (size + 3) & ~0x03;
  
  if ((arena->ah_Used + size) >= arena->ah_Size)
  {
    PARROT_ERR
    (
      "Cannot Allocate Memory!\n"
      "Reason: Allocation is to large for arena"
      PARROT_ERR_INT("Arena")
      PARROT_ERR_INT("ARENA::ah_Used")
      PARROT_ERR_INT("ARENA::ah_Size")
      PARROT_ERR_INT("arg size")
      PARROT_ERR_INT("arg zeroFill"),
      arena,
      arena->ah_Used,
      arena->ah_Size,
      size,
      zeroFill
    );
    goto CLEAN_EXIT;
  }

  result = (APTR) ((ULONG*) (arena + 1) + arena->ah_Used);

  if (zeroFill == TRUE)
  {
    FillMem((UBYTE*)result, size, 0);
  }

  arena->ah_Used += size;

CLEAN_EXIT:
  return result;
}
