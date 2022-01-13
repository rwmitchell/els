/******************************************************************************
  sysInfo.c -- System information
  
  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: October 27, 2004
  
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
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>

#include "defs.h"
#include "auxil.h"
#include "sysInfo.h"

Ulong osVersion;


Ulong get_osVersion(void)
{
  Ulong release, revision, minor_rev;
  char *cp;
  struct utsname name;
  uname(&name);
  cp = name.release;
  /* Some OSes (e.g. HPUX) preface the release with letters: */
  while (*cp && !isDigit(*cp)) cp++;
  release = strtoul(cp, &cp, 10);
  if (*cp == '.') cp++;
  revision = strtoul(cp, &cp, 10);
  if (revision > 99) revision = 99;
  if (*cp == '.') cp++;
  minor_rev = strtoul(cp, &cp, 10);
  if (minor_rev > 99) minor_rev = 99;
  osVersion = release * 10000 + revision * 100 + minor_rev;
  return(osVersion);
}


void show_options(void)
{
  char *compiledOsName = "Unknown";
  /* FIXME: compiledOsName needs to be changed to use OS_NAME from
     config.h meaning that `uname -s` type CPP names be used */
#if defined(SUNOS)
  compiledOsName = "SUNOS";
#endif
#if defined(HPUX)
  compiledOsName = "HPUX";
#endif
#if defined(LINUX)
  compiledOsName = "LINUX";
#endif
#if defined(CYGWIN)
  compiledOsName = "CYGWIN";
#endif
#if defined(FREEBSD)
  compiledOsName = "FREEBSD";
#endif
#if defined(DARWIN)
  compiledOsName = "DARWIN";
#endif
#if defined(OSF1)
  compiledOsName = "OSF1";
#endif
#if defined(AIX)
  compiledOsName = "AIX";
#endif
#if defined(IRIX)
  compiledOsName = "IRIX";
#endif
#if defined(SCO)
  compiledOsName = "SCO";
#endif
#if defined(ULTRIX)
  compiledOsName = "ULTRIX";
#endif
#if defined(DYNIX)
  compiledOsName = "DYNIX";
#endif
#if defined(SYSV_OLD)
  compiledOsName = "SYSV_OLD";
#endif
  printf("	Compiled with %s=%d\n", compiledOsName, OS_VERSION);
  if (get_osVersion() > 0)
    printf("	Running OS version: %lu\n", osVersion);
  printf("\n");

#if defined(__GNUC__)
  printf("	Compiled with __GNUC__=%d\n", __GNUC__);
#else
  printf("	Compiled *without* __GNUC__\n");
#endif

#ifdef NO_ST_BLOCKS
  printf("	Compiled with NO_ST_BLOCKS\n");
#endif
#ifdef HAVE_TIMELOCAL
  printf("	Compiled with HAVE_TIMELOCAL\n");
#endif
#ifdef HAVE_TM_ZONE
  printf("	Compiled with HAVE_TM_ZONE\n");
#endif
#ifdef HAVE_ACL
  printf("	Compiled with HAVE_ACL\n");
#else
  printf("	Compiled *without* HAVE_ACL\n");
#endif
#ifdef HAVE_STAT64
  printf("	Compiled with HAVE_STAT64\n");
#else
  printf("	Compiled *without* HAVE_STAT64\n");
#endif
#ifdef HAVE_DIRENT64
  printf("	Compiled with HAVE_DIRENT64\n");
#else
  printf("	Compiled *without* HAVE_DIRENT64\n");
#endif
#ifdef HAVE_LONG_LONG_TIME
  printf("	Compiled with HAVE_LONG_LONG_TIME\n");
#else
  printf("	Compiled *without* HAVE_LONG_LONG_TIME\n");
#endif
#ifdef HAVE_O_NOATIME
  printf("	Compiled with HAVE_O_NOATIME\n");
#endif

  return;
}
