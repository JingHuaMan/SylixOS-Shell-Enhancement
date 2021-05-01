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
** ��   ��   ��: ttinyVar.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 08 �� 06 ��
**
** ��        ��: һ����С�͵� shell ϵͳ�ı���������.

** BUG:
2012.10.23  �������ɾ���ӿ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  Ӧ�ü� API
*********************************************************************************************************/
#include "../SylixOS/api/Lw_Api_Kernel.h"
#include "../SylixOS/api/Lw_Api_System.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
#include "../SylixOS/shell/hashLib/hashHorner.h"
#include "../SylixOS/shell/ttinyShell/ttinyShell.h"
#include "../SylixOS/shell/ttinyShell/ttinyShellLib.h"
#include "ttinyVarLib.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
VOIDFUNCPTR  _G_pfuncTSVarHook = LW_NULL;                               /*  �����ı�ص�                */
/*********************************************************************************************************
** ��������: API_TShellVarHookSet
** ��������: �������ı�ʱ, ���õ��û��ص�
** �䡡��  : pfuncTSVarHook    �µĻص�����
** �䡡��  : ��ǰ�Ļص�����.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOIDFUNCPTR  API_TShellVarHookSet (VOIDFUNCPTR  pfuncTSVarHook)
{
    VOIDFUNCPTR  pfuncTSVarOldHook = _G_pfuncTSVarHook;
    
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  �����в���ע������          */
        _ErrorHandle(ENOTSUP);
        return  (LW_NULL);
    }
    
    _G_pfuncTSVarHook = pfuncTSVarHook;
    
    return  (pfuncTSVarOldHook);
}
/*********************************************************************************************************
** ��������: API_TShellVarGetRt
** ��������: ���һ��������ֵ
** �䡡��  : pcVarName     ������
**           pcVarValue    ������ֵ
** �䡡��  : ����ֵ���� or ERROR 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_TShellVarGetRt (CPCHAR  pcVarName, 
                         PCHAR   pcVarValue,
                         INT     iMaxLen)
{
    if (!pcVarName || !pcVarValue || !iMaxLen) {                        /*  �������                    */
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (PX_ERROR);
    }
    
    return  (__tshellVarGetRt(pcVarName, pcVarValue, iMaxLen));
}
/*********************************************************************************************************
** ��������: API_TShellVarGet
** ��������: ���һ��������ֵ
** �䡡��  : pcVarName     ������
** �䡡��  : ������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PCHAR   API_TShellVarGet (CPCHAR  pcVarName)
{
    REGISTER ULONG      ulError;
             PCHAR      pcVarValue;
    
    if (!pcVarName) {                                                   /*  �������                    */
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (LW_NULL);
    }
    
    ulError = __tshellVarGet(pcVarName, &pcVarValue);
    if (ulError) {
        return  (LW_NULL);
    } else {
        return  (pcVarValue);
    }
}
/*********************************************************************************************************
** ��������: API_TShellVarSet
** ��������: ����һ��������ֵ
** �䡡��  : pcVarName     ������
**           pcVarValue    ������ֵ
**           iIsOverwrite  �Ƿ񸲸�
**                         ��� iIsOverwrite ��Ϊ0�����ñ���ԭ�������ݣ���ԭ���ݻᱻ��Ϊ���� pcVarValue 
**                         ��ָ�ı������ݣ���� iIsOverwrite Ϊ0���Ҹû��������������ݣ�
**                         ����� pcVarValue �ᱻ���ԡ�
** �䡡��  : ִ�гɹ��򷵻�0���д�����ʱ����-1��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT   API_TShellVarSet (CPCHAR  pcVarName, CPCHAR  pcVarValue, INT  iIsOverwrite)
{
    REGISTER ULONG      ulError;
    
    if (!pcVarName) {                                                   /*  �������                    */
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (PX_ERROR);
    }
    
    ulError = __tshellVarSet(pcVarName, pcVarValue, iIsOverwrite);
    if (ulError == ERROR_TSHELL_EVAR) {
        ulError =  __tshellVarAdd(pcVarName, pcVarValue, 
                                  lib_strlen(pcVarName));
    }
    
    if (ulError) {
        return  (PX_ERROR);
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: API_TShellVarDelete
** ��������: ɾ��һ������
** �䡡��  : pcVarName     ������
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_TShellVarDelete (CPCHAR  pcVarName)
{
    REGISTER ULONG      ulError;
    
    if (!pcVarName) {                                                   /*  �������                    */
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (PX_ERROR);
    }
    
    ulError = __tshellVarDeleteByName(pcVarName);
    if (ulError) {
        return  (PX_ERROR);
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: API_TShellVarGetNum
** ��������: ��ñ�������
** �䡡��  : NONE
** �䡡��  : ��������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT   API_TShellVarGetNum (VOID)
{
    return  (__tshellVarNum());
}
/*********************************************************************************************************
** ��������: API_TShellVarDup
** ��������: dup shell ����
** �䡡��  : pfuncMalloc       �ڴ���亯��
**           ppcEvn            dup Ŀ��
**           ulMax             ������
** �䡡��  : dup ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT   API_TShellVarDup (PVOID (*pfuncMalloc)(size_t stSize), PCHAR  ppcEvn[], ULONG  ulMax)
{
    if (!pfuncMalloc || !ppcEvn || !ulMax) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (__tshellVarDup(pfuncMalloc, ppcEvn, ulMax));
}
/*********************************************************************************************************
** ��������: API_TShellVarSave
** ��������: ���� shell ����
** �䡡��  : pcFile    �����ļ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_TShellVarSave (CPCHAR  pcFile)
{
    if (!pcFile) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (__tshellVarSave(pcFile));
}
/*********************************************************************************************************
** ��������: API_TShellVarSave
** ��������: ��ȡ shell ����
** �䡡��  : pcFile    �����ļ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_TShellVarLoad (CPCHAR  pcFile)
{
    if (!pcFile) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (__tshellVarLoad(pcFile));
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
