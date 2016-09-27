/******************************************************************************
  elsFilter.c -- els filter functions
  
  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: September 2, 2015
  
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
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <utime.h>
#include <dirent.h>
#include <sys/param.h>
#include <unistd.h>
#include <stdlib.h>

#include "defs.h"
#include "getdate32.h"
#include "time32.h"
#include "auxil.h"
#include "els.h"
#include "elsFilter.h"
#include "elsMisc.h"
#include "sysdep.h"
#include "phLib.h"

extern Boole rwm_filtering, 
             rwm_ifreg,      /* Regular */
             rwm_ifexe,      /* Executable */
             rwm_ifwrt,      /* Writable   */
             rwm_ifred,      /* Readable   */
             rwm_ifdir,      /* Directory */
             rwm_ifchr,      /* Char Special */
             rwm_ifblk,      /* Block Special */
             rwm_ififo,      /* Fifo */
             rwm_iflnk,      /* Symbolic Link */
             rwm_ifsock,     /* Socket */
             rwm_ifunk,      /* unkown */
             rwm_docomma,
             rwm_dospace;    /* similiar to find -print0 */

/********** Globals Defined **********/

int fexpr_ORcount;
int fexpr_ANDcount;
char *fexpr_ORfilter[MAX_FEXPR_FILTER];
char *fexpr_ANDfilter[MAX_FEXPR_FILTER];

/* List of wildcard patterns for "E"xcluding file/dir names: */
int exc_file_count;
char *exc_file_filter[MAX_EXC_FILTER];
int exc_dir_count;
char *exc_dir_filter[MAX_EXC_FILTER];

/* List of wildcard patterns for "I"ncluding file/dir names: */
int inc_file_count;
char *inc_file_filter[MAX_INC_FILTER];
int inc_dir_count;
char *inc_dir_filter[MAX_INC_FILTER];

/********** Locals Defined **********/

Local int fexprLevel = 0;
Local Boole filter_fexprINT(Dir_Item *file, char *path, char *ptr, char **end);

/* Static messages: */
Local char *NUM_TOO_BIG = "Number too big for data type";


struct stat *getCwdInfo(char *path)
{
  static struct stat info;
  static Boole lastCwdWasDot = FALSE;
  if (CwdItem != NULL)
    return(&CwdItem->info);
  else
  {
    char *cwd;
    char *slash = strrchr(path, '/');
    if (slash != NULL)
    {
      *slash = CNULL;
      cwd = path;
    }
    else
    {
      if (lastCwdWasDot) return(&info);
      cwd = ".";
    }
      
    /* Ignore return status, as stat_dir() will eventually detect any errno: */
# if defined(S_IFLNK)
    /* System has symbolic links: */
    sigSafe_lstat(cwd, &info);
# else
    /* System does not have symbolic links: */
    sigSafe_stat(cwd, &info);
# endif

    if (slash != NULL)
    {
      *slash = '/';
      lastCwdWasDot = FALSE;
    }
    else
    {
      lastCwdWasDot = TRUE;
    }
    return(&info);
  }
}


#if defined(HPUX)
# define LOCAL_BLOCKSIZE 1024
#else
# define LOCAL_BLOCKSIZE  512
#endif

/* NB: The blocksize calculated by this routine has *nothing* to do with
   the st_blksize element returned by stat()!  Regreatably st_blksize
   refers to the preferred blocksize for file system I/O and almost never
   equals the size of each st_blocks unit, whereas getCwdBlocksize()
   attempts to heuristically calculate the size of each st_blocks unit
   using the characteristics of the current working directory (cwd). */

int getCwdBlocksize(char *path)
{
  static int lastCwdBlocksize = LOCAL_BLOCKSIZE;
# ifdef NO_STAT_BLOCKS
  CwdBlocksize = LOCAL_BLOCKSIZE;
# else
  /* FIXME???: If +z is specified this routine returns lastCwdBlocksize */
  if (CwdBlocksize == 0)
  {
    struct stat *cwd_info = getCwdInfo(path);
    int bs;

    /* ANOMALOUS CASES:
       1) Most HPUX directories are a multiple of 1024, but it is possible
       for directories under HPUX's vxfs filesystem to be as small as 26 bytes.
       2) Directory sizes of ClearCase's mvfs filesystem under SunOS5 can be
       *any* non-negative integer.  Moreover, a ClearCase mvfs directory
       containing 0 files will have 0 blocks and 0 bytes.
       3) SunOS5.6+'s directory sizes of an indirect autofs map are 1 byte if
       the option -nobrowse is not specified and the filesystem is not mounted.
       */

    if ((Debug&4) && (cwd_info->st_size <= 0 || cwd_info->st_blocks == 0 ||
		      (cwd_info->st_size & 511) != 0))
      printf("STRANGE DIRECTORY: blocks=%lu, size=%lu (%s)\n",
	     (Ulong)cwd_info->st_blocks, (Ulong)cwd_info->st_size, path);

    /* Round up to the next power of 2 >= 512: */
    if (cwd_info->st_blocks == 0)
    {
      /* Files with errno set will probably end up here (this makes the file
	 seem sparse which is OK so that perror() eventually gets called): */
      bs = lastCwdBlocksize;  /* Best available guess??? */
    }
    else
    {
      bs = cwd_info->st_size / cwd_info->st_blocks;
      if (bs < 256)
	/* Attempt to handle HPUX vxfs, ClearCase mvfs, and SunOS5.6+
	   browsable autofs filesystems: */
	bs = lastCwdBlocksize;  /* Best available guess??? */
      else if (bs <= 512)
	bs = 512;
      else if (bs <= 1024)
	bs = 1024;
      else if (bs <= 2048)
	bs = 2048;
      else if (bs <= 4096)
	bs = 4096;
      else if (bs <= 8192)
	bs = 8192;
      else
      {
	/* A blocksize should never bigger than this!?: */
	fprintf(stderr, "Block size error\n");
	exit(LISTING_ERROR);
      }
    }

    if (Debug&2)
      printf("Blocksize = %d, %lu = %lu/%lu (%s)\n", bs,
	     (Ulong)cwd_info->st_size / (Ulong)cwd_info->st_blocks,
	     (Ulong)cwd_info->st_size, (Ulong)cwd_info->st_blocks,
	     path);
    if ((Debug&4) && lastCwdBlocksize != bs)
      printf("BLOCKSIZE CHANGING FROM %d TO %d (%s)\n",
	     lastCwdBlocksize, bs, path);
    
    CwdBlocksize = bs;
    lastCwdBlocksize = bs;
  }
# endif
  return(CwdBlocksize);
}


void filter_file(Dir_Item *file, char *path)
{
  int i;
  Boole file_is_listable = TRUE;
  Boole file_is_searchable = FALSE;

  if (file->fname[0] == CNULL)
  {
    /* This situation is unlikely!?: */
    file_is_listable = FALSE;
  }
  /* Avoid NetApp .snapshot directories usually found at the top-level of
     a mount-point having "vol option VOL_NAME nosnapdir off" set: */
  if (file->isdir && !IncludeSS && strcmp(file->fname, ".snapshot") == 0)
  {
    if (verboseLevel > 0) fprintf(stderr, "\
%s: %s: Skipping snapshot directory\n", Progname, path);
    file_is_listable = FALSE;
  }
  else
  {
    /*****************************************/
    /* Check for ".", "..", and ".xxx" files */
    /*****************************************/
    
    /* The user asked for the file explicitly on the command-line
       whenever recursion_level == 0, thus, always grant such a
       request; otherwise, hidden files must meet the list_* criteria.
       */

    if (file->dotdir)
      file_is_listable = list_dotdir || recursion_level == 0;
    else if (file->hidden)
      file_is_listable = list_hidden || recursion_level == 0;
    
    /******************************************/
    /* Check file against "F"ilter expression */
    /******************************************/

    if (file_is_listable && fexpr_ANDcount > 0)
    {
      /* ALL of the following "AND" filters need to be TRUE: */
      file_is_listable = TRUE;
      for (i = 0; i < fexpr_ANDcount && file_is_listable; i++)
	file_is_listable = filter_fexpr(file, path,
					fexpr_ANDfilter[i], NULL);
    }

    if (file_is_listable && fexpr_ORcount > 0)
    {
      /* ANY of the following "OR" filters needs to be TRUE: */
      file_is_listable = FALSE;
      for (i = 0; i < fexpr_ORcount && !file_is_listable; i++)
	file_is_listable = filter_fexpr(file, path,
					fexpr_ORfilter[i], NULL);
    }

    /********************************************/
    /* Check file name against "E"xclude filter */
    /********************************************/

    if (file_is_listable && exc_file_count > 0 && !file->isdir)
    {
      Boole found = FALSE;
      for (i = 0; i < exc_file_count && !found; i++)
	found = wildcard_match(file->fname, exc_file_filter[i]);
      file_is_listable = !found;
    }
    
    /*******************************************/
    /* Check dir name against "e"xclude filter */
    /*******************************************/

    if (file_is_listable && exc_dir_count > 0 && file->isdir)
    {
      Boole found = FALSE;
      for (i = 0; i < exc_dir_count && !found; i++)
	if (strchr(exc_dir_filter[i], '/'))
	  /* Exclude a directory having a path component (this ad hoc
	     code facilitates cases such as "els +e/usr/local /usr"): */
	  found = wildcard_match(path, exc_dir_filter[i]);
	else
	  /* Exclude a directory having *no* path component: */
	  found = wildcard_match(file->fname, exc_dir_filter[i]);
      file_is_listable = !found;
    }
    
    /********************************************/
    /* Check file name against "I"nclude filter */
    /********************************************/

    if (file_is_listable && inc_file_count > 0 && !file->isdir)
    {
      Boole found = FALSE;
      for (i = 0; i < inc_file_count && !found; i++)
	found = wildcard_match(file->fname, inc_file_filter[i]);
      file_is_listable = found;
    }

    /* Special circumstances warrant not listing directories when explicitly
       "I"ncluding files (although directories will still be considered
       searchable; moreover, the search filters "e" and "i" will still play
       a role): */
    if (file_is_listable && inc_file_count > 0 && file->isdir)
      file_is_listable = FALSE;

    /*******************************************/
    /* Check dir name against "i"nclude filter */
    /*******************************************/

    if (file_is_listable && inc_dir_count > 0 && file->isdir)
    {
      Boole found = FALSE;
      for (i = 0; i < inc_dir_count && !found; i++)
	found = wildcard_match(file->fname, inc_dir_filter[i]);
      file_is_listable = found;
    }

    /************************************/
    /* Decide whether dir is searchable */
    /************************************/

    /* When filtering a subdirectory, we want to search the subdirectory's
       contents, even if it doesn't meet our listing criterion, otherwise
       we will end up with hardly any files or subdirectories to search!
       The exceptions to this rule are: 1) when the user explicitly
       specifies directories to include using the "+i" option; 2) when
       listing normally or via exclusion, we want to search subdirectories
       only if they didn't get scratched from our search list; 3) the
       directory is hidden and list_hidden has not been enabled.  Even with
       these exceptions we still end up searching quite a few files. */

    if (file->isdir)
    {
      if (file_is_listable)
	file_is_searchable = TRUE;
      else if (file->hidden && !list_hidden && VersionLevel >= 144)
	file_is_searchable = FALSE;
      else
      {
	file_is_searchable = TRUE;
	if (file_is_searchable && exc_dir_count > 0)
	{
	  Boole found = FALSE;
	  for (i = 0; i < exc_dir_count && !found; i++)
	    if (strchr(exc_dir_filter[i], '/'))
	      /* Exclude a directory having a path component (this ad hoc
		 code facilitates cases such as "els +e/usr/local /usr"): */
	      found = wildcard_match(path, exc_dir_filter[i]);
	    else
	      /* Exclude a directory having *no* path component: */
	      found = wildcard_match(file->fname, exc_dir_filter[i]);
	  file_is_searchable = !found;
	}
	if (file_is_searchable && inc_dir_count > 0)
	{
	  Boole found = FALSE;
	  for (i = 0; i < inc_dir_count && !found; i++)
	    found = wildcard_match(file->fname, inc_dir_filter[i]);
	  file_is_searchable = found;
	}
      }
    }
  }

  /* FINALLY, file is listable if it met the filtering criteria and
     1) it isn't a dir, 2) it's a non-expandable dir, or 3) the list_*
     criteria specify directories (unless this is the very first recursion
     level and trimmings aren't being avoided).  All this is needed to
     mimic "ls" behavior: */
  
  file->listable =
    file_is_listable && (!file->isdir ||
			 !expand_directories ||
			 (list_directories && (recursion_level > 0 ||
					       list_topdir)));

  file->searchable = file_is_searchable;

 if ( file->listable && rwm_filtering ) {
    int ftype;
    register struct stat *info = &file->info;

    ftype = (info->st_mode & S_IFMT);

    switch (ftype) {
      case S_IFREG: 
        if ( !rwm_ifreg ) file->listable = FALSE; /* Regular */
        if ( !(info->st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)) && rwm_ifexe) file->listable = FALSE;
        if ( !(info->st_mode & (S_IWUSR|S_IWGRP|S_IWOTH)) && rwm_ifwrt) file->listable = FALSE;
        if ( !(info->st_mode & (S_IRUSR|S_IRGRP|S_IROTH)) && rwm_ifred) file->listable = FALSE;
	break;
      case S_IFDIR:  if ( !rwm_ifdir ) file->listable = FALSE; break; /* Directory */
      case S_IFCHR:  if ( !rwm_ifchr ) file->listable = FALSE; break; /* Char Special */
      case S_IFBLK:  if ( !rwm_ifblk ) file->listable = FALSE; break; /* Block Special */
#ifdef S_IFIFO
      case S_IFIFO:  if ( !rwm_ififo ) file->listable = FALSE; break; /* Fifo */
#endif
#ifdef S_IFLNK
      case S_IFLNK:  if ( !rwm_iflnk ) file->listable = FALSE; break; /* Symbolic Link */
#endif
#ifdef S_IFSOCK
      case S_IFSOCK: if ( !rwm_ifsock) file->listable = FALSE; break; /* Socket */
#endif
      default:       if ( !rwm_ifunk ) file->listable = FALSE; break; /* unkown */
    }
  }

/*#define SHOW_FILTERING*/
#ifdef SHOW_FILTERING
      printf("sl, d, dd, h: %d, %d, %d, %d -- l, s: %d, %d -- %s\n",
	     file->symlink, file->dir,
	     file->dotdir, file->hidden,
	     file->listable, file->searchable, file->fname);
#endif

  return;
}


Boole filter_fexpr(Dir_Item *file, char *path, char *ptr, char **end)
{
  /******************************************/
  /* Check file against "F"ilter expression */
  /******************************************/
  
  Boole sts;
  int saveFexprLevel = fexprLevel;
  char *SaveCurrent_Opt = Current_Opt;
  char *SaveCurrent_Arg = Current_Arg;

  fexprLevel = 0;
  Current_Opt = "+F";
  Current_Arg = ptr;

  /* Allow optional '*' or '+' at beginning of any filter expression 
     (this is used to define whether GLOBAL logic is AND or OR): */
  if (IS_MEMBER(*ptr, "*+")) ptr++;

  sts = filter_fexprINT(file, path, ptr, end);

  fexprLevel = saveFexprLevel;
  Current_Opt = SaveCurrent_Opt;
  Current_Arg = SaveCurrent_Arg;

  return(sts);
}


Local Boole filter_fexprINT(Dir_Item *file, char *path, char *ptr, char **end)
{
  Boole file_is_listable = FALSE;
  Boole looking_expr;
  char close_paren = CNULL;
  fexprLevel++;

  looking_expr = TRUE;
  while (looking_expr && (!file_is_listable || end))
  {
    Boole term_good = TRUE;
    Boole looking_term;

    looking_term = TRUE;
    while (looking_term)
    {
      Boole negate = FALSE;
      Boole_Function filter = NULL;
      
      if (*ptr == fexpr_NOT || *ptr == fexpr_FNOT)
      {
	negate = TRUE;
	ptr++;
      }
      
      switch (*ptr)
      {
      case fexpr_BEGIN1:
      case fexpr_BEGIN2:
	ptr--;
	filter = filter_fexprINT;
	break;
      case 'A':
	filter = filter_access;
	break;
      case 'T':
	filter = filter_type;
	break;
      case 'P':
	filter = filter_perm;
	break;
      case 'Q':
	filter = filter_quantity;
	break;
      case 'U':
	filter = filter_unusual;
	break;
      case 'c':
	filter = filter_ccase;
	break;
      case 'l':	/* Experimental */
	/* A pre-condition of link-filter term being found good when used as
	   the first term is that must be a link beforehand.  This creates
	   the implication: '+F~l{e}' --> +F'T{l}&&~l{e}'.  Without this
	   implication then '+F~l{e}' would list all NON-linked files. */
	term_good &= file->islink;  /* Pre-condition */
	filter = filter_link;
	break;
      default:
	expecting_error(!negate, "({ATPQUcl", FALSE, FALSE, ptr);
	break;
      }

      ptr++;
      if (*ptr == fexpr_BEGIN1)
	close_paren = fexpr_END1;
      else if (*ptr == fexpr_BEGIN2)
	close_paren = fexpr_END2;
      else
	expecting_error(FALSE, "({", FALSE, FALSE, ptr);
      ptr++;
      
      term_good &=
	negate ? !filter(file, path, ptr, &ptr)
	  :       filter(file, path, ptr, &ptr);
      
      ptr--;
      if (*ptr != close_paren)
      {
	char paren[2]; paren[0] = close_paren; paren[1] = CNULL;
	expecting_error(FALSE, paren, FALSE, FALSE, ptr);
      }
      ptr++;
      
      switch (*ptr)
      {
      case fexpr_AND:
	if (*(ptr+1) == fexpr_AND) ptr++;	/* Allow && */
      case fexpr_FAND:
	break;

      case fexpr_OR:
	if (*(ptr+1) == fexpr_OR) ptr++;	/* Allow || */
      case fexpr_FOR:
	looking_term = FALSE;
	break;

      case fexpr_END1:
      case fexpr_END2:
	if (fexprLevel == 1) filter_error_msg("Extraneous parenthesis", ptr);
	looking_term = FALSE;
	looking_expr = FALSE;
	break;

      case CNULL:
	if (fexprLevel != 1) filter_error_msg("Missing parenthesis", ptr);
	looking_term = FALSE;
	looking_expr = FALSE;
	break;

      default:
	expecting_error(FALSE, NULL, TRUE, FALSE, ptr);
	break;
      }

      if (*ptr) ptr++;
    }

    file_is_listable |= term_good;
  }

  fexprLevel--;
  if (end) *end = ptr;
  return(file_is_listable);
}


Boole filter_access(Dir_Item *file, char *path, char *ptr, char **end)
{
  /**************************************/
  /* Check file against "A"ccess filter */
  /**************************************/
  
  Boole file_is_listable = FALSE;
  Boole looking_expr;
  
  looking_expr = TRUE;
  while (looking_expr && (!file_is_listable || end))
  {
    Boole term_good = TRUE;
    Boole looking_term;

    looking_term = TRUE;
    while (looking_term)
    {
      Boole negate = FALSE;
      Boole test_result = FALSE;
      Boole have_access = FALSE;
      Uint access_bits = 0;
      
      if (*ptr == fexpr_NOT || *ptr == fexpr_FNOT)
      {
	negate = TRUE;
	ptr++;
      }

      while (IS_MEMBER(*ptr, "rwxe"))
      {
	have_access = TRUE;
	switch (*ptr)
	{
	case 'r':
	  access_bits |= R_OK;
	  break;
	case 'w':
	  access_bits |= W_OK;
	  break;
	case 'x':
	  access_bits |= X_OK;
	  break;
	case 'e':
	  access_bits |= F_OK;
	  break;
	}
	ptr++;
      }
      /* NB: F_OK is defined as 0, with bits 1,2,4 used for R_OK, W_OK, X_OK.
	 But since if queried for 'A{ex}' then the file must exist if it's
	 also to be eXecutable, so 'A{ex}' is redundant and means the same
	 thing as 'A{x}' */

      if (!have_access)
	expecting_error(!negate, "rwxe", FALSE, FALSE, ptr);

      /* ??? may need to use getaccess() for ACLs under HPUX9 ??? */
      test_result = access(path, access_bits) == 0;
      term_good &= negate ? !test_result : test_result;

      switch (*ptr)
      {
      case fexpr_AND:
	if (*(ptr+1) == fexpr_AND) ptr++;	/* Allow && */
      case fexpr_FAND:
	break;

      case fexpr_OR:
	if (*(ptr+1) == fexpr_OR) ptr++;	/* Allow || */
      case fexpr_FOR:
	looking_term = FALSE;
	break;

      case fexpr_END1:
      case fexpr_END2:
	looking_term = FALSE;
	looking_expr = FALSE;
	break;

      default:
	expecting_error(FALSE, NULL, TRUE, TRUE, ptr);
	break;
      }
      ptr++;
    }

    file_is_listable |= term_good;
  }

  if (end) *end = ptr;
  return(file_is_listable);
}


Boole filter_type(Dir_Item *file, char *path, char *ptr, char **end)
{
  /************************************/
  /* Check file against "T"ype filter */
  /************************************/
  
  Boole file_is_listable = FALSE;
  Boole looking_expr;
  int fmode = file->info.st_mode;
  int ftype = (fmode & S_IFMT);
  
  looking_expr = TRUE;
  while (looking_expr && (!file_is_listable || end))
  {
    Boole term_good = TRUE;
    Boole looking_term;
    Boole redundancy_check = FALSE;

    looking_term = TRUE;
    while (looking_term)
    {
      Boole negate = FALSE;
      Boole test_result = FALSE;
      
      if (*ptr == fexpr_NOT || *ptr == fexpr_FNOT)
      {
	negate = TRUE;
	ptr++;
      }

      switch (*ptr)
      {
      case 'f': 	/* 'r' is too hard to remember */
      case symtype_REG:
      case type_REG:	/* Regular */
	test_result = ftype == S_IFREG;
	break;
	
      case symtype_DIR:
      case type_DIR:	/* Directory */
	test_result = ftype == S_IFDIR;
	break;
	
      case type_CHR:	/* Char Special */
	test_result = ftype == S_IFCHR;
	break;
	
      case type_BLK:	/* Block Special */
	test_result = ftype == S_IFBLK;
	break;

      /*case symtype_FIFO: -- this might get confused for || operator */
      case type_FIFO:	/* Fifo */
#ifdef S_IFIFO
	test_result = ftype == S_IFIFO;
#endif
	break;
      case symtype_LNK:
      case type_LNK:	/* Symbolic Link */
#ifdef S_IFLNK
	if (expand_symlink)
	  /* If expanding symlinks and the user wants symlinks filtered(!)
	     then do what is asked (even though it's a contradiction): */
	  test_result = file->islink;
	else
	  test_result = ftype == S_IFLNK;
#endif
	break;
      case symtype_SOCK:
      case type_SOCK:	 /* Socket */
#ifdef S_IFSOCK
	test_result = ftype == S_IFSOCK;
#endif
	break;
      case symtype_DOOR:
      case type_DOOR:	 /* Door */
#ifdef S_IFDOOR
	test_result = ftype == S_IFDOOR;
#endif
	break;
      case type_SPECIAL:	/* Any Device (c or b) */
	test_result = filter_type(file, path, "c:b", NULL);
	break;

      default:
	expecting_error(!negate, "frdcbplsDS or -/@=>", FALSE, FALSE, ptr);
	break;
      }

      if (!negate)
      {
	if (redundancy_check)
	  redundancy_warning("types", ptr);
	redundancy_check = TRUE;
      }
      ptr++;
      
      term_good &= negate ? !test_result : test_result;
      
      switch (*ptr)
      {
      case fexpr_AND:
	if (*(ptr+1) == fexpr_AND) ptr++;	/* Allow && */
      case fexpr_FAND:
	break;

      case fexpr_OR:
	if (*(ptr+1) == fexpr_OR) ptr++;	/* Allow || */
      case fexpr_FOR:
	looking_term = FALSE;
	break;

      case fexpr_END1:
      case fexpr_END2:
	looking_term = FALSE;
	looking_expr = FALSE;
	break;

      default:
	expecting_error(FALSE, NULL, TRUE, TRUE, ptr);
	break;
      }
      ptr++;
    }

    file_is_listable |= term_good;
  }

  if (end) *end = ptr;
  return(file_is_listable);
}


Boole filter_perm(Dir_Item *file, char *path, char *ptr, char **end)
{
  /******************************************/
  /* Check file against "P"ermission filter */
  /******************************************/

  Boole file_is_listable = FALSE;
  Boole looking_expr;
  register int fmode = file->info.st_mode;
  
  looking_expr = TRUE;
  while (looking_expr && (!file_is_listable || end))
  {
    Boole term_good = TRUE;
    Boole looking_term;
    Boole redundancy_check = FALSE;

    looking_term = TRUE;
    while (looking_term)
    {
      Boole negate = FALSE;
      Boole test_result = FALSE;
#define ALL_USER  (S_IWUSR|S_IRUSR|S_IXUSR|S_ISUID|S_ISVTX)
#define ALL_GROUP (S_IWGRP|S_IRGRP|S_IXGRP|S_ISGID)
#define ALL_OTHER (S_IWOTH|S_IROTH|S_IXOTH)
      Boole perm_any = FALSE;
      Boole perm_have = FALSE;
      Boole perm_missing = FALSE;
      Boole perm_exact = FALSE;
      Uint perm_field = 0;
      Uint perm_allow = 0;
      Uint perm_deny = 0;
      Boole octal_value = FALSE;
      
      if (*ptr == fexpr_NOT || *ptr == fexpr_FNOT)
      {
	negate = TRUE;
	ptr++;
      }
      
      /* Attempt to read an absolute mode (base 8): */
      if (isDigit(*ptr) && *ptr < '8')
      {
	perm_allow = strtoul(ptr, &ptr, 8);
	perm_field = (ALL_USER|ALL_GROUP|ALL_OTHER);
	perm_exact = TRUE;
	octal_value = TRUE;

	if (perm_allow > 07777)
	  filter_error_msg("Out of bounds [MAX == 07777]", ptr-1);
	if (!negate)
	{
	  if (redundancy_check)
	    redundancy_warning("permission fields", ptr-1);
	}
      }
      else
      {
	while (IS_MEMBER(*ptr, "ugoa"))
	{
	  switch (*ptr)
	  {
	  case 'u':
	    perm_field |= ALL_USER;
	    break;
	  case 'g':
	    perm_field |= ALL_GROUP;
	    break;
	  case 'o':
	    perm_field |= ALL_OTHER;
	    break;
	  case 'a':
	    perm_field |= (ALL_USER|ALL_GROUP|ALL_OTHER);
	    break;
	  }
	  ptr++;
	}

	switch (*ptr)
	{
	  case '+':
	    perm_have = TRUE;
	    break;
	  case '-':
	    perm_missing = TRUE;
	    break;
	  case '=':
	    perm_exact = TRUE;
	    break;
	  default:
	    if (perm_field == 0)
	      expecting_error(!negate, "ugoa+-=' or 'octal-value",
			      FALSE, FALSE, ptr);
	    else
	      expecting_error(FALSE, "ugoa+-=",
			      FALSE, FALSE, ptr);
	    break;
	}
	ptr++;
	
	/* If no permission field has been explicitly defined by now,
	   then "any" permission field is assumed: */
	if (perm_field == 0) perm_any = TRUE;
	
	while (IS_MEMBER(*ptr, "rwxstl"))
	{
	  switch (*ptr)
	  {
	  case 'r':
	    perm_allow |= (S_IRUSR|S_IRGRP|S_IROTH);
	    break;
	    
	  case 'w':
	    perm_allow |= (S_IWUSR|S_IWGRP|S_IWOTH);
	    break;
	    
	  case 'x':
	    perm_allow |= (S_IXUSR|S_IXGRP|S_IXOTH);
	    break;
	    
	  case 's':
	    perm_allow |= (S_ISUID|S_ISGID);
	    if (perm_any) perm_field |= (ALL_USER|ALL_GROUP);
	    if ((perm_field & ALL_OTHER) != 0)
	      filter_error_msg("Invalid context", ptr);
	    break;
	    
	  case 't':
	    perm_allow |= (S_ISVTX);
	    if (perm_any) perm_field |= ALL_USER;
	    if ((perm_field & (ALL_GROUP|ALL_OTHER)) != 0)
	      filter_error_msg("Invalid context", ptr);
	    break;
	    
	  case 'l':
	    perm_allow |= (S_ISGID);
	    perm_deny |= (S_IXGRP);
	    if (perm_any) perm_field |= ALL_GROUP;
	    if ((perm_field & (ALL_USER|ALL_OTHER)) != 0)
	      filter_error_msg("Invalid context", ptr);
	    break;
	  }
	  ptr++;
	}
	    
        if (perm_field == 0)
	  perm_field = (ALL_USER|ALL_GROUP|ALL_OTHER);
      }
      
      if (perm_any)
      {
	Uint fmode_user = fmode & ALL_USER;
	Uint fmode_group = fmode & ALL_GROUP;
	Uint fmode_other = fmode & ALL_OTHER;
	
	/* Test for having all specified permissions in any field: */
	if (perm_have)
	  test_result =
	    (((perm_field & ALL_USER) != 0 &&
	      (fmode_user & perm_deny) == 0 &&
	      (fmode_user & perm_allow) == (perm_allow & ALL_USER)) ||
	     ((perm_field & ALL_GROUP) != 0 &&
	      (fmode_group & perm_deny) == 0 &&
	      (fmode_group & perm_allow) == (perm_allow & ALL_GROUP)) ||
	     ((perm_field & ALL_OTHER) != 0 &&
	      (fmode_other & perm_deny) == 0 &&
	      (fmode_other & perm_allow) == (perm_allow & ALL_OTHER)));
	
	/* Test for missing all specified permissions from any field: */
	else if (perm_missing)
	  test_result = 
	    (((perm_field & ALL_USER) != 0 &&
	      ((fmode_user & perm_deny) == (perm_deny & ALL_USER) &&
	       (fmode_user & perm_allow) == 0)) ||
	     ((perm_field & ALL_GROUP) != 0 &&
	      ((fmode_group & perm_deny) == (perm_deny & ALL_GROUP) &&
	       (fmode_group & perm_allow) == 0)) ||
	     ((perm_field & ALL_OTHER) != 0 &&
	      ((fmode_other & perm_deny) == (perm_deny & ALL_OTHER) &&
	       (fmode_other & perm_allow) == 0)));
	
	/* Test for having nothing other than specified permissions
	   in any field: */
	else if (perm_exact)
	  test_result =
	    (((perm_field & ALL_USER) != 0 &&
	      (fmode_user & perm_deny) == 0 &&
	      (fmode_user) == (perm_allow & ALL_USER)) ||
	     ((perm_field & ALL_GROUP) != 0 &&
	      (fmode_group & perm_deny) == 0 &&
	      (fmode_group) == (perm_allow & ALL_GROUP)) ||
	     ((perm_field & ALL_OTHER) != 0 &&
	      (fmode_other & perm_deny) == 0 &&
	      (fmode_other) == (perm_allow & ALL_OTHER)));
      }
      else
      {
	/* Strip masks of undesired fields (IMPORTANT STEP!): */
	perm_allow &= perm_field;
	perm_deny &= perm_field;
	
	/* Test for having all specified permissions
	   in all specified fields: */
	if (perm_have)
	  test_result = ((fmode & perm_deny) == 0 &&
			 (fmode & perm_allow) == perm_allow);

	/* Test for missing all specified permissions
	   in all specified fields: */
	else if (perm_missing)
	  test_result = ((fmode & perm_deny) == perm_deny &&
			 (fmode & perm_allow) == 0);

	/* Test for having nothing other than specified permissions
	   in all specified fields: */
	else if (perm_exact)
	  test_result = ((fmode & perm_deny) == 0 &&
			 (fmode & perm_field) == perm_allow);
      }

      if (!negate)
	redundancy_check = TRUE;

      term_good &= negate ? !test_result : test_result;

      switch (*ptr)
      {
      case fexpr_AND:
	if (*(ptr+1) == fexpr_AND) ptr++;	/* Allow && */
      case fexpr_FAND:
	break;

      case fexpr_OR:
	if (*(ptr+1) == fexpr_OR) ptr++;	/* Allow || */
      case fexpr_FOR:
	looking_term = FALSE;
	break;

      case fexpr_END1:
      case fexpr_END2:
	looking_term = FALSE;
	looking_expr = FALSE;
	break;

      default:
	if (octal_value)
	  expecting_error(FALSE, NULL, TRUE, TRUE, ptr);
	else
	  expecting_error(FALSE, "rwxstl", TRUE, TRUE, ptr);
	break;
      }
      ptr++;
    }

    file_is_listable |= term_good;
  }

  if (end) *end = ptr;
  return(file_is_listable);
}


Boole filter_quantity(Dir_Item *file, char *path, char *ptr, char **end)
{
  /****************************************/
  /* Check file against "Q"uantity filter */
  /****************************************/

  Boole file_is_listable = FALSE;
  Boole looking_expr;
  int fmode = file->info.st_mode;
  int ftype = (fmode & S_IFMT);
  char q_type = CNULL;
  
  looking_expr = TRUE;
  while (looking_expr && (!file_is_listable || end))
  {
    Boole term_good = TRUE;
    Boole looking_term;

    looking_term = TRUE;
    while (looking_term)
    {
      Boole negate = FALSE;
      Boole test_result = FALSE;
      PhVal q_info = {0};
      PhVal q_val = {0};
      PhVal q_val2 = {0};
      int q_op = 0;
      PhVal q_relative = {0};
      Boole signed_compare = FALSE;
      
      if (*ptr == fexpr_NOT || *ptr == fexpr_FNOT)
      {
	negate = TRUE;
	ptr++;
      }
      
      q_type = *ptr;
      switch (*ptr)
      {
      case 'i':
	q_info.u = file->info.st_ino;
	break;
      case 'A':
#     if !defined(HAVE_ACL)
	filter_warning_msg("Program compiled without ACL support", ptr);
#     endif
	q_info.u = get_acl_count(file, path);
	break;
      case 'l':
	q_info.u = file->info.st_nlink;
	break;
      case 'u':
	q_info.u = uid2Ulong(file->info.st_uid);
	break;
      case 'g':
	q_info.u = gid2Ulong(file->info.st_gid);
	break;
      case 's':
	if (ftype == S_IFBLK || ftype == S_IFCHR)
	  q_info.u = 0;
	else
	  q_info.u = file->info.st_size;
	break;
      case 'm':
	q_info.u = file->info.st_mtime;
	break;
      case 'a':
	q_info.u = file->info.st_atime;
	break;
      case 'c':
	q_info.u = file->info.st_ctime;
	break;
      default:
	expecting_error(!negate, "iAlugsmac", FALSE, FALSE, ptr);
	break;
      }
      ptr++;
      
#define Q_EQ 1
#define Q_NE 2
#define Q_GT 3
#define Q_GE 4
#define Q_LT 5
#define Q_LE 6
      switch (*ptr)
      {
      case '=':
	q_op = Q_EQ;
	if (ptr[1] == '=')
	{
	  q_op = Q_EQ; ptr++;
	}
	break;
      case '!':
      case '~':
	q_op = Q_NE;
	ptr++;
	if (*ptr != '=')
	  expecting_error(FALSE, "=", FALSE, FALSE, ptr);
	break;
      case '<':
	q_op = Q_LT;
	if (ptr[1] == '=')
	{
	  q_op = Q_LE; ptr++;
	}
	else if (ptr[1] == '>')
	{
	  q_op = Q_NE; ptr++;
	}
	break;
      case '>':
	q_op = Q_GT;
	if (ptr[1] == '=')
	{
	  q_op = Q_GE; ptr++;
	}
	break;
      default:
	expecting_error(FALSE, "!~<>=", FALSE, FALSE, ptr);
	break;
      }
      ptr++;
      
      /* Read a comparison value: */
      if (IS_MEMBER(q_type, "ug") && *ptr == '.')
      {
	struct stat *cwd_info = getCwdInfo(path);
	if (q_type == 'u')
	  q_val.u = uid2Ulong(cwd_info->st_uid);
	else
	  q_val.u = gid2Ulong(cwd_info->st_gid);
	ptr++;
      }
      else if (!phLookup(ptr, &ptr, &q_val, &q_relative, &q_val2))
      {
	Ulong factor = 1;
	int power2 = 0;
	int power10 = 0;
	char *start = ptr;
	char *dot = NULL;
	double fraction = 0.0;
	Boole time_relative = FALSE;

	/* ??? Should negative values also be allowed ??? */
	if (isDigit(*ptr) || *ptr == '.')
	{
	  if (IS_MEMBER(q_type, "mac"))
	    /* NB: base10 is enforced for 'mac' because of ambiguity caused by
	       +F'Q{m<0xabcD}' where 'D' can mean Days or be part of 0xabcD */
	    q_val.u = strtoPhVal(ptr, &ptr, 10);
	  else
	    q_val.u = strtoPhVal(ptr, &ptr, 0);
	  if (errno == ERANGE)
	    filter_error_msg(NUM_TOO_BIG, start);

	  if (*ptr == '.') {
	    dot = ptr;
	    ptr++;
	    sscanf(start, "%lf", &fraction);
	    while (isDigit(*ptr)) ptr++;
	  }
	}
	else if (IS_MEMBER(q_type, "ug"))
	{
	  struct passwd *pwd = NULL;
	  struct group *grp = NULL;
	  char name[MAX_USER_GROUP_NAME+2];  /* allow 1 char overrun + CNULL */
	  int i = 0;
	  while (i <= MAX_USER_GROUP_NAME && IS_NOT_MEMBER(*ptr, "&,|:)}") &&
		 *ptr != CNULL)
	    name[i++] = *ptr++;
	  name[i] = CNULL;
	  if (i > 0 && i <= MAX_USER_GROUP_NAME)
	  {
	    if (q_type == 'u')
	    {
	      pwd = getpwnam(name);
	      if (pwd != NULL)
		q_val.u = uid2Ulong(pwd->pw_uid);
	    }
	    else if (q_type == 'g')
	    {
	      grp = getgrnam(name);
	      if (grp != NULL)
		q_val.u = gid2Ulong(grp->gr_gid);
	    }
	  }
	  if (pwd == NULL && grp == NULL)
	  {
	    char msg[128];
	    sprintf(msg, "%s ID '%s' %s",
		    q_type == 'u' ? "User" : "Group",
		    i == 0 ? "?" : name,
		    i == 0 || i > MAX_USER_GROUP_NAME ? "is illegal" : "fails lookup");
	    filter_error_msg(msg, start);
	  }
	}
	else
	  filter_error_msg("Expected positive floating-point number", ptr);
	
	if (IS_MEMBER(q_type, "mac"))
	{
	  if (isAlpha(*ptr))
	  {
	    switch (*ptr)
	    {
	    case 'Y':
	      factor = SECS_PER_YEAR;
	      break;
	    case 'M':
	      factor = SECS_PER_MONTH;
	      break;
	    case 'W':
	      factor = SECS_PER_WEEK;
	      break;
	    case 'D':
	      factor = SECS_PER_DAY;
	      break;
	    case 'h':
	      factor = SECS_PER_HOUR;
	      break;
	    case 'm':
	      factor = SECS_PER_MIN;
	      break;
	    case 's':
	      factor = 1; /*SECS_PER_SEC*/
	      break;
	    case 'c':
	      factor = 1; /*ABSOLUTE SECS_PER_SEC (clock value)*/
	      break;
	    default:
	      expecting_error(FALSE, "YMWDhmsc", FALSE, FALSE, ptr);
	      break;
	    }
	    time_relative = TRUE;
	    q_relative.u = The_Time; /* Compare using relative time */
	    if (*ptr == 'c')
	    {
	      time_relative = FALSE;
	      q_relative.u = 0; /* Compare using absolute time */
	    }
	    ptr++;
	  }
	  else if (ptr-start < 6)
	    expecting_error(FALSE, "YMWDhmsc' or 'ISO8601-date",
			    FALSE, FALSE, ptr);
	  else if (dot == NULL && ptr-start != 8)
	    /* For purposes of meaningful error messages, assume that 6, 7,
	       or 9+ digits was intended to be an ISO8601-date: */
	    filter_error_msg("Date not in YYYYMMDD[.hhmmss] form", ptr-1);
	  else
	  {
	    /* Re-parse ISO8601-style dates: */
	    factor = 1; /* start over */
	    power2 = 0; /* start over */
	    power10 = 0; /* start over */
	    ptr = start; /* start over */
	    dot = NULL; /* start over */
	    if (q_op == Q_EQ || q_op == Q_NE)
	    {
	      q_val.u = convert_iso8601(Current_Arg, &ptr, FALSE);
	      ptr = start;
	      q_val2.u = convert_iso8601(Current_Arg, &ptr, TRUE);
	    }
	    else if (q_op == Q_GT || q_op == Q_LE)
	      q_val.u = convert_iso8601(Current_Arg, &ptr, TRUE);
	    else
	      q_val.u = convert_iso8601(Current_Arg, &ptr, FALSE);
	    time_relative = FALSE;
	    q_relative.u = 0; /* Compare using absolute time */
	  }
	}
	else
	{
	  if (*ptr == 'K') {
	    factor = 1 << 10; ptr++; }
	  else if (*ptr == 'M') {
	    factor = 1 << 20; ptr++; }
	  else if (*ptr == 'G') {
	    factor = 1 << 30; ptr++; }
	  else if (*ptr == 'T') {
	    factor = 1 << 30; power2 += 10; ptr++; }
	  else if (*ptr == 'k') {
	    factor = 1000; ptr++; }
	  else if (*ptr == 'm') {
	    factor = 1000000; ptr++; }
	  else if (*ptr == 'g') {
	    factor = 1000000000; ptr++; }
	  else if (*ptr == 't') {
	    factor = 1000000000; power10 += 3; ptr++; }
	  else if (IS_NOT_MEMBER(*ptr, "&,|:)}"))
	  {
	    char *choices = (dot != NULL ? "KMGTkmgt" : "KMGTkmgt' or '.");
	    expecting_error(FALSE, choices, TRUE, TRUE, ptr);
	  }
	}

	if (dot == NULL)
	{
	  PhVal old = q_val;
	  q_val.u *= factor;
	  if (q_val.u / factor != old.u)
	    filter_error_msg(NUM_TOO_BIG, start);
	}
	else
	{
	  fraction *= factor;
	  q_val.u = fraction;
	  if (q_val.u < fraction-1 || q_val.u > fraction)
	    filter_error_msg(NUM_TOO_BIG, start);
	}

	while (power2 > 0) {
	  PhVal old = q_val;
	  q_val.u *= 2; power2--;
	  if (q_val.u / 10 != old.u)
	    filter_error_msg(NUM_TOO_BIG, start);
	}
	
	while (power10 > 0) {
	  PhVal old = q_val;
	  q_val.u *= 10; power10--;
	  if (q_val.u / 10 != old.u)
	    filter_error_msg(NUM_TOO_BIG, start);
	}
	
	if (time_relative)
	{
	  /* Positive relative times only know about time from the EPOCH until
	     now -- future time does not exist (unless negative relative times
	     allowed): */
	  if (q_val.u > The_Time)
	    filter_error_msg("Resulting time precedes the EPOCH", start);
	  q_val.u = The_Time - q_val.u;  /* Calculate new time relative to now */
	}

	phStore(start, ptr, q_val, q_relative, q_val2);
      }

      if (q_relative.u != 0)
      {
	/* Relative time's operator meanings are reversed: */
	PhVal tmp = q_val;
	q_val = q_info;
	q_info = tmp;
      }

#   if defined(HAVE_LONG_LONG_TIME)
      signed_compare = IS_MEMBER(q_type, "mac");
#   else
      /*signed_compare = FALSE; (by default) */
#   endif

      switch (q_op)
      {
      case Q_EQ:
	if (q_val2.u == 0)
	  test_result = q_info.u == q_val.u;
	else
	{
	  if (signed_compare)
	    test_result = q_info.s >= q_val.s && q_info.s <= q_val2.s;
	  else
	    test_result = q_info.u >= q_val.u && q_info.u <= q_val2.u;
	}
	break;
      case Q_NE:
	if (q_val2.u == 0)
	  test_result = q_info.u != q_val.u;
	else
	{
	  if (signed_compare)
	    test_result = q_info.s < q_val.s || q_info.s > q_val2.s;
	  else
	    test_result = q_info.u < q_val.u || q_info.u > q_val2.u;
	}
	break;
      case Q_GT:
	if (signed_compare)
	  test_result = q_info.s > q_val.s;
	else
	  test_result = q_info.u > q_val.u;
	break;
      case Q_GE:
	if (signed_compare)
	  test_result = q_info.s >= q_val.s;
	else
	  test_result = q_info.u >= q_val.u;
	break;
      case Q_LT:
	if (signed_compare)
	  test_result = q_info.s < q_val.s;
	else
	  test_result = q_info.u < q_val.u;
	break;
      case Q_LE:
	if (signed_compare)
	  test_result = q_info.s <= q_val.s;
	else
	  test_result = q_info.u <= q_val.u;
	break;
      default:
	/* Can't happen!? */
	break;
      }

      term_good &= negate ? !test_result : test_result;

      switch (*ptr)
      {
      case fexpr_AND:
	if (*(ptr+1) == fexpr_AND) ptr++;	/* Allow && */
      case fexpr_FAND:
	break;

      case fexpr_OR:
	if (*(ptr+1) == fexpr_OR) ptr++;	/* Allow || */
      case fexpr_FOR:
	looking_term = FALSE;
	break;

      case fexpr_END1:
      case fexpr_END2:
	looking_term = FALSE;
	looking_expr = FALSE;
	break;

      default:
	expecting_error(FALSE, NULL, TRUE, TRUE, ptr);
	break;
      }
      ptr++;
    }

    file_is_listable |= term_good;
  }

  if (end) *end = ptr;
  return(file_is_listable);
}


Boole filter_unusual(Dir_Item *file, char *path, char *ptr, char **end)
{
  /***************************************/
  /* Check file against "U"nusual filter */
  /***************************************/
  
  Boole file_is_listable = FALSE;
  Boole looking_expr;
  
  looking_expr = TRUE;
  while (looking_expr && (!file_is_listable || end))
  {
    Boole term_good = TRUE;
    Boole looking_term;

    looking_term = TRUE;
    while (looking_term)
    {
      Boole negate = FALSE;
      Boole test_result = FALSE;
      
      if (*ptr == fexpr_NOT || *ptr == fexpr_FNOT)
      {
	negate = TRUE;
	ptr++;
      }

      /* Maybe list GID != parent GID,
	 symbolic links to files that don't exist */

      switch (*ptr)
      {
      case 't':
	test_result = filter_fexpr(file, path,
				   "~T{r:d:l}", NULL);
	break;

      case 'p':
	test_result = filter_fexpr(file, path, "\
P{o+w:+t},T{~l}|P{+s,+x:+l},~T{d:l}", NULL);
	break;

      case 'P':
	{
	  Uint u = (file->info.st_mode >> 6) & 007;
	  Uint g = (file->info.st_mode >> 3) & 007;
	  Uint o = (file->info.st_mode     ) & 007;
	  test_result = o > g || o > u || g > u;
	}
	break;

      case 'A':
	test_result = get_acl_count(file, path) > 0;
	break;

      case 'l':
	test_result = filter_fexpr(file, path,
				   "Q{l>1}&T{~d}", NULL);
	break;

      case 'u':
	/* Look for NFS nobody/noaccess UIDs and/or
	   UIDs not listed in /etc/passwd: */
	{
	  Ulong uid = uid2Ulong(file->info.st_uid);
	  test_result =
	    uid == 60001 ||
	      uid == 60002 ||
		uid == 65534 ||
		  uid == 4294967294UL ||   /* (Ulong)((short)65534 << 16) */
		    uid2name(uid) == NULL;
	}
	break;

      case 'g':
	/* Look for NFS nobody/noaccess GIDs and/or
	   GIDs not listed in /etc/group: */
	{
	  Ulong gid = gid2Ulong(file->info.st_gid);
	  test_result =
	    gid == 60001 ||
	      gid == 60002 ||
		gid == 65534 ||
		  gid == 4294967294UL ||   /* (Ulong)((short)65534 << 16) */
		    gid2name(gid) == NULL;
	}
	break;

      case 'm':
      case 'a':
      case 'c':
	/* Look for future or zero dates: */
	{
	  ELS_time_t info = (ELS_time_t)(*ptr == 'm' ? file->info.st_mtime :
					 *ptr == 'a' ? file->info.st_atime :
					 file->info.st_ctime);
	  test_result = info > (ELS_time_t)The_Time_in_an_hour || info == 0;
#     if defined(HAVE_LONG_LONG_TIME)
	  /* Pre-epoch test valid for signed time_t only: */
	  test_result |= info < 0;
#     endif
	}
	/* Look for modification time greater than change time (this may
	   seem impossible, but I have observed modification times greater
	   than change times on shared memory files under SunOS5.10!  Such
	   files will not get dumped during incremental backups): */
	if ((*ptr == 'm' || *ptr == 'c') &&
	    (ELS_time_t)file->info.st_mtime > (ELS_time_t)file->info.st_ctime)
	  test_result = TRUE;
	break;

      case 'n':
	test_result = quote_str(file->fname, NULL, QS_FILE) != NULL;
	break;

      case 'N':
	if (using_full_names)
	  test_result = quote_str(path, NULL, QS_FILE) != NULL ||
	    wildcard_match(path, "*/-*");
	else
	  test_result = quote_str(file->fname, NULL, QS_FILE) != NULL;
	break;

      case 'L':
	/* ??? Should the 'L' result get quote tested ??? */
	test_result = FALSE;
# if defined(S_IFLNK)
	/* System has symbolic links: */
	if (file->islink)
	{
	  if (expand_symlink)
	    test_result = file->stat_errno != 0;
	  else
	  {
	    Void sigSafe_stat(path, &file->info);
	    test_result = errno != 0;
	  }
	}
# endif
	break;

      case 's':
	/* Look for sparse files (i.e. partially filled w/data): */
# ifdef NO_STAT_BLOCKS
	filter_warning_msg("Filtering for sparse files unsupported", ptr);
# else
	/* One strange case to beware of is regular files located on a NetApp
	   file server whose size is <= 64 bytes (these peculiar files occupy
	   0 blocks!); thus, as a precaution avoid consideration of any file
	   smaller than 512 bytes. */
	if (file->stat_errno != 0 || /* FIXME???: too forgiving? */
	    (file->info.st_mode & S_IFMT) != S_IFREG ||
	    file->info.st_size <= 512)
	{
	  test_result = FALSE;
	}
	else
	{
	  int blksize = getCwdBlocksize(path);
	  ELS_st_blocks blkbytes = file->info.st_blocks * blksize;

	  if (blksize == 1024)
	  {
	    /* Eliminate non-sparse files from possible HPUX file server: */
	    /* CONSIDER: What would happen if non-HPUX file server with file
	       size > 2^31 were encountered on a non-LARGEFILE system?.
	       blkbytes would overflow, but that wouldn't matter as EFBIG
	       error should occur first!?  Just to be safe make sure
	       LARGEFILES are enabled (i.e. sizeof(ELS_st_blocks) > 4) or
	       fewer than 0x200000 blocks exist before testing for HPUX
	       sparse files. */
	    if (sizeof(ELS_st_blocks) == 4 && file->info.st_blocks >= 0x200000)
	      test_result = FALSE;
	    else
	      test_result = file->info.st_size > blkbytes;
	  }
	  else
	  {
	    /* Detect sparse files from non-HPUX file servers: */
	    test_result = file->info.st_size > blkbytes;
	  }
	}
# endif
	break;

      case 'G':
	/* General tests: */
	test_result = filter_fexpr(file, path,
				   "U{t:p:P:A:l:u:g:m:a:c:N:L:s}", NULL);
	/* Tests 'n' and 'S' were deliberately excluded from 'G'
	   as these are subsets of other tests. */
	break;

      case 'S':
	/* Security tests: */
	test_result = filter_fexpr(file, path, "\
~T{d:l},P{o+w}|T{d},P{o+w,-t}|T{r},P{+s,+x}|T{b:c}", NULL);
	break;

      default:
	expecting_error(!negate, "tpPAlugmacnNLsGS", FALSE, FALSE, ptr);
	break;
      }
      ptr++;
      
      term_good &= negate ? !test_result : test_result;
      
      switch (*ptr)
      {
      case fexpr_AND:
	if (*(ptr+1) == fexpr_AND) ptr++;	/* Allow && */
      case fexpr_FAND:
	break;

      case fexpr_OR:
	if (*(ptr+1) == fexpr_OR) ptr++;	/* Allow || */
      case fexpr_FOR:
	looking_term = FALSE;
	break;

      case fexpr_END1:
      case fexpr_END2:
	looking_term = FALSE;
	looking_expr = FALSE;
	break;

      default:
	expecting_error(FALSE, NULL, TRUE, TRUE, ptr);
	break;
      }
      ptr++;
    }

    file_is_listable |= term_good;
  }

  if (end) *end = ptr;
  return(file_is_listable);
}


Boole filter_ccase(Dir_Item *file, char *path, char *ptr, char **end)
{
  /*****************************************/
  /* Check file against "C"learCase filter */
  /*****************************************/
  
  Boole file_is_listable = FALSE;
  Boole looking_expr;
  
  looking_expr = TRUE;
  while (looking_expr && (!file_is_listable || end))
  {
    Boole term_good = TRUE;
    Boole looking_term;

    looking_term = TRUE;
    while (looking_term)
    {
      Boole negate = FALSE;
      Boole test_result = FALSE;
      
      if (*ptr == fexpr_NOT || *ptr == fexpr_FNOT)
      {
	negate = TRUE;
	ptr++;
      }

      /* NB: Even though SunOS5.7+/Linux2.6.18-x86_64 define st_ino as "long
	 long", ClearCase 7.0.1 still uses 2^31 and above as the inode
	 number of private files.  Linux3.10.0-x86_64 ClearCase 8.0.1 VIEW
	 server with ClearCase 7.1.2 VOB server also behaves this way.
	 FIXME: Need to test with this with both VIEW/VOB running 8.0.1
	 and/or VOB feature levels > 5. */
      switch (*ptr)
      {
      case 'e':	/* VOB element */
	test_result = (file->info.st_ino & 0x80000000) == 0;
	break;
	
      case 'p':	/* VIEW private */
	test_result = (file->info.st_ino & 0x80000000) != 0;
	break;
	
      default:
	expecting_error(!negate, "ep", FALSE, FALSE, ptr);
	break;
      }
      ptr++;
      
      term_good &= negate ? !test_result : test_result;
      
      switch (*ptr)
      {
      case fexpr_AND:
	if (*(ptr+1) == fexpr_AND) ptr++;	/* Allow && */
      case fexpr_FAND:
	break;

      case fexpr_OR:
	if (*(ptr+1) == fexpr_OR) ptr++;	/* Allow || */
      case fexpr_FOR:
	looking_term = FALSE;
	break;

      case fexpr_END1:
      case fexpr_END2:
	looking_term = FALSE;
	looking_expr = FALSE;
	break;

      default:
	expecting_error(FALSE, NULL, TRUE, TRUE, ptr);
	break;
      }
      ptr++;
    }

    file_is_listable |= term_good;
  }

  if (end) *end = ptr;
  return(file_is_listable);
}


/* START OF EXPERIMENT*/
Boole filter_link(Dir_Item *file, char *path, char *ptr, char **end)
{
  /************************************/
  /* Check file against "l"ink filter */
  /************************************/
  
  Boole file_is_listable = FALSE;
  Boole looking_expr;
  
  looking_expr = TRUE;
  while (looking_expr && (!file_is_listable || end))
  {
    Boole term_good = TRUE;
    Boole looking_term;

    looking_term = TRUE;
    while (looking_term)
    {
      Boole negate = FALSE;
      Boole test_result = FALSE;
      
      if (*ptr == fexpr_NOT || *ptr == fexpr_FNOT)
      {
	negate = TRUE;
	ptr++;
      }

      switch (*ptr)
      {
      case 'e': /* Does link destination exist? */
	test_result = FALSE;
# if defined(S_IFLNK)
	/* System has symbolic links: */
	if (file->islink)
	{
	  if (expand_symlink)
	    test_result = file->stat_errno == 0;
	  else
	  {
	    Void sigSafe_stat(path, &file->info);
	    test_result = errno == 0;
	  }
	}
# endif
	break;
	
      case 't': /* Does link destination traverse filesystems? */
	test_result = FALSE;
# if defined(S_IFLNK)
	/* System has symbolic links: */
	if (file->islink)
	{
	  /* NOTE: If link destination is a NON-existent file outside the
	     current filesystem it is NOT considered to be a traverse-link. */
	  if (expand_symlink)
	    test_result = file->stat_errno == 0 && diff_mp(&file->info) != 0;
	  else
	  {
	    struct stat dest;
	    int sts = sigSafe_stat(path, &dest);
	    test_result = (sts == 0 && diff_mp(&dest) != 0);
	  }
	}
# endif
	break;
	
      default:
	expecting_error(!negate, "et", FALSE, FALSE, ptr);
	break;
      }
      ptr++;
      
      term_good &= negate ? !test_result : test_result;
      
      switch (*ptr)
      {
      case fexpr_AND:
	if (*(ptr+1) == fexpr_AND) ptr++;	/* Allow && */
      case fexpr_FAND:
	break;

      case fexpr_OR:
	if (*(ptr+1) == fexpr_OR) ptr++;	/* Allow || */
      case fexpr_FOR:
	looking_term = FALSE;
	break;

      case fexpr_END1:
      case fexpr_END2:
	looking_term = FALSE;
	looking_expr = FALSE;
	break;

      default:
	expecting_error(FALSE, NULL, TRUE, TRUE, ptr);
	break;
      }
      ptr++;
    }

    file_is_listable |= term_good;
  }

  if (end) *end = ptr;
  return(file_is_listable);
}
/* END OF EXPERIMENT*/


void filter_error_msg(char *msg, char *ptr)
{
  carrot_msg(NULL, Current_Opt, Current_Arg, msg, ptr);
  exit(USAGE_ERROR);
}


Boole filter_warning_msg(char *msg, char *ptr)
{
# define MAX_WARN 16
  static char *warned[MAX_WARN];
  static int nwarned = 0;
  int i;
  if (nwarned < MAX_WARN)
  {
    Boole found = FALSE;
    for (i = 0; i < nwarned && !found; i++)
      found = warned[i] == ptr;
    if (!found)
    {
      carrot_msg("Warning", Current_Opt, Current_Arg, msg, ptr);
      warned[nwarned++] = ptr;
      return(TRUE);
    }
  }
  else
    filter_error_msg(msg, ptr);

  return(FALSE);
}


void redundancy_warning(char *types, char *ptr)
{
  if (filter_warning_msg("Redundant term", ptr))
    fprintf(stderr, "\
Perhaps you meant to OR the two preceding %s instead of ANDing?\n\
\n", types);
  return;
}


void expecting_error(Boole negate, char *items, Boole logic, Boole end,
		     char *ptr)
{
  char choices[128];
  char spaced_choices[256];
  char msg[256+32];

  choices[0] = CNULL;
  if (items)
  {
    /*if (negate) strcat(choices, "[!~]");*/
    if (negate) strcat(choices, "!~");
    strcat(choices, items);
  }
  if (logic)
  {
    if (items)
      strcat(choices, "' or '");
    strcat(choices, "& , | :");
  }
  if (end)
  {
    if (items || logic)
      strcat(choices, "' or '");
    strcat(choices, ") }");
  }

  {
    char *c = choices;
    char *sc = spaced_choices;
    Boole spaced_out = FALSE;
    while (*c != CNULL)
    {
      *sc++ = *c++;
      spaced_out = spaced_out || (*c == '\'');
      if (! spaced_out) *sc++ = ' ';
    }
    *sc = CNULL;
    if (!spaced_out) *--sc = CNULL;
  }

  if (logic || (items && strlen(items) > 1))
    sprintf(msg, "Expected one of '%s'", spaced_choices);
  else
    sprintf(msg, "Expected '%s'", spaced_choices);
  filter_error_msg(msg, ptr);
  return;
}
