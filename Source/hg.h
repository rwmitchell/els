#ifndef HG_H
#define HG_H
#ident "$Id$"

#include <sys/stat.h>

/* hg.c */
_Bool RMisdir (const char *name);
char *loadpipe(const char *cmd, off_t *f_sz);
char *fullpath(char *path);
char *is_hg(char *dir );
char *load_hgstatus(const char *dir);
char  get_hgstatus( char *dir, char *file, char *hgs );

#endif
