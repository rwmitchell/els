/******************************************************************************
  defs.h -- Ubiquitous defines for ELS
  
  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: September 6, 2012
  
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

/*===========================================================================*/

#ifndef ELS__DEFINE_STAT64
#define ELS__DEFINE_STAT64

/*---------------------------------------------------------------------------*/
/* Globally redefine "stat" to be "stat64" (this is the easiest way I've
   found to deal with OSes using stat/stat64): */

#if defined(HAVE_STAT64) && !defined(ELS__UNDEF_STAT64)
#  define stat  stat64
#  define lstat lstat64
#else
#  undef  stat
#  undef  lstat
#endif

/*---------------------------------------------------------------------------*/
/* Globally redefine "open" to be "open64" (this is the easiest way I've
   found to deal with OSes using stat/stat64).  It seems safe to assume
   that if stat64 is implemented then so is open64 (I have confirmed this
   on SunOS5.6, HPUX10.20, and LINUX2.2 -- I am not certain about IRIX6.x).
   So far cases open64 appears to work with normal close: */

#if defined(HAVE_STAT64) && !defined(ELS__UNDEF_STAT64)
#  define open  open64
#else
#  undef  open
#endif

/*---------------------------------------------------------------------------*/
#endif /*ELS__DEFINE_STAT64*/

/*===========================================================================*/

#ifndef ELS__DEFINE_DIRENT64
#define ELS__DEFINE_DIRENT64

/*---------------------------------------------------------------------------*/
/* Globally redefine "dirent" to be "dirent64" (this is the easiest way I've
   found to deal with OSes using dirent/dirent64): */

#if defined(HAVE_DIRENT64) && !defined(ELS__UNDEF_DIRENT64)
#  define dirent  dirent64
#  define readdir readdir64
#else
#  undef  dirent
#  undef  readdir
#endif

/*---------------------------------------------------------------------------*/
#endif /*ELS__DEFINE_DIRENT64*/

/*===========================================================================*/

#ifndef ELS__DEFS_H
#define ELS__DEFS_H

/*---------------------------------------------------------------------------*/
#ifdef NO_INCLUDE_CONFIG_H
typedef  unsigned int        ELS_uid_t;
typedef  unsigned int        ELS_gid_t;
typedef  unsigned long       ELS_time_t;
typedef  unsigned long       ELS_st_ino;
typedef  unsigned long       ELS_st_size;
typedef  unsigned long       ELS_st_nlink;
typedef  unsigned long       ELS_st_blocks;

#else
/* NB: If any of the following data types are long long, then use strtoull
   during elsFilter routines: */

#if   defined(TU_uid_t)
typedef  TU_uid_t            ELS_uid_t;
#elif ELS_SIZEOF_uid_t == ELS_SIZEOF_long
typedef  unsigned long       ELS_uid_t;
#elif ELS_SIZEOF_uid_t == ELS_SIZEOF_llong
typedef  unsigned long long  ELS_uid_t;
#define  USE_STRTOULL
#else
typedef  unsigned            ELS_uid_t;
#endif

#if   defined(TU_gid_t)
typedef  TU_gid_t            ELS_gid_t;
#elif ELS_SIZEOF_gid_t == ELS_SIZEOF_long
typedef  unsigned long       ELS_gid_t;
#elif ELS_SIZEOF_gid_t == ELS_SIZEOF_llong
typedef  unsigned long long  ELS_gid_t;
#define  USE_STRTOULL
#else
typedef  unsigned            ELS_gid_t;
#endif

#if   defined(T_time_t) && defined(HAVE_LONG_LONG_TIME)
typedef  T_time_t            ELS_time_t;
#elif defined(TU_time_t)
typedef  TU_time_t           ELS_time_t;
#elif ELS_SIZEOF_time_t == ELS_SIZEOF_long
typedef  unsigned long       ELS_time_t;
#elif ELS_SIZEOF_time_t == ELS_SIZEOF_llong
typedef  unsigned long long  ELS_time_t;
#define  USE_STRTOULL
#else
typedef  unsigned            ELS_time_t;
#endif

#if   defined(TU_st_ino)
typedef  TU_st_ino           ELS_st_ino;
#elif ELS_SIZEOF_st_ino == ELS_SIZEOF_long
typedef  unsigned long       ELS_st_ino;
#elif ELS_SIZEOF_st_ino == ELS_SIZEOF_llong
typedef  unsigned long long  ELS_st_ino;
#define  USE_STRTOULL
#else
typedef  unsigned            ELS_st_ino;
#endif

#if   defined(TU_st_size)
typedef  TU_st_size          ELS_st_size;
#elif ELS_SIZEOF_st_size == ELS_SIZEOF_long
typedef  unsigned long       ELS_st_size;
#elif ELS_SIZEOF_st_size == ELS_SIZEOF_llong
typedef  unsigned long long  ELS_st_size;
#define  USE_STRTOULL
#else
typedef  unsigned            ELS_st_size;
#endif

#if   defined(TU_st_nlink)
typedef  TU_st_nlink         ELS_st_nlink;
#elif ELS_SIZEOF_st_nlink == ELS_SIZEOF_long
typedef  unsigned long       ELS_st_nlink;
#elif ELS_SIZEOF_st_nlink == ELS_SIZEOF_llong
typedef  unsigned long long  ELS_st_nlink;
#define  USE_STRTOULL
#else
typedef  unsigned            ELS_st_nlink;
#endif

#if   defined(TU_st_blocks)
typedef  TU_st_blocks         ELS_st_blocks;
#elif ELS_SIZEOF_st_blocks == ELS_SIZEOF_long || defined(NO_ST_BLOCKS)
typedef  unsigned long        ELS_st_blocks;
#elif ELS_SIZEOF_st_blocks == ELS_SIZEOF_llong
typedef  unsigned long long   ELS_st_blocks;
#define  USE_STRTOULL
#else
typedef  unsigned             ELS_st_blocks;
#endif

#endif
/*---------------------------------------------------------------------------*/

#define Void	(void)	/* void type-cast sans parentheses */
#define Local	static	/* synonym for static storage-class */
#define Global	extern	/* synonym for extern storage-class */
#define Fast  register	/* synonym for register storage-class */
#define Hw    volatile	/* type-qualifier used for all hardware addresses */

typedef char		Boole;	/* TRUE or FALSE variable or function */
typedef char		(*Boole_Function)();
typedef	unsigned char	Uchar;
typedef	unsigned short	Ushort;
typedef	unsigned int	Uint;
typedef	unsigned long	Ulong;
typedef	unsigned long	Paddr;	/* Physical address in its non-pointer form */

#if   ELS_SIZEOF_long == 4
typedef long		int32;
typedef unsigned long	Uint32;
#elif ELS_SIZEOF_int == 4
typedef int		int32;
typedef unsigned int	Uint32;
#else
typedef int		int32;
typedef unsigned int	Uint32;
#endif

#if   ELS_SIZEOF_long == 8
typedef long		int64;
typedef unsigned long	Uint64;
#elif ELS_SIZEOF_llong == 8
typedef long long	int64;
typedef unsigned long long Uint64;
#else
typedef long long	int64;
typedef unsigned long long Uint64;
#endif

#ifdef __cplusplus
typedef int		(*Int_Function)(void);
typedef void		(*Void_Function)(void);
#else
typedef int		(*Int_Function)();
typedef void		(*Void_Function)();
#endif

#undef   NULL
#define  NULL	0
#define  CNULL	'\0'
#undef   TRUE
#define  TRUE	(Boole)1
#undef   FALSE
#define  FALSE	(Boole)0

#define  SECS_PER_MIN   ((Ulong)60)
#define  SECS_PER_HOUR  (60 * SECS_PER_MIN)
#define  SECS_PER_DAY   (24 * SECS_PER_HOUR)
#define  SECS_PER_WEEK  (7 * SECS_PER_DAY)
#define  SECS_PER_YEAR  (365 * SECS_PER_DAY + SECS_PER_DAY / 4) /* Average */
/* More accurate is the average number of Gregorian seconds per year:
   #define SECS_PER_YEAR  (SECS_PER_DAY * (365*400 + 97) / 400) */
#define  SECS_PER_MONTH (SECS_PER_YEAR / 12)			/* Average */

#define  IS_MEMBER(c,s)     ((c) != CNULL && strchr(s,c) != NULL)
#define  IS_NOT_MEMBER(c,s) ((c) == CNULL || strchr(s,c) == NULL)

/* Set one or more bits: */
#define  SETBITS(x,bits)  x |= (bits)
#define  SETBIT(x,bit)    SETBITS(x,bit)
/* Clear one or more bits: */
#define  CLRBITS(x,bits)  x &= ~(bits)
#define  CLRBIT(x,bit)    CLRBITS(x,bit)
/* Test for ALL bits being set: */
#define  ALLBITS(x,bits)  (((x) & (bits)) == (bits))
/* Test for ANY bit being set: */
#define  ANYBIT(x,bit)    (((x) & (bit)) != 0)

/*---------------------------------------------------------------------------*/
/* To avoid discrepancies between OSes, ELS prefers to use its own version
   of strto*l and strto*ll functions.  However, some linkers complain if
   the same names are used as the system functions, so the following
   aliases are used instead: */
#define strtol   elsStrtol
#define strtoul  elsStrtoul
#define strtoll  elsStrtoll
#define strtoull elsStrtoull

/*---------------------------------------------------------------------------*/
/* "ctype.h" wrapper defines to prevent nasty sign-extension: */

#define isGraph(c) isgraph((c) & 0xff)
#define isSpace(c) isspace((c) & 0xff)
#define isDigit(c) isdigit((c) & 0xff)
#define isAlpha(c) isalpha((c) & 0xff)
#define isAlnum(c) isalnum((c) & 0xff)
#define isUpper(c) isupper((c) & 0xff)
#define isLower(c) islower((c) & 0xff)
#define toLower(c) tolower((c) & 0xff)
#define toUpper(c) toupper((c) & 0xff)

/* NOTE: I prefer masking (c & 0xff) rather than casting (unsigned char)c,
   because casting hides mistakes from the compiler.  Consider the following:
   {
     char *c = "This code is INCORRECT because c is pointer";
     isgraph((unsigned char)c);  // Casting forces compiler to accept mistakes
     isgraph(c & 0xff);   // Masking allows compiler to catch mistakes
   }
   */

/*---------------------------------------------------------------------------*/
/* Largest signed negative number for given data-type: */

#define MAXNEG_LLONG  (long long)(1ULL << (8*sizeof(long long)-1))
#define MAXNEG_LONG        (long)(1UL  << (8*sizeof(long)-1))
#define MAXNEG_INT          (int)(1U   << (8*sizeof(int)-1))
#define MAXNEG_SHORT      (short)(1U   << (8*sizeof(short)-1))
#define MAXNEG_CHAR        (char)(1U   << (8*sizeof(char)-1))
#define MAXPOS_LLONG  (~MAXNEG_LLONG)
#define MAXPOS_LONG   (~MAXNEG_LONG)
#define MAXPOS_INT    (~MAXNEG_INT)
#define MAXPOS_SHORT  (~MAXNEG_SHORT)
#define MAXPOS_CHAR   (~MAXNEG_CHAR)

/* NOTE: certain compilers accept either 1L/1LL or 1UL/1ULL, while
   other compilers (e.g. Sun C Compiler 5.8) *require* 1UL/1ULL,
   thus, use 1UL/1ULL so as to make everyone happy. */
/* NB: if -m32 then sizeof(long) == 4, if -m64 then sizeof(long) == 8 */
/* FIXME: The above macros assume 8 bits == 1 byte */

/*---------------------------------------------------------------------------*/
#endif /*ELS__DEFS_H*/

/*===========================================================================*/
