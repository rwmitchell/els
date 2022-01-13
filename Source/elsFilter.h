/******************************************************************************
  elsFilter.h -- els filter functions and defines
  
  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: September 19, 2011
  
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

#ifndef ELS__ELSFILTER_H
#define ELS__ELSFILTER_H

extern struct stat *getCwdInfo(char *path);
extern int getCwdBlocksize(char *path);
extern void filter_file(Dir_Item *file, char *path);
extern Boole filter_fexpr(Dir_Item *file, char *path, char *ptr, char **end);
extern Boole filter_access(Dir_Item *file, char *path, char *ptr, char **end);
extern Boole filter_type(Dir_Item *file, char *path, char *ptr, char **end);
extern Boole filter_perm(Dir_Item *file, char *path, char *ptr, char **end);
extern Boole filter_quantity(Dir_Item *file, char *path, char *ptr, char **end);
extern Boole filter_unusual(Dir_Item *file, char *path, char *ptr, char **end);
extern Boole filter_ccase(Dir_Item *file, char *path, char *ptr, char **end);
extern Boole filter_link(Dir_Item *file, char *path, char *ptr, char **end);

extern void filter_error_msg(const char *msg, char *ptr);
extern Boole filter_warning_msg(char *msg, char *ptr);
extern void redundancy_warning(char *types, char *ptr);
extern void expecting_error(Boole negate, char *items, Boole logic, Boole end,
		     char *ptr);

#define MAX_FEXPR_FILTER  16
extern int fexpr_ORcount;
extern int fexpr_ANDcount;
extern char *fexpr_ORfilter[MAX_FEXPR_FILTER];
extern char *fexpr_ANDfilter[MAX_FEXPR_FILTER];
#define fexpr_NOT	'!'
#define fexpr_AND	'&'
#define fexpr_OR	'|'
#define fexpr_FNOT	'~'
#define fexpr_FAND	','
#define fexpr_FOR	':'
#define fexpr_BEGIN1	'('
#define fexpr_END1	')'
#define fexpr_BEGIN2	'{'
#define fexpr_END2	'}'

/* List of wildcard patterns for "E"xcluding file/dir names: */
#define MAX_EXC_FILTER  16
extern int exc_file_count;
extern char *exc_file_filter[MAX_EXC_FILTER];
extern int exc_dir_count;
extern char *exc_dir_filter[MAX_EXC_FILTER];

/* List of wildcard patterns for "I"ncluding file/dir names: */
#define MAX_INC_FILTER  16
extern int inc_file_count;
extern char *inc_file_filter[MAX_INC_FILTER];
extern int inc_dir_count;
extern char *inc_dir_filter[MAX_INC_FILTER];

#endif /*ELS__ELSFILTER_H*/
