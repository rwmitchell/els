/******************************************************************************
  chdate.c -- Change date a la ISO 8601
  
  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: November 8, 2012
  
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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <utime.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef HAVE_LOCALE
#include <locale.h>
#endif

#include "defs.h"
#include "getdate32.h"
#include "auxil.h"
#include "sysInfo.h"

/********** Global Routines Referenced **********/

extern char *getenv();

/********** Global Routines Defined **********/

void do_getenv(void);
void do_options(char *options);
void do_options_minus(char *options);
void do_options_plus(char *options);
void do_options_minus_minus(char *options);
void give_usage_or_help(char U_or_H);
void chdate(char *path);

/********** Global Variables Defined **********/

int  Iarg;
int  Argc;
char **Argv;
char *Progname;
char *Time_Zone = NULL;
char *Current_Arg = NULL;
char LGC;
time_t atime_arg;
time_t mtime_arg;
Boole get_atime_arg;
Boole get_mtime_arg;
Boole change_atime;
Boole change_mtime;
Boole update_ctime;
Boole ignore_symlink;
Boole expand_symlink;
Boole files_follow;
Boole echo_event;
int verboseLevel;
Boole warning_suppress;

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

  /* Knowing at runtime which version of OS allows certain decisions
     (e.g. SunOS5 decides at runtime whether ACLs are supported, or
     whether to exec 32bit version if stat64 is not supported): */
  osVersion = get_osVersion();

#if SUNOS >= 50600 && defined(HAVE_STAT64)
  if (osVersion < 50600)
  {
    char *new_prog = memAlloc(strlen(Argv[0]) + 16);
    sprintf(new_prog, "%s-32bit", Argv[0]);
    if (getenv("ELS_32BIT") != NULL)
    {
      /* If env var set then don't try re-execing again and again and ...: */
      fprintf(stderr, "\
%s: This program compiled with HAVE_STAT64 defined, but is intended\n\
%s: for non-stat64 capable OSes.  Please read ELS technical guide.\n",
	      new_prog, new_prog);
      return(EXEC_ERROR);
    }
    else
    {
      /* Set an env var so that we can detect recursion should a version
	 of els compiled with HAVE_STAT64 be renamed to els-32bit: */
      putenv("ELS_32BIT=1");
      /* NB: execvp() does NOT replace Argv[0] with the value of new_prog! */
      execvp(new_prog, Argv);
      /* If execvp() is successful then the following is never reached: */
      /*errnoMsgExit(-1, new_prog, EXEC_ERROR);*/
      fprintf(stderr, "%s: Unable to exec %s\n", Progname, new_prog);
      return(EXEC_ERROR);
    }
  }
#endif

  /* Read any evironment settings relevent to ELS: */
  do_getenv();

  /* Establish initial defaults: */
  LGC = ' ';	/* Grant LGC latitude to choose 'L' or 'G' */
  change_atime = FALSE;
  change_mtime = FALSE;
  update_ctime = FALSE;
  /* ignore_symlink and expand_symlink are mutually exclusive: */
  ignore_symlink = TRUE; /* DEFAULT */
  expand_symlink = FALSE;
  files_follow = FALSE;
  echo_event = FALSE;
  verboseLevel = 0;
  warning_suppress = FALSE;

  do
  {
    /* Establish defaults for each round: */
    get_atime_arg = FALSE;
    get_mtime_arg = FALSE;
    
    while (Iarg < Argc && IS_MEMBER(*(Argv[Iarg]), "+-"))
    {
      if ((Argv[Iarg][0]) == '-' && isDigit(Argv[Iarg][1])) break;
      do_options(Argv[Iarg++]);
    }
    
    /* If -m, -a, or -c have not been specified, then assume -m: */
    if (Iarg < Argc-1 && !files_follow &&
	!get_atime_arg && !get_mtime_arg && !update_ctime)
      get_mtime_arg = TRUE;

    if (get_atime_arg || get_mtime_arg)
    {
      time_t time_arg;
      if (Iarg < Argc-1)
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
	} while (Iarg < Argc-1 && IS_NOT_MEMBER(*(Argv[Iarg]), "+-"));

	date_arg[MAX_DATE_ARG] = CNULL;  /* JIC */
	if (SPACE_LEFT == 0)
	  arg_error_msg("Date too long (use -F option for multiple files)",
			date_arg, &date_arg[MAX_DATE_ARG]);
	time_arg = getdate32(date_arg, NULL);

	if (get_atime_arg) {atime_arg = time_arg; change_atime = TRUE;}
	if (get_mtime_arg) {mtime_arg = time_arg; change_mtime = TRUE;}
      }
      else
	usage_error();	/* Missing time argument or missing file */
    }
      
  } while (Iarg < Argc && IS_MEMBER(*(Argv[Iarg]), "+-") && !files_follow);

  if (!change_atime && !change_mtime && !update_ctime)
    usage_error();  /* No date specified */
    
  if (Iarg < Argc)
  {
    /* Change date of each file: */
    while (Iarg < Argc)
    {
      chdate(Argv[Iarg]);
      Iarg++;
    }
  }
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

#if defined(CYGWIN)
  tzset(); /* Recommended by the cygwin faq file */
#endif

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

    case 'a':
      get_atime_arg = OPTION(TRUE);
      break;

    case 'm':
      get_mtime_arg = OPTION(TRUE);
      break;

    case 'c':
      /* Can only change ctime to current time: */
      update_ctime = OPTION(TRUE);
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

    case 'h':
      ignore_symlink = OPTION(TRUE);
      expand_symlink = !ignore_symlink;
      break;

    case 'F': /* Multiple files follow */
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      if (strcmp(Current_Arg, "-F") != 0)
	opt_error_msg("Cannot mix -F with other options", options);
      files_follow = TRUE;
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
    case 'e': /* echo event */
      echo_event = OPTION(TRUE);
      break;

    case 'h':
      expand_symlink = OPTION(TRUE);
      ignore_symlink = !expand_symlink;
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
  FILE *out, *fopen(), *popen();
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
Usage: %s [-LG] [-h|+h] [+Z=zonename] \\\n\
		[[-m] supported_date] [-a supported_date] [-c] [-F] files\n\
       %s -C [-h|+h] \\\n\
		[[-m] unix_clock] [-a unix_clock] [-c] [-F] files\n\
\n\
       A supported_date is any one of: Floating-point style; LS style;\n\
       DD-MON-YYYY, YYYY-MON-DD, or YYYY-MM-DD styles;\n\
       DOS style or Windows style as described below.\n\
\n\
       If a unix_clock is given then it must be prefaced by -C and\n\
       consist of an integer or hexadecimal value.\n\
       If neither -m nor -a is used to preface a date, them -m is assumed.\n\
\n\
", Progname, Progname);

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
    -h: If file is a symbolic link then silently ignore (default)\n\
    +h: If file is a symbolic link then change date of target\n\
	(-h and +h are mutually exclusive)\n\
    +H: Give HELP piped through PAGER\n\
\n\
    +Z: Specify timezone to be used in place of current TZ setting.\n\
	The timezone should be in the form of:\n\
	    All OSes:		STDoffset[DST[offset][,rule]]\n\
	Additionally, if your host provides zoneinfo then you can also\n\
	use names from the appropriate zoneinfo directory:\n\
	    SunOS/Solaris:	/usr/share/lib/zoneinfo\n\
	    Linux2, FreeBSD:	/usr/share/zoneinfo\n\
	    OSF1:		/etc/zoneinfo\n\
\n\
    -m: Change modification time of files (default)\n\
    -a: Change access time of files\n\
    -c: Update the change time of files\n\
\n\
    -F: Multiple files follow (NB: If -F is specified then it must be the\n\
	last option and must precede all file arguments.  If -F is not\n\
	specified then only the final argument will be treated as a file.)\n\
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
        assumed (you can avoid possible ambiguity by specifying both the\n\
        year and the time, which is how file dates are listed if you use\n\
        'els -l' instead of 'ls -l').  The time, if given, is specified\n\
        as either 00:00 through 23:59 or 00:00:00 through 23:59:59.  Any\n\
        unspecified hh, mm, ss parameters will default to 0.  If AM/PM is\n\
        unspecified then military time is assumed.\n\
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
");

    fprintf(out, "\
Examples:\n\
    Change the modification date to April 1, 1995 12:34 Local time:\n\
	%s 19950401.1234 myfile\n\
\n\
    Change the modification date to April 1 12:34 Local time using\n\
    ls-style dates (if no year is specified then the most recent year\n\
    gone by is assumed):\n\
	%s Apr  1 12:34 myfile\n\
\n\
    Change the modification date to August 5, 2056 1:00 P.M. Local time\n\
    using dos-style dates:\n\
	%s  8-5-56  1:00p myfile\n\
\n\
    Change both the access and modification dates to January 1, 2050\n\
    12:34:56 Local time:\n\
	%s -am 20500101.123456 -F file1 file2 file3\n\
\n\
    Change the modification date to Feb 29, 1996 12:34:56 GMT and\n\
    change the access date to April 1, 1995 12:34:56 Local time:\n\
	%s -Gm 19960229.123456 -La 19950401.123456 -F file1 file2 file3\n\
\n\
    Change the modification date to a clock value of 0x87654321 and\n\
    change the access date to Feb 29, 1996 12:34:56 GMT:\n\
	%s -Cm 0x87654321 -Ga 19960229.123456 -F file1 file2 file3\n\
\n\
    Change the modification time using specified timezone:\n\
	%s --setenv:TZ=US/Mountain -m 24 July, 2012 myFile\n\
\n\
IMPORTANT NOTE:\n\
    You must use 'els' instead of 'ls' to correctly list the\n\
    date of any file set beyond Jan 19, 2038 03:14:08 GMT.\n\
\n\
    Unix does not allow setting the time of a symbolic link; rather, only\n\
    the file pointed at by the symbolic link can have its date changed.\n\
    To ensure that this restriction is fully understood, 'chdate' requires\n\
    that the '+h' option be specified if and when the user truly intends to\n\
    change the date of a symbolic link's target.\n\
\n\
    On the other hand, if the '-h' option is specified then 'chdate' will\n\
    silently ignore symbolic links (i.e. if the file is a symbolic link\n\
    then nothing will be changed and no errors will be reported).\n\
\n\
    If changing the date of multiple files then '-F' must be the last\n\
    option specified and must precede all file arguments, otherwise only\n\
    the final argument will be treated as a file.\n\
\n\
", Progname, Progname, Progname, Progname, Progname, Progname, Progname);
  }

  if (more)
    pclose(out);
  
  return;
}


void echoEvent(void)
{
  int argc = Argc;
  char **argv = Argv;
  int i, nout;
  for (i = 0, nout = 0; i < argc && nout < 512; i++)
  {
    if (i > 0) printf(" ");
    printf("%s", argv[i]);
    nout += strlen(argv[i]);
  }
  if (i < argc) printf(" ...");
  printf("\n");
  return;
}


void chdate(char *path)
{
  struct stat info;
  struct utimbuf tb;
  int sts;

# if defined(S_IFLNK)
  /* System has symbolic links: */
  sts = lstat(path, &info);
  if (sts == 0 && S_ISLNK(info.st_mode))
  {
    if (ignore_symlink)
    {
      /* Silently ignore symlinks: */
      return;
    }
    else if (expand_symlink)
    {
      /* Use info regarding the actual file rather than the link: */
      sts = stat(path, &info);
    }
    else
    {
      if (!warning_suppress)
	fprintf(stderr, "\
%s: %s: Cannot change date of a symbolic link\n\
  (NOTE: if -h is specified then symbolic links will be silently ignored;\n\
         if +h is specified the date of the file pointed at will be changed)\n\
", Progname, path);
      exit(USAGE_ERROR);
    }
  }
# else
  /* System does not have symbolic links: */
  sts = stat(path, &info);
# endif

  if (sts != 0)
  {
    errnoMsgExit(-1, path, GENERAL_ERROR);
  }
  else
  {
    /* Avoid setting directory access times, as these are extremely volatile.
       Specifying a directory's atime is usually accidental and causes +e to
       overreport and causes change times to needlessly get changed): */

    if (change_atime && !S_ISDIR(info.st_mode))
      tb.actime = atime_arg;
    else
      tb.actime = info.st_atime;

    if (change_mtime)
      tb.modtime = mtime_arg;
    else
      tb.modtime = info.st_mtime;


    /* Set date only if specified time(s) differ from current times
       (unless -c specified which forces change to update ctime regardless
       of whether atime or mtime differ): */
    if (tb.actime != info.st_atime ||
	tb.modtime != info.st_mtime ||
	update_ctime)
    {
      if (echo_event)
	echoEvent();
      if (utime(path, &tb) != 0)
	errnoMsgExit(-1, path, GENERAL_ERROR);
    }
    else
    {
      if (verboseLevel > 0)
      {
	printf("%s: %s: Date is currently set to requested time(s)\n",
	       Progname, path);
      }
    }
  }

  return;
}
