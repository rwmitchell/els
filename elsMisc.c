/******************************************************************************
  elsMisc.c -- els miscellaneous functions
  
  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: June 21, 2005
  
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
#include <sys/types.h>
#include <sys/stat.h>	/* sys/stat.h needed by els.h */
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "defs.h"
#include "auxil.h"
#include "els.h"
#include "elsMisc.h"
#include "sysdep.h"


/******************************************************************************
 * gmatch: This function can be used as a replacement for Solaris' gmatch()
 * and provides a superset of functionality compared to Solaris' version. */

int gmatch(char *text, char *pattern)
{
  return(wildcard_match(text, pattern));
}


/******************************************************************************
 * wildcard_match: Test whether text matches a wildcard pattern using the
 * same syntax and semantics as used by /bin/csh, /bin/tcsh, or /bin/bash.
 *
 * Written by Mark Baranowski, (c)1996.
 *
 *  ?	Scan a single text character
 *  *	Scan zero or more text characters
 *  []	Test whether a text character matches a set or a range of chars.  For
 *	example, match A through M, all digits, or the set "!@#": [A-M0-9!@#]
 *  [^]	Same as above but negate the result.  For example, match all
 *	characters EXCEPT A through M, digits, or the set "!@#": [^A-M0-9!@#]
 *  {}	Match any of the enclosed substrings separated by commas
 *  \	Match following character verbatim
 *  All other characters not listed above are matched verbatim.
 *
 *  NB: The meta characters ?*[]{,}\ might need to be prefaced by \ in certain
 *	contexts if a LITERAL match is desired.  For example, if attempting
 *	to LITERALLY match any of ?*[{} then these must be prefaced by \ in
 *	all contexts except when inside [...].  If attempting to LITERALLY
 *	match "," then it needs to be prefaced by \ only when inside {...}.
 *	Finally, if attempting to LITERALLY match ] or \ then these should
 *	always be prefaced by \ regardless of the context.
 *
 * EXAMPLES:
 *  Test whether text starts with a letter or digit:
 *	wildcard_match(text, "[a-zA-Z0-9]*");
 *  Test whether text contains at least three or more characters:
 *	wildcard_match(text, "???*");
 *  Test whether text is the name of a *.c or *.h file:
 *	wildcard_match(text, "*.[ch]");
 *  Test whether text is the name of a *.c, *.h, *.cc, *.cpp, *.hh, *.hpp file:
 *	wildcard_match(text, "*.{[ch],cc,cpp,hh,hpp}");
 *  Test whether text contains the substring "Markb" or "markb":
 *	wildcard_match(text, "*[Mm]arkb*");
 *	wildcard_match(text, "*{Markb,markb}*");
 *  Test whether text matches any of ab*ch, ab*dh, aeeffh, aeegg*h, or aeeh:
 *	wildcard_match(text, "a{b*[cd],ee{ff,gg*,}}h");
 *	(Notice braces can be used recursively, e.g. {xx,yy{aa,bb}} and a match
 *	with an empty comma field e.g. {xx,} matches either "xx" or nothing.)
 */

Boole wildcard_match(char *text, char *pattern)
{
  if (text == NULL || pattern == NULL)
    return(FALSE);

  for (;;)
  {
    switch (*pattern)
    {
    case CNULL:
      /* END OF PATTERN: */
      return(*text == CNULL);
      break;

    case '?':
      /* SCAN ONE CHAR OF ANY KIND: */
      if (*text == CNULL) return(FALSE);
      text++;
      pattern++;
      break;

    case '[':
      /* MATCH ONE CHAR FROM A SET OR A RANGE: */
      {
	Boole match = FALSE;
	Boole first = TRUE;
	Boole negate;
	char lastpat = CNULL;
	pattern++;
	negate = (*pattern == '^');
	if (negate) pattern++;
	while (*pattern != CNULL && *pattern != ']')
	{
	  if (*pattern == '\\')
	  {
	    pattern++;
	    if (*pattern == CNULL) return(FALSE); /* Missing ] */
	    match |= (*text == *pattern);
	  }
	  else if (*pattern == '-' && !first)
	  {
	    pattern++;
	    if (*pattern == CNULL) return(FALSE); /* Missing ] */
	    match |= (lastpat <= *text && *text <= *pattern);
	  }
	  else
	  {
	    match |= (*text == *pattern);
	  }
	  lastpat = *pattern;
	  first = FALSE;
	  pattern++;
	}
	if (*pattern == CNULL) return(FALSE); /* Missing ] */
	match ^= negate;
	if (!match) return(FALSE);
      }
      text++;
      pattern++;
      break;

    case '*':
      /* SCAN ZERO OR MORE CHARS: */
      while (*pattern == '*') pattern++;
      if (*pattern == CNULL) return(TRUE);

      /* Speed things up by scanning ahead for the first match of a REGULAR
	 CHAR.  This step is optional and could be skipped without affecting
	 the final results of this algorithm: */
      if (IS_NOT_MEMBER(*pattern, "?[\\{"))
	while (*text != CNULL && *text != *pattern) text++;

      /* Recursively look for consecutive groups of matches and/or
	 nested "{}"patterns: */
      {
	Boole match;
	do {
	  /* Test up to and including *text==CNULL in cases such as "*{,}": */
	  match = wildcard_match(text, pattern);
	  if (*text == CNULL) break;
	  text++;
	} while (!match);

	return(match);
      }
      break;

    case '{':
      /* RECURSIVELY SCAN SUBSTRINGS: */
      {
	char *subptr;
	char *substart = NULL;
	int depth;
	char *poststart;
	int postlen;

	subptr = pattern;
	depth = 0;
	do
	{
	  if (*subptr == '\\')
	    subptr++;
	  else if (*subptr == '{')
	    depth++;
	  else if (*subptr == '}')
	    depth--;
	  if (*subptr == CNULL) return(FALSE); /* Missing } */
	  subptr++;
	} while (depth > 0);
	poststart = subptr;
	postlen = strlen(poststart);

	subptr = pattern;
	depth = 0;
	do
	{
	  Boole doit = FALSE;
	  if (*subptr == '\\')
	    subptr++;
	  else if (*subptr == '{')
	  {
	    depth++;
	    if (depth == 1) substart = subptr + 1;
	  }
	  else if (*subptr == ',')
	  {
	    if (depth == 1) doit = TRUE;
	  }
	  else if (*subptr == '}')
	  {
	    if (depth == 1) doit = TRUE;
	    depth--;
	  }
	  if (doit)
	  {
#	    define NEWBUF_SIZE  32
	    Boole match;
	    char *newpat, newbuf[NEWBUF_SIZE];
	    int sublen, newlen;

	    sublen = subptr - substart;
	    newlen = sublen + postlen + 1;
	    newpat = newbuf;
	    if (newlen > NEWBUF_SIZE) newpat = memAlloc(newlen);

	    strncpy(newpat, substart, sublen);
	    strncpy(&newpat[sublen], poststart, postlen+1);
	    match = wildcard_match(text, newpat);

	    if (newlen > NEWBUF_SIZE) memFree(newpat);
	    if (match) return(TRUE);
	    substart = subptr + 1;
	  }
	  subptr++;
	} while (depth > 0);
      }
      return(FALSE);
      break;

    case '\\':
      /* MATCH ONE SPECIAL CHAR: */
      pattern++;
      if (*text == CNULL) return(FALSE);
      if (*text != *pattern) return(FALSE);
      text++;
      if (*pattern != CNULL) pattern++; /* consider pattern of: "xyz\^@abc" */
      break;

    default:
      /* MATCH ONE REGULAR CHAR: */
#ifdef REQUIRE_RH_QUOTES
      /* Define REQUIRE_RH_QUOTES to require \] and \} quoting for unbalanced
	 implied literals (wildcard matching under /bin/csh works unquoted): */
      if (*text == ']') return(FALSE); /* Missing [ */
      if (*text == '}') return(FALSE); /* Missing { */
#endif
      if (*text == CNULL) return(FALSE);
      if (*text != *pattern) return(FALSE);
      text++;
      pattern++;
      break;
    }
  }
  /*NOTREACHED*/
}


char *uid2name(uid_t uid)
{
/* UID_CACHE_SIZE must be a power of two: */
# define UID_CACHE_SIZE 64
  static uid_t uids[UID_CACHE_SIZE];
  static char  name[UID_CACHE_SIZE][MAX_USER_GROUP_NAME+1];
  static Boole first_time = TRUE;
  struct passwd *passwd;
  int i;

  /* ??? Should uid be purified by first passing through uid2Ulong ??? */

  if (first_time)
  {
    /* The zero-th entry must initially be non-zero and all other entries
       must initially be zero in order for the cache to function: */
    uids[0] = 1;
    first_time = FALSE;
  }

  i = uid & (UID_CACHE_SIZE-1);
  if (uids[i] != uid)
  {
    if ((passwd = getpwuid(uid)) != NULL)
      strncpy(name[i], passwd->pw_name, MAX_USER_GROUP_NAME);
    else
      name[i][0] = CNULL;
    uids[i] = uid;
  }

  if (name[i][0] == CNULL)
    return(NULL);
  else
    return(name[i]);
}


char *gid2name(gid_t gid)
{
/* GID_CACHE_SIZE must be a power of two: */
# define GID_CACHE_SIZE 32
  static gid_t gids[GID_CACHE_SIZE];
  static char  name[GID_CACHE_SIZE][MAX_USER_GROUP_NAME+1];
  static Boole first_time = TRUE;
  struct group *group;
  int i;

  /* ??? Should gid be purified by first passing through gid2Ulong ??? */

  if (first_time)
  {
    /* The zero-th entry must initially be non-zero and all other entries
       must initially be zero in order for the cache to function: */
    gids[0] = 1;
    first_time = FALSE;
  }

  i = gid & (GID_CACHE_SIZE-1);
  if (gids[i] != gid)
  {
    if ((group = getgrgid(gid)) != NULL)
      strncpy(name[i], group->gr_name, MAX_USER_GROUP_NAME);
    else
      name[i][0] = CNULL;
    gids[i] = gid;
  }

  if (name[i][0] == CNULL)
    return(NULL);
  else
    return(name[i]);
}


/* As of release 1.48b2, ELS will consider any UID/GID in the range of
   0xffff0000UL to 0xffffffffUL to really be in the range 0 to 65535, as
   any such UID/GID in this range probably fell victim to sign-extension
   (UID/GID sign-extension usually occurs when an old NFS2 file systems
   using 16-bit UIDs/GIDs is misinterpreted by a NFS3 host using 32-bit
   UIDs/GIDs).  The environment variable ELS_NO_MASK_ID, if defined, will
   cause sign-extended UIDs/GIDs to be masked appropriately.  As of release
   1.50 this behavior was changed to display actual UID/GIDs *unless*
   environment variable ELS_MASK_ID is defined. */
/* Please see els/TECH_NOTES on how to unintentionally create 32-bit negative
   UIDs/GIDs (although I don't recommend doing this). */

uid_t uidMask(uid_t uid)
{
#if ELS_SIZEOF_uid_t > 2
  if (MaskId && (uid & 0xffff0000) == 0xffff0000)
    uid &= 0x0000ffff;
#endif
  return(uid);
}


gid_t gidMask(gid_t gid)
{
#if ELS_SIZEOF_gid_t > 2
  if (MaskId && (gid & 0xffff0000) == 0xffff0000)
    gid &= 0x0000ffff;
#endif
  return(gid);
}


Ulong uid2Ulong(uid_t uid)
{
  /* Promote to Ulong without sign extension: */
  ELS_uid_t id = uidMask(uid);
  return(id);
}


Ulong gid2Ulong(gid_t gid)
{
  /* Promote to Ulong without sign extension: */
  ELS_gid_t id = gidMask(gid);
  return(id);
}


int strToVer(char *str, char **ptr)
{
  int major = 0;
  int minor = 0;
  major = strtol(str, &str, 10);
  if (major < 100) major *= 100;
  if (*str == '.')
  {
    str++;
    if (isDigit(*str)) minor += (*str++ - '0') * 10;
    if (isDigit(*str)) minor += (*str++ - '0');
  }
  if (ptr) *ptr = str;
  return(major + minor);
}


char *verToStr(int ver)
{
  char *ver_str = memAlloc(16);
  int major = ver / 100;
  int minor = ver - major * 100;
  if (major > 99) major = 99;
  if (minor > 99) minor = 99;
  sprintf(ver_str, "%d.%02d", major, minor);
  return(ver_str);
}


void printArgs(char **argv, int argc, int version)
{
  int i, nout;
  for (i = 0, nout = 0; i < argc && nout < 512; i++)
  {
    char *arg = quote_str(argv[i], NULL, QS_ARG);
    if (arg == NULL) arg = argv[i];
    if (i > 0) printf(" ");
    printf("%s", arg);
    nout += strlen(arg);
    if (i == 0 && version != 0) printf(" +v=%s", verToStr(version));
  }
  if (i < argc) printf(" ...");
  return;
}


void printEnvVarSetSh(char *name, char *val)
{
  char *qval;
  qval = quote_str(val, NULL, QS_ARG|QS_ENV); /* Quote any special chars */
  if (qval != NULL) val = qval;
  printf("%s=%s; export %s", name, val, name);
  return;
}


void printEnvVarSet(char *name, char *val)
{
  char *qval;
  static Boole shell_checked = FALSE;
  static Boole is_csh = FALSE;
  qval = quote_str(val, NULL, QS_ARG|QS_ENV); /* Quote any special chars */
  if (qval != NULL) val = qval;
  if (! shell_checked && VersionLevel >= 152)
  {
    char *shell = getenv("SHELL");
    char *last_slash = NULL;
    if (shell != NULL && (last_slash = strrchr(shell, '/')) != NULL)
      is_csh = (strcmp(last_slash, "/csh") == 0 ||
		strcmp(last_slash, "/tcsh") == 0);
    shell_checked = TRUE;
  }
  if (is_csh)
    printf("setenv %s %s", name, val);
  else
    printf("%s=%s; export %s", name, val, name);
  return;
}


void printEnvVar(char *envvar)
{
  char *name_val = strdup(envvar);
  char *name, *eq, *val;
  
  name = name_val;
  eq = strchr(name_val, '=');
  if (eq != NULL) {
    *eq = CNULL;  /* Split name_val in half at the '=' */
    val = eq + 1; /* Second half is value */
  }
  else {
    val = "";
  }
  printEnvVarSet(name, val);
  
  memFree(name_val);
  return;
}


/* Note: Darwin 10.7 complains about "printf(indent)" and "printf(blank_line)"
   saying: "format not a string literal and no format arguments"; thus, the
   printWS(ws) macro used to ease his worries: */
#define printWS(ws) printf("%s", ws)

void showCommandEnv(char *indent, char *command)
{
  char blank_line[3];
  char *indent2 = "  ";

  if (indent == NULL) indent = "";
  if (indent[0] == CNULL)
    strcpy(blank_line, "\n");
  else
  {
    blank_line[0] = indent[0];
    strcpy(&blank_line[1], "\n");
  }

  /* Show user and date: */
  {
    char vdate[42];
    T_print(vdate, "v", The_Time, NULL, FALSE, FALSE);
    
    printWS(indent); printf("%s by %s\n", command, get_userAtHost());
    printWS(indent); printf("on %s\n", vdate);
    printWS(blank_line);
  }

  /* Show any SYSTEM and all ELS_* env-vars that may affect behavior: */
  {
    Boole first_msg = TRUE;
    extern char **environ;
    char **e;

    /* FYI: root's TZ setting under SunOS5 is "".  Apparently this has
       no negative affect other than to cause the system TZ value to be used
       instead.  Perhaps in such cases this ambiguity should be suppressed!?
       Or should the TZ setting always be faithfully reported? */
    /* NB: Ignore LC_ALL setting, as ELS gives precedence to ELS_LC_* and
       LC_* settings.  So by recording these settings, user can still repeat
       previous results regardless of LC_ALL or system settings. */
    for (e = environ;  *e != NULL;  e++)
      if ((strncmp("TZ=", *e, 3) == 0) ||
	  (strncmp("ELS_", *e, 4) == 0))
      {
	/* ELS envvars we don't want to see: */
	if (strncmp("ELS_32BIT", *e, 9) == 0) continue;
	if (strncmp("ELS_LC_TIME=", *e, 12) == 0 && !useLcTime) continue;
	if (strncmp("ELS_LC_COLLATE=", *e, 15) == 0 && !useLcCollate) continue;

	if (first_msg)
	{
	  printWS(indent); printf("ENVIRONMENT:\n");
	  first_msg = FALSE;
	}
	printWS(indent); printWS(indent2); printEnvVar(*e); printf("\n");
      }

    if (! first_msg) printWS(blank_line);
  }

  /* Show commands used: */
  {
    printWS(indent); printf("COMMANDS:\n");

    /* Show any ClearCase settings that may affect behavior: */
    {
      char *cc;
      if ((cc = getenv("CLEARCASE_ROOT")) != NULL)
      {
	if (strncmp("/view/", cc, 6) == 0) cc += 6;
	printWS(indent); printWS(indent2); printf("cleartool setview %s\n", cc);
      }
    }
    /* Show current working directory and command used: */
    {
      char *cwd, *qcwd, dir[MAX_DNAME];
      /* NB: Linux 3.13/gcc4.8.4 complains if getcwd()'s return unused(!) */
      if ((cwd = getcwd(dir, MAX_DNAME)) == NULL)
      {
	/* ANOMALY: Using SunOS5.8/NetApp "cd .snapshot/name; els +c",
	   getcwd() returns NULL without errno being set.  Even odder is
	   that envvar PWD seems to correct in most if not all cases. */
	char *PWD;
	if (errno != 0) errnoMsg(-1, NULL);
	cwd = "<cannot determine cwd>";
	PWD = getenv("PWD");
	if (PWD != NULL) cwd = PWD;
      }
      if (strncmp("/tmp_mnt/", cwd, 9) == 0) cwd += 8; /* automount fodder */
      qcwd = quote_str(cwd, NULL, QS_ARG);
      if (qcwd != NULL) cwd = qcwd;

      printWS(indent); printWS(indent2); printf("cd %s\n", cwd);
      printWS(indent); printWS(indent2);
      printArgs(Argv, Argc, VersionLevelArg ? 0 : VersionLevel); printf("\n");
    }

    printWS(blank_line);
  }

  return;
}
