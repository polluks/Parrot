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
#include <Parrot/Arena.h>
#include <Parrot/Requester.h>
#include <Parrot/String.h>
#include <Parrot/Archive.h>

#include "Asset.h"

#include <proto/exec.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>
#include <exec/lists.h>

STATIC struct MinList OpenArchives;

STATIC CHAR* BasePath;
STATIC CHAR* JoinPathStr;
STATIC struct OBJECT_TABLE RoomTable;
STATIC struct OBJECT_TABLE ImageTable;
STATIC struct OBJECT_TABLE PaletteTable;
STATIC struct OBJECT_TABLE EntityTable;

#define ARCHIVE_ID 0x9640c817ul

struct ARCHIVE
{
  struct MinNode    pa_Node;
  BPTR              pa_File;
  struct IFFHandle* pa_Iff;
  ULONG             pa_Usage;
  UWORD             pa_Id;
};

EXPORT VOID UnpackBitmap(APTR asset, struct IFFHandle* iff);
EXPORT VOID PackBitmap(APTR asset);

#define ASSET_CTOR VOID(*Ctor)(APTR, struct IFFHandle*)
#define ASSET_DTOR VOID(*Dtor)(APTR)

struct ASSET_FACTORY
{
  ULONG af_NodeType;
  ULONG af_Size;
  struct OBJECT_TABLE* af_Table;
  VOID(*af_Ctor)(APTR, struct IFFHandle*);
  VOID(*af_Dtor)(APTR);
};

struct ASSET_FACTORY AssetFactories[] = {
  { CT_GAME_INFO, sizeof(struct GAME_INFO), NULL, NULL, NULL },
  { CT_PALETTE, sizeof(struct PALETTE_TABLE), &PaletteTable, NULL, NULL },
  { CT_ROOM, sizeof(struct ROOM), &RoomTable, NULL, NULL },
  { CT_IMAGE, sizeof(struct IMAGE), &ImageTable, UnpackBitmap, PackBitmap },
  { CT_ENTITY, 0,  &EntityTable, NULL, NULL },
  { 0, 0 }
};

STATIC struct ASSET_FACTORY* FindFactory(ULONG nodeType)
{
  struct ASSET_FACTORY* factory;

  factory = &AssetFactories[0];

  while (factory->af_NodeType != 0)
  {
    if (factory->af_NodeType == nodeType)
      return factory;

    factory++;
  }

  return NULL;
}

STATIC VOID ClearTables()
{
  struct ASSET_FACTORY* factory;
  struct OBJECT_TABLE* table;

  factory = &AssetFactories[0];

  while (factory->af_NodeType != 0)
  {
    table = factory->af_Table;
    
    if (NULL != table)
    {
      FillMem((UBYTE*)table, sizeof(struct OBJECT_TABLE), 0);
    }

    factory++;
  }
}

EXPORT VOID InitialiseArchives(CHAR* path)
{
  BasePath = path; /* StrDuplicate(path); */

  if (StrEndsWith(BasePath, '/'))
    JoinPathStr = "%s%ld.Parrot";
  else
    JoinPathStr = "%s/%ld.Parrot";

  NEW_MIN_LIST(OpenArchives);
  ClearTables();
}

STATIC struct ARCHIVE* ArchiveReadFromFile(UWORD id)
{
  struct ARCHIVE* archive;
  BPTR file;
  CHAR path[128];

  if (0 == StrFormat(&path[0], sizeof(path), JoinPathStr, BasePath, (ULONG) id))
  {
    PARROT_ERR(
      "Unable to load archive from disk.\n"
      "Reason: Base path is to long to be assembled into an archive path"
      PARROT_ERR_INT("Id")
      PARROT_ERR_STR("Base Path"),
      (ULONG)id,
      BasePath
    );
  }

  file = Open(&path[0], MODE_OLDFILE);

  if (NULL == file)
  {
    PARROT_ERR(
      "Unable to load archive from disk.\n"
      "Reason: Path does not exist or file is in use"
      PARROT_ERR_INT("Id")
      PARROT_ERR_STR("Path")
      PARROT_ERR_STR("Base Path"),
      (ULONG)id,
      path,
      BasePath
    );
  }

  archive = (struct ARCHIVE*) NewObject(ArenaGame, sizeof(struct ARCHIVE), TRUE);

  if (NULL == archive)
  {
    ErrorF("Did not allocate arena memory for Archive");
  }

  AddHead((struct List*) & OpenArchives, (struct Node*) archive);

  archive->pa_Id = id;
  archive->pa_Usage = 0;
  archive->pa_File = file;

  archive->pa_Iff = AllocIFF();
  archive->pa_Iff->iff_Stream = archive->pa_File;
  InitIFFasDOS(archive->pa_Iff);

  return archive;
}


EXPORT struct ARCHIVE* OpenArchive(UWORD id)
{
  struct ARCHIVE* archive;

  for (archive = (struct ARCHIVE*) OpenArchives.mlh_Head; archive->pa_Node.mln_Succ; archive = (struct ARCHIVE*) archive->pa_Node.mln_Succ)
  {
    if (archive->pa_Id == id)
    {
      return archive;
    }
  }

  return ArchiveReadFromFile(id);
}

EXPORT VOID CloseArchive(UWORD id)
{
  struct ARCHIVE* archive;

  for (archive = (struct ARCHIVE*) OpenArchives.mlh_Head; archive->pa_Node.mln_Succ; archive = (struct ARCHIVE*) archive->pa_Node.mln_Succ)
  {
    if (archive->pa_Id == id)
    {

      FreeIFF(archive->pa_Iff);
      Close(archive->pa_File);

      archive->pa_File = NULL;
      archive->pa_Iff = NULL;
      archive->pa_Id = 0;
      archive->pa_Usage = 0;

      Remove((struct Node*) archive);

      return;
    }
  }

}


EXPORT VOID CloseArchives()
{
  struct ARCHIVE* archive;

  while ( (archive = (struct ARCHIVE*) RemHead((struct List*) &OpenArchives) ) != NULL )
  {
    FreeIFF(archive->pa_Iff);
    Close(archive->pa_File);

    archive->pa_File = NULL;
    archive->pa_Iff = NULL;
    archive->pa_Id = 0;
    archive->pa_Usage = 0;
  }
}

EXPORT struct ASSET* ReadAssetFromArchive(struct ARCHIVE* archive, ULONG nodeType, UWORD chunkId, UWORD chunkArch, UWORD expectedSize, ASSET_CTOR, struct ARENA* arena)
{
  struct ContextNode* node;
  struct CHUNK_HEADER chunkHeader;
  struct ASSET* asset;
  APTR obj;
  LONG err;
  CHAR idBuf[5];
  BOOL rc;

  asset = NULL;

  if (NULL == archive)
  {
    ErrorF("Null Archive");
  }

  if (NULL == archive->pa_File)
  {
    ErrorF("Null pa_File");
  }

  Seek(archive->pa_File, 0, OFFSET_BEGINNING);
  LONG v = OpenIFF(archive->pa_Iff, IFFF_READ | IFFF_RSEEK);
  
  while (TRUE)
  {
    err = ParseIFF(archive->pa_Iff, IFFPARSE_RAWSTEP);

    if (err == IFFERR_EOC)
    {
      continue;
    }
    else if (err)
    {
      ErrorF("Chunk Read Error %ld", err);

      goto CLEAN_EXIT;
    }

    node = CurrentChunk(archive->pa_Iff);

    if (node->cn_ID == nodeType)
    {

      if (Ctor != NULL || expectedSize == 0 || (node->cn_Size - sizeof(struct CHUNK_HEADER)) == expectedSize)
      {
        ReadChunkBytes(archive->pa_Iff, &chunkHeader, sizeof(struct CHUNK_HEADER));

        if (chunkHeader.ch_Id != chunkId)
        {
          continue;
        }

        if ((chunkHeader.ch_Flags & chunkArch) == 0)
        {
          continue;
        }

        /*
          Entites may vary on size based on their sub-type.
        */
        if (nodeType == CT_ENTITY)
        {
          expectedSize = node->cn_Size;
        }

        asset = (struct ASSET*) NewObject(arena, node->cn_Size + sizeof(struct ASSET), FALSE);

        asset->as_Id = chunkId;
        asset->as_ClassType = nodeType;
        asset->as_Arch = chunkArch;

        obj = (APTR)(asset + 1);

        ReadChunkBytes(archive->pa_Iff, obj, expectedSize);

        if (Ctor != NULL)
        {
          Ctor(obj, archive->pa_Iff);
        }

        goto CLEAN_EXIT;
      }

      ErrorF("Error Chunk %s is a different size (%ld) to (%ld)",
        IDtoStr(node->cn_ID, idBuf),
        node->cn_Size,
        expectedSize
      );

      goto CLEAN_EXIT;
    }
  }

CLEAN_EXIT:

  CloseIFF(archive->pa_Iff);

  return asset;
}

STATIC struct OBJECT_TABLE_ITEM* FindInTable(struct OBJECT_TABLE* table, UWORD id, UWORD arch)
{
  struct OBJECT_TABLE_ITEM* item;

  if (NULL == table)
  {

    PARROT_ERR(
      "Unable to load asset from Object Table.\n"
      "Reason: Object Table is null"
      PARROT_ERR_STR("Asset Id")
      PARROT_ERR_INT("Arch"),
      (ULONG)id,
      (ULONG) arch
    );
    return NULL;
  }

  if (id < table->ot_IdMin || id > table->ot_IdMax)
  {
    TraceF("Not in this table Min=%ld, Max=%ld, Need=%ld", (ULONG)table->ot_IdMin, (ULONG)table->ot_IdMax, (ULONG)item->ot_Id, (ULONG) id);

    return NULL;
  }

  item = &table->ot_Items[0];

  while (item->ot_Id != 0)
  {
    if (item->ot_Id == id && (item->ot_Flags & arch) != 0)
    {
      return item;
    }

    item++;
  }

  return NULL;
}


EXPORT APTR LoadAsset(struct ARENA* arena, UWORD archiveId, ULONG classType, UWORD assetId, UWORD arch)
{
  struct ASSET* asset;
  struct ARCHIVE* archive;
  struct ASSET_FACTORY* factory;
  struct OBJECT_TABLE_ITEM* tableItem;
  APTR   obj;
  CHAR   strtype[5];

  asset = NULL;
  archive = NULL;
  factory = NULL;
  tableItem = NULL;
  obj = NULL;

  factory = FindFactory(classType);

  if (factory == NULL)
  {
    ErrorF("Could not find registered factory for \"%s\"", IDtoStr(classType, strtype));
    return NULL;
  }

  if (archiveId == ARCHIVE_UNKNOWN)
  {
    if (factory->af_Table == NULL)
    {
      PARROT_ERR(
        "Could not Load Asset.\n"
        "Reason: It is in an Unknown Archive without a Table"
        PARROT_ERR_STR("Class Type")
        PARROT_ERR_INT("Asset Id")
        PARROT_ERR_INT("Arch"),
        IDtoStr(classType, strtype),
        (ULONG)assetId,
        (ULONG)arch
      );
      return NULL;
    }

    tableItem = FindInTable(factory->af_Table, assetId, arch);

    if (tableItem != NULL && tableItem->ot_Ptr != NULL)
    {
      obj = (struct ASSET*) tableItem->ot_Ptr;
      return (APTR) (obj);
    }

    archive = OpenArchive(tableItem->ot_Archive);

    if (archive == NULL)
    {
      ErrorF("Could not open archive %ld", (ULONG)archiveId);
      return NULL;
    }
  }
  else
  {
    archive = OpenArchive(archiveId);

    if (archive == NULL)
    {
      ErrorF("Could not open archive %ld", (ULONG)archiveId);
      return NULL;
    }
  }

  asset = ReadAssetFromArchive(archive, classType, assetId, arch, factory->af_Size, factory->af_Ctor, arena);

  if (asset == NULL)
  {
    ErrorF("Could not load asset %s:%ld from archive %ld", IDtoStr(classType, strtype), (ULONG) assetId, (ULONG) archiveId);
    return NULL;
  }

  obj = (APTR)(asset + 1);

  if (tableItem == NULL && factory->af_Table != NULL)
  {
    tableItem = FindInTable(factory->af_Table, assetId, arch);
  }

  if (tableItem != NULL)
  {
    tableItem->ot_Ptr = obj;
  }

  return obj;
}

EXPORT VOID UnloadAsset(struct ARENA* arena, APTR obj)
{
  struct ASSET* asset;
  struct ARCHIVE* archive;
  struct ASSET_FACTORY* factory;
  struct OBJECT_TABLE_ITEM* tableItem;
  CHAR   strtype[5];

  archive = NULL;
  factory = NULL;
  tableItem = NULL;
  asset = NULL;

  if (obj == NULL)
  {
    ErrorF(
      "Unable to unload asset.\n"
      "Reason: Asset is null"
    );
    goto CLEAN_EXIT;
  }

  asset = asset = ((struct ASSET*)obj) - 1;

  factory = FindFactory(asset->as_ClassType);

  if (factory == NULL)
  {
    PARROT_ERR(
      "Unable to unload asset.\n"
      "Reason: Factory could not be found for asset"
      PARROT_ERR_STR("Class Type")
      PARROT_ERR_PTR("Id"),
      PARROT_ERR_PTR("Asset"),
      IDtoStr(asset->as_ClassType, strtype),
      (ULONG) asset->as_Id,
      obj
    );

    goto CLEAN_EXIT;
  }
  
  if (factory->af_Table != NULL)
  {
    tableItem = FindInTable(factory->af_Table, asset->as_Id, asset->as_Arch);

    if (tableItem != NULL && tableItem->ot_Ptr == obj)
    {
      tableItem->ot_Ptr = NULL;
    }
  }

  if (factory->af_Dtor != NULL)
  {
    factory->af_Dtor(obj);
  }

CLEAN_EXIT:

  if (obj != NULL)
  {
    FillMem(obj, factory->af_Size, 0);
  }
  
}

EXPORT VOID LoadObjectTable(struct OBJECT_TABLE_REF* ref)
{
  struct ARCHIVE* archive;
  struct OBJECT_TABLE* table;
  struct ASSET_FACTORY* assetFactory;
  struct ContextNode* node;
  struct CHUNK_HEADER chunkHeader;
  LONG err;
  CHAR idtype[5];
  UWORD ii;

  table = NULL;
  assetFactory = &AssetFactories[0];

  while (assetFactory->af_NodeType != 0)
  {
    if (assetFactory->af_NodeType == ref->tr_ClassType)
    {
      table = assetFactory->af_Table;
      break;
    }

    assetFactory++;
  }

  if (table == NULL)
  {
    PARROT_ERR(
      "Unable to load Object Table from archive file.\n"
      "Reason: Could not find suitable table to write to. (1)"
      PARROT_ERR_STR("Class Type")
      PARROT_ERR_INT("Chunk ID")
      PARROT_ERR_INT("Archive ID"),
      IDtoStr(ref->tr_ClassType, idtype),
      (ULONG)ref->tr_ChunkHeaderId,
      (ULONG)ref->tr_ArchiveId
    );

    return;
  }
  
  archive = OpenArchive(ref->tr_ArchiveId);

  if (archive == NULL)
  {
    PARROT_ERR(
      "Unable to load Object Table from archive file.\n"
      "Reason: Archive could not be opened. (2)"
      PARROT_ERR_STR("Class Type")
      PARROT_ERR_INT("Chunk ID")
      PARROT_ERR_INT("Archive ID"),
      IDtoStr(ref->tr_ClassType, idtype),
      (ULONG)ref->tr_ChunkHeaderId,
      (ULONG)ref->tr_ArchiveId
    );
    return;
  }
  
  if (archive->pa_File == NULL)
  {

    PARROT_ERR(
      "Unable to load Object Table from archive file.\n"
      "Reason: File(pa_File) was NULL. (3)"
      PARROT_ERR_STR("Class Type")
      PARROT_ERR_INT("Chunk ID")
      PARROT_ERR_INT("Archive ID"),
      IDtoStr(ref->tr_ClassType, idtype),
      (ULONG)ref->tr_ChunkHeaderId,
      (ULONG)ref->tr_ArchiveId
    );

  }

  Seek(archive->pa_File, 0, OFFSET_BEGINNING);
  LONG v = OpenIFF(archive->pa_Iff, IFFF_READ | IFFF_RSEEK);

  while (TRUE)
  {
    err = ParseIFF(archive->pa_Iff, IFFPARSE_RAWSTEP);

    if (err == IFFERR_EOC)
    {
      continue;
    }
    else if (err == IFFERR_EOF)
    {
      break;
    }
    else if (err)
    {

      PARROT_ERR(
        "Unable to load Object Table from archive file.\n"
        "Reason: Chunk Error whilst parsing IFF. (4)"
        PARROT_ERR_STR("Class Type")
        PARROT_ERR_INT("Chunk ID")
        PARROT_ERR_INT("Archive ID")
        PARROT_ERR_INT("err"),
        IDtoStr(ref->tr_ClassType, idtype),
        (ULONG)ref->tr_ChunkHeaderId,
        (ULONG)ref->tr_ArchiveId,
        err
      );

      goto CLEAN_EXIT;
    }

    node = CurrentChunk(archive->pa_Iff);

    if (node->cn_ID == CT_TABLE)
    {
      if ((node->cn_Size - sizeof(chunkHeader)) == sizeof(struct OBJECT_TABLE))
      {
        ReadChunkBytes(archive->pa_Iff, &chunkHeader, sizeof(struct CHUNK_HEADER));

        if (chunkHeader.ch_Id != ref->tr_ChunkHeaderId)
        {
          continue;
        }

        ReadChunkBytes(archive->pa_Iff, table, sizeof(struct OBJECT_TABLE));

        table->ot_Next = NULL;

        for (ii = 0; ii < MAX_ITEMS_PER_TABLE; ii++)
        {
          table->ot_Items[ii].ot_Ptr = NULL;
        }

        goto CLEAN_EXIT;
      }


      PARROT_ERR(
        "Unable to load Object Table from archive file.\n"
        "Reason: Chunk is to large to fit in OBJECT_TABLE structure. (5)"
        PARROT_ERR_STR("Class Type")
        PARROT_ERR_INT("Chunk ID")
        PARROT_ERR_INT("Archive ID")
        PARROT_ERR_INT("node->cn_Size"),
        IDtoStr(ref->tr_ClassType, idtype),
        (ULONG)ref->tr_ChunkHeaderId,
        (ULONG)ref->tr_ArchiveId,
        node->cn_Size
      );

      goto CLEAN_EXIT;
    }
  }

  PARROT_ERR(
    "Unable to load Object Table from archive file.\n"
    "Reason: End of File"
    PARROT_ERR_STR("Class Type")
    PARROT_ERR_INT("Chunk ID")
    PARROT_ERR_INT("Archive ID"),
    IDtoStr(ref->tr_ClassType, idtype),
    (ULONG) ref->tr_ChunkHeaderId,
    (ULONG) ref->tr_ArchiveId
  );

CLEAN_EXIT:

  CloseIFF(archive->pa_Iff);
}

UWORD FindAssetArchive(UWORD assetId, ULONG classType, ULONG arch)
{
  struct ASSET_FACTORY* factory;
  struct OBJECT_TABLE_ITEM* tableItem;
  UWORD archiveId;
  char strtype[5];


  archiveId = 0;

  factory = FindFactory(classType);

  if (factory == NULL)
  {
    ErrorF("Could not find registered factory for \"%s\"", IDtoStr(classType, strtype));
    goto CLEAN_EXIT;
  }

  if (factory->af_Table != NULL)
  {
    tableItem = FindInTable(factory->af_Table, assetId, arch);

    if (tableItem != NULL)
    {
      archiveId = tableItem->ot_Archive;
    }
  }


CLEAN_EXIT:

  return archiveId;
}
