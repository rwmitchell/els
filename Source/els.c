/******************************************************************************
  els.c -- An Enhanced LS look-alike with many additional features.

  Author: Mark Baranowski and James M. Gleason
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

  Acknowledgements and contributions:
  This program is based on an earlier program written by James M. Gleason.
  Access filtering idea contributed by Larry Gensch.
  AIX ACL feature contributed by Maurizio Sartori.
  SCO port contributed by Todd A. Andrews.
  Darwin ACL feature contributed by Morque Ozwald.

  Note: The most recent version of this program may be obtained from
        http://els-software.org

  ****************************************************************************/

#include "version.h"
#include "sysdefs.h"

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
/*#include <pwd.h>*/
/*#include <grp.h>*/
#include <string.h>
#include <utime.h>
#include <dirent.h>
#include <sys/param.h>
#include <sys/mount.h>    // statfs()
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>     // rwm
#include <wchar.h>    // rwm
#define HAVE_LOCALE
#ifdef HAVE_LOCALE
#include <locale.h>
#endif

#ifdef LINUX
#include <sys/statfs.h>
#endif

#if defined(DARWIN) || defined(FREEBSD) || defined(ULTRIX)
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
#include "hg.h"
#include "git.h"

char LGC = ' '; /* LGC default value used by convert_iso8601 */

/********** Global Routines Referenced **********/

extern char *getenv();

/********** Global Routines Defined **********/

void finishExit(void);
void do_getenv(void);
void do_options(char *options);
void do_options_minus(char *options);
void do_options_plus(char *options);
void do_options_minus_minus(char *options);
void stamp_setup(int pass);
void untouch_setup(int pass);
void give_usage_or_help(char U_or_H);
void analyze_options(void);
void append_dir(Dir_List *dlist, char *fname, int fname_size);
void copy_info(Dir_Item *dest, Dir_Item *src);
int  read_dir(Dir_List *dlist, char *dname, DIR *dir);
void stat_dir(Dir_List *dlist, char *dname);
void name_dir(Dir_List *dlist, char *dname);
void sort_dir(Dir_List *dlist, char *dname, int n);
void list_dir(Dir_List *dlist, char *dname);
void free_dir(Dir_List *dlist);
Boole list_item(Dir_Item *file, char *dname);
char *find_first_directive(char *list, char *fmt);
char *G_print(char *buff, char *fmt, char *dname, Dir_Item *file);
char *N_print(char *buff, char *fmt, char *dname, Dir_Item *file);
char *T_print(char *buff, char *fmt, time_t ftime,
        struct tm *fdate, Boole gmt, Boole meridian);
char *T_print_width(char *buff, char *fmt, time_t ftime,
        struct tm *fdate, Boole gmt, Boole meridian, int width);
char *separate(char *bp, char icase);
char *finish(char *bp);
char *full_name(char *dname, char *fname);
Dir_Item *dirItemAlloc(void);
void dirItemFree(Dir_Item *file);
void dirItemShow(FILE *out);
void watchSigHandler(int sig);
int sigSafe_stat(char *path, struct stat *buf);
int sigSafe_lstat(char *path, struct stat *buf);
void spin_wheel(void);

/********** Global Variables Defined **********/

int  Iarg;
int  Argc;
char **Argv;
char *Progname;
char *LSICONS = NULL,      // rwm - from LS_ICONS
     *HGICONS = NULL,      // rwm - from HG_ICONS
     *GTICONS = NULL;      // rwm - from GIT_ICONS
const
char *LSCOLOR = NULL,      // rwm - from LS_COLORS
     *FSCOLOR = NULL,      // rwm - ELS_FS_COLOR  - file size
     *FSWIDTH = NULL,
     *HGSTATS = NULL,
     *GTSTATS = NULL,
     *EXFAT   = NULL;
// export ELS_FT_COLORS="86400=0;32;1:6480000=0;32:7121234=0;32;2:31557600=1;33;2:-1=0;31;1:"
char *FTCOLOR = NULL,      // rwm - ELS_FT_COLORS - file ages and colors
     *rwm_cols[32];
const
char *inv = "[7;0m",     // invert foreground/background colors
     *rinv= "[27;0m",    // reset invert
     *cs  = "[0m";       // clear ansii codes
Ulong rwm_ages[32];        // rwm - 32 date colors should be enough for anyone
int   rwm_ftcnt=0,
      rwm_szwdth=0,
      rwm_fxwdth=0;
uid_t Whoami;
time_t The_Time;
time_t The_Time_in_an_hour;
char *Time_Zone = NULL;
int Debug = 0;
int VersionLevel = VERSION_LEVEL;
int MaxVersionLevel;
Boole VersionLevelArg = FALSE;
Boole ArgOrder = FALSE;
Boole TruncateName = FALSE;
Boole MaskId = FALSE;
Boole QuitOnError = FALSE;
Boole SortCI = FALSE;
Boole SortDICT = FALSE;
Boole First_listing = TRUE;
char *Current_Opt = NULL;
char *Current_Arg = NULL;
Dir_Item *CwdItem = NULL;
int CwdBlocksize = 0;
const
char *CwdPath = ".";

Boole listingError = FALSE;

Boole zero_file_args;
Boole multiple_file_args;
Boole using_full_names;
Boole avoid_trimmings;
char *hg_root = NULL,
     *hg_stat = NULL,
     *gt_root = NULL,
     *gt_stat = NULL;
Boole list_topdir;      // defined in elsVars.h
char first_mac;
int recursion_level = 0;

/* SEMANTICS: In general, when intrepreting options ELS leans more
 * towards SYS5 (with a little GNU sprinkled in) than it does towards
 * BSD.  On the other hand, when it comes to listing format and
 * spacing, ELS leans more towards BSD.
 *
 * Meaningful combinations are:
 * Flags  Bits             Behavior
 * +4     SEM_ELS|SEM_BSD  Interpret options a la BSD, output a la ELS.
 * +5     SEM_ELS|SEM_SYS5 Interpret options a la SYS5, output a la ELS.
 * +g     SEM_ELS|SEM_GNU  Interpret options a la GNU, output a la ELS.
 * +l4    SEM_LS|SEM_BSD   Behave as much like BSD's /bin/ls as is reasonable.
 * +l5    SEM_LS|SEM_SYS5  Behave as much like SYS5's /bin/ls as is reasonable.
 * +lg    SEM_LS|SEM_GNU   Behave as much like GNU's /bin/ls as is reasonable.
 * +l     SEM_LS           Output as much like native /bin/ls as is reasonable.
 * none   SEM_ELS          Interpret options a la SYS5 (except for
 *                         -g, -G, and -s options), output a la ELS.
 */
Uint Sem;

Boole Aa_option;
Boole list_hidden;
Boole list_dotdir;
Boole list_long;
Boole list_long_numeric;
Boole list_long_omit_gid;
Boole list_atime;
Boole list_ctime;
Boole list_inode;
Boole list_size_in_blocks;
Boole list_size_human_2;
Boole list_size_human_10;
Boole expand_symlink;
Boole expand_directories;
Boole mark_files;
Boole mark_dirs;
Boole dash_b;
Boole dash_Q;
Boole recursive;
Boole reverse_sort;
Boole time_sort;
Boole unsort;
Boole g_flag;
Boole G_flag;
Boole plus_c;
cksumType cksum_type;
int cksum_size;
Boole cksum_unaccess;
Boole list_directories;
Boole list_directories_specified;
Boole plus_q;
Boole plus_q_specified;
Boole traverse_mp;
Boole traverse_expanded_symlink;
Boole traverse_specified;
Boole useLcCollate;
Boole useLcTime;
int verboseLevel;
Boole warning_suppress;
Boole watch_progress;
FILE *ttyin = NULL, *ttyout = NULL;
Boole zero_st_info;
Boole zero_st_info_specified;
Ulong zero_st_mask;
#define ZERO_DIR_MTIME    0000001
#define ZERO_DIR_ATIME    0000002
#define ZERO_DIR_SIZE     0000004
#define ZERO_LINK_MTIME   0000010
#define ZERO_LINK_ATIME   0000020
#define ZERO_LINK_OWNER   0000040
#define ZERO_DEV_MTIME    0000100
#define ZERO_DEV_ATIME    0000200
#define ZERO_FIFO_MTIME   0001000
#define ZERO_FIFO_ATIME   0002000
#define ZERO_CKSUM_NONREG 0010000
#define ZERO_CLEARCASE    0100000

Boole CCaseMode = FALSE;
Boole GTarStyle = FALSE;
Boole Tar5Style = FALSE;
Boole FirstFound = FALSE;
Boole OncePerDir = FALSE;
int DirDepth = -1;
Boole IncludeSS = FALSE;
Boole IncludeSS_specified = FALSE;  /* Don't let envvar override */

/********** Local Variables Defined **********/

Local Boole G_option = FALSE;
Local Boole N_option = FALSE;
Local Boole T_option = FALSE;

Local char *G_format;
#define Gf_INODE    'i'
#define Gf_TYPE_IN_QUIET  'q'
#define Gf_TYPE_IN_ALPHA  't'
#define Gf_TYPE_IN_SYMBOLIC 'T'
#define Gf_PERM_IN_ALPHA  'p'
#define Gf_PERM_IN_NUMERIC  'P'
#define Gf_PERM_IN_SYMBOLIC 'M'
#define Gf_ACL_INDICATOR  'A'
#define Gf_SIZE_IN_BYTES  's'
#define Gf_SIZE_IN_BLOCKS 'S'
#define Gf_SIZE_HUMAN_2   'H'
#define Gf_SIZE_HUMAN_10  'h'
#define Gf_TIME_MODIFIED  'm' /* Calls T_format */
#define Gf_TIME_ACCESSED  'a' /* Calls T_format */
#define Gf_TIME_MODE_CHANGED  'c' /* Calls T_format */
#define Gf_UID_IN_ALPHA   'u'
#define Gf_UID_IN_NUMERIC 'U'
#define Gf_GID_IN_ALPHA   'g'
#define Gf_GID_IN_NUMERIC 'G'
#define Gf_OWNER_IN_ALPHA 'o'
#define Gf_OWNER_IN_NUMERIC 'O'
#define Gf_CHECKSUM   'C'
#define Gf_NAME_FORMAT    'n' /* Calls N_format */
#define Gf_FULL_NAME_FORMAT 'N' /* Calls N_format */
#define Gf_LINK_COUNT   'l'

Local char *N_format;
#define Nf_LINK_NAME    'l'
#define Nf_LINK_PTR_NAME  'L' /* Shared by G_format */
#define Nf_QUOTE_NAME   'q'
#define Nf_FULL_NAME    'F' /* Shared by G_format */
#define Nf_DIR_NAME   'd' /* Shared by G_format */
#define Nf_FILE_NAME    'f' /* Shared by G_format */

Local char *T_format;
#define Tf_ABS_TIME   'a'
#define Tf_DELTA_TIME   'd'
#define Tf_REL_TIME   'r'
#define Tf_YOFFSET_TIME   'y'
#define Tf_ALPHA_DATE   'A'
#define Tf_NUM_DATE   'N'
#define Tf_GMT_DATE   'G'
#define Tf_LOCAL_DATE   'L'
#define Tf_MERIDIAN_TIME  'M'
#define Tf_HEX_OUTPUT   'x'

#define Tf_FLOATING_POINT_DATE  'F' /* YYYYMMDD.hhmmss */
#define Tf_ISO8601_DATE   'I' /* YYYYMMDD.hhmmss */
#define Tf_FLOATING_POINT_DAY 'f' /* YYYYMMDD */
#define Tf_ISO8601_DAY    'i' /* YYYYMMDD */
#define Tf_ELS_DATE   'e'
#define Tf_LS_DATE    'l'
#define Tf_DOS_DATE   'd'
#define Tf_WINDOWS_DATE   'w'
#define Tf_VERBOSE_DATE   'v'
#define Tf_ELAPSED_TIME   'E' /* D+h:m:s */

#define Tf_YEARS    'Y'
#define Tf_MONTHS   'M'
#define Tf_WEEKS    'W'
#define Tf_DAYS     'D'
#define Tf_HOURS    'h'
#define Tf_MINS     'm'
#define Tf_SECS     's'
#define Tf_CLOCK    'c'
#define Tf_TIME_2   't' /* h:m */
#define Tf_TIME_3   'T' /* h:m:s */
#define Tf_TIME_OR_YEAR   'Q'
#define Tf_YEARS_MOD_100  'y'
#define Tf_ZONE_NAME    'Z'

Local int munge = 0;
Local int untouch = 0;
Local Boole cksumming = FALSE;
Local Boole filtering = FALSE;
Local Boole inc_filtering = FALSE;
Local Boole exc_filtering = FALSE;
Local Boole quotaling = FALSE;
Local Boole stamping = FALSE;
Local char *stamp_option;
Local Boole execute_mode = FALSE;
Local Boole execute_symlinks = FALSE;
Local Boole execute_sts_check = FALSE;
Local Uint execute_sts_good = 0;

Local char output_buff[MAX_OUTPUT];
Local int  output_nlines = 0;

Local int squeeze_cnt = 0;
Local Boole squeeze = FALSE;
Local Boole fielding = FALSE;
Local Boole as_is;
Local Boole separated;
Local char separator = ' ';

/* Static messages: */
const Local char *NON_NEGATABLE = "Non-negatable option";
const Local char *MISSING_FILTER = "Missing filter";
const Local char *TOO_MANY_FILTERS = "Too many filters specified";

Boole (*rwm_col_ext)();
Boole rwm_col_ext1( char *fn, int *b, int *f, int *s, int *i );
Boole rwm_col_ext2( char *fn, int *b, int *f, int *s, int *i );
Boole rwm_col_ext3( char *fn, int *b, int *f, int *s, int *i );

Boole rwm_filtering = FALSE,
      rwm_ifreg     = FALSE, // Regular
      rwm_ifexe     = FALSE, // Executable
      rwm_ifwrt     = FALSE, // Writable
      rwm_ifred     = FALSE, // Readable
      rwm_ifdir     = FALSE, // Directory
      rwm_ifchr     = FALSE, // Char Special
      rwm_ifblk     = FALSE, // Block Special
      rwm_ififo     = FALSE, // Fifo
      rwm_iflnk     = FALSE, // Symbolic Link
      rwm_ifsock    = FALSE, // Socket
      rwm_ifunk     = FALSE, // unkown
      rwm_docolor   = TRUE,  // colorize output
      rwm_doicons   = FALSE, // add icons
      rwm_doperms   = TRUE,  // ignore file permissions
      rwm_docomma   = TRUE,
      rwm_dospace   = FALSE; // similar to find -print0
Local int rwm_type,          // copy of file "type"
          rwm_mode,          // copy of file "mode"
          rwm_ext_mode=3;    // rwm_col_extN mode

Local ELS_st_blocks dir_block_total;
Local int dir_file_count;
Local Ulong dirItemAllocAvail = 0;
Local Ulong dirItemAllocInUse = 0;

/* Uncomment the desired signal type for +W: */
/*#define WATCH_SIGNAL  SIGINT*/  /* watch == ^C */
#define WATCH_SIGNAL  SIGQUIT   /* watch == ^\ (^| on some systems) */
Local Void_Function defaultSigHandler = NULL;
Local Boole sigEvent = FALSE;
Local Boole sigAskAbort = FALSE;

/*****************************************************************************/
int rwm_env2ft( char *env, char sep, Ulong *age, char **col ) {
  int  cnt = 0;
  char *p1, *p2;

  p1   = env;

  while ( *p1 ) {
     p2 = strchr( p1, sep );  // find separator
    *p2 = '\0';               // truncate string

    age[cnt] = strtoul( p1, NULL, 10 );
    p1=strchr( p1, '=' );
    col[cnt] = p1+1;

//  printf("%2d: #[%sm%20lu[m#  %s\n", cnt, col[cnt], age[cnt], col[cnt] );

    cnt++;
    p1 = p2+1;
  }

  return ( cnt );
}
/*****************************************************************************/

int main(int argc, char *argv[])
{
  char *dname;
  Dir_List dlist;

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
%s: for non-stat64 capable OSes.  Please read TECH_NOTES #1.\n",
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

#ifdef Wformat
  fprintf(stderr, "%s: Warning: compiled with -DWformat (zero_pad disabled)\n",
    Progname);
#endif

#ifndef BUFFER_STDOUT
  /* Define BUFFER_STDOUT if you desire buffering on stdout and stderr
     (buffering stdout and stderr will improve performance, but it will
     also cause errors to be randomly placed within the output): */
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);
#endif /*!BUFFER_STDOUT*/

  /* Set the default OPTION values: */

  if (strcmp(Progname, "els5") == 0)
    Sem = SEM_ELS|SEM_SYS5;
  else if (strcmp(Progname, "els4") == 0)
    Sem = SEM_ELS|SEM_BSD;
  else if (strcmp(Progname, "ls5") == 0)
    Sem = SEM_LS|SEM_SYS5;
  else if (strcmp(Progname, "ls4") == 0)
    Sem = SEM_LS|SEM_BSD;
  else if (strcmp(Progname, "ls") == 0)
#if defined(SUNOS4) || defined(DARWIN) || defined(FREEBSD) || defined(ULTRIX)
    Sem = SEM_LS|SEM_BSD;
#elif defined(LINUX)
    Sem = SEM_LS|SEM_GNU;
#else
    Sem = SEM_LS|SEM_SYS5;
#endif
  else /* default is ELS with neither SYS5 nor BSD specific flavorings: */
    Sem = SEM_ELS;

  Aa_option = FALSE;
  list_hidden = FALSE;
  list_dotdir = FALSE;
  list_long = FALSE;
  list_long_numeric = FALSE;
  list_long_omit_gid = FALSE;
  list_atime = FALSE;
  list_ctime = FALSE;
  list_inode = FALSE;
  list_size_in_blocks = FALSE;
  list_size_human_2 = FALSE;
  list_size_human_10 = FALSE;
  expand_symlink = FALSE;
  expand_directories = TRUE;
  mark_files = FALSE;
  mark_dirs = FALSE;
  dash_b = FALSE;
  dash_Q = FALSE;
  recursive = FALSE;
  reverse_sort = FALSE;
  time_sort = FALSE;
  unsort = FALSE;
  g_flag = FALSE;
  G_flag = FALSE;
  plus_c = FALSE;
  cksum_type = CKSUM_UNSPECIFIED;
  cksum_size = 0;
  cksum_unaccess = FALSE;
  list_directories = TRUE;
  list_directories_specified = FALSE;
  plus_q = FALSE;
  plus_q_specified = FALSE;
  traverse_mp = FALSE;
  if (VersionLevel <= 145) traverse_mp = TRUE;
  traverse_expanded_symlink = FALSE;
  traverse_specified = FALSE;
#ifdef HAVE_LOCALE
  useLcCollate = TRUE;
  useLcTime = TRUE;
#else
  useLcCollate = FALSE;
  useLcTime = FALSE;
#endif
  verboseLevel = 0;
  warning_suppress = FALSE;
  watch_progress = FALSE;
  zero_st_info = FALSE;
  zero_st_info_specified = FALSE;

  /* Default listing formats: */
# define  G_format_DEFAULT  "n" /* Gf_NAME_FORMAT */
# define  N_format_DEFAULT  "f" /* Nf_FILE_NAME */
# define  T_format_DEFAULT  "e" /* Tf_ELS_DATE */
  G_format = G_format_DEFAULT;
  N_format = N_format_DEFAULT;
  T_format = T_format_DEFAULT;

  fexpr_ORcount = 0;
  fexpr_ANDcount = 0;
  exc_file_count = 0;
  exc_dir_count = 0;
  inc_file_count = 0;
  inc_dir_count = 0;

  /* How many files? */
  {
    int i, n_files = 0;
    for (i = 1; i < Argc; i++)
    {
      if (IS_NOT_MEMBER(Argv[i][0], "+-"))
  n_files++;
    }
    zero_file_args = (n_files == 0);
    multiple_file_args = (n_files > 1);
  }

  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
  /* This section processes command and file arguments in the order they
     arrive--this was the standard mode for ELS version 1.42 and below
     (enabled by setting the environment variable ELS_ARG_ORDER): */
  if (ArgOrder)
  {
    Iarg = 1;
    do
    {
      /* Process options in the order they arive: */
      while (Iarg < Argc && IS_MEMBER(Argv[Iarg][0], "+-"))
      {
  do_options(Argv[Iarg]);
  Iarg++;
      }
      analyze_options();

      /* Process files and directories in the order they arrive: */
      dlist.head = NULL;
      while (Iarg < Argc && IS_NOT_MEMBER(Argv[Iarg][0], "+-"))
      {
  if (Argv[Iarg][0] != CNULL)
    append_dir(&dlist, Argv[Iarg], 0);
  Iarg++;
      }

      /* If there were no valid files and this is the first listing, then by
   default list all files.  However, prevent "els +G... f1 f2 +X" from
   listing "f1 f2" and then applying +X to every file the second time
   around.  On the other hand, "els +G... f1 f2 +X f3 f4" is
   considered valid (although f1 f2 will be listed while +X will apply
   only to f3 f4). */
      if (dlist.head == NULL) {
        if (First_listing)
          append_dir(&dlist, ".", 0);
        else
        {
          Current_Arg = Argv[Argc-1];
          opt_error_msg("Option must precede files to have any effect",
            Current_Arg+1);
        }
      }

      /* Perform the listing: */
      dname = "";
      stat_dir(&dlist, dname);
      sort_dir(&dlist, dname, -1);
      list_dir(&dlist, dname);
      free_dir(&dlist);
      First_listing = FALSE;
    } while (Iarg < Argc);
  }

  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
  /* This section processes command arguments during the first pass and then
     lists any file arguments during the second pass--this is the standard
     mode as of ELS version 1.43 and above: */
  else
  {
    /* Process options in the first pass: */
    for (Iarg = 1; Iarg < Argc; Iarg++)
    {
      if (IS_MEMBER(Argv[Iarg][0], "+-"))
  do_options(Argv[Iarg]);
    }
    analyze_options();

    /* Process files and directories in the second pass: */
    dlist.head = NULL;
    for (Iarg = 1; Iarg < Argc; Iarg++)
    {
      if (IS_NOT_MEMBER(Argv[Iarg][0], "+-") && Argv[Iarg][0] != CNULL)
    append_dir(&dlist, Argv[Iarg], 0);
    }
    if (dlist.head == NULL)
      append_dir(&dlist, ".", 0);

    /* Perform the listing: */
    dname = "";
    stat_dir(&dlist, dname);
//  printf( "\n\nSTART: <%s>\n\n", dlist.head->fname );
    struct statfs fsbuf;
    if ( statfs( dlist.head->fname, &fsbuf ) != 0 ) {
      // RWM - ignore the error, it isn't critical
//    printf( "statfs failed, exiting" );
//    exit(0);
    } else {
        // f_type can be generated using gnu stat:
        // stat -f -c "Type:%T %t" .
//      printf( "\nFS Type: %0X\n", fsbuf.f_type );
        // 0x001C is type smbfs  - Big Sur  ?
        // 0x001E is type smbfs  - Monterey ? - 2022-04-29
        if ( fsbuf.f_type == 0x001E       // exfat
          || fsbuf.f_type == 0x001F )     // msdos
          rwm_doperms = FALSE;
    }
    sort_dir(&dlist, dname, -1);
    list_dir(&dlist, dname);
    free_dir(&dlist);
  }
  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

  if ( rwm_fxwdth ) fprintf( stderr, "Use: ELS_FS_WIDTH=%d\n", rwm_fxwdth );

  finishExit();

  /*NOTREACHED*/
  return(NORMAL_EXIT);
}


void finishExit(void)
{
  if (quotaling)
    grandQuotal_print();

  /* Put a period on it: */
  if (plus_c || stamping || untouch > 0)
    printf("# EOF.\n");

  if (Debug&0x20)
  {
    FILE *out = (ttyout != NULL ? ttyout : stderr);
    memShow(out);
    dirItemShow(out);
  }

  /* Return status back to SHELL: */
  if (ANYBIT(Sem,SEM_BSD) || VersionLevel <= 147)
    /* BSD returns zero status regardless of listing errors;
       ELS prior to 1.47 behaved similar to BSD: */
    exit(NORMAL_EXIT);
  else
    /* Return non-zero status if any error occurred during listing: */
    exit(listingError ? LISTING_ERROR : NORMAL_EXIT);
}


void do_getenv(void)
{
  Whoami = geteuid();

#ifdef HAVE_LOCALE
  /* Freshen locale env settings from lowest to highest precedence: */
  {
    char *lc;
    if ((lc = getenv("LANG")) != NULL)
      lc = setlocale(LC_ALL, lc);
    if ((lc = getenv("LC_COLLATE")) != NULL)
      lc = setlocale(LC_COLLATE, lc);
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
  The_Time_in_an_hour = The_Time + SECS_PER_HOUR;

  /* Set any debugging bits: */
  {
    char *debug = getenv("ELS_DEBUG");
    if (debug != NULL) Debug = strtoul(debug, NULL, 0);
  }

  FSWIDTH = getenv( "ELS_FS_WIDTH" );  // Set additional width padding
  if ( FSWIDTH ) rwm_szwdth = strtol( FSWIDTH, NULL, 10 );
  if ( rwm_docolor ) {
    LSCOLOR = getenv( "LS_COLORS"     );       // color by extension
    LSICONS = getenv( "LS_ICONS"      );       // file icons
    HGICONS = getenv( "HG_ICONS"      );       // hg  status icons
    GTICONS = getenv( "GIT_ICONS"     );       // git status icons
    FSCOLOR = getenv( "ELS_FS_COLOR"  );       // color by file size
    FTCOLOR = getenv( "ELS_FT_COLORS" );       // color by file time/age
    HGSTATS = getenv( "ELS_HG_STATUS" );
    GTSTATS = getenv( "ELS_GIT_STATUS" );
    EXFAT   = getenv( "ELS_EXFAT"     );       // ignore file permissions

    if ( FTCOLOR)
      rwm_ftcnt = rwm_env2ft( FTCOLOR, ':', rwm_ages, rwm_cols );
#ifdef  RWM_DEBUG
    printf("rwm_ftcnt: %d\n", rwm_ftcnt );

    for (int i=0; i<rwm_ftcnt; ++i )
      printf("%2d: #[%sm%20lu[39m#  %-8s\n", i, rwm_cols[i], rwm_ages[i], rwm_cols[i] );
    printf("----\n");
#endif
  }
  if ( ! LSCOLOR ) rwm_docolor = FALSE;
  if (   LSICONS ) rwm_doicons = TRUE;
  if (   EXFAT   ) rwm_doperms = FALSE;

  if ( LSICONS ) {
    char *ps =LSICONS;
    while ( *ps != '\0' ) { *ps = toupper( *ps ); ++ps; }
  }

  /* Enable behavior of earlier ELS releases if so requested: */
  MaxVersionLevel = VersionLevel;
  {
    char *els_ver_level = getenv("ELS_VER_LEVEL");
    if (els_ver_level != NULL)
    {
      char version[16];
      sprintf(version, "+v%.8s", els_ver_level);
      do_options(version);
      VersionLevelArg = FALSE; /* Env var considered different than arg */
    }
    if (Argc >= 2 && strncmp(Argv[1], "+v", 2) == 0)
      do_options(Argv[1]);
    if (VersionLevel > MaxVersionLevel)
      fprintf(stderr, "\
%s: Warning: Requested level %s exceeds this version's supported level %s\n\
  Consider upgrading to a newer version of ELS\n",
        Progname, verToStr(VersionLevel), verToStr(MaxVersionLevel));
  }

  /* Parse options and file args in their given order if env var set: */
  ArgOrder = envGetBoole("ELS_ARG_ORDER") || VersionLevel <= 142;

  /* Truncate annoyingly long USER/GROUP names if env var set: */
  TruncateName = envGetBoole("ELS_TRUNCATE_NAME");

  /* Mask USER/GROUP IDs over 0xffff0000 if env var set: */
  MaskId = envGetBoole("ELS_MASK_ID") ||
    (VersionLevel <= 149 && !envGetBoole("ELS_NO_MASK_ID"));
  /* Note: in versions 150+ MaskId defaults to FALSE unless "ELS_MASK_ID" set;
     in versions 149- MaskId defaulted to TRUE unless "ELS_NO_MASK_ID" set. */

  /* Quit immediately if any listing errors occur: */
  QuitOnError = envGetBoole("ELS_QUIT_ON_ERROR");

  /* Allow SnapShots to always be included: */
  if (! IncludeSS_specified)
    IncludeSS = envGetBoole("ELS_INCLUDE_SS");

  /* Sort Case Insensitive: */
  SortCI = envGetBoole("ELS_SORT_CI");

  /* Sort dictionary order (similar to "sort -df" and xx.UTF-8 locale): */
  SortDICT = envGetBoole("ELS_SORT_DICT");

  return;
}


void do_options(char *options)
{
  static int level = 0;
  if (level++ == 0) Current_Arg = options;

  if (Debug&1) printf("%d: do_options(%s)\n", level, options);
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
      options = ""; /* No more options */
      break;

    case 'a': /* List all files including "." and ".." */
      list_hidden = OPTION(TRUE);
      list_dotdir = OPTION(TRUE);
      Aa_option = TRUE; /* One of 'A', 'a', '~A', '~a' was specified */
      break;

    case 'A': /* List all files except "." and ".." */
      list_hidden = OPTION(TRUE);
      list_dotdir = FALSE;
      /* Note: both -A and -~A must disable "list_dotdir" otherwise -~A
   would have the strange effect of ignoring all hidden files *except*
   "." and ".."! */
      Aa_option = TRUE; /* One of 'A', 'a', '~A', '~a' was specified */
      break;

    case 'd': /* Inhibit expansion of directories specified as params */
      expand_directories = OPTION(FALSE);
      break;

    case 'l': /* Long listing as in ls -l */
      list_long = OPTION(TRUE);
      break;

    case 'n': /* Long listing with UID and GID in numeric */
      list_long = OPTION(TRUE);
      list_long_numeric = OPTION(TRUE);
      break;

    case 'o': /* Long listing omitting GID */
      list_long = OPTION(TRUE);
      list_long_omit_gid = OPTION(TRUE);
      break;

    case 'g': /* Interpret -g flag using either BSD or SYS5 semantics */
      g_flag = OPTION(TRUE);
      break;

    case 'G': /* Interpret -G flag using GNU semantics */
      G_flag = OPTION(TRUE);
      break;

    case 'L': /* Expand symbolic link */
      expand_symlink = OPTION(TRUE);
      break;

    case 'u': /* List atime */
      list_atime = OPTION(TRUE);
      /* Both BSD and SYS5/GNU agree that -u always supersedes -c: */
      if (list_atime) list_ctime = FALSE;
      break;

    case 'c': /* List ctime */
      list_ctime = OPTION(TRUE);
      if (list_ctime)
      {
  /* In SYS5/GNU the last -c or -u specified supersedes the previous,
     while in BSD/ELS if -u is specified then -c is ignored: */
        if (!ANYBIT(Sem,SEM_BSD|SEM_ELS)) list_atime = FALSE;
        if (ANYBIT(Sem,SEM_BSD|SEM_ELS) && list_atime) list_ctime = FALSE;
      }
      break;

    case 'i': /* List inode */
      list_inode = OPTION(TRUE);
      break;

    case 's': /* List size in blocks */
      list_size_in_blocks = OPTION(TRUE);
      break;

    case 'h': /* List size human using 2^10 (first -h) or 10^3 (second -h) */
      {
  Boole last_human_2 = list_size_human_2;
  list_size_human_2  = FALSE;
  list_size_human_10 = FALSE;
  if (last_human_2)
    list_size_human_10 = OPTION(TRUE);
  else
    list_size_human_2  = OPTION(TRUE);
      }
      break;

    case 'r': /* Reverse sort */
      reverse_sort = OPTION(TRUE);
      break;

    case 't': /* Sort by time */
      time_sort = OPTION(TRUE);
      break;

    case 'U': /* Unsort */
      unsort = OPTION(TRUE);
      break;

    case 'R': /* Recursively list all directories found (same as +R) */
      recursive = OPTION(TRUE);
      break;

    case 'F': /* Mark files */
      mark_files = OPTION(TRUE);
      break;

    case 'p': /* Mark directories */
      mark_dirs = OPTION(TRUE);
      break;

    case 'b': /* List non-printable characters using \nnn octal */
      dash_b = OPTION(TRUE);
      break;

    case 'Q': /* List names using C-syntax a la GNU */
      dash_Q = OPTION(TRUE);
      break;

    case '1': /* List single column */
      /* Currently the default */
      break;

    case 'C': /* List multi column */  /* hijacked -rwm 1999-01-11 */
      /* Unimplemented */
      rwm_docomma = FALSE;    // use -C to disable both addons
      rwm_docolor = FALSE;
      break;

    case '0': /* rwm: space in filename handling */
      rwm_dospace = TRUE;
      break;

    case 'f': /* rwm: filter files */
      rwm_filtering = TRUE;
      switch ( *options++ ) {
        case 'd': rwm_ifdir = TRUE; break;
        case 'x': rwm_ifexe = TRUE; rwm_ifreg = TRUE; break;
        case 'w': rwm_ifwrt = TRUE; rwm_ifreg = TRUE; break;
        case 'r': rwm_ifred = TRUE; rwm_ifreg = TRUE; break;
        case 'R': rwm_ifreg = TRUE; break;
        case 'c': rwm_ifchr = TRUE; break;  /* Char Special */
        case 'b': rwm_ifblk = TRUE; break;  /* Block Special */
        case 'f': rwm_ififo = TRUE; break;  /* Fifo */
        case 'l': rwm_iflnk = TRUE; break;  /* Symbolic Link */
        case 's': rwm_ifsock= TRUE; break;  /* Socket */
        case 'u': rwm_ifunk = TRUE; break;  /* unkown */
        default: break;
      }
      break;

    case 'x': rwm_ext_mode = strtol( options, NULL, 10 );
              if ( rwm_ext_mode == 0 ) rwm_ext_mode = 3;
              else options++;
//            printf( "ext mode: %dX\n", rwm_ext_mode );
              break;

    default: /* Give error message and usage, then exit: */
      opt_error_msg("Unrecognized '-' option", options);
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
      /* Exactly one of either SEM_ELS or SEM_LS must always be set;
   thus, clearing one implies setting the other: */
    case 'l':
      CLRBITS(Sem, SEM_ELS|SEM_LS);
      if (negate)
  SETBIT(Sem, SEM_ELS);
      else
  SETBIT(Sem, SEM_LS);
      break;

      /* Either one or none of SEM_ELS, SEM_SYS5, or SEM_GNU can be set;
   thus, clearing one does not imply setting any of the others: */
    case 'b': /* '+b' is DEPRECATED, use +4 instead */
    case '4':
      if (negate)
  CLRBIT(Sem, SEM_BSD);
      else
      {
  CLRBITS(Sem, SEM_BSD|SEM_SYS5|SEM_GNU);
  SETBIT(Sem, SEM_BSD);
      }
      break;

    case '5':
      if (negate)
  CLRBIT(Sem, SEM_SYS5);
      else
      {
  CLRBITS(Sem, SEM_BSD|SEM_SYS5|SEM_GNU);
  SETBIT(Sem, SEM_SYS5);
      }
      break;

    case 'g':
      /* undocumented option */
      if (negate)
  CLRBIT(Sem, SEM_GNU);
      else
      {
  CLRBITS(Sem, SEM_BSD|SEM_SYS5|SEM_GNU);
  SETBIT(Sem, SEM_GNU);
      }
      break;

    case 'c': /* Generate command/environment message */
      plus_c = OPTION(TRUE);
      break;

    case 'C': /* checksum algorithm */
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      if (*options == '=')
      {
  options++;
  do {
    int length = 1;
    if (*options == 'B' || strncmp(options, "bsd", 3) == 0)
    {
      cksum_type = CKSUM_BSD;
      cksum_size = 16;
      if (*options == 'b') length = 3;
    }
    else if (*options == 'S' || strncmp(options, "sysv", 4) == 0)
    {
      cksum_type = CKSUM_SYSV;
      cksum_size = 16;
      if (*options == 's') length = 4;
    }
    else if (*options == 'P' || strncmp(options, "posix", 5) == 0)
    {
      cksum_type = CKSUM_POSIX;
      cksum_size = 32;
      if (*options == 'p') length = 5;
    }
    else if (*options == 'C' || strncmp(options, "crc32", 5) == 0)
    {
      cksum_type = CKSUM_CRC32;
      cksum_size = 32;
      if (*options == 'c') length = 5;
    }
    else if (*options == 'U' || strncmp(options, "unaccess", 8) == 0)
    {
      cksum_unaccess = TRUE;
      if (*options == 'u') length = 8;
    }
    else
    {
      opt_error_msg("Expected B|bsd, S|sysv, P|posix, C|crc32, or U|unaccess",
        options+1);
    }
    options += length;
    if (*options == ',')
      options++;
    else if (length > 1 && *options != ',' && *options != '\0')
      opt_error_msg("Expected ','", options+1);
  } while (*options != '\0');
      }
      else
      {
  opt_error_msg("Expected =algorithm", options+1);
      }
      break;

    case 'D': /* '+D' is DEPRECATED, use +d instead */
    case 'd': /* List directories as files */
      /* NB: This does not affect the expansion of directories, rather, the
   purpose is to generate directory listings without directory files. */
      list_directories = OPTION(FALSE);
      list_directories_specified = TRUE;
      break;

    case 'h': /* Allow +X to act on symbolic links */
      execute_symlinks = OPTION(TRUE);
      break;

    case 'o': /* Specify output file for stdout */
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      if (*options != '=')
  opt_error_msg("Expected '=file_name'", options+1);
      else
      {
  /* Close stdout and then reopen using file_name (this assumes
     the first available file descriptor gets reused): */
  if (close(1) != 0)
    opt_error_msg("Non-working option for this OS", options+1);
  options++;
  if (fopen(options, "w") == NULL)
    /*opt_error_msg("Unable to open output file", options+1);*/
    errnoMsgExit(-1, options, GENERAL_ERROR);
      }
      options = ""; /* No more options */
      break;

    case 'q': /* Quote file names having special characters */
      plus_q = OPTION(TRUE);
      plus_q_specified = TRUE;  /* Command-line args take precedence */
      break;

    case 'R': /* Recursively list all directories found (same as -R) */
      recursive = OPTION(TRUE);
      break;

    case 't': /* Traverse mount-points during recursive listings */
      /* Look for and read "=type": */
      traverse_specified = TRUE;
      if (*options == '=')
      {
  options++;
  if (IS_NOT_MEMBER(*options, "~ML"))
      opt_error_msg("Expected one of '~ML'", options+1);

  while (IS_MEMBER(*options, "~ML"))
  {
    Boole save_negate = negate;
    char type = *options++;

    if (type == '~')
    {
      negate = !negate;
      type = *options++;
    }

    if (type == 'M')
      traverse_mp = OPTION(TRUE);
    else if (type == 'L')
      /* If a directory is symbolically linked to a parent directory,
         this will cause els (and /bin/ls) to loop infinitely.
         Possible infinite symlink loops can be avoided by disabling
         symlink traversal by specifying "+~t=L" or "+t=~L". */
      traverse_expanded_symlink = OPTION(TRUE);
    else
      opt_error_msg("Expected one of 'ML'", options);

    negate = save_negate;
  }
      }
      else
      {
  traverse_mp = OPTION(TRUE);
  if (VersionLevel <= 145) traverse_mp = OPTION(FALSE);

  /* Changing the meaning of +t was an agonizing decision but was
     necessary to allow +t=<type> specifying the types of directories
     and/or filesystems to traverse: */
  if (VersionLevel >= 147)
  {
    static Boole warned = FALSE;
    if (! warned)
    {
      fprintf(stderr, "\
\n\
%s: Warning: The meaning of +t has changed as of release 1.47 (May 2000)\n\
Specifying +t under release 1.47 causes file-systems to be traversed whereas\n\
before it inhibited traversal.  Moreover, as of release 1.48 (March 2002)\n\
you should specify '+t=M' for traversing mount-points and '+t=L' for\n\
traversing expanded symbolic-links.\n\
\n\
", Progname);
      warned = TRUE;
    }
  }
      }
      break;

    case 'w': /* Warning suppression */
      warning_suppress = OPTION(TRUE);
      /* TBD: 'w' will eventually take a mask argument and maybe have
   its logic reversed? */
      break;

    case 'W': /* Watch progress */
      watch_progress = OPTION(TRUE);
      /* TBD: 'W' might eventually take an argument to specify report
   interval or report type (e.g. number, spinning wheel). */
      if (watch_progress)
      {
  if (defaultSigHandler == NULL)
    defaultSigHandler = signal(WATCH_SIGNAL, watchSigHandler);
  else
    (void) signal(WATCH_SIGNAL, watchSigHandler);
  if (ttyin  == NULL) ttyin  = fopen("/dev/tty", "r");
  if (ttyout == NULL) ttyout = fopen("/dev/tty", "w");
  if (ttyin == NULL || ttyout == NULL)
  {
    fprintf(stderr, "%s: Warning: Unable to open /dev/tty\n" , Progname);
    if (ttyin == NULL) ttyin = stdin;
    if (ttyout == NULL) ttyout = stderr;
  }
      }
      else
      {
  if (defaultSigHandler != NULL)
    (void) signal(WATCH_SIGNAL, defaultSigHandler);
      }
      break;

    case 'z': /* Zero-out meaningless stat info */
      zero_st_info = OPTION(TRUE);
      zero_st_info_specified = TRUE;  /* Command-line args take precedence */
      zero_st_mask = 0xffffffff;  /* Mask everything is the default */
      /* Look for and read optional/undocumented bit-mask: */
      if (*options == '=')
      {
  char *tmp;
  /* Since never documented using any other syntax, '=' is required
     and no subsequent options may follow: */
  options++; tmp = options;
  zero_st_mask = strtoul(options, &options, 0);
  if (*options != CNULL || tmp == options)
    opt_error_msg("Invalid bit-mask", options+1);
      }
      break;

    case 'M': /* MUNGE dates and file names */
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      munge++;  /* Go to next munge-level */

      /* Modification and change dates have interest for symbolic links,
   but access times are not interesting because stat()ing a symbolic
   link causes its access time to change, thus levels 2 and 4 employ
   +F*T{~l}.  Nevertheless, leave L in level 2's +G format in case -L
   is explicitly specified. */
      {
  char *g_opt;
  char *n_opt = "+NFL";
  if (munge == 1)
  {
    g_opt = "+Gmn";
  }
  else if (munge == 2)
  {
    g_opt = "+Gan";
  }
  else if (munge == 3)
  {
    g_opt = "+Gcn";
  }
  else if (munge == 4)  /* undocumented */
  {
    g_opt = "+G'chdate -m %m -a %a%n'";
    n_opt = "+NF";
  }
  else
  {
    g_opt = "";
    n_opt = "";
  }

  /* Defer to any previous G option from the command-line: */
  if (!G_option) { do_options(g_opt); G_option = FALSE; }

  /* Defer to any previous N option from the command-line: */
  if (!N_option) { do_options(n_opt);  N_option = FALSE; }

  /* Do not defer to any previous T option, as all munge levels imply
     that we want ISO8601 dates: */
  do_options("+TI");  /* Tf_ISO8601_DATE */
      }

      /* Additional options may follow any MUNGE command */
      break;

    case 'U': /* Generate Untouch script commands */
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      untouch++;
      quotaling = FALSE;  /* Untouch negates quotaling/stamping  */
      stamping = FALSE;
      untouch_setup(1);
      break;

    case 'f': /* Specify field separtor */
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      fielding = TRUE;
      separator = *options++;
      if (separator == CNULL)
  opt_error_msg("Missing character", options);
      break;

    case 'G': /* Specify the general format */
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      G_option = TRUE;  /* Command-line args take precedence */
      G_format = options;
      options = ""; /* No more options */
      break;

    case 'N': /* Specify the name format */
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      N_option = TRUE;  /* Command-line args take precedence */
      N_format = options;
      options = ""; /* No more options */
      break;

    case 'T': /* Specify the time & date format */
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      T_option = TRUE;  /* Command-line args take precedence */
      T_format = options;
      options = ""; /* No more options */
      break;

    case 'E': /* Exclude file names filter */
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      filtering = TRUE;
      exc_filtering = TRUE;
      if (*options == CNULL)
  opt_error_msg(MISSING_FILTER, options);
      else if (exc_file_count >= MAX_EXC_FILTER)
  opt_error_msg(TOO_MANY_FILTERS, options);
      else
  exc_file_filter[exc_file_count++] = options;
      options = ""; /* No more options */
      break;

    case 'e': /* Exclude directory names filter */
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      filtering = TRUE;
      exc_filtering = TRUE;
      if (*options == CNULL)
  opt_error_msg(MISSING_FILTER, options);
      else if (exc_dir_count >= MAX_EXC_FILTER)
  opt_error_msg(TOO_MANY_FILTERS, options);
      else
  exc_dir_filter[exc_dir_count++] = options;
      options = ""; /* No more options */
      break;

    case 'I': /* Include file names filter */
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      filtering = TRUE;
      inc_filtering = TRUE;
      if (*options == CNULL)
  opt_error_msg(MISSING_FILTER, options);
      else if (inc_file_count >= MAX_INC_FILTER)
  opt_error_msg(TOO_MANY_FILTERS, options);
      else
  inc_file_filter[inc_file_count++] = options;
      options = ""; /* No more options */
      break;

    case 'i': /* Include directory names filter */
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      filtering = TRUE;
      inc_filtering = TRUE;
      if (*options == CNULL)
  opt_error_msg(MISSING_FILTER, options);
      else if (inc_dir_count >= MAX_INC_FILTER)
  opt_error_msg(TOO_MANY_FILTERS, options);
      else
  inc_dir_filter[inc_dir_count++] = options;
      options = ""; /* No more options */
      break;

    case 'F': /* Filter expression */
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      filtering = TRUE;
      {
  char *filter_ns = strdup_ns(options);
  if (*filter_ns == '*')
  {
    exc_filtering = TRUE;
    if (fexpr_ANDcount < MAX_FEXPR_FILTER)
      fexpr_ANDfilter[fexpr_ANDcount++] = filter_ns;
    else
      opt_error_msg(TOO_MANY_FILTERS, options);
  }
  else
  {
    inc_filtering = TRUE;
    if (fexpr_ORcount < MAX_FEXPR_FILTER)
      fexpr_ORfilter[fexpr_ORcount++] = filter_ns;
    else
      opt_error_msg(TOO_MANY_FILTERS, options);
  }
      }
      options = ""; /* No more options */
      break;

    case 'Q': /* Quotals */
      /* In version 1.45, +Q meant to quote characters, in version 1.47
   +Q was deprecated, and in version 1.48 +Q was changed to quotal, */
      if (VersionLevel <= 147)
  plus_q = OPTION(TRUE);
      else
      {
  quotaling = OPTION(TRUE);
  if (quotaling)
  {
    untouch = 0;  /* Quotaling negates untouch/stamping */
    stamping = FALSE;
    /* Don't disguise any IDs unless specifically requested: */
    if (!envGetBoole("ELS_MASK_ID"))
      MaskId = FALSE; /* override 149's default behavior */
  }
      }
      break;

    case 'S': /* Select a Stamp format */
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      stamping = TRUE;
      untouch = 0;  /* Stamping negates untouch/quotaling */
      quotaling = FALSE;

      stamp_option = options;
      stamp_setup(1);
      options++;
      break;

    case 'v': /* Give version and bits of information: */
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      if (*options == '=' || isDigit(*options))
      {
  if (Iarg != 1 || options != Current_Arg+2)
    opt_error_msg("If specified, +v must be first argument", options);
  if (*options == '=') options++;
  VersionLevel = strToVer(options, &options);
  if (*options != CNULL)
    opt_error_msg("Invalid version", options+1);
  VersionLevelArg = TRUE;
      }
      else
      {
  puts(VersionID);
  if (verboseLevel > 0)
  {
    printf("  Compiled ELS version: %d\n", MaxVersionLevel);
    printf("  Running ELS version: %d\n", VersionLevel);
    show_options();
    printf("\n");
  }
  if (verboseLevel > 0)
  {
    printf("  Max dir name: %d\n", MAX_DNAME-1);
    printf("  Max file name: %d\n", MAX_FNAME-1);
    printf("  sizeof char,short,int,long,llong: %d,%d,%d,%d,%d\n",
     ELS_SIZEOF_char, ELS_SIZEOF_short, ELS_SIZEOF_int,
     ELS_SIZEOF_long, ELS_SIZEOF_llong);
    printf("  sizeof uid_t,gid_t,time_t,tm_year: %d,%d,%d,%d\n",
     ELS_SIZEOF_uid_t, ELS_SIZEOF_gid_t,
     ELS_SIZEOF_time_t, ELS_SIZEOF_tm_year);
    printf("  sizeof st_ino,st_size,st_nlink,st_blocks: %d,%d,%d,%d\n",
     ELS_SIZEOF_st_ino, ELS_SIZEOF_st_size, ELS_SIZEOF_st_nlink,
#   if defined(ELS_SIZEOF_st_blocks)
     ELS_SIZEOF_st_blocks
#   else
     -1
#   endif
     );
    printf("\n");
  }
  if (verboseLevel > 1)
  {
    char *f = " %10s: %d, %signed\n";
# define SHOW_TYPE(name,type) \
    printf(f, name, (int)sizeof(type), \
     (type)-2 < (type)2 ? "s" : "uns")
      SHOW_TYPE("Uint32",     Uint32);
    SHOW_TYPE("Uint64",     Uint64);
    SHOW_TYPE("size_t",     size_t);
    SHOW_TYPE("uid_t",      uid_t);
    SHOW_TYPE("gid_t",      gid_t);
    SHOW_TYPE("time_t",     time_t);
    SHOW_TYPE("ELS_time_t", ELS_time_t);
# if defined(MARKB_PREFERENCES) && \
    (SUNOS >= 50700 || LINUX >= 20400 || DARWIN >= 60000)
      /* Additional stuff I'm currious about: */
      printf("\n");
    SHOW_TYPE("dev_t", dev_t);
    SHOW_TYPE("ino_t", ino_t);
    SHOW_TYPE("mode_t", mode_t);
    SHOW_TYPE("nlink_t", nlink_t);
    SHOW_TYPE("off_t", off_t);
    /*SHOW_TYPE("timestruc_t", timestruc_t);*/
#   if !defined(LINUX)
    SHOW_TYPE("blksize_t", blksize_t);
#   endif
    SHOW_TYPE("blkcnt_t", blkcnt_t);
# endif
    printf("\n");
  }
  if (verboseLevel > 2)
  {
    printf("  Debug,ArgOrder,TruncateName,MaskId: %d,%d,%d,%d\n",
     Debug, ArgOrder, TruncateName, MaskId);
    printf("\n");
  }
  exit(USAGE_ERROR);
      }
      break;

    case 'V': /* Verbose level */
      verboseLevel++; /* Go to next verbose level */
      if (negate) verboseLevel = 0;
      break;

    case 'X': /* Execute mode */
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      execute_mode = TRUE;
      execute_sts_check = FALSE;  /* No status checking is the default */
      execute_sts_good = 0;
      /* +X must always use names quoted using +q whenever unusual file names
   occur (-b and -Q are inadequate for these purposes!): */
      do_options("+q");
      do_options("-~b~Q");
      /* Detect the following, otherwise unpleasant surprises may occur: */
      if (!First_listing)
  errorMsgExit("+X must precede any file name arguments", USAGE_ERROR);
      /* Look for and read optional status check value: */
      if (*options == '=' || isDigit(*options))
      {
  char *tmp;
  execute_sts_check = TRUE;
  /* Originally status could come imbedded with other args, e.g. +X0V,
     so allow it as well as +X=0V or +X=0 +V */
  if (*options == '=') options++;
  tmp = options;
  execute_sts_good = strtoul(options, &options, 0);
  if (options == tmp)
    opt_error_msg("Invalid return code", options+1);
  if (execute_sts_good > 255)
    /* ??? Maybe issue warning if > 255 ??? */
    execute_sts_good &= 0xff;
      }
      break;

    case 'Z': /* Set zonename */
      if (negate) opt_error_msg(NON_NEGATABLE, options);
      if (*options == '=')
      {
  options++;
  Time_Zone = get_validTZ(options);
  set_currentTZ(Time_Zone);
  options = ""; /* No more options */
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
      opt_error_msg("Unrecognized '+' option", options);
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

  else if (strcmp_ci(options, "CCaseMode") == 0 ||
     strcmp_ci(options, "ClearCaseMode") == 0)
  {
    CCaseMode = OPTION(TRUE);
  }

  else if (strcmp_ci(options, "GTarStyle") == 0)
  {
    /* Modify ELS behavior to mimic GNU "tar tv" listing: */
    GTarStyle = OPTION(TRUE);
  }

  else if (strcmp_ci(options, "Tar5Style") == 0)
  {
    /* Modify ELS behavior to mimic Sys5 "tar tv" listing: */
    Tar5Style = OPTION(TRUE);
  }

  else if (strcmp_ci(options, "FirstFound") == 0)
  {
    /* List first occurrence found then exit (used to locate hierarchies
       containing at least one or more files with desired properties): */
    FirstFound = OPTION(TRUE);
  }

  else if (strcmp_ci(options, "OncePerDir") == 0)
  {
    /* List once per directory (used to locate directories containing one
       or more files with desired properties): */
    OncePerDir = OPTION(TRUE);
  }

  else if (strncmp_ci(options, "DirDepth", strlen("DirDepth")) == 0)
  {
    /* Limit the recursion depth of directories: */
    if (negate) opt_error_msg(NON_NEGATABLE, options+1);
    options += strlen("DirDepth");
    if (*options == '=')
    {
      char *tmp;
      /* Since never documented using any other syntax, '=' is required
   and no subsequent options may follow: */
      options++; tmp = options;
      DirDepth = strtol(options, &options, 0);
      if (*options != CNULL || tmp == options)
  opt_error_msg("Invalid DirDepth", options+1);
    }
    else
    {
      opt_error_msg("Missing =integer", options);
    }
  }

  else if (strcmp_ci(options, "IncludeSS") == 0 ||
     strcmp_ci(options, "IncludeSnapShot") == 0)
  {
    /* Include listing of snapshot directories: */
    IncludeSS = OPTION(TRUE);
    IncludeSS_specified = TRUE;
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


void stamp_setup(int pass)
{
  if (pass == 1)
  {
    Boole stamp_uses_cksum = FALSE;
    if (*stamp_option == 'o')
    {
      do_options("+GCtPUG~%+8s~mn"); /* ~%+8s~ lists device files as X,Y */
      do_options("+NFL"); /* Nf_FULL_NAME Nf_LINK_PTR_NAME */
      do_options("+TI");  /* Tf_ISO8601_DATE */
      stamp_uses_cksum = TRUE;
    }
    else if (*stamp_option == 'O')
    {
      do_options("+GtPUG~%+8s~mn");  /* ~%+8s~ lists device files as X,Y */
      do_options("+NFL"); /* Nf_FULL_NAME Nf_LINK_PTR_NAME */
      do_options("+TI");  /* Tf_ISO8601_DATE */
    }
    else if (*stamp_option == 'f')
    {
      do_options("+GCtP~%+8s~mn"); /* ~%+8s~ lists device files as X,Y */
      do_options("+NFL"); /* Nf_FULL_NAME Nf_LINK_PTR_NAME */
      do_options("+TI");  /* Tf_ISO8601_DATE */
      stamp_uses_cksum = TRUE;
    }
    else if (*stamp_option == 'F')
    {
      do_options("+GtP~%+8s~mn");  /* ~%+8s~ lists device files as X,Y */
      do_options("+NFL"); /* Nf_FULL_NAME Nf_LINK_PTR_NAME */
      do_options("+TI");  /* Tf_ISO8601_DATE */
    }
    else if (*stamp_option == 'C' || *stamp_option == 'c')
    {
      /* Tracking directory dates is pointless for sorce code: */
      do_options("+d");

      do_options("+Gm  u~%+-6s~n"); /* ~%+-6s~ lists device files as X,Y */
      do_options("+NFL"); /* Nf_FULL_NAME Nf_LINK_PTR_NAME */
      do_options("+TI");  /* Tf_ISO8601_DATE */

      if (*stamp_option == 'C')
      {
  /* Exclude library, object, and dependency files: */
  do_options("+E*.[aod]");

  /* Exclude git directories: */
  do_options("+e.git");

  /* Exclude CVS directories: */
  do_options("+eCVS");
  do_options("+eCVSROOT");
  do_options("+ecvsroot");

  /* Exclude tilde backup files generated by Emacs: */
  do_options("+E*~");

#ifdef MARKB_PREFERENCES
  /* I like to use an older (and faster) version of GNU emacs that
     generates .~* backup files instead of *~: */
  do_options("+E.~*");

  /* I use .v directories with numbers on the end of my files to
     designate versions; e.g. .v/els.c.49, .v/els.c.50, .v/els.c.51,
     etc. thus I choose to ignore them: */
  do_options("+e.v");
#endif
      }
    }
    else
    {
      opt_error_msg("Expected one of 'CcFfOo'", stamp_option+1);
    }

    /* Stamp version 1.54 and above implies +q unless previously specified: */
    if (!plus_q_specified && VersionLevel >= 154)
      do_options("+q");

    /* Stamp version 1.53 *only* implies +z unless previously specified
       (rescinded in 1.54 because of possible adverse consequences): */
    if (!zero_st_info_specified && VersionLevel == 153)
      do_options("+z");

    /* Choose stamp's checksum algorithm (if not previously specified): */
    if (stamp_uses_cksum && cksum_type == CKSUM_UNSPECIFIED)
    {
      if (VersionLevel <= 148)
  /* Version 1.48 and below uses BSD by default: */
  do_options("+C=bsd");
      else
  /* Version 1.49 and above uses POSIX by default: */
  do_options("+C=posix");
    }
  }
  else
  {
    char *cksum_title = (cksum_size > 16 ? " Checksum " : "CKSum");
    char *cksum_dash  = (cksum_size > 16 ? "----------" : "-----");

    /*showCommandEnv("\t", "Stamp performed");*/
    showCommandEnv("# ", "Stamp performed");

    if (*stamp_option == 'o')
    {
      /* +GCtPUG~%+8s~mn +NFL */
      printf("\
%s T Mode   UID   GID    Size       Date          File\n\
%s - ----   ---   ---   ------ --------------- ----------\n\
", cksum_title, cksum_dash);
    }
    else if (*stamp_option == 'O')
    {
      /* +GtPUG~%+8s~mn +NFL */
      printf("\
T Mode   UID   GID    Size       Date          File\n\
- ----   ---   ---   ------ --------------- ----------\n\
");
    }
    else if (*stamp_option == 'f')
    {
      /* +GCtP~%+8s~mn +NFL */
      printf("\
%s T Mode    Size       Date          File\n\
%s - ----   ------ --------------- ----------\n\
", cksum_title, cksum_dash);
    }
    else if (*stamp_option == 'F')
    {
      /* +GtP~%+8s~mn +NFL */
      printf("\
T Mode    Size       Date          File\n\
- ----   ------ --------------- ----------\n\
");
    }
    else if (*stamp_option == 'C' || *stamp_option == 'c')
    {
      /* +Gm  u~%+-6s~n +NFL */
      printf("\
     Date        Owner    Size      File\n\
---------------  -----    -----  ----------\n\
");
    }
  }
  return;
}


void untouch_setup(int pass)
{
  if (pass == 1)
  {
    if (untouch == 1)
    {
      do_options("+G'untouch  %C %-8s %m  %N'");
    }
    else
    {
      do_options("+G'untouch  %C %-8s %m %a  %N'");
      do_options("+C=U"); /* Preserve access times (i.e. 'Unaccess') */
    }
    /* Use posix checksums if no algorithm was previously specified: */
    if (cksum_type == CKSUM_UNSPECIFIED) do_options("+C=posix");
    do_options("+q");   /* Use quoted names */
    do_options("+NF");    /* Use full names (no symlinks) */
    do_options("+TI");    /* Use Iso8601 dates */
    do_options("+F*T{r}");  /* List regular files only */
  }
  else
  {
    char *tz;
    printf("\
#!/bin/sh\n\
#\n\
");
    showCommandEnv("# ", "'untouch' shell script created");
    printf("\
# NB: Just because you are able to 'touch' a file doesn't necessarily mean\n\
# that you can 'untouch' it.  The ability to untouch a file may require\n\
# that you be the direct owner of the file or have super-user privileges.\n\
#\n\
# Save the following output into a script file so that it can then be run\n\
# at a later time in order to recover file dates following a 'touch':\n\
#\n\
");
    if (untouch == 1)
      printf("\n\
untouch()\n\
{\n\
   CKSUM=$1; SIZE=$2; DATE=$3; FILE=\"$4\"\n\
   [ -f \"$FILE\" ] && \\\n\
     [ `els +G%%m +TI \"$FILE\"` != $DATE ] && \\\n\
       [ `els +G%%s \"$FILE\"` = $SIZE ] && \\\n\
   [ `els +C=%s +G%%C \"$FILE\"` = $CKSUM ] && \\\n\
     chdate -m $DATE \"$FILE\"\n\
}\n", cksumTypeToName(cksum_type));
    else
      printf("\n\
untouch()\n\
{\n\
   CKSUM=$1; SIZE=$2; DATE=$3; ACCESS=$4; FILE=\"$5\"\n\
   [ -f \"$FILE\" ] && \\\n\
     [ `els +G%%m,%%-a +TI \"$FILE\"` != $DATE,$ACCESS ] && \\\n\
       [ `els +G%%s \"$FILE\"` = $SIZE ] && \\\n\
   [ `els +C=%s,U +G%%C \"$FILE\"` = $CKSUM ] && \\\n\
     chdate -m $DATE -a $ACCESS \"$FILE\"\n\
}\n", cksumTypeToName(cksum_type));

    /* Include TZ setting regardless of whether it's from the environment
       or from the command line, as the script file needs to be told: */
    /* FYI: root's TZ setting under SunOS5 is "".  Apparently this has
       no negative affect other than to cause the system TZ value to be used
       instead.  Perhaps in such cases this ambiguity should be suppressed!?
       Or should the TZ setting always be faithfully reported? */
    if ((tz = getenv("TZ")) != NULL && *tz != CNULL) {
      printf("\n"); printEnvVarSetSh("TZ", tz); printf("\n\n");
    }

    {
      char *cksum_title = (cksum_size > 16 ? "  Checksum" : "CKSum");
      char *cksum_line  = (cksum_size > 16 ? "----------" : "-----");
      if (untouch == 1)
  printf("\
#        %s Size           Date       File\n\
#        %s ----     ---------------  ----\n\
", cksum_title, cksum_line);
      else
  printf("\
#        %s Size           Date           Access      File\n\
#        %s ----     --------------- ---------------  ----\n\
", cksum_title, cksum_line);
    }
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
Usage: %s [-~aAbcCdFgGhilLnopQrRstuU1] [+~cdhHlqQRvVwz45] \\\n\
  [+C=[BSPCU]] [+M[M[M]]] [+o=outfile] [+~t=[~ML]] \\\n\
  [+Z=zonename] [+f char] [+GTN format] \\\n\
  [+IEie pattern] [+F fexpr] \\\n\
  [+S[CcOoFf]] [+U] [+W] [+X status] files\n\
\n\
", Progname);

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
\n\
LS -- STANDARD OPTIONS:\n\
\n\
    -a: List all files including . and ..\n\
    -A: List all files except . and ..\n\
    -b: List non-graphic characters using octal notation\n\
    -c: List time of last status change(SYS5, ELS),\n\
        List time of last status change and sort(BSD)\n\
    -C: List multi-columns (unimplemented)\n\
    -C: Disable commas in file sizes and colors in filenames (implemented-rwm)\n\
    -d: List directories as files, but don't list their contents\n\
    -F: Mark files\n\
    -g: List GIDs(BSD), long listing omitting UIDs(SYS5), ignored(ELS)\n\
    -G: Don't list GIDs(ELS)\n\
    -h: List size in human readable format scaled by powers of 1024\n\
  If a second -h (i.e. -hh), then scaled by powers of 1000\n\
    -i: List inode number\n\
    -l: Long listing using BSD, SYS5, or ELS semantics\n\
    -L: List actual file rather than symbolic link\n\
    -n: Long listing using numeric UIDs and GIDs(SYS5, ELS)\n\
    -o: Long listing omitting GIDs(SYS5, ELS)\n\
    -p: Mark directories\n\
    -Q: List non-graphic characters using double-quotes and C-style notation\n\
    -r: Reverse the sort\n\
    -R: Recursively list contents of all directories\n\
    -s: List size in KBytes(BSD, ELS) or in blocks(SYS5)\n\
    -t: Sort files according to time\n\
    -u: List time of last access\n\
    -U: List unsorted(ELS)\n\
    -1: List single column\n\
     ~: If a tilde precedes any of the above options then that option is\n\
  reset, effectively causing it to be canceled.\n\
\n\
");

    fprintf(out, "\
\n\
Enhanced LS -- MISCELLANEOUS:\n\
\n\
    +H: Give HELP piped through PAGER\n\
    +l: Mimic /bin/ls behavior\n\
    +4: Use BSD semantics\n\
    +5: Use SYS5 semantics\n\
    +c: Output ELS options and environment settings used to produce listing\n\
  (useful for documenting how a particular listing was generated)\n\
    +d: Don't list directories as files, but list their contents (has the\n\
  opposite effect of -d)\n\
    +h: Allow +X to act upon symbolically linked files\n\
    +q: Quote unusual and troublesome file names (besides doing a more\n\
  thorough job than either -b or -Q, this option also quotes special\n\
  and non-graphic characters so that the file name can subsequently be\n\
  used as an argument for most Unix commands; thus, the +q option is\n\
  automatically implied whenever using the +X option).\n\
    +Q: Display Quotals (quota+total) for indicated hierarchy or files.\n\
  The quotal option will list the total size and number of files\n\
  owned by each user/UID and group/GID.  For example, to display\n\
  quotals for all files in /usr hierarchy:\n\
    els +Q -AR /usr\n\
    +R: Recursively list contents of all directories (same as -R)\n\
    +v: Print els version and information\n\
    +V: Verbose mode\n\
    +w: Suppress warnings about unreadable files, etc.\n\
    +z: List volatile file information as zero (volatile file information is\n\
  defined as those values that change whenever a file is listed or\n\
  those values that aren't preserved after a hierarchy gets copied\n\
  or restored; for example, a directory or a symbolic link changes its\n\
  access time whenever being listed; a symbolic link changes its\n\
  modification time and/or ownership after being copied; a directory\n\
  doesn't always preserve its size after being copied).  Also, since\n\
  volatile file information can change over time, this option is\n\
  particularly useful when stamping a hierarchy (see +S option), or\n\
  when comparing the listings of two very similar hierarchies.\n\
     ~: If a tilde precedes any of the above options then that option is\n\
  reset, effectively causing it to be canceled.\n\
\n\
    +C: Specify checksum algorithm (B|bsd, S|sysv, P|posix, or C|crc32).\n\
  Additionally, if U|unaccess is specified then each file's access\n\
  time will be preserved (if possible).  'bsd' is similar to\n\
  /usr/ucb/sum under SunOS5/Solaris or '/usr/bin/sum -r' under\n\
  Linux; 'sysv' is similar to /usr/bin/sum under SunOS5/Solaris or\n\
  '/usr/bin/sum -s' under Linux; 'posix' is similar to\n\
  '/usr/bin/cksum' under SunOS5/Solaris, Linux, HPUX10+, etc; and\n\
  'crc32' is the algorithm used by ZLIB et al.  For example, to\n\
  recursively list checksums of all files:\n\
    els +C=bsd +z +GCSN -L -RA  # similar to /usr/ucb/sum\n\
    els +C=sysv +5l +z +GCSN -L -RA # similar to /usr/bin/sum\n\
    els +C=posix +z +GCsN -L -RA  # similar to /usr/bin/cksum\n\
    els +C=crc32 +z +GCsN -L -RA  # crc32 algorithm\n\
    els +C=posix,U +z +GCsN -L -RA  # preserve access times\n\
    els +C=PU +z +GCsN -L -RA # same as 'posix,unaccess'\n\
\n\
    +M: Macro 1 for listing ISO8601 *modification* dates of all files\n\
  except directories (same as: els +Gmn +NFL +TI +d).  For example, to\n\
  list the 30 most recently modified source code files in a hierarchy:\n\
    els +M -RA +I'*.{c,h,cc,hh}' | sort | tail -30\n\
   +MM: Macro 2 for listing ISO8601 *access* dates of all files except\n\
  directories and symbolic links (same as: els +Gan +NF +TI +d \\\n\
  +F'T{~l}').  For example, to list the 100 most recently accessed \n\
  executable files under the /usr hierarchy:\n\
    els +MM -RA +F'P{+x}' /usr | sort | tail -100\n\
  +MMM: Macro 3 for listing ISO8601 *change* dates of all files and\n\
  directories (same as: els +Gcn +NFL +TI).  For example, to list\n\
  the 100 most recently changed files and directories under the /usr\n\
  hierarchy:\n\
    els +MMM -RA /usr | sort | tail -100\n\
\n\
    +o: Specify file for output, e.g.:  els -laR +o=output.dat\n\
\n\
    +t: Traverse mount-points(M) and/or expanded symbolic-links(L) during\n\
  recursive listings.  For example, to traverse any mount-point (i.e.\n\
  a different file-system) encountered during a recursive listing:\n\
    els -laR +t=M\n\
  To traverse both mount-points and expanded symbolic-links:\n\
    els -laR -L +t=ML\n\
  Note: '+t=L' is effective only if '-L' is also specified, as '-L'\n\
  is what enables expansion of symbolic-links.  '+t=M' must also be\n\
  specified with '+t=L' (i.e. +t=ML), if you want to traverse expanded\n\
  symbolic-links that vector-off onto different mount-points.  By\n\
  default all traversing is disabled unless requested.  To disable all\n\
  traversal specify '+t=~L~M' (or the logical equivalent '+~t=LM').\n\
\n\
    +Z: Specify timezone to be used in place of current TZ setting.\n\
  The timezone should be in the form of:\n\
      All OSes:   STDoffset[DST[offset][,rule]]\n\
  Additionally, if your host provides zoneinfo then you can also\n\
  use names from the appropriate zoneinfo directory:\n\
      SunOS/Solaris:    /usr/share/lib/zoneinfo\n\
      Linux2, Darwin, FreeBSD:  /usr/share/zoneinfo\n\
      OSF1:     /etc/zoneinfo\n\
  For example:\n\
      els -la +Z=EST5EDT +TIZ (Available on *all* OSes)\n\
      els -la +Z=US/Eastern +Tv (Available on OSes with zoneinfo)\n\
\n\
");

    fprintf(out, "\
\n\
Enhanced LS -- FORMATTING:\n\
\n\
    +f: Field separator character, e.g.:  els -la +f: +TI\n\
\n\
    +G: GENERAL format, e.g.:  els +G~tp~lusmn +NfL (same as els -l)\n\
  i:  inode number\n\
      t,T:  type of file in alpha(t), symbolic(T)\n\
      p,P:  permission in alpha(p), numeric(P)\n\
  M:  permission in chmod format (e.g. u=rwxs,g=x,o=x)\n\
  A:  ACL indicator '+' if present\n\
  l:  link count\n\
      u,U:  UID in alpha(u), numeric(U)\n\
      g,G:  GID in alpha(g), numeric(G)\n\
      o,O:  owner in alpha(o == u:g), numeric(O == U:G)\n\
      s,S:  size in bytes(s), blocks(S)\n\
        h:  size scaled by powers of 1000 with one of kmgtpezy\n\
        H:  size scaled by powers of 1024 with one of KMGTPEZY\n\
    m,a,c:  time modified(m), accessed(a), status changed(c)\n\
      (time displayed using +T format)\n\
      n,N:  file name(n), full file name(N)\n\
      (name displayed using +N format)\n\
    d,f,F:  directory name(d), file name(f), full file name(F)\n\
  L:  Symbolic link target prefaced by '->'\n\
  C:  checksum (performed on regular files only using\n\
      algorithm and/or flags specified by +C option)\n\
\n\
    +T: TIME format, e.g.:  els -la +T^rD (give age of file in days)\n\
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
    +N: NAME format, e.g.:  els -laR +NF\n\
       ^q:  Modifier for quoting unusual file names (same as +q)\n\
  F:  Full file name (same as +Nd/f)\n\
  d:  Directory name\n\
  f:  File name\n\
  l:  Symbolic link target\n\
  L:  Symbolic link target prefaced by '->'\n\
\n\
    +G, +T, +N format controls\n\
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
  E.g.: els +G%%10u%%-10gN (print the uid right justified and the gid\n\
  left justified in two 10 character fields followed by the file name)\n\
\n\
  If a string occurs within an inner set of quotes then the string\n\
  is output verbatim (except for any directives within the inner\n\
  quotes prefaced by a %%).  Thus, the following are equivalent:\n\
\n\
  +T\"'DATE: '^N%%M/D/Y\"    # M/D/Y in outer quotes (%% optional)\n\
  +T'\"DATE: \"^NM/%%D/Y'    # M/D/Y in outer quotes (%% optional)\n\
  +T'\"DATE: ^N%%M/%%D/%%Y\"'  # M/D/Y in INNER quotes (%% REQUIRED)\n\
\n\
");

    fprintf(out, "\
\n\
Enhanced LS -- FILTERING:\n\
\n\
    +I: Include specified files, e.g.:  els -laR +I\"*.{c,h,cc,hh}\"\n\
    +E: Exclude specified files, e.g.:  els -laR +E\"*.o\"\n\
    +i: Include specified directories, e.g.:  els -laR +i\"[a-m]*\"\n\
    +e: Exclude specified directories, e.g.:  els -laR +e\".git\"\n\
\n\
    +F: FILTER EXPRESSION\n\
\n\
  A filter expression (fexpr) consists of one or more of the\n\
  following filter types separated by Boolean operators:\n\
    A{...} -- Access Filter\n\
    T{...} -- Type Filter\n\
    P{...} -- Permission Filter\n\
    Q{...} -- Quantity Filter\n\
    U{...} -- Unusual Filter\n\
    c{...} -- clearcase Filter\n\
    l{...} -- link Filter\n\
\n\
  Each filter type consists of one or more terms separated by\n\
  Boolean operators.  The following lists each term appropriate\n\
  for the associated filter type:\n\
      Access Filter Terms:\n\
    rwxe: read, write, execute(x), or existence(e)\n\
      Type Filter Terms:\n\
    rf: regular file (r and f are synonymous)\n\
    dcb: directory, char device, block device\n\
    plsD: pipe/fifo, symbolic link, socket, Door\n\
    S: Special/device file (same as 'b|c')\n\
      Permission Filter Terms:\n\
    value: octal value <= 07777\n\
    ugo: user field, group field, other field\n\
    a: all fields (same as 'ugo')\n\
    +: test if indicated field(s) contain any attribute(s)\n\
    -: test if indicated field(s) missing any attribute(s)\n\
    =: test if indicated field(s) match attribute(s) exactly\n\
    rwx: read attribute, write attribute, execute attribute\n\
    s: setuid/setgid attribute (applies to u and/or g fields)\n\
    t: sticky attribute (applies to u field)\n\
    l: mandatory lock attribute (applies to g field)\n\
      Quantity Filter Terms:\n\
    iAlugs: inode, ACL count, link count, uid, gid, size\n\
    mac: time modified, accessed, status changed\n\
    == != ~= : equals, not equals, not equals\n\
    > >= : greater than, greater than or equals\n\
    < <= : less than, less than or equals\n\
    value: positive integer to be compared against\n\
    YMWD: Years, Months, Weeks, Days\n\
    hmsc: hours, minutes, seconds, clock\n\
      Unusual Filter Terms:\n\
    t: unusual type (i.e. !regular & !directory & !symlink)\n\
    p: unusual permissions (i.e. o+w & !+t & !symlink |\n\
       setuid | setgid | mandatory_locking)\n\
    P: unusual permissions (i.e. access(o) > access(g) |\n\
       access(o) > access(u) | access(g) > access(u))\n\
    A: ACL_count > 0\n\
    l: link_count > 1\n\
    u: nobody/noaccess UID (60001, 60002, 65534, or 4294967294)\n\
       or unassigned UID (i.e. not listed in /etc/passwd)\n\
    g: nobody/noaccess GID (60001, 60002, 65534, or 4294967294)\n\
       or unassigned GID (i.e. not listed in /etc/group)\n\
    mac: modification, access, change time is in the future\n\
    n: name containing unusual or troublesome characters\n\
    N: full pathname containing unusual or troublesome characters\n\
    L: symbolic link pointing to non-existent file\n\
    s: sparse file (i.e. partially filled w/data)\n\
    G: General tests (i.e. perform all the above unusual tests)\n\
    S: Security related tests (i.e. !directory & o+w |\n\
       directory & o+w & !+t | setuid | setgid | device_file)\n\
      clearcase Filter terms:\n\
    e: VOB element\n\
    p: VIEW private\n\
      link Filter terms:\n\
    e: symbolic link target exists\n\
    t: symbolic link traverses filesystem and/or clearcase VOB\n\
\n\
  Boolean operators are as follows:\n\
    ! or ~ -- Boolean NOT (evaluated first)\n\
    & or , -- Boolean AND (evaluated second)\n\
    | or : -- Boolean OR (evaluated last)\n\
  (The reason for allowing multiple symbols for Boolean operations is\n\
  that characters such as '!' have special meaning within the shell\n\
  and are awkward to use.  Also, characters such as ',' can improve\n\
  readability when grouping lists of terms, e.g.: 'u+rw,g-rw,o-rw'\n\
  is more readable than 'u+rw&g-rw&o-rw'.)\n\
\n\
  The syntax for each term is based upon its associated filter type.\n\
  In all cases, any filter having multiple terms must use Boolean\n\
  operators to separate each of its terms.  The 'Access' and 'Type'\n\
  Filters are the simplest cases when it comes to syntax, as each of\n\
  their terms consists of a single letter.\n\
\n\
  The syntax for each term of the permission filter is very similar to\n\
  Unix's chmod command with the following exceptions: if the left-hand\n\
  side of a term is blank then it is taken to mean ANY instead of ALL\n\
  fields; the symbol '+' is taken to mean HAS instead of GIVE; the\n\
  symbol '-' is taken to mean MISSING instead of TAKE; and the symbol\n\
  '=' is taken to mean MATCH EXACTLY instead of SET EXACTLY.  Thus,\n\
  the filter '+FP{+rw}' means ANY FIELD HAVING READ-WRITE PERMISSION,\n\
  while the filter '+FP{a=}' means ALL FIELDS HAVING EXACTLY NO\n\
  PERMISSIONS.\n\
\n\
  The syntax for each term of the quantity filter consists of a\n\
  single letter and an integer quantity separated by a comparison\n\
  operator.  Moreover, if the quantity being compared represents the\n\
  file modification, access, or status change time (i.e. one of\n\
  'mac'), then the integer quantity must be followed by a qualifier\n\
  designating Years, Months, Weeks, Days, hours, minutes, seconds,\n\
  or clock (i.e. one of 'YMWDhmsc').\n\
\n\
  Examples:\n\
      Recursively list regular files in /usr/bin that are\n\
      setuid/setgid having a UID/GID of less than 10:\n\
    els -laR +F'T{r}&P{u+s|g+s}&A{x}&Q{u<10|g<10}' /usr/bin\n\
\n\
      Recursively list all non-directory files that share a common\n\
      inode (i.e. hard-link) in /etc and /dev:\n\
    els +GilN +F'Q{l>1}&T{~d}' -R /etc /dev | sort\n\
\n\
      Recursively list all files modified on or after April 1, 1999\n\
      belonging to either the user 'markb' or the group 'projectx':\n\
    els -laR +F'Q{m>=19990401}&Q{u=markb|g=projectx}'\n\
\n\
      Display all files where g or o have more permission than u:\n\
    els -laR +F'P{u-w,+w}|P{u-x,+x}|P{u-r,+r}' +NF\n\
\n\
      Display all files having 'unusual' types or permissions\n\
      (e.g., world-writable or sticky files excluding symlinks,\n\
      setuid/setgid/locking files excluding symlinks and dirs,\n\
      or any file type other than regular/directory/symlink):\n\
    els -laR +NF +F'(P{o+w:+t},T{~l}) | (P{+s,+x:+l},~T{d:l}) |\n\
      (~T{r:d:l})'\n\
\n\
      List all files greater than 100K bytes that have not been\n\
      accessed in over half a year (.5Y) and were modified over one\n\
      year ago (1Y):\n\
    els +GsamN +T^rD +d -AR +F'Q{s>=100000 & a>182D & m>365D}'\n\
    els +GsamN +T^rD +d -AR +F'Q{s>=100K & a>.5Y & m>1Y}'\n\
\n\
      List all files having troublesome characters in their name:\n\
    els -laR -Q +NF +F'U{N}'\n\
\n\
");

    fprintf(out, "\
\n\
Enhanced LS -- UTILITIES:\n\
\n\
    +S: STAMP: The output from this option is used for recording the state\n\
  of a hierarchy for future comparison to determine any changes.\n\
\n\
  C:  Stamp a source code hierarchy excluding *.[aod], *~\n\
      and .git files, e.g.:  els +SC -R /home/myfiles/src\n\
  c:  Same as +SC, except nothing is excluded\n\
\n\
  O:  Stamp an operating system hierarchy (no files are excluded and\n\
      more detail given), e.g.:  els +SO +z -R  /  /usr /var /opt\n\
      (Note that +z is recommended to zero volatile data)\n\
  o:  Same as +SO, except checksumming is also performed, e.g.:\n\
      els +C=posix +So +z -R  /  /usr /var /opt\n\
\n\
  F:  Stamp a file hierarchy (same as +SO except that UIDs and GIDs\n\
      are excluded), e.g.:  els +SF +z -R /etc /dev /usr /var\n\
      (Note that +z is recommended to zero volatile data)\n\
  f:  Same as +SF, except checksumming is also performed\n\
\n\
    +U: UNTOUCH: Create a script which can be saved into a file for later\n\
  recovery of modification dates following a /bin/touch, e.g.:\n\
      els +U -R * > untouch  # Create untouch script file\n\
      /bin/touch *           # Clobber modification dates\n\
      sh untouch             # Recover modification dates\n\
\n\
    +W: WATCH: Watch the progress of how many files have been examined.\n\
  This option is best used in conjunction with +F filtering when\n\
  output is sparse and there are numerous files being examined or\n\
  when stdout is being redirected to a file, e.g.:\n\
      els +W +F'Q{s=0}' +NF -laR\n\
      els +W -laR > /tmp/output\n\
  Additionally, +W will intercept SIGQUIT (i.e. ^\\) while listing\n\
  and display the name of the directory currently being processed.\n\
\n\
    +X: EXECUTE: The output of the +G format will be executed instead of\n\
  listed for each file.  If +X is followed by a number then els will\n\
  terminate whenever the command returns a status of some other value,\n\
  otherwise the return status will be ignored.  Additionally, if +V is\n\
  specified then each command will be echoed before being executed.\n\
\n\
  E.g., fix the mode of any .c, .h, .cc, .hh file having 'x' mode set\n\
  in any of its fields:\n\
      els +F'P{+x}' +I'*.{c,h,cc,hh}' +G'\"chmod a-x %%N\"' +XV -RA\n\
\n\
  E.g., fix the mode of any file having 'x' mode set in the 'u' field\n\
  but is missing from the 'g' or 'o' fields:\n\
      els +F'P{u+x,g-x|u+x,o-x}' +G'\"chmod go+x %%N\"' +X0 +V -RA\n\
\n\
");

    fprintf(out, "\
\n\
Enhanced LS -- SPECIAL FEATURES:\n\
\n\
    --version\n\
  Print els version and information (same as +v)\n\
    --CCaseMode, --ClearCaseMode\n\
  Mimic ClearCase behavior such as masking 'w' permissions, etc.\n\
    --GTarStyle\n\
  Mimic GNU 'tar tv' listings, e.g.:\n\
      els -lAR --GTarStyle\n\
    --Tar5Style\n\
  Mimic Sys5 'tar tv' listings, e.g.:\n\
      els -lAR --Tar5Style\n\
    --FirstFound\n\
  List first occurrence found then exit (used to locate hierarchies\n\
  containing one or more files with desired properties), e.g.:\n\
      els +F'T{r}&Q{a<30D}' -AR +NF --FirstFound /tmp\n\
    --OncePerDir\n\
  List once per directory (used to locate directories containing one\n\
  or more files with desired properties)\n\
    --DirDepth=N\n\
  Limit the recursion depth of directories to N\n\
    --IncludeSS, --IncludeSnapShot\n\
  Include listing of snapshot directories\n\
    --setenv:VARIABLE=VALUE\n\
  Create and set named environment variable to given value, e.g.:\n\
      els -laR +c --setenv:LC_ALL=fr_CA.ISO8859-1\n\
      els -laR +c --setenv:TZ=US/Mountain\n\
    --unsetenv:VARIABLE\n\
  Unset and delete named environment variable, e.g.:\n\
      els -laR +c --unsetenv:TZ\n\
\n\
");

    fprintf(out, "\
\n\
Enhanced LS -- ENVIRONMEMT:\n\
\n\
    PAGER: Name of pager program for displaying help text\n\
\n\
    ELS_LC_COLLATE, ELS_LC_TIME, LC_ALL, LC_COLLATE, LC_TIME:\n\
  The Posix LC_ALL environment variable supersedes both LC_COLLATE and\n\
  LC_TIME (e.g. SunOS5/Solaris and Linux behavior).  But ELS_LC_COLLATE\n\
  and ELS_LC_TIME further supersede this behavior as follows:\n\
\n\
  Collate/sorting locale determined as follows:\n\
     Use ELS_LC_COLLATE if defined, else use LC_ALL if defined,\n\
     else use LC_COLLATE if defined, else use 'C' locale.\n\
\n\
  Time locale determined as follows:\n\
     Use ELS_LC_TIME if defined, else use LC_ALL if defined,\n\
     else use LC_TIME if defined, else use 'C' locale.\n\
\n\
  ELS_TRUNCATE_NAME=1   - truncate annoyingly long user/group names\n\
  ELS_FS_COLOR=35;1     - show size in red\n\
  ELS_FT_COLORS=86400=32;1:6480000=32:15724800=33:3155760=33;2:-1=31;1:\n\
  ELS_FS_WIDTH=7        - minimize size width, increase for more width\n\
  ELS_HG_STATUS='hg status -mardui'  - add hg status\n\
  ELS_GIT_STATUS='git status -s --ignored --porcelain --untracked-files' - add git status\n\
  ELS_EXFAT=1           - ignore execution bit - should be auto determined\n\
\n\
");
  }

  if (more)
    pclose(out);

  return;
}


void analyze_options()
{
  char *cp;

#ifdef HAVE_LOCALE
  /* As of ELS 150 special listings use C locale, but others use whatever is
     defined.  Prior to ELS 150 everyone used C locale: */
  if (stamping || untouch > 0 || munge > 0 || G_option || VersionLevel < 150)
  {
    useLcCollate = FALSE;
    useLcTime = FALSE;
  }

  /* LC_ALL environment variable supersedes both LC_COLLATE and LC_TIME for
     both SunOS5 and Linux2.  But ELS also allows ELS_LC_COLLATE to supersede
     LC_ALL/LC_COLLATE and ELS_LC_TIME to supersede LC_ALL/LC_TIME. */

  if (useLcCollate)
  {
    char *lc;
    if ((lc = getenv("ELS_LC_COLLATE")) != NULL)
    {
      /* ELS_LC_COLLATE, if set, overrides all other system/envvar locale
   settings: */
      lc = setlocale(LC_COLLATE, lc);
      envUnset("LC_COLLATE");  /* Prevent +c reporting overridden envvar */
    }
    else
    {
      /* Use setlocale(..., "") to capture current setting: */
      lc = setlocale(LC_COLLATE, "");
      if (lc != NULL) envSetNameVal("ELS_LC_COLLATE", lc); /* For +c reporting */
    }
  }
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


  if (plus_c && !stamping && untouch == 0)
    showCommandEnv("# ", "Listing performed");

  if (stamping) stamp_setup(2);

  if (untouch > 0) untouch_setup(2);

  if (execute_mode)
  {
    if (!G_option) errorMsgExit("+X option requires +G option", USAGE_ERROR);
    /* Disallow +X if running as "ls": */
    if (strcmp(Progname, "ls") == 0) errorMsgExit("+X option disallowed", USAGE_ERROR);
  }


  /* When filtering without the +NF option being specified, dot-dirs can
     make for very sloppy output! */
  if (VersionLevel < 151)
    /* Note: This practice was started as of 1.49a1 and was disabled starting
       with 1.51a1 as it is can be misleading. */
    if (filtering) list_dotdir = FALSE;


  /* When quotaling or stamping ensure all files are considered unless
     "-~A" or "-~a" options were explicitly set on the command line: */
  if ((quotaling || stamping) && ! Aa_option)
    list_hidden = TRUE;

  /* As of ELS 1.60 and above "-A" is to be implied when FILTERING or using
     +X/+U utilities unless "-~A" or "-~a" options were explicitly set on
     the command line.  This prevents important files from being overlooked
     and keeps commands such as +I".cshrc*" from behaving unexpectedly: */
  if ((inc_filtering || execute_mode || untouch > 0) && ! Aa_option)
    if (VersionLevel >= 160) list_hidden = TRUE;   /* FUTURISTIC */

  /* FIXME: Should exc_filtering also set list_hidden?  But since +M levels 2
     and 4 have implied "+F*" exc_filtering (as well as untouch) then this
     would cause +M levels 2 and 4 to also imply -A as a side-effect! */


  /* root gets special treatment under BSD: */
  if (ANYBIT(Sem,SEM_BSD) && Whoami == 0) list_hidden = TRUE;


  /* The -g flag implies long listing for SYS5: */
  if (ANYBIT(Sem,SEM_SYS5) && g_flag) list_long = TRUE;


  /* Define Name and Time formats for a long listing, unless either +N
     or +T were explicitly defined on the command-line: */
  if (list_long)
  {
    /* Defer to any previous N option from the command-line: */
    if (!N_option)
    {
      do_options("+NfL"); /* Nf_FILE_NAME Nf_LINK_PTR_NAME */
      N_option = FALSE; /* reset deferrability */
    }

    /* Defer to any previous T option from the command-line: */
    if (!T_option)
    {
      if (ANYBIT(Sem,SEM_ELS))
  do_options("+Te");  /* Tf_ELS_DATE */
      else
  do_options("+Tl");  /* Tf_LS_DATE */
      T_option = FALSE; /* reset deferrability */
    }
  }


  /* Define Name and format for either --FirstFound or --OncePerDir,
     unless +N was explicitly defined on the command-line: */
  if (FirstFound || OncePerDir)
  {
    /* Defer to any previous N option from the command-line: */
    if (!N_option)
    {
      do_options("+NFL"); /* Nf_FULL_NAME Nf_LINK_PTR_NAME */
      N_option = FALSE; /* reset deferrability */
    }
  }


  /* Generate G_format if no implicit G_format given on the command-line: */
  if (!G_option && munge == 0)
  {
    static char cmd[32];
    if (list_long)
    {
      char *gfmt;
      char *uid, *gid;

      /* NOTE: SYS5 interprets the -g flag to mean "exclude the UID *and*
   perform a long listing", whereas BSD interprets the -g flag to mean
   "include GID *if* performing a long listing".

   It's also true that BSD has no -n or -o flag, however, just for fun
   let's also support -n and -o for BSD with the understanding that
   when combined -n and -o that the -g flag will always follow SYS5
   semantics. */

      if (list_long_numeric && list_long_omit_gid)
      {
  uid = "%-8U";
  gid = "";
      }
      else if (list_long_numeric)
      {
  uid = "%-8U";
  gid = "%-8G";
      }
      else if (list_long_omit_gid)
      {
  uid = "u";
  gid = "";
      }
      else
      {
  uid = "u";
  gid = "g";
      }

      if (ANYBIT(Sem,SEM_SYS5) || list_long_numeric || list_long_omit_gid)
      {
  /* Take -g flag to mean omit UID [sic] in this context: */
  if (g_flag) uid = "";
      }
      else if (ANYBIT(Sem,SEM_BSD))
      {
  /* Handle -g flag a la BSD: */
  if (!g_flag) gid = "";
      }

      /* Handle -G flag a la GNU: */
      if (G_flag) gid = "";

      /* If ACLs are supported on this platform, then scrunch the link-count
   so as to maintain columns following ACL '+'.  This can violate els'
   philosophy of having every item separated by a space by causing the
   link-count to butt-up against the '+' if an ACL is present and the
   link-count is greater than 99: */
      {
  Boole gfmt_with_acl;
# if defined(HAVE_ACL)
  /* ACLs are supported on this platform: */
  gfmt_with_acl = TRUE;
#   if defined(SUNOS)
  /* SunOS decides at run-time whether or not ACLs are supported: */
  if (osVersion < 50500) gfmt_with_acl = FALSE;
#   endif
# else
  /* ACLs are NOT supported on this platform: */
  gfmt_with_acl = FALSE;
# endif
  if (gfmt_with_acl)
  {
    if (fielding)
      gfmt = "+G%s%s~tpA~l%s%s%c%cn";
    else
      gfmt = "+G%s%s~tp%%+A%%+l~%s%s%c%cn";
  }
  else
    gfmt = "+G%s%s~tp~l%s%s%c%cn";
      }

      sprintf(cmd, gfmt,
        list_inode ? "i" : "",
        list_size_in_blocks ? "S" : "",
        uid,
        gid,
        list_size_human_2  ? Gf_SIZE_HUMAN_2 :
        list_size_human_10 ? Gf_SIZE_HUMAN_10 :
        /*list_size_in_bytes*/ Gf_SIZE_IN_BYTES,
        list_atime ? Gf_TIME_ACCESSED :
        list_ctime ? Gf_TIME_MODE_CHANGED :
        /*list_mtime*/ Gf_TIME_MODIFIED);
    }
    else
      sprintf(cmd, "+G%s%sn",
        list_inode ? "i" : "",
        list_size_in_blocks ? "S" : "");

    do_options(cmd);
    G_option = FALSE; /* G_format was not specified on the command-line */
  }


  if (GTarStyle && Tar5Style)
  {
    fprintf(stderr, "\
%s: Both --GTarSytle and --Tar5Style special features defined.\n\
%s: Only one may be defined at a time.\n",
      Progname, Progname);
    exit(USAGE_ERROR);
  }

  if (GTarStyle || Tar5Style)
  {
    if (list_long)
    {
      if (! G_option) {
  static char cmd[32];
  Boole numeric = (Tar5Style || list_long_numeric);
  Boole omit_gid = (list_long_omit_gid);
  char *id = "o";
  if (!numeric && !omit_gid) id = "o";
  if ( numeric && !omit_gid) id = "O";
  if (!numeric &&  omit_gid) id = "u";
  if ( numeric &&  omit_gid) id = "U";
  sprintf(cmd, "+G~tp~%ssmn", id);
  do_options(cmd);
  G_option = FALSE; /* G_format was not specified on the command-line */
      }
      if (! N_option) {
  do_options("+NFL");
  N_option = FALSE; /* N_format was not specified on the command-line */
      }
      if (! T_option) {
  if (GTarStyle) do_options("+T^NY-M-D h:m:s");
  if (Tar5Style) do_options("+TM%_DY h:m");
  T_option = FALSE; /* T_format was not specified on the command-line */
      }
      /* GTar/Tar5 both mark directories: */
      do_options("-p");
      /* FIXME: should do_options("-A") be implied? */
      /* FIXME: should do_options("-U") be implied, since tar is unsorted? */
    }
    else
    {
      /* GTar/Tar5 styles only take effect during long listings: */
      GTarStyle = FALSE;
      Tar5Style = FALSE;
    }
  }


  /* All munge levels prior to 20130819 have implied +d unless explicitly
     disabled via +~d.  But since "change" times are of interest on all files
     types, munge level 3 now implies +~d unless explictly enabled via +d: */
  if (!list_directories_specified &&
      (munge > 0 && munge != 3)) do_options("+d");

  /* Munge level 2 displays access times of the files that are within
     a hierarchy; munge level 4 attempts to revert modification and access
     times.  Both levels should avoid symbolic links that point elsewhere
     unless -L was explicitly specified: */
  /* Don't ignore symlinks for munge level 3 as both SunOS5.x and Linux2.6.x
     are known to set change times on symlink source. */
  if (!expand_symlink &&
      (munge == 2 || munge == 4)) do_options("+F*T{~l}");

  /* All munge levels as of version 1.54 and above imply +q unless previously
     specified: */
  if (munge > 0 && !plus_q_specified && VersionLevel >= 154)
    do_options("+q");

  /* Munge level 4 has implied quoted names since 1.49f1: */
  if (munge == 4) do_options("+q"); /* Use quoted names */


  /* If +N was explicitly defined on the command line then automatically
     assume using full file names, otherwise determine whether using full
     file names by looking for 'F' or 'd' directives in either the +N or +G
     formats: */
  if (N_option)
    /* +N was explicitly defined on the command line (set "using_full_names"
       so to allow processing of filename args having directory components): */
    using_full_names = TRUE;
  else
  {
    /* +N is being implicitly defined: */
    char list[3];

    /* First: examine +G format for the "N" directive which if found then
       the desired +N format is "F" unless N_format was changed internally
       from default: */
    list[0] = Gf_FULL_NAME_FORMAT;
    list[1] = CNULL;
    if (find_first_directive(list, G_format) != NULL)
    {
      if (strcmp(N_format, N_format_DEFAULT) == 0)  /* Using default? */
  N_format = "F"; /* Nf_FULL_NAME */
    }

    /* Second: look for presense of 'F' or 'd' in either N_ or G_format: */
    list[0] = Nf_FULL_NAME; /* Shared by G_format */
    list[1] = Nf_DIR_NAME;  /* Shared by G_format */
    list[2] = CNULL;
    using_full_names = ((find_first_directive(list, N_format) != NULL) ||
      (find_first_directive(list, G_format) != NULL));
  }

  /* Determine the default sort-by-time field by finding the first
     occurrence of 'm', 'a', or 'c' in the +G format: */
  {
    cp = find_first_directive("mac", G_format);
    if (cp == NULL)
      first_mac = CNULL;
    else
      first_mac = *cp;
  }

  /* Determine if cksumming: */
  cksumming = (find_first_directive("C", G_format) != NULL);
  if (cksumming && cksum_type == CKSUM_UNSPECIFIED)
  {
    if (VersionLevel <= 148)
      /* Version 1.48 and below uses BSD by default: */
      do_options("+C=bsd");
    else
      /* Version 1.49 and above uses POSIX by default: */
      do_options("+C=posix");
  }


  /* Avoid "ls" trimmings if using full names or executing output: */
  avoid_trimmings = using_full_names || execute_mode || quotaling;

  /* dotdirs are very annoying and very redundant when avoiding trimmings
     as the parent directory has already been listed: */
  if (list_dotdir && avoid_trimmings) list_dotdir = FALSE;

  /* Starting from 1.40a (Sept 1997), "list_topdir" and "avoid_trimmings"
     have been considered synonymous by elsFilter routines.  Under this broad
     interpretation, '+Gn -A' correctly does not list "." but '+GN -A' does
     surprisingly list "." which seems to contradict the definition of "-A"
     (although other reasons exist that justify this behavior). */
  /* FIXME: list_topdir is implied by execute_mode, but this could surprise
     someone to have or not have topdir be present.  Fortunately commands
     such as "rm -fr ." are ignored by modern Unixes. */
  /* FIXME: Should implicit list_topdir be overruled if "-A" is specified? */
  list_topdir = using_full_names || execute_mode || quotaling;

  /* If following /bin/ls semantics and if traverse option NOT explicitly
     specified, then enable all traversing: */
  /* Should this be followed for BSD, SYS5, GNU, semantics ??? */
  if (!traverse_specified && ANYBIT(Sem,SEM_BSD|SEM_SYS5|SEM_GNU|SEM_LS))
  {
    traverse_mp = TRUE;
    traverse_expanded_symlink = TRUE;
  }

  /* BSD's (i.e. SunOS4's) and GNU's /bin/ls truncates long USER/GROUP IDs: */
  if (ANYBIT(Sem,SEM_BSD|SEM_GNU))
  {
    TruncateName = TRUE;
  }

  /* If +G was explcitly defined on the command line then check whether
     +G'...' or +G"..." specified, and if so imply +q as a safety
     precaution (e.g. don't get tricked by ";" inside filenames): */
  if (G_option)
  {
    Boole quote_mode = FALSE;
    char *cp = G_format;
    while (*cp != CNULL)
    {
      /* Look for start of quote mode (i.e. " or ' not preceded by \): */
      if (*cp == '"' || *cp == '\'')
      {
  quote_mode = TRUE;
  break;
      }
      /* Skip a single quoted char: */
      else if (*cp == '\\')
  cp++;
      if (*cp != CNULL) cp++;
    }
    if (quote_mode)
    {
      if (verboseLevel > 0 && !plus_q)
  fprintf(stderr, "%s: Notice: +q implied due to +G'...' argument\n",
    Progname);
      do_options("+q"); /* Use quoted names */
    }
  }

  /* Report on special options if verboseLevel > 0: */

  if (verboseLevel > 0)
  {
    Boole listed = FALSE;
    if (CCaseMode)
    {
      fprintf(stderr, "\
%s: --CCaseMode: Mimic ClearCase permissions (e.g. mask 'w' permissions)\n\
", Progname);
      listed = TRUE;
    }

    if (GTarStyle)
    {
      fprintf(stderr, "\
%s: --GTarStyle: Mimic GNU 'tar tv' listings\n\
", Progname);
      listed = TRUE;
    }

    if (Tar5Style)
    {
      fprintf(stderr, "\
%s: --Tar5Style: Mimic Sys5 'tar tv' listings\n\
", Progname);
      listed = TRUE;
    }

    if (FirstFound)
    {
      fprintf(stderr, "\
%s: --FirstFound: Only the first file found matching search criteria listed\n\
", Progname);
      listed = TRUE;
    }

    if (OncePerDir)
    {
      fprintf(stderr, "\
%s: --OncePerDir: Each directory containing search criteria listed once only\n\
", Progname);
      listed = TRUE;
    }

    if (DirDepth >= 0)
    {
      fprintf(stderr, "\
%s: --DirDepth: Recursion depth of directories is being limited to %d\n\
", Progname, DirDepth);
      listed = TRUE;
    }

    if (IncludeSS)
    {
      fprintf(stderr, "\
%s: --IncludeSS: Listing of snapshot directories included\n\
", Progname);
      listed = TRUE;
    }
    if (listed) fprintf(stderr, "\n");
  }

  switch( rwm_ext_mode ) {
    case  1: rwm_col_ext = &rwm_col_ext1; break;
    case  2: rwm_col_ext = &rwm_col_ext2; break;
    default: rwm_col_ext = &rwm_col_ext3; break;
  }

  return;
}


void append_dir(Dir_List *dlist,
    char *fname,
    int fname_size)
{
  register Dir_Item *file;

  /* Append file name to directory list: */
  file = dirItemAlloc();
  if (dlist->head == NULL)
    dlist->head = file;
  else
    dlist->tail->next = file;
  dlist->tail = file;
  file->next = NULL;

  if (fname_size > 0)
  {
    file->mem = memAlloc(fname_size+1); /* +1 needed by LINUX, CYGWIN */
    file->fname = file->mem;
    strcpy(file->fname, fname);
  }
  else
  {
    /* Command line arguments do not need to have mem alloc'ed: */
    file->mem = NULL;
    file->fname = fname;
  }

  file->readdir_errno = 0;
  file->stat_errno = 0;

  return;
}


void copy_info(Dir_Item *dest, Dir_Item *src)
{
  /* Copy just the control values and info portion of a Dir_Item:
     (This wouldn't be necessary if the same Dir_Item could exist on
     more than one Dir_List or if we didn't need to know the stat info
     of the Dir_Item in the secondary list.) */
  dest->islink = src->islink;
  dest->isdir = src->isdir;
  dest->isdev = src->isdev;

  dest->dotdir = src->dotdir;
  dest->hidden = src->hidden;
  dest->listable = src->listable;
  dest->searchable = src->searchable;

  dest->readdir_errno = src->readdir_errno;
  dest->stat_errno = src->stat_errno;
  dest->info = src->info;

  return;
}


int read_dir(Dir_List *dlist, char *dname, DIR *dir)
{
  register struct dirent *dp;
  int nread = 0;
  int save_errno = errno;
  int readdir_errno;

  do {
    errno = 0;
    dlist->head = NULL;
    sigEvent = FALSE;
    while (!sigEvent && (dp = readdir(dir)) != NULL)
    {
      /* 20150916: Although d_reclen has proven reliable for all OSes
   (except CYGWIN), I have decided to use strlen(fname) regardless:  */
#   define DONT_USE_DIRENT_RECLEN
#   if defined(CYGWIN) || defined(DONT_USE_DIRENT_RECLEN)
      int fname_size = strlen(dp->d_name);
#   else
      /* NB: Rather than have to calculate strlen(fname), d_reclen can be
   used as a rough guess assuming that d_reclen >= strlen(fname).
   This assumption has proven true for SunOS4.x, SunOS5.x, Linux2.x,
   FREEBSD, Darwin, HP-UX10.x, HP-UX11.x, OSF1, AIX, IRIX, SCO.
   The only OS I know of where this is not the case is CYGWIN. */
      int fname_size = dp->d_reclen;
#   endif
      if (fname_size < 1) fname_size = 1;
      append_dir(dlist, dp->d_name, fname_size);
      nread++;
    }

    /* Note: Some OSes set the final errno from readdir to indicate no more
       entries, in which case it shouldn't be considered an error.  Only in
       cases where no reads occurred should errno be considered valid.  */
    readdir_errno = errno;

    /* Re-read directory if signal took place during read: */
    if (sigEvent)
    {
      /*fprintf(stderr, "readdir errno == %d, dlist == %p\n",
  errno, dlist->head);*/
      if (dlist->head != NULL) free_dir(dlist);
      rewinddir(dir);
      nread = 0;
    }
  } while (sigEvent);

  /* If readdir() fails on the first access then dlist->head will be NULL
     in which case append a dummy directory head of "".  There are two
     known cases during which this can occur: 1) listing a corrupt
     directory on a damaged local disk, and 2) while logged in as "root"
     listing a directory that is NFS mounted from a host that doesn't
     export with "root=myhost" and the directory has settings "chmod 700;
     chown root:root" (stat() detects errors only for unprivileged users
     in this situation). */
  if (dlist->head == NULL)
  {
    /* Append dummy directory head and record errno: */
    append_dir(dlist, "", 0);
    dlist->head->readdir_errno = readdir_errno;

    /* Something's very off if errno NOT set and directory is empty: */
    if (readdir_errno == 0)
    {
      fprintf(stderr, "%s: %s: Directory has no \".\" or \"..\"\n",
        Progname, dname);
      listingError = TRUE;
    }
  }
  else
  {
    /* The "." dir is always the very first item in the list, but search
       for it anyway just in case: */
    register Dir_Item *file = dlist->head;
    while (file != NULL && strcmp(file->fname, ".") != 0)
      file = file->next;
    CwdItem = file;
  }

  errno = save_errno;
  return(nread);
}


void stat_dir(Dir_List *dlist, char *dname)
{
  register Dir_Item *file;
  char *path;
  int sts;
  int save_errno = errno;

  dir_block_total = 0;
  dir_file_count = 0;

  for (file = dlist->head; file != NULL; file = file->next)
  {
    char *slash;

    /* Start by setting as many attributes as possible.  These attributes
       must not depend on any information from stat(): */

    path = full_name(dname, file->fname);

    /* Deal with cases such as: "els ../dir ../.. ../../.xyz" */
    slash = strrchr(file->fname, '/');
    if (slash)
      slash++;
    else
      slash = file->fname;
    file->dotdir = strcmp(slash,".") == 0 || strcmp(slash,"..") == 0;
    file->hidden = !file->dotdir && slash[0] == '.';

    /* Set the remaining attributes that are dependent on stat(): */

    errno = 0;
# if defined(S_IFLNK)
    /* System has symbolic links: */
    sts = sigSafe_lstat(path, &file->info);
    file->islink = (sts == 0 && S_ISLNK(file->info.st_mode));
    if (file->islink)
    {
      if (expand_symlink)
      {
  /* Return info regarding the actual file rather than the link: */
  sts = sigSafe_stat(path, &file->info);
      }
      else if (zero_st_info && VersionLevel >= 143)
      {
  /* If not expanding symlinks then modification and access times can
     be annoying as st_mtime gets changed whenever a symlink gets
     copied via dump/restore, tar, etc.; also, st_atime gets updated
     whenever listing a symlink. */
  if ((zero_st_mask & ZERO_LINK_MTIME) != 0) file->info.st_mtime = 0;
  if ((zero_st_mask & ZERO_LINK_ATIME) != 0) file->info.st_atime = 0;
  /* When first created, the uid/gid of a symbolic link is that
     of the user, but after being copied via dump/restore, tar,
     etc. the symbolic link ends up owned by root: */
  if ((zero_st_mask & ZERO_LINK_OWNER) != 0)
  {
    file->info.st_uid = 0;
    file->info.st_gid = 0;
  }
      }
    }
# else
    /* System does not have symbolic links: */
    sts = sigSafe_stat(path, &file->info);
    file->islink = FALSE;
# endif
    file->stat_errno = errno;
    if (Debug&8)
      printf("st_blksize = %lu (%s)\n", (Ulong)file->info.st_blksize, path);

    if (!file->stat_errno)
    {
      if (S_ISREG(file->info.st_mode) && zero_st_info)
      {
  /* ClearCase's clearfsimport doesn't preserve modification times of
     regular files if the file size is 0: */
  if (CCaseMode && (zero_st_mask & ZERO_CLEARCASE) != 0)
    if (file->info.st_size == 0) file->info.st_mtime = 0;
  /* ClearCase turns all "w" bits off regular files during checkin: */
  if (CCaseMode && (zero_st_mask & ZERO_CLEARCASE) != 0)
    file->info.st_mode &= ~(S_IWUSR|S_IWGRP|S_IWOTH);
      }

      file->isdir = S_ISDIR(file->info.st_mode);
      if (file->isdir && zero_st_info && VersionLevel >= 143)
      {
  /* While directory modification times are usually preserved after
     being copied via dump/restore, tar, etc., they can still be a
     distraction when changes occur: */
  if ((zero_st_mask & ZERO_DIR_MTIME) != 0) file->info.st_mtime = 0;
  /* Access time can be annoying when listing directories as st_atime
     gets changed whenever listing the contents of a directory: */
  if ((zero_st_mask & ZERO_DIR_ATIME) != 0) file->info.st_atime = 0;
  /* Directories can change size over time or after being copied
     via dump/restore, tar, etc.: */
  if ((zero_st_mask & ZERO_DIR_SIZE) != 0)
  {
    file->info.st_size = 0;
#       ifndef NO_ST_BLOCKS
    file->info.st_blocks = 0;
#       endif
  }
  /* ClearCase doesn't grok "g+s" bits on directories: */
  if (CCaseMode && (zero_st_mask & ZERO_CLEARCASE) != 0)
    file->info.st_mode &= ~(S_ISGID);
      }

      if (GTarStyle || Tar5Style)
      {
  if (file->isdir)
    /* GTar/Tar5 list directories as zero size: */
    file->info.st_size = 0;
  else if (file->islink && GTarStyle)
    /* GTar lists symlinks as zero size, Tar5 lists normal size: */
    file->info.st_size = 0;
      }

      file->isdev = (S_ISBLK(file->info.st_mode) ||
         S_ISCHR(file->info.st_mode));
      if (file->isdev && zero_st_info && VersionLevel >= 144)
      {
  if ((zero_st_mask & ZERO_DEV_MTIME) != 0) file->info.st_mtime = 0;
  if ((zero_st_mask & ZERO_DEV_ATIME) != 0) file->info.st_atime = 0;
  /* Note: Sometime the owner of a device file matters, sometimes
     it doesn't; thus, "owner" should never be zeroed. */
      }

      if (S_ISFIFO(file->info.st_mode) && zero_st_info && VersionLevel >= 149)
      {
  /* FYI: This should really be a 150 feature, but I want it now. */
  if ((zero_st_mask & ZERO_FIFO_MTIME) != 0) file->info.st_mtime = 0;
  if ((zero_st_mask & ZERO_FIFO_ATIME) != 0) file->info.st_atime = 0;
      }

      filter_file(file, path);
      if (file->listable) dir_block_total += getStatBlocks(&file->info);
    }
    else
    {
      file->isdir = FALSE;
      file->isdev = FALSE;
      /* Even though an error was detected during stat(), we have enough
   information to see if it is to be filtered from our display list.
   If the file qualifies for filtering, then no error needs be reported
   during the output stage! */
      filter_file(file, path);
    }

    if (file->listable) dir_file_count++;

    if (watch_progress)
    {
      static int nfiles = 0;
      static int nreport = 0;
      if (nfiles == nreport)
      {
  sigAskAbort = FALSE; /* Ask to abort via ^C only once an interval */
  fprintf(ttyout, "%d\r", nfiles);
  fflush(ttyout);
  nreport += 1000;
      }
      /* Don't count dotdirs as they were already counted in the parent dir! */
      if (!file->dotdir) nfiles++;
    }
  }

  errno = save_errno;
  return;
}


void name_dir(Dir_List *dlist, char *dname)
{
  Boole skip_total = filtering && (dir_file_count == 0);

  if (!avoid_trimmings && !skip_total)
  {
    Boole list_total = list_long;
    Boole unreadable = (dlist == NULL || dlist->head->fname[0] == CNULL);
    if (output_nlines > 0) putchar('\n');

    if (ALLBITS(Sem,SEM_SYS5|SEM_LS))
    {
      /* Trimmings a la SYS5/LS: */
      /* SunOS5,HPUX10 list both dname and total, even if unreadable(!),
   except for OSF1 which behaves more like BSD if unreadable. */
      if (dname[0] != CNULL && (recursive || multiple_file_args))
      {
  if (zero_file_args && recursion_level > 1 &&
      strncmp(dname, "./", 2) != 0)
    printf("./%s:\n", dname);
  else
    printf("%s:\n", dname);
      }
    }
    else if (ALLBITS(Sem,SEM_GNU|SEM_LS))
    {
      /* Trimmings a la GNU/LS: */
      if (dname[0] != CNULL && recursive)
      {
  /* GNU doesn't list dname or total if unreadable: */
  if (unreadable)
    list_total = FALSE;
  else
    printf("%s:\n", dname);
      }
    }
    else
    {
      /* Trimmings a la BSD/LS and ALL/ELS: */
      if (multiple_file_args || recursion_level > 1)
      {
  /* BSD and ELS don't list dname or total if unreadable: */
  if (unreadable)
  {
    /* Unreadable directory a la BSD and ELS writes an informational
       message to stdout: */
    printf("%s unreadable\n", dname);
    list_total = FALSE;
  }
  else
    printf("%s:\n", dname);
      }
    }

    if (list_total)
    {
      char nblocks[64];
      sprintf(nblocks, FU_st_blocks(FALSE,0,
        ALLBITS(Sem,SEM_SYS5|SEM_LS) ?
        dir_block_total : dir_block_total/2));
      printf("total %s\n", nblocks);
    }
    output_nlines++;
  }

  if (dlist == NULL)
  {
    if (!warning_suppress) fprintf(stderr, "%s: %s: Permission denied\n",
           Progname, dname);
    listingError = TRUE;
  }
  else if (dlist->head->readdir_errno != 0)
  {
    /* SunOS's /bin/ls lets readdir() errors slide-by undectected (only
       stat() errors get reported): */
    if (ANYBIT(Sem,SEM_ELS))  /* What does SEM_GNU do??? */
    {
      if (!warning_suppress)
  errnoMsg(dlist->head->readdir_errno, dname);
      listingError = TRUE;
    }
  }

  return;
}


int count_dir(Dir_List *dlist, char *dname)
{
  register Dir_Item *ptr = dlist->head;
  int n = 0;
  while (ptr != NULL)
  {
    ptr = ptr->next; n++;
  }
  return(n);
}


Local void qsortDirName(Dir_Item *a[], int n);
Local int  qsortDirName_compare(const void *arg1, const void *arg2);
Local void qsortDirTime(Dir_Item *a[], int n);
Local int  qsortDirTime_compare(const void *arg1, const void *arg2);
Local char sortTimeBy;


void sort_dir(Dir_List *dlist, char *dname, int n)
{
  Dir_Item *ptr, **darray;
  int i;

  if (!unsort)
  {
    if (n == -1) n = count_dir(dlist, dname);
    if (n == 0) return;
    darray = memAlloc(n * sizeof(Dir_Item **));

    i = 0;
    ptr = dlist->head;
    while (ptr != NULL && i < n)
    {
      darray[i++] = ptr;
      ptr = ptr->next;
    }
    /*DBG: ptr and i should both terminate together*/
    /*if (ptr != NULL || i != n) printf("Error in sort_dir loop\n");*/

    /* Since time and name sorts may resort to calling each other when
       comparisons are 0, always set sortTimeBy setting regardless: */
    if (list_atime)
      /* Sort by time accessed regardless of what's in G_format: */
      sortTimeBy = Gf_TIME_ACCESSED;
    else if (list_ctime)
      /* Sort by time mode changed regardless of what's in G_format: */
      sortTimeBy = Gf_TIME_MODE_CHANGED;
    else
    {
      /* Use first occurrence of either m, a, or c as found in G_format: */
      if (first_mac == CNULL)
  sortTimeBy = Gf_TIME_MODIFIED;  /* default */
      else
  sortTimeBy = first_mac;
    }

    /* Perform sort: */
    if (time_sort)
      qsortDirTime(darray, n);
    else if (ANYBIT(Sem,SEM_BSD|SEM_GNU) && list_ctime)
      /* BSD's and GNU's ls -c implies ls -ct, but SYS5's does NOT! */
      qsortDirTime(darray, n);
    else
      qsortDirName(darray, n);

    dlist->head = darray[0];
    for (i = 0; i < n-1; i++)
      darray[i]->next = darray[i+1];
    darray[i]->next = NULL;
    dlist->tail = darray[i];

    memFree(darray);
  }

  return;
}


/* Return string position of first alphanumeric, else if no alphanumeric
   found then return entire string: */
Local char *firstAlnum(char *str)
{
  char *cp = str;
  while (*cp != CNULL && !isAlnum(*cp)) cp++;
  return(*cp != CNULL ? cp : str);
}


Local int qsortDirName_compare(const void *arg1, const void *arg2)
{
  const Dir_Item *a1 = *(Dir_Item **)arg1;
  const Dir_Item *a2 = *(Dir_Item **)arg2;
  int diff;

  if (SortDICT)
  {
    /* Sort dictionary order (similar to "sort -df" and xx.UTF-8 locale): */
    diff = strcmp_ci(firstAlnum(a1->fname), firstAlnum(a2->fname));
  }
  else if (SortCI)
  {
    diff = strcmp_ci(a1->fname, a2->fname);
  }
  else
  {
#ifdef HAVE_LOCALE
    if (useLcCollate)
      diff = strcoll(a1->fname, a2->fname);
    else
#endif
      diff = strcmp(a1->fname, a2->fname);
  }

  if (diff == 0)
    diff = strcmp(a1->fname, a2->fname);

  if (reverse_sort)
    diff = -diff;

  return(diff);
}


Local void qsortDirName(Dir_Item *a[], int n)
{
  qsort(a, n, sizeof(Dir_Item *), qsortDirName_compare);
  return;
}


Local int qsortDirTime_compare(const void *arg1, const void *arg2)
{
  const Dir_Item *a1 = *(Dir_Item **)arg1;
  const Dir_Item *a2 = *(Dir_Item **)arg2;
  ELS_time_t t1, t2;
  int diff;

  /* NB: if defined(HAVE_LONG_LONG_TIME), then ELS_time_t is 64-bit signed
     so as to sort negative times as required, otherwise ELS_time_t will be
     unsigned 32-bit. */

  if (sortTimeBy == Gf_TIME_MODIFIED)
  {
    t1 = (ELS_time_t)a1->info.st_mtime;
    t2 = (ELS_time_t)a2->info.st_mtime;
  }
  else if (sortTimeBy == Gf_TIME_ACCESSED)
  {
    t1 = (ELS_time_t)a1->info.st_atime;
    t2 = (ELS_time_t)a2->info.st_atime;
  }
  else if (sortTimeBy == Gf_TIME_MODE_CHANGED)
  {
    t1 = (ELS_time_t)a1->info.st_ctime;
    t2 = (ELS_time_t)a2->info.st_ctime;
  }
  else /* Default is time modified */
  {
    t1 = (ELS_time_t)a1->info.st_mtime;
    t2 = (ELS_time_t)a2->info.st_mtime;
  }

  /* Carefully calculate "diff" so as to avoid possible underflow: */
  diff = (t1 == t2 ? 0 :
    t1 <  t2 ? 1 : -1);

  if (diff == 0)
  {
    static int rec_level = 0;
    rec_level++;
    if (rec_level == 1) diff = qsortDirName_compare(arg1, arg2);
    rec_level--;
  }
  else if (reverse_sort)
  {
    diff = -diff;
  }

  return(diff);
}


Local void qsortDirTime(Dir_Item *a[], int n)
{
  qsort(a, n, sizeof(Dir_Item *), qsortDirTime_compare);
  return;
}


void list_dir(Dir_List *dlist,
        char *dname)
{
  register Dir_Item *ptr;
  Dir_List sub_dlist;
  DIR *dir;
  Boole item_listed = FALSE;

  recursion_level++;
  sub_dlist.head = NULL;

  hg_root = is_hg ( fullpath( (char *) dname ) );
  if ( hg_root ) hg_stat = load_hgstatus ( hg_root );

  gt_root = is_git( fullpath( (char *) dname ), false );
  if ( gt_root ) gt_stat = load_gitstatus( gt_root );

  for (ptr = dlist->head; ptr != NULL; ptr = ptr->next)
  {
    if (OncePerDir && item_listed)
    {
      /* No more listings this directory, but recursively list subdirs: */
      /* DO NOTHING */
    }
    else
    {
      item_listed = list_item(ptr, dname);

      if (FirstFound && item_listed)
      {
  /* No more listings or recursion desired: */
  finishExit();
  /*NOTREACHED*/
      }
    }

    /* Recursively list directory (if appropriate): */
    if (ptr->searchable ||
  (recursion_level == 1 && ptr->dotdir))
    {
      if (expand_directories && (recursion_level == 1 || recursive))
      {
  if (recursion_level == 1 || !ptr->dotdir)
  {
    append_dir(&sub_dlist, ptr->fname, 0);
    copy_info(sub_dlist.tail, ptr);
  }
      }
    }
  }

  if (quotaling)
  {
    subQuotal_print();
  }

  if (DirDepth >= 0 && recursion_level-1 > DirDepth)
  {
    /* Skip subdirectories: */
    /* DO NOTHING */
  }
  else
  {
    /* Process subdirectories: */
    for (ptr = sub_dlist.head; ptr != NULL; ptr = ptr->next)
    {
      char cur_dname[MAX_FULL_NAME];
      if (dname[0] == CNULL || strcmp(dname, ".") == 0)
  strcpy(cur_dname, ptr->fname);
      else
      {
  char *fmt;
  if (dname[strlen(dname)-1] == '/')
    fmt = "%s%s";
  else
    fmt = "%s/%s";
  sprintf(cur_dname, fmt, dname, ptr->fname);
      }

      if ((dir = opendir(cur_dname)) == NULL)
      {
  name_dir(NULL, cur_dname);
      }
      else
      {
  Boole ok_to_cross = TRUE; /* allow crossing by default */
  struct stat *old_mp = NULL;

  /* Save top-most mount-point: */
  if (recursion_level == 1) set_mp(&ptr->info);

  if (ptr->islink)
  {
    /* Designate a temporary mount-point for expanded symlinked
       directory and then cross if option enabled: */
    old_mp = set_mp(&ptr->info);
    ok_to_cross = traverse_expanded_symlink;
  }
  else if (!traverse_mp)
    /* If not traversing mount-points then allow crossing only if
       the mount-point hasn't changed since last set: */
    ok_to_cross = diff_mp(&ptr->info) == 0;

  if (ok_to_cross)
  {
    Dir_List cur_dlist;
    int n;
    const
    char *saveCwdPath = CwdPath;
    CwdPath = cur_dname;

    static Boole first = TRUE;
    // 2020-12-13: XYZZY check here for hg repo
    // if new contains old, don't redo hg

    if ( first || strncmp( saveCwdPath, CwdPath, strlen( saveCwdPath ))) {
      hg_root = is_hg ( fullpath( (char *) CwdPath ) );
      if ( hg_root ) hg_stat = load_hgstatus ( hg_root );

      gt_root = is_git( fullpath( (char *) CwdPath ), false );
      if ( gt_root ) gt_stat = load_gitstatus( gt_root );

      first=FALSE;
    }

    n = read_dir(&cur_dlist, cur_dname, dir);
    stat_dir(&cur_dlist, cur_dname);
    name_dir(&cur_dlist, cur_dname);
    sort_dir(&cur_dlist, cur_dname, n);
    list_dir(&cur_dlist, cur_dname);
    free_dir(&cur_dlist);
    CwdPath = saveCwdPath;
  }
  else if (verboseLevel > 0)
  {
    char *path = full_name(dname, ptr->fname);
    fprintf(stderr, "%s: %s: %s not traversed\n",
      Progname, path,
      ptr->islink ? "symlink" : "mount-point");
  }
  closedir(dir);

  /* Restore old mount-point prior to symlink traversal (if any): */
  if (old_mp != NULL) set_mp(old_mp);
      }
    }
  }

  free_dir(&sub_dlist);
  recursion_level--;

  return;
}


void free_dir(Dir_List *dlist)
{
  register Dir_Item *ptr1, *ptr2;

  /* free_dir() invalidates the Cwd* global variables: */
  CwdItem = NULL;
  CwdBlocksize = 0;

  ptr1 = dlist->head;
  while (ptr1 != NULL)
  {
    ptr2 = ptr1->next;
    if (ptr1->mem != NULL) memFree(ptr1->mem);
    dirItemFree(ptr1);
    ptr1 = ptr2;
  }
  dlist->head = NULL;

  return;
}


Boole list_item(Dir_Item *file,
    char *dname)
{
  Boole item_listed = FALSE;

  /* Perform G_print only if no errors were detected while getting the file
     status and if the file is of interest (i.e. it needs to satisfy our
     listing criteria): */
  if (!file->stat_errno && file->listable)
  {
    if (quotaling)
      Quotal(file);
    else
    {
//    printf(  "# XYZZY %s | %s\n", dname, file->fname );      // XYZZY
      char *bp = G_print(output_buff, G_format, dname, file);
      if ( !rwm_dospace ) {
        if ( rwm_docolor ) {
          strcat( bp, cs );    // "[;0m" );  // Reset all color settings at EOL
          rwm_type = 0;
          rwm_mode = 0;
        }
        strcat(bp, "\n");        // XYZZY - end of line output
      }
    }
  }

  if (file->stat_errno)
  {
    /* An error was detected while getting the file status or during G_print.
       Report the error only if the file is of interest (i.e. it needs to
       satisfy either our listing or search criteria): */
    if (file->listable || file->searchable)
    {
      if (!warning_suppress)
  errnoMsg(file->stat_errno, full_name(dname, file->fname));
      listingError = TRUE;
    }
  }
  else
  {
    /* No errors were detected while getting the file status or during
       G_print.  List the file only if it is of interest (i.e. it needs to
       satisfy our listing criteria): */
    if (file->listable)
    {
      if (execute_mode)
      {
  Uint sts;
  if (file->islink && !execute_symlinks)
  {
    fprintf(stderr, "%s: %s: +X ignoring symbolic link\n",
      Progname, full_name(dname, file->fname));
    fprintf(stderr, "\
  (NOTE: +h flag must be specified for +X to act on symbolic links)\n");
/*(+X requires that +h flag be specified before acting on symbolic links)*/
    sts = 1;
  }
  else
  {
    if (verboseLevel > 0) fputs(output_buff, stdout);
    sts = system(output_buff);
  }
  if (QuitOnError || execute_sts_check)
  {
    if ((sts & 0xff) == 0) sts >>= 8;
    if (sts != execute_sts_good)
    {
      if (verboseLevel > 0)
        fprintf(stderr, "%s: Terminated: return status %u != %u\n",
          Progname, sts, execute_sts_good);
      exit(sts);
    }
  }
      }
      else {
        if ( rwm_docolor
           && !file->isdir
           && file->info.st_nlink > 1 ) fputs( inv, stdout );    // Invert FG/BG
  fputs(output_buff, stdout);
  if ( rwm_dospace ) fprintf(stdout, "%c", '\0');
      }
      item_listed = TRUE;
      output_nlines++;
    }
        if ( rwm_docolor && file->info.st_nlink > 1 ) fputs(rinv, stdout );    // Revert FG/BG
  }

  if (QuitOnError && listingError)
  {
    if (verboseLevel > 0)
      fprintf(stderr, "%s: Quitting due to error as requested\n", Progname);
    exit(LISTING_ERROR);
  }

  return(item_listed);
}


char *find_first_directive(char *list, char *fmt)
{
  if (fmt != NULL)
  {
    char *cp = fmt;
    while (*cp != CNULL)
    {
      /* Look for "'%...'" directive inside string: */
      if (*cp == '"' || *cp == '\'')
      {
  char delimiter = *cp++;
  while (*cp != CNULL && *cp != delimiter)
  {
    /* Skip a quoted character inside string: */
    if (*cp == '\\')
      cp++;
    /* Look for "'%...'" directive inside string (ignore "%%"): */
    else if (*cp == '%')
    {
      cp++;
      if (*cp != '%')
      {
        while (IS_MEMBER(*cp, "0_+-")) cp++;
        while (isDigit(*cp)) cp++;  /* Scan past width (if any) */
        if (IS_MEMBER(*cp, list))
    return(cp);
      }
    }
    if (*cp != CNULL) cp++;
  }
      }
      /* Skip a quoted character outside of string: */
      else if (*cp == '\\')
  cp++;
      /* Look for directive outside of string: */
      else if (IS_MEMBER(*cp, list))
  return(cp);
      if (*cp != CNULL) cp++;
    }
  }
  return(NULL);
}


/* FYI: scaleSizeToHuman() shared with df.c */
char *scaleSizeToHuman(ELS_st_size val, int base)
{
  /* NOTE: zero_pad not implemented (and probably not desirable anyway).
     Both GNU and SYS5's base2 scaling uses 1024 as initial non-scaled
     threshold and then switches to 1000 once scaling begins.  Base10
     scaling, however, uses 1000 for all thresholds. */
  static char buf[64];
  if (base == 2 && val < 1024) /* Yes, 1024 for base2! (see note above) */
  {
    sprintf(buf, F_st_size(FALSE,0, val));
  }
  else if (base == 10 && val < 1000)
  {
    sprintf(buf, F_st_size(FALSE,0, val));
  }
  else
  {
    int i = 0;
    ELS_st_size mantissa = val;
    ELS_st_size scale = 1;

    do {
      if (mantissa < 10) {
  sprintf(buf, "%.1f", (double)val/scale);
  break;
      }
      else if (mantissa < 1000) { /* Yes, 1000 for all! (see note above) */
  ELS_st_size hscale = scale / 2;
  sprintf(buf, F_st_size(FALSE,0, (val+hscale)/scale));
  break;
      }
      if (base == 2)
      {
  scale = scale << 10;
  mantissa = mantissa >> 10;
      }
      else
      {
  scale = scale * 1000;
  mantissa = mantissa / 1000;
      }
      i++;
    } while (i < 8);

    {
      /* RESOLVE: do "B/b" misleadingly suggest "blocks"? */
      char *mag = (base == 2 ? "BKMGTPEZY" : "bkmgtpezy");
      if (i >= 0 && i <= 8)
  strncat(buf, &mag[i], 1);
      else
  strcat(buf, "?");
    }
  }
  return(buf);
}

char *rwm_col_age( char *buff, time_t ftime, Boole flag ) {
  char tmp[MAX_INFO];
  strncpy( tmp, buff, MAX_INFO);

  // colorize dates based on ages in ELS_FT_COLORS
  if (flag ) {                  // start of date string
    int i=0;
    Ulong f_age = (Ulong)The_Time - (Ulong) ftime;
    while ( i<rwm_ftcnt && rwm_ages[i] < f_age ) ++i;

//  printf("%2d: %lu %lu AGE\n", i, rwm_ages[i], f_age );

    if ( i<rwm_ftcnt ) sprintf(buff, "[%sm%s", rwm_cols[i], tmp );
  } else                        // end   of date string
      sprintf(buff, "%s[39;0m", tmp);   // reset foreground color

  buff += strlen(buff);
  return ( buff );
}

#define ZERO_PAD_DEFAULT  FALSE
char *G_print(char *buff,
        char *fmt,
        char *dname,
        Dir_Item *file)
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
  Boole ttoggle = FALSE;
  register struct stat *info = &file->info;
  int fmode = file->info.st_mode;
  int ftype = (fmode & S_IFMT);

  bp = buff;
  *bp = CNULL;

  zero_pad = ZERO_PAD_DEFAULT;
  squeeze_cnt = 0;
  squeeze = FALSE;
  as_is = FALSE;
  separated = FALSE;

  while ((icase = *fmt++) != CNULL)
  {
    if (modifier)
    {
      /* Modifiers generate no output: */
      switch (icase)
      {
      default:
  /* G_format currently has no modifiers(!): */
  carrot_msg(NULL, "+G", G_format, "Unrecognized +G modifier", fmt-1);
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
        carrot_msg(NULL, "+G", G_format, "Too big", fmt-1);
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
    width = 0;
    width_specified = TRUE;
  }

  /* Process a directive: */
  switch (icase)
  {
  case Gf_TYPE_IN_QUIET:
  case Gf_TYPE_IN_ALPHA:
    {
      char type;
      if      (ftype == S_IFREG)  /* Regular */
        type = symtype_REG; /* sic! */
      else if (ftype == S_IFDIR)  /* Directory */
        type = type_DIR;
      else if (ftype == S_IFCHR)  /* Char Special */
        type = type_CHR;
      else if (ftype == S_IFBLK)  /* Block Special */
        type = type_BLK;
#ifdef S_IFIFO
      else if (ftype == S_IFIFO)  /* Fifo */
        type = type_FIFO;
#endif
#ifdef S_IFLNK
      else if (ftype == S_IFLNK) {  /* Symbolic Link */
        struct stat tinfo;
        int sts;
        type = type_LNK;
        // RWM get info of actual file
        char *path = full_name(dname, file->fname);
        if ( (sts=sigSafe_stat(path, &tinfo)) == 0 )
          fmode = tinfo.st_mode;
        else type = type_ORPH;
      }
#endif
#ifdef S_IFSOCK
      else if (ftype == S_IFSOCK) /* Socket */
        type = type_SOCK;
#endif
#ifdef S_IFDOOR
      else if (ftype == S_IFDOOR) /* Door */
        type = type_DOOR;
#endif
      else
        type = '?';

              if (!width_specified) width = 1;

              if ( rwm_docolor ) {
                rwm_type   = type;
                rwm_mode   = fmode;
              }
              // Gf_TYPE_IN_QUIET allows type processing without actual output
              if ( icase == Gf_TYPE_IN_ALPHA )
                sprintf(bp, "%*c",width, type);
            }
    break;

    /* Add various flavorings: */

#if defined(SUNOS4)
/* SunOS4.x is known to not mark fifos. */
#  define  FIFO_MARK  ' '
#else
/* Since SunOS only started marking fifos as of 5.5, it shouldn't hurt for all
   versions of SunOS5 to use it.  Linux marks fifos as early as 2.2 (maybe
   earlier?), HP-UX 10 (and maybe earlier) marks fifos. */
#  define  FIFO_MARK  symtype_FIFO
#endif

#if defined(HAVE_MANDATORY_LOCKING) && defined(SUNOS5)
/* As of SunOS5.9, Sun's native /bin/ls started listing directories having a
   mode of g+s,g-x as 'S'.  From SunOS5.0(?) through SunOS5.8 such
   directories were incorrectly listed as 'l'.  SunOS5.x Files having a mode
   of g+s,g-x have always been correctly listed as 'l'.  Mimic this behavior
   at runtime: */
#  define  S_OR_l     (ftype == S_IFDIR && osVersion >= 50900 ? 'S' : 'l')
#elif defined(HAVE_MANDATORY_LOCKING)
/* Anyone else supporting mandatory locking would list directories correctly
   one would imagine: */
#  define  S_OR_l     (ftype == S_IFDIR ? 'S' : 'l')
#else
#  define  S_OR_l     'S'
#endif

  case Gf_TYPE_IN_SYMBOLIC:
    {
      char type;
      if      (ftype == S_IFREG)  /* Regular */
        type = (fmode & (S_IXUSR|S_IXGRP|S_IXOTH) ? '*' : ' ');
      else if (ftype == S_IFDIR)  /* Directory */
        type = symtype_DIR;
      else if (ftype == S_IFCHR)  /* Char Special */
        type = ' ';
      else if (ftype == S_IFBLK)  /* Block Special */
        type = ' ';
#ifdef S_IFIFO
      else if (ftype == S_IFIFO)  /* Fifo */
        type = FIFO_MARK;
#endif
#ifdef S_IFLNK
      else if (ftype == S_IFLNK)  /* Symbolic Link */
        type = symtype_LNK;
#endif
#ifdef S_IFSOCK
      else if (ftype == S_IFSOCK) /* Socket */
        type = symtype_SOCK;
#endif
#ifdef S_IFDOOR
      else if (ftype == S_IFDOOR) /* Door */
        type = symtype_DOOR;
#endif
      else
        type = '?';

      if (!width_specified) width = 1;
      sprintf(bp, "%*c",width, type);
    }
    break;

#ifdef Gf_TYPE_IN_NUMERIC
  case Gf_TYPE_IN_NUMERIC:
    /* This is probably non-portable to some of the more obscure Unixes: */
    if (!width_specified) width = 2;
    sprintf(bp, "%*d",width, ftype >> 12);
    break;
#endif /*Gf_TYPE_IN_NUMERIC*/

  case Gf_PERM_IN_ALPHA:
    {
      char perm[16], *pp;
      pp = perm;
      if (fmode & S_IRUSR)
        *pp++ = 'r';
      else
        *pp++ = '-';

      if (fmode & S_IWUSR)
        *pp++ = 'w';
      else
        *pp++ = '-';

      if ((fmode & S_IXUSR) && (fmode & S_ISUID))
        *pp++ = 's';
      else if (fmode & S_ISUID)
        *pp++ = 'S';
      else if (fmode & S_IXUSR)
        *pp++ = 'x';
      else
        *pp++ = '-';

      if (fmode & S_IRGRP)
        *pp++ = 'r';
      else
        *pp++ = '-';

      if (fmode & S_IWGRP)
        *pp++ = 'w';
      else
        *pp++ = '-';

      if ((fmode & S_IXGRP) && (fmode & S_ISGID))
        *pp++ = 's';
      else if (fmode & S_ISGID)
        *pp++ = S_OR_l;
      else if (fmode & S_IXGRP)
        *pp++ = 'x';
      else
        *pp++ = '-';

      if (fmode & S_IROTH)
        *pp++ = 'r';
      else
        *pp++ = '-';

      if (fmode & S_IWOTH)
        *pp++ = 'w';
      else
        *pp++ = '-';

      if ((fmode & S_IXOTH) && (fmode & S_ISVTX))
        *pp++ = 't';
      else if (fmode & S_ISVTX)
        *pp++ = 'T';
      else if (fmode & S_IXOTH)
        *pp++ = 'x';
      else
        *pp++ = '-';

      *pp = CNULL;

      if (!width_specified) width = 9;
      sprintf(bp, "%*s",width, perm);
    }
    break;

  case Gf_PERM_IN_NUMERIC:
    if (!width_specified || width < 4) width = 4;
    sprintf(bp, "%0*o",width, (fmode & 07777));
    break;

  case Gf_PERM_IN_SYMBOLIC:
    {
      char perm[32], *pp;
      pp = perm;
      *pp++ = 'u'; *pp++ = '=';
      if (fmode & S_IRUSR) *pp++ = 'r';
      if (fmode & S_IWUSR) *pp++ = 'w';
      if (fmode & S_IXUSR) *pp++ = 'x';
      *pp++ = ','; *pp++ = 'g'; *pp++ = '=';
      if (fmode & S_IRGRP) *pp++ = 'r';
      if (fmode & S_IWGRP) *pp++ = 'w';
      if (fmode & S_IXGRP) *pp++ = 'x';
      *pp++ = ','; *pp++ = 'o'; *pp++ = '=';
      if (fmode & S_IROTH) *pp++ = 'r';
      if (fmode & S_IWOTH) *pp++ = 'w';
      if (fmode & S_IXOTH) *pp++ = 'x';

      {
        /* Pre-SunOS5.9 chmod complains if "g+s" given to a directory
     already having "g-x,g+s"; this is fixed as of SunOS5.9:

     mkdir -p test; chmod g=rwx,g+s test  # Starting condition
     chmod g=,g+s test  # SunOS5.7==FAILS, SunOS5.9==OK
     chmod g=,g-s,g+s test  # SunOS5.7==OK, SunOS5.9==OK
     chmod g=,g-s,+l test # SunOS5.7==OK, SunOS5.9==OK
     chmod g=x,g+s test # SunOS5.7==OK, SunOS5.9==OK
     chmod g-x test   # SunOS5.7==FAILS, SunOS5.9==OK

     Thus, the following two sequences work on all OS versions:
     chmod g=,g-s,+l test
     chmod g=,g-s,g+s test

     NB: All versions of SunOS5 have treated "g+s" the same as
     "+l" for directories, but "g+s" is more universal.
     */
#ifdef HAVE_MANDATORY_LOCKING
        Boole lock = (fmode & S_ISGID) &&
    !(fmode & S_IXGRP) && (ftype != S_IFDIR);
        Boole setgid = (fmode & S_ISGID) &&
    ((fmode & S_IXGRP) || (ftype == S_IFDIR));
#else
        Boole lock = FALSE;
        Boole setgid = (fmode & S_ISGID) != 0;
#endif
        Boole setuid = (fmode & S_ISUID) != 0;
        Boole sticky = (fmode & S_ISVTX) != 0;

        /* SunOS (and possibly others) do not remove "s" on directories
     when "g=" or 700 modes specified, thus remove explicitly: */
        if (ftype == S_IFDIR && !(fmode & S_IXGRP))
        {
    *pp++ = ','; *pp++ = 'g'; *pp++ = '-'; *pp++ = 's';
        }

        if (setuid || setgid)
        {
    *pp++ = ',';
    if (setuid)  *pp++ = 'u';
    if (setgid)  *pp++ = 'g';
    *pp++ = '+'; *pp++ = 's';
        }

        if (lock || sticky)
        {
    *pp++ = ','; *pp++ = '+';
    if (lock)    *pp++ = 'l';
    if (sticky)  *pp++ = 't';
        }
      }

      *pp = CNULL;

      /* Worst cases: u=rwxs,g=rwxs,o=rwx,+t    (files) */
      /*              u=rwx,g=rwx,o=rwx,ug+s,+t (all) */
      /*              u=rwx,g=rw,o=rwx,u+s,+lt  (sunos5 files) */
      /*              u=rwx,g=rwx,o=rwx,u+s,g-s,+t (dir without g+s) */
      /*              1234567890123456789012345 */
      if (!width_specified) width = -25;
      sprintf(bp, "%*s",width, perm);
    }
    break;

  case Gf_ACL_INDICATOR:
    if (!width_specified) width = 1;
    sprintf(bp, "%*s",width,
      get_acl_count(file, full_name(dname, file->fname)) > 0 ?
      "+" : "");
    break;

  case Gf_INODE:
    /* SYS5-R2 == 5
       SunOS4.1.3 == 6, SunOS5.0-5.5.1 == 5(!), SunOS5.6-5.8 == 10
       HPUX10.20 == 6
       Linux2.2 == 7 */
    /* Most modern UNIXes assume inodes take at least 6 places: */
    if (!width_specified) width = 6;
    sprintf(bp, F_st_ino(zero_pad,width, info->st_ino));
    break;

  case Gf_SIZE_HUMAN_2:
  case Gf_SIZE_HUMAN_10:
  case Gf_SIZE_IN_BYTES:
    if (!width_specified)
    {
      if (icase == Gf_SIZE_HUMAN_2 || icase == Gf_SIZE_HUMAN_10)
        width = 7;
      else
        width = (ANYBIT(Sem,SEM_BSD|SEM_GNU|SEM_ELS) ? 8 : 7);
    }

    if (file->isdev)
    {
      char major_dev[16], minor_dev[16], str[32];
      sprintf(major_dev, "%lu", (Ulong)major(info->st_rdev));
      sprintf(minor_dev, "%lu", (Ulong)minor(info->st_rdev));
      /* Leading spaces are added to the major field for device numbers
         less than 100 (unless squeeze or width specified) so as to mimic
         /bin/ls when the minor field exceeds 999 (or 9999 for BSD) thus
         keeping it from pushing its way into the major field.  If true
         mimicing of BSD's or SYS5's /bin/ls is desired when listing
         devices, then specify either +4l or +5l flags respectively. */
      sprintf(str, "%*s,%*s",
        (squeeze || width_specified ? 0 : 3),
        major_dev,
        (squeeze ? 0 : (ANYBIT(Sem,SEM_BSD|SEM_GNU|SEM_ELS) ? 4 : 3)),
        minor_dev);
      sprintf(bp, "%*s",width, str);
    }
    else if (icase == Gf_SIZE_HUMAN_2)
      sprintf(bp, "%*s",width, scaleSizeToHuman(info->st_size, 2));
    else if (icase == Gf_SIZE_HUMAN_10)
      sprintf(bp, "%*s",width, scaleSizeToHuman(info->st_size, 10));
    else {
      if ( !rwm_docomma )
      sprintf(bp, F_st_size(zero_pad,width, info->st_size));
//      sprintf(bp, F_st_size(zero_pad,width), info->st_size);
      else {
        char str[32];
        int i, sz, firstblock = 1;
        unsigned long long x, tmp1, tmp2;

        width = rwm_szwdth ? rwm_szwdth : width+7;
        tmp1 = info->st_size;
        sz = (tmp1 != 0) ? (int) log10( (double) info->st_size) : 0;
        i= 3 * (int) (sz/3);
        str[0] = '\0';
        for (; i>=0; i -= 3) {
          x = (int) pow( (double)(10), (double)(i));
          tmp2 = (x != 0) ?  tmp1/ x : 0;
          if (firstblock) {
            Void sprintf(str, "%s%*llu%s",  str, 3, tmp2, i==0 ? "" : "," );
            firstblock=0;
          } else
            Void sprintf(str, "%s%0*llu%s", str, 3, tmp2, i==0 ? "" : "," );
          tmp1 -= (tmp2 * x);
        }
        if ( width < (int) strlen( str ) )
          rwm_fxwdth = MAX( rwm_fxwdth, (int) strlen( str ) );
        if ( !FSCOLOR ) Void sprintf(bp, "%*s", width, str);
        else            Void sprintf(bp, "[%sm%*s[39m", FSCOLOR, width, str);

      }
    }
    break;

  case Gf_SIZE_IN_BLOCKS:
    {
      ELS_st_blocks st_blocks;
      /* Emulate weirdo-cases where BSD's and SysV's sum commands return
         the filesize disguised to look like the real block count (i.e.
         indirect blocks are not included in sum's "block count"): */
      if (cksumming && !file->isdev && (cksum_type == CKSUM_BSD ||
                cksum_type == CKSUM_SYSV))
      {
        if (ANYBIT(Sem,SEM_BSD|SEM_GNU|SEM_ELS))
    st_blocks = (info->st_size + 1023)/512; /* /2 occurs later! */
        else
    st_blocks = (info->st_size + 511)/512;
      }
      else
        st_blocks = getStatBlocks(&file->info);

      if (!width_specified) width = 4;
      sprintf(bp, FU_st_blocks(zero_pad,width,
        ANYBIT(Sem,SEM_BSD|SEM_GNU|SEM_ELS) ?
        st_blocks/2 : st_blocks));
    }
    break;

  case Gf_LINK_COUNT:
    /* ELS version > 1.45 always uses 3 for link count and reserves
       1 for possible ACL unless LS and BSD (i.e. +l4) specified.
       ELS version <= 1.45 uses 2 for link count except where support
       for ACLs exists or LS and not BSD (i.e. +l) specified. */
    if (!width_specified) width = (ALLBITS(Sem,SEM_BSD|SEM_LS) ? 2 : 3);
# if !defined(HAVE_ACL)
    if (VersionLevel <= 145 && !width_specified)
      width = (ANYBIT(Sem,SEM_BSD|SEM_ELS) ? 2 : 3);
# endif

    // 2021-10-16 Modified to support color and show directory
    // entries, not including . and .. (hence the -2)
    // single original line
//  sprintf(bp, F_st_nlink(zero_pad,width, info->st_nlink));

    char str[32];
    memset( str, '\0', 32 );
    sprintf(str, F_st_nlink(zero_pad,width,
      info->st_nlink - ( file->isdir ? 2 : 0 )));

    if ( !FSCOLOR ) Void sprintf(bp, "%*s", width, str);
    else            Void sprintf(bp, "[%sm%*s[39m", FSCOLOR, width, str);
    // End new code

    break;

  case Gf_TIME_MODIFIED:
  case Gf_TIME_ACCESSED:
  case Gf_TIME_MODE_CHANGED:
          if ( rwm_docolor && rwm_ftcnt )
            bp = rwm_col_age( bp, info->st_mtime, 1 );
    bp = T_print_width(bp, T_format,
           icase == Gf_TIME_MODIFIED ? info->st_mtime :
           icase == Gf_TIME_ACCESSED ? info->st_atime :
           /*icase == Gf_TIME_MODE_CHANGED ?*/ info->st_ctime,
           NULL, FALSE, FALSE, width);
          if ( rwm_docolor && rwm_ftcnt )
            bp = rwm_col_age( bp, info->st_mtime, 0 );
    break;

  case Gf_UID_IN_ALPHA:
    {
      char *pw_name = uid2name(info->st_uid);
      int trunc = MAX_USER_GROUP_NAME;
      if (!width_specified) width = -10;     // 2019-03-17 RWM - was -8 (rwmitchell)
      if (TruncateName && width != 0)
        trunc = (width > 0 ? width : -width);
      if (pw_name != NULL)
        sprintf(bp, "%*.*s",width,trunc, pw_name);
      else
        /* No name, print the number instead: */
        sprintf(bp, FU_uid_t(zero_pad,width, uidMask(info->st_uid)));
    }
    break;

  case Gf_GID_IN_ALPHA:
    {
      char *gr_name = gid2name(info->st_gid);
      int trunc = MAX_USER_GROUP_NAME;
      if (!width_specified) width = -8;
      if (TruncateName && width != 0)
        trunc = (width > 0 ? width : -width);
      if (gr_name != NULL)
        sprintf(bp, "%*.*s",width,trunc, gr_name);
      else
        /* No name, print the number instead: */
        sprintf(bp, FU_gid_t(zero_pad,width, gidMask(info->st_gid)));
    }
    break;

  case Gf_UID_IN_NUMERIC:
    if (!width_specified) width = 5;
    sprintf(bp, FU_uid_t(zero_pad,width, uidMask(info->st_uid)));
    break;

  case Gf_GID_IN_NUMERIC:
    if (!width_specified) width = 5;
    sprintf(bp, FU_gid_t(zero_pad,width, gidMask(info->st_gid)));
    break;

  case Gf_OWNER_IN_ALPHA:
    {
      char tmp[2*MAX_USER_GROUP_NAME + 2];
      char *fmt = (GTarStyle || Tar5Style ? "%1u/%-g" : "%8u:%-g");
      if (!width_specified) width = -17;
      G_print(tmp, fmt, dname, file);
      sprintf(bp, "%*s",width, tmp);
      break;
    }

  case Gf_OWNER_IN_NUMERIC:
    {
      char tmp[4*ELS_SIZEOF_uid_t + 4*ELS_SIZEOF_gid_t + 2];
      char *fmt = (GTarStyle || Tar5Style ? "%1U/%-G" : "%6U:%-G");
      if (!width_specified) width = -13;
      G_print(tmp, fmt, dname, file);
      sprintf(bp, "%*s",width, tmp);
      break;
    }

  case Gf_CHECKSUM:
    /* Perform checksum on regular files and directories: */
    {
      Uint32 cksum = 0;

      if (ftype == S_IFREG)
      {
        int save_errno = errno;
        char *path = full_name(dname, file->fname);

        /* Perform checksum and record any errno generated by cksum: */
        errno = 0;
        cksum = cksumFile(path, cksum_type, NULL);
        if (errno != 0)
        {
    cksum = cksumErrorCode(cksum_type);
    file->stat_errno = errno;  /* Record errno */
        }
        else if (cksum_unaccess)
        {
    /* "Attempt" to restore access time (ignore any errno): */
    struct utimbuf tb;
    tb.actime = info->st_atime;
    tb.modtime = info->st_mtime;
    utime(path, &tb);
    /* ??? if (verboseLevel > 0 && errno != 0) then warn about
       access time being clobbered ??? */
        }
        errno = save_errno;
      }

      else if ((zero_st_mask & ZERO_CKSUM_NONREG) != 0)
      {
        /* Zero cksum of non-regular file as requested: */
        cksum = 0;
      }

      else
      {
        /* Generate cksum ERROR for any file not handled above: */
        cksum = cksumErrorCode(cksum_type);
        file->info.st_size = 0; /* FIXME: this mimics cksum!? */
      }

      /* if CKSUM_BSD   then  zero_pad = TRUE, width = 5
         if CKSUM_SYSV  then  zero_pad = optional, width = 5
         if CKSUM_POSIX then  zero_pad = optional, width = 10
         if CKSUM_CRC32 then  zero_pad = optional, width = 10 */
      if (!width_specified) width = (cksum_size == 16 ? 5 : 10);
      if (cksum_type == CKSUM_BSD) zero_pad = TRUE;
      sprintf(bp, F_32U(zero_pad,width, cksum));
    }
    break;

  case Gf_NAME_FORMAT:
  case Gf_FULL_NAME_FORMAT:
  case Nf_FULL_NAME:
  case Nf_DIR_NAME:
  case Nf_FILE_NAME:
    {
      char *N_format_to_use;
      char N_format_case[2];
      if (icase == Gf_NAME_FORMAT || icase == Gf_FULL_NAME_FORMAT)
      {
        N_format_to_use = N_format;
      }
      else
      {
        N_format_case[0] = icase;
        N_format_case[1] = CNULL;
        N_format_to_use = N_format_case;
      }

      if (!width_specified)
        bp = N_print(bp, N_format_to_use, dname, file);
      else
      {
        char tmp[MAX_FULL_NAME];
        Void N_print(tmp, N_format_to_use, dname, file);
        sprintf(bp, "%*s",width, tmp);
      }
    }
    break;

  case Nf_LINK_PTR_NAME:
#     if defined(S_IFLNK)
    if (file->islink && !expand_symlink)
    {
      char *N_format_to_use;
      char N_format_case[2];

      N_format_case[0] = icase;
      N_format_case[1] = CNULL;
      N_format_to_use = N_format_case;

      if (!width_specified)
        bp = N_print(bp, N_format_to_use, dname, file);
      else
      {
        char tmp[MAX_FULL_NAME];
        Void N_print(tmp, N_format_to_use, dname, file);
        sprintf(bp, "%*s",width, tmp);
      }
    }
#     endif
    break;

  default:
    if (isAlnum(icase))
    {
      carrot_msg(NULL, "+G", G_format, "Unrecognized +G directive", fmt-1);
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

  /* Remove trailing spaces caused by marking files and/or symbolic
     links.  This is caused by: 1) the -F option when marking a regular
     file 2) the +GL/+NL option when listing a non-symbolically linked
     file.  Regrettably, this removes legitimate trailing spaces from
     the end of file names, etc.: */
  while (bp != buff && isSpace(bp[-1])) bp--;
  *bp = CNULL;

  if (quote_mode) errorMsgExit("Missing delimiter in +G format", USAGE_ERROR);
  if (!directive_seen) errorMsgExit("Incomplete +G format", USAGE_ERROR);

  return(bp);
}
#undef  ZERO_PAD_DEFAULT

Boole rwm_get_cs( char *pat, int *b, int *f, int *s, int *i ) { // background, foreground, style, icon
  char *mat = NULL;
  int d = 0;                          // distance

  if ( !rwm_doicons ) {
    mat = strstr( LSCOLOR, pat );     // use LSCOLOR for colors
    if ( mat ) mat = strchr( mat, '=');
    if ( mat ) d=sscanf( mat, "=%d;%d;%d", b, f, s );
    if ( *s == 0 ) *s = 8;            // 0 turns color off, 8 is normal 2020-11-10 true?
  } else {
    *f =  7;
    *s =  0;
    mat = strstr( LSICONS, pat );     // use LSICONS only for icons
    if ( mat ) mat = strchr( mat, '=' );
    if ( mat ) {
      d=sscanf( mat, "=%d;%d;%d;%lc:", f, b, s, i );  // f / b order reversed
    }
//    printf( "\nB: %3d F: %3d S: %2d : < %lc >\n", *b, *f, *s, *i );
  }

  return ( mat != NULL );
}
Boole rwm_get_hg( char pat, int *b, int *i ) {                 // background, icon
  char *mat = NULL;
  int d = 0;                          // distance

  if ( HGICONS ) {
    mat = strchr( HGICONS, pat );
    if ( mat ) mat = strchr( mat, '=' );
    if ( mat ) {
      d=sscanf( mat, "=%d;%lc:", b, i );  // f / b order reversed
    }
//    printf( "\nB: %3d F: %3d S: %2d : < %lc >\n", *b, *f, *s, *i );
  }

  return ( mat != NULL );
}
Boole rwm_get_git( char pat, int *b, int *i ) {                 // background, icon
  char *mat = NULL;
  int d = 0;                          // distance

  if ( GTICONS ) {
    mat = strchr( GTICONS, pat );
    if ( mat ) mat = strchr( mat, '=' );
    if ( mat ) {
      d=sscanf( mat, "=%d;%lc:", b, i );  // f / b order reversed
    }
//    printf( "\nB: %3d F:     S:     : < %lc >\n", *b, *i );
  }

  return ( mat != NULL );
}
Boole rwm_col_type( int *b, int *f, int *s, int *i ) {
  Boole rc = TRUE;
  char *pat=NULL;

  if ( !rwm_doicons ) {
    switch( rwm_type ) {
      case    type_DIR :  pat="di="; break;
      case    type_CHR :  pat="cd="; break;  // character device
      case    type_BLK :  pat="bd="; break;  // block     device
      case    type_FIFO:  pat="pi="; break;  // pipe
      case    type_LNK :  pat="ln="; break;
      case    type_SOCK:  pat="so="; break;
      case    type_DOOR:  pat="do="; break;
  //  case    type_orph:  pat="or="; break;  // symlink orphan, no type?
      case type_SPECIAL:  pat="or="; break;  // special - no LS_COLOR attribute?

      case symtype_REG :
                          if ( rwm_mode & (S_IXUSR|S_IXGRP|S_IXOTH)) pat="ex=";
                          if ( rwm_mode & S_ISUID )                  pat="su=";
                          if ( !pat ) rc = FALSE;
                          break;
      case symtype_DIR :
      case symtype_FIFO:
      case symtype_LNK :
      case symtype_SOCK:
      case symtype_DOOR:
      case    type_REG :
      default: rc = FALSE; break;
    }
  } else {
    switch( rwm_type ) {
      case    type_DIR : pat="DIRECTORY="; break;
      case    type_CHR :  pat="CHARDEV=";  break;  // character device
      case    type_BLK :  pat="BLOCKDEV="; break;  // block     device
      case    type_FIFO:  pat="PIPE=";     break;  // pipe
      case    type_LNK :  pat="LINK=";     break;
      case    type_SOCK:  pat="SOCKET=";   break;
      case    type_DOOR:  pat="DOOR=";     break;
      case    type_ORPH:  pat="ORPHAN="; break;  // symlink orphan, no type?
      case    type_REG :  pat="FILE=";     printf("\n\nREGULAR\n\n"); break;   // never happens
      case type_SPECIAL:  pat="or=";       break;  // special - no LS_COLOR attribute?

      case symtype_REG :
                          if ( rwm_doperms
                            && rwm_mode & (S_IXUSR|S_IXGRP|S_IXOTH)) pat="EXEC=";
                          if ( rwm_mode & S_ISUID )                  pat="SETUID=";
                          if ( !pat ) rc = FALSE;
                          break;
      case symtype_DIR :
      case symtype_FIFO:
      case symtype_LNK :
      case symtype_SOCK:
      case symtype_DOOR:
      default: rc = FALSE; break;
    }

  }

  rwm_type = 0;         // initialize it before called for symlink
  if ( pat ) rc = rwm_get_cs( pat, b, f, s, i );

  return( rc );
}
Boole rwm_col_ext1( char *fn, int *b, int *f, int *s, int *i ) {
  // show .ORIG extension icon (with .sssssssss added )
  char *ext = NULL,
        pat[256],
        vs[32];
  Boole rc = FALSE;
  int v=0;

  ext = strrchr( fn, '.' );
  if ( ext ) {
    sscanf( (ext+1), "%d", &v );
    sprintf(vs, ".%d", v);
    if ( v > 100 && strncmp( ext, vs, strlen( vs )) == 0 ) {
      rc = rwm_get_cs( ".ORIG", b, f, s, i );
    } else {
      sprintf( pat, "%.13s=", ext );     // 2020-11-10 removed leading asterick '*'
      ext = strchr( pat, '.' );
      char *ps = pat;
      if ( ext != pat ) *ext = '\0';
      while ( *ps != '\0' ) { *ps = toupper( *ps ); ++ps; }
      rc = rwm_get_cs( pat, b, f, s, i );
    }
  }
  return( rc );
}
Boole rwm_col_ext2( char *fn, int *b, int *f, int *s, int *i ) {
  // show original extension icon (with .sssssssss added )
  char *ext = NULL,
       *tfn = strdup( fn ),
        pat[256],
        vs[32];
  Boole rc = FALSE;
  int v=0;

  ext = strrchr( tfn, '.' );
  if ( ext ) {
    sscanf( (ext+1), "%d", &v );
    sprintf(vs, ".%d", v);
    if ( v > 100 && strncmp( ext, vs, strlen( vs )) == 0 ) {
      *ext='\0';
      ext--;
      ext = strrchr( tfn, '.' );
    }
  }
  if ( ext ) {
    sprintf( pat, "%.13s=", ext );     // 2020-11-10 removed leading asterick '*'
    ext = strchr( pat, '.' );
    char *ps = pat;
    if ( ext != pat ) { *ext = '\0'; printf( "NEW: %s\n", pat ); }
    while ( *ps != '\0' ) { *ps = toupper( *ps ); ++ps; }
    rc = rwm_get_cs( pat, b, f, s, i );
  }
  free( tfn  );
  return( rc );
}
Boole rwm_col_ext3( char *fn, int *b, int *f, int *s, int *i ) {
  // keep searching for a match, ie: file.tar.baz  - match on .tar
  char *ext = NULL,
       *ps  = NULL,
       *tfn = strdup( fn ),
        pat[32];
  Boole rc   = FALSE,
        done = FALSE;

  while ( ! done ) {

    ext = strrchr( tfn, '.' );

    if ( ext ) {
      sprintf( pat, "%.13s=", ext );     // 2020-11-10 removed leading asterick '*'
      ps = pat;
      while ( *ps != '\0' ) { *ps = toupper( *ps ); ++ps; }
      done = rc = rwm_get_cs( pat, b, f, s, i );
      if ( ! done ) *ext = '\0';
    } else done = TRUE;                  // give up

  }
  free( tfn  );
  return( rc );
}
Boole rwm_col_name( char *fn, int *b, int *f, int *s, int *i ) {
  char  pat[256];
  Boole rc = FALSE;

  // 2020-11-08 truncing filename fixes finding match on whole filenames,
  // such as CHANGE_LOG, INSTALL, Makefile
//    ext = strrchr( fn, '/' );
//    if ( !ext ) ext=fn;
//    printf( "EXT: |%s|(%s)\n", ext, fn );
//    else ext++;

  sprintf( pat, "%s=", fn );
  char *ps = pat;
  while ( *ps != '\0' ) { *ps = toupper( *ps ); ++ps; }
//printf("PAT: |%s|\n", pat);
  rc = rwm_get_cs( pat, b, f, s, i );
  return( rc );
}
Boole rwm_col_wild( char *fn, char y, int *b, int *f, int *s, int *i ) {
  const
  char *pls = NULL,
       *ps;
  char  pat[32],
       *pt,
       *tfn = strdup( fn );
  int   d = 0;
  Boole done = FALSE,
        mtch = FALSE;

  // reverse search method
  //   find '*' in LSCOLOR/LSICON,
  //   get associated pattern
  //   find pattern _inside_ filename
  //   assume format of:   :*PAT=

  pt = tfn;
  while ( *pt != '\0' ) { *pt = toupper( *pt ); ++pt; }

  pls = rwm_doicons ? LSICONS : LSCOLOR;

  while ( !done ) {
    pls = strchr( pls, y );           // y='*', ''
    if ( pls ) {
      pls++;      // move past 'y' or pattern type char

//    sscanf( pls, "%31s=", pat );    // sscanf() does NOT stop at =
      ps = pls;
      pt = pat;
      int l=31;
      while ( *ps != '=' ) { *pt++ = *ps++; l--; }
      *pt = '\0';

      // 2021-02-14 support wildcard at end of pattern
      if ( *pat == '\0' ) {           // wildcard appears at end
        while ( *ps != ':' ) ps--;    // rewind to start of pattern
        ps++;                         // move past ':'
        while ( *ps != y ) { *pt++ = *ps++; l--; }
        *pt = '\0';
      }

//    printf( "Wild %c: |%s| [%s]\n", y, tfn, pat );
      pt = strstr( tfn, pat );
#ifdef EXACT_MATCH    // breaks matching *rc for .zshrc-path
      if ( pt && strlen( pt ) == strlen ( pat )) mtch = TRUE;
#else
      if ( pt ) mtch = TRUE;        // found pat in filename
#endif
      if ( mtch ) {
        pls = strchr( pls, '=' );
        if ( pls ) {
          if ( ! rwm_doicons )
            d=sscanf( pls, "=%d;%d;%d", b, f, s );
          else
            d=sscanf( pls, "=%d;%d;%d;%lc:", f, b, s, i );  // f / b order reversed
          done = ( d >= 2 ) ? TRUE : FALSE;
        }
      }
    } else done = TRUE;
  }
  free( tfn );

  return( d > 0 );
}
void rwm_get_col( char *fn, int *b, int *f, int *s, int *i ) {
  *b = *f = *s = 0;
  Boole rc = FALSE;

//printf( "COL: %d: %s\n", rwm_type == type_DIR, fn );
  if ( rwm_type == type_DIR )
     rc = ( rwm_col_wild( fn, '^', b, f, s, i ) );
  if      ( rc );
  else if ( rwm_col_type(          b, f, s, i ) );    // DIR, SOCKET, SUID, etc
  else if ( rwm_col_ext ( fn,      b, f, s, i ) );    // file extension
  else if ( rwm_col_name( fn,      b, f, s, i ) );    // entire name
  else if ( rwm_col_wild( fn, '*', b, f, s, i ) );    // find pat in name
  else if ( rwm_col_ext1( fn,      b, f, s, i ) );    // retry looking for cpbu files
  else      rwm_get_cs  ( "FILE=", b, f, s, i );      // default
}
char *rwm_dir_col( char *dnam ) {
  static char dcol[512];    // MAXNAMLEN
  int  rwm_b, rwm_f, rwm_s, rwm_i,
       type = rwm_type;
  char rwm_col[512],     // MAXNAMLEN plus cushion for escape codes
       rwm_tmp[512],
       rwm_bg [ 32],
      *tn = strdup( dnam ),
      *ps = tn,
      *pe = tn+1;
  Boole done = FALSE;

//printf( "rwm_dir_col: >%s<\n", tn);

  memset( dcol, '\0', 512 );

  while ( ! done ) {
    pe = strchr( pe, '/' );
    if ( pe ) *pe = '\0';
    else done = TRUE;

    rwm_type = type_DIR;     // something resets rwm_type in this loop
//  printf( "coloring: %d >%s<\n", rwm_type == type_DIR, ps );

    rwm_get_col( ps, &rwm_b, &rwm_f, &rwm_s, &rwm_i );
//    printf( "Coloring: >%s< %d\n", ps, rwm_f );

    if ( rwm_b >  0  ) sprintf( rwm_bg,    "[48;5;%dm",        rwm_b );
    else               rwm_bg[0] = '\0';

    // Foreground color
    if ( rwm_s <= 0 ) sprintf( rwm_col, "%s[38;5;%dm",    cs, rwm_f );
    else              sprintf( rwm_col, "%s[38;5;%d;%dm", cs, rwm_f, rwm_s );
    strcat( rwm_col, rwm_bg );

    sprintf(rwm_tmp, "%s%s%s%c", rwm_col, ps, cs, pe ? '/' : '\0' );
    strcat( dcol, rwm_tmp );

    ps = ++pe;
  }
//printf( "rwm_dir_col: >%s< | <%s>\n", tn, dcol );
  rwm_type = type;

  if ( tn ) free( tn );
  return( dcol );
}

#define ZERO_PAD_DEFAULT  FALSE
char *N_print(char *buff, char *fmt,
        char *dname, Dir_Item *file)
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
  Boole ttoggle = FALSE;
  Boole use_quotes = plus_q;
  char *fname = file->fname;  /* Use "fname" to avoid side-effects! */
  char *slash = NULL;
  Boole markable = FALSE;

  /* NB: You must reference "fname" instead of "file->fname" for the
     duration of this procedure to avoid probable side-effects! */

  /*****************************************************/
  /* Put dir name and/or file name into canonical form */
  /*****************************************************/

  if (using_full_names)
  {
    if (strcmp(dname, "/") == 0)
    {
      /* Do nothing */;
    }
    else if ((slash = strrchr(fname, '/')) != NULL)
    {
      if (slash == fname)
  dname = "/";
      else
  dname = fname;
      *slash = CNULL;
      fname = slash+1;
    }
    else
    {
      int last_dir_char = strlen(dname) - 1;
      if (last_dir_char > 0 && dname[last_dir_char] == '/')
      {
  slash = &dname[last_dir_char];
  *slash = CNULL;
      }
    }
  }

  bp = buff;
  *bp = CNULL;

  zero_pad = ZERO_PAD_DEFAULT;
  separated = FALSE;

  while ((icase = *fmt++) != CNULL)
  {
    if (modifier)
    {
      /* Modifiers generate no output: */
      switch (icase)
      {
      case 'Q':   /* '+N^Q' is DEPRECATED, use '+N^q' instead */
      case Nf_QUOTE_NAME:
  use_quotes = TRUE;
  break;

      default:
  carrot_msg(NULL, "+N", N_format, "Unrecognized +N modifier", fmt-1);
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
        carrot_msg(NULL, "+N", N_format, "Too big", fmt-1);
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

      case 'Q':   /* '+NQ' is DEPRECATED, use '+N^q' instead */
      case Nf_QUOTE_NAME: /* '+Nq' is DEPRECATED, use '+N^q' instead */
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
  }
  break;  /* modifiers generate no output */

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
    width = 0;
    width_specified = TRUE;
  }

        char rwm_col[128],
             rwm_bg [ 32],
             rwm_gl [ 16];
        int rwm_b = 49, rwm_f = 39, rwm_s=29,   // back, fore, and style
             hg_b =  0,
             gt_b =  0;
        Boole rc = FALSE;
        wchar_t rwm_i = ' ',   // 0xf118;   // happy face
                 hg_i = ' ',
                 gt_i = ' ';

        char hg = '\0',
             gt = '\0';
        if ( hg_stat ) hg =  get_hgstatus( dname, fname, hg_stat);
        if ( HGICONS && hg != ' ' ) rc =    rwm_get_hg ( hg, &hg_b, &hg_i);
        if ( !rc ) hg_i = (wchar_t) hg;

        if ( gt_stat ) gt =  get_gitstatus( dname, fname, gt_stat);


        if ( GTICONS && gt != ' ' ) rc =    rwm_get_git( gt, &gt_b, &gt_i);
        if ( !rc ) gt_i = (wchar_t) gt;

        if ( rwm_docolor ) {
//        printf( "START: %lc:%lc:\n", 0x42, 0xf118 );
//        printf( "rwm_i: %0x:%lc:\n", rwm_i, rwm_i );

          rwm_get_col( fname, &rwm_b, &rwm_f, &rwm_s, &rwm_i );

//        printf( "rwm_I: %0x:%lc:\n", rwm_i, rwm_i );
          if ( rwm_doicons ) {
            if ( hg_stat ) sprintf ( rwm_gl, "%lc %lc  ", hg_i, rwm_i );
            if ( gt_stat ) sprintf ( rwm_gl, "%lc %lc  ", gt_i, rwm_i );
            else           sprintf ( rwm_gl, "%lc  ",           rwm_i );
          }
          else               rwm_gl[0] = '\0';

          // Background color
          if ( hg_b ) rwm_b = hg_b;
          if ( gt_b ) rwm_b = gt_b;
          if ( rwm_b >  0  ) sprintf( rwm_bg,    "[48;5;%dm", rwm_b );
//        if ( hg == 'M'   ) sprintf( rwm_bg,    "[48;5;%dm",      8 );
          else               rwm_bg[0] = '\0';

          // Foreground color
          if ( rwm_s <= 0 ) sprintf( rwm_col, "%s[38;5;%dm",    cs, rwm_f );
          else              sprintf( rwm_col, "%s[38;5;%d;%dm", cs, rwm_f, rwm_s );
          strcat( rwm_col, rwm_bg );
        } else rwm_col[0] = '\0';

  /* Process a directive: */
  switch (icase)
  {
  case Nf_FULL_NAME:
    {
      static char *dlast = NULL,
                  *dcolr = NULL;
      char *d, *s;

      if (strcmp(dname, "/") == 0)
      {
        d = ""; s = "/";
      }
      else if (dname[0] != CNULL && strcmp(dname, ".") != 0)
      {
        d = dname; s = "/";
//      printf( "DNAME: %s\n", d );

//      printf( "Pre: >%s< | <%s>\n", dname, dlast );
        if ( !dlast || strcmp( dname, dlast) ) {     // has dname changed?
//        printf( "Start\n");
          if ( dlast ) free( dlast );
          if ( dname ) dlast = strdup( dname );

          dcolr = rwm_dir_col( dlast );
//        printf ("Dir: >%s%s< [%s] : %d\n", dcolr, cs, fname, rwm_f );
        }
          d = dcolr;

      }
      else
      {
        d = ""; s = "";
      }

      if (!width_specified) {
        sprintf(bp, "%s%s%s%s%s%s%s", rwm_col, rwm_gl, cs, d, s, rwm_col, fname);
        if ((d = quote_fname(bp, dash_b, dash_Q, use_quotes)) != NULL)
          sprintf(bp, "%s", d);
      } else {
        char tmp[MAX_FULL_NAME];
        sprintf(tmp, "%s%s%s%s%s%s%s", rwm_col, rwm_gl, cs, d, s, rwm_col, fname);
        if ((d = quote_fname(tmp, dash_b, dash_Q, use_quotes)) != NULL)
          sprintf(bp, "%*s",width, d);
        else
          sprintf(bp, "%*s",width, tmp);
      }
      markable = TRUE;
    }
    break;

  case Nf_DIR_NAME:
    {
      char *d;
      if (dname[0] == CNULL) dname = ".";
      if ((d = quote_fname(dname, dash_b, dash_Q, use_quotes)) != NULL)
        sprintf(bp, "%*s",width, d);
      else
        sprintf(bp, "%*s",width, dname);
      markable = TRUE;
    }
    break;

  case Nf_FILE_NAME:
    {
      char *f;
      if ((f = quote_fname(fname, dash_b, dash_Q, use_quotes)) != NULL)
        sprintf(bp, "%s%*s",rwm_col, width, f);
      else
        sprintf(bp, "%s%*s",rwm_col, width, fname);
      markable = TRUE;
    }
    break;

  case Nf_LINK_PTR_NAME:
  case Nf_LINK_NAME:
#     if defined(S_IFLNK)
    if (file->islink && !expand_symlink)
    {
      int i;
      char lname[MAX_FULL_NAME];
      char *fmt;
      i = readlink(full_name(dname, fname),
       lname, MAX_FULL_NAME);
      if (i < 0 || i >= MAX_FULL_NAME) i = 0;
      lname[i] = CNULL;
      if (fielding || icase == Nf_LINK_NAME)
        fmt = "%*s";
      else
        fmt = (squeeze ? "->%*s" : "-> %*s");
      {
        char *l;
        if ((l = quote_fname(lname, dash_b, dash_Q, plus_q)) != NULL)
    sprintf(bp, fmt,width, l);
        else {

                // 49 is default BG color
                // 39 is default FG color
                // 29 might be default style
                if ( rwm_docolor ) {            // XYZZY - fix symlink dest color
                  int b = 49, f = 39, s = 29;   // back, fore, and style
                  int icon=' ';
                  char tname[MAX_FULL_NAME];
                  rwm_get_col( lname, &b, &f, &s, &icon );
                  sprintf( tname, "[%d;%d;%dm%s[39;49m", b, f, s, lname );
                  sprintf(bp, fmt,width, tname);
                } else
                  sprintf(bp, fmt,width, lname);

              }
      }

# ifdef OBSOLETE_BEHAVIOR
      /* This is how SunOS5.x, Linux 2.2.x, Linux 2.4.x, Linux 2.6.9
         mark symlinks via -F (-p is ignored): */
      if (mark_files || mark_dirs)
      {
        /* Mark file using info from actual file rather than link: */
        Dir_Item tmp;
        tmp = *file;
        if (sigSafe_stat(full_name(dname, tmp.fname), &tmp.info) == 0)
        {
    bp += strlen(bp); /* Find where we are */
    if (mark_files)
      bp = G_print(bp, "T", dname, &tmp);
    else if (mark_dirs && S_ISDIR(tmp.info.st_mode))
    {
      *bp++ = '/';
      *bp = CNULL;
    }
        }
        markable = FALSE;
      }
# else
      /* This is how SunOS4.x and Linux 2.6.18 mark symlinks
         via -F and -p: */
      if (mark_files)
      {
        /* Mark file using info from actual file rather than link: */
        Dir_Item tmp;
        tmp = *file;
        if (sigSafe_stat(full_name(dname, tmp.fname), &tmp.info) == 0)
        {
    bp += strlen(bp); /* Find where we are */
    bp = G_print(bp, "T", dname, &tmp);
        }
        markable = FALSE;
      }
      /* Note: SunOS4.x has one small difference where non-long
         listings mark symlinks pointing to dir when using '-F' */
# endif
    }
#     endif
    break;

  default:
    if (isAlnum(icase))
    {
      carrot_msg(NULL, "+N", N_format, "Unrecognized +N directive", fmt-1);
      exit(USAGE_ERROR);
    }
    else
      /* Output non-directive/non-alphanumeric as-is: */
      as_is = TRUE;
    break;
  }

  if (markable)
  {
    bp += strlen(bp); /* Find where we are */
    if (mark_files)
    {
      /* Short listings (i.e. not long) always mark files,
         long listings mark files unless a non-expanded symbolic link: */
      if (!list_long || !(file->islink && !expand_symlink))
        bp = G_print(bp, "T", dname, file);
    }
    else if (mark_dirs && file->isdir)
    {
      *bp++ = '/';
      *bp = CNULL;
    }
    markable = FALSE;
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

  if (slash != NULL)
    *slash = '/';

  if (quote_mode) errorMsgExit("Missing delimiter in +N format", USAGE_ERROR);
  if (!directive_seen) errorMsgExit("Incomplete +N format", USAGE_ERROR);

  return(bp);
}
#undef  ZERO_PAD_DEFAULT


#define ZERO_PAD_DEFAULT  (!tics)
/*#define ZERO_PAD_DEFAULT  (!tics || !yoffset) -- FIXME: I'm undecided */
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
  time_t abs_ftime = ftime; /* Avoid possible side-effects */
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

      case 'A':     /* '+TA' is DEPRECATED, use '+T^a' instead */
      case 'R':     /* '+TR' is DEPRECATED, use '+T^r' instead */
      case Tf_NUM_DATE:   /* '+TN' is DEPRECATED, use '+T^N' instead */
      case Tf_GMT_DATE:   /* '+TG' is DEPRECATED, use '+T^G' instead */
      case Tf_LOCAL_DATE: /* '+TL' is DEPRECATED, use '+T^L' instead */
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
  break;  /* modifiers generate no output */

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
    if (ftime == 0 && (zero_st_info || munge > 0 ||
           (stamping && VersionLevel >= 154)))
    {
      /* Special case where zeroes used instead of whatever the zero
         date translates to in the local time zone: */
      /* NB: This section REMOVED in edate. */
      if (!width_specified) width = 0;
      sprintf(bp, "%*s",width, "00000000.000000");
    }
    else
      bp = T_print_width(bp, "^N~YMD.hms~", ftime, fdate,
             gmt, meridian, width);
    break;

  case Tf_FLOATING_POINT_DAY:
  case Tf_ISO8601_DAY:
    if (ftime == 0 && (zero_st_info || munge > 0 ||
           (stamping && VersionLevel >= 154)))
    {
      /* Special case where zeroes used instead of whatever the zero
         date translates to in the local time zone: */
      /* NB: This section REMOVED in edate. */
      if (!width_specified) width = 0;
      sprintf(bp, "%*s",width, "00000000");
    }
    else
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
#   ifdef HAVE_LOCALE
      if (useLcTime)
      {
        char month[32];
        strftime32(month, 32, "%b", fdate);
        sprintf(bp, "%*s",width, month);
      }
      else
#   endif
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
#   ifdef HAVE_LOCALE
      if (useLcTime)
      {
        char wday[32];
        strftime32(wday, 32, "%a", fdate);
        sprintf(bp, "%*s",width, wday);
      }
      else
#   endif
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
#     define YOFFSET_HOURS (fdate->tm_yday * 24 + fdate->tm_hour)
#     define YOFFSET_MINS  (YOFFSET_HOURS * 60 + fdate->tm_min)
#     define YOFFSET_SECS  (YOFFSET_MINS * 60 + fdate->tm_sec)
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
#     if defined(HAVE_LONG_LONG_TIME)
        /* Use default format as this uses signed 64-bits: */
        sprintf(rsp, F_time_t(FALSE,0, ftime));
#     else
        /* Use "unsigned" format as this value may use all 32-bits: */
        sprintf(rsp, FU_time_t(FALSE,0, ftime));
#     endif
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


char *full_name(char *dname,
    char *fname)
{
  /* Be careful that the return value doesn't get overworked! */
  static char full_name[MAX_FULL_NAME];

  /* Gather any information on this parameter */
  if (dname[0] == CNULL || strcmp(dname, ".") == 0)
  {
    return(fname);
  }
  else
  {
    if (strcmp(dname, "/") == 0) dname = "";
    sprintf(full_name, "%s/%s", dname, fname);
    return(full_name);
  }
}


#define DIR_LIST_CHUNK  (4096/sizeof(Dir_Item))
Dir_List dir_item_list = {NULL, NULL};
Dir_Item *dirItemAlloc(void)
{
  register Dir_Item *file;
  if (dir_item_list.head == NULL)
  {
    dir_item_list.head = memAlloc(DIR_LIST_CHUNK*sizeof(Dir_Item));
    dir_item_list.tail = dir_item_list.head+DIR_LIST_CHUNK-1;
    for (file = dir_item_list.head; file != dir_item_list.tail; file++)
      file->next = file+1;
    file->next = NULL;
    dirItemAllocAvail += DIR_LIST_CHUNK;
  }

  file = dir_item_list.head;
  dir_item_list.head = dir_item_list.head->next;
  dirItemAllocInUse++;
  return(file);
}


void dirItemFree(Dir_Item *file)
{
  if (dir_item_list.head == NULL)
    dir_item_list.head = file;
  else
    dir_item_list.tail->next = file;

  dir_item_list.tail = file;
  file->next = NULL;
  dirItemAllocInUse--;
  return;
}


void dirItemShow(FILE *out)
{
  fprintf(out, "\rdirItemAlloc: Avail, InUse: %lu %lu\n",
    dirItemAllocAvail, dirItemAllocInUse);
  return;
}


void watchSigHandler(int sig)
{
#if WATCH_SIGNAL == SIGINT
  Local Boole sigAskNoMore = FALSE;
#endif
#ifdef SIG_IGN
  signal(WATCH_SIGNAL, SIG_IGN);
#endif
  sigEvent = TRUE;
  fprintf(ttyout, "\rDIR == %s\n", CwdPath);
  if (Debug&0x20)
  {
    memShow(ttyout);
    dirItemShow(ttyout);
  }

#if WATCH_SIGNAL == SIGINT
  if (sigAskAbort && !sigAskNoMore)
  {
    char resp[10];
    fprintf(ttyout, "Abort? [ynN]: ");
    fgets(resp, 9, ttyin);
    if (resp[0] == 'y')
    {
      if (defaultSigHandler != NULL) defaultSigHandler();
      exit(0);
    }
    else if (resp[0] == 'N')
      sigAskNoMore = TRUE;
  }
  /* Next time ask to abort if ^C occurs more than once during an interval: */
  sigAskAbort = TRUE;
#endif

  signal(WATCH_SIGNAL,  watchSigHandler);
  return;
}


int sigSafe_stat(char *path, struct stat *buf)
{
  int sts;
  do {
    int save_errno = errno;
    sigEvent = FALSE;
    sts = stat(path, buf);
    /*if (sigEvent) fprintf(ttyout, "SS_stat errno == %d\n", errno);*/
    if (sigEvent) errno = save_errno;
  } while (sigEvent);
  return(sts);
}


int sigSafe_lstat(char *path, struct stat *buf)
{
  int sts;
  do {
    int save_errno = errno;
    sigEvent = FALSE;
    sts = lstat(path, buf);
    /*if (sigEvent) fprintf(ttyout, "SS_lstat errno == %d\n", errno);*/
    if (sigEvent) errno = save_errno;
  } while (sigEvent);
  return(sts);
}


void spin_wheel(void)
{
  static int i = 0;
  char *wheel = "/-\\|";
  fprintf(ttyout, "%c\b", wheel[i]);
  fflush(ttyout);
  i = (i+1) & 3;
  return;
}
