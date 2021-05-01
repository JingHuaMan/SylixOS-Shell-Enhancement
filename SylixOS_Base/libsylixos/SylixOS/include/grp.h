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
** ��   ��   ��: grp.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 03 �� 30 ��
**
** ��        ��: UNIX group file.
*********************************************************************************************************/

#ifndef __GRP_H
#define __GRP_H

#include <sys/types.h>

#ifndef _PATH_GROUP
#define _PATH_GROUP        "/etc/group"
#endif

struct group {
    char        *gr_name;                                               /* group name                   */
    char        *gr_passwd;                                             /* group password               */
    gid_t       gr_gid;                                                 /* group id                     */
    char        **gr_mem;                                               /* group members                */
};

#ifdef __cplusplus
extern "C" {
#endif

extern struct group    *getgrgid(gid_t);
extern struct group    *getgrnam(const char *);
extern int             getgrnam_r(const char *, struct group *,
                           char *, size_t, struct group **);
extern int             getgrgid_r(gid_t, struct group *,
                           char *, size_t, struct group **);
            
extern struct group    *getgrent(void);
extern void            setgrent(void);
extern void            endgrent(void);

LW_API int             setgroups(int groupsun, const gid_t grlist[]);
LW_API int             getgroups(int groupsize, gid_t grlist[]);
extern int             initgroups(const char *name, gid_t basegid);

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  __GRP_H                     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
