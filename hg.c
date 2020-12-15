#ident "$Id"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>     // MAXNAMLEN
#include <unistd.h>     // getcwd()
#include <sys/stat.h>   // struct stat
#include "els.h"


// 2020-12-13 copying routines from mylib to keep this autonymous
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
  char  buf[1024];

  if ( data ) { free( data ); data = NULL; }

  *f_sz = 0;
  if ( (pipe=popen( cmd, "r" )) > 0 ) {
    while (  (rc = fread( buf, 1, 1023, pipe )) > 0 ) {
//    printf( "%8ld bytes read\n", rc );
//    printf( "====\n%s\n===\n", buf );
      if ( sz + rc >= *f_sz ) {
        *f_sz += 2048;
        data = realloc( data, *f_sz );
        if ( !data ) { printf( "realloc(%lld) failed on %s\n", *f_sz, cmd); exit(-1); }
      }
      strcat( data, buf );
      memset( buf, '\0', 1024 );
      sz += rc;
    }
    pclose( pipe );
  } else {
    fprintf(stderr, "Unable to open %s\n", cmd );
  }

  return( data );
}
char *fullpath( char *path ) {
  if ( *path == '/' ) return ( path  );
  if ( *path == '.' ) return ( getcwd( NULL, 0 ) );

  static
  char fpath[ MAX_DNAME ];

  sprintf( fpath, "%s/%s", getcwd( NULL, 0 ), path );
  return( fpath  );
}
char *is_hg( char *dir ) {
  static
  char  cwd[ MAXNAMLEN ];
  char *pd,
       *hg="/.hg",
       *po;

  bool  is_hg = false;

  if ( dir && RMisdir( dir ) ) pd = strcpy( cwd, dir );
  else return NULL;

  while ( !is_hg && pd && (  strlen(pd) > 1 ) ) {
    po = strrchr( pd, '/' );
    if ( po ) {
      strcat( pd, hg );
      if ( ! ( is_hg = RMisdir( pd ) ) ) {
        *po = '\0';                    // truncate last directory
      } else {
        po = strrchr( pd, '/' );
        *po = '\0';                    // truncate last directory
      }
    } else *pd = '\0';
  }

  return( is_hg ? cwd : NULL );
}
char *load_hgstatus( const char *dir ) {
  off_t sz;
  static
  char *lst = NULL,
       *hld = NULL;
  char *cmd = NULL,
       *hgc = "hg status -mardui";

  if ( !lst || strcmp( dir, lst ) ) {
    if( lst ) free( lst );
    lst = strdup( dir );
    cmd = malloc( strlen( dir ) + strlen( hgc ) + 4 );
    sprintf( cmd, "%s %s", hgc, dir );
    return ( (hld=loadpipe( cmd, &sz )) );
  } else return( hld );
}
char  get_hgstatus( char *dir, char *file, char *hgs ) {
  char *pt1 = NULL,
       *pt2 = NULL,
       *pf  = NULL,
        buf[MAX_DNAME],
        ch  = ' ';

  if ( RMisdir( file ) ) return( ch );

  pt1 = pt2 = hgs;

  if ( ! strcmp( dir, "." ) ) {
    pf = file;
  } else {
    sprintf( buf, "%s/%s", dir, file );
    pf = buf;
  }

  bool done = false;
  do {
    pt1 = pt2;
    pt2 = strstr( pt1, pf );
    if ( pt2 ) {
      if ( *(pt2-1) == ' ' ) done = true;
      else pt2++;
    } else done = true;

  } while ( !done );

  if ( pt2 ) {
    pt2 -= 2;
    switch( *pt2 ) {
      case 'M': ch = 'M'; break;   // modified
      case 'A': ch = 'A'; break;   // added
      case 'R': ch = 'R'; break;   // removed
      case 'D': ch = 'D'; break;   // deleted/missing
      case '?': ch = '?'; break;   // unknown/not tracked
      case 'I': ch = 'I'; break;   // ignored
      default : ch = 'E'; printf( ">%c: %s<\n", *pt2, pf ); break;   // error
    }
  }

  return( ch );
}
