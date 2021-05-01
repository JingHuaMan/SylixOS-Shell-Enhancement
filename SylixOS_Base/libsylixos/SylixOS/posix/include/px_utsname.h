/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: px_utsname.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 03 �� 10 ��
**
** ��        ��: posix utsname ���ݿ�.
*********************************************************************************************************/

#ifndef __PX_UTSNAME_H
#define __PX_UTSNAME_H

#include "SylixOS.h"                                                    /*  ����ϵͳͷ�ļ�              */
#include "limits.h"

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  utsname structure
*********************************************************************************************************/

#define __PX_UTSNAME_SYSNAME_SIZE           16
#define __PX_UTSNAME_NODENAME_SIZE          MAX_FILENAME_LENGTH
#define __PX_UTSNAME_RELEASE_SIZE           64
#define __PX_UTSNAME_VERSION_SIZE           128
#define __PX_UTSNAME_MACHINE_SIZE           64

struct utsname {
    char  sysname[__PX_UTSNAME_SYSNAME_SIZE];                           /* Name of this implementation  */
                                                                        /* of the operating system.     */
                                                                        
    char  nodename[__PX_UTSNAME_NODENAME_SIZE];                         /* Name of this node within the */
                                                                        /* communications network to    */
                                                                        /* which this node is attached, */
                                                                        /* if any.                      */
    
    char  release[__PX_UTSNAME_RELEASE_SIZE];                           /* Current release level of this*/
                                                                        /* implementation.              */
                                                                        
    char  version[__PX_UTSNAME_VERSION_SIZE];                           /* Current version level of this*/
                                                                        /* release.                     */
                                                                        
    char  machine[__PX_UTSNAME_MACHINE_SIZE];                           /* Name of the hardware type on */
                                                                        /* which the system is running. */
};

/*********************************************************************************************************
  timeb api
*********************************************************************************************************/

LW_API int  uname(struct utsname *name);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
#endif                                                                  /*  __PX_UTSNAME_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
