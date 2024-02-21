/******************************************************************************
  edate.c -- An enhanced date
  
  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: August 10, 2012
  
  This program is provided "as is" in the hopes that it might serve some
  higher purpose.  If you want this program to serve some lower purpose,
  then that too is all right.  So as to keep these hopes alive you may
  freely distribute this program so long as this header remains intact.
  You may also freely distribute modified versions of this program so long
  as you indicate that such versions are modified and so long as you
  provide access to the unmodified original copy.
  
  Note: The most recent version of this program may be obtained from
        http://els-software.org

  The following routines support Unix time beyond Jan 19 03:14:08 2038 GMT,
  assuming that the future standard will treat the 32-bits used by Unix's
  present-day file system as unsigned.

  ****************************************************************************/

#include "version.h"
#include "sysdefs.h"

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#ifdef HAVE_LOCALE
#include <locale.h>
#endif

#include "defs.h"
#include "getdate32.h"
#include "time32.h"
#include "auxil.h"
#include "format.h"

/* Maximum amount of info per line: */
#define  MAX_INFO  128

/* Maximum amount of output: */
#define  MAX_OUTPUT  (2*MAX_INFO)

/********** Global Routines Referenced **********/

// extern char *getenv();   // defined in stdlib.h

/********** Global Routines Defined **********/

void do_getenv(void);
void do_options(char *options);
void do_options_minus(char *options);
void do_options_plus(char *options);
void do_options_minus_minus(char *options);
void give_usage_or_help(char U_or_H);
void edate(void);
char *T_print(char *buff, char *fmt, time_t ftime,
	      struct tm *fdate, Boole gmt, Boole meridian);
char *T_print_width(char *buff, char *fmt, time_t ftime,
		    struct tm *fdate, Boole gmt, Boole meridian, int width);
char *separate(char *bp, char icase);
char *finish(char *bp);

/********** Global Variables Defined **********/

int  Iarg;
int  Argc;
char **Argv;
char *Progname;
time_t The_Time;
char *Time_Zone = NULL;
char *Current_Arg = NULL;
char LGC;
time_t time_arg;
Boole have_time_arg;
Boole have_output_arg;
Boole have_settime_arg;
Boole useLcTime;
int verboseLevel;
Boole warning_suppress;

Boole T_option;
char *T_format;
#define Tf_ABS_TIME		'a'
#define Tf_DELTA_TIME		'd'
#define Tf_REL_TIME		'r'
#define Tf_YOFFSET_TIME		'y'
#define Tf_ALPHA_DATE		'A'
#define Tf_NUM_DATE		'N'
#define Tf_GMT_DATE		'G'
#define Tf_LOCAL_DATE		'L'
#define Tf_MERIDIAN_TIME	'M'
#define Tf_HEX_OUTPUT		'x'

#define Tf_FLOATING_POINT_DATE	'F'	/* YYYYMMDD.hhmmss */
#define Tf_ISO8601_DATE		'I'	/* YYYYMMDD.hhmmss */
#define Tf_FLOATING_POINT_DAY	'f'	/* YYYYMMDD */
#define Tf_ISO8601_DAY		'i'	/* YYYYMMDD */
#define Tf_ELS_DATE		'e'
#define Tf_LS_DATE		'l'
#define Tf_DOS_DATE		'd'
#define Tf_WINDOWS_DATE		'w'
#define Tf_VERBOSE_DATE		'v'
#define Tf_ELAPSED_TIME		'E'	/* D+h:m:s */

#define Tf_YEARS		'Y'
#define Tf_MONTHS		'M'
#define Tf_WEEKS		'W'
#define Tf_DAYS			'D'
#define Tf_HOURS		'h'
#define Tf_MINS			'm'
#define Tf_SECS			's'
#define Tf_CLOCK		'c'
#define Tf_TIME_2		't'	/* h:m */
#define Tf_TIME_3		'T'	/* h:m:s */
#define Tf_TIME_OR_YEAR		'Q'
#define Tf_YEARS_MOD_100	'y'
#define Tf_ZONE_NAME		'Z'

int squeeze_cnt = 0;
Boole squeeze = FALSE;
Boole fielding = FALSE;
Boole as_is;
Boole separated;
char separator = ' ';

/* FIXME: use include file to define common definitions: */
#define EXEC_ERROR      3
#define GENERAL_ERROR   1
#define USAGE_ERROR     1
#define NORMAL_EXIT     0
#define fexpr_FNOT	'~'
#define fexpr_NOT	'!'

/* Static messages: */
char *NON_NEGATABLE = "Non-negatable option";

/*****************************************************************************/

int main(int argc, char *argv[])
{
  Iarg = 1;
  Argc = argc;
  Argv = argv;

  /* Simplify access to the program name */
  {
    char *cp;
    if ((cp = strrchr(Argv[0], '/')) != NULL)
      Progname = cp + 1;
    else
      Progname = Argv[0];
  }

  /* Read any evironment settings relevent to ELS: */
  do_getenv();

#ifdef Wformat
  fprintf(stderr, "%s: Warning: compiled with -DWformat (zero_pad disabled)\n",
	  Progname);
#endif

  /* Establish initial defaults: */
  LGC = ' ';	/* Grant LGC latitude to choose 'L' or 'G' */
  have_time_arg = FALSE;
  have_output_arg = FALSE;
  have_settime_arg = FALSE;
#ifdef HAVE_LOCALE
  useLcTime = TRUE;
#else
  useLcTime = FALSE;
#endif
  verboseLevel = 0;
  warning_suppress = FALSE;

  /* Output defaults: */
  T_option = FALSE;
  T_format = "v";
  squeeze_cnt = 0;
  squeeze = FALSE;
  as_is = FALSE;
  separated = FALSE;

  while (Iarg < Argc)
  {
    while (Iarg < Argc && IS_MEMBER(*(Argv[Iarg]), "+-"))
    {
      if ((Argv[Iarg][0]) == '-' && isDigit(Argv[Iarg][1])) break;
      do_options(Argv[Iarg++]);
    }
    
    if (Iarg < Argc)
    {
      if (!have_time_arg)
      {
#	define MAX_DATE_ARG 127
#	define SPACE_LEFT (i < MAX_DATE_ARG ? MAX_DATE_ARG - i : 0)
	char date_arg[MAX_DATE_ARG+1];
	int i = 0;
	
	do {
	  char *options = Argv[Iarg++];
	  if (i > 0)
	  {
	    strncpy(&date_arg[i], " ", SPACE_LEFT);
	    i++;
	  }
	  strncpy(&date_arg[i], options, SPACE_LEFT);
	  i += strlen(options);
	} while (Iarg < Argc && IS_NOT_MEMBER(*(Argv[Iarg]), "+-"));

	date_arg[MAX_DATE_ARG] = CNULL;  /* JIC */
	if (SPACE_LEFT == 0)
	  arg_error_msg("Date too long", date_arg, &date_arg[MAX_DATE_ARG]);
	time_arg = getdate32(date_arg, NULL);

	LGC = ' '; /* After each use, LGC reverts back to default */
	have_time_arg = TRUE;
      }
      else
	usage_error();
    }
  }
    
#ifdef HAVE_LOCALE
  /* LC_ALL environment variable supersedes both LC_COLLATE and LC_TIME for
     both SunOS5 and Linux2.  But ELS also allows ELS_LC_COLLATE to supersede
     LC_ALL/LC_COLLATE and ELS_LC_TIME to supersede LC_ALL/LC_TIME. */

  if (useLcTime)
  {
    char *lc;
    if ((lc = getenv("ELS_LC_TIME")) != NULL)
    {
      /* ELS_LC_TIME, if set, overrides all other system/envvar locale
	 settings: */
      lc = setlocale(LC_TIME, lc);
      envUnset("LC_TIME");  /* Prevent +c reporting overridden envvar */
    }
    else
    {
      /* Use setlocale(..., "") to capture current setting: */
      lc = setlocale(LC_TIME, "");
      if (lc != NULL) envSetNameVal("ELS_LC_TIME", lc); /* For +c reporting */
    }
  }
#endif

  if (Iarg == Argc)
    edate();
  else
    usage_error();

  return(NORMAL_EXIT);
}


void do_getenv(void)
{
#ifdef HAVE_LOCALE
  /* Freshen locale env settings from lowest to highest precedence: */
  {
    char *lc;
    if ((lc = getenv("LANG")) != NULL)
      lc = setlocale(LC_ALL, lc);
    if ((lc = getenv("LC_TIME")) != NULL)
      lc = setlocale(LC_TIME, lc);
    if ((lc = getenv("LC_ALL")) != NULL)
      lc = setlocale(LC_ALL, lc);
  }
#endif

  /* Get the the current time only at the start of program: */
#if defined(CYGWIN)
  tzset(); /* Recommended by the cygwin faq file */
#endif
  The_Time = time(NULL);

  return;
}


void do_options(char *options)
{
  static int level = 0;
  if (level++ == 0) Current_Arg = options;

  if (*options == '-')
    do_options_minus(&options[1]);
  else if (*options == '+')
    do_options_plus(&options[1]);

  if (--level == 0) Current_Arg = NULL;
}


#define OPTION(val)  (negate ? !(val) : (val))
void do_options_minus(char *options)
{
  char opt;
  while ((opt = *options++) != CNULL)
  {
    Boole negate = (opt == fexpr_FNOT || opt == fexpr_NOT);
    if (negate) opt = *options++;
    switch (opt)
    {
    case '-': /* Process extended -- option */
      do_options_minus_minus(options);
      options = "";	/* No more options */
      break;

    case 'L':
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      LGC = 'L';
      set_currentTZ(get_defaultTZ());
      break;

    case 'G':
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      LGC = 'G';
      break;

    case 'C':
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      LGC = 'C';
      break;

    case 'o':
      have_output_arg = OPTION(TRUE);
      break;

    case 's':
      have_settime_arg = OPTION(TRUE);
      break;

    default: /* Give error message and usage, then exit: */
      opt_error_msg("Unrecognized option", options);
      break;
    }
  }
  
  return;
}


void do_options_plus(char *options)
{
  char opt;
  while ((opt = *options++) != CNULL)
  {
    Boole negate = (opt == fexpr_FNOT || opt == fexpr_NOT);
    if (negate) opt = *options++;
    switch (opt)
    {
    case 'f': /* Specify field separtor */
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      fielding = TRUE;
      separator = *options++;
      break;

    case 'T': /* Specify the time & date format */
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      T_option = TRUE;	/* Command-line args take precedence */
      T_format = options;
      options = "";	/* No more options */
      have_output_arg = TRUE; /* Having a format implies output wanted */
      break;
      
    case 'v': /* Give version and bits of information: */
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      puts(VersionID);
      exit(USAGE_ERROR);
      break;

    case 'V': /* Verbose level */
      verboseLevel++;	/* Go to next verbose level */
      if (negate) verboseLevel = 0;
      break;

    case 'w': /* Warning suppression */
      warning_suppress = OPTION(TRUE);
      break;

    case 'Z': /* Set zonename */
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      if (*options == '=')
      {
	options++;
	Time_Zone = get_validTZ(options);
	set_currentTZ(Time_Zone);
	options = "";	/* No more options */
      }
      else
      {
	opt_error_msg("Missing =ZONENAME", options);
      }
      break;

    case 'H': /* Give help and exit: */
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      give_usage_or_help('H');
      exit(USAGE_ERROR);
      break;

    default: /* Give error message and usage, then exit: */
      opt_error_msg("Unrecognized option", options);
      break;
    }
  }
  
  return;
}


void do_options_minus_minus(char *options)
{
  Boole negate = (*options == fexpr_FNOT || *options == fexpr_NOT);
  if (negate) options++;

  if (strlen(options) > 0 &&
      strncmp(options, "help", strlen(options)) == 0)
  {
    if (negate) opt_error_msg(NON_NEGATABLE, options);
    give_usage_or_help('h');
    exit(USAGE_ERROR);
  }

  else if (strcmp_ci(options, "version") == 0)
  {
    do_options("+v");
  }

  else if (strncmp_ci(options, "setenv:", strlen("setenv:")) == 0)
  {
    char *env = strchr(options, ':') + 1;
    envSet(env);
    /* Re-read any evironment settings relevent to ELS: */
    do_getenv();
  }

  else if (strncmp_ci(options, "unsetenv:", strlen("unsetenv:")) == 0)
  {
    char *env = strchr(options, ':') + 1;
    envUnset(env);
    /* Re-read any evironment settings relevent to ELS: */
    do_getenv();
  }

  else
  {
    /* Give error message and usage, then exit: */
    opt_error_msg("Unrecognized '--' option", options+1);
  }

  return;
}


void give_usage_or_help(char U_or_H)
{
  /* U: Print usage to stderr so as to avoid any redirection.
     H: Print help to stdout so as to make it pipe-able through PAGER.
     h: Print help to stdout but without PAGER pipe. */
  FILE *out;    // , *fopen(), *popen();
  Boole more = FALSE;

  if (U_or_H == 'H')
  {
    char *places[] = {"/usr/ucb/more", "/usr/bin/more", "/bin/more", NULL};
    char *pager, **path;
    pager = getenv("PAGER");
    if (pager == NULL)
    {
      for (path = places; *path && fopen(*path, "r") == NULL; path++) /*...*/;
      pager = *path;
    }
    if (pager != NULL && (out = popen(pager, "w")) != NULL)
      more = TRUE;
    else
      out = stdout;
  }
  else if (U_or_H == 'h')
    out = stdout;
  else
    out = stderr;

  fprintf(out, "\
\n\
Usage: Display current time:\n\
       %s [-LGC] [+Z=zonename] [+f char] [+T format]\n\
\n\
       Convert from one time format to another:\n\
       %s [-LG] [+Z=zonename] supported_date \\\n\
		-o [-LGC] [+Z=zonename] [+f char] [+T format]\n\
       %s -C unix_clock \\\n\
		-o [-LGC] [+Z=zonename] [+f char] [+T format]\n\
\n\
       Set system time (must be super-user):\n\
       %s [-LG] [+Z=zonename] supported_date -s\n\
       %s -C unix_clock -s\n\
\n\
       A supported_date is any one of: Floating-point style; LS style;\n\
       DD-MON-YYYY, YYYY-MON-DD, or YYYY-MM-DD styles;\n\
       DOS style or Windows style as described below.\n\
\n\
       If a unix_clock is given then it must be prefaced by -C and\n\
       consist of an integer or hexadecimal value.\n\
\n\
", Progname, Progname, Progname, Progname, Progname);

  if (U_or_H == 'U')
  {
    fprintf(out, "\
For more help: %s +H, or %s --help\n\
\n\
", Progname, Progname);
  }
  else
  {
    fprintf(out, "\
Where:\n\
    -L: Local time (default)\n\
    -G: GMT\n\
    -C: 32-bit clock\n\
    -o: Output converted value rather than setting time\n\
    -s: Set time rather than output converted value (must be super-user)\n\
    +H: Give HELP piped through PAGER\n\
\n\
    +Z: Specify timezone to be used in place of current TZ setting.\n\
	The timezone should be in the form of:\n\
	    All OSes:		STDoffset[DST[offset][,rule]]\n\
	Additionally, if your host provides zoneinfo then you can also\n\
	use names from the appropriate zoneinfo directory:\n\
	    SunOS/Solaris:		/usr/share/lib/zoneinfo\n\
	    Linux2, Darwin, FreeBSD:	/usr/share/zoneinfo\n\
	    OSF1:			/etc/zoneinfo\n\
	For example:\n\
	    edate +Z=EST5EDT +TIZ	(Available on *all* OSes)\n\
	    edate +Z=US/Eastern		(Available on OSes with zoneinfo)\n\
\n\
    +f: Field separator character, e.g.:  edate +f# +Tv\n\
\n\
    +T: TIME format, e.g.:  edate Jan 1 +T^rD (number of days since Jan 1)\n\
       ^a:  Modifier for absolute time since the epoch\n\
       ^d:  Modifier for delta time from now (i.e. difference)\n\
       ^r:  Modifier for relative time from now (i.e. age)\n\
       ^y:  Modifier for relative time since start of year\n\
       ^A:  Modifier for alpha dates instead of numeric\n\
       ^N:  Modifier for numeric dates instead of alpha\n\
       ^G:  Modifier for GMT dates instead of local\n\
       ^L:  Modifier for local dates instead of GMT\n\
       ^M:  Modifier for meridian instead of military time\n\
	F:  Floating-point style (same as +T^N~YMD.hms~)\n\
	I:  Iso8601 style\n\
	e:  els style (default, same as +TM%%_DYt)\n\
	l:  ls style (same as +TM%%_DQ)\n\
	d:  dos style (same as +T^N%%_M-D-y^M%%_h:~mp~)\n\
	w:  windows style (same as +T\"^N%%_M/D/y^M%%_tP'M'\")\n\
	v:  verbose style (same as +TWM%%_DTZY)\n\
  Y,M,D,W:  year(Y), month(M), day(D), weekday(W)\n\
  h,m,s,c:  hour(h), minutes(m), seconds(s), clock(c)\n\
      t,T:  time as h:m(t), h:m:s(T)\n\
	Q:  time or year depending on age\n\
	p:  'a' or 'p' depending on meridian\n\
	    (meaningful only with ^M modifier)\n\
	P:  'A' or 'P' depending on meridian\n\
	    (meaningful only with ^M modifier)\n\
	y:  year modulo 100\n\
	Z:  zone name\n\
\n\
    +T format controls\n\
	\\:  Output following character verbatim\n\
	~:  Toggle spacing off/on\n\
       %%%%:  Output a single %% character\n\
       %%D:  Output directive 'D' using default width and default padding\n\
      %%_D:  Pad left side of output with blanks using default width\n\
      %%0D:  Pad left side of output with zeros using default width\n\
      %%-D:  Suppress all padding and use minimum width\n\
     %%0nD:  Output a zero padded field 'n' characters wide\n\
     %%_nD:  Output a blank padded field 'n' characters wide\n\
      %%nD:  Output a right justified field 'n' characters wide\n\
     %%-nD:  Output a left justified field 'n' characters wide\n\
     %%+nD:  Output a field 'n' characters wide regardless of ~ spacing\n\
\n\
	If a string occurs within an inner set of quotes then the string\n\
	is output verbatim (except for any directives within the inner\n\
	quotes prefaced by a %%).  Thus, the following are equivalent:\n\
\n\
	+T\"'DATE: '^N%%M/D/Y\"    # M/D/Y in outer quotes (%% optional)\n\
	+T'\"DATE: \"^NM/%%D/Y'    # M/D/Y in outer quotes (%% optional)\n\
	+T'\"DATE: ^N%%M/%%D/%%Y\"'  # M/D/Y in INNER quotes (%% REQUIRED)\n\
\n\
    Floating-point style :== YYYYMMDD[.hhmm[ss]]\n\
        The date must be all digits (except for the optional period\n\
        preceding the hour hh).  The year YYYY must be not less than 1970\n\
        and not more than 2106.  The month MM is specified as 01 through\n\
        12, and the day DD is specified as 01 through 31.  The time, if\n\
        given, is specified as either 0000 through 2359 or 000000 through\n\
        235959.  Any unspecified hh, mm, ss parameters will default to 00.\n\
\n\
    LS style          :== MON DD [YYYY] [[hh:mm[:ss] [AM|PM]]\n\
    DD-MON-YYYY style :== DD MON [YYYY] [[hh:mm[:ss] [AM|PM]]\n\
                      :== DD-MON[-YYYY] [[hh:mm[:ss] [AM|PM]]\n\
        MON is the three-letter Posix C locale abbreviation for the\n\
        month.  The day DD is specified as 1 through 31.  The year YYYY,\n\
        if given, must be not less than 1970 and not more than 2106.  If\n\
        the year is omitted then the most recent year gone by will be\n\
        assumed.  The time, if given, is specified as either 00:00 through\n\
        23:59 or 00:00:00 through 23:59:59.  Any unspecified hh, mm, ss\n\
        parameters will default to 0.  If AM/PM is unspecified then\n\
        military time is assumed.\n\
\n\
        If both the year and the time are specified, it doesn't matter\n\
        which is first.  Thus, a date such as 'Jan 31 12:34 2001' is\n\
        equivalent to 'Jan 31 2001 12:34'.  Furthermore, weekdays (e.g.\n\
        Mon, Tue, Wed, etc.) and timezones (e.g. EST, CDT, MST, etc.)\n\
        are ignored.  Thus, a date such as 'Wed Jan 31 12:34:56 EST 2001'\n\
        is equivalent to 'Jan 31 2001 12:34:56'.\n\
\n\
    YYYY-MON-DD style :== YYYY MON DD [[hh:mm[:ss] [AM|PM]]\n\
                      :== YYYY-MON-DD [[hh:mm[:ss] [AM|PM]]\n\
    YYYY-MM-DD style  :== YYYY MM DD [[hh:mm[:ss] [AM|PM]]\n\
                      :== YYYY-MM-DD [[hh:mm[:ss] [AM|PM]]\n\
        The year YYYY must be not less than 1970 and not more than 2106.\n\
        MON is the three-letter Posix C locale abbreviation for the month,\n\
        or a numeric month MM can be specified as 01 through 12. The day\n\
        DD is specified as 1 through 31.  The time, if given, is specified\n\
        as either 00:00 through 23:59 or 00:00:00 through 23:59:59.\n\
        Any unspecified hh, mm, ss parameters will default to 0.\n\
        If AM/PM is unspecified then military time is assumed.\n\
\n\
    DOS style :== MM-DD-[YY]YY [hh:mm[:ss][a|p]]\n\
    Windows style :== MM/DD/[YY]YY [hh:mm[:ss] [AM|PM]]\n\
        MM, DD, and hh can be either one or two digits in length and\n\
        the first digit can optionally be zero.  The year can either be\n\
        two digits (YY) or four digits (YYYY).  If a two digit year YY is\n\
        specified, then YY from 70 through 99 will be taken to mean 1900+YY,\n\
        and YY from 00 through 68 will be taken to mean 2000+YY.  Years\n\
        beyond 2068 must specify all four digits.  If any combination\n\
        of 'a', 'p', 'am', 'P.m', 'A', 'P', 'AM', 'P.M.', etc. follows\n\
        the time, then the time will be converted to either A.M. or P.M.\n\
        If a/p or AM/PM is unspecified then military time is assumed.\n\
\n\
    unix_clock :== unsigned 32 bit decimal, octal, or hexadecimal value\n\
        in the range 0 through 4294967294 or 0 through 0xfffffffe (note\n\
        that -1 is used to represent certain error conditions).  Unix\n\
        clock values can be listed using 'els -l +T^as'\n\
\n\
");

    fprintf(out, "\
\n\
SPECIAL FEATURES:\n\
\n\
    --version\n\
	Print els version and information (same as +v)\n\
    --setenv:VARIABLE=VALUE\n\
	Create and set named environment variable to given value\n\
    --unsetenv:VARIABLE\n\
	Unset and delete named environment variable\n\
\n\
");
  
    fprintf(out, "\
\n\
ENVIRONMEMT:\n\
\n\
    PAGER: Name of pager program for displaying help text\n\
\n\
    ELS_LC_TIME, LC_ALL, LC_TIME:\n\
	The Posix LC_ALL environment variable supersedes LC_TIME\n\
	(e.g. SunOS5/Solaris and Linux behavior).  But ELS_LC_TIME\n\
	further supersede this behavior as follows:\n\
\n\
	Time locale determined as follows:\n\
	   Use ELS_LC_TIME if defined, else use LC_ALL if defined,\n\
	   else use LC_TIME if defined, else use 'C' locale.\n\
\n\
");

    fprintf(out, "\
Examples:\n\
    Display the current time in Iso8601 format:\n\
	%s +TI\n\
\n\
    Display the current time in Japan:\n\
	%s +Z=Japan      (SunOS/Solaris, Linux, OSF1 only)\n\
	%s +Z=Asia/Tokyo (FreeBSD only)\n\
	%s +Z=JST-9      (All OSes)\n\
\n\
    Display the current time using specified locale:\n\
	%s +Tv --setenv:LC_TIME=fr_FR.ISO8859-1\n\
\n\
    Convert the date Feb 29, 1996 12:34:56 GMT to a Unix clock value:\n\
	%s -G 19960229.123456 -o -C\n\
\n\
    Convert the DOS date 2-29-00 1:00p from Japan time to Local time:\n\
	%s +Z=Japan 2-29-00 1:00p -o -L +Td\n\
	%s +Z=JST-9 2-29-00 1:00p -o -L +Td\n\
\n\
    Convert the date Feb 29, 2096 12:34:56 from Japan time to US/Eastern:\n\
	%s +Z=Japan Feb 29, 2096 12:34:56 -o +Z=US/Eastern\n\
	%s +Z=JST-9 Feb 29, 2096 12:34:56 -o +Z=EST5EDT\n\
\n\
    Convert the Unix clock value of 0x87654321 to Local time and specify\n\
    the output format:\n\
	%s -C 0x87654321 +T'%%M %%D, %%Y %%h:%%m:%%s'\n\
\n\
    Set the system date to April 1, 2000 12:34 Local time:\n\
	%s -s 20000401.1234\n\
\n\
    Set the system date to April 1, 2000 12:34:56 Local time using\n\
    ls-style dates:\n\
	%s -s Apr 1 2000 12:34:56\n\
\n\
    Set the system date to January 1, 2050 12:34:56 GMT:\n\
	%s -sG 20500101.123456\n\
\n\
IMPORTANT NOTE:\n\
    The program 'edate' can be used to correctly set and display\n\
    dates beyond Jan 19, 2038 03:14:08 GMT.  The companion program 'els'\n\
    can also be used to correctly list any files created beyond this\n\
    date.  Please keep in mind that setting your clock beyond 2038 might\n\
    cause unexpected results and/or undesirable consequences.  Simply\n\
    converting and displaying dates beyond 2038, however, poses no risk.\n\
\n\
",	    Progname, Progname, Progname, Progname, Progname,
	    Progname, Progname, Progname, Progname, Progname,
	    Progname, Progname, Progname, Progname);
  }

  if (more)
    pclose(out);
  
  return;
}


void edate(void)
{
  /* Define OLDSTYLE_OUTPUT if you want to enable the mode prior to 1.47
     where edate would attempt to set the date whenever a date was given and
     -o and/or +T were absent.  This mode was somewhat dangerous as it would
     sometimes surprise the user by setting the date when not intended. */
/*#define OLDSTYLE_OUTPUT*/
#ifdef OLDSTYLE_OUTPUT
  have_settime_arg = (have_time_arg && !have_output_arg);
#endif

  if (have_settime_arg)
  {
    if (!have_time_arg)
      errorMsgExit("Failed to set date: No time given", USAGE_ERROR);

#ifdef HAVE_STIME
    {
      long t;
      t = time_arg;
      if (stime(&t) != 0)
	errnoMsgExit(-1, "Failed to set date", GENERAL_ERROR);
    }
#else
    {
      struct timeval t;
      t.tv_sec = time_arg;
      t.tv_usec = 0;
      if (settimeofday(&t, NULL) != 0)
	errnoMsgExit(-1, "Failed to set date", GENERAL_ERROR);
    }
#endif
  }
  else
  {
    char buff[MAX_OUTPUT];
    time_t the_time;

    if (have_time_arg)
      the_time = time_arg;
    else
      the_time = time(NULL);

    if (LGC == 'L' || LGC == ' ')
      Void T_print(buff, T_format, the_time, localtime32(&the_time), FALSE, FALSE);
    else if (LGC == 'G')
      Void T_print(buff, T_format, the_time, gmtime32(&the_time), TRUE, FALSE);
    else if (LGC == 'C')
    {
      /* Use this if no explicit T_format given on the command-line: */
      if (! T_option) T_format = "~^as~";

      Void T_print(buff, T_format, the_time, NULL, FALSE, FALSE);
    }

    fputs(buff, stdout);
    fputs("\n", stdout);
  }

  return;
}


#define ZERO_PAD_DEFAULT  (!tics)
/*#define ZERO_PAD_DEFAULT  (!tics || !yoffset)	-- FIXME: I'm undecided */
char *T_print(char *buff, char *fmt, time_t ftime,
	      struct tm *fdate, Boole gmt, Boole meridian)
{
  register char *bp;
  char icase;
  char delimiter = CNULL;
  Boole quote_mode = FALSE;
  Boole percent_specified = FALSE;
  Boole zero_pad;
  Boole hard_width = FALSE;
  Boole width_specified = FALSE;
  int width = 0;
  Boole directive_seen = FALSE;
  Boole modifier = FALSE;
  Boole delta_time = FALSE;
  Boole future = FALSE;
  Boole tics = FALSE;
  Boole numeric = FALSE;
  Boole yoffset = FALSE;
  Boole hex_output = FALSE;
  Boole ttoggle = FALSE;
  time_t abs_ftime = ftime;	/* Avoid possible side-effects */
  static char *dow[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  static char *moy[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  char legacy_modifier = CNULL;
# define BOOLE_XOR(a,b) ((a) != (b))

  bp = buff;
  *bp = CNULL;

  zero_pad = ZERO_PAD_DEFAULT;
  separated = FALSE;

  while ((icase = *fmt++) != CNULL)
  {
    if (modifier)
    {
      /* Make legacy_modifier substitution if needed: */
      if (legacy_modifier != CNULL)
      {
	icase = legacy_modifier;
	legacy_modifier = CNULL;
      }
      /* Modifiers generate no output: */
      switch (icase)
      {
      case Tf_ABS_TIME:
	tics = TRUE;
	numeric = FALSE;
	yoffset = FALSE;
	ftime = abs_ftime;
	fdate = NULL; /* Recalculate fdate */
	break;

      case Tf_DELTA_TIME:
      case Tf_REL_TIME:
	tics = TRUE;
	numeric = FALSE;
	yoffset = FALSE;
	delta_time = (icase == Tf_DELTA_TIME);
	if ((Ulong)The_Time < (Ulong)abs_ftime)
	{
	  future = TRUE;
	  ftime = abs_ftime - The_Time;
	}
	else
	{
	  future = FALSE;
	  ftime = The_Time - abs_ftime;
	}
	fdate = NULL; /* Recalculate fdate */
	break;

      case Tf_YOFFSET_TIME:
	tics = FALSE;
	numeric = FALSE;
	yoffset = TRUE;
	break;

      case Tf_ALPHA_DATE:
	tics = FALSE;
	numeric = FALSE;
	yoffset = FALSE;
	break;

      case Tf_NUM_DATE:
	tics = FALSE;
	numeric = TRUE;
	yoffset = FALSE;
	break;

      case Tf_GMT_DATE:
	gmt = TRUE;
	fdate = NULL; /* Recalculate fdate */
	break;

      case Tf_LOCAL_DATE:
	gmt = FALSE;
	fdate = NULL; /* Recalculate fdate */
	break;

      case Tf_MERIDIAN_TIME:
	meridian = TRUE;
	break;

      case Tf_HEX_OUTPUT:
	hex_output = TRUE;
	break;

      default:
	carrot_msg(NULL, "+T", T_format, "Unrecognized +T modifier", fmt-1);
	exit(USAGE_ERROR);
	break;
      }
      zero_pad = ZERO_PAD_DEFAULT;
      modifier = FALSE; /* reset for next time */
    }
    else
    {
      switch (icase)
      {
      case '"':
      case '\'':
	if (!quote_mode)
	{
	  quote_mode = TRUE;
	  delimiter = icase;
	}
	else if (delimiter == icase)
	{
	  quote_mode = FALSE;
	}
	break;

      case '%':
	if (*fmt == '%')
	{
	  as_is = TRUE;
	  icase = *fmt++;
	  bp = separate(bp, icase);
	}
	else
	{
	  Boole minus = FALSE;
	  percent_specified = TRUE;
	  if (*fmt == '0') {zero_pad = TRUE; fmt++;}
	  if (*fmt == '_') {zero_pad = FALSE; fmt++;}
	  if (*fmt == '+') {hard_width = TRUE; fmt++;}
	  if (*fmt == '-') {minus = TRUE; fmt++;}
	  width_specified = (isDigit(*fmt) != 0);
	  if (width_specified)
	  {
	    width = (*fmt++ - '0');
	    while (isDigit(*fmt))
	      width = width * 10 + (*fmt++ - '0');
	    if (width < 0 || width > F_MAX_WIDTH)
	    {
	      /* NB: width < 0 implies overflow! */
	      carrot_msg(NULL, "+T", T_format, "Too big", fmt-1);
	      exit(USAGE_ERROR);
	    }
	    if (minus)
	    {
	      width = -width;
	      /* Make "%0*d",-9 behave like "%0-9d" for negative widths: */
	      zero_pad = FALSE;
	    }
	  }
	  else if (minus)
	  {
	    /* Make "%-d" mimic Linux's date +format: */
	    width = 0;
	    width_specified = TRUE;
	  }
	}
	break;

      case '^':
	if (*fmt == '^')
	{
	  as_is = TRUE;
	  icase = *fmt++;
	  bp = separate(bp, icase);
	}
	else
	{
	  modifier = TRUE;
	}
	break;

      case '~':
	ttoggle = !ttoggle;
	if (ttoggle)
	  squeeze_cnt++;
	else
	  squeeze_cnt--;
	squeeze = (squeeze_cnt != 0);
	if (!squeeze) bp = separate(bp, icase);
	break;

      case '\\':
	/* Print any character following the \quote as-is: */
	as_is = TRUE;
	if (*fmt != CNULL)
	{
	  icase = *fmt++;
	  if      (icase == 'n') icase = '\n';
	  else if (icase == 't') icase = '\t';
	  else if (icase == 'r') icase = '\r';
	  else if (icase == 'b') icase = '\b';
	  else if (icase == 'f') icase = '\f';
	  else if (icase == '0') icase = '\0';
	  bp = separate(bp, icase);
	}
	break;

      case 'A':			/* '+TA' is DEPRECATED, use '+T^a' instead */
      case 'R':			/* '+TR' is DEPRECATED, use '+T^r' instead */
      case Tf_NUM_DATE:		/* '+TN' is DEPRECATED, use '+T^N' instead */
      case Tf_GMT_DATE:		/* '+TG' is DEPRECATED, use '+T^G' instead */
      case Tf_LOCAL_DATE:	/* '+TL' is DEPRECATED, use '+T^L' instead */
	if (quote_mode)
	{
	  /* Don't confuse a quoted alpha character for a legacy modifier
	     (this bug exists in version 145 and earlier): */
	  as_is = TRUE;
	  bp = separate(bp, icase);
	  break;
	}
	else
	{
	  /* Handle legacy modifiers before "^" was introduced by treating
	     these as if they were prefaced by "^" by re-entering while-loop
	     with "modifier" flag set TRUE and with fmt reset to original
	     character: */
	  modifier = TRUE;
	  fmt--;
	  /* Special case: substitute +TA, +TR  with  +T^a, +T^r */
	  if (icase == 'A') legacy_modifier = 'a';
	  if (icase == 'R') legacy_modifier = 'r';
	  /* No other legacy modifiers need substitution. */
	}
	break;	/* modifiers generate no output */

      default:
	/* A character within quotes is a directive only when preceded by %: */
	if (quote_mode && !percent_specified)
	{
	  as_is = TRUE;
	  bp = separate(bp, icase);
	  break;
	}

	/* Remove extraneous spacing if appropriate: */
	if ((fielding || (squeeze && !hard_width)) &&
	    (!zero_pad || width != 0))
	{
	  if (icase == Tf_MINS || icase == Tf_SECS || icase == Tf_YEARS_MOD_100)
	    /* Tf_MINS, Tf_SECS, Tf_YEARS_MOD_100 should never have a width
	       less than 2 so as to allow zero_pad: */
	    width = 2;
	  else
	    width = 0;
	  width_specified = TRUE;
	}

	/* Determine file's date using local time (if needed): */
	if (fdate == NULL)
	{
	  if (gmt)
	    fdate = gmtime32(&ftime);
	  else
	    fdate = localtime32(&ftime);
	}

	/* Process a directive: */
	switch (icase)
	{
	case Tf_FLOATING_POINT_DATE:
	case Tf_ISO8601_DATE:
	  bp = T_print_width(bp, "^N~YMD.hms~", ftime, fdate,
			     gmt, meridian, width);
	  break;

	case Tf_FLOATING_POINT_DAY:
	case Tf_ISO8601_DAY:
	  bp = T_print_width(bp, "^N~YMD~", ftime, fdate,
			     gmt, meridian, width);
	  break;

	case Tf_ELS_DATE:
	  bp = T_print_width(bp, "M%_DYt", ftime, fdate, gmt, meridian, width);
	  break;

	case Tf_LS_DATE:
	  bp = T_print_width(bp, "M%_DQ", ftime, fdate, gmt, meridian, width);
	  break;

	case Tf_DOS_DATE:
	  bp = T_print_width(bp, "^N%_M-D-y^M%_h:~mp~",
			     ftime, fdate, gmt, meridian, width);
	  break;

	case Tf_WINDOWS_DATE:
	  bp = T_print_width(bp, "^N%_M/D/y^M%_tP'M'",
			     ftime, fdate, gmt, meridian, width);
	  break;

	case Tf_VERBOSE_DATE:
	  bp = T_print_width(bp, "WM%_DTZY",
			     ftime, fdate, gmt, meridian, width);
	  break;

	case Tf_ELAPSED_TIME:
	  /* Elapsed time only works for gmt==TRUE */
	  bp = T_print_width(bp, "~^aD+^Nh:m:s~",
			     ftime, fdate, TRUE, meridian, width);
	  break;

	case Tf_TIME_OR_YEAR:
#       define HALF_A_YEAR  (SECS_PER_YEAR/2)
	  if ((Ulong)The_Time - (Ulong)ftime < (Ulong)HALF_A_YEAR)
	    bp = T_print_width(bp, zero_pad ? "%0t" : "%_t",
			       ftime, fdate, gmt, meridian, width);
	  else
	  {
	    if (!width_specified && !(fielding || squeeze)) width = 5;
	    bp = T_print_width(bp, "Y", ftime, fdate, gmt, meridian, width);
	  }
	  break;

	case Tf_YEARS:
	case Tf_YEARS_MOD_100:
	  if (tics)
	  {
	    long r_years = ftime / SECS_PER_YEAR;
	    if (BOOLE_XOR(future, delta_time)) r_years = -r_years;
	    if (!width_specified) width = 2;
	    sprintf(bp, F_LD(zero_pad,width, r_years));
	  }
	  else
	  {
	    int year = fdate->tm_year;
	    if (icase == Tf_YEARS)
	    {
	      year += 1900;
	      if (!width_specified) width = 4;
	    }
	    else
	    {
	      year %= 100;
	      if (!width_specified) width = 2;
	    }
	    sprintf(bp, F_D(zero_pad,width, year));
	  }
	  break;

	case Tf_MONTHS:
	  if (tics)
	  {
	    long r_months = ftime / SECS_PER_MONTH;
	    if (BOOLE_XOR(future, delta_time)) r_months = -r_months;
	    if (!width_specified) width = 3;
	    sprintf(bp, F_LD(zero_pad,width, r_months));
	  }
	  else if (numeric || yoffset)
	  {
	    if (!width_specified) width = 2;
	    sprintf(bp, F_D(zero_pad,width, fdate->tm_mon + 1));
	  }
	  else
	  {
	    if (!width_specified) width = 3;
#	  ifdef HAVE_LOCALE
	    if (useLcTime)
	    {
	      char month[32];
	      strftime32(month, 32, "%b", fdate);
	      sprintf(bp, "%*s",width, month);
	    }
	    else
#	  endif
	      sprintf(bp, "%*s",width, moy[fdate->tm_mon]);
	  }
	  break;

	case Tf_WEEKS:
	  if (tics)
	  {
	    long r_weeks = ftime / SECS_PER_WEEK;
	    if (BOOLE_XOR(future, delta_time)) r_weeks = -r_weeks;
	    if (!width_specified) width = 4;
	    sprintf(bp, F_LD(zero_pad,width, r_weeks));
	  }
	  else if (numeric)
	  {
	    if (!width_specified) width = 2;
	    sprintf(bp, F_D(zero_pad,width, fdate->tm_wday + 1));
	  }
	  else if (yoffset)
	  {
	    char str[5];
	    int week_num;
	    strftime32(str, 4, "%W", fdate);
	    sscanf(str, "%d", &week_num);
	    if (!width_specified) width = 2;
	    sprintf(bp, F_D(zero_pad,width, week_num));
	  }
	  else
	  {
	    if (!width_specified) width = 3;
#	  ifdef HAVE_LOCALE
	    if (useLcTime)
	    {
	      char wday[32];
	      strftime32(wday, 32, "%a", fdate);
	      sprintf(bp, "%*s",width, wday);
	    }
	    else
#	  endif
	      sprintf(bp, "%*s",width, dow[fdate->tm_wday]);
	  }
	  break;

	case Tf_DAYS:
	  if (tics)
	  {
	    long r_days = ftime / SECS_PER_DAY;
	    if (BOOLE_XOR(future, delta_time)) r_days = -r_days;
	    if (!width_specified) width = 5;
	    sprintf(bp, F_LD(zero_pad,width, r_days));
	  }
	  else if (yoffset)
	  {
	    if (!width_specified) width = 3;
	    sprintf(bp, F_D(zero_pad,width, fdate->tm_yday + 1));
	  }
	  else
	  {
	    if (!width_specified) width = 2;
	    sprintf(bp, F_D(zero_pad,width, fdate->tm_mday));
	  }
	  break;

	case Tf_HOURS:
	  if (tics)
	  {
	    long r_hours = ftime / SECS_PER_HOUR;
	    if (BOOLE_XOR(future, delta_time)) r_hours = -r_hours;
	    if (!width_specified) width = 6;
	    sprintf(bp, F_LD(zero_pad,width, r_hours));
	  }
	  else if (yoffset)
	  {
#	    define YOFFSET_HOURS (fdate->tm_yday * 24 + fdate->tm_hour)
#	    define YOFFSET_MINS  (YOFFSET_HOURS * 60 + fdate->tm_min)
#	    define YOFFSET_SECS  (YOFFSET_MINS * 60 + fdate->tm_sec)
	    if (!width_specified) width = 4;
	    sprintf(bp, F_D(zero_pad,width, YOFFSET_HOURS));
	  }
	  else
	  {
	    int hour = fdate->tm_hour;
	    if (meridian)
	    {
	      if (hour > 12)
		hour -= 12;
	      else if (hour == 0)
		hour  = 12;
	    }
	    if (!width_specified) width = 2;
	    sprintf(bp, F_D(zero_pad,width, hour));
	  }
	  break;

	case 'p':
	case 'P':
	  if (! meridian)
	  {
	    carrot_msg(NULL, "+T", T_format,
		       "Must first specify meridian modifier", fmt-1);
	    exit(USAGE_ERROR);
	  }
	  else
	  {
	    char ap;
	    if (icase == 'p')
	      ap = (fdate->tm_hour < 12 ? 'a' : 'p');
	    else
	      ap = (fdate->tm_hour < 12 ? 'A' : 'P');
	    if (!width_specified) width = 1;
	    sprintf(bp, "%*c",width, ap);
	  }
	  break;

	case Tf_MINS:
	  if (tics)
	  {
	    long r_mins = ftime / SECS_PER_MIN;
	    if (BOOLE_XOR(future, delta_time)) r_mins = -r_mins;
	    if (!width_specified) width = 8;
	    sprintf(bp, F_LD(zero_pad,width, r_mins));
	  }
	  else if (yoffset)
	  {
	    if (!width_specified) width = 6;
	    sprintf(bp, F_D(zero_pad,width, YOFFSET_MINS));
	  }
	  else
	  {
	    if (!width_specified) width = 2;
	    sprintf(bp, F_D(zero_pad,width, fdate->tm_min));
	  }
	  break;

	case Tf_SECS:
	  if (tics)
	  {
	    char *rsp, r_secs[32];
	    rsp = r_secs;
	    /* This cruft avoids using "long long": */
	    if (BOOLE_XOR(future, delta_time)) *rsp++ = '-';
	    if (!width_specified) width = 10;
	    if (hex_output)
	      sprintf(rsp, FX_time_t(FALSE,0, ftime));
	    else
	    {
#	    if defined(HAVE_LONG_LONG_TIME)
	      /* Use default format as this uses signed 64-bits: */
	      sprintf(rsp, F_time_t(FALSE,0, ftime));
#	    else
	      /* Use "unsigned" format as this value may use all 32-bits: */
	      sprintf(rsp, FU_time_t(FALSE,0, ftime));
#	    endif
	    }
	    sprintf(bp, "%*s",width, r_secs);
	  }
	  else if (yoffset)
	  {
	    if (!width_specified) width = 8;
	    sprintf(bp, F_D(zero_pad,width, YOFFSET_SECS));
	  }
	  else
	  {
	    if (!width_specified) width = 2;
	    sprintf(bp, F_D(zero_pad,width, fdate->tm_sec));
	  }
	  break;

	case Tf_CLOCK:
	  {
	    char clk_fmt[32];
	    sprintf(clk_fmt, "%s^as", hex_output ? "^x" : "");
	    bp = T_print_width(bp, clk_fmt,
			       ftime, fdate, gmt, meridian, width);
	  }
	  break;

	case '2': /* '+T2' is DEPRECATED, use '+Tt' instead */
	case Tf_TIME_2:
	  bp = T_print_width(bp, zero_pad ? "%0h:m" : "%_h:m",
			     ftime, fdate, gmt, meridian, width);
	  break;

	case '3': /* '+T3' is DEPRECATED, use '+TT' instead */
	case Tf_TIME_3:
	  bp = T_print_width(bp, zero_pad ? "%0h:m:s" : "%_h:m:s",
			     ftime, fdate, gmt, meridian, width);
	  break;

	case Tf_ZONE_NAME:
	  if (!width_specified) width = 3;
#ifdef HAVE_TM_ZONE
	  /* tm_zone is right no matter what: */
	  sprintf(bp, "%*s",width, fdate->tm_zone);
#else
	  /* tzname is not always what we want: */
	  if (gmt)
	    sprintf(bp, "%*s",width, "GMT");
	  else
	  {
	    extern char *tzname[2];
	    sprintf(bp, "%*s",width, tzname[fdate->tm_isdst]);
	  }
#endif
	  break;

	default:
	  if (isAlnum(icase))
	  {
	    carrot_msg(NULL, "+T", T_format, "Unrecognized +T directive", fmt-1);
	    exit(USAGE_ERROR);
	  }
	  else
	    /* Output non-directive/non-alphanumeric as-is: */
	    as_is = TRUE;
	  break;
	}

	percent_specified = FALSE;
	zero_pad = ZERO_PAD_DEFAULT;
	hard_width = FALSE;
	width_specified = FALSE;
	width = 0;
	if (!as_is) directive_seen = TRUE;
	bp = separate(bp, icase);
	break;
      }
    }
  }
  bp = finish(bp);

  if (quote_mode) errorMsgExit("Missing delimiter in +T format", USAGE_ERROR);
  if (!directive_seen) errorMsgExit("Incomplete +T format", USAGE_ERROR);

  return(bp);
}
#undef  ZERO_PAD_DEFAULT


char *T_print_width(char *buff, char *fmt, time_t ftime,
		    struct tm *fdate, Boole gmt, Boole meridian, int width)
{
  if (width == 0)
    buff = T_print(buff, fmt, ftime, fdate, gmt, meridian);
  else
  {
    char tmp[MAX_INFO];
    Void T_print(tmp, fmt, ftime, fdate, gmt, meridian);
    sprintf(buff, "%*s",width, tmp);
    buff += strlen(buff);
  }
  return(buff);
}


char *separate(char *bp, char icase)
{
  bp += strlen(bp); /* Find where we are */
  if (as_is)
  {      
    /* Remove separation when it precedes an "as-is" character: */
    if (separated)
    {
      bp--;
      separated = FALSE;
    }
    
    /* Add the character as-is: */
    *bp++ = icase;  *bp = CNULL;
    as_is = FALSE;
  }
  else if (squeeze)
  {
    /* If squeezing then don't inject a separator: */
    separated = FALSE;
  }
  else
  {
    *bp++ = separator; *bp = CNULL;
    separated = TRUE;
  }

  return(bp);
}


char *finish(char *bp)
{
  bp += strlen(bp); /* Find where we are */
  /* Remove final separation: */
  if (separated)
  {
    bp--; *bp = CNULL;
    separated = FALSE;
  }

  return(bp);
}
