/******************************************************************************
  els.h -- els functions, variables, and defines
  
  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: April 28, 2016
  
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

#ifndef ELS__ELS_H
#define ELS__ELS_H

/* Add /usr/include/sys/stat.h defs if necessary: */
#if !defined(S_ISLNK)
#  define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#endif
#if !defined(S_ISDIR)
#  define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#endif
#if !defined(S_ISBLK)
#  define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#endif
#if !defined(S_ISCHR)
#  define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#endif
#if !defined(S_ISFIFO)
#  define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#endif
#if defined(SUNOS) && !defined(S_IFDOOR)
/* This definition will not upset SUNOS4: */
#  define S_IFDOOR  0xd000
#endif

/* Determine maximum size of file name and path name based on information
   found in /usr/include/dirent.h and /usr/include/sys/param.h: */

/* Momentarily disable dirent64 defines (if currently set): */
#undef  ELS__DEFINE_DIRENT64
#define ELS__UNDEF_DIRENT64
#include "defs.h"

#include <dirent.h>
#include <sys/param.h>

/* Reenable dirent64 defines (if previously set): */
#undef  ELS__DEFINE_DIRENT64
#undef  ELS__UNDEF_DIRENT64
#include "defs.h"

/* Maximum size of directory name (+1 for CNULL): */
#if defined(MAXPATHLEN)
#  define MAX_DNAME (MAXPATHLEN+1)	/* SunOS4&5, Ultrix, HPUX */
#elif defined(_D_NAME_MAX)
#  define MAX_DNAME (_D_NAME_MAX+1)	/* AIX */
#elif defined(PATH_MAX)
#  define MAX_DNAME (PATH_MAX+1)	/* Linux2 */
#else
/* Be overly generous with default: */
/* OSF1 believed to use 512 */
#  define MAX_DNAME (4096+1)		/* Default: OSF1 */
#endif

/* Maximum size of file name (+1 for CNULL): */
#if defined(MAXNAMLEN)
#  define MAX_FNAME (MAXNAMLEN+1)	/* SunOS4&5, Ultrix, OSF1 */
#elif defined(_MAXNAMLEN)
#  define MAX_FNAME (_MAXNAMLEN+1)	/* HPUX */
#elif defined(NAME_MAX)
#  define MAX_FNAME (NAME_MAX+1)	/* Linux2 */
#else
/* Be overly generous with default: */
/* AIX believed to use 256 */
#  define MAX_FNAME (1024+1)		/* Default: AIX */
#endif

/* Maximum size of full name (+4 for quotes): */
#define  MAX_FULL_NAME  (MAX_DNAME+MAX_FNAME+4)

/* Maximum amount of info (besides the full file name) per line: */
#define  MAX_INFO  256

/* Maximum amount of output generated per line (sizeof(info) +
   sizeof(file name) + sizeof(optional symlink name) + sizeof(" -> ")): */
#define  MAX_OUTPUT  (MAX_INFO+2*MAX_FULL_NAME+4)

/* Maximum amount of output generated by a message: */
#define  MAX_MSG  (MAX_INFO+MAX_FULL_NAME)

/* Manually define maximum User/Group name to some reasonable length.
   I've yet to find these in any include file although at least one OS
   (i.e. SunOS4.x) allows more than 70 chars (which seems crazy!?): */
#define  MAX_USER_GROUP_NAME  64
#if  MAX_USER_GROUP_NAME < 12    /* strlen("-2147483648")+1 */
/* Minimum of 12 characters in case UID/GID has no passwd/group entry: */
#  error: Minimum of 12 characters required for UID/GID name
#endif


typedef struct Dir_Item
{
  char *mem;
  char *fname;

  Boole islink;
  Boole isdir;
  Boole isdev;

  Boole dotdir;
  Boole hidden;
  Boole listable;
  Boole searchable;

  int readdir_errno;		/* errno value set by readdir() */
  int stat_errno;		/* errno value set by stat() */
  struct stat info;		/* info structure from stat() */

  struct Dir_Item *next;
} Dir_Item;

typedef struct Dir_List
{
  Dir_Item *head;
  Dir_Item *tail;
} Dir_List;

/********** Global Routines Defined **********/

#include <time.h> /* Needed for T_print() prototype */

/* extern void finishExit(void); */
/* extern void do_getenv(void); */
/* extern void do_options(char *options); */
/* extern void do_options_minus(char *options); */
/* extern void do_options_plus(char *options); */
/* extern void stamp_setup(int pass); */
/* extern void untouch_setup(int pass); */
/* extern void give_usage_or_help(char U_or_H); */
/* extern void analyze_options(void); */
/* extern void append_dir(Dir_List *dlist, char *fname, int fname_size); */
/* extern void copy_info(Dir_Item *dest, Dir_Item *src); */
/* extern int  read_dir(Dir_List *dlist, char *dname, DIR *dir); */
/* extern void stat_dir(Dir_List *dlist, char *dname); */
/* extern void name_dir(Dir_List *dlist, char *dname); */
/* extern void sort_dir(Dir_List *dlist, char *dname, int n); */
/* extern void list_dir(Dir_List *dlist, char *dname); */
/* extern void free_dir(Dir_List *dlist); */
/* extern Boole list_item(Dir_Item *file, char *dname); */
/* extern char *find_first_directive(char *list, char *fmt); */
extern char *scaleSizeToHuman(ELS_st_size val, int base);
/* extern char *G_print(char *buff, char *fmt, char *dname, Dir_Item *file); */
/* extern char *N_print(char *buff, char *fmt, char *dname, Dir_Item *file); */
extern char *T_print(char *buff, char *fmt, time_t ftime,
		     struct tm *fdate, Boole gmt, Boole meridian);
/* extern char *T_print_width(char *buff, char *fmt, time_t ftime,
		    struct tm *fdate, Boole gmt, Boole meridian, int width);  */
/* extern char *separate(char *bp, char icase); */
/* extern char *finish(char *bp); */
/* extern char *full_name(char *dname, char *fname); */
/* extern Dir_Item *dirItemAlloc(void); */
/* extern void dirItemFree(Dir_Item *file); */
/* extern void dirItemShow(FILE *out); */
/* extern void watchSigHandler(int sig); */
extern int sigSafe_stat(char *path, struct stat *buf);
extern int sigSafe_lstat(char *path, struct stat *buf);
/* extern void spin_wheel(void); */

/********** Global Variables Defined **********/

extern Dir_Item *CwdItem;
#include "elsVars.h"

#endif /*ELS__ELS_H*/
