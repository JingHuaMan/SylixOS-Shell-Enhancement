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
** ��   ��   ��: px_fnmatch.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 07 �� 09 ��
**
** ��        ��: ���� posix fnmatch ��.
*********************************************************************************************************/

#ifndef __PX_FNMATCH_H
#define __PX_FNMATCH_H

#include "SylixOS.h"                                                    /*  ����ϵͳͷ�ļ�              */

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

#define FNM_NOMATCH      1                                              /* Match failed.                */
#define FNM_NOSYS        2                                              /* not supported (unused).      */

#define FNM_NOESCAPE     0x01                                           /* Disable backslash escaping.  */
#define FNM_PATHNAME     0x02                                           /* Slash must matched by slash. */
#define FNM_PERIOD       0x04                                           /* period must matched by period*/
#define FNM_LEADING_DIR  0x08                                           /* Ignore /<tail> after Imatch. */
#define FNM_CASEFOLD     0x10                                           /* Case insensitive search.     */

#define FNM_IGNORECASE   FNM_CASEFOLD
#define FNM_FILE_NAME    FNM_PATHNAME

/*********************************************************************************************************
  api 
*********************************************************************************************************/

LW_API int fnmatch(const char *pattern, const char *string, int flags);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
#endif                                                                  /*  __PX_SYSLOG_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
