/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: pwd.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 03 �� 30 ��
**
** ��        ��: UNIX passwd file.
*********************************************************************************************************/

#ifndef __PWD_H
#define __PWD_H

#include <sys/types.h>

#ifndef _PATH_PASSWD
#define _PATH_PASSWD        "/etc/passwd"

#define	_PW_KEYBYNAME		'1'	                                        /* stored by name               */
#define	_PW_KEYBYNUM		'2'	                                        /* stored by entry in the "file"*/
#define	_PW_KEYBYUID		'3'	                                        /* stored by uid                */

#define	_PASSWORD_EFMT1		'_'	                                        /* extended DES encryption forma*/
#define	_PASSWORD_NONDES	'$'	                                        /* non-DES encryption formats   */

#define _PASSWORD_LEN       128                                         /* max length, not counting NULL*/
#endif                                                                  /* _PATH_PASSWD                 */

struct passwd {
    char    *pw_name;                                                   /* user name                    */
    char    *pw_passwd;                                                 /* encrypted password           */
    uid_t    pw_uid;                                                    /* user uid                     */
    gid_t    pw_gid;                                                    /* user gid                     */
    char    *pw_comment;                                                /* comment                      */
    char    *pw_gecos;                                                  /* Honeywell login info         */
    char    *pw_dir;                                                    /* home directory               */
    char    *pw_shell;                                                  /* default shell                */
};

#ifdef __cplusplus
extern "C" {
#endif

extern struct passwd    *getpwuid(uid_t);
extern struct passwd    *getpwnam(const char *);
extern int               getpwnam_r(const char *, struct passwd *,
                             char *, size_t , struct passwd **);
extern int               getpwuid_r(uid_t, struct passwd *, char *,
                             size_t, struct passwd **);
            
extern struct passwd    *getpwent(void);
extern void              setpwent(void);
extern void              endpwent(void);

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  __PWD_H                     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
