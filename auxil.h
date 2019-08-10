/******************************************************************************
  auxil.h -- Auxiliary functions and defines
  
  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: July 17, 2000
  
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

#ifndef ELS__AUXIL_H
#define ELS__AUXIL_H

extern void *memAlloc(int size);
extern void *memAllocZero(int size);
extern void memFree(void *mem);
extern void memShow(FILE *out);

extern char *strdup_ns(const char *str);
extern int strcmp_ci(const char *s1, const char *s2);
extern int strncmp_ci(const char *s1, const char *s2, int n);
extern long strtol(const char *str, char **ptr, int base);
extern Ulong strtoul(const char *str, char **ptr, int base);
#ifdef USE_STRTOULL
extern long long strtoll(const char *str, char **ptr, int base);
extern unsigned long long strtoull(const char *str, char **ptr, int base);
#endif /*USE_STRTOULL*/
#ifdef USE_SNPRINTF
extern int snprintf(char *s, size_t n, const char *format, /*args*/ ...);
#endif /*USE_SNPRINTF*/
extern int envSet(char *var);
extern int envSetNameVal(char *name, char *val);
extern int envUnset(char *var);
extern int envGetValue(char *var);
extern Boole envGetBoole(char *var);
extern char *strFindFirstOccurrence(char *str, char *look_for);
extern void carrot_msg(char *tag, char *opt, char *arg, char *msg, char *ptr);
extern char *quote_fname(char *fname, Boole dash_b, Boole dash_Q, Boole plus_q);
extern char *quote_str(char *str, char **ptr, Ulong mode);
extern void errorMsg(char *msg);
extern void errorMsgExit(char *msg, int exit_sts);
extern void errnoMsg(int errnum, char *msg);
extern void errnoMsgExit(int errnum, char *msg, int exit_sts);
extern char *Current_Arg;
extern void opt_error_msg(const char *msg, char *ptr);
extern void arg_error_msg(char *msg, char *arg, char *ptr);
extern void usage_error(void);

extern char *Progname;
extern Boole warning_suppress;

#define QS_ARG  0x01
#define QS_FILE 0x02
#define QS_ENV  0x04

#endif /*ELS__AUXIL_H*/
