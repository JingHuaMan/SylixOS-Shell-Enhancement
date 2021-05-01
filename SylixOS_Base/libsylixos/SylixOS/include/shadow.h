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
** ��   ��   ��: shadow.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 03 �� 30 ��
**
** ��        ��: UNIX libshadow.
*********************************************************************************************************/

#ifndef __SHADOW_H
#define __SHADOW_H

#include <sys/types.h>

#ifndef _PATH_SHADOW
#define _PATH_SHADOW        "/etc/shadow"
#endif

struct spwd {
    char    *sp_namp;                                                   /* user login name              */
    char    *sp_pwdp;                                                   /* encrypted password           */
    long    sp_lstchg;                                                  /* last password change         */
    
    int     sp_min;                                                     /* days until change allowed.   */
    int     sp_max;                                                     /* days before change required  */
    int     sp_warn;                                                    /* days warning for expiration  */
    int     sp_inact;                                                   /* days before account inactive */
    int     sp_expire;                                                  /* date when account expires    */
    int     sp_flag;                                                    /* reserved for future use      */
};

#ifdef __cplusplus
extern "C" {
#endif

extern void setspent(void);
extern void endspent(void);
extern struct spwd *getspent(void);
extern struct spwd *getspnam(const char *name);
extern struct spwd *fgetspent(FILE *stream);
extern int putspent(struct spwd *p, FILE *stream);

extern int getspent_r(struct spwd *result_buf, char *buffer,
		       size_t buflen, struct spwd **result);

extern int getspnam_r(const char *name, struct spwd *result_buf,
		       char *buffer, size_t buflen,
		       struct spwd **result);

extern int fgetspent_r(FILE *stream, struct spwd *result_buf,
			char *buffer, size_t buflen,
			struct spwd **result);

extern int passwdcheck(const char *name, const char *pass);             /* 0 is OK                      */
extern int userlogin(const char *name, const char *pass,
                     int pass_delay_en);                                /* 0 is OK and set uid gid...   */

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  __SHADOW_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
