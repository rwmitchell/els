/******************************************************************************
  format.c -- dummy functions to detect compile-time format warnings
  
  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: May 7, 2002
  
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
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <utime.h>
#include <dirent.h>
#include <sys/param.h>
#include <unistd.h>
#include <stdlib.h>

#include "defs.h"
#undef  Wformat
#define Wformat
#include "format.h"


/* Note: If you're compiling with "CFLAGS += -Wall"
   and you get a warning message from this file, it means that the printf
   format for one of the following time or stat structure elements
   is wrongly defined in "format.h".  The following function never gets
   called, rather, it is a dummy function that gets compiled with Wformat
   defined which is the only way to get gcc to detect format errors. */

void formatCompileTest(struct passwd *pwd, time_t ftime, struct stat *file)
{
  printf(F_uid_t(FALSE, 0, pwd->pw_uid));
  printf(F_gid_t(FALSE, 0, pwd->pw_gid));
  printf(F_time_t(FALSE, 0, ftime));
  printf(F_st_ino(FALSE, 0, file->st_ino));
  printf(F_st_size(FALSE, 0, file->st_size));
  printf(F_st_nlink(FALSE, 0, file->st_nlink));
#ifndef NO_ST_BLOCKS
  printf(F_st_blocks(FALSE, 0, file->st_blocks));
#endif
  return;
}
