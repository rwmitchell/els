/******************************************************************************
  auxil.c -- Auxiliary functions
  
  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: May 20, 2008
  
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
#include <string.h>
#include <stdlib.h>

#include "defs.h"
#include "auxil.h"

extern void give_usage_or_help(char U_or_H);

Local Ulong memAllocTotal = 0;
Local Ulong memAllocCount = 0;
Local Ulong memFreeCount = 0;


void *memAlloc(int size)
{
  char *ptr;
  
  ptr = malloc(size);
  if (ptr == NULL)
  {
    fprintf(stderr, "%s: Unable to allocate working memory\n",
	    Progname);
    exit(1);
  }
  memAllocTotal += size;
  memAllocCount++;
  
  return((void *)ptr);
}


void *memAllocZero(int size)
{
  char *ptr = memAlloc(size);
  memset(ptr, 0, size);
  return((void *)ptr);
}


void memFree(void *mem)
{
  free(mem);
  memFreeCount++;
  return;
}


void *memRealloc( void *ptr, off_t osize, off_t asize ) {
  void *new = memAllocZero( osize + asize );

  if ( new == NULL ) {
    fprintf(stderr, "%s: Unable to allocate working memory\n",
	    Progname);
    exit(1);
  }
  memcpy( new, ptr, osize );
  memFree( ptr );
  return( new );
}

void memShow(FILE *out)
{
  fprintf(out, "\rmemAlloc: Total, Alloc, Free, InUse: %lu %lu %lu %lu\n",
	  memAllocTotal, memAllocCount, memFreeCount,
	  memAllocCount-memFreeCount);
  return;
}


/* strdup_ns: same as strdup, except duplicate string contains No Spaces */
char *strdup_ns(register const char *str)
{
  char *newstr = memAlloc(strlen(str)+1);
  register char *dup = newstr;
  while (*str != CNULL)
  {
    if (!isSpace(*str)) *dup++ = *str;
    str++;
  }
  *dup = CNULL;
  return(newstr);
}


/* strcmp_ci: same as strcmp, except comparisons are case-insensitive */
int strcmp_ci(register const char *s1,
	      register const char *s2)
{
  register char c1, c2;
  int diff = 0;
  do
  {
    c1 = *s1++;
    c2 = *s2++;
    if (isUpper(c1)) c1 = toLower(c1);
    if (isUpper(c2)) c2 = toLower(c2);
  } while ((diff = c1 - c2) == 0 && c1 && c2);
  return(diff);
}


/* strncmp_ci: same as strncmp, except comparisons are case-insensitive */
int strncmp_ci(register const char *s1,
	       register const char *s2,
	       register int n)
{
  register char c1, c2;
  int diff = 0;
  do
  {
    if (n-- <= 0) break;
    c1 = *s1++;
    c2 = *s2++;
    if (isUpper(c1)) c1 = toLower(c1);
    if (isUpper(c2)) c2 = toLower(c2);
  } while ((diff = c1 - c2) == 0 && c1 && c2);
  return (diff);
}


/* Not all unixes have strtoul, so here's my version: */
Local Ulong _strtoul(const char *str, char **ptr, int base,
		     Boole scan_ws, Boole is_signed,
		     Boole *negative, Boole *overflow)
{
  int digit;
  Ulong result, last_result;
  const char *next = str;

  result = 0;
  last_result = 0;
  *negative = FALSE;
  *overflow = FALSE;
  
  /* Scan leading whitespace only if specified: */
  if (scan_ws) while (isSpace(*str)) str++;
  
  if (*str == '-')
  {
    *negative = TRUE;
    str++;
  }
  else if (*str == '+')
    str++;
  
  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   * Base may be either explicitly specified (i.e. the parameter "base" is
   * non-zero) or implicitly discerned (e.g. the number starts with 0x,
   * etc.).  If the base is explicit then the number may or may not be
   * preceded with an implicit base; however, if both an implicit and an
   * explicit base are specified they both need to agree.
   */
  
  if (*str == '0')
  {
    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
     * Attempt to discern an implicit base:
     *	0x... --> implicit base = 16
     *	0...  --> implicit base = 8
     */
    str++;
    next = str;
    if ((base == 0 || base == 16) && (*str == 'x' || *str == 'X'))
    {
      str++;
      base = 16;
    }
    else if ((base == 0 || base == 8) && (*str != '\0'))
    {
      base = 8;
    }
    else
    {
      /* We have either a single zero or an explicit base: */
      str--;	/* restore the zero, we may need it later */
      next = str;
    }
  }
  
  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   * If no explicit base was specified and no implicit base was discerned,
   * then use base 10:
   */
  if (base == 0) base = 10;

  if (base < 2 || base > 36)
    errno = EINVAL;
  else
  {
    Boole digit_ok;
    do {
      digit = base;
      if (isascii(*str))
      {
	if (isDigit(*str))
	  digit = *str - '0';
	else if (isLower(*str))
	  digit = *str - 'a' + 10;
	else if (isUpper(*str))
	  digit = *str - 'A' + 10;
      }
      digit_ok = (digit < base);
      
      if (digit_ok)
      {
	result *= base;
	result += digit;
	str++;
	next = str;
	if (result / base != last_result) *overflow = TRUE;
	if (is_signed && (long)result < 0) *overflow = TRUE;
	last_result = result;
      }
    } while (digit_ok && *str);
    
    if (*negative) result = (Ulong)(-(long)result);
  }
    
  /* Certain compilers complain about de-consting "next" even though the
     pointer VARIABLE itself is non-const!: */
  if (ptr) *ptr = (char *)next;
  
  return(result);
}


long strtol(const char *str, char **ptr, int base)
{
  Boole negative, overflow;
  long l;
  l = (long)_strtoul(str, ptr, base, TRUE, TRUE, &negative, &overflow);
  if (overflow)
  {
    long maxneg = MAXNEG_LONG;
    errno = ERANGE;
    return(negative ? maxneg : ~maxneg);
  }
  else
    return(l);
}


Ulong strtoul(const char *str, char **ptr, int base)
{
  Boole negative, overflow;
  Ulong ul;
  ul = _strtoul(str, ptr, base, TRUE, FALSE, &negative, &overflow);
  if (overflow)
  {
    errno = ERANGE;
    return(~(Ulong)0);
  }
  else
    return(ul);
}


#ifdef USE_STRTOULL
/* Not all unixes have strtoull, so here's my version: */
Local unsigned long long _strtoull(const char *str, char **ptr, int base,
				   Boole scan_ws, Boole is_signed,
				   Boole *negative, Boole *overflow)
{
  int digit;
  unsigned long long result, last_result;
  const char *next = str;

  result = 0;
  last_result = 0;
  *negative = FALSE;
  *overflow = FALSE;
  
  /* Scan leading whitespace only if specified: */
  if (scan_ws) while (isSpace(*str)) str++;
  
  if (*str == '-')
  {
    *negative = TRUE;
    str++;
  }
  else if (*str == '+')
    str++;
  
  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   * Base may be either explicitly specified (i.e. the parameter "base" is
   * non-zero) or implicitly discerned (e.g. the number starts with 0x,
   * etc.).  If the base is explicit then the number may or may not be
   * preceded with an implicit base; however, if both an implicit and an
   * explicit base are specified they both need to agree.
   */
  
  if (*str == '0')
  {
    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
     * Attempt to discern an implicit base:
     *	0x... --> implicit base = 16
     *	0...  --> implicit base = 8
     */
    str++;
    next = str;
    if ((base == 0 || base == 16) && (*str == 'x' || *str == 'X'))
    {
      str++;
      base = 16;
    }
    else if ((base == 0 || base == 8) && (*str != '\0'))
    {
      base = 8;
    }
    else
    {
      /* We have either a single zero or an explicit base: */
      str--;	/* restore the zero, we may need it later */
      next = str;
    }
  }
  
  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   * If no explicit base was specified and no implicit base was discerned,
   * then use base 10:
   */
  if (base == 0) base = 10;

  if (base < 2 || base > 36)
    errno = EINVAL;
  else
  {
    Boole digit_ok;
    do {
      digit = base;
      if (isascii(*str))
      {
	if (isDigit(*str))
	  digit = *str - '0';
	else if (isLower(*str))
	  digit = *str - 'a' + 10;
	else if (isUpper(*str))
	  digit = *str - 'A' + 10;
      }
      digit_ok = (digit < base);
      
      if (digit_ok)
      {
	result *= base;
	result += digit;
	str++;
	next = str;
	if (result / base != last_result) *overflow = TRUE;
	if (is_signed && (long long)result < 0) *overflow = TRUE;
	last_result = result;
      }
    } while (digit_ok && *str);
    
    if (*negative) result = (unsigned long long)(-(long long)result);
  }
    
  /* Certain compilers complain about de-consting "next" even though the
     pointer VARIABLE itself is non-const!: */
  if (ptr) *ptr = (char *)next;
  
  return(result);
}


long long strtoll(const char *str, char **ptr, int base)
{
  Boole negative, overflow;
  long long l;
  l = (long long)_strtoull(str, ptr, base, TRUE, TRUE, &negative, &overflow);
  if (overflow)
  {
    long long maxneg = MAXNEG_LLONG;
    errno = ERANGE;
    return(negative ? maxneg : ~maxneg);
  }
  else
    return(l);
}


unsigned long long strtoull(const char *str, char **ptr, int base)
{
  Boole negative, overflow;
  unsigned long long ul;
  ul = _strtoull(str, ptr, base, TRUE, FALSE, &negative, &overflow);
  if (overflow)
  {
    errno = ERANGE;
    return(~(unsigned long long)0);
  }
  else
    return(ul);
}
#endif /*USE_STRTOULL*/


# ifdef USE_SNPRINTF
/*****************************************************************************/
/* vsnprintf(), snprintf()

   These are a poor-man's version of vsnprinf/snprintf that piggy-back off
   the system provided vsprintf.  vsnprintf uses 256 bytes of stack space
   for small buffers (so as to preserve re-entrancy), and for larger
   buffers it hopes for the best as we don't want to use malloc nor do we
   want to chew up a large amount of stack.

   Linux 2.6 says the following about sprintf/vsprintf/snprintf/vsnprintf:
   Upon successful return, these functions return the number of characters
   printed (not including the NULL used to end output to strings).  The
   functions snprintf and vsnprintf do not write more than the specified
   buffer "size" in bytes (including the NULL).  If the output was
   truncated due to this limit then the return value is the number of
   characters (not including the NULL) which would have been written to
   the final string if enough space had been available. Thus, a return
   value of the specified buffer "size" or more means that the output was
   truncated.  If an output error is encountered, a negative value is
   returned.

   This contradicts SunOS5.9 which says snprintf/vsnprintf() returns the
   number of characters that were formatted *INCLUDING* the NULL character.
   SunOS5.9 agree in all other respects.

   This implmentation follows Linux2.6 and it also insures that buffer
   is always NULL terminated. */

#include <stdarg.h>

# ifdef __HIDE_VSNPRINTF__
static
# endif
int vsnprintf(char *s, size_t n, const char *format, va_list ap)
{
  int iprint;

  if ((int)n <= 0)
    iprint = -1;  /* Indicate error */
  else
  {
#   define MIN_BUFF_SIZE 256
    char buff_stack[MIN_BUFF_SIZE];
    char *buff;

    /* Guarantee that "buff" has a minimum of MIN_BUFF_SIZE bytes, as it's
       most likely the little buffers that overflow -- the bigger guys are on
       their own, as it's undesirable to chew up a large amount of stack.
       I originally used malloc()/free() to create a buffer 4*n for the bigger
       guys, but much to my chagrin I discovered an interrupt procedure that
       was calling snprintf() and then passing the string to logMsg -- this
       innocent act was causing the thread to block!  Thus, to protect naive
       applications I now shun malloc()/free() during all I/O: */
    if (n < MIN_BUFF_SIZE)
      buff = buff_stack;	
    else
      buff = s;

    if (buff == NULL)
      iprint = -1;  /* Indicate error */
    else
    {
      iprint = vsprintf(buff, format, ap);
      if (buff != s) strncpy(s, buff, n);

      /* NULL terminate if truncation occurred: */
      if (iprint >= (int)n) s[n-1] = '\0';
    }
  }
  return(iprint);
}


int snprintf(char *s, size_t n, const char *format, /*args*/ ...)
{
  int iprint;
  va_list ap;
  va_start(ap, format);
  iprint = vsnprintf(s, n, format, ap);
  va_end(ap);
  return(iprint);
}
#endif /*USE_SNPRINTF*/


/******************************************************************************
 * envSet()/envUnset(): set and unset environment variables
 *
 * envSet() acts the same and returns the same value putenv(), plus it also
 * tries to recycle any env slots that were made available by envUnset().
 * envUnset returns 0 if the env var is unset, otherwise it returns 1.
 */

/* Define the following if you want to allow spaces to precede '=' in
   the var name, otherwise spaces trailing the var name are considered
   part of the name.  Some OSes (e.g. VxWorks5.3.1) treat putenv("X  =1")
   to be the same as putenv("X=1"), whereas other OSes (e.g. SunOS5.7) treat
   these as two separate vars named "X  " and "X". */
/*#define IGNORE_SPACES_FOLLOWING_VARNAME*/

extern char **environ;
Local char *cannon_fodder = "CaNnOn_FoDdEr=0";
Local char **recyclable_env_slot = NULL;

int envSet(char *var)
{
  if (strchr(var, '=') == NULL)
  {
    /* Malformed envvar: correct by using envSetNameVal() to append "="
       which recursively calls this routine with a well-formed envvar. */
    return(envSetNameVal(var, ""));
  }
  else
  {
    /* Well-formed envvar: */
    char **e;
    Boole found;
    int i, sts = 0;
    
    found = FALSE;
    for (e = environ, i = 0;  *e != NULL && !found;  i++, e++)
    {
      Fast char *cp = *e, *vp = var;
      while (*cp && *vp && *cp != '=' && *vp == *cp) cp++, vp++;
# ifdef IGNORE_SPACES_FOLLOWING_VARNAME
      while (isSpace(*cp)) cp++;
      while (isSpace(*vp)) vp++;
# endif
      found = (*vp == *cp && (*vp == '\0' || *vp == '='));
    }
    
    if (found)
    {
      /* Replace existing variable: */
      i--;
      environ[i] = var;
    }
    else if (recyclable_env_slot != NULL)
    {
      /* Recycle last remembered env slot: */
      *recyclable_env_slot = var;
      recyclable_env_slot = NULL;
    }
    else
    {
      /* Look for any recyclable env slots that might have been forgotten: */
      found = FALSE;
      for (e = environ, i = 0;  *e != NULL && !found;  i++, e++)
	found = (*e == cannon_fodder);
      
      if (found)
      {
	/* A recyclable env slot was discovered: */
	i--;
	environ[i] = var;
      }
      else
	/* No recyclable env slots found; create a new variable: */
	sts = putenv(var);
    }
    return(sts);
  }
}


int envSetNameVal(char *name, char *val)
{
  char *space;
  if (name == NULL) return(1);
  if (val == NULL) val = "";
  space = memAlloc(strlen(name) + strlen("=") + strlen(val) + 1);
  sprintf(space, "%s=%s", name, val);
  return(envSet(space));
}


int envUnset(char *var)
{
  char **e;
  Boole found;
  int i, sts = 1;

  /* Note: If var not found, or if var itself is NULL then it might not
     be considered a serious error as it will essentially be unset. */

  if (var != NULL)
  {
    found = FALSE;
    for (e = environ, i = 0;  *e != NULL && !found;  i++, e++)
    {
      Fast char *cp = *e, *vp = var;
      while (*cp && *vp && *cp != '=' && *vp == *cp) cp++, vp++;
#   ifdef IGNORE_SPACES_FOLLOWING_VARNAME
      while (isSpace(*cp)) cp++;
      while (isSpace(*cp)) vp++;
#   endif
      found = (*vp == '\0' && *cp == '=');
    }
    
    if (found)
    {
      /* Unset an existing variable by marking it as fodder (it might be
	 easier to just set it to '\0', but some OSes might get disturbed
	 by this): */
      i--;
      environ[i] = cannon_fodder;
      recyclable_env_slot = &environ[i];
      sts = 0;
    }
  }    
  return(sts);
}


int envGetValue(char *var)
{
  int val = 0;
  char *str = getenv(var);

  if (str != NULL)
  {
    char *leftovers;
    val = strtol(str, &leftovers, 0);
    if (*leftovers != NULL) val = 0;
  }

  return(val);
}


Boole envGetBoole(char *var)
{
  Boole val = FALSE;
  char *str = getenv(var);

  if (str != NULL)
  {
    if (strcmp_ci(str, "0")     != 0 &&
	strcmp_ci(str, "FALSE") != 0)
      val = TRUE;
  }

  return(val);
}


/* Find first occurrence of any of the 'look_for' characters in the given
   'str' and return a pointer to whatever occurrence was found (if any): */
char *strFindFirstOccurrence(char *str, char *look_for)
{
  while (*str != '\0' && strchr(look_for, *str) == NULL) str++;
  return(*str != '\0' ? str : NULL);
}


void carrot_msg(char *tag, char *opt, char *arg, char *msg, char *ptr)
{
  char preface[128];
  sprintf(preface, "%s: %s%s%s",
	  Progname,
	  tag ? tag : "",
	  tag ? ": " : "",
	  opt ? opt : "");
  if (arg)
  {
    int carrot, indent;
    char *qarg, *qptr = ptr;
    if (ptr == NULL || ptr < arg) ptr = arg; /*JIC*/

    if ((qarg = quote_str(arg, &qptr, QS_ARG)) != NULL)
    {
      fprintf(stderr, "%s%s\n", preface, qarg);
      carrot = strlen(preface) + ((Ulong)qptr - (Ulong)qarg);
    }
    else
    {
      fprintf(stderr, "%s%s\n", preface, arg);
      carrot = strlen(preface) + ((Ulong)ptr - (Ulong)arg);
    }

    indent = carrot - strlen(msg);
    if (indent > 0 && indent < 256)
      fprintf(stderr, "%*c%s^\n", indent, ' ', msg);
    else if (carrot > 0 && carrot < 256)
      fprintf(stderr, "%*c^%s\n", carrot, ' ', msg);
    else
      fprintf(stderr, "%s\n", msg);
  }
  else
    fprintf(stderr, "%s: %s\n", preface, msg);

  return;
}


#define FNAME_TROUBLE(c) \
((!isGraph(c) && (c) != ' ') || (dash_Q && IS_MEMBER(c, "\\\"")))
/* FNAME_TROUBLE determines set of troublesome characters that are to be
   printed in octal and/or c-style, e.g.:
   dash_b: BAD<tab>FILE --> BAD\011FILE	
   dash_Q: B\A\D<tab>FILE --> "B\\A\\D\tFILE"
   dash_Q: A B\A\D<tab>"FILE" --> "A B\\A\\D\t\"FILE\""
   */

char *quote_fname(char *fname, Boole dash_b, Boole dash_Q, Boole plus_q)
{
  if (dash_b || dash_Q)
  {
    char *cp = fname;
    int trouble = 0;

    while (*cp != CNULL)
    {
      if (FNAME_TROUBLE(*cp)) trouble++;
      cp++;
    }
    if (dash_Q || trouble > 0)
    {
      static char *last_qstr = NULL;
      static int last_qstr_len = 0;
      int qstr_len;
      char *qstr;
      
      qstr_len = strlen(fname) + 2 + 3*trouble + 1;
      if (last_qstr != NULL && last_qstr_len >= qstr_len)
      {
	qstr_len = last_qstr_len;
	qstr = last_qstr;
      }
      else
      {
	if (last_qstr != NULL) memFree(last_qstr);
	qstr = memAlloc(qstr_len);
	/*printf("qstr_len, qstr: %d, %x\n", qstr_len, (Ulong)qstr);*/
      }
      
      last_qstr_len = qstr_len;
      last_qstr = qstr;
      
      cp = fname;
      if (dash_Q) *qstr++ = '"';
      while (trouble > 0 && *cp != CNULL)
      {
	char c_char;
	if (FNAME_TROUBLE(*cp))
	{
	  trouble--;
	  c_char = CNULL;
	  if (dash_Q)
	  {
	    if (*cp == '\\') c_char = '\\';
	    if (*cp == '"')  c_char = '"';
	    if (*cp == '\0') c_char = '0';	/* ^@ NUL */
	    if (*cp == '\b') c_char = 'b';	/* ^H BS  */
	    if (*cp == '\f') c_char = 'f';	/* ^L FF  */
	    if (*cp == '\n') c_char = 'n';	/* ^J NL  */
	    if (*cp == '\r') c_char = 'r';	/* ^M CR  */
	    if (*cp == '\t') c_char = 't';	/* ^I TAB */
#ifdef SKIP_OBSCURE
	    if (*cp == '\a') c_char = 'a';	/* ^G BEL */
	    if (*cp == '\e') c_char = 'e';	/* ^[ ESC */
	    if (*cp == '\v') c_char = 'v';	/* ^K VT  */
#endif
	  }
	  if (c_char != CNULL)
	  {
	    sprintf(qstr, "\\%c", c_char);
	    qstr += 2;
	  }
	  else
	  {
	    /* Don't allow SIGN EXTENSION to occur as this will generate 12
	       bytes of %o output instead of 3 bytes and overflow qstr! */
	    sprintf(qstr, "\\%03o", *(Uchar *)cp);
	    /* sprintf(qstr, "\\x%02x", *(Uchar *)cp);
	       -- uses \xff instead of \0377 */
	    qstr += 4;
	  }
	}
	else
	{
	  *qstr++ = *cp;
	}
	cp++;
      }
      sprintf(qstr, "%s%s", cp, dash_Q ? "\"" : "");

      if (plus_q)
      {
	char *q = quote_str(last_qstr, NULL, QS_FILE);
	if (q != NULL) return(q);
      }
      return(last_qstr);
    }
    else
      return(NULL);
  }

  else if (plus_q)
    return(quote_str(fname, NULL, QS_FILE));

  else
    return(NULL);
}


#define STR_TROUBLE(c) IS_MEMBER(c, "$`\\\"'!")
/* STR_TROUBLE determines set of troublesome characters that can't be
   contained inside single quotes, e.g.:
   BAD`FILE'NAME    -->  BAD\`FILE\'NAME
   A BAD FILE NAME  -->  'A BAD FILE NAME'
   A BAD`FILE'NAME  -->  'A BAD'\`'FILE'\''NAME'
   !pwd    -->  \!pwd
   \!*pwd  -->  '\'\!'*pwd'
   a!=b    -->  a!=b
   a ! b   -->  'a ! b'

   Note: some strings echo differently under various shells.
   sh and bash:  echo '\!pwd' --> \!pwd
   csh and tcsh: echo '\!pwd' --> !pwd
   csh and tcsh: echo a!=b    --> a!=b	 (does not require \ or ' quotes!)
   csh and tcsh: echo a ! b   --> a ! b	 (does not require \ or ' quotes!)
   sh and bash: '#' isn't a problem unless it's the first character
   csh and tcsh: '#' needs quoting in all contexts in script files only

   SETENV_NOTE: In csh and tcsh, users will sometimes accidentally declare
   environement variables as: "setenv ENVVAR=VALUE" instead of the correct
   "setenv ENVVAR VALUE".  In these accidental cases the value returned by
   getenv("ENVVAR") is "VALUE=" [sic], since putenv() adds a gratuitous
   "=" between the variable name and the value (in this case ENVVAR=VALUE
   is passed to putenv() as the variable name, and NULL is passed as the
   value!).  Thus, any embedded "=" in the VALUE field needs to be quoted
   so as to draw attention to this particular aberration.  getenv() doesn't
   help matters any, as getenv() considers the first "=" to delimit the
   variable name from the value.

   */
char *quote_str(char *str, char **ptr, Ulong mode)
{
  char *cp = str;
  char *sptr = (ptr != NULL ? *ptr : NULL);
  Boole pre;
  Boole quote;
  int trouble = 0;

  /* Return special case of a string of zero length: */
  if (str[0] == CNULL)
    return("\"\"");

  /* If file, the following characters need prefacing: */
  pre = ((mode & QS_FILE) && IS_MEMBER(*cp, "-+"));

  /* The following leading characters need quoting: */
  quote = IS_MEMBER(*cp, "~#");

  /* Look for troublesome characters.  Also, if any characters from the
     special set are detected then the entire string needs quoting: */
  while (quote == FALSE && *cp != CNULL)
  {
    quote = !isGraph(*cp) ||
      IS_MEMBER(*cp, "{}[]()<>&*|?;#^$\\\"'") ||
	((mode & QS_ENV) && *cp == '='); /* see SETENV_NOTE above */
    if (STR_TROUBLE(*cp)) trouble++;
    cp++;
  }

  /* Finish looking for troublesome characters: */
  while (*cp != CNULL)
  {
    if (STR_TROUBLE(*cp)) trouble++;
    cp++;
  }

  if (pre || quote || trouble > 0)
  {
    static char *last_qstr = NULL;
    static int last_qstr_len = 0;
    int qstr_len;
    char *qstr;
    char *qptr = NULL;
    char qchar, *qnext;

    /* Worst case: qstr_len = strlen(str) + strlen("./") +
       sizeof(start_quote) + sizeof(final_quote) + 
       sizeof(trouble_overhead) + sizeof(CNULL); */
    qstr_len = strlen(str) + 4 + 3*trouble + 1;
    if (last_qstr != NULL && last_qstr_len >= qstr_len)
    {
      qstr_len = last_qstr_len;
      qstr = last_qstr;
    }
    else
    {
      if (last_qstr != NULL) memFree(last_qstr);
      qstr = memAlloc(qstr_len);
      /*printf("qstr_len, qstr: %d, %x\n", qstr_len, (Ulong)qstr);*/
    }
    
    last_qstr_len = qstr_len;
    last_qstr = qstr;

    cp = str;

    qchar = '\'';
    if ((qnext = strFindFirstOccurrence(cp, "$`\\\"'")) != NULL)
      if (*qnext == '\'') qchar = '"';

    if (quote) *qstr++ = qchar;
    if (pre) {*qstr++ = '.'; *qstr++ = '/';}
    while (trouble > 0 && *cp != CNULL)
    {
      if (STR_TROUBLE(*cp))
      {
	trouble--;
	if ((quote && ((qchar == '\'' && IS_MEMBER(*cp, "$`\\\"")) ||
		       (qchar == '"'  && IS_MEMBER(*cp, "'"))
		       )) ||
	    (*cp == '!' && IS_MEMBER(*(cp+1), "= ")))  /* NO quotes needed! */
	{
	  /* No trouble after all: */
	  if (cp == sptr) qptr = qstr;
	  *qstr++ = *cp;
	}
	else
	{
	  if (quote) *qstr++ = qchar;

	  *qstr++ = '\\';
	  if (cp == sptr) qptr = qstr;
	  *qstr++ = *cp;

	  qchar = '\'';
	  if ((qnext = strFindFirstOccurrence(cp+1, "$`\\\"'")) != NULL)
	    if (*qnext == '\'') qchar = '"';

	  if (quote) *qstr++ = qchar;
	}
      }
      else
      {
	if (cp == sptr) qptr = qstr;
	*qstr++ = *cp;
      }
      cp++;
    }
    if (qptr == NULL) qptr = qstr + (sptr - cp);
    sprintf(qstr, "%s%c", cp, quote ? qchar : CNULL);

    if (ptr != NULL) *ptr = qptr;
    return(last_qstr);
  }
  else
    return(NULL);
}


#define MAX_MSG 2048

/* Display error message: */
void errorMsg(char *msg)
{
  char bigMsg[MAX_MSG];

  bigMsg[0] = CNULL;
  if (Progname != NULL)
    strncat(bigMsg, Progname, MAX_MSG-strlen(bigMsg)-1);
  if (Progname != NULL && msg != NULL)
    strncat(bigMsg, ": ", MAX_MSG-strlen(bigMsg)-1);
  if (msg != NULL)
    strncat(bigMsg, msg, MAX_MSG-strlen(bigMsg)-1);

  fprintf(stderr, "%s\n", msg);

  return;
}


/* Display error message and exit: */
void errorMsgExit(char *msg, int exit_sts)
{
  errorMsg(msg);
  exit(exit_sts);
}


/* Display errno message: */
void errnoMsg(int errnum, char *msg)
{
  if (!warning_suppress)
  {
    char bigMsg[MAX_MSG];
    int save_errno = errno;
    
    bigMsg[0] = CNULL;
    if (Progname != NULL)
      strncat(bigMsg, Progname, MAX_MSG-strlen(bigMsg)-1);
    if (Progname != NULL && msg != NULL)
      strncat(bigMsg, ": ", MAX_MSG-strlen(bigMsg)-1);
    if (msg != NULL)
      strncat(bigMsg, msg, MAX_MSG-strlen(bigMsg)-1);
    
    /* Temporarily substitute the global errno with given errnum (unless
       errnum is negative in which case use the saved errno): */
    if (errnum >= 0)
      errno = errnum;
    else
      errno = save_errno;
    
    if (errno != 0)
      perror(bigMsg);
    else
      fprintf(stderr, "%s: Unknown error\n", bigMsg);
    
    /* Restore global errno: */
    errno = save_errno;
  }
  return;
}


/* Display errno message and exit: */
void errnoMsgExit(int errnum, char *msg, int exit_sts)
{
  errnoMsg(errnum, msg);
  exit(exit_sts);
}


extern char *Current_Arg;
void opt_error_msg(const char *msg, char *ptr)
{
  fprintf(stderr, "\n");
  carrot_msg(NULL, NULL, Current_Arg, msg, ptr-1);
  give_usage_or_help('U');
  exit(1);
}


void arg_error_msg(char *msg, char *arg, char *ptr)
{
  carrot_msg(NULL, NULL, arg, msg, ptr-1);
  exit(1);
}


void usage_error(void)
{
  give_usage_or_help('U');
  exit(1);
}
