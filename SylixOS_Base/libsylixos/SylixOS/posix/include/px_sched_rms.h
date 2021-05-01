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
** ��   ��   ��: px_sched_rms.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2015 �� 04 �� 17 ��
**
** ��        ��: �߾��� RMS ������ (ʹ�� LW_CFG_TIME_HIGH_RESOLUTION_EN ��Ϊʱ�侫�ȱ�֤).
*********************************************************************************************************/

#ifndef __PX_SCHED_RMS_H
#define __PX_SCHED_RMS_H

#include "SylixOS.h"                                                    /*  ����ϵͳͷ�ļ�              */

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0 && LW_CFG_POSIXEX_EN > 0

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  RMS �ṹ
*********************************************************************************************************/

typedef struct {
    int              PRMS_iStatus;
    struct timespec  PRMS_tsSave;
    void            *PRMS_pvPad[16];
} sched_rms_t;

#ifdef __SYLIXOS_KERNEL
#define PRMS_STATUS_INACTIVE    0
#define PRMS_STATUS_ACTIVE      1
#endif                                                                  /*  __SYLIXOS_KERNEL            */

LW_API int      sched_rms_init(sched_rms_t  *prms, pthread_t  thread);
LW_API int      sched_rms_destroy(sched_rms_t  *prms);
LW_API int      sched_rms_period(sched_rms_t  *prms, const struct timespec *period);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_POSIXEX_EN > 0       */
#endif                                                                  /*  __PX_SCHED_RMS_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
