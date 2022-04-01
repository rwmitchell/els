#ifndef GIT_H
#define GIT_H
#ident "$Id$"

/* Source/git.c */
char *is_git(char *dir);
char *load_gitstatus(const char *dir);
char get_gitstatus(char *dir, char *file, char *gs);

#endif
