/******************************************************************************
  getdate32.c -- get ISO8601/LS-style/unix-clock date using all 32 bits
  
  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: November 26, 2012
  
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
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

/* Needed for get_validTZ(): */
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include "defs.h"
#include "getdate32.h"
#include "time32.h"
#include "auxil.h"

// extern char *getenv();   // defined in stdlib.h

Local void validate_year(char *arg, char *cp, Ulong YYYY, Boole next);
Local void validate_month(char *arg, char *cp, Ulong MM, Boole next);
Local void validate_day(char *arg, char *cp, Ulong DD, Boole next);
Local void validate_hour(char *arg, char *cp, Ulong hh, Boole next);
Local void validate_minute(char *arg, char *cp, Ulong mm, Boole next);
Local void validate_second(char *arg, char *cp, Ulong ss, Boole next);
Local void date2_error_msg(char *arg, char *cp, char *msg);
Local void syntax_error_msg(char *arg, char *cp, char *msg);
Local void date_error_msg(char *arg, char *msg, char *ptr);

#if defined(HAVE_LONG_LONG_TIME)
# define MAX_YYYY 9999
#else
# define MAX_YYYY 2106
#endif


/* Note: The global variable LGC can be either 'L', 'G', 'C', or ' ' (i.e.
   blank), where 'L' == Localtime, 'G' == GMT, 'C' == 32-bit Unix Clock
   value.  Additionally if LGC is ' ', then getdate32() has the latitude to
   select either 'L' or 'G' based on whether 'GMT' is specified in the
   timezone portion of an LS-style/alpha-numeric date.

   Giving 'GMT' in the timezone portion without specifying either 'G' or ' '
   as the LGC value is considered an error.  Giving a non-Unix clock value
   while specifying 'C' as the LGC value is also considered an error. */

time_t getdate32(char *arg, char **ptr)
{
  time_t t = 0;
  char *cp;
  Boole ls_style = FALSE;
  Boole ddmonyyyy_style = FALSE;/* DD mon YYYY or DD-mon-YYYY */
  Boole dos_style = FALSE;	/* MM-DD-YY or MM-DD-YYYY */
  Boole windows_style = FALSE;	/* MM/DD/YY or MM/DD/YYYY */
  Boole yyyymondd_style = FALSE;/* YYYY mon DD or YYYY-mon-DD */
  Boole yyyymmdd_style = FALSE;	/* YYYY MM DD or YYYY-MM-DD */
  Boole iso_style = FALSE;	/* YYYYMMDD non-punctuated number */
  Boole clock_style = FALSE;	/* -C followed by non-punctuated number */
  char save_lgc = LGC;
  static char *weekday_name[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"};
  static char *month_name[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"};

  if (arg == NULL) usage_error();
  if (ptr)
    cp = *ptr;
  else
    cp = arg;
  
  /* Scan any leading blanks and determine date style: */
  while (*cp != CNULL && isSpace(*cp)) cp++;

  if (isAlpha(*cp))
    ls_style = TRUE;
  else if (isDigit(*cp))
  {
    Boole profiled = FALSE;
    /* Profile <digit><next> or <digit><digit><next>: */
    char *next_dd = (! isDigit(cp[1]) ? &cp[1] :
		     ! isDigit(cp[2]) ? &cp[2] :
		     NULL);
    /* Profile <digit><digit><digit><digit><next>: */
    char *next_yyyy = (isDigit(cp[1]) && isDigit(cp[2]) && isDigit(cp[3]) &&
		       ! isDigit(cp[4]) ? &cp[4] :
		       NULL);
    /* NB: If next_dd or next_yyyy is '-' then no spaces can follow after */

    if (!profiled && next_dd != NULL)
    {
      char *after = next_dd;
      while (isSpace(*after)) after++;
      profiled = TRUE;
      if ((*next_dd == '-' && isAlpha(next_dd[1])) ||
	  (isSpace(*next_dd) && isAlpha(*after)))
	ddmonyyyy_style = TRUE;
      else if (*next_dd == '-' && isDigit(next_dd[1]))
	dos_style = TRUE;
      else if (*next_dd == '/')
	windows_style = TRUE;
      else
	profiled = FALSE;
    }
    if (!profiled && next_yyyy != NULL)
    {
      char *after;
      after = next_yyyy;
      while (isSpace(*after)) after++;
      profiled = TRUE;
      if ((*next_yyyy == '-' && isAlpha(next_yyyy[1])) ||
	  (isSpace(*next_yyyy) && isAlpha(*after)))
	yyyymondd_style = TRUE;
      else if ((*next_yyyy == '-' && isDigit(next_yyyy[1])) ||
	       (isSpace(*next_yyyy) && isDigit(*after)))
	yyyymmdd_style = TRUE;
      else
	profiled = FALSE;
    }
    if (!profiled)
    {
      profiled = TRUE;
      if (LGC == 'C')
	clock_style = TRUE;
      else
	iso_style = TRUE;
    }
  }
  else if (*cp == '-' && isDigit(cp[1]) && LGC == 'C')
    /* Assume that a negative clock value is really an unsigned 32-bit
       value greater than 2^31 that was mistreated as a signed shell
       value (e.g. /bin/csh or /bin/tcsh come to mind): */
    clock_style = TRUE;
  else
    usage_error();

  if (!clock_style && LGC == 'C')
  {
    char msg[80];
    sprintf(msg, "%s dates must be Local time or GMT",
	    ls_style ? "LS-style/alpha-numeric" :
	    ddmonyyyy_style ? "DD-MON-YYYY style" :
	    dos_style ? "DOS-style" :
	    windows_style ? "Windows-style" :
	    yyyymondd_style ? "YYYY-MON-DD style" :
	    yyyymmdd_style ? "YYYY-MM-DD style" :
	    "ISO8601-style");
    date2_error_msg(arg, cp+1, msg);
  }

  if (ls_style || ddmonyyyy_style)
  {
    /* LS-style dates: first convert to ISO8601, and then convert to clock */
    Ulong YYYY = 0, MM = 0, WD, DD = 0, hh = 0, mm = 0, ss = 0;
    Boole have_year = FALSE;
    Boole have_time = FALSE;
    Boole have_timezone = FALSE;
    Boole looking;
    char *next;
    int len = 0;

    if (ls_style)
    {
      /* Ignore optional weekday: */
      len = 0;
      while (isAlpha(cp[len])) len++;
      if (len >= 3)
      {
	/* A weekday name must have at least 3 characters that match: */
	for (WD = 0; WD < 7; WD++)
	  if (strncmp_ci(weekday_name[WD], cp, len) == 0) break;
	if (WD < 7)
	{
	  cp += len;
	  while (*cp != CNULL && isSpace(*cp)) cp++;
	}
      }
      
      len = 0;
      while (isAlpha(cp[len])) len++;
      if (len >= 3)
      {
	/* A month name must have 3 or more characters that match: */
	for (MM = 0; MM < 12; MM++)
	  if (strncmp_ci(month_name[MM], cp, len) == 0) break;
	MM++;
      }
      else
      {
	MM = 0;
      }
      cp += len;
      validate_month(arg, cp, MM, isSpace(*cp) != 0);
      
      DD = strtoul(cp, &cp, 10);
      validate_day(arg, cp, DD, *cp == CNULL || isSpace(*cp) || *cp == ',');
    }
    else if (ddmonyyyy_style)
    {
      DD = strtoul(cp, &cp, 10);
      validate_day(arg, cp, DD, isSpace(*cp) || *cp == '-');
      
      cp++;
      len = 0;
      while (isAlpha(cp[len])) len++;
      if (len >= 3)
      {
	/* A month name must have 3 or more characters that match: */
	for (MM = 0; MM < 12; MM++)
	  if (strncmp_ci(month_name[MM], cp, len) == 0) break;
	MM++;
      }
      else
      {
	MM = 0;
      }
      cp += len;
      validate_month(arg, cp, MM,
		     *cp == CNULL || isSpace(*cp) || *cp == '-' || *cp == ',');
      if (*cp == '-' && isDigit(cp[1])) cp++;
    }

    /* Some people like putting a comma before the year/time: */
    if (*cp == ',') cp++;

    looking = TRUE;
    while (looking)
    {
      while (*cp != CNULL && isSpace(*cp)) cp++;
      if (isDigit(*cp))
      {
	Void strtoul(cp, &next, 10);
	if (*next != ':')
	{
	  if (!have_year)
	  {
	    YYYY = strtoul(cp, &cp, 10);
	    validate_year(arg, cp, YYYY, *cp == CNULL || isSpace(*cp));
	    have_year = TRUE;
	  }
	  else
	    looking = FALSE; /* Only one year allowed */
	}
	else
	{
	  if (!have_time)
	  {
	    hh = strtoul(cp, &cp, 10);
	    if (*cp != ':') date_error_msg(arg, "Invalid time", cp);
	    cp++;
	    if (!isDigit(*cp)) date_error_msg(arg, "Invalid time", cp);
	    mm = strtoul(cp, &cp, 10);
	    if (*cp == ':')
	    {
	      cp++;
	      if (!isDigit(*cp)) date_error_msg(arg, "Invalid time", cp);
	      ss = strtoul(cp, &cp, 10);
	    }
	    /* Some ls-style dates (e.g. MS Mail headers) have AM/PM: */
	    while (*cp != CNULL && isSpace(*cp)) cp++;
	    if ((strncmp_ci(cp, "AM", 2) == 0 ||
		 strncmp_ci(cp, "PM", 2) == 0) &&
		(*(cp+2) == CNULL || isSpace(*(cp+2))))
	    {
	      /* 12am        --> 0 hours
		 1am to 11am --> 1 to 11 hours
		 12pm        --> 12 hours
		 1pm to 11pm --> 13 to 23 hours */
	      Boole am = (*cp == 'a' || *cp == 'A');
	      Boole pm = (*cp == 'p' || *cp == 'P');
	      cp += 2;
	      if (hh == 0 || hh > 12) date2_error_msg(arg, cp, "Hour out of range");
	      if (am && hh == 12) hh = 0;
	      if (pm && hh != 12) hh += 12;
	    }
	    have_time = TRUE;
	  }
	  else
	    looking = FALSE; /* Only one time allowed */
	}
      }
      else if (isAlpha(*cp))
      {
	if (!have_timezone)
	{
	  if (strncmp(cp, "GMT", 3) == 0) 
	  {
	    if (LGC == 'L')
	      date_error_msg(arg, "Date is not Local time", cp);
	    else if (LGC == ' ')
	      LGC = 'G';
	    cp += 3;
	  }
	  else
	  {
	    if (LGC == 'G')
	      date_error_msg(arg, "Date is not GMT", cp);
	    else if (LGC == ' ')
	      LGC = 'L';
	    while (*cp != CNULL && !isSpace(*cp)) cp++;
	  }
	  have_timezone = TRUE;
	}
	else
	  looking = FALSE; /* Only one timezone allowed */
      }
      else
      {
	looking = FALSE; /* End of date */
      }
    }

    /* There should be no data leftover: */
    if (*cp != CNULL) date_error_msg(arg, NULL, cp);

    /* If no year given then make an intelligent guess: */
    if (!have_year)
    {
      /* It is typical of "ls -l" listings to not bother giving the year
	 unless the file is more than 6 months old.  Hence, the best we can
	 do is assume that the missing year is the most recent gone by: */
      struct tm arg_time;
      time_t the_time = time(NULL);
      localtime32_r(&the_time, &arg_time);
      YYYY = arg_time.tm_year + 1900;
      if ((MM - 1 > arg_time.tm_mon) ||
	  (MM - 1 == arg_time.tm_mon && DD > arg_time.tm_mday))
	YYYY--;
    }

    /* Finally, convert ISO8601 date to clock: */
    {
      char tmp_date[128];
      sprintf(tmp_date, "%04lu%02lu%02lu.%02lu%02lu%02lu",
	      YYYY, MM, DD, hh, mm, ss);
      /* There should be exactly 15 characters in the date: */
      if (strlen(tmp_date) != 15) date_error_msg(arg, NULL, cp);
      
      t = getdate32(tmp_date, NULL);
    }
  }
  else if (dos_style || windows_style)
  {
    /* DOS-style dates: first convert to ISO8601, and then convert to clock */
    Ulong YYYY = 0, MM, DD, hh = 0, mm = 0, ss = 0;
    char splitter = (dos_style ? '-' : '/');
    
    MM = strtoul(cp, &cp, 10);
    validate_month(arg, cp, MM, *cp == splitter);
    cp++;

    DD = strtoul(cp, &cp, 10);
    validate_day(arg, cp, DD, *cp == splitter);
    cp++;

    if (!isDigit(*cp)) date2_error_msg(arg, cp, "Invalid year");
    YYYY = strtoul(cp, &cp, 10);
    /* 00-68 --> 2000-2068,
       70-99 --> 1970-1999,
       69 is undefined */
    if (YYYY <= 68)
      YYYY += 2000;
    else if (YYYY >= 70 && YYYY <= 99)
      YYYY += 1900;
    validate_year(arg, cp, YYYY, *cp == CNULL || isSpace(*cp));

    while (*cp != CNULL && isSpace(*cp)) cp++;
    if (isDigit(*cp))
    {
      hh = strtoul(cp, &cp, 10);
      validate_hour(arg, cp, hh, *cp == ':');
      cp++;

      if (!isDigit(*cp)) date2_error_msg(arg, cp, "Invalid time");
      mm = strtoul(cp, &cp, 10);
      validate_minute(arg, cp, mm, *cp == CNULL || isSpace(*cp) ||
		      IS_MEMBER(*cp, ":aApP"));
      if (*cp == ':')
      {
	cp++;
	if (!isDigit(*cp)) date2_error_msg(arg, cp, "Invalid time");
	ss = strtoul(cp, &cp, 10);
	validate_second(arg, cp, ss, *cp == CNULL || isSpace(*cp) ||
			IS_MEMBER(*cp, "aApP"));
      }
      while (*cp != CNULL && isSpace(*cp)) cp++;
      if (IS_MEMBER(*cp, "aApP"))
      {
	/* 12am        --> 0 hours
	   1am to 11am --> 1 to 11 hours
	   12pm        --> 12 hours
	   1pm to 11pm --> 13 to 23 hours */
	Boole am = (*cp == 'a' || *cp == 'A');
	Boole pm = (*cp == 'p' || *cp == 'P');
	cp++;
	if (hh == 0 || hh > 12) date2_error_msg(arg, cp, "Hour out of range");
	if (am && hh == 12) hh = 0;
	if (pm && hh != 12) hh += 12;
	/* Following [aApP], dots and/or [mM] are optional: */
	if (*cp == '.') cp++;
	if (*cp == 'm' || *cp == 'M')
	{
	  cp++;
	  if (*cp == '.') cp++;
	}
      }
    }

    while (*cp != CNULL && isSpace(*cp)) cp++;
    
    /* There should be no data leftover: */
    if (*cp != CNULL) date2_error_msg(arg, cp, NULL);
    
    /* Finally, convert ISO8601 date to clock: */
    {
      char tmp_date[128];
      sprintf(tmp_date, "%04lu%02lu%02lu.%02lu%02lu%02lu",
	      YYYY, MM, DD, hh, mm, ss);
      /* There should be exactly 15 characters in the date: */
      if (strlen(tmp_date) != 15) date2_error_msg(arg, cp, NULL);
      
      t = getdate32(tmp_date, NULL);
    }
  }
  else if (yyyymondd_style || yyyymmdd_style)
  {
    /* yyyymondd-style or yyyymndd-style dates: first convert to ISO8601,
       and then convert to clock */
    Ulong YYYY = 0, MM = 0, DD = 0, hh = 0, mm = 0, ss = 0;

    YYYY = strtoul(cp, &cp, 10);
    validate_year(arg, cp, YYYY, isSpace(*cp) || *cp == '-');
    if (*cp == '-') cp++;

    if (yyyymondd_style)
    {
      int len = 0;
      while (isSpace(*cp)) cp++;
      while (isAlpha(cp[len])) len++;
      if (len >= 3)
      {
	/* A month name must have 3 or more characters that match: */
	for (MM = 0; MM < 12; MM++)
	  if (strncmp_ci(month_name[MM], cp, len) == 0) break;
	MM++;
      }
      else
      {
	MM = 0;
      }
      cp += len;
    }
    else
    {
      MM = strtoul(cp, &cp, 10);
    }
    validate_month(arg, cp, MM, isSpace(*cp) || *cp == '-');
    if (*cp == '-') cp++;

    DD = strtoul(cp, &cp, 10);
    validate_day(arg, cp, DD, isSpace(*cp) || *cp == CNULL);
    while (isSpace(*cp)) cp++;

    if (*cp != CNULL)
    {
      hh = strtoul(cp, &cp, 10);
      if (*cp != ':') date_error_msg(arg, "Invalid time", cp);
      cp++;
      if (!isDigit(*cp)) date_error_msg(arg, "Invalid time", cp);
      mm = strtoul(cp, &cp, 10);
      if (*cp == ':')
      {
	cp++;
	if (!isDigit(*cp)) date_error_msg(arg, "Invalid time", cp);
	ss = strtoul(cp, &cp, 10);
      }
      /* Some ls-style dates (e.g. MS Mail headers) have AM/PM: */
      while (*cp != CNULL && isSpace(*cp)) cp++;
      if ((strncmp_ci(cp, "AM", 2) == 0 ||
	   strncmp_ci(cp, "PM", 2) == 0) &&
	  (*(cp+2) == CNULL || isSpace(*(cp+2))))
      {
	/* 12am        --> 0 hours
	   1am to 11am --> 1 to 11 hours
	   12pm        --> 12 hours
	   1pm to 11pm --> 13 to 23 hours */
	Boole am = (*cp == 'a' || *cp == 'A');
	Boole pm = (*cp == 'p' || *cp == 'P');
	cp += 2;
	if (hh == 0 || hh > 12) date2_error_msg(arg, cp, "Hour out of range");
	if (am && hh == 12) hh = 0;
	if (pm && hh != 12) hh += 12;
      }
    }

    /* There should be no data leftover: */
    if (*cp != CNULL) date_error_msg(arg, NULL, cp);

    /* Finally, convert ISO8601 date to clock: */
    {
      char tmp_date[128];
      sprintf(tmp_date, "%04lu%02lu%02lu.%02lu%02lu%02lu",
	      YYYY, MM, DD, hh, mm, ss);
      /* There should be exactly 15 characters in the date: */
      if (strlen(tmp_date) != 15) date_error_msg(arg, NULL, cp);
      
      t = getdate32(tmp_date, NULL);
    }
  }
  else if (clock_style)
  {
    t = (time_t)strtoul(cp, &cp, 0);
    while (*cp != CNULL && isSpace(*cp)) cp++;
    if (*cp != CNULL)
      date_error_msg(arg, "Clock contains non-numeric character", cp);
  }
  else if (iso_style)
  {
    t = convert_iso8601(arg, &cp, FALSE);
    while (*cp != CNULL && isSpace(*cp)) cp++;
    if (*cp != CNULL)
      date_error_msg(arg, "Date contains invalid character", cp);
  }

  /* The only 32-bit time value not allowed is "-1" or 0xffffffff because
     this could also be confused with the return value denoting failure: */
  if (t == (time_t)-1)
    date_error_msg(arg, "Date is out of range", cp-1);

#if defined(HAVE_LONG_LONG_TIME)
  /* gmtime(-67768040609697600)   == Thu Jan  1 12:00:00 GMT -2147481748
     gmtime(67767976233485999)    == Tue Dec 31 10:59:59 GMT 2147483647
     export TZ=Pacific/Fiji;
     localtime(67767976233485999) == Tue Dec 31 23:59:59 FJST 2147483647
     See TECH_NOTES, TN#4 for more explanation. */
  if (t < -67768040609697600)
    date_error_msg(arg, "Cannot represent years before -2147481748", cp-1);
  if (t >  67767976233485999)
    date_error_msg(arg, "Cannot represent years beyond 2147483647", cp-1);
#endif

  if (ptr) *ptr = cp;
  LGC = save_lgc;

  return(t);
}


/* Convert YYYYMMDD.hhmmss to clock time: */
time_t convert_iso8601(char *arg, char **ptr, Boole max_time)
{
  time_t t = 0;
  char *cp;
  int len;
  long n, min, hour, yday, days, month, year;
  Boole isleapyear = FALSE;

  if (arg == NULL) usage_error();
  if (ptr)
    cp = *ptr;
  else
    cp = arg;

  /* Special case of zero ISO8601 date that is independent of the
     local time zone: */
  if (strncmp(cp, "00000000.000000", 15) == 0)
  {
    cp += 15;
    if (ptr) *ptr = cp;
    return(0);
  }
  
  len = 0;
  n = 0;
  min = 0;
  hour = 0;
  yday = 0;
  days = 0;
  month = 0;
  year = 0;
  while (isDigit(*cp))
  {
    len++;
    if ((len & 1) != 0)
      n = (*cp++ - '0') * 10;
    else
    {
      n += (*cp++ - '0');
      switch(len)
      {
      case 2: /* YY: century */
	year = n * 100;
	break;
	
      case 4: /* YY: year */
	year += n;
	validate_year(arg, cp, year, TRUE);
	days = (year - 1970) * 365;
	
	/* Tack on days for leap years in years already gone by (the first year
	   this occurs is 1973): */
	days += ((year - 1969) / 4);
	if (year > 2100)
	{
	  int c = ((year - 2001) / 100);
	  int cs = c / 4;
	  days -= (c - cs);
	}
	
	/* Determine whether the current year is a leap-year: */
	isleapyear = (year % 4 == 0);
	if (isleapyear && year % 100 == 0)
	  isleapyear = (year % 400 == 0);
	break;
	
      case 6: /* MM: month */
	switch (n) {
	case 12:
	  yday += 30;
	case 11:
	  yday += 31;
	case 10:
	  yday += 30;
	case 9:
	  yday += 31;
	case 8:
	  yday += 31;
	case 7:
	  yday += 30;
	case 6:
	  yday += 31;
	case 5:
	  yday += 30;
	case 4:
	  yday += 31;
	case 3:
	  yday += (isleapyear ? 29 : 28);
	case 2:
	  yday += 31;
	case 1:
	  break;
	default:
	  date_error_msg(arg, "Invalid month", cp-1);
	}
	month = n;
	break;
	
      case 8: /* DD: day */
	{
	  int last_day;
	  switch (month) {
	  case 12:
	  case 10:
	  case 8:
	  case 7:
	  case 5:
	  case 3:
	  case 1:
	    last_day = 31;
	    break;
	  case 11:
	  case 9:
	  case 6:
	  case 4:
	    last_day = 30;
	    break;
	  case 2:
	    last_day = (isleapyear ? 29 : 28);
	    break;
	  default:
	    last_day = 0;
	    break;
	  }
	  if (n < 1 || n > last_day)
	    date_error_msg(arg, "Invalid day", cp-1);
	}
	yday = yday + n - 1;
	days += yday;
	t = days * SECS_PER_DAY;
	break;
	
      case 10: /* hh: hour */
	if (n > 23)
	  date_error_msg(arg, "Invalid hour", cp-1);
	hour = n;
	t += hour * SECS_PER_HOUR;
	break;
	
      case 12: /* mm: minute */
	if (n > 59)
	  date_error_msg(arg, "Invalid minute", cp-1);
	min = n;
	t += min * SECS_PER_MIN;
	break;
	
      case 14: /* ss: second */
	if (n > 59)
	  date_error_msg(arg, "Invalid second", cp-1);
	t += n;
	break;
	
      default:
	break;
      }
      /* Ignore optional dot/_ separator in 8th possition: */
      if (len == 8 && IS_MEMBER(*cp, "._")) cp++;
    }
  }

  /* Make sure that date has 8, 10, 12, or 14 numbers while allowing
     for an optional dot/_ separator between date and time: */

  if (IS_MEMBER(*cp, "._"))
    /* Detect optional dot separator other than in 8th possition: */
    date_error_msg(arg, "Date must be in the form of YYYYMMDD.hhmmss", cp);
  else if (len > 14)
    /* Check for too many digits *must* precede check for too few: */
    date_error_msg(arg, "Date has too many digits", cp-1);
  else if (len < 8 || (len & 1) != 0)
    date_error_msg(arg, "Date has too few digits", cp-1);
  
  /* If the max_time flag is active then set any undefined hhmmss time
     paramaters to maximum values, otherwise leave any undefined hhmmss
     time parameters as 0: */
  if (max_time)
  {
    if (len < 10) {t += 23 * SECS_PER_HOUR; hour = 23;}
    if (len < 12) {t += 59 * SECS_PER_MIN;  min  = 59;}
    if (len < 14)  t += 59;
  }

  /* Test if date invalid: */
  if ((time_t)t == -1
#   if defined(HAVE_LONG_LONG_TIME)
      /* FIXME: Need more complete date test for LONG_LONG_TIME */
#   else
      || (year <= 1970 && (time_t)t < 0)
      || (year >= 2106 && (time_t)t >= 0)
#   endif
      )
    date_error_msg(arg, "Date is out of range", cp-1);

  if (LGC == 'L' || LGC == ' ')
  {
    /* Take an initial guess at the GMT offset.  If the date happens to
       be within a few hours of when daylight savings begins/ends, then
       a second guess will be necessary. */

    /* First guess: */
    t += guess_gmtoff(&t, yday, hour, min);
    
    /* Take a second guess in case the date happened to be changing
       between daylight savings and standard time during the first guess: */
#   if defined(HAVE_LONG_LONG_TIME)
    t += guess_gmtoff(&t, yday, hour, min);
#   else
    /* Things get touchy during the year 2106 if TIME32: */
    if (year < 2106)
      t += guess_gmtoff(&t, yday, hour, min);
#   endif
    
    /* Test if conversion to local time made date invalid: */
    if ((time_t)t == -1
#     if defined(HAVE_LONG_LONG_TIME)
	/* FIXME: Need more complete date test for LONG_LONG_TIME */
#     else
	|| (year <= 1970 && (time_t)t < 0)
	|| (year >= 2106 && (time_t)t >= 0)
#     endif
	)
      date_error_msg(arg, "Date is out of range", cp-1);
  }

  if (ptr) *ptr = cp;
  
  return(t);
}


long guess_gmtoff(time_t *clock, int yday, int hour, int min)
{
  struct tm loc;
  int gmt_offset;
  int day_offset;
  
  /* Try to guess the GMT timezone/daylight-savings offset by picking
     a clock value that comes closest to the desired yday and hour parameters.
     (There are simpler ways of doing this, but not all Unixes support the
     tm_gmtoff value in the tm structure.) */
  
  localtime32_r(clock, &loc);
  
  /* Test for year-end wrap-around: */
  if (yday == 0 && loc.tm_yday >= 364)
    day_offset = 1;
  else if (loc.tm_yday == 0 && yday >= 364)
    day_offset = -1;
  else
    day_offset = yday - loc.tm_yday;
  
  gmt_offset = SECS_PER_DAY * day_offset +
    SECS_PER_HOUR * (hour - loc.tm_hour) +
      SECS_PER_MIN * (min - loc.tm_min);

#ifdef DEBUG
  printf("24*3600*(%d=%d-%d) + 3600*(%d-%d) + 60*(%d-%d) = %d\n",
	 day_offset, yday, loc.tm_yday,
	 hour, loc.tm_hour, min, loc.tm_min, gmt_offset);
#endif
    
  return(gmt_offset);
}
/* Example of time needing second guesses (GMT offset == -0500):
% export TZ=US/Eastern
% edate 20151101.0000
24*3600*(1=304-303) + 3600*(0-20) + 60*(0-0) = 14400
24*3600*(0=304-304) + 3600*(0-0) + 60*(0-0) = 0
Sun Nov  1 00:00:00 EDT 2015
% edate 20151101.0100
24*3600*(1=304-303) + 3600*(1-21) + 60*(0-0) = 14400
24*3600*(0=304-304) + 3600*(1-1) + 60*(0-0) = 0
Sun Nov  1 01:00:00 EDT 2015
% edate 20151101.0200
24*3600*(1=304-303) + 3600*(2-22) + 60*(0-0) = 14400
24*3600*(0=304-304) + 3600*(2-1) + 60*(0-0) = 3600
Sun Nov  1 02:00:00 EST 2015
% edate 20151101.0300
24*3600*(1=304-303) + 3600*(3-23) + 60*(0-0) = 14400
24*3600*(0=304-304) + 3600*(3-2) + 60*(0-0) = 3600
Sun Nov  1 03:00:00 EST 2015
% edate 20151101.0400
24*3600*(0=304-304) + 3600*(4-0) + 60*(0-0) = 14400
24*3600*(0=304-304) + 3600*(4-3) + 60*(0-0) = 3600
Sun Nov  1 04:00:00 EST 2015
% edate 20151101.0500
24*3600*(0=304-304) + 3600*(5-1) + 60*(0-0) = 14400
24*3600*(0=304-304) + 3600*(5-4) + 60*(0-0) = 3600
Sun Nov  1 05:00:00 EST 2015
% edate 20151101.0600
24*3600*(0=304-304) + 3600*(6-1) + 60*(0-0) = 18000
24*3600*(0=304-304) + 3600*(6-6) + 60*(0-0) = 0
Sun Nov  1 06:00:00 EST 2015
*/


/* Display date error message and exit: */
Local void date_error_msg(char *arg, char *msg, char *ptr)
{
  if (msg == NULL) msg = "Invalid date";
  carrot_msg(NULL, NULL, arg, msg, ptr);
  exit(1);
}


Local void validate_year(char *arg, char *cp, Ulong YYYY, Boole next)
{
  /* Even though "Dec 31, 1969 23:00 Local time" might be valid,
     the GMT conversion algorithm gets flakey around this time: */
  if (YYYY < 1970)
    date2_error_msg(arg, cp, "Cannot represent years before 1970");
  if (YYYY > MAX_YYYY)
  {
    char msg[64];
    sprintf(msg, "Cannot represent years beyond %d", MAX_YYYY);
    date2_error_msg(arg, cp, msg);
  }
  else if (!next)
    syntax_error_msg(arg, cp, NULL);
  return;
}


Local void validate_month(char *arg, char *cp, Ulong MM, Boole next)
{
  if (MM == 0 || MM > 12)
    date2_error_msg(arg, cp, "Invalid month");
  else if (!next)
    syntax_error_msg(arg, cp, NULL);
  return;
}


Local void validate_day(char *arg, char *cp, Ulong DD, Boole next)
{
  if (DD == 0 || DD > 31)
    date2_error_msg(arg, cp, "Invalid day");
  else if (!next)
    syntax_error_msg(arg, cp, NULL);
  return;
}


Local void validate_hour(char *arg, char *cp, Ulong hh, Boole next)
{
  if (hh > 23)
    date2_error_msg(arg, cp, "Invalid hour");
  else if (!next)
    syntax_error_msg(arg, cp, NULL);
  return;
}


Local void validate_minute(char *arg, char *cp, Ulong mm, Boole next)
{
  if (mm > 59)
    date2_error_msg(arg, cp, "Invalid minute");
  else if (!next)
    syntax_error_msg(arg, cp, NULL);
  return;
}


Local void validate_second(char *arg, char *cp, Ulong ss, Boole next)
{
  if (ss > 59)
    date2_error_msg(arg, cp, "Invalid second");
  else if (!next)
    syntax_error_msg(arg, cp, NULL);
  return;
}


/* Display date error message and exit: */
Local void date2_error_msg(char *arg, char *cp, char *msg)
{
  if (msg == NULL) msg = "Invalid date";
  carrot_msg(NULL, NULL, arg, msg, cp-1);
  exit(1);
}


/* Display syntax error message and exit: */
Local void syntax_error_msg(char *arg, char *cp, char *msg)
{
  if (msg == NULL) msg = "Invalid syntax";
  carrot_msg(NULL, NULL, arg, msg, cp);
  exit(1);
}


#ifdef CREATE_YOUR_OWN_TZ
Local char *append_TZoffset(char *zname, char *zp, struct tm *date)
{
  int h, m;
  zp += strlen(zp);

  /* Some TZs (e.g. GMT+7) have numeric offsets embedded in the name: */
  while (zp != zname && isDigit(zp[-1]))  zp--;
  if (zp[-1] == '+' || zp[-1] == '-') zp--;
  
  h = date->tm_hour;
  m = date->tm_min;
  if (date->tm_mday < 22)
  {
    h = 24 - h;
    if (m > 0) {h--; m = 60 - m;}
  }
  else
  {
    *zp++ = '-';
  }
  sprintf(zp, "%d", h);
  zp += strlen(zp);
  if (m > 0)
  {
    sprintf(zp, ":%02d", m);
    zp += strlen(zp);
  }
  return(zp);
}
#endif /*CREATE_YOUR_OWN_TZ*/


char *guess_TZ(void)
{
  char *tz;
  if ((tz = getenv("TZ")) != NULL)
    return(tz);
  else
  {
#ifndef CREATE_YOUR_OWN_TZ
    return(NULL);
#else
    /* This section is for creating your own TZ environment setting.  If
       you're lucky it might even be the correct value for your timezone
       :-).  Since this section extracts the STD/DST parameters in use on
       your system during the solstices, it is generally correct, although
       if your region changes from STD to DST differently than the United
       States then you might not always get the desired results.  Note: This
       section will generate TZs such as MST7MDT6 instead of the more
       compact MST7MDT, although either one is correct.  This extra step is
       needed to account for irregular cases such as KDT9:30KST10:00 */
    char zname[128], *zp;
    time_t std, dst;
    struct tm date;
    char save_lgc = LGC;
    LGC = 'G';
    std = convert_iso8601("19991222.000000", NULL, FALSE);
    dst = convert_iso8601("19990622.000000", NULL, FALSE);
    LGC = save_lgc;
# ifdef HAVE_TM_ZONE
    localtime32_r(&std, &date);
    zp = zname;
    strcpy(zp, date.tm_zone);
    zp = append_TZoffset(zname, zp, &date);

    localtime32_r(&dst, &date);
    if (date.tm_isdst)
    {
      strcpy(zp, date.tm_zone);
      zp = append_TZoffset(zname, zp, &date);
    }
# else
    {
      extern char *tzname[2];
      localtime32_r(&std, &date);
      zp = zname;
      strcpy(zp, tzname[0]);
      zp = append_TZoffset(zname, zp, &date);
      
      localtime32_r(&dst, &date);
      if (date.tm_isdst)
      {
	strcpy(zp, tzname[1]);
	zp = append_TZoffset(zname, zp, &date);
      }
    }
    
# endif
    /* Some TZs (e.g. Poland under SunOS5.7) have embedded blanks in the
       name which are invalid in TZ environment variable. Change to '_': */
    zp = zname;
    while (*zp != CNULL)
    {
      if (isSpace(*zp)) *zp = '_';
      zp++;
    }

    return(strdup(zname));
#endif /*CREATE_YOUR_OWN_TZ*/
  }
}


char *get_currentTZ(void)
{
  return(getenv("TZ"));
}


int set_currentTZ(char *TZ)
{
  int sts = 0;

  /* Make sure default TZ has been saved: */
  (void)get_defaultTZ();

  if (TZ == NULL)
  {
    /* Delete TZ variable from the environment!: */
    envUnset("TZ");
  }
  else
  {
    static char *last_tz = NULL;
    char *tz = memAlloc(strlen("TZ=") + strlen(TZ) + 1);
    sprintf(tz, "TZ=%s", TZ);
    sts = envSet(tz);
    if (last_tz != NULL) memFree(last_tz);
    last_tz = tz;
  }
  tzset();	/* Is tzset() necessary??? */
  return(sts);
}


char *get_defaultTZ(void)
{
  static Boole defaultTZ_known = FALSE;
  static char *defaultTZ = NULL;
  if (!defaultTZ_known)
  {
    /* Realize that even though the defaultTZ might be NULL, it might be
       '\0', or it might be a string, it is still "known" to be one of these
       things once it's been gotten: */
    defaultTZ = getenv("TZ");
    /* Save a copy of "TZ" JIC it's really necessary!?: */
    if (defaultTZ != NULL) defaultTZ = strdup(defaultTZ);
    defaultTZ_known = TRUE;
  }
  return(defaultTZ);
}


#if defined(SUNOS)
/* Used by SunOS4 and SunOS5: */
#  define ZONEINFO "/usr/share/lib/zoneinfo"
#elif defined(LINUX) || defined(DARWIN) || defined(FREEBSD)
#  define ZONEINFO "/usr/share/zoneinfo"
#elif defined(OSF1)
#  define ZONEINFO "/etc/zoneinfo"
#endif

char *TZ_WARN_PREFACE = "Warning: Timezone may be invalid";
# define TZ_WARN(msg,ptr) \
{ if (syntax_ok) carrot_msg(TZ_WARN_PREFACE, NULL, Current_Arg, msg, ptr); \
  syntax_ok = FALSE; }

char *get_validTZ(char *TZ)
{
  Boole syntax_ok = TRUE;
  Boole zoneinfo_ok = FALSE;
  char *zoneinfo_file = NULL, *zoneinfo_tmp = NULL;
  char *cp, *valid_TZ;

  if (TZ == NULL || *TZ == CNULL)
    return(NULL);

  /* Assume that current TZ is valid unless we changed our minds later on: */
  valid_TZ = TZ;

#ifdef ZONEINFO
  {
    int save_errno;

    cp = TZ;
    if (*TZ == ':') cp++;

    if (*cp == '/')
      zoneinfo_file = cp;
    else
    {
      zoneinfo_tmp = memAlloc(sizeof(ZONEINFO) + strlen(cp) + 2);
      zoneinfo_file = zoneinfo_tmp;
      sprintf(zoneinfo_file, "%s/%s", ZONEINFO, cp);
    }

# ifdef NO_ABSOLUTE_ZONEINFO_PATH
    /* Warn if absolute path used when not allowed: */
    if (*cp == '/') TZ_WARN("Absolute path not allowed", cp);
# endif

    /* Check whether zoneinfo is readable and is a regular file: */
    save_errno = errno;    
    zoneinfo_ok = (access(zoneinfo_file, R_OK) == 0);
    if (zoneinfo_ok)
    {
      struct stat info;
      int sts = stat(zoneinfo_file, &info);
      zoneinfo_ok = (sts == 0 && (info.st_mode & S_IFMT) == S_IFREG);
      if (sts == 0 && (info.st_mode & S_IFMT) == S_IFDIR)
      {
	char *tmp_msg = memAlloc(sizeof(zoneinfo_file)+128);
	sprintf(tmp_msg, "%s is a directory", zoneinfo_file);
	TZ_WARN(tmp_msg, cp+strlen(cp)-1);
	memFree(tmp_msg);
      }
    }
    errno = save_errno;

# ifdef OSF1
    /* OSF1 *requires* that all zoneinfo references be prefaced by ':'.  All
       other OSes providing zoneinfo treat ':' as optional. */
    if (zoneinfo_ok && *TZ != ':')
    {
      static char *new_TZ = NULL;
      if (new_TZ != NULL) memFree(new_TZ);
      new_TZ = memAlloc(strlen(TZ) + 2);
      sprintf(new_TZ, ":%s", TZ);
      valid_TZ = new_TZ;
    }
# endif /*OSF1*/
  }
#endif /*ZONEINFO*/

  /* According to HPUX 10 man page, TZ can be set using the format:
     
     [:]STDoffset[DST[offset][,rule]]
     
     STD and DST: Three or more bytes that designate the standard time
     zone (STD) and summer (or daylight-savings) time zone (DST) STD
     is required.  If DST is not specified, summer time does not apply
     in this locale.  Any characters other than digits, comma (,),
     minus (-), plus (+), or ASCII NUL are allowed.
     
     Offset is of the form: hh[:mm[:ss]] Hour (hh) is any value from 0
     through 23.  The optional minutes (mm) and seconds (ss) fields
     are a value from 0 through 59.  The hour field is required.  If
     offset is preceded by a -, the time zone is east of the Prime
     Meridian.  A + preceding offset indicates that the time zone is
     west of the Prime Meridian.  The default case is west of the
     Prime Meridian.
     
     The rule has the form: date/time,date/time */
     
  if (!zoneinfo_ok)
  {
    char *start;
#if defined(SUNOS5)
    /* SunOS5 seems to be the only OS that allows less than 3 characters: */
#   define MIN_TZNAME_LEN 1
#else
#   define MIN_TZNAME_LEN 3
#endif

#if defined(SUNOS5)
    /* SunOS5 seems to be the only OS that CAN'T tolerate any spaces which
       is odd considering "TZ=Poland" generates "MET DST" for tzname[1]: */
    if ((cp = strFindFirstOccurrence(TZ, " \t")) != NULL)
      TZ_WARN("Invalid character", cp);
#endif
    cp = TZ;
    if (*TZ == ':') cp++;

    /* STDoffset required: */
    start = cp;
    while (!isDigit(*cp) && IS_NOT_MEMBER(*cp, "+-,") && *cp != CNULL) cp++;
    if (cp - start == 0)
      TZ_WARN("Missing STD name", cp);
    if (cp - start < MIN_TZNAME_LEN)
      TZ_WARN("STD has too few characters", cp-1);
    if (zoneinfo_file != NULL && *cp == CNULL)
    {
      char *tmp_msg = memAlloc(sizeof(zoneinfo_file)+128);
      sprintf(tmp_msg, "%s not found", zoneinfo_file);
      TZ_WARN(tmp_msg, cp-1);
      memFree(tmp_msg);
    }
    if (*cp == '+' || *cp == '-') cp++;
    if (!isDigit(*cp))
      TZ_WARN("Invalid or missing STD offset", cp-1);
    while (isDigit(*cp) || *cp == ':') cp++;

    /* DST optional: */
    if (*cp != CNULL)
    {
      start = cp;
      while (!isDigit(*cp) && IS_NOT_MEMBER(*cp, "+-,") && *cp != CNULL) cp++;
      if (cp - start == 0)
	TZ_WARN("Missing DST name", cp);
      if (cp - start < MIN_TZNAME_LEN)
	TZ_WARN("DST has too few characters", cp-1);
      
      /* DST's offset optional: */
      if (*cp != CNULL && *cp != ',')
      {
	if (*cp == '+' || *cp == '-') cp++;
	if (!isDigit(*cp))
	  TZ_WARN("Invalid or missing DST offset", cp-1);
	while (isDigit(*cp) || *cp == ':') cp++;
      }
      
      /* DST's ,rule optional: */
      if (*cp != CNULL)
      {
	/* Validate that rule starts with a ',' but for now don't bother
	   checking the validity of the rule itself: */
	if (*cp != ',')
	  TZ_WARN("Invalid or missing DST rule", cp);
      }
    }
    /* Some OSes tolerate ":" prefaced to STDoffset... while other OSes
       don't perform correctly.  The best solution is to always remove
       the ":" preface from :STDoffset... for all OSes: */
    if (*TZ == ':')
      valid_TZ = TZ+1;
  }

  if (zoneinfo_tmp != NULL) memFree(zoneinfo_tmp);

  return(valid_TZ);
}
