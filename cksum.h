/******************************************************************************
  cksum.h -- els checksum functions and defines
  
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
  
  Note: The most recent version of these functions may be obtained from
        http://els-software.org

  ****************************************************************************/

#ifndef ELS__CKSUM_H
#define ELS__CKSUM_H

typedef enum cksumType {
  CKSUM_UNSPECIFIED = 0,
  CKSUM_BSD,
  CKSUM_SYSV,
  CKSUM_POSIX,
  CKSUM_CRC32} cksumType;

extern char *cksumTypeToName(cksumType type);
extern Uint32 cksumErrorCode(cksumType type);
extern Uint32 cksumFile(char *filename, cksumType type, ELS_st_size *nread);
extern Uint32 cksumBsd(int fd, ELS_st_size *nread);
extern Uint32 cksumSysv(int fd, ELS_st_size *nread);
extern Uint32 cksumPosix(int fd, ELS_st_size *nread);
extern Uint32 cksumCrc32(int fd, ELS_st_size *nread);

#endif /*ELS__CKSUM_H*/
