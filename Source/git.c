#ident "$Id$"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>     // getcwd()
#include <sys/stat.h>   // struct stat
#include "els.h"
#include "auxil.h"

#define USE_HGC
#ifdef  USE_HGC

#include "hg.h"

#else

// 2020-12-13 copying routines from mylib to keep this autonomous
bool  RMisdir     ( const char *name ) {
  struct stat sbuf;
  bool        isdir = false;

  if (stat(name,&sbuf) == 0)
    if ( S_ISDIR( sbuf.st_mode ) ) isdir = true;

  return( isdir );
}
char *loadpipe( const char *cmd, off_t *f_sz ) {
  ssize_t rc = 0;
  off_t   sz = 0;
  FILE *pipe = NULL;
  static
  char *data = NULL;
  static off_t of_sz = 0;
  char  buf[1024];

  if ( data ) { memset( data,'\0', of_sz ); }

  *f_sz = of_sz;
  if ( (pipe=popen( cmd, "r" )) > 0 ) {
//  printf("cmd: %s : %ld\n", cmd, *f_sz );
    while (  (rc = fread( buf, 1, 1023, pipe )) > 0 ) {
//    printf( "%8ld bytes read\n", rc );
//    printf( "====\n%s\n===\n", buf );
      if ( sz + rc > *f_sz ) {
#ifdef OLDMETHOD // __linux__
        *f_sz += 2048;
 //     printf( "loadpipe: realloc( %p, %ld ): %ld %ld\n", data, *f_sz, sz, rc );
        char *newdata = memAllocZero( *f_sz );
        memcpy( newdata, data, (*f_sz-2048) );
        memFree( (void *) data );
        data = newdata;
//      data = realloc ( data, *f_sz );
        if ( !data ) { printf( "realloc(%ld) failed on %s\n", *f_sz, cmd); exit(-1); }
#else
        // if memRealloc() fails, it exits
        data = (char *) memRealloc ( (void *) data, *f_sz, 2048 );
        *f_sz += 2048;
//      if ( !data ) { printf( "realloc(%lld) failed on %s\n", *f_sz, cmd); exit(-1); }
#endif
      }
      strcat( data, buf );
      memset( buf, '\0', 1024 );
      sz += rc;
    }
    pclose( pipe );
  } else {
    fprintf(stderr, "Unable to open %s\n", cmd );
  }

  of_sz = *f_sz;
  return( data );
}
char *fullpath( char *path ) {
  static
  char fpath[ MAX_DNAME ];
  char *p = NULL;

  if ( *path == '/' ) return ( path  );
  p = getcwd( NULL, 0 );
  if ( *path == '.' ) {
    sprintf( fpath, "%s", p );
    free( p );
    return( fpath );
  }

  sprintf( fpath, "%s/%s", p, path );
  free( p );
  return( fpath  );
}

#endif

char gitsub[1024] = { '\0' };  // current subdir in a git path
int  gitlen = 0;

char *is_git( char *dir, bool sub ) {
  static
  char  cwd[ MAX_DNAME ];
  char *pd,
       *gt="/.git",
       *po;

  int   cwdlen = 0,
        dirlen = 0;
  bool  is_git = false;

  if ( dir && RMisdir( dir ) ) pd = strcpy( cwd, dir );
  else return NULL;

  while ( !is_git && pd && (  strlen(pd) > 1 ) ) {
    po = strrchr( pd, '/' );
    if ( po ) {
      strcat( pd, gt );
      if ( ! ( is_git = RMisdir( pd ) ) ) {
        *po = '\0';                    // truncate last directory
      } else {
        po = strrchr( pd, '/' );
        *po = '\0';                    // truncate last directory
      }
    } else *pd = '\0';
  }
  if ( is_git && sub ) cwdlen = strlen( cwd ) + 1;

//fprintf( stderr, "is_git: >%s< :%d: >%s<\n", dir, cwdlen, cwd );

  memset( gitsub, '\0', 1024 );        // just to be sure, probably not necessary

  dirlen = strlen( dir );
  if ( dirlen - cwdlen > 0 ) {
    strncpy( gitsub, (dir+cwdlen), dirlen-cwdlen );
    gitlen = strlen( gitsub );

    // Need the last char to be a /
    if ( gitsub[ gitlen-1 ] != '/' ) gitsub[ gitlen ] = '/';
  }

//fprintf( stderr, "sub : %s : len: %d -> %d\n", gitsub, dirlen, dirlen-cwdlen );

//fprintf( stderr, "Dir : %s : sub: %d\n",  dir, sub );
//fprintf( stderr, "Root: %s : len: %d\n", pd,  cwdlen );

  return( is_git ? pd + cwdlen : NULL );
}
char *load_gitstatus( const char *dir, char *git ) {
  off_t sz;
  static
  char *lst = NULL,
       *hld = NULL;
  char *cmd = NULL;        // 2020-12-20 issue is not cmd, fails without malloc

//fprintf( stderr, "Load: %s -> %s\n", dir, git );

  if ( ! git ) return( hld );  // return quickly if not set

  if ( !lst || strcmp( dir, lst ) ) {
    if( lst ) free( lst );
    lst = strdup( dir );
    cmd = memAllocZero( strlen( dir ) + strlen( git ) + 8 );
//  sprintf( cmd, "%s %s &>/dev/null", git, dir ); // 2025-03-20: everything  goes into /dev/null
    sprintf( cmd, "%s %s 2>/dev/null", git, dir ); // 2025-03-20: only stderr goes into /dev/null
//  fprintf( stderr, "CMD: %s\n", cmd );
//  fprintf( stderr, "DIR: %s\n", dir );
    hld=loadpipe( cmd, &sz );
    free   ( cmd );
    return ( hld );
  } else return( hld );
}
char  get_gitstatus( char *dir, char *file, char *gs ) {
  static char *pdir = NULL,
              *pt3  = NULL,
              ch    = ' ';

  static bool B_DM = false;    // directory match

  char *pt1 = NULL,
       *pt2 = NULL,
       *pgs = NULL, // pointer to git subdir
        buf[MAX_DNAME];

//if ( RMisdir( file ) ) return( ch );

  if ( !B_DM ) ch = ' ';

//nprintf( stderr, "getting status: >%s<  >%s<\n", pdir, dir );

  if ( pdir != dir ) {
    B_DM = false;
    ch   = ' ';
    pdir = dir;

#ifdef  MatchSubDir
    // An easier fix it change ELS_GIT_STATUS to be:
    // git status -s --ignored --porcelain --untracked-files
    //
    // First check if the subdir we are in is ignored
    // ( subdir could also be untracked )
    // If it is, set ch to 'I' and don't check again

    pt1 = pt2 = gs;
    pt3 = is_git( fullpath( dir ), true );
    while ( pt3 && *pt3 && ch == ' ' ) {

      if ( strstr( pt1, pt3 ) ) {

        if ( strlen( pt1 ) == strlen( pt3 ) + 4 ) ch = 'I';
        else {

          if ( pt2 && *pt2 == '/' ) *pt2 = '\0';
          pt2=strrchr( pt3, '/' );
          if (  pt2 ) *(pt2+1) = '\0';
          else pt3 = NULL;
        }
      } else {
        if ( *pt2 == '/' ) *pt2 = '\0';
        pt2=strrchr( pt3, '/' );
        if ( *pt2 ) *(pt2+1) = '\0';
        else pt3 = NULL;
      }
    }
    if ( ch == 'I' ) B_DM = true;
#endif

    pt3 = is_git( fullpath( dir ), true );

  }

  pt1 = pt2 = gs;

  if ( ! strcmp( dir, "." ) || *dir == '\0' ) {
    if ( strlen( pt3 ) ) {

      sprintf( buf, "%s/%s", pt3, file );
      pgs = buf;
    } else pgs = file;
  } else {
    sprintf( buf, "%s/%s", dir, file );
    pgs = buf;
  }

//fprintf( stderr, "LEN: %lu  :  %d\n", strlen( dir ), gitlen );

  // len of 1 allows for '.'
  if ( strlen(dir) <= 1 && gitlen > 0 ) {
    sprintf( buf, "%s%s", gitsub, file);
    if ( buf[strlen( buf ) ] == '.' ) buf[ strlen(buf) ] = '\0';
    pgs = buf;
//  fprintf( stderr, "BUF: %s  <- :%s:   :%s:\n", pgs, gitsub, file );
  }

//fprintf( stderr, "STAT: >%s< -> %s ->\n%s <--\n", dir, file, gs );

  bool done = ( B_DM && ch == 'I' ),
       mtch = false;
  int len1, len2;
  len1 = strlen( pgs );
  do {
    pt1 = pt2;
    pt2 = strstr( pt1, pgs );
//  fprintf( stderr, "PT1 : >%s<\n", pt1 );
//  fprintf( stderr, "PGS : >%s<\n", pgs );
//  fprintf( stderr, "PT2 : >%s<\n", pt2 );
    if ( pt2 ) {
      len2 = strchr( pt2, '\n' ) - pt2;

      // git status returns the full path relative to the git root
      // when in a git subdir, *(pt2-1) will be the '/' on a filename match
      // XYZZY - this needs to compare the full git relative path

      if ( RMisdir( file ) ) {
        if ( len1 < len2 && *(pt2 +len1)  == '/' ) mtch = true;
        else mtch = false;
      }
      else mtch = ( len1 == len2   );

      if ( mtch && *(pt2-1) == ' ' ) {
        done = true;
        pt2 -= 3;
        if ( *pt2 == ' ' ) pt2++; // first char can be a space or indicator
        // XYZZY - a file can be both Added and Modified ! Should indicate that
        switch( *pt2 ) {
          case 'M': ch = 'M'; break;   // modified
          case 'A': ch = 'A'; break;   // added                // XYZZY not verified
          case 'R': ch = 'R'; break;   // removed              // XYZZY not verified
          case 'D': ch = 'D'; break;   // deleted/missing      // XYZZY not verified
          case '?': ch = '?'; break;   // unknown/not tracked
          case '!': ch = 'I'; break;   // ignored
          case '-':           break;   // moved files report as: old -> new; ignore
          default : ch = 'E'; printf( ">%c: %s<\n", *pt2, pgs ); break;   // error
        }
      }
      else pt2++;
    } else done = true;

  } while ( !done );

  return( ch );
}
