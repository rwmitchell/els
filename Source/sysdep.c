/******************************************************************************
  sysdep.c -- System dependencies
  
  Author: Mark Baranowski
  Email:  requestXXX@els-software.org (remove XXX)
  Download: http://els-software.org

  Last significant change: March 14, 2001
  
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
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <utime.h>
#include <dirent.h>
#include <sys/param.h>
#include <unistd.h>
#include <sys/utsname.h>

#if defined(HAVE_ACL)
#  if defined(LINUX)
/*   Tested on Linux 2.6.9: */
#    include <sys/acl.h>
#    include <acl/libacl.h> /* prototypes: acl_entries() */
#  elif defined(DARWIN)
#    include <sys/acl.h>
#    include <sys/types.h>
#  elif defined(AIX)
#    include <acl.h>
#    include <sys/acl.h>
#  else  /* SUNOS, HPUX, OSF1, IRIX: */
#    include <sys/acl.h>
#  endif
#endif

#include "defs.h"
#include "auxil.h"
#include "els.h"
#include "elsMisc.h"
#include "sysInfo.h"
#include "sysdep.h"


char *get_userAtHost(void)
{
  static char *userAtHost = NULL;
  if (userAtHost == NULL)
  {
    char *user = uid2name(Whoami);
    struct utsname host;
    uname(&host);
    userAtHost = memAlloc(strlen(user) + strlen(host.nodename) + 4);
    sprintf(userAtHost, "%s@%s", user, host.nodename);
  }
  return(userAtHost);
}


int get_acl_count(Dir_Item *file, char *path)
{
  int aclcnt = 0;

  /* Special consideration for symbolic links:
     For now treat all symbolic links as having a 0 ACL count based upon
     the following information:
     1) SunOS5.7's /bin/ls treats the ACL count of all symbolic links
     as 0 unless -L specified.
     2) OSF1's acl_get_file() generates a Segmentation fault if acl_get_file()
     called on a symbolic link that points to a non-existent file. */
  if (file->islink && !expand_symlink) return(0);

#ifdef HAVE_ACL

# if defined(SUNOS)
  /* Determine at runtime whether the version of SunOS being run supports
     ACLs.  This is necessary so that if the executable is compiled under
     SunOS5.5 using shared objects, then the *same* executable can be used
     on other machines running SunOS5.4.  This feat is accomplished by
     avoiding calls to acl() whenever necessary: */
  if (osVersion >= 50500)
  {
    /* NB: acl returns -1 if error */
    aclcnt = acl(path, GETACLCNT, 0, NULL);
    if (aclcnt > MIN_ACL_ENTRIES)
      aclcnt -= MIN_ACL_ENTRIES;
    else
      aclcnt = 0;
  }

# elif defined(LINUX)
  /* Tested on Linux 2.6.9: */
  {
#   define MIN_ACL_ENTRIES 3   /* Pre-defined constant missing */
    acl_t acl = acl_get_file(path, ACL_TYPE_ACCESS);
    if (acl != NULL)
    {
      aclcnt = acl_entries(acl);
      if (aclcnt > MIN_ACL_ENTRIES)
	aclcnt -= MIN_ACL_ENTRIES;
      else
	aclcnt = 0;
      acl_free(acl);
    }
    else
      aclcnt = 0;
  }

# elif defined(DARWIN)
  {
    acl_t acl = acl_get_link_np(path, ACL_TYPE_EXTENDED);
    if (acl != NULL)
    {
      aclcnt = 1;	/* There may be more than 1 ACL ??? */
      acl_free(acl);
    }
    else
      aclcnt = 0;
  }

# elif defined(HPUX)
  aclcnt = getacl(path, 0, NULL);
  if (aclcnt > NBASEENTRIES)
    aclcnt -= NBASEENTRIES;
  else
    aclcnt = 0;

# elif defined(OSF1)
  {
    acl_t acl = acl_get_file(path, ACL_TYPE_ACCESS);
    aclcnt = acl->acl_num;
    if (aclcnt > ACL_MIN_ENTRIES)
      aclcnt -= ACL_MIN_ENTRIES;
    else
      aclcnt = 0;
  }

# elif defined(AIX)
  /* ??? this needs to be retested ??? */
  {
    struct acl pAcl;
    statacl(path, STX_NORMAL, &pAcl, sizeof(pAcl));
    if (pAcl.acl_mode & S_IXACL)
      aclcnt = 1;	/* There may be more than 1 ACL ??? */
  }

# elif defined(IRIX)
  /* ??? this part is missing ??? */
  {
    /* Need code for setting the number of ACLs (i.e. aclcnt) here. */
  }

# else
  fprintf(stderr, "\
get_acl_count: '#ifdef HAVE_ACL' section needs completion for this target\n");

# endif
#endif /*HAVE_ACL*/

  return(aclcnt);
}

/*===========================================================================*/

ELS_st_blocks getStatBlocks(struct stat *info)
{
#ifdef NO_ST_BLOCKS
  /* Crude approximation to what the st_blocks should be under SVR3.x: */
  return((info->st_size + 511)/512);
#else
  return(info->st_blocks);
#endif
}

/*===========================================================================*/

#ifdef NO_ST_DEV
struct stat *set_mp(struct stat *dir)
{
  return;
}

int diff_mp(struct stat *dir)
{
  return(0);
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
#else

Local struct stat *mp;
struct stat *set_mp(struct stat *dir)
{
  struct stat *old_mp;
  /*printf("SET: %x\n", dir->st_dev);*/
  old_mp = mp;
  mp = dir;
  return(old_mp);
}

int diff_mp(struct stat *dir)
{
  int diff;
  /*printf("DIFF: %x %x\n", mp->st_dev, dir->st_dev);*/
  diff = memcmp(&mp->st_dev, &dir->st_dev, sizeof(dev_t));
  return(diff);
}

#endif /*NO_ST_DEV*/
/*===========================================================================*/
