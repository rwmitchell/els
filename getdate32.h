/******************************************************************************
  getdate32.h -- get ISO8601/LS-style/unix-clock date using all 32 bits
  
  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: July 17, 2000
  
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

#ifndef ELS__GETDATE32_H
#define ELS__GETDATE32_H

extern time_t getdate32(char *arg, char **ptr);
extern time_t convert_iso8601(char *arg, char **ptr, Boole max_time);
extern long guess_gmtoff(time_t *clock, int yday, int hour, int min);
extern char *guess_TZ(void);
extern char *get_currentTZ(void);
extern int set_currentTZ(char *TZ);
extern char *get_defaultTZ(void);
extern char *get_validTZ(char *TZ);

extern char LGC;

#endif /*ELS__GETDATE32_H*/
