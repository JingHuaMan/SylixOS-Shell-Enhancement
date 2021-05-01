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
** ��   ��   ��: Lw_Api_Shell.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 14 ��
**
** ��        ��: ����ϵͳ�ṩ�� C / C++ �û����ں�Ӧ�ó���ӿڲ㡣
                 Ϊ����Ӧ��ͬ����ϰ�ߵ��ˣ�����ʹ���˺ܶ��ظ�����.
*********************************************************************************************************/

#ifndef __LW_API_SHELL_H
#define __LW_API_SHELL_H

/*********************************************************************************************************
    API
*********************************************************************************************************/

#define Lw_TShell_Init                  API_TShellInit
#define Lw_TShell_Startup               API_TShellStartup
#define Lw_TShell_SetStackSize          API_TShellSetStackSize
#define Lw_TShell_Create                API_TShellCreate
#define Lw_TShell_CreateEx              API_TShellCreateEx

#define Lw_TShell_GetUserName           API_TShellGetUserName
#define Lw_TShell_GetUserHome           API_TShellGetUserHome
#define Lw_TShell_GetGrpName            API_TShellGetGrpName
#define Lw_TShell_FlushCache            API_TShellFlushCache

#define Lw_TShell_KeywordAdd            API_TShellKeywordAdd
#define Lw_TShell_KeywordAddEx          API_TShellKeywordAddEx
#define Lw_TShell_FormatAdd             API_TShellFormatAdd
#define Lw_TShell_HelpAdd               API_TShellHelpAdd
#define Lw_TShell_Exec                  API_TShellExec
#define Lw_TShell_ExecBg                API_TShellExecBg

#define Lw_TShell_VarHookSet            API_TShellVarHookSet
#define Lw_TShell_VarGetRt              API_TShellVarGetRt
#define Lw_TShell_VarGet                API_TShellVarGet
#define Lw_TShell_VarSet                API_TShellVarSet
#define Lw_TShell_VarDelete             API_TShellVarDelete
#define Lw_TShell_VarGetNum             API_TShellVarGetNum
#define Lw_TShell_VarDup                API_TShellVarDup

#endif                                                                  /*  __LW_API_SHELL_H            */
/*********************************************************************************************************
    OBJECT
*********************************************************************************************************/
