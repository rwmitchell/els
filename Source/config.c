#define OS_NAME_TO_TYPE(name) \
(strcmp(OS_NAME, "SunOS"   ) == 0 ? "SUNOS"    : \
 strcmp(OS_NAME, "HPUX"    ) == 0 ? "HPUX"     : \
 strcmp(OS_NAME, "Linux"   ) == 0 ? "LINUX"    : \
 strcmp(OS_NAME, "CYGWIN"  ) == 0 ? "CYGWIN"   : \
 strcmp(OS_NAME, "FreeBSD" ) == 0 ? "FREEBSD"  : \
 strcmp(OS_NAME, "Darwin"  ) == 0 ? "DARWIN"   : \
 strcmp(OS_NAME, "OSF1"    ) == 0 ? "OSF1"     : \
 strcmp(OS_NAME, "AIX"     ) == 0 ? "AIX"      : \
 strcmp(OS_NAME, "IRIX"    ) == 0 ? "IRIX"     : \
 strcmp(OS_NAME, "SCO_SV"  ) == 0 ? "SCO"      : \
 strcmp(OS_NAME, "ULTRIX"  ) == 0 ? "ULTRIX"   : \
 strcmp(OS_NAME, "DYNIXptx") == 0 ? "DYNIX"    : \
 strcmp(OS_NAME, "SYSV_OLD") == 0 ? "SYSV_OLD" : \
 "OS_NAME_Unknown")
/* FIXME: "isc" target has no derivable "OS_NAME" from "uname -s" */

#define OS_NAME_TO_NUM(name) \
(strcmp(name, "SunOS"   ) == 0 ?  1 : \
 strcmp(name, "HPUX"    ) == 0 ?  2 : \
 strcmp(name, "Linux"   ) == 0 ?  3 : \
 strcmp(name, "CYGWIN"  ) == 0 ?  4 : \
 strcmp(name, "FreeBSD" ) == 0 ?  5 : \
 strcmp(name, "Darwin"  ) == 0 ?  6 : \
 strcmp(name, "OSF1"    ) == 0 ?  7 : \
 strcmp(name, "AIX"     ) == 0 ?  8 : \
 strcmp(name, "IRIX"    ) == 0 ?  9 : \
 strcmp(name, "SCO_SV"  ) == 0 ? 10 : \
 strcmp(name, "ULTRIX"  ) == 0 ? 11 : \
 strcmp(name, "DYNIXptx") == 0 ? 12 : \
 strcmp(name, "SYSV_OLD") == 0 ? 13 : \
 0)

#undef OS_NUM
#if defined(SUNOS)
#  define OS_NUM  1
#endif
#if defined(HPUX)
#  define OS_NUM  2
#endif
#if defined(LINUX)
#  define OS_NUM  3
#endif
#if defined(CYGWIN)
#  define OS_NUM  4
#endif
#if defined(FREEBSD)
#  define OS_NUM  5
#endif
#if defined(DARWIN)
#  define OS_NUM  6
#endif
#if defined(__APPLE__)
#  define OS_NUM  6
#endif
#if defined(OSF1)
#  define OS_NUM  7
#endif
#if defined(AIX)
#  define OS_NUM  8
#endif
#if defined(IRIX)
#  define OS_NUM  9
#endif
#if defined(SCO)
#  define OS_NUM  10
#endif
#if defined(ULTRIX)
#  define OS_NUM  11
#endif
#if defined(DYNIX)
#  define OS_NUM  12
#endif
#if defined(SYSV_OLD)
#  define OS_NUM  13
#endif

#if !defined(OS_NUM)
#include <stdio.h>
int main(int argc, char *argv[])
{

  fprintf(stderr, "\
\n\
\"%s\" unable to to uniquely determine OS TYPE during configuration.\n\
The most likely solution is to re-run \"make\" as follows:\n\
\n\
	make clean	# Remove any residue from previous build\n\
	make		# Execute \"make\" without any arguments\n\
\n\
Please read the INSTALL file for more help.\n\
\n", argv[0]);
  return(1);  /* Return "1" so as to terminate "make" */
}
#endif

#if defined(OS_NUM)
#define NO_INCLUDE_CONFIG_H
#include "version.h"
#include "sysdefs.h"

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#if defined(LINUX)
/* Look for Linux GNU extensions such as O_NOATIME: */
/* TBD: Does __USE_GNU introduce other gremlins? */
# define __USE_GNU
# include <fcntl.h>
# undef  __USE_GNU
#else
# include <fcntl.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
/*#include <pwd.h>*/
/*#include <grp.h>*/
#include <string.h>
#include <utime.h>
#include <dirent.h>
#include <sys/param.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#if defined(DARWIN) || defined(FREEBSD) || defined(ULTRIX) || defined(__APPLE__)
/* Generic BSD defines major/minor in sys/types.h */
#else
/* Everybody else (including SunOS4!): */
#  include <sys/sysmacros.h>
#  if SUNOS >= 50000
/*   Override SunOS5's major/minor defines which follow SVR3 (this is what
     SunOS 5.4 through at least 5.10 uses for what reason who only knows!?): */
#    undef   major
#    define  major(x) getemajor(x)
#    undef   minor
#    define  minor(x) geteminor(x)
#  endif
#endif

#include "defs.h"
#include "getdate32.h"
#include "time32.h"
#include "auxil.h"
#include "els.h"
#include "elsFilter.h"
#include "elsMisc.h"
#include "sysInfo.h"
#include "sysdep.h"
#include "quotal.h"
#include "format.h"
#include "cksum.h"


int test_long_long_time(void);

void config_usage_error(char *progname)
{
  printf("Usage: %s [outfile]\n", progname);
  exit (1);
}

int main(int argc, char *argv[])
{
  FILE *out;
  FILE *fp;
  struct stat file;
  struct tm   date;
  unsigned int u;

  printf( "Starting\n\n" );
  printf( "OS_NAME_TO_NUM: %d\n", OS_NAME_TO_NUM( OS_NAME));
  printf( "OS_NUM        : %d\n",                 OS_NUM );

  fprintf(stdout, "OS_NAME_TO_NUM: %d\n", OS_NAME_TO_NUM( OS_NAME));
  fprintf(stdout, "OS_NUM        : %d\n",                 OS_NUM );

  if (argc > 2)
    config_usage_error(argv[0]);

  if (OS_NAME_TO_NUM(OS_NAME) != OS_NUM)
  {
    fprintf(stderr, "\
\n\
\"%s\" unable to to uniquely determine OS TYPE during configuration.\n\
The most likely solution is to re-run \"make\" as follows:\n\
\n\
	make clean	# Remove any residue from previous build\n\
	make		# Execute \"make\" without any arguments\n\
\n\
Please read the INSTALL file for more help.\n\
\n", argv[0]);
    /*exit(1); -- FIXME: CONTINUE ANYWAY!? */
  }

  if (argc == 1)
    out = stdout;
  else
  {
    out = fopen(argv[1], "w");
    if (out == NULL) config_usage_error(argv[0]);
  }

  fprintf(out, "/* This file was AUTOGENERATED by \"%s\" */\n", argv[0]);
  fprintf(out, "/* Any changes made to this file might be overwritten! */\n");
  fprintf(out, "/*%s*/\n", VersionID);
  fprintf(out, "\n");

  fprintf(out, "#ifndef ELS__CONFIG_H\n");
  fprintf(out, "#define ELS__CONFIG_H\n");
  fprintf(out, "\n");

  /* Validate config.h (keep #warnings insolated from older CPPs): */
  fprintf(out, "#ifdef %s\n", OS_NAME_TO_TYPE(OS_NAME));
  fprintf(out, "#  if %s != %d\n",
	  OS_NAME_TO_TYPE(OS_NAME), OS_VERSION);
  fprintf(out, "#  warning: config.h generated for different OS version\n");
  fprintf(out, "#  warning: \"make clean\" is advisable\n");
  fprintf(out, "#  endif\n");
  fprintf(out, "#else\n");
  fprintf(out, "#  warning: config.h generated for different OS\n");
  fprintf(out, "#  warning: \"make clean\" is required\n");
  fprintf(out, "#endif\n");
  fprintf(out, "\n");

  /* Translate from "uname -sr" to INTERNAL_NAME = XYYZZ: */
  fprintf(out, "#define %-10s  %d\n",
	  OS_NAME_TO_TYPE(OS_NAME), OS_VERSION);
  fprintf(out, "#define OS_NAME     \"%s\"\n", OS_NAME);
  fprintf(out, "#define OS_VERSION  %d\n", OS_VERSION);
  fprintf(out, "\n");

  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
  /* Determine whether setlocale()/strcoll() functions present:
     Everyone seems to have setlocale/strcoll except anachronistic UNIXes */

#if !defined(DYNIX) && !defined(ISC) && !defined(SYSV_OLD)
  fp = fopen("/usr/include/locale.h", "r");
#else
  fp = NULL;
#endif
  fprintf(out, "#%-6s HAVE_LOCALE\n",
	  fp != NULL ? "define" : "undef");
  if (fp != NULL) fclose(fp);
  
  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
  /* Determine whether system supports ACLs: */

  fp = fopen("/usr/include/sys/acl.h", "r");
  fprintf(out, "#%-6s HAVE_SYS_ACL_H\n",
	  fp != NULL ? "define" : "undef");
  if (fp != NULL) fclose(fp);

  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
  /* Determine whether time_t supports long long: */

  fprintf(out, "#%-6s HAVE_LONG_LONG_TIME\n",
	  test_long_long_time() ? "define" : "undef");

  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
  /* Determine whether system supports O_NOATIME: */

  fprintf(out, "#%-6s HAVE_O_NOATIME\n",
#if defined(O_NOATIME)
	  "define"
#else
	  "undef"
#endif
	  );
  fprintf(out, "\n");

  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
  /* NB: sizeof() under different compilers/OS is of different types (e.g.
     "long", "unsigned long", "unsigned int" ,etc).  So as to avoid any
     fprintf() format warnings, first assign sizeof(...) value to a
     variable of a known data-type, e.g. "u=sizeof(...)", and then use a
     format compatible with the assigned data-type.  It is my fervent
     belief that casting is evil as it hides would-be errors from the
     compiler -- assigning the sizeof(...) value to a variable instead
     allows the compiler to detect and/or correct any problems. */

  fprintf(out, "#define ELS_SIZEOF_char  %u\n", u=sizeof(char));
  fprintf(out, "#define ELS_SIZEOF_short %u\n", u=sizeof(short));
  fprintf(out, "#define ELS_SIZEOF_int   %u\n", u=sizeof(int));
  fprintf(out, "#define ELS_SIZEOF_long  %u\n", u=sizeof(long));
  fprintf(out, "#define ELS_SIZEOF_llong %u\n", u=sizeof(long long));
  fprintf(out, "\n");
  fprintf(out, "#define ELS_SIZEOF_uid_t     %u\n", u=sizeof(uid_t));
  fprintf(out, "#define ELS_SIZEOF_gid_t     %u\n", u=sizeof(gid_t));
  fprintf(out, "#define ELS_SIZEOF_time_t    %u\n", u=sizeof(time_t));
  fprintf(out, "#define ELS_SIZEOF_tm_year   %u\n", u=sizeof(date.tm_year));
  fprintf(out, "#define ELS_SIZEOF_st_ino    %u\n", u=sizeof(file.st_ino));
  fprintf(out, "#define ELS_SIZEOF_st_size   %u\n", u=sizeof(file.st_size));
  fprintf(out, "#define ELS_SIZEOF_st_nlink  %u\n", u=sizeof(file.st_nlink));
#ifndef NO_ST_BLOCKS
  fprintf(out, "#define ELS_SIZEOF_st_blocks %u\n", u=sizeof(file.st_blocks));
#endif
  fprintf(out, "\n");

#if __GNUC__ >= 3
#  if DARWIN > 0 && DARWIN < 80000
  /* Darwin 6.8 has GNUC == 3, but has weird complaints regarding "typeof()",
     so don't use it ("typeof()" is known to be fixed in Darwin 8.11.0; not
     sure about 7.x, however): */
#    undef USE_GNUC3_COMPATIBLE_TYPEOF
#  else
  /* "typeof()" is supported: */
#    define USE_GNUC3_COMPATIBLE_TYPEOF
#  endif
#else
  /* "typeof()" is not supported: */
#  undef USE_GNUC3_COMPATIBLE_TYPEOF
#endif

#if defined(USE_GNUC3_COMPATIBLE_TYPEOF)

/* Name of actual type: */
# define Type(x) \
  (__builtin_types_compatible_p(typeof(x), short)              ? "short" : \
   __builtin_types_compatible_p(typeof(x), int)                ? "int" : \
   __builtin_types_compatible_p(typeof(x), unsigned short)     ? "unsigned short" : \
   __builtin_types_compatible_p(typeof(x), unsigned)           ? "unsigned" : \
   __builtin_types_compatible_p(typeof(x), long)               ? "long" : \
   __builtin_types_compatible_p(typeof(x), unsigned long)      ? "unsigned long" : \
   __builtin_types_compatible_p(typeof(x), long long)          ? "long long" : \
   __builtin_types_compatible_p(typeof(x), unsigned long long) ? "unsigned long long" : \
   sizeof(x) <= 4 ? "unsigned" : "unsigned long long")

/* Name of equivalent unsigned type: */
# define TypeU(x) \
  (__builtin_types_compatible_p(typeof(x), short)              ? "unsigned short" : \
   __builtin_types_compatible_p(typeof(x), int)                ? "unsigned int" : \
   __builtin_types_compatible_p(typeof(x), unsigned short)     ? "unsigned short" : \
   __builtin_types_compatible_p(typeof(x), unsigned)           ? "unsigned" : \
   __builtin_types_compatible_p(typeof(x), long)               ? "unsigned long" : \
   __builtin_types_compatible_p(typeof(x), unsigned long)      ? "unsigned long" : \
   __builtin_types_compatible_p(typeof(x), long long)          ? "unsigned long long" : \
   __builtin_types_compatible_p(typeof(x), unsigned long long) ? "unsigned long long" : \
   sizeof(x) <= 4 ? "unsigned" : "unsigned long long")

  fprintf(out, "#define T_uid_t      %s\n", Type(uid_t));
  fprintf(out, "#define T_gid_t      %s\n", Type(gid_t));
  fprintf(out, "#define T_time_t     %s\n", Type(time_t));
  fprintf(out, "#define T_st_ino     %s\n", Type(file.st_ino));
  fprintf(out, "#define T_st_size    %s\n", Type(file.st_size));
  fprintf(out, "#define T_st_nlink   %s\n", Type(file.st_nlink));
  fprintf(out, "#define T_st_blocks  %s\n", Type(file.st_blocks));
  fprintf(out, "\n");

  fprintf(out, "#define TU_uid_t     %s\n", TypeU(uid_t));
  fprintf(out, "#define TU_gid_t     %s\n", TypeU(gid_t));
  fprintf(out, "#define TU_time_t    %s\n", TypeU(time_t));
  fprintf(out, "#define TU_st_ino    %s\n", TypeU(file.st_ino));
  fprintf(out, "#define TU_st_size   %s\n", TypeU(file.st_size));
  fprintf(out, "#define TU_st_nlink  %s\n", TypeU(file.st_nlink));
  fprintf(out, "#define TU_st_blocks %s\n", TypeU(file.st_blocks));
  fprintf(out, "\n");

/* Format for actual type: */
# define Fmt(x) \
  (__builtin_types_compatible_p(typeof(x), short) || \
   __builtin_types_compatible_p(typeof(x), int)                ? "F_D"  : \
   __builtin_types_compatible_p(typeof(x), unsigned short) || \
   __builtin_types_compatible_p(typeof(x), unsigned)           ? "F_U" : \
   __builtin_types_compatible_p(typeof(x), long)               ? "F_LD" : \
   __builtin_types_compatible_p(typeof(x), unsigned long)      ? "F_LU" : \
   __builtin_types_compatible_p(typeof(x), long long)          ? "F_LLD" : \
   __builtin_types_compatible_p(typeof(x), unsigned long long) ? "F_LLU" : \
   sizeof(x) <= 4 ? "F_U" : "F_LLU")

/* Format for equivalent unsigned type: */
# define FmtU(x) \
  (__builtin_types_compatible_p(typeof(x), short) || \
   __builtin_types_compatible_p(typeof(x), int) || \
   __builtin_types_compatible_p(typeof(x), unsigned short) || \
   __builtin_types_compatible_p(typeof(x), unsigned)           ? "F_U" : \
   __builtin_types_compatible_p(typeof(x), long) || \
   __builtin_types_compatible_p(typeof(x), unsigned long)      ? "F_LU" : \
   __builtin_types_compatible_p(typeof(x), long long) || \
   __builtin_types_compatible_p(typeof(x), unsigned long long) ? "F_LLU" : \
   sizeof(x) <= 4 ? "F_U" : "F_LLU")

/* Format for equivalent hex type: */
# define FmtX(x) \
  (__builtin_types_compatible_p(typeof(x), short) || \
   __builtin_types_compatible_p(typeof(x), int) || \
   __builtin_types_compatible_p(typeof(x), unsigned short) || \
   __builtin_types_compatible_p(typeof(x), unsigned)           ? "F_X" : \
   __builtin_types_compatible_p(typeof(x), long) || \
   __builtin_types_compatible_p(typeof(x), unsigned long)      ? "F_LX" : \
   __builtin_types_compatible_p(typeof(x), long long) || \
   __builtin_types_compatible_p(typeof(x), unsigned long long) ? "F_LLX" : \
   sizeof(x) <= 4 ? "F_X" : "F_LLX")

  fprintf(out, "#define F_uid_t(z,w,v)      %s(z,w,v)\n", Fmt(uid_t));
  fprintf(out, "#define F_gid_t(z,w,v)      %s(z,w,v)\n", Fmt(gid_t));
  fprintf(out, "#define F_time_t(z,w,v)     %s(z,w,v)\n", Fmt(time_t));
  fprintf(out, "#define F_st_ino(z,w,v)     %s(z,w,v)\n", Fmt(file.st_ino));
  fprintf(out, "#define F_st_size(z,w,v)    %s(z,w,v)\n", Fmt(file.st_size));
  fprintf(out, "#define F_st_nlink(z,w,v)   %s(z,w,v)\n", Fmt(file.st_nlink));
  fprintf(out, "#define F_st_blocks(z,w,v)  %s(z,w,v)\n", Fmt(file.st_blocks));
  fprintf(out, "\n");

  fprintf(out, "#define FU_uid_t(z,w,v)     %s(z,w,v)\n", FmtU(uid_t));
  fprintf(out, "#define FU_gid_t(z,w,v)     %s(z,w,v)\n", FmtU(gid_t));
  fprintf(out, "#define FU_time_t(z,w,v)    %s(z,w,v)\n", FmtU(time_t));
  fprintf(out, "#define FU_st_ino(z,w,v)    %s(z,w,v)\n", FmtU(file.st_ino));
  fprintf(out, "#define FU_st_size(z,w,v)   %s(z,w,v)\n", FmtU(file.st_size));
  fprintf(out, "#define FU_st_nlink(z,w,v)  %s(z,w,v)\n", FmtU(file.st_nlink));
  fprintf(out, "#define FU_st_blocks(z,w,v) %s(z,w,v)\n", FmtU(file.st_blocks));
  fprintf(out, "\n");
  
  fprintf(out, "#define FX_time_t(z,w,v)    %s(z,w,v)\n", FmtX(time_t));
  fprintf(out, "\n");
  
#else

  fprintf(out, "\
/* Unable to determine data types in \"config.h\" for:\n\
       T_uid_t,  T_gid_t,  T_time_t,\n\
       TU_uid_t, TU_gid_t, TU_time_t,\n\
       T_st_ino,  T_st_size,  T_st_nlink,  T_st_blocks\n\
       TU_st_ino, TU_st_size, TU_st_nlink, TU_st_blocks\n\
   Data types found in \"defs.h\" will be used instead. */\n\
\n");

  fprintf(out, "\
/* Unable to determine format types in \"config.h\" for:\n\
       F_uid_t,   F_gid_t,  F_time_t,\n\
       FU_uid_t,  FU_gid_t, FU_time_t,\n\
       F_st_ino,  F_st_size,  F_st_nlink,  F_st_blocks\n\
       FU_st_ino, FU_st_size, FU_st_nlink, FU_st_blocks\n\
       FX_time_t\n\
   Format types found in \"format.h\" will be used instead. */\n\
\n");

#endif

  fprintf(out, "#endif /*ELS__CONFIG_H*/\n");
  fprintf(out, "/* EOF */\n");

  return(0);
}
#endif /*defined(OS_NUM)*/


#include <time.h>

int test_long_long_time(void)
{
  time_t clock;
  int test1, test2;
  struct tm *lt;

  clock = (unsigned long)0x88800000;
  lt = gmtime(&clock);
  /*printf("%d %d\n", lt->tm_year+1900, lt->tm_mon+1);*/
  test1 = (lt->tm_year+1900 == 2042 && lt->tm_mon+1 == 7);

  clock = clock << 3;
  lt = gmtime(&clock);
  /*printf("%d %d\n", lt->tm_year+1900, lt->tm_mon+1);*/
  test2 = (lt->tm_year+1900 == 2550 && lt->tm_mon+1 == 7);

  return(test1 && test2);
}
