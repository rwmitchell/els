/******************************************************************************
  phLib.c -- Pointer-hash library
  
  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: September 28, 1999
  
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

#include "defs.h"
#include "auxil.h"
#include "phLib.h"


#define PH_SIZE   (1<<5)
#define PH_MASK   (PH_SIZE-1)
#define PH_SHIFT  1
#define PH_TRY    (PH_SIZE)
#define PH_FREE   8
#define PH_ALG(p) (((Ulong)(p) >> phShift) & PH_MASK)

typedef struct PhTable
{
  char *cmd_start;
  char *cmd_end;
  PhVal val;
  PhVal val2;
  PhVal val3;
} PhTable;


Local PhTable phTable[PH_SIZE];

Local int phShift = PH_SHIFT;
Local int phTry   = PH_TRY;
Local int phFree  = PH_FREE;

Local int phFilled;
Local int phTooMany;
Local int phTooHard;

Local int phTotal;
Local int phTotalNFind;
Local int phTotalNFail;


Boole phLookup(char *cmd, char **pcmd, PhVal *pval, PhVal *pval2, PhVal *pval3)
{
  int n = 0;
  int i = PH_ALG(cmd);

  while (++n <= phTry && 
	 phTable[i].cmd_start != cmd &&
	 phTable[i].cmd_start != NULL)
    i = (i + 1) & PH_MASK;

  phTotal++;

  if (phTable[i].cmd_start == cmd)
  {
    phTotalNFind += n;
    *pcmd = phTable[i].cmd_end;
    if (pval  != NULL) *pval  = phTable[i].val;
    if (pval2 != NULL) *pval2 = phTable[i].val2;
    if (pval3 != NULL) *pval3 = phTable[i].val3;
    return(TRUE);
  }
  else
  {
    phTotalNFail += n;
    if (n > phTry) phTooHard++;
    return(FALSE);
  }
}


Boole phStore(char *cmd, char *pcmd, PhVal pval, PhVal pval2, PhVal pval3)
{
  int n = 0;
  int i = PH_ALG(cmd);

  if (phFilled > PH_SIZE-(PH_SIZE/phFree))
  {
    phTooMany++;
    return(FALSE);
  }

  while (++n <= phTry && 
	 phTable[i].cmd_start != cmd &&
	 phTable[i].cmd_start != NULL)
    i = (i + 1) & PH_MASK;

  if (n > phTry)
  {
    phTooHard++;
    return(FALSE);
  }
  else
  {
    if (phTable[i].cmd_start == NULL) phFilled++;
    phTable[i].cmd_start = cmd;
    phTable[i].cmd_end = pcmd;
    phTable[i].val = pval;
    phTable[i].val2 = pval2;
    phTable[i].val3 = pval3;
    return(TRUE);
  }
}
