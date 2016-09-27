/******************************************************************************
  time32.h -- extended time functions that use all 32 bits
  
  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: October 12, 2000
  
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

#ifndef ELS__TIME32_H
#define ELS__TIME32_H

#include <time.h>

extern struct tm *localtime32_r(const time_t *clock, struct tm *res);
extern struct tm *gmtime32_r(const time_t *clock, struct tm *res);
#ifdef USE_POSIX_TIME_R
extern char *asctime32_r(const struct tm *tm, char *buf);
extern char *ctime32_r(const time_t *clock, char *buf);
#else
extern char *asctime32_r(const struct tm *tm, char *buf, int buflen);
extern char *ctime32_r(const time_t *clock, char *buf, int buflen);
#endif

extern struct tm *localtime32(const time_t *clock);
extern struct tm *gmtime32(const time_t *clock);
extern size_t strftime32(char *str, size_t max,
			 const char *format, const struct tm *tm);
extern char *asctime32(const struct tm *tm);
extern char *ctime32(const time_t *clock);

#ifdef HAVE_MKTIME
extern time_t mktime32(const struct tm *tm);
#endif

#ifdef HAVE_TIMELOCAL
/* FreeBSD/Darwin do NOT declare these args as "const": */
extern time_t timelocal32(struct tm *tm);
extern time_t timegm32(struct tm *tm);
#endif

#endif /*ELS__TIME32_H*/
