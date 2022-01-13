/******************************************************************************
  elsMisc.h -- els miscellaneous functions
  
  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: August 9, 2000
  
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

#ifndef ELS__ELSMISC_H
#define ELS__ELSMISC_H

extern int gmatch(char *text, char *pattern);
extern Boole wildcard_match(char *text, char *pattern);

extern char *uid2name(uid_t uid);
extern char *gid2name(gid_t gid);
extern uid_t uidMask(uid_t uid);
extern gid_t gidMask(gid_t gid);
extern Ulong uid2Ulong(uid_t uid);
extern Ulong gid2Ulong(gid_t gid);

extern int strToVer(char *str, char **ptr);
extern char *verToStr(int ver);
extern void printArgs(char **argv, int argc, int version);
extern void printEnvVarSetSh(char *name, char *val);
extern void printEnvVarSet(char *name, char *val);
extern void printEnvVar(char *envvar);
extern void showCommandEnv(char *indent, char *command);

#endif /*ELS__ELSMISC_H*/
