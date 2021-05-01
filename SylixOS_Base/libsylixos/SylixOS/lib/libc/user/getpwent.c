/*
 *  POSIX 1003.1b - 9.2.2 - User Database Access Routines
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 *
 *  $Id: getpwent.c,v 1.13 2009/09/15 04:18:40 ralf Exp $
 */
 
#include "stdio.h"
#include "sys/types.h"
#include "pwd.h"
#include "grp.h"
#include "shadow.h"
#include "errno.h"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#include "limits.h"
#include "ctype.h"


/*
 * Static, thread-unsafe, buffers
 */
static FILE *passwd_fp;
static char pwbuf[256];
static struct passwd pwent;
static FILE *group_fp;
static char grbuf[256];
static struct group grent;

/*
 * Initialize useable but dummy databases
 */
void __attribute__((weak)) init_etc_passwd_group(void)
{
  FILE *fp;
  static char etc_passwd_initted = 0;

  if (etc_passwd_initted)
    return;
  etc_passwd_initted = 1;

  /*
   *  Initialize /etc/passwd
   *  root, hanhui ����ӳ�䵽 /etc/shadow ��
   *  Ĭ��ͣ�� anonymous �û�
   */
  if (access("/etc/passwd", R_OK) < 0) {
    if ((fp = fopen("/etc/passwd", "w")) != NULL) {
      fprintf(fp, "root:x:0:0:root::/root:/bin/sh\n"
                  "hanhui:x:1:0:admin::/home/hanhui:/bin/sh\n"
                  "anonymous:!:400:400:anonymous::/home/anonymous:/bin/false\n");
      fclose(fp);
    }
  }

  /*
   *  Initialize /etc/group
   */
  if (access("/etc/group", R_OK) < 0) {
    if ((fp = fopen("/etc/group", "w")) != NULL) {
      fprintf(fp, "root:x:0:root,hanhui\n"
                  "anonymous:x:400:anonymous\n");
      fclose(fp);
    }
  }
}

/*
 * Extract a string value from the database
 */
int
scanString(FILE *fp, char **name, char **bufp, size_t *nleft, int nlFlag)
{
  int c;

  *name = *bufp;
  for (;;) {
    c = getc(fp);
    if (c == ':') {
        if (nlFlag)
            return 0;
        break;
    }
    if (c == '\n') {
        if (!nlFlag)
            return 0;
        break;
    }
    if (c == EOF)
      return 0;
    if (*nleft < 2)
      return 0;
    **bufp = (char)c;
    ++(*bufp);
    --(*nleft);
  }
  **bufp = '\0';
  ++(*bufp);
  --(*nleft);
  return 1;
}

/*
 * Extract an integer value from the database
 */
int
scanInt(FILE *fp, int *val)
{
  int c;
  unsigned int i = 0;
  unsigned int limit = INT_MAX;
  int sign = 0;
  int d;

  for (;;) {
    c = getc(fp);
    if (c == ':')
      break;
    if (sign == 0) {
      if (c == '-') {
        sign = -1;
        limit++;
        continue;
      }
      sign = 1;
    }
    if (!isdigit(c))
      return 0;
    d = c - '0';
    if ((i > (limit / 10))
     || ((i == (limit / 10)) && (d > (limit % 10))))
      return 0;
    i = i * 10 + d;
  }
  if (sign == 0)
    return 0;
  *val = i * sign;
  return 1;
}

/*
 * Extract a single password record from the database
 */
int scanpw(
  FILE *fp,
  struct passwd *pwd,
  char *buffer,
  size_t bufsize
)
{
  int pwuid, pwgid;

  if (!scanString(fp, &pwd->pw_name, &buffer, &bufsize, 0)
   || !scanString(fp, &pwd->pw_passwd, &buffer, &bufsize, 0)
   || !scanInt(fp, &pwuid)
   || !scanInt(fp, &pwgid)
   || !scanString(fp, &pwd->pw_comment, &buffer, &bufsize, 0)
   || !scanString(fp, &pwd->pw_gecos, &buffer, &bufsize, 0)
   || !scanString(fp, &pwd->pw_dir, &buffer, &bufsize, 0)
   || !scanString(fp, &pwd->pw_shell, &buffer, &bufsize, 1))
    return 0;
  pwd->pw_uid = pwuid;
  pwd->pw_gid = pwgid;
  return 1;
}

static int getpw_r(
  const char     *name,
  int             uid,
  struct passwd  *pwd,
  char           *buffer,
  size_t          bufsize,
  struct passwd **result
)
{
  FILE *fp;
  int match;

  init_etc_passwd_group();

  if ((fp = fopen("/etc/passwd", "r")) == NULL) {
    errno = EINVAL;
    return -1;
  }
  for(;;) {
    if (!scanpw(fp, pwd, buffer, bufsize)) {
      break;
    }
    if (name) {
      match = (strcmp(pwd->pw_name, name) == 0);
    }
    else {
      match = (pwd->pw_uid == uid);
    }
    if (match) {
      fclose(fp);
      *result = pwd;
      return 0;
    }
  }
  fclose(fp);
  errno = EINVAL;
  return -1;
}

int getpwnam_r(
  const char     *name,
  struct passwd  *pwd,
  char           *buffer,
  size_t          bufsize,
  struct passwd **result
)
{
  return getpw_r(name, 0, pwd, buffer, bufsize, result);
}

struct passwd *getpwnam(
  const char *name
)
{
  struct passwd *p;

  if(getpwnam_r(name, &pwent, pwbuf, sizeof pwbuf, &p))
    return NULL;
  return p;
}

int getpwuid_r(
  uid_t           uid,
  struct passwd  *pwd,
  char           *buffer,
  size_t          bufsize,
  struct passwd **result
)
{
  return getpw_r(NULL, uid, pwd, buffer, bufsize, result);
}

struct passwd *getpwuid(
  uid_t uid
)
{
  struct passwd *p;

  if(getpwuid_r(uid, &pwent, pwbuf, sizeof pwbuf, &p))
    return NULL;
  return p;
}

struct passwd *getpwent(void)
{
  if (passwd_fp == NULL)
    return NULL;
  if (!scanpw(passwd_fp, &pwent, pwbuf, sizeof pwbuf))
    return NULL;
  return &pwent;
}

void setpwent(void)
{
  init_etc_passwd_group();

  if (passwd_fp != NULL)
    fclose(passwd_fp);
  passwd_fp = fopen("/etc/passwd", "r");
}

void endpwent(void)
{
  if (passwd_fp != NULL) {
    fclose(passwd_fp);
    passwd_fp = NULL;
  }
}

/*
 * Extract a single group record from the database
 */
int scangr(
  FILE *fp,
  struct group *grp,
  char *buffer,
  size_t bufsize
)
{
  int grgid;
  char *grmem, *cp;
  int memcount;

  if (!scanString(fp, &grp->gr_name, &buffer, &bufsize, 0)
   || !scanString(fp, &grp->gr_passwd, &buffer, &bufsize, 0)
   || !scanInt(fp, &grgid)
   || !scanString(fp, &grmem, &buffer, &bufsize, 1))
    return 0;
  grp->gr_gid = grgid;

  /*
   * Determine number of members
   */
  for (cp = grmem, memcount = 1 ; *cp != 0 ; cp++) {
    if(*cp == ',')
      memcount++;
  }

  /*
   * Hack to produce (hopefully) a suitably-aligned array of pointers
   */
  if (bufsize < (((memcount+1)*sizeof(char *)) + 15))
    return 0;
  grp->gr_mem = (char **)(((uintptr_t)buffer + 15) & ~15);

  /*
   * Fill in pointer array
   */
  grp->gr_mem[0] = grmem;
  for (cp = grmem, memcount = 1 ; *cp != 0 ; cp++) {
    if(*cp == ',') {
      *cp = '\0';
      grp->gr_mem[memcount++] = cp + 1;
    }
  }
  grp->gr_mem[memcount] = NULL;
  return 1;
}

static int getgr_r(
  const char     *name,
  int             gid,
  struct group   *grp,
  char           *buffer,
  size_t          bufsize,
  struct group  **result
)
{
  FILE *fp;
  int match;

  init_etc_passwd_group();

  if ((fp = fopen("/etc/group", "r")) == NULL) {
    errno = EINVAL;
    return -1;
  }
  for(;;) {
    if (!scangr(fp, grp, buffer, bufsize)) {
      break;
    }
    if (name) {
      match = (strcmp(grp->gr_name, name) == 0);
    }
    else {
      match = (grp->gr_gid == gid);
    }
    if (match) {
      fclose(fp);
      *result = grp;
      return 0;
    }
  }
  fclose(fp);
  errno = EINVAL;
  return -1;
}

int getgrnam_r(
  const char     *name,
  struct group   *grp,
  char           *buffer,
  size_t          bufsize,
  struct group  **result
)
{
  return getgr_r(name, 0, grp, buffer, bufsize, result);
}

struct group *getgrnam(
  const char *name
)
{
  struct group *p;

  if(getgrnam_r(name, &grent, grbuf, sizeof grbuf, &p))
    return NULL;
  return p;
}

int getgrgid_r(
  gid_t           gid,
  struct group   *grp,
  char           *buffer,
  size_t          bufsize,
  struct group  **result
)
{
  return getgr_r(NULL, gid, grp, buffer, bufsize, result);
}

struct group *getgrgid(
  gid_t gid
)
{
  struct group *p;

  if(getgrgid_r(gid, &grent, grbuf, sizeof grbuf, &p))
    return NULL;
  return p;
}

struct group *getgrent(void)
{
  if (group_fp == NULL)
    return NULL;
  if (!scangr(group_fp, &grent, grbuf, sizeof grbuf))
    return NULL;
  return &grent;
}

void setgrent(void)
{
  init_etc_passwd_group();

  if (group_fp != NULL)
    fclose(group_fp);
  group_fp = fopen("/etc/group", "r");
}

void endgrent(void)
{
  if (group_fp != NULL) {
    fclose(group_fp);
    group_fp = NULL;
  }
}

int 
initgroups (const char *name, gid_t basegid)
{
  char bufgroup[512];
  struct group grpbuf;
  FILE *grp;
  
  gid_t gitlist[NGROUPS_MAX];
  int   index = 1;
  
  if (!name) {
    errno = EINVAL;
    return -1;
  }
  
  init_etc_passwd_group();
  
  grp = fopen("/etc/group", "r");
  if (grp == NULL) {
    return 0;
  }
  
  gitlist[0] = basegid;
  
  for (;;) {
    int j;
    
    if (!scangr(grp, &grpbuf, bufgroup, sizeof bufgroup)) {
      break;
    }
    
    if (grpbuf.gr_gid != basegid) {
      for (j = 0; grpbuf.gr_mem[j]; j++) {
        if (strcmp(grpbuf.gr_mem[j], name) == 0) {
            
          gitlist[index++] = grpbuf.gr_gid;
          if (index >= NGROUPS_MAX) {
            break;
          }
        }
      }
    }
  }
  
  fclose(grp);
  if (index > 0) {
    return setgroups(index, gitlist);
  
  } else {
    return 0;
  }
}

/* end */
