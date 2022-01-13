/******************************************************************************
  cksum.c -- checksum functions

  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: September 8, 2015
  
  These functions are provided "as is" in the hopes that they might serve some
  higher purpose.  If you want these functions to serve some lower purpose,
  then that too is all right.  So as to keep these hopes alive you may
  freely distribute these functions so long as this header remains intact.
  You may also freely distribute modified versions of these functions so long
  as you indicate that such versions are modified and so long as you
  provide access to the unmodified original copy.

  NOTE: the POSIX and CRC32 algorithms contained here-in do NOT rely on
  pre-computed CRC tables (it took me way too long searching the 'Net to
  find versions NOT based on pre-computed CRC tables, and when I did find
  something close to what I wanted I still had to modify them so as to do
  run-time initialization!).  I do not like pre-computed CRC tables because:
  a long list of binary numbers isn't meaningful to a human, it takes
  very little time to initialize these tables at run-time, and generating
  them at run-time avoids portability issues between big/little-endian and
  32/64-bit machines.

  ACKNOWLEDGEMENTS:
  1) Mark Baranowski is the author of cksumBsd() and cksumSysv().  I
     hereby give myself (and everyone else) permission to use my functions.
  2) The POSIX checksum functions fill_r(), remainder(),
     and cksumPosix() were adapted from GNU's coreutils-5.0.tar.gz.
     Please see GNU copyright header below pertaining to these functions.
  3) THE ANSI/CRC32 checksum functions cksumCrc32Init() and cksumCrc32()
     were adapted from ZLIB's crc32.c funcion.  Please see ZLIB copyright
     header below pertaining to these functions.

  Note: The most recent version of these functions may be obtained from
        http://els-software.org

  ****************************************************************************/

#include "sysdefs.h"

#include <stdio.h>
#include <errno.h>
#if defined(HAVE_O_NOATIME) && defined(LINUX)
/* __USE_GNU allows O_NOATIME */
/* TBD: Does __USE_GNU introduce other gremlins? */
# define __USE_GNU
# include <fcntl.h>
# undef  __USE_GNU
#else
# include <fcntl.h>
#endif
#include <sys/types.h>
#include <unistd.h>

#include "defs.h"
#include "auxil.h"
#include "cksum.h"
#include "elsVars.h"

/* NB: sizeof(long) does not always == 4, e.g. on OSF1 sizeof(long) == 8 */
#if ELS_SIZEOF_st_size <= ELS_SIZEOF_long
#define MAX_ST_SIZE  MAXPOS_LONG
#else
#define MAX_ST_SIZE  MAXPOS_LLONG
#endif
#define MAX_READ     0x2000


/* The "read size" must carefully match the OS's limitations, because some
   OSes (such as SunOS4.x) have problems if attempting to read more data
   than the size of the file when the file's size is between 0x7fffe000 and
   0x7fffffff (even if using getc() this is still a problem because stdio
   reads in blocks of 0x2000 and since the final block exceeds the maximum
   allowed file size, errno = EINVAL gets set!).  The function read_size()
   prevents this because it limits the final block "read size" to 0x1fff. */

Local int read_size(ELS_st_size offset)
{
  /* NB: "ELS_st_size" is assumed to be unsigned */
  if (offset >= MAX_ST_SIZE)
    return(0);  /* Already past MAX_ST_SIZE */
  else if (offset + MAX_READ > MAX_ST_SIZE)
    return((int)(MAX_ST_SIZE - offset));  /* Don't exceed MAX_ST_SIZE */
  else
    return(MAX_READ);  /* OK to read a full buffer */
}


/* Convert cksumType to algorithm name: */
char *cksumTypeToName(cksumType type)
{
  char *name;
  if (type == CKSUM_BSD)
    name = "bsd";
  else if (type == CKSUM_SYSV)
    name = "sysv";
  else if (type == CKSUM_POSIX)
    name = "posix";
  else if (type == CKSUM_CRC32)
    name = "crc32";
  else
    name = "unknown";
  return(name);
}


/* Both /usr/ucb/sum and /usr/bin/sum display 0 if error occurs.
   crc32 displays 0 if error occurs just because.
   SunOS /usr/bin/cksum displays 4294967295 if cksum of directory, while
   Linux's /usr/bin/cksum prints error message and no value in such cases.
   Prior to VersionLevel 152 els displayed 4294967295 (unless +z in effect),
   but since zero byte files also return 4294967295 it makes better sense
   to display 0 like everyone else. */

Uint32 cksumErrorCode(cksumType type)
{
  extern int VersionLevel;  /* From els.h */
  if (type == CKSUM_POSIX && VersionLevel <= 151)
    /* Beware of x64_64 Linux where sizeof(Ulong) == 8: */
    return (Uint32)0xffffffff;
  else
    return 0;
}


/* Perform checksum of file using requested algorithm: */
Uint32 cksumFile(char *filename, cksumType type, ELS_st_size *nread)
{
  int fd;
  Uint32 checksum = 0;
  if (nread != NULL) *nread = 0;

#if defined(HAVE_O_NOATIME)
  /* TBD: Does O_NOATIME perturb non-Linux FSes? */
  {
    int save_errno = errno;
    static Boole give_warning = TRUE;
    fd = open(filename, O_RDONLY|O_NOATIME);
    if (fd < 0)
    {
      /* Discard errno from previous attempt and re-try without O_NOATIME: */
      errno = save_errno;
      fd = open(filename, O_RDONLY);
      if (fd >= 0 && verboseLevel >= 1 && give_warning)
      {
	fprintf(stderr, "\
%s: FYI: Unable to preserve access time: %s\n\
", Progname, filename);
	if (verboseLevel == 1)
	{
	  fprintf(stderr, "\
%s: Subsequent access time warnings suppressed\n\
", Progname);
	  give_warning = FALSE;
	}
      }
    }
  }
#else
  fd = open(filename, O_RDONLY);
#endif
  if (fd >= 0)
  {
    if (type == CKSUM_BSD)
      checksum = cksumBsd(fd, nread);    /* Same algorithm as /usr/ucb/sum */
    else if (type == CKSUM_SYSV)
      checksum = cksumSysv(fd, nread);   /* Same algorithm as /usr/bin/sum */
    else if (type == CKSUM_POSIX)
      checksum = cksumPosix(fd, nread);  /* Same algorithm as /usr/bin/cksum */
    else if (type == CKSUM_CRC32)
      checksum = cksumCrc32(fd, nread);  /* CRC32 algorithm */
    close(fd);
  }
  
  return(checksum);
}


/* PERFORM 16-BIT BSD CHECKSUM (same algorithm as /usr/bin/sum under SunOS4,
   /usr/ucb/sum under SunOS5/Solaris, and '/usr/bin/sum -r' under Linux): */
Uint32 cksumBsd(int fd, ELS_st_size *nread)
{
  register Uint32 checksum = 0;
  Uchar buf[MAX_READ];
  ELS_st_size offset = 0;
  int n;
  int save_errno = errno;
  errno = 0;

  while ((n = read(fd, buf, read_size(offset))) > 0)
  {
    register Uchar *cp = buf;
    offset += n;
    while (n-- > 0)
    {
      Boole rotate = (checksum & 1) != 0;
      checksum >>= 1;
      if (rotate) checksum |= 0x8000;
      checksum += *cp++;
      checksum &= 0xffff;
    }
  }

  if (errno != 0)
    checksum = cksumErrorCode(CKSUM_BSD);  /* Keep current errno */
  else
    errno = save_errno;  /* Restore previous errno */

  if (nread != NULL) *nread = offset;
  return(checksum);
}


/* PERFORM 16-BIT SYSV CHECKSUM (same algorithm as /usr/5bin/sum under SunOS4,
   /usr/bin/sum under SunOS5/Solaris, and '/usr/bin/sum -s' under Linux): */
Uint32 cksumSysv(int fd, ELS_st_size *nread)
{
  register Uint32 checksum = 0;
  Uchar buf[MAX_READ];
  ELS_st_size offset = 0;
  int n;
  int save_errno = errno;
  errno = 0;

  while ((n = read(fd, buf, read_size(offset))) > 0)
  {
    register Uchar *cp = buf;
    offset += n;
    while (n-- > 0)
    {
      checksum += *cp++;
    }
  }

  if (sizeof(checksum) > 4) checksum &= 0xffffffff;
  checksum = (checksum & 0xffff) + (checksum >> 16);
  checksum = (checksum & 0xffff) + (checksum >> 16);

  if (errno != 0)
    checksum = cksumErrorCode(CKSUM_SYSV);  /* Keep current errno */
  else
    errno = save_errno;  /* Restore previous errno */

  if (nread != NULL) *nread = offset;
  return(checksum);
}


/*****************************************************************************/
/* PERFORM 32-BIT POSIX CHECKSUM (same algorithm as /usr/bin/cksum under
   both SunOS5/Solaris and Linux.  See also Posix1003.2/D11.2): */

/* cksum -- calculate and print POSIX checksums and sizes of files
   Copyright (C) 92, 1995-2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Written by Q. Frank Xia, qx@math.columbia.edu.
   Cosmetic changes and reorganization by David MacKenzie, djm@gnu.ai.mit.edu.
   
   As Bruce Evans pointed out to me, the crctab in the sample C code
   in 4.9.10 Rationale of P1003.2/D11.2 is represented in reversed order.
   Namely, 0x01 is represented as 0x80, 0x02 is represented as 0x40, etc.
   The generating polynomial is crctab[0x80]=0xedb88320 instead of
   crctab[1]=0x04C11DB7.  But the code works only for a non-reverse order
   crctab.  Therefore, the sample implementation is wrong.
   
   This software is compatible with neither the System V nor the BSD
   `sum' program.  It is supposed to conform to P1003.2/D11.2,
   except foreign language interface (4.9.5.3 of P1003.2/D11.2) support.
   Any inconsistency with the standard except 4.9.5.3 is a bug.  */

/* Adapted to ELS by Mark Baranowski, requestXXX@els-software.org (remove XXX)
   THIS VERSION HAS BEEN MODIFIED FROM THE ORIGINAL.  The original cksum.c
   can be found in coreutils-5.0.tar.gz (available from http://www.gnu.org).
   */

# define BIT(x)	((Uint32) 1 << (x))
# define SBIT	BIT(31)

/* The generating polynomial is:

         32  26  23  22  16  12  11  10  8   7   5   4   2   1   0
   G(X)=X + X + X + X + X + X + X + X + X + X + X + X + X + X + X

   The i bit in POSIX_GEN is set if X^i is a summand of G(X) except X^32. */

#define POSIX_GEN (BIT(26) | BIT(23) | BIT(22) | BIT(16) | BIT(12) | \
		   BIT(11) | BIT(10) | BIT(8) | BIT(7) | BIT(5) | \
		   BIT(4) | BIT(2) | BIT(1) | BIT(0))

Local Uint32 *cksumPosix_crctab = NULL;
Local Uint32 r[8];


Local void cksumPosix_fill_r(Uint32 GEN)
{
  int i;

  r[0] = GEN;
  for (i = 1; i < 8; i++)
    r[i] = (r[i - 1] << 1) ^ ((r[i - 1] & SBIT) ? GEN : 0);
  return;
}


Local Uint32 cksumPosix_remainder(int m)
{
  Uint32 rem = 0;
  int i;

  for (i = 0; i < 8; i++)
    if (BIT(i) & m)
      rem ^= r[i];

  return(rem & 0xFFFFFFFF);	/* Make it run on 64-bit machine.  */
}


Local void cksumPosixInit(Uint32 GEN)
{
  if (cksumPosix_crctab == NULL)
  {
    int i;
    cksumPosix_crctab = memAlloc(256 * sizeof(Uint32));

    cksumPosix_fill_r(GEN);
    cksumPosix_crctab[0] = 0;
    for (i = 1; i < 256; i++)
      cksumPosix_crctab[i] = cksumPosix_remainder(i);
  }
  return;
}


Uint32 cksumPosix(int fd, ELS_st_size *nread)
{
  register Uint32 crc = 0;
  Uchar buf[MAX_READ];
  ELS_st_size offset = 0;
  int n;
  int save_errno = errno;
  errno = 0;

  if (cksumPosix_crctab == NULL)
    cksumPosixInit(POSIX_GEN);

  while ((n = read(fd, buf, read_size(offset))) > 0)
  {
    register Uchar *cp = buf;
    offset += n;
    while (n-- > 0)
    {
      crc = (crc << 8) ^ cksumPosix_crctab[((crc >> 24) ^ *cp++) & 0xFF];
    }
  }

  if (nread != NULL) *nread = offset;
  
  for (; offset; offset >>= 8)
    crc = (crc << 8) ^ cksumPosix_crctab[((crc >> 24) ^ offset) & 0xFF];

  crc = ~crc & 0xFFFFFFFF;

  if (errno != 0)
    crc = cksumErrorCode(CKSUM_POSIX);  /* Keep current errno */
  else
    errno = save_errno;  /* Restore previous errno */

  return(crc);
}


/*****************************************************************************/
/* PERFORM 32-BIT CRC32 CHECKSUM (same algorithm as used by ZLIB, et al.
   See also ANSI X3.66): */

/* crc32.c -- compute the CRC-32 of a data stream
 * Copyright (C) 1995-2002 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

/* zlib.h -- interface of the 'zlib' general purpose compression library
  version 1.1.4, March 11th, 2002

  Copyright (C) 1995-2002 Jean-loup Gailly and Mark Adler

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Jean-loup Gailly        Mark Adler
  jloup@gzip.org          madler@alumni.caltech.edu
*/

/*
   Generate a table for a byte-wise 32-bit CRC calculation on the polynomial:
   x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1.
   
   Polynomials over GF(2) are represented in binary, one bit per coefficient,
   with the lowest powers in the most significant bit.  Then adding polynomials
   is just exclusive-or, and multiplying a polynomial by x is a right shift by
   one.  If we call the above polynomial p, and represent a byte as the
   polynomial q, also with the lowest power in the most significant bit (so the
   byte 0xb1 is the polynomial x^7+x^3+x+1), then the CRC is (q*x^32) mod p,
   where a mod b means the remainder after dividing a by b.
   
   This calculation is done using the shift-register method of multiplying and
   taking the remainder.  The register is initialized to zero, and for each
   incoming bit, x^32 is added mod p to the register if the bit is a one (where
   x^32 mod p is p+x^32 = x^26+...+1), and the register is multiplied mod p by
   x (which is shifting right by one and adding x^32 mod p if the bit shifted
   out is a one).  We start with the highest power (least significant bit) of
   q and repeat for all eight bits of q.
   
   The table is simply the CRC of all possible eight bit values.  This is all
   the information needed to generate CRC's on data a byte at a time for all
   combinations of CRC register values and incoming bytes.
   */

/* Adapted to ELS by Mark Baranowski, requestXXX@els-software.org (remove XXX)
   THIS VERSION HAS BEEN MODIFIED FROM THE ORIGINAL.  The original crc32.c
   can be obtained from http://www.zlib.org.
   */

Local Uint32 *cksumCrc32_crctab = NULL;


Local void cksumCrc32Init(void)
{
  Uint32 c;
  int n, k;
  Uint32 poly;            /* polynomial exclusive-or pattern */
  /* terms of polynomial defining this crc (except x^32): */
  static const char p[] = {0,1,2,4,5,7,8,10,11,12,16,22,23,26};
  
  if (cksumCrc32_crctab == NULL)
  {
    cksumCrc32_crctab = memAlloc(256 * sizeof(Uint32));
    
    /* make exclusive-or pattern from polynomial (0xedb88320L) */
    poly = 0L;
    for (n = 0; n < sizeof(p); n++)
      poly |= 1L << (31 - p[n]);
    
    for (n = 0; n < 256; n++)
    {
      c = (Uint32)n;
      for (k = 0; k < 8; k++)
	c = c & 1 ? poly ^ (c >> 1) : c >> 1;
      cksumCrc32_crctab[n] = c;
    }
  }
  return;
}


Uint32 cksumCrc32(int fd, ELS_st_size *nread)
{
  register Uint32 crc = 0;
  Uchar buf[MAX_READ];
  ELS_st_size offset = 0;
  int n;
  int save_errno = errno;
  errno = 0;

  if (cksumCrc32_crctab == NULL)
    cksumCrc32Init();

  crc = crc ^ 0xffffffffL;

  while ((n = read(fd, buf, read_size(offset))) > 0)
  {
    register Uchar *cp = buf;
    offset += n;
    while (n-- > 0)
    {
      crc = cksumCrc32_crctab[((int)crc ^ *cp++) & 0xff] ^ (crc >> 8);
    }
  }

  if (errno != 0)
    crc = cksumErrorCode(CKSUM_CRC32);  /* Keep current errno */
  else
    errno = save_errno;  /* Restore previous errno */

  if (nread != NULL) *nread = offset;
  return(crc ^ 0xffffffffL);
}
