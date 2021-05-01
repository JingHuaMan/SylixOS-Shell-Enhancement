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
** ��   ��   ��: crypt.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 03 �� 30 ��
**
** ��        ��: UNIX libcrypt.
*********************************************************************************************************/

#ifndef __CRYPT_H
#define __CRYPT_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern char *crypt(const char *key, const char *salt);
extern char *crypt_safe(const char *key, const char *salt, char *res, size_t reslen);

extern int   setkey(const char *key);
extern int   encrypt(char *block, int flag);

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  __CRYPT_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
