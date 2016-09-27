/******************************************************************************
  phLib.h -- Pointer-hash library
  
  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: September 2, 2015
  
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

#ifndef ELS__PHLIB_H
#define ELS__PHLIB_H

#if defined(USE_STRTOULL)
typedef union PhVal {
  long long          s;
  unsigned long long u;
} PhVal;
#define strtoPhVal  strtoull

#else
typedef union PhVal {
  long long          s;
  unsigned long long u;
} PhVal;
#define strtoPhVal  strtoul

#endif /*USE_STRTOULL*/

extern Boole phLookup(char *cmd, char **pcmd,
		      PhVal *pval, PhVal *pval2, PhVal *pval3);
extern Boole phStore(char *cmd, char *pcmd,
		     PhVal pval, PhVal pval2, PhVal pval3);

#endif /*ELS__PHLIB_H*/
