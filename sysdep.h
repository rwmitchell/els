/******************************************************************************
  sysdep.h -- System dependencies
  
  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: March 14, 2001
  
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

#ifndef ELS__SYSDEP_H
#define ELS__SYSDEP_H

extern char *get_userAtHost(void);
extern int get_acl_count(Dir_Item *file, char *path);
extern ELS_st_blocks getStatBlocks(struct stat *info);
extern struct stat *set_mp(struct stat *dir);
extern int diff_mp(struct stat *dir);

#endif /*ELS__SYSDEP_H*/
