/******************************************************************************
  sysInfo.h -- System information
  
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

#ifndef ELS__SYSINFO_H
#define ELS__SYSINFO_H

extern Ulong osVersion;

extern Ulong get_osVersion(void);
extern void show_options(void);

#endif /*ELS__SYSINFO_H*/
