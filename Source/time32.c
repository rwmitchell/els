/******************************************************************************
  time32.c -- extended time functions that use all 32 bits
  
  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: May 6, 2015
  
  These functions are provided "as is" in the hopes that they might serve some
  higher purpose.  If you want these functions to serve some lower purpose,
  then that too is all right.  So as to keep these hopes alive you may
  freely distribute these functions so long as this header remains intact.
  You may also freely distribute modified versions of these functions so long
  as you indicate that such versions are modified and so long as you
  provide access to the unmodified original copy.
  
  Note: The most recent version of these functions may be obtained from
        http://els-software.org

  The following functions support Unix time beyond Jan 19 03:14:08 2038 GMT,
  assuming that the future standard will treat the 32-bits used by Unix's
  present-day file system as unsigned.

  These functions work by mapping years within the region 2038-2106 down
  into the region of 2010-2037.  All fields of the "tm" structure,
  including those fields dealing with day-of-week and daylight savings time
  are correct!  Bear in mind, however, that the definition of daylight
  savings time changes with the whims of man, thus the notion of daylight
  savings held during 2010-2037 may not be the same as the notion held
  thereafter.

  See also: time(3)

  ****************************************************************************/

#include "sysdefs.h"

#include "defs.h"
#include "time32.h"
#include "sysInfo.h"

/*****************************************************************************/
#if defined(HAVE_LONG_LONG_TIME)

struct tm *localtime32_r(const time_t *clock, struct tm *res)
{return localtime_r(clock, res);}

struct tm *gmtime32_r(const time_t *clock, struct tm *res)
{return gmtime_r(clock, res);}

#ifdef USE_POSIX_TIME_R
char *asctime32_r(const struct tm *tm, char *buf)
{return asctime_r(tm, buf);}

char *ctime32_r(const time_t *clock, char *buf)
{return ctime_r(clock, buf);}
#else
char *asctime32_r(const struct tm *tm, char *buf, int buflen)
{return asctime_r(tm, buf, buflen);}

char *ctime32_r(const time_t *clock, char *buf, int buflen)
{return ctime_r(clock, buf, buflen);}
#endif

struct tm *localtime32(const time_t *clock)
{return localtime(clock);}

struct tm *gmtime32(const time_t *clock)
{return gmtime(clock);}

size_t strftime32(char *str, size_t max,
		  const char *format, const struct tm *tm)
{return strftime(str, max, format, tm);}

char *asctime32(const struct tm *tm)
{return asctime(tm);}

char *ctime32(const time_t *clock)
{return ctime(clock);}

#ifdef HAVE_MKTIME
time_t mktime32(const struct tm *tm)
{return mktime(tm);}
#endif

#ifdef HAVE_TIMELOCAL
/* FreeBSD/Darwin do NOT declare these args as "const": */
time_t timelocal32(struct tm *tm)
{return timelocal(tm);}

time_t timegm32(struct tm *tm)
{return timegm(tm);}
#endif

/*****************************************************************************/
#else

Local void mapclock32(time_t *clock, int *years);
Local void mapyears32(int *years, time_t *clock);

#if !defined(HAVE_TIME_R)
struct tm *localtime_r(const time_t *clock, struct tm *xtime);
struct tm *gmtime_r(const time_t *clock, struct tm *xtime);
# ifdef USE_POSIX_TIME_R
char *asctime_r(const struct tm *tm, char *buf);
char *ctime_r(const time_t *clock, char *buf);
# else
char *asctime_r(const struct tm *tm, char *buf, int buflen);
char *ctime_r(const time_t *clock, char *buf, int buflen);
# endif
#endif /* !defined(HAVE_TIME_R) */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

struct tm *localtime32_r(const time_t *clock, struct tm *xtime)
{
  time_t xclock = *clock;
  int xyears;
  mapclock32(&xclock, &xyears);
  localtime_r(&xclock, xtime);
  xtime->tm_year += xyears;
  return(xtime);
}


struct tm *localtime32(const time_t *clock)
{
  static struct tm xtime; /* return value must be static */
  return(localtime32_r(clock, &xtime));
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

struct tm *gmtime32_r(const time_t *clock, struct tm *xtime)
{
  time_t xclock = *clock;
  int xyears;
  mapclock32(&xclock, &xyears);
  gmtime_r(&xclock, xtime);
  xtime->tm_year += xyears;
  return(xtime);
}


struct tm *gmtime32(const time_t *clock)
{
  static struct tm xtime; /* return value must be static */
  return(gmtime32_r(clock, &xtime));
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* NB: strftime32 will work correctly for all years > 2038 IF and only
   IF the "tm" parameter given it was generated using one of the
   time32 functions: localtime32(), localtime32_r(), gmtime32(), or
   gmtime32_r(). */
/* NB: strftime() is known to be in SunOS4/5, HPUX10/11, Linux2.2/4/6;
   So far Only SunOS5 is known to have cftime() and ascftime() */
size_t strftime32(char *str, size_t max,
		  const char *format, const struct tm *tm)
{
#if defined(SUNOS)
  /* SunOS 5.8, 5.9, 5.10 has a quirk where "strftime(..., ..., "%a", tm)"
     corrupts tzname[0] and tzname[1] for certain values of tm_year, e.g.
     "edate -C 0xd0700000" currupts tzname[], but "edate -C 0xd0800000"
     doesn't.  Setting tm_year within spec fixes this problem, but creates
     a different problem if asked to print the year.  All other OSes
     including SunOS5.7 appear to take tm_year at face value. */
  if (osVersion == 0) osVersion = get_osVersion();
  if (osVersion >= 50800)
  {
    struct tm xtime = *tm;
    time_t xclock = 0;
    mapyears32(&xtime.tm_year, &xclock);
    return(strftime(str, max, format, &xtime));
  }
#endif
  /* Fall through for OS versions that take tm_year at face value: */
  return(strftime(str, max, format, tm));
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* NB: asctime32() and asctime32_r() will work correctly for all years > 2038
   IF and only IF the "tm" parameter given it was generated using one of the
   time32 functions: localtime32(), localtime32_r(), gmtime32(), or
   gmtime32_r(). */

#ifdef USE_POSIX_TIME_R
char *asctime32_r(const struct tm *tm, char *buf)
{
  return(asctime_r(tm, buf));
}
#else
char *asctime32_r(const struct tm *tm, char *buf, int buflen)
{
  return(asctime_r(tm, buf, buflen));
}
#endif


char *asctime32(const struct tm *tm)
{
# define BUF_SIZE (26+8)  /* 8 bytes of slack */
  static char buf[BUF_SIZE]; /* return value must be static */
#ifdef USE_POSIX_TIME_R
  return(asctime32_r(tm, buf));
#else
  return(asctime32_r(tm, buf, BUF_SIZE));
#endif
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifdef USE_POSIX_TIME_R
char *ctime32_r(const time_t *clock, char *buf)
{
  struct tm xtime;
  return(asctime32_r(localtime32_r(clock, &xtime), buf));
}
#else
char *ctime32_r(const time_t *clock, char *buf, int buflen)
{
  struct tm xtime;
  return(asctime32_r(localtime32_r(clock, &xtime), buf, buflen));
}
#endif


char *ctime32(const time_t *clock)
{
# define BUF_SIZE (26+8)  /* 8 bytes of slack */
  static char buf[BUF_SIZE]; /* return value must be static */
#ifdef USE_POSIX_TIME_R
  return(ctime32_r(clock, buf));
#else
  return(ctime32_r(clock, buf, BUF_SIZE));
#endif
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* mktime is known to be supported by: HPUX 10+, SunOS5+, Linux2.2+, OSF1
   mktime is known to NOT be supported by: SunOS4 */
#ifdef HAVE_MKTIME
time_t mktime32(const struct tm *tm)
{
  struct tm xtime = *tm;
  time_t xclock;
  mapyears32(&xtime.tm_year, &xclock);
  return(mktime(&xtime) + xclock);
}
#endif /*HAVE_MKTIME*/


/* timelocal and timegm are only known to be supported by SunOS4.  Perhaps
   older BSD-based OSes also support them, but POSIX based UNIXes do not. */
#ifdef HAVE_TIMELOCAL
time_t timelocal32(struct tm *tm)
{
  struct tm xtime = *tm;
  time_t xclock;
  mapyears32(&xtime.tm_year, &xclock);
  return(timelocal(&xtime) + xclock);
}


time_t timegm32(struct tm *tm)
{
  struct tm xtime = *tm;
  time_t xclock;
  mapyears32(&xtime.tm_year, &xclock);
  return(timegm(&xtime) + xclock);
}
#endif /*HAVE_TIMELOCAL*/


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define  _28_YEARS   ( 28 * SECS_PER_YEAR)
#define  _68_YEARS   ( 68 * SECS_PER_YEAR)
#define  _90_YEARS   ( 88 * SECS_PER_YEAR + 2*365*SECS_PER_DAY)
#define  _96_YEARS   ( 96 * SECS_PER_YEAR)
#define  JAN_1_2100  (128 * SECS_PER_YEAR + 2*365*SECS_PER_DAY)
#define  JAN_1_2102  (JAN_1_2100 + 2*365*SECS_PER_DAY) /* leap year missing */
#define  JAN_1_2106  (JAN_1_2102 + 4*SECS_PER_YEAR)
#define  TIME32      ((Ulong)0x80000000)

Local void mapclock32(time_t *clock, int *years)
{
  Ulong xclock = (Ulong)*clock;
  int xyears = 0;

  /* Prevent certain processors (e.g. DEC_ALPHA) from sign extending: */
  if (sizeof(Ulong) > 4) xclock &= 0xffffffff;

  /* Years from 1970 up until Jan 19 03:14:08 2038 GMT need no mapping. */

  if (xclock >= TIME32)
  {
    /* Map years beyond Jan 19 03:14:08 2038 GMT: */
    if (xclock >= JAN_1_2100)
    {
      if (xclock < JAN_1_2102)
      {
	/* Map years 2100 and 2101:
	   (These two years must be contiguously mapped in order for
	   localtime(JAN_1_2101) to return the correct "tm_yday" value.
	   Hint: think about how Jan 1 2101, 00:00 GMT maps into
	   Dec 31 2100 LOCAL TIME). */
	
	xclock -= _90_YEARS;
	xyears = 90;
      }
      else if (xclock < JAN_1_2106)
      {
	/* Map years from 2102 up until 2106: */
	xclock -= _68_YEARS;
	xclock += SECS_PER_DAY; /* Compensate for missing leap year in 2100! */
	xyears = 68;
      }
      else
      {
	/* Map years from Jan 1 2106 up until Feb 7 04:28:14 2106 GMT: */
	xclock -= _96_YEARS;
	xclock += SECS_PER_DAY; /* Compensate for missing leap year in 2100! */
	xyears = 96;
      }
    }
    else
    {
      /* Map years from 2038 up until 2100: */
      while (xclock >= TIME32)
      {
	xclock -= _28_YEARS;
	xyears += 28;
      }
    }
  }

  *clock = xclock;
  *years = xyears;
  return;
}


#define  _1970  ( 70)
#define  _2038  ( 68 + _1970)
#define  _2100  (130 + _1970)
#define  _2102  (132 + _1970)
#define  _2106  (136 + _1970)

Local void mapyears32(int *years, time_t *clock)
{
  Ulong xclock = 0;
  int xyears = *years;
    
  /* Years from 1970 up until Jan 19 03:14:08 2038 GMT need no mapping. */

  if (xyears >= _2038)
  {
    /* Map years beyond Jan 19 03:14:08 2038 GMT: */
    if (xyears >= _2100)
    {
      if (xyears < _2102)
      {
	/* Map years 2100 and 2101:
	   (These two years must be contiguously mapped in order for
	   localtime(JAN_1_2101) to return the correct "tm_yday" value.
	   Hint: think about how Jan 1 2101, 00:00 GMT maps into
	   Dec 31 2100 LOCAL TIME). */
	
	xyears -= 90;
	xclock = _90_YEARS;
      }
      else if (xyears < _2106)
      {
	/* Map years from 2102 up until 2106: */
	xyears -= 68;
	xclock = _68_YEARS;
	xclock -= SECS_PER_DAY; /* Compensate for missing leap year in 2100! */
      }
      else
      {
	/* Map years from Jan 1 2106 up until Feb 7 04:28:14 2106 GMT: */
	xyears -= 96;
	xclock = _96_YEARS;
	xclock -= SECS_PER_DAY; /* Compensate for missing leap year in 2100! */
      }
    }
    else
    {
      /* Map years from 2038 up until 2100: */
      while (xyears >= _2038)
      {
	xyears -= 28;
	xclock += _28_YEARS;
      }
    }
  }

  *years = xyears;
  *clock = xclock;
  return;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* The following ERSATZ *time_r() routines localtime_r(), gmtime_r(),
   asctime_r(), and ctime_r() are provided here as cheap substitutes to be
   used ONLY by those Unixes that are lacking these functions.  Moreover,
   when building on a Unix that has NATIVE support for the following ERSATZ
   functions be sure to define HAVE_TIME_R so that the compiler will instead
   make use of its NATIVE *time_r() routines.

   NOTE: If you must resort to using the following ERSATZ *time_r() routines
   then as a consequence the *time32_r() functions in the previous section
   will not be fully reentrant.  HOWEVER, if you can AVOID using the
   following routines and instead make use of your Unix's NATIVE *time_r()
   routines then as a result the *time32_r() in the previous section will
   also be reentrant! */

#if !defined(HAVE_TIME_R)

struct tm *localtime_r(const time_t *clock, struct tm *xtime)
{
  *xtime = *localtime(clock);
  return(xtime);
}
struct tm *gmtime_r(const time_t *clock, struct tm *xtime)
{
  *xtime = *gmtime(clock);
  return(xtime);
}

#include <string.h>
# ifdef USE_POSIX_TIME_R
char *asctime_r(const struct tm *tm, char *buf)
{
  strcpy(buf, asctime(tm));
  return(buf);
}
char *ctime_r(const time_t *clock, char *buf)
{
  strcpy(buf, ctime(clock));
  return(buf);
}
# else
char *asctime_r(const struct tm *tm, char *buf, int buflen)
{
  strncpy(buf, asctime(tm), buflen);
  return(buf);
}
char *ctime_r(const time_t *clock, char *buf, int buflen)
{
  strncpy(buf, ctime(clock), buflen);
  return(buf);
}
# endif

#endif /* !defined(HAVE_TIME_R) */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*****************************************************************************/
#endif /* !defined(HAVE_LONG_LONG_TIME) */
