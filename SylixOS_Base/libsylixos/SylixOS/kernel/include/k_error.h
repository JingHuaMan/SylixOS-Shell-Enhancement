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
** ��   ��   ��: K_error.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 14 ��
**
** ��        ��: ����ϵͳ�����������

** BUG
2007.11.04 ϵͳ����� 301 ��ʼ���� POSIX ��׼��.
*********************************************************************************************************/

#ifndef __K_ERROR_H
#define __K_ERROR_H

/*********************************************************************************************************
  ��ȫ��ȷ
*********************************************************************************************************/

#define ERROR_SUCCESSFUL                   0                            /*  ϵͳû���κδ���            */
#define ERROR_NONE                         0                            /*  ϵͳû���κδ���            */

/*********************************************************************************************************
  ϵͳ����   300 - 500
*********************************************************************************************************/

#define ERROR_KERNEL_PNAME_NULL          301                            /*  ����ָ��Ϊ NULL             */
#define ERROR_KERNEL_PNAME_TOO_LONG      302                            /*  ����̫��                    */
#define ERROR_KERNEL_HANDLE_NULL         303                            /*  �������                    */
#define ERROR_KERNEL_IN_ISR              304                            /*  ϵͳ�����ж���              */
#define ERROR_KERNEL_RUNNING             305                            /*  ϵͳ��������                */
#define ERROR_KERNEL_NOT_RUNNING         306                            /*  ϵͳû������                */
#define ERROR_KERNEL_OBJECT_NULL         307                            /*  OBJECT Ϊ��                 */
#define ERROR_KERNEL_LOW_MEMORY          308                            /*  ȱ���ڴ�                    */
#define ERROR_KERNEL_BUFFER_NULL         309                            /*  ȱ�ٻ���                    */
#define ERROR_KERNEL_OPTION              310                            /*  ѡ�����                    */
#define ERROR_KERNEL_VECTOR_NULL         311                            /*  �ж���������                */
#define ERROR_KERNEL_HOOK_NULL           312                            /*  �ں˹��ӳ���                */
#define ERROR_KERNEL_OPT_NULL            313                            /*  �ں˹���ѡ�����            */
#define ERROR_KERNEL_MEMORY              314                            /*  �ڴ��ַ���ִ���            */
#define ERROR_KERNEL_LOCK                315                            /*  �ں˱�������                */
#define ERROR_KERNEL_CPU_NULL            316                            /*  ָ�� CPU ����               */
#define ERROR_KERNEL_HOOK_FULL           317                            /*  hook ������                 */
#define ERROR_KERNEL_KEY_CONFLICT        318                            /*  key ��ͻ                    */

/*********************************************************************************************************
  �̴߳��� 500 - 1000
*********************************************************************************************************/

#define ERROR_THREAD_STACKSIZE_LACK      501                            /*  ��ջ̫С                    */
#define ERROR_THREAD_STACK_NULL          502                            /*  ȱ�ٶ�ջ                    */
#define ERROR_THREAD_FP_STACK_NULL       503                            /*  �����ջ                    */
#define ERROR_THREAD_ATTR_NULL           504                            /*  ȱ�����Կ�                  */
#define ERROR_THREAD_PRIORITY_WRONG      505                            /*  ���ȼ�����                  */
#define ERROR_THREAD_WAIT_TIMEOUT        506                            /*  �ȴ���ʱ                    */
#define ERROR_THREAD_NULL                507                            /*  �߳̾����Ч                */
#define ERROR_THREAD_FULL                508                            /*  ϵͳ�߳�����                */
#define ERROR_THREAD_NOT_INIT            509                            /*  �߳�û�г�ʼ��              */
#define ERROR_THREAD_NOT_SUSPEND         510                            /*  �߳�û�б�����              */
#define ERROR_THREAD_VAR_FULL            511                            /*  û�б������ƿ����          */
#define ERROR_THERAD_VAR_NULL            512                            /*  ���ƿ���Ч                  */
#define ERROR_THREAD_VAR_NOT_EXIST       513                            /*  û���ҵ����ʵĿ��ƿ�        */
#define ERROR_THREAD_NOT_READY           514                            /*  �߳�û�о���                */
#define ERROR_THREAD_IN_SAFE             515                            /*  �̴߳��ڰ�ȫģʽ            */
#define ERROR_THREAD_OTHER_DELETE        516                            /*  �Ѿ��������߳��ڵȴ�ɾ��    */
#define ERROR_THREAD_JOIN_SELF           517                            /*  �̺߳ϲ��Լ�                */
#define ERROR_THREAD_DETACHED            518                            /*  �߳��Ѿ��趨Ϊ���ɺϲ�      */
#define ERROR_THREAD_JOIN                519                            /*  �߳��Ѿ��������̺߳ϲ�      */
#define ERROR_THREAD_NOT_SLEEP           520                            /*  �̲߳�û��˯��              */
#define ERROR_THREAD_NOTEPAD_INDEX       521                            /*  ���±���������              */
#define ERROR_THREAD_OPTION              522                            /*  �߳�ѡ����ִ�в�������      */
#define ERROR_THREAD_RESTART_SELF        523                            /*  û�������ź�ϵͳ, ����      */
                                                                        /*  ���������Լ�                */
#define ERROR_THREAD_DELETE_SELF         524                            /*  û�������ź�ϵͳ  ����      */
                                                                        /*  ɾ���Լ�                    */
#define ERROR_THREAD_NEED_SIGNAL_SPT     525                            /*  ��Ҫ�ź�ϵͳ֧��            */
#define ERROR_THREAD_DISCANCEL           526                            /*  �߳������� DISCANCEL ��־   */
#define ERROR_THREAD_INIT                527                            /*  �߳��ڳ�ʼ��״̬            */
#define ERROR_THREAD_RESTART_DELAY       528                            /*  �߳̽������һ�� unsafe ����*/

/*********************************************************************************************************
  ʱ����� 1000 - 1500
*********************************************************************************************************/

#define ERROR_TIME_NULL                 1000                            /*  ʱ��Ϊ��                    */

/*********************************************************************************************************
  �¼����� 1500 - 2000
*********************************************************************************************************/

#define ERROR_EVENT_MAX_COUNTER_NULL    1500                            /*  ���ֵ����                  */
#define ERROR_EVENT_INIT_COUNTER        1501                            /*  ��ʼֵ����                  */
#define ERROR_EVENT_NULL                1502                            /*  �¼����ƿ����              */
#define ERROR_EVENT_FULL                1503                            /*  �¼����ƿ�������            */
#define ERROR_EVENT_TYPE                1504                            /*  �¼����ͳ���                */
#define ERROR_EVENT_WAS_DELETED         1505                            /*  �¼��Ѿ���ɾ��              */
#define ERROR_EVENT_NOT_OWN             1506                            /*  û���¼�����Ȩ              */

/*********************************************************************************************************
  �жϴ��� 1550 - 2000
*********************************************************************************************************/

#define ERROR_INTER_LEVEL_NULL          1550                            /*  LOCK ��ڲ���Ϊ��           */

/*********************************************************************************************************
  �¼����� 2000 - 2500
*********************************************************************************************************/

#define ERROR_EVENTSET_NULL             2000                            /*  �¼���û��                  */
#define ERROR_EVENTSET_FULL             2001                            /*  �¼�������                  */
#define ERROR_EVENTSET_TYPE             2002                            /*  �¼������ʹ���              */
#define ERROR_EVENTSET_WAIT_TYPE        2003                            /*  �¼��ȴ�������              */
#define ERROR_EVENTSET_WAS_DELETED      2004                            /*  �¼����Ѿ���ɾ��            */
#define ERROR_EVENTSET_OPTION           2005                            /*  �¼�ѡ�����                */

/*********************************************************************************************************
  ��Ϣ���д��� 2500 - 3000
*********************************************************************************************************/

#define ERROR_MSGQUEUE_MAX_COUNTER_NULL     2500                        /*  ��Ϣ������                  */
#define ERROR_MSGQUEUE_MAX_LEN_NULL         2501                        /*  ����ȿ�                  */
#define ERROR_MSGQUEUE_FULL                 2502                        /*  ��Ϣ������                  */
#define ERROR_MSGQUEUE_NULL                 2503                        /*  ��Ϣ���п�                  */
#define ERROR_MSGQUEUE_TYPE                 2504                        /*  ��Ϣ���ʹ�                  */
#define ERROR_MSGQUEUE_WAS_DELETED          2505                        /*  ���б�ɾ����                */
#define ERROR_MSGQUEUE_MSG_NULL             2506                        /*  ��Ϣ��                      */
#define ERROR_MSGQUEUE_MSG_LEN              2507                        /*  ��Ϣ���ȴ�                  */
#define ERROR_MSGQUEUE_OPTION               2508                        /*  OPTION ѡ���               */

/*********************************************************************************************************
  ��ʱ������ 3000 - 3500
*********************************************************************************************************/

#define ERROR_TIMER_FULL                    3000                        /*  ��ʱ������                  */
#define ERROR_TIMER_NULL                    3001                        /*  ��ʱ��Ϊ��                  */
#define ERROR_TIMER_CALLBACK_NULL           3002                        /*  �ص�Ϊ��                    */
#define ERROR_TIMER_ISR                     3003                        /*  ��ʱ��������ɸò���        */
                                                                        /*  �����ڸ��� _ItimerThread    */
                                                                        /*  �߳��в��� ITIMER ����      */
#define ERROR_TIMER_TIME                    3004                        /*  ʱ�����                    */
#define ERROR_TIMER_OPTION                  3005                        /*  ѡ�����                    */

/*********************************************************************************************************
  PARTITION 3500 - 4000
*********************************************************************************************************/

#define ERROR_PARTITION_FULL                3500                        /*  ȱ��PARTITION���ƿ�         */
#define ERROR_PARTITION_NULL                3501                        /*  PARTITION��ز���Ϊ��       */
#define ERROR_PARTITION_BLOCK_COUNTER       3502                        /*  �ֿ���������                */
#define ERROR_PARTITION_BLOCK_SIZE          3503                        /*  �ֿ��С����                */
#define ERROR_PARTITION_BLOCK_USED          3504                        /*  �зֿ鱻ʹ��                */

/*********************************************************************************************************
  REGION 4000 - 4500
*********************************************************************************************************/

#define ERROR_REGION_FULL                   4000                        /*  ȱ�����ƿ�                  */
#define ERROR_REGION_NULL                   4001                        /*  ���ƿ����                  */
#define ERROR_REGION_SIZE                   4002                        /*  ������С̫С                */
#define ERROR_REGION_USED                   4003                        /*  ��������ʹ��                */
#define ERROR_REGION_ALIGN                  4004                        /*  �����ϵ����                */
#define ERROR_REGION_NOMEM                  4005                        /*  û���ڴ�ɹ�����            */

/*********************************************************************************************************
  RMS 4500 - 5000
*********************************************************************************************************/

#define ERROR_RMS_FULL                      4500                        /*  ȱ�����ƿ�                  */
#define ERROR_RMS_NULL                      4501                        /*  ���ƿ����                  */
#define ERROR_RMS_TICK                      4502                        /*  TICK ����                   */
#define ERROR_RMS_WAS_CHANGED               4503                        /*  ״̬���ı�                  */
#define ERROR_RMS_STATUS                    4504                        /*  ״̬����                    */

/*********************************************************************************************************
  RTC 5000 - 5500
*********************************************************************************************************/

#define ERROR_RTC_NULL                      5000                        /*  û�� RTC                    */
#define ERROR_RTC_TIMEZONE                  5001                        /*  ʱ������                    */

/*********************************************************************************************************
  VMM 5500 - 6000
*********************************************************************************************************/

#define ERROR_VMM_LOW_PHYSICAL_PAGE         5500                        /*  ȱ������ҳ��                */
#define ERROR_VMM_LOW_LEVEL                 5501                        /*  �ײ������������            */
#define ERROR_VMM_PHYSICAL_PAGE             5502                        /*  ����ҳ�����                */
#define ERROR_VMM_VIRTUAL_PAGE              5503                        /*  ����ҳ�����                */
#define ERROR_VMM_PHYSICAL_ADDR             5504                        /*  �����ַ����                */
#define ERROR_VMM_VIRTUAL_ADDR              5505                        /*  �����ַ����                */
#define ERROR_VMM_ALIGN                     5506                        /*  �����ϵ����                */
#define ERROR_VMM_PAGE_INVAL                5507                        /*  ҳ����Ч                    */
#define ERROR_VMM_LOW_PAGE                  5508                        /*  ȱ��ҳ��                    */

#endif                                                                  /*  __K_ERROR_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
