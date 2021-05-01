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
** ��   ��   ��: px_sched_param.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 02 �� 26 ��
**
** ��        ��: posix ���ȼ��ݿ� struct sched_param ����.

** BUG:
2013.09.17  ���ǵ������� struct sched_param �ṹ���� sched_pad ������, ͬʱ��������������ɼ�����Ӱ��.
*********************************************************************************************************/

#ifndef __PX_SCHED_PARAM_H
#define __PX_SCHED_PARAM_H

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  sched priority convert with SylixOS
*********************************************************************************************************/

#define PX_PRIORITY_CONVERT(prio)   (LW_PRIO_LOWEST - (prio))

/*********************************************************************************************************
  sched policies
*********************************************************************************************************/

struct sched_param {
    int                 sched_priority;                                 /*  POSIX �������ȼ�            */
                                                                        /*  SCHED_SPORADIC parameter    */
    int                 sched_ss_low_priority;                          /*  Low scheduling priority for */
                                                                        /*  sporadic server.            */
    struct timespec     sched_ss_repl_period;                           /*  Replenishment period for    */
                                                                        /*  sporadic server.            */
    struct timespec     sched_ss_init_budget;                           /*  Initial budget for sporadic */
                                                                        /*  server.                     */
    int                 sched_ss_max_repl;                              /*  Max pending replenishments  */
                                                                        /*  for sporadic server.        */
    ULONG               sched_pad[12];                                  /*  ��չ����                    */
};

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __PX_SCHED_PARAM_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
