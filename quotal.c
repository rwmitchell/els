/******************************************************************************
  quotal.c -- els quotal functions
  
  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: November 5, 2012
  
  These functions are provided "as is" in the hopes that they might serve some
  higher purpose.  If you want these functions to serve some lower purpose,
  then that too is all right.  So as to keep these hopes alive you may
  freely distribute these functions so long as this header remains intact.
  You may also freely distribute modified versions of these functions so long
  as you indicate that such versions are modified and so long as you
  provide access to the unmodified original copy.
  
  Note: The most recent version of these functions may be obtained from
        http://els-software.org

  ****************************************************************************/


#include "sysdefs.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>	/* sys/stat.h needed by els.h */
#include <pwd.h>
#include <grp.h>
#include <string.h>	/* string.h needed by memset */

#include "defs.h"
#include "auxil.h"
#include "els.h"
#include "elsMisc.h"
#include "sysdep.h"
#include "quotal.h"
#include "format.h"


/* Safety check: */
#if ELS_SIZEOF_uid_t != ELS_SIZEOF_gid_t
# error: ELS assumes that sizeof(uid_t) == sizeof(gid_t)
#endif
#if ELS_SIZEOF_uid_t > ELS_SIZEOF_long
# error: ELS assumes that sizeof(uid_t) <= sizeof(long)
#endif

typedef struct Quotal_Item
{
  ELS_st_blocks nblocks;
  ELS_st_ino nfiles;
} Quotal_Item;

typedef struct Quotal_List
{
  Ulong id;  /* Used to store either uid_t or gid_t */
  Quotal_Item item;
  struct Quotal_List *next;
} Quotal_List;

typedef struct Quotal_Hash
{
  Ulong id;  /* Used to store either uid_t or gid_t */
  struct Quotal_List *ptr;
} Quotal_Hash;


#define NFS2_ID_LIMIT  0x10000
Local Quotal_Item *grandQuotal_uid = NULL;
Local Quotal_Item *grandQuotal_gid = NULL;
Local Quotal_Item *subQuotal_uid = NULL;
Local Quotal_Item *subQuotal_gid = NULL;
Local char *UNASSIGNED = "UNASSIGNED";

/* BIG_UID_HASH_SIZE must be a power of 2: */
#define BIG_UID_HASH_SIZE  0x8000
/* BIG_GID_HASH_SIZE must be a power of 2: */
#define BIG_GID_HASH_SIZE  0x4000
Local Quotal_Hash *bigUidHashTable = NULL;
Local Quotal_Hash *bigGidHashTable = NULL;
Local Quotal_List *bigUidList = NULL;
Local Quotal_List *bigGidList = NULL;


Local Quotal_List *searchIdList(char type, Ulong id, Ulong id_hash,
				Quotal_Hash *id_hash_table,
				Quotal_List **id_list_ptr)
{
  if ((Debug&16) && id_hash_table[id_hash].id != 0)
    printf("Hash %cid ?: %7ld == %7ld\n",
	   type, id_hash_table[id_hash].id, id);

  if (id_hash_table[id_hash].id == id)
  {
    /* Hash-hit -- Return hashed element: */
    if (Debug&16) printf("Hashed %cid: %7ld\n", type, id);
    return(id_hash_table[id_hash].ptr);
  }
  else if (*id_list_ptr == NULL)
  {
    /* Start of new-list -- Create and return first element: */
    Quotal_List *ptr;
    if (Debug&16) printf("START %cid:  %7ld\n", type, id);
    *id_list_ptr = memAllocZero(sizeof(Quotal_List));
    ptr = *id_list_ptr;
    ptr->id = id;
    ptr->next = NULL;
    return(ptr);
  }
  else
  {
    /* Search list for element and create new element if necessary: */
    Quotal_List *ptr = *id_list_ptr;
    Quotal_List *prevptr = NULL;
    Quotal_List *newptr;
    while (ptr != NULL && ptr->id < id)
    {
      prevptr = ptr;
      ptr = ptr->next;
    }
    if (ptr != NULL && ptr->id == id)
    {
      /* Found existing element: */
      if (Debug&16) printf("Found %cid:  %7ld\n", type, id);
      id_hash_table[id_hash].id = id;
      id_hash_table[id_hash].ptr = ptr;
      return(ptr);
    }
    newptr = memAllocZero(sizeof(Quotal_List));
    newptr->id = id;
    if (prevptr == NULL)
    {
      /* Prepend new element to front of list: */
      if (Debug&16) printf("Prepend %cid:%7ld\n", type, id);
      newptr->next = ptr;
      *id_list_ptr = newptr;
    }
    else if (ptr == NULL)
    {
      /* Append new element to end of list: */
      if (Debug&16) printf("Append %cid: %7ld\n", type, id);
      prevptr->next = newptr;
      newptr->next = NULL;
    }
    else
    {
      /* Insert new element inside list: */
      if (Debug&16) printf("Insert %cid: %7ld\n", type, id);
      prevptr->next = newptr;
      newptr->next = ptr;
    }
    id_hash_table[id_hash].id = id;
    id_hash_table[id_hash].ptr = newptr;
    return(newptr);
  }
}


Local Quotal_List *searchUidList(Ulong uid)
{
  Ulong uid_hash = uid & (BIG_UID_HASH_SIZE - 1);
  if (bigUidHashTable == NULL)
  {
    bigUidHashTable = memAllocZero(sizeof(Quotal_Hash) * BIG_UID_HASH_SIZE);
    bigUidHashTable[0].id = 1;  /* Element zero must be non-zero */
  }
  return(searchIdList('U', uid, uid_hash, bigUidHashTable, &bigUidList));
}


Local Quotal_List *searchGidList(Ulong gid)
{
  Ulong gid_hash = gid & (BIG_GID_HASH_SIZE - 1);
  if (bigGidHashTable == NULL)
  {
    bigGidHashTable = memAllocZero(sizeof(Quotal_Hash) * BIG_GID_HASH_SIZE);
    bigGidHashTable[0].id = 1;  /* Element zero must be non-zero */
  }
  return(searchIdList('G', gid, gid_hash, bigGidHashTable, &bigGidList));
}


void Quotal(Dir_Item *file)
{
  ELS_st_blocks nblocks;
  Ulong uid = uid2Ulong(file->info.st_uid);
  Ulong gid = gid2Ulong(file->info.st_gid);

  if (grandQuotal_uid == NULL)
  {
    int mem_size = sizeof(Quotal_Item) * NFS2_ID_LIMIT;
    grandQuotal_uid = memAllocZero(mem_size);
    grandQuotal_gid = memAllocZero(mem_size);
    subQuotal_uid = memAllocZero(mem_size);
    subQuotal_gid = memAllocZero(mem_size);
  }

  nblocks = getStatBlocks(&file->info);

  if (uid < NFS2_ID_LIMIT)
  {
    grandQuotal_uid[uid].nfiles++;
    grandQuotal_uid[uid].nblocks += nblocks;
    subQuotal_uid[uid].nfiles++;
    subQuotal_uid[uid].nblocks += nblocks;
  }
  else
  {
    Quotal_List *bigUid = searchUidList(uid);
    bigUid->item.nfiles++;
    bigUid->item.nblocks += nblocks;
    /* FIXME: subQuotals for bigUids not necessary at this time */
  }

  if (gid < NFS2_ID_LIMIT)
  {
    grandQuotal_gid[gid].nfiles++;
    grandQuotal_gid[gid].nblocks += nblocks;
    subQuotal_gid[gid].nfiles++;
    subQuotal_gid[gid].nblocks += nblocks;
  }
  else
  {
    Quotal_List *bigGid = searchGidList(gid);
    bigGid->item.nfiles++;
    bigGid->item.nblocks += nblocks;
    /* FIXME: subQuotals for bigGids not necessary at this time */
  }

  return;
}


Local char *quotalUid(uid_t i)
{
  static char Uid[5 + MAX_USER_GROUP_NAME + 32 + 2];
  char num[32];
  char *uid = uid2name(i);
  if (uid == NULL) uid = UNASSIGNED;
  sprintf(num, FU_uid_t(FALSE,0, i));
  sprintf(Uid, "UID: %10s/%-6s", uid, num);
  return(Uid);
}


Local char *quotalGid(gid_t i)
{
  static char Gid[5 + MAX_USER_GROUP_NAME + 32 + 2];
  char num[32];
  char *gid = gid2name(i);
  if (gid == NULL) gid = UNASSIGNED;
  sprintf(num, FU_gid_t(FALSE,0, i));
  sprintf(Gid, "GID: %10s/%-6s", gid, num);
  return(Gid);
}


Local void quotalPrint(char *id, ELS_st_blocks nblocks, ELS_st_ino nfiles)
{
  printf("%-23s ", id);
  if (list_size_human_10 || list_size_human_2)
  {
    ELS_st_size n = nblocks * 512;
    printf("%10s", scaleSizeToHuman(n, list_size_human_10 ? 10 : 2));
  }
  else
  {
    ELS_st_blocks n = nblocks;
    n = n / 2;	/* Convert blocks to KBytes */
    printf(F_st_blocks(FALSE, 10, n));
  }
  printf(" ");
  printf(F_st_ino(FALSE, 10, nfiles));
  printf("\n");
  return;
}


Local void quotalHeading(char *UorG)
{
  printf("\n");
  /*      xID: 1234567890/1234567 1234567890 1234567890 */
  printf("     %5s %4s/%-7s %10s %10s\n",
	 UorG,
	 "Name",
	 "Number",
	 list_size_human_10 || list_size_human_2 ? "Bytes" : "KBytes",
	 "Files"); 
  return;
}


void grandQuotal_print(void)
{
  int i;
  Quotal_List *ptr;
  ELS_st_blocks grandQuotal_nblocks = 0;
  ELS_st_ino grandQuotal_nfiles = 0;

  /* NB: grandQuotal_uid might be NULL if no files were listed: */
  if (grandQuotal_uid == NULL) return;

  quotalHeading("User");

  for (i = 0; i < NFS2_ID_LIMIT; i++)
  {
    Quotal_Item *q = &grandQuotal_uid[i];
    if (q->nfiles > 0)
    {
      quotalPrint(quotalUid(i), q->nblocks, q->nfiles);
      grandQuotal_nblocks += q->nblocks;
      grandQuotal_nfiles += q->nfiles;
    }
  }
  for (ptr = bigUidList; ptr != NULL; ptr = ptr->next)
  {
    Quotal_Item *q = &ptr->item;
    quotalPrint(quotalUid(ptr->id), q->nblocks, q->nfiles);
    grandQuotal_nblocks += q->nblocks;
    grandQuotal_nfiles += q->nfiles;
  }
  
  quotalHeading("Group");

  for (i = 0; i < NFS2_ID_LIMIT; i++)
  {
    Quotal_Item *q = &grandQuotal_gid[i];
    if (q->nfiles > 0)
    {
      quotalPrint(quotalGid(i), q->nblocks, q->nfiles);
    }
  }
  for (ptr = bigGidList; ptr != NULL; ptr = ptr->next)
  {
    Quotal_Item *q = &ptr->item;
    quotalPrint(quotalGid(ptr->id), q->nblocks, q->nfiles);
  }

  /*      xID: 1234567890/1234567 1234567890 1234567890 */
  printf("                        ---------- ----------\n");
  quotalPrint("TOTAL:", grandQuotal_nblocks, grandQuotal_nfiles);
  printf("\n");

  return;
}


/* FIXME: Are subQuotals ever going to be of any use? */
void subQuotal_print(void)
{
#ifdef SUBQUOTAL
  int i;
  int mem_size = sizeof(Quotal_Item) * NFS2_ID_LIMIT;

  if (subQuotal_uid != NULL)
  {
    for (i = 0; i < NFS2_ID_LIMIT; i++)
    {
      Quotal_Item *q = &subQuotal_uid[i];
      if (q->nfiles > 0)
      {
	quotalPrint(quotalUid(i), q->nblocks, q->nfiles);
      }
    }
    memset(subQuotal_uid, 0, mem_size);
  }
  /* FIXME: subQuotals for bigUids not implemented */
  
  if (subQuotal_gid != NULL)
  {
    for (i = 0; i < NFS2_ID_LIMIT; i++)
    {
      Quotal_Item *q = &subQuotal_gid[i];
      if (q->nfiles > 0)
      {
	quotalPrint(quotalGid(i), q->nblocks, q->nfiles);
      }
    }
    memset(subQuotal_gid, 0, mem_size);
  }
  /* FIXME: subQuotals for bigGids not implemented */

#endif
  return;
}


