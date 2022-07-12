#ifndef GIT_H
#define GIT_H
#ident "$Id$"

#include <stdbool.h>

/* Source/git.c */
char *is_git(char *dir, bool );
char *load_gitstatus(const char *dir);
char get_gitstatus(char *dir, char *file, char *gs);

#endif
