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
** ��   ��   ��: ttinyString.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 07 �� 27 ��
**
** ��        ��: һ����С�͵� shell ϵͳ, �ַ�������������.
*********************************************************************************************************/

#ifndef __TTINYSTRING_H
#define __TTINYSTRING_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

ULONG  __tshellStrConvertVar(CPCHAR       pcCmd, PCHAR  pcCmdOut);
ULONG  __tshellStrDelCRLF(CPCHAR       pcCmd);
ULONG  __tshellStrFormat(CPCHAR       pcCmd, PCHAR  pcCmdOut);
ULONG  __tshellStrKeyword(CPCHAR       pcCmd, PCHAR  pcBuffer, PCHAR  *ppcParam);
ULONG  __tshellStrGetToken(CPCHAR       pcCmd, PCHAR  *ppcNext);
ULONG  __tshellStrDelTransChar(CPCHAR       pcCmd, PCHAR  pcCmdOut);

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
#endif                                                                  /*  __TTINYSTRING_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
