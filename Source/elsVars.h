/******************************************************************************
  elsVars.h -- els variables and defines
  
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

#ifndef ELS__ELSVARS_H
#define ELS__ELSVARS_H

/********** Global Routines Defined **********/

/* See "els.h" for Global Routines */

/********** Global Variables Defined **********/

/* extern int  Iarg; */
extern int  Argc;
extern char **Argv;
/* extern char *Progname; */
extern uid_t Whoami;
extern time_t The_Time;
extern time_t The_Time_in_an_hour;
extern char *Time_Zone;
extern int Debug;
extern int VersionLevel;
/* extern int MaxVersionLevel; */
extern Boole VersionLevelArg;
/* extern Boole ArgOrder; */
/* extern Boole TruncateName; */
extern Boole MaskId;
/* extern Boole QuitOnError; */
/* extern Boole SortCI; */
/* extern Boole SortDICT; */
/* extern Boole First_listing; */
extern char *Current_Opt;
/* extern char *Current_Arg; */
/* extern Dir_Item *CwdItem; */
extern int CwdBlocksize;
/* extern char *CwdPath; */

#define EXEC_ERROR      3
#define LISTING_ERROR   2
#define USAGE_ERROR     1
#define GENERAL_ERROR   1
#define NORMAL_EXIT     0
extern Boole listingError;

/* extern Boole zero_file_args; */
/* extern Boole multiple_file_args; */
extern Boole using_full_names;
/* extern Boole avoid_trimmings; */
extern Boole list_topdir;
/* extern char first_mac; */
extern int recursion_level;

#define SEM_ELS  0x01
#define SEM_LS   0x02
#define SEM_BSD  0x04
#define SEM_SYS5 0x08
#define SEM_GNU  0x10
extern Uint Sem;

extern Boole list_hidden;
extern Boole list_dotdir;
/* extern Boole list_long; */
/* extern Boole list_long_numeric; */
/* extern Boole list_long_omit_gid; */
/* extern Boole list_atime; */
/* extern Boole list_ctime; */
/* extern Boole list_inode; */
/* extern Boole list_size_in_blocks; */
extern Boole list_size_human_10;
extern Boole list_size_human_2;
extern Boole expand_symlink;
extern Boole expand_directories;
/* extern Boole mark_files; */
/* extern Boole mark_dirs; */
/* extern Boole dash_b; */
/* extern Boole dash_Q; */
/* extern Boole recursive; */
/* extern Boole reverse_sort; */
/* extern Boole time_sort; */
/* extern Boole unsort; */
/* extern Boole g_flag; */
/* extern Boole G_flag; */
/* extern Boole plus_c; */
/* extern cksumType cksum_type; */
/* extern int cksum_size; */
/* extern Boole cksum_unaccess; */
extern Boole list_directories;
/* extern Boole list_directories_specified; */
/* extern Boole plus_q; */
/* extern Boole plus_q_specified; */
/* extern Boole traverse_mp; */
/* extern Boole traverse_expanded_symlink; */
/* extern Boole traverse_specified; */
extern Boole useLcCollate;
extern Boole useLcTime;
extern int verboseLevel;
extern Boole warning_suppress;
/* extern Boole watch_progress; */
/* extern FILE *ttyin, *ttyout; */
/* extern Boole zero_st_info; */
/* extern Boole zero_st_info_specified; */
/* extern Ulong zero_st_mask; */

/* extern Boole CCaseMode; */
/* extern Boole GTarStyle; */
/* extern Boole Tar5Style; */
/* extern Boole FirstFound; */
/* extern Boole OncePerDir; */
/* extern int DirDepth; */
extern Boole IncludeSS;
/* extern Boole IncludeSS_specified; */

#define type_REG	'r'
#define type_DIR	'd'
#define type_CHR	'c'
#define type_BLK	'b'
#define type_FIFO	'p'
#define type_LNK	'l'
#define type_ORPH	'X'
#define type_SOCK	's'
#define type_DOOR	'D'
#define type_SPECIAL	'S'	/* c:b */
#define symtype_REG	'-'
#define symtype_DIR	'/'
#define symtype_FIFO	'|'
#define symtype_LNK	'@'
#define symtype_SOCK	'='
#define symtype_DOOR	'>'

#endif /*ELS__ELSVARS_H*/
