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
** ��   ��   ��: s_error.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 02 �� 13 ��
**
** ��        ��: ����ϵͳ�����������

** BUG:
2012.10.20  ���ںܶ��������, ��Ҫʹ�ñ�׼ I/O �� errno, ���Խ��쿪ʼͳһ��������� errno.
*********************************************************************************************************/

#ifndef __S_ERROR_H
#define __S_ERROR_H

/*********************************************************************************************************
  SYSTEM
*********************************************************************************************************/

#define ERROR_SYSTEM_LOW_MEMORY                49500                    /*  SYSTEM ��ȱ���ڴ�           */

/*********************************************************************************************************
  I/O system           50000 - 50500
*********************************************************************************************************/

#define ERROR_IOS_DEVICE_NOT_FOUND             ENXIO                    /*  �豸û���ҵ�                */
#define ERROR_IOS_DRIVER_GLUT                  50001                    /*  �޿���������                */
#define ERROR_IOS_INVALID_FILE_DESCRIPTOR      EBADF                    /*  �ļ�������ȱʧ              */
#define ERROR_IOS_TOO_MANY_OPEN_FILES          EMFILE                   /*  �ļ��򿪵�̫��              */
#define ERROR_IOS_DUPLICATE_DEVICE_NAME        EEXIST                   /*  �ظ�����                    */
#define ERROR_IOS_DRIVER_NOT_SUP               ENOSYS                   /*  ��������֧��              */
#define ERROR_IOS_FILE_OPERATIONS_NULL         50008                    /*  ȱ�� file_operations        */
#define ERROR_IOS_FILE_NOT_SUP                 ENOSYS                   /*  �ļ���֧����ز���          */
#define ERROR_IOS_FILE_WRITE_PROTECTED         EWRPROTECT               /*  �ļ�д����                  */
#define ERROR_IOS_FILE_READ_PROTECTED          50011                    /*  �ļ�������                  */
#define ERROR_IOS_FILE_SYMLINK                 50012                    /*  �ļ�Ϊ���������ļ�          */

/*********************************************************************************************************
  I/O                  50500 - 51000
*********************************************************************************************************/

#define ERROR_IO_NO_DRIVER                     50500                    /*  ȱ����������                */
#define ERROR_IO_UNKNOWN_REQUEST               ENOSYS                   /*  ��Ч ioctl ����             */
#define ERROR_IO_DEVICE_ERROR                  EIO                      /*  �豸����                    */
#define ERROR_IO_DEVICE_TIMEOUT                ETIMEDOUT                /*  �豸��ʱ                    */
#define ERROR_IO_WRITE_PROTECTED               EWRPROTECT               /*  �豸д����                  */
#define ERROR_IO_DISK_NOT_PRESENT              ENOSYS                   /*  �����豸��֧��              */
#define ERROR_IO_CANCELLED                     ECANCELED                /*  ��ǰ������ cancel ��ֹ      */
#define ERROR_IO_NO_DEVICE_NAME_IN_PATH        EINVAL                   /*  û���豸��                  */
#define ERROR_IO_NAME_TOO_LONG                 ENAMETOOLONG             /*  ����̫��                    */
#define ERROR_IO_UNFORMATED                    EFORMAT                  /*  û�и�ʽ�����ʽ����        */
#define ERROR_IO_FILE_EXIST                    EEXIST                   /*  �ļ��Ѿ�����                */
#define ERROR_IO_BUFFER_ERROR                  50513                    /*  ����������                  */
#define ERROR_IO_ABORT                         ECANCELED                /*  ioctl FIOWAITABORT �쳣     */

#define ERROR_IO_ACCESS_DENIED                 EACCES                   /*  �ļ�����ʧ�� (��Ȩ��,       */
                                                                        /*  ����д����ֻ���ļ��ȵ�)     */
#define ERROR_IO_VOLUME_ERROR                  50518                    /*  �����                      */
#define ERROR_IO_FILE_BUSY                     EBUSY                    /*  ���ظ�·������, ���ܲ���  */
#define ERROR_IO_NO_FILE_NAME                  EFAULT                   /*  �ļ���ַ����                */

/*********************************************************************************************************
  select
*********************************************************************************************************/

#define ERROR_IO_SELECT_UNSUPPORT_IN_DRIVER    50600                    /*  ��������֧�� select       */
#define ERROR_IO_SELECT_CONTEXT                50601                    /*  �߳�û�� select context �ṹ*/
#define ERROR_IO_SELECT_WIDTH                  50602                    /*  ����ļ��Ŵ���              */
#define ERROR_IO_SELECT_FDSET_NULL             50603                    /*  �ļ���Ϊ��                  */

/*********************************************************************************************************
  THREAD POOL           51000 - 51500
*********************************************************************************************************/

#define ERROR_THREADPOOL_NULL                  51000                    /*  �̳߳�Ϊ��                  */
#define ERROR_THREADPOOL_FULL                  51001                    /*  û�п����̳߳ؿ��ƿ�        */
#define ERROR_THREADPOOL_MAX_COUNTER           51004                    /*  �߳������������            */

/*********************************************************************************************************
  SYSTEM HOOK LIST      51500 - 52000
*********************************************************************************************************/

#define ERROR_SYSTEM_HOOK_NULL                 51500                    /*  HOOK Ϊ��                   */

/*********************************************************************************************************
  EXCEPT & log               52000 - 52500
*********************************************************************************************************/

#define ERROR_EXCE_LOST                        52000                    /*  ��Ϣ��ʧ                    */
                                                                        /*  except, netjob or hotplug...*/
#define ERROR_LOG_LOST                         52001                    /*  LOG ��Ϣ��ʧ                */
#define ERROR_LOG_FMT                          52002                    /*  LOG ��ʽ���ִ�����          */
#define ERROR_LOG_FDSET_NULL                   52003                    /*  fd_set Ϊ��                 */

/*********************************************************************************************************
  DMA                   52500 - 53000
*********************************************************************************************************/

#define ERROR_DMA_CHANNEL_INVALID              52500                    /*  ͨ������Ч                  */
#define ERROR_DMA_TRANSMSG_INVALID             52501                    /*  ������ƿ����              */
#define ERROR_DMA_DATA_TOO_LARGE               52502                    /*  ����������                  */
#define ERROR_DMA_NO_FREE_NODE                 52503                    /*  û�п��еĽڵ���            */
#define ERROR_DMA_MAX_NODE                     52504                    /*  �ȴ��ڵ����Ѿ���������      */

/*********************************************************************************************************
  POWER MANAGEMENT      53000 - 53500
*********************************************************************************************************/

#define ERROR_POWERM_NODE                      53000                    /*  �ڵ����                    */
#define ERROR_POWERM_TIME                      53001                    /*  ʱ�����                    */
#define ERROR_POWERM_FUNCTION                  53002                    /*  �ص���������                */
#define ERROR_POWERM_NULL                      53003                    /*  �������                    */
#define ERROR_POWERM_FULL                      53004                    /*  û�п��о��                */
#define ERROR_POWERM_STATUS                    53005                    /*  ״̬����                    */

/*********************************************************************************************************
  SIGNAL MANAGEMENT      53500 - 54000
*********************************************************************************************************/

#define ERROR_SIGNAL_SIGQUEUE_NODES_NULL       53500                    /*  ȱ�ٶ��нڵ���ƿ�          */

/*********************************************************************************************************
  HOTPLUG MANAGEMENT     54000 - 54500
*********************************************************************************************************/

#define ERROR_HOTPLUG_POLL_NODE_NULL           54000                    /*  �޷��ҵ�ָ���� poll �ڵ�    */
#define ERROR_HOTPLUG_MESSAGE_NULL             54001                    /*  �޷��ҵ�ָ���� msg �ڵ�     */

#endif                                                                  /*  __S_ERROR_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
