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
** ��   ��   ��: logLib.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 06 �� 12 ��
**
** ��        ��: ϵͳ��־����ϵͳ, ��������������жϺ��������źž��������.
*********************************************************************************************************/

#ifndef __LOGLIB_H
#define __LOGLIB_H

/*********************************************************************************************************
  LOG PRIORITY Linux (compatible)
*********************************************************************************************************/

#define KERN_EMERG      "<0>"                                   /* system is unusable                   */
#define KERN_ALERT      "<1>"                                   /* action must be taken immediately     */
#define KERN_CRIT       "<2>"                                   /* critical conditions                  */
#define KERN_ERR        "<3>"                                   /* error conditions                     */
#define KERN_WARNING    "<4>"                                   /* warning conditions                   */
#define KERN_NOTICE     "<5>"                                   /* normal but significant condition     */
#define KERN_INFO       "<6>"                                   /* informational                        */
#define KERN_DEBUG      "<7>"                                   /* debug-level messages                 */

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_LOG_LIB_EN > 0

extern  int             console_printk[];

#define console_loglevel            (console_printk[0])
#define default_message_loglevel    (console_printk[1])
#define minimum_console_loglevel    (console_printk[2])
#define default_console_loglevel    (console_printk[3])

/*********************************************************************************************************
  API FUNC
*********************************************************************************************************/

LW_API INT    API_LogFdSet(INT    iWidth, fd_set  *pfdsetLog);

LW_API INT    API_LogFdGet(INT  *piWidth, fd_set  *pfdsetLog);

LW_API INT    API_LogFdAdd(INT  iFd);

LW_API INT    API_LogFdDelete(INT  iFd);

LW_API INT    API_LogPrintk(CPCHAR    pcFormat, ...);

LW_API INT    API_LogMsg(CPCHAR       pcFormat,
                         PVOID        pvArg0,
                         PVOID        pvArg1,
                         PVOID        pvArg2,
                         PVOID        pvArg3,
                         PVOID        pvArg4,
                         PVOID        pvArg5,
                         PVOID        pvArg6,
                         PVOID        pvArg7,
                         PVOID        pvArg8,
                         PVOID        pvArg9,
                         BOOL         bIsNeedHeader);
                         
#define logFdSet         API_LogFdSet
#define logFdGet         API_LogFdGet

#define logFdAdd         API_LogFdAdd
#define logFdDelete      API_LogFdDelete

#ifndef printk
#define printk           API_LogPrintk
#endif                                                                  /*  printk                      */

#endif                                                                  /*  LW_CFG_LOG_LIB_EN > 0 &&    */
#endif                                                                  /*  __LOGLIB_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
