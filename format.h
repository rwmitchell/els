/******************************************************************************
  format.h -- Ubiquitous formats for ELS
  
  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: August 22, 2003
  
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

#ifndef ELS__FORMAT_H
#define ELS__FORMAT_H

/* Maximum width allowed in %nX specification: */
#define F_MAX_WIDTH  50

#define F_d        "%*d"
#define F_0d       "%0*d"
#define F_ld       "%*ld"
#define F_0ld      "%0*ld"
#define F_lld      "%*lld"
#define F_0lld     "%0*lld"
#define F_u        "%*u"
#define F_0u       "%0*u"
#define F_lu       "%*lu"
#define F_0lu      "%0*lu"
#define F_llu      "%*llu"
#define F_0llu     "%0*llu"
#define F_x        "0x%*x"
#define F_0x       "0x%0*x"
#define F_lx       "0x%*lx"
#define F_0lx      "0x%0*lx"
#define F_llx      "0x%*llx"
#define F_0llx     "0x%0*llx"

#ifdef Wformat
/* Since gcc's -Wformat option can only detect printf format problems when the
   format is completely known at compile-time, the following macros should
   occasionally be enabled whenever trying to find format problems (this
   will prevent zero_pad from working, however!): */
#define F_D(z,w,v)   F_d,w,v
#define F_LD(z,w,v)  F_ld,w,v
#define F_LLD(z,w,v) F_lld,w,v
#define F_U(z,w,v)   F_u,w,v
#define F_LU(z,w,v)  F_lu,w,v
#define F_LLU(z,w,v) F_llu,w,v
#define F_X(z,w,v)   F_x,w,v
#define F_LX(z,w,v)  F_lx,w,v
#define F_LLX(z,w,v) F_llx,w,v

#else
/* The following macros allow for possible zero padding in conjunction with
   dynamic widths (gcc's -Wformat option will not detect format problems,
   however): */
#define F_D(z,w,v)   (z ? F_0d   : F_d),w,v
#define F_LD(z,w,v)  (z ? F_0ld  : F_ld),w,v
#define F_LLD(z,w,v) (z ? F_0lld : F_lld),w,v
#define F_U(z,w,v)   (z ? F_0u   : F_u),w,v
#define F_LU(z,w,v)  (z ? F_0lu  : F_lu),w,v
#define F_LLU(z,w,v) (z ? F_0llu : F_llu),w,v
#define F_X(z,w,v)   (z ? F_0x   : F_x),w,v
#define F_LX(z,w,v)  (z ? F_0lx  : F_lx),w,v
#define F_LLX(z,w,v) (z ? F_0llx : F_llx),w,v
#endif

#if   ELS_SIZEOF_long == 4
#  define F_32D(z,w,v) F_LD(z,w,v)
#  define F_32U(z,w,v) F_LU(z,w,v)
#  define F_32X(z,w,v) F_LX(z,w,v)
#elif ELS_SIZEOF_int == 4
#  define F_32D(z,w,v) F_D(z,w,v)
#  define F_32U(z,w,v) F_U(z,w,v)
#  define F_32X(z,w,v) F_X(z,w,v)
#endif

#if   ELS_SIZEOF_llong == 8
#  define F_64D(z,w,v) F_LLD(z,w,v)
#  define F_64U(z,w,v) F_LLU(z,w,v)
#  define F_64X(z,w,v) F_LLX(z,w,v)
#elif ELS_SIZEOF_long == 8
#  define F_64D(z,w,v) F_LD(z,w,v)
#  define F_64U(z,w,v) F_LU(z,w,v)
#  define F_64X(z,w,v) F_LX(z,w,v)
#endif

/*---------------------------------------------------------------------------*/
/* The following F_ defines are used only if "config.h" was unable to
   determine the appropriate types.  While some of these values might be
   signed under various OSes, it causes no harm and simplifies things to
   consider them as unsigned.  The only problem that might be caused is
   printf() might complain when "unsigned long" format is used to print an
   "int" -- in cases where "sizeof(int) == sizeof(long)" force the value
   to be "unsigned long" prior to printing so as to squelch any complaints. */

#if   ELS_SIZEOF_int == ELS_SIZEOF_long
#define INT_SAMEAS_ULONG(v) (unsigned long)(v)
#else
#define INT_SAMEAS_ULONG(v) (v)
#endif

#if !defined(F_uid_t)
#  if   ELS_SIZEOF_uid_t == ELS_SIZEOF_long
#    define  FU_uid_t(z,w,v)  F_LU(z,w, INT_SAMEAS_ULONG(v))
#  elif ELS_SIZEOF_uid_t == ELS_SIZEOF_llong
#    define  FU_uid_t(z,w,v)  F_LLU(z,w,v)
#  else
#    define  FU_uid_t(z,w,v)  F_U(z,w,v)
#  endif
#  define  F_uid_t(z,w,v)  FU_uid_t(z,w,v)
#endif

#if !defined(F_gid_t)
#  if   ELS_SIZEOF_gid_t == ELS_SIZEOF_long
#    define  FU_gid_t(z,w,v)  F_LU(z,w, INT_SAMEAS_ULONG(v))
#  elif ELS_SIZEOF_gid_t == ELS_SIZEOF_llong
#    define  FU_gid_t(z,w,v)  F_LLU(z,w,v)
#  else
#    define  FU_gid_t(z,w,v)  F_U(z,w,v)
#  endif
#  define  F_gid_t(z,w,v)  FU_gid_t(z,w,v)
#endif

/* NB: ELS always displays "time_t" as unsigned */
#if !defined(F_time_t)
#  if   ELS_SIZEOF_time_t == ELS_SIZEOF_long
#    define  FU_time_t(z,w,v)  F_LU(z,w, INT_SAMEAS_ULONG(v))
#    define  FX_time_t(z,w,v)  F_LX(z,w, INT_SAMEAS_ULONG(v))
#  elif ELS_SIZEOF_time_t == ELS_SIZEOF_llong
#    define  FU_time_t(z,w,v)  F_LLU(z,w,v)
#    define  FX_time_t(z,w,v)  F_LLX(z,w,v)
#  else
#    define  FU_time_t(z,w,v)  F_U(z,w,v)
#    define  FX_time_t(z,w,v)  F_X(z,w,v)
#  endif
#  define  F_time_t(z,w,v)  FU_time_t(z,w,v)
#endif

#if !defined(F_st_ino)
#  if   ELS_SIZEOF_st_ino == ELS_SIZEOF_long
#    define  FU_st_ino(z,w,v)  F_LU(z,w, INT_SAMEAS_ULONG(v))
#  elif ELS_SIZEOF_st_ino == ELS_SIZEOF_llong
#    define  FU_st_ino(z,w,v)  F_LLU(z,w,v)
#  else
#    define  FU_st_ino(z,w,v)  F_U(z,w,v)
#  endif
#  define  F_st_ino(z,w,v)  FU_st_ino(z,w,v)
#endif

#if !defined(F_st_size)
#  if   ELS_SIZEOF_st_size == ELS_SIZEOF_long
#    define  FU_st_size(z,w,v)  F_LU(z,w, INT_SAMEAS_ULONG(v))
#  elif ELS_SIZEOF_st_size == ELS_SIZEOF_llong
#    define  FU_st_size(z,w,v)  F_LLU(z,w,v)
#  else
#    define  FU_st_size(z,w,v)  F_U(z,w,v)
#  endif
#  define  F_st_size(z,w,v)  FU_st_size(z,w,v)
#endif

#if !defined(F_st_nlink)
#  if   ELS_SIZEOF_st_nlink == ELS_SIZEOF_long
#    define  FU_st_nlink(z,w,v)  F_LU(z,w, INT_SAMEAS_ULONG(v))
#  elif ELS_SIZEOF_st_nlink == ELS_SIZEOF_llong
#    define  FU_st_nlink(z,w,v)  F_LLU(z,w,v)
#  else
#    define  FU_st_nlink(z,w,v)  F_U(z,w,v)
#  endif
#  define  F_st_nlink(z,w,v)  FU_st_nlink(z,w,v)
#endif

#if !defined(F_st_blocks)
#  if   ELS_SIZEOF_st_blocks == ELS_SIZEOF_long || defined(NO_ST_BLOCKS)
#    define  FU_st_blocks(z,w,v)  F_LU(z,w, INT_SAMEAS_ULONG(v))
#  elif ELS_SIZEOF_st_blocks == ELS_SIZEOF_llong
#    define  FU_st_blocks(z,w,v)  F_LLU(z,w,v)
#  else
#    define  FU_st_blocks(z,w,v)  F_U(z,w,v)
#  endif
#  define  F_st_blocks(z,w,v)  FU_st_blocks(z,w,v)
#endif

/* NB: sizeof(long) does not always == 4, e.g. on OSF1 sizeof(long) == 8 */

/*---------------------------------------------------------------------------*/

#endif /*ELS__FORMAT_H*/
