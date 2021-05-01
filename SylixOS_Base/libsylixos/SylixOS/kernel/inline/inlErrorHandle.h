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
** ��   ��   ��: inlErrorHandle.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 14 ��
**
** ��        ��: ����ϵͳ��ǰ���󱣴溯��

** BUG
2007.03.22  ����ϵͳû������ʱ�Ĵ��������
2007.11.04  ���� posix error ����.
2008.03.28  �޸��� error handle ��ͨ��ѡ��
2009.03.10  ������ errno ����ػ���, ����ѡ�� API �ɹ�������� errno, �����ڴ��û����.
            �û����ָ��ʹ�� errno = 0;
2009.06.30  _DebugHandle() �Դ���Ĵ������ __FILE__ �� __LINE__ ���ж�λ, ������Щ��������֧�� __func__
            __FUNCTION__ ����, Դ���м�������������.
2009.07.09  ������߳�������ʾ.
2012.07.21  ����ʹ�ú�. ���в���ȫ������ _ErrorLib.c ��ʵ��
*********************************************************************************************************/

#ifndef __INLERRORHANDLE_H
#define __INLERRORHANDLE_H

/*********************************************************************************************************
  ���󱣴���
*********************************************************************************************************/

VOID  _ErrorHandle(ULONG  ulErrorCode);

/*********************************************************************************************************
  �����ӡ
*********************************************************************************************************/

VOID  _DebugMessage(INT  iLevel, CPCHAR  pcPosition, CPCHAR  pcString);

#if LW_CFG_ERRORMESSAGE_EN > 0 || LW_CFG_LOGMESSAGE_EN > 0
#define _DebugHandle(level, msg) \
        (((level) & __ERRORMESSAGE_LEVEL) ? \
         _DebugMessage((level), __func__, (msg)) : \
         _DebugMessage((level), LW_NULL, (msg)))
#else
#define _DebugHandle(level, msg)
#endif                                                                  /*  LW_CFG_ERRORMESSAGE_EN > 0  */
                                                                        /*  LW_CFG_LOGMESSAGE_EN > 0    */
/*********************************************************************************************************
  �����ʽ����ӡ
*********************************************************************************************************/

VOID  _DebugFmtMsg(INT  iLevel, CPCHAR  pcPosition, CPCHAR  pcFmt, ...);

#if LW_CFG_ERRORMESSAGE_EN > 0 || LW_CFG_LOGMESSAGE_EN > 0
#define _DebugFormat(level, fmt, ...)   \
        (((level) & __ERRORMESSAGE_LEVEL) ? \
         _DebugFmtMsg((level), __func__, (fmt), ##__VA_ARGS__) : \
         _DebugFmtMsg((level), LW_NULL, (fmt), ##__VA_ARGS__))
#else
#define _DebugFormat(level, fmt, ...)
#endif                                                                  /*  LW_CFG_ERRORMESSAGE_EN > 0  */
                                                                        /*  LW_CFG_LOGMESSAGE_EN > 0    */
/*********************************************************************************************************
  BUG ��ӡ
*********************************************************************************************************/

#if LW_CFG_BUGMESSAGE_EN > 0
#define _BugHandle(cond, stop, msg) \
        if (LW_UNLIKELY(cond)) {    \
            _DebugMessage(__BUGMESSAGE_LEVEL, __func__, (msg)); \
            if (LW_KERN_BUG_REBOOT_EN_GET()) {  \
                archReboot(LW_REBOOT_FORCE, 0ul);   \
            }   \
            if (stop) { \
                for (;;);   \
            }   \
        }
#else
#define _BugHandle(cond, stop, msg) \
        if (LW_UNLIKELY(cond)) {    \
            if (LW_KERN_BUG_REBOOT_EN_GET()) {  \
                archReboot(LW_REBOOT_FORCE, 0ul);   \
            }   \
            if (stop) { \
                for (;;);   \
            }   \
        }
#endif                                                                  /*  LW_CFG_BUGMESSAGE_EN > 0    */

/*********************************************************************************************************
  BUG ��ʽ����ӡ
*********************************************************************************************************/

#if LW_CFG_BUGMESSAGE_EN > 0
#define _BugFormat(cond, stop, fmt, ...)    \
        if (LW_UNLIKELY(cond)) {    \
            _DebugFmtMsg(__BUGMESSAGE_LEVEL, __func__, (fmt), ##__VA_ARGS__); \
            if (LW_KERN_BUG_REBOOT_EN_GET()) {  \
                archReboot(LW_REBOOT_FORCE, 0ul);   \
            }   \
            if (stop) { \
                for (;;);   \
            }   \
        }
#else
#define _BugFormat(cond, stop, fmt, ...)    \
        if (LW_UNLIKELY(cond)) {    \
            if (LW_KERN_BUG_REBOOT_EN_GET()) {  \
                archReboot(LW_REBOOT_FORCE, 0ul);   \
            }   \
            if (stop) { \
                for (;;);   \
            }   \
        }
#endif                                                                  /*  LW_CFG_BUGMESSAGE_EN > 0    */

/*********************************************************************************************************
  PRINT ��ӡ
*********************************************************************************************************/

#define _PrintHandle(msg)   \
        _DebugMessage(__PRINTMESSAGE_LEVEL, LW_NULL, msg)
#define _PrintFormat(fmt, ...)  \
        _DebugFmtMsg(__PRINTMESSAGE_LEVEL, LW_NULL, fmt, ##__VA_ARGS__)

#endif                                                                  /*  __INLERRORHANDLE_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
