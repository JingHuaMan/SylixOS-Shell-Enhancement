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
** ��   ��   ��: ttinyShellReadline.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 03 �� 25 ��
**
** ��        ��: shell ���ն˶�ȡһ������.
*********************************************************************************************************/

#ifndef __TTINYSHELLREADLINE_H
#define __TTINYSHELLREADLINE_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

ssize_t  __tshellReadline(INT  iFd, PVOID  pcBuffer, size_t  stSize);
VOID     __tshellReadlineClean(LW_OBJECT_HANDLE  ulId, PVOID  pvRetVal, PLW_CLASS_TCB  ptcbDel);
VOID     __tshellHistoryBackup(PLW_CLASS_TCB  ptcbDel);
VOID     __tshellRefreshHistory(INT iFd, PVOID  pcBuffer, size_t  stSize, INT returnValue);

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
#endif                                                                  /*  __TTINYSHELLREADLINE_H      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
