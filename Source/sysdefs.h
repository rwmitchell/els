/******************************************************************************
  sysdefs.h -- System defines for ELS
  
  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: June 26, 2008
  
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

#ifndef ELS__SYSDEFS_H
#define ELS__SYSDEFS_H

#ifdef NO_INCLUDE_CONFIG_H
/* Do nothing */
#else
#include "config.h"
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Define one of SUNOS4/SUNOS5 if SUNOS is in the proper range: */
#if defined(SUNOS)
#  if (SUNOS >= 40000 && SUNOS < 50000)
#    define SUNOS4
#  elif (SUNOS >= 50000)
#    define SUNOS5
#  endif
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* BSD-ish timelocal() function present:
   SunOS 4.x, FreeBSD, Darwin >= 6.x, ULTRIX */

#if defined(SUNOS4) || defined(FREEBSD) || DARWIN >= 60000 || defined(ULTRIX)
#  define HAVE_TIMELOCAL
#endif

/* NOTE: I seem to remember that Darwin5.x did not support timelocal()!?
   but I have no way of confirming this; Darwin6.x+ does support timelocal. */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* BSD-ish "tm_zone" element present in "tm" structure:
   SunOS 4.x, FreeBSD, Darwin, ULTRIX */

#if defined(SUNOS4) || defined(FREEBSD) || defined(DARWIN) || defined(ULTRIX)
#  define HAVE_TM_ZONE
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* SVR3-ish anachronisms: DYNIX/ptx, ISC, SYSV_OLD */

#if defined(DYNIX) || defined(ISC) || defined(SYSV_OLD)
#  define NO_ST_BLOCKS
#  define HAVE_STIME
#endif

/* FIXME: ISC's uname -s is busted, so how to determine OS_NAME for config.c?
   "make isc" or "make sysv-old" need to be implicity specified to get to
   here.  SYSV_OLD is a currently a mind-set rather than an OS_NAME. */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Mandatory locking (i.e. chmod +l): */
#if SUNOS >= 50000
#  define HAVE_MANDATORY_LOCKING
#endif

/* NOTE: Currently only SunOS5 has mandatory locking as far as I know (see
   "man chmod").  Linux as of 2.6.9-67 is known to not support mandatory
   locks according to "info coreutils chmod". */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* ACLs: */

#if defined(HAVE_SYS_ACL_H)
#  if SUNOS >= 50500 || defined(LINUX) || defined(DARWIN) || \
      defined(HPUX) || defined(OSF1) || defined(AIX) || defined(IRIX)
#    define HAVE_ACL
#  else
/*   NOTE: SunOS5.4 has sys/acl.h, but doesn't seem to work until SunOS5.5 */
#    undef HAVE_ACL
#  endif
#else
#    undef HAVE_ACL
#endif

/* CYGWIN-1.5.24 has sys/acl.h, but CYGWIN's /usr/lib is lacking full
   support. */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* All versions of HPUX should have this defined: */
#if defined(HPUX)
#  ifndef _HPUX_SOURCE
#    define _HPUX_SOURCE
#  endif
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* The define HAVE_TIME_R is helpful but not mandatory.  For example, there
   may be other untested OS versions not included in the following list that
   may also support localtime_r(), gmtime_r(), asctime_r(), and ctime_r().  It
   may also be possible that earlier versions of an OS (e.g.  HPUX9) support
   *time_r(), but until proven otherwise it is better to assume that *time_r()
   functions are missing. */

/* The following OSes are known to support *time_r() with _REENTRANT flag:
   SunOS5, HPUX10, OSF1/DigitalUNIX4.0 */
#if defined(SUNOS5) || HPUX >= 100000 || defined(OSF1)
#  define HAVE_TIME_R
#  define _REENTRANT
/* Note: SunOS5.4, HPUX10, OSF1 require that _REENTRANT be defined,
   while SunOS5.5+ requires either of _REENTRANT or __EXTENSIONS__ */
#endif

/* vxWorks5.1 through vxWorks6.9 are known to support *time_r() with the
   _EXTENSION_POSIX_REENTRANT flag: */
#if VXWORKS >= 510
#  define HAVE_TIME_R
#  define _EXTENSION_POSIX_REENTRANT
/* Note: vxWorks does not require that _EXTENSION_POSIX_REENTRANT be defined
   as it gets gratuitously defined inside types/vxANSI.h (how ironic!). */
#endif

/* The following OSes are known to support *time_r() without _REENTRANT flag:
   Linux 2.2.12, Cygwin 1.3.22/1.5.20, FreeBSD 3.2, FreeBSD 4.0, Darwin7.9,
   AIX 3.0 */
#if defined(LINUX) || defined(CYGWIN) || defined(FREEBSD) || \
    DARWIN >= 60000 || defined(AIX)
#  define HAVE_TIME_R
#endif

/* HPUX10 has *time_r() functions, but they return "int" with a return value
   of -1 meaning failure and 0 meaning success.  All other known OSes having
   *time_r() functions return "char *" with a return of a NULL pointer
   meaning failure and a non-NULL pointer pointing to the buffer.  Rather
   than try to repair this convolution just pretend that HPUX10 doesn't have
   *time_r().  (This problem has been fixed in HPUX11) */
#if HPUX >= 100000 && HPUX < 110000
#  undef HAVE_TIME_R  /* Don't use */
#  undef _REENTRANT   /* Don't use */
#endif

/* The following OSes are known to NOT support *time_r():
   SunOS4, Darwin5.2.2(!), vxWorks5.0.2 */
#if defined(SUNOS4) || (DARWIN>0 && DARWIN<60000) || (VXWORKS>0 && VXWORKS<510)
#  undef HAVE_TIME_R
#endif


#if HPUX >= 110000 || defined(LINUX) || defined(CYGWIN) || \
    defined(FREEBSD) || DARWIN >= 60000 || defined(OSF1) || \
    defined(AIX) || defined(IRIX) || defined(SCO)
/* The following OSes are known to use 2 args for asctime_r() and ctime_r():
   Linux 2.2.12, FreeBSD 3.2, Darwin7.9, FreeBSD 4.0, OSF1, HPUX11 */
/* The following OSes are believed to use 2 args for asctime_r() and ctime_r():
   AIX 3.0, IRIX 6.x, SCO */
#  define USE_POSIX_TIME_R
#else
/* The following OSes are known to use 3 args for asctime_r() and ctime_r():
   SunOS5, HPUX10, vxWorks5.1 through vxWorks6.9 */
#  undef USE_POSIX_TIME_R
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* STAT64: SunOS >= 5.6,  LINUX >= 2.2.x,  HPUX >= 10.0,  IRIX >= 6.x */

/* Note: It is known that Linux2.2.12 and that Irix6.5 have stat64.  From
   this it is assumed that all versions of Linux 2.2.x and IRIX 6.x and above
   also have stat64; however, it is unknown and therefore possible that
   earlier versions of these OSes may also support stat64.

   NOTE: FreeBSD and Darwin do NOT have stat64, but their regular stat uses
   an offset type of 64 bits thus making their stat equivalent to the newer
   stat64 structure.

   NOTE: CYGWIN 1.5.24 supports stat64, but it is unnecessary to use it as
   stat and stat64 are equivalent under this and probably earlier versions.
   */
/* NOTE:  This code shared by df.c */

/* Define HAVE_STAT64 if this OS release is known to support it: */
#if SUNOS >= 50600 || LINUX >= 20200 || HPUX >= 100000 || IRIX >= 60000
#  define HAVE_STAT64
#endif

/* dirent64 is known to be available on Linux2.4 and Linux2.6.9/i686, but
   is optional.  dirent64 is REQUIRED on SunOS5.7+ and Linux2.6.18+/x64 to
   fix a quirky bug that only exhibits itself on very specific OS/NAS
   combinations (might be required for all Linux/x64?).  It is also
   assumed to be required on SunOS5.6, but has not be tested.
   Note: HAVE_DIRENT64 infers having HAVE_STAT64 but HAVE_STAT64
   does not infer HAVE_DIRENT64 (e.g. HP-UX 11.x has stat64, but is without
   dirent64/readdir64). */
/* Define HAVE_DIRENT64 if this OS release is known to support it: */
#if SUNOS >= 50600 || LINUX >= 20400
#  define HAVE_DIRENT64
#endif

/* If HAVE_STAT64 is defined, then define any additional OS-specific features
   that are also required: */
#if defined(HAVE_STAT64)
/* SunOS5.6 through 5.10, Linux2.2.9 through 2.6.18, and HP-UX 10.20
   require _LARGEFILE64_SOURCE for 64 bit file interface: */
#  define _LARGEFILE64_SOURCE

/* HP-UX 10.x also requires the following if using _LARGEFILE64_SOURCE: */
#  if defined(HPUX) && !defined(__STDC_EXT__)
#    define __STDC_EXT__
#  endif
#endif /*HAVE_STAT64*/

#if defined(FREEBSD) || defined(DARWIN)
/* FreeBSD 3.4, Darwin5.2.2, Darwin6.4, and Darwin7.9 are known to only
   support 64-bit offset types in their regular stat structure.  Apparently
   they have no need to support older binaries compiled using obsolete
   "stat" structures and thus they have made a clean-cut from the past
   by assuming that "stat" has always been 64-bit aware.
   I wish my world were so simple. */
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#endif /*ELS__SYSDEFS_H*/
