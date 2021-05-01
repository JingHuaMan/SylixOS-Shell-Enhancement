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
** ��   ��   ��: ttinyVar.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 08 �� 06 ��
**
** ��        ��: һ����С�͵� shell ϵͳ�ı���������.
*********************************************************************************************************/

#ifndef __TTINYVAR_H
#define __TTINYVAR_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

/*********************************************************************************************************
  API
*********************************************************************************************************/
LW_API VOIDFUNCPTR  API_TShellVarHookSet(VOIDFUNCPTR  pfuncTSVarHook);

LW_API INT          API_TShellVarGetRt(CPCHAR     pcVarName, 
                                       PCHAR      pcVarValue,
                                       INT        iMaxLen);
LW_API PCHAR        API_TShellVarGet(CPCHAR       pcVarName);
LW_API INT          API_TShellVarSet(CPCHAR       pcVarName, CPCHAR       pcVarValue, INT  iIsOverwrite);
LW_API INT          API_TShellVarDelete(CPCHAR  pcVarName);
LW_API INT          API_TShellVarGetNum(VOID);
LW_API INT          API_TShellVarDup(PVOID (*pfuncMalloc)(size_t stSize), PCHAR  ppcEvn[], ULONG  ulMax);

LW_API INT          API_TShellVarSave(CPCHAR  pcFile);
LW_API INT          API_TShellVarLoad(CPCHAR  pcFile);

#define tshellVarHookSet        API_TShellVarHookSet
#define tshellVarGetRt          API_TShellVarGetRt
#define tshellVarGet            API_TShellVarGet
#define tshellVarSet            API_TShellVarSet
#define tshellVarDelete         API_TShellVarDelete
#define tshellVarGetNum         API_TShellVarGetNum
#define tshellVarDup            API_TShellVarDup
#define tshellVarSave           API_TShellVarSave
#define tshellVarLoad           API_TShellVarLoad

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
#endif                                                                  /*  __TTINYVAR_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
