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
** ��   ��   ��: gjbext.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2016 �� 04 �� 13 ��
**
** ��        ��: ���� GJB7714 �ӿڿ�.
*********************************************************************************************************/

#ifndef __GJBEXT_H
#define __GJBEXT_H

#ifndef __PX_GJBEXT_H
#include <posix/include/px_gjbext.h>
#endif                                                                  /*  __PX_GJBEXT_H               */

#ifndef __PX_PTHREAD_H
#include <posix/include/px_pthread.h>
#endif                                                                  /*  __PX_PTHREAD_H              */

#ifndef __PX_SEMAPHORE_H
#include <posix/include/px_semaphore.h>
#endif                                                                  /*  __PX_SEMAPHORE_H            */

#ifndef __PX_MQUEUE_H
#include <posix/include/px_mqueue.h>
#endif                                                                  /*  __PX_MQUEUE_H               */

#if (LW_CFG_POSIX_EN > 0) && (LW_CFG_GJB7714_EN > 0)
#define mount   gjb_mount
#define umount  gjb_umount
#define format  gjb_format
#define cat     gjb_cat
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_GJB7714_EN > 0       */
#endif                                                                  /*  __GJBEXT_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
