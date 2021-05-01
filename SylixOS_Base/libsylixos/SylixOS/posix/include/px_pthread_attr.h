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
** ��   ��   ��: px_pthread_attr.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 02 �� 26 ��
**
** ��        ��: pthread ���Լ��ݿ�.

** BUG:
2013.09.17  Ϊ�����δ���Ŀ���չ��, �������� pthread_attr_t ��С, ���ǳ�Ա������������, ����Խ����������
            ����������.
*********************************************************************************************************/

#ifndef __PX_PTHREAD_ATTR_H
#define __PX_PTHREAD_ATTR_H

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

#include "px_sched_param.h"                                             /*  ���Ȳ���                    */

/*********************************************************************************************************
  pthread ���
*********************************************************************************************************/

typedef LW_OBJECT_HANDLE    pthread_t;

/*********************************************************************************************************
  pthread attr
*********************************************************************************************************/

#define PTHREAD_CREATE_DETACHED             0
#define PTHREAD_CREATE_JOINABLE             1                           /*  default                     */

#define PTHREAD_INHERIT_SCHED               0
#define PTHREAD_EXPLICIT_SCHED              1                           /*  default                     */

#define PTHREAD_SCOPE_PROCESS               0
#define PTHREAD_SCOPE_SYSTEM                1                           /*  default                     */

#if (LW_CFG_GJB7714_EN > 0) && !defined(__SYLIXOS_POSIX)

typedef struct {
    char                   *name;                                       /*  ����                        */
    void                   *stackaddr;                                  /*  ָ����ջ��ַ                */
    size_t                  stackguard;                                 /*  ��ջ���������С            */
    size_t                  stacksize;                                  /*  ��ջ��С                    */
    int                     schedpolicy;                                /*  ���Ȳ���                    */
    int                     inheritsched;                               /*  �Ƿ�̳е��Ȳ���            */
    unsigned long           option;                                     /*  ѡ��                        */
    struct sched_param      schedparam;                                 /*  ���Ȳ���                    */
    ULONG                   reservepad[8];                              /*  ����                        */
} pthread_attr_t;

#else

typedef struct {
    char                   *PTHREADATTR_pcName;                         /*  ����                        */
    void                   *PTHREADATTR_pvStackAddr;                    /*  ָ����ջ��ַ                */
    size_t                  PTHREADATTR_stStackGuard;                   /*  ��ջ���������С            */
    size_t                  PTHREADATTR_stStackByteSize;                /*  ��ջ��С                    */
    int                     PTHREADATTR_iSchedPolicy;                   /*  ���Ȳ���                    */
    int                     PTHREADATTR_iInherit;                       /*  �Ƿ�̳е��Ȳ���            */
    unsigned long           PTHREADATTR_ulOption;                       /*  ѡ��                        */
    struct sched_param      PTHREADATTR_schedparam;                     /*  ���Ȳ���                    */
    ULONG                   PTHREADATTR_ulPad[8];                       /*  ����                        */
} pthread_attr_t;

#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __PX_PTHREAD_ATTR_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
