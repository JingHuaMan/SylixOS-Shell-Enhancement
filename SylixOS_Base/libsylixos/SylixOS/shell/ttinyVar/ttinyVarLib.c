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
** ��   ��   ��: ttinyVarLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 08 �� 06 ��
**
** ��        ��: һ����С�͵� shell ϵͳ�ı����������ڲ��ļ�.

** BUG:
2011.06.03  ���뻷�������ı���Ͷ�ȡ����.
2012.10.19  __tshellVarGet() ֱ�ӷ��ر�����ֵ���ڴ�, ����ʹ�� static buffer.
            __tshellVarSet() ʱ����б� __tshellVarGet() ���ù��ڴ�, ���ͷ��ϵ��ڴ�!
2013.05.10  __tshellVarDefine() �кϷ��ı�����֧�ֺ�������.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
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
#include "../SylixOS/shell/ttinyShell/ttinyString.h"
#include "ttinyVarLib.h"
/*********************************************************************************************************
  ִ�лص�����
*********************************************************************************************************/
extern VOIDFUNCPTR  _G_pfuncTSVarHook;

#define __TSHELL_RUN_HOOK(pcVarName)            \
        {                                       \
            if (_G_pfuncTSVarHook) {            \
                _G_pfuncTSVarHook(pcVarName);   \
            }                                   \
        }
/*********************************************************************************************************
  �ڲ�����ͷ
*********************************************************************************************************/
static PLW_LIST_LINE    _G_plineTSVarHeader = LW_NULL;                  /*  ͳһ����ͷ                  */
static PLW_LIST_LINE    _G_plineTSVarHeaderHashTbl[LW_CFG_SHELL_VAR_HASH_SIZE];
                                                                        /*  hash ɢ�б�                 */
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
INT  __tshellExec(CPCHAR  pcCommandExec, VOIDFUNCPTR  pfuncHook);
/*********************************************************************************************************
** ��������: __tshellVarAdd
** ��������: �� ttiny shell ϵͳ���һ������
** �䡡��  : pcVarName     ������
**           pcVarValue    ������ֵ
**           stNameStrLen  ���ֳ���
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __tshellVarAdd (CPCHAR       pcVarName, CPCHAR       pcVarValue, size_t  stNameStrLen)
{
    REGISTER __PTSHELL_VAR        pskvNode         = LW_NULL;           /*  �����ڵ�                    */
    REGISTER PLW_LIST_LINE       *pplineHashHeader = LW_NULL;
    REGISTER INT                  iHashVal;
    REGISTER size_t               stValueStrLen;
    
    /*
     *  �����ڴ�
     */
    pskvNode = (__PTSHELL_VAR)__SHEAP_ALLOC(sizeof(__TSHELL_VAR));
    if (!pskvNode) {                                                    /*  ����ʧ��                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (ERROR_SYSTEM_LOW_MEMORY);                              /*  ȱ���ڴ�                    */
    }
    
    pskvNode->SV_pcVarName = (PCHAR)__SHEAP_ALLOC(stNameStrLen + 1);
    if (!pskvNode->SV_pcVarName) {
        __SHEAP_FREE(pskvNode);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (ERROR_SYSTEM_LOW_MEMORY);                              /*  ȱ���ڴ�                    */
    }
    
    lib_strcpy(pskvNode->SV_pcVarName, pcVarName);                      /*  ���������                  */
    
    pskvNode->SV_ulRefCnt = 0ul;                                        /*  û���ⲿ���ô˱�������      */
    
    if (pcVarValue) {
        stValueStrLen = lib_strlen(pcVarValue);
        
        pskvNode->SV_pcVarValue = (PCHAR)__SHEAP_ALLOC(stValueStrLen + 1);
        if (!pskvNode->SV_pcVarValue) {
            __SHEAP_FREE(pskvNode->SV_pcVarName);
            __SHEAP_FREE(pskvNode);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (ERROR_SYSTEM_LOW_MEMORY);                          /*  ȱ���ڴ�                    */
        }
        
        lib_strcpy(pskvNode->SV_pcVarValue, pcVarValue);
    
    } else {
        pskvNode->SV_pcVarValue = (PCHAR)__SHEAP_ALLOC(1);
        if (!pskvNode->SV_pcVarValue) {
            __SHEAP_FREE(pskvNode->SV_pcVarName);
            __SHEAP_FREE(pskvNode);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (ERROR_SYSTEM_LOW_MEMORY);                          /*  ȱ���ڴ�                    */
        }
        
        lib_strcpy(pskvNode->SV_pcVarValue, "\0");
    }
    
    iHashVal = __hashHorner(pcVarName, LW_CFG_SHELL_VAR_HASH_SIZE);     /*  ȷ��һ��ɢ�е�λ��          */

    __TTINY_SHELL_LOCK();                                               /*  �������                    */
    /*
     *  �����������
     */
    _List_Line_Add_Ahead(&pskvNode->SV_lineManage, 
                         &_G_plineTSVarHeader);                         /*  ��������ͷ                */
    
    /*
     *  �����ϣ��
     */
    pplineHashHeader = &_G_plineTSVarHeaderHashTbl[iHashVal];
    
    _List_Line_Add_Ahead(&pskvNode->SV_lineHash, 
                         pplineHashHeader);                             /*  ������Ӧ�ı�                */
    
    __TTINY_SHELL_UNLOCK();                                             /*  �ͷ���Դ                    */
    
    __TSHELL_RUN_HOOK(pcVarName);                                       /*  ���ø�д�ص�                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellVarDelete
** ��������: �� ttiny shell ϵͳ��ɾ��һ������
** �䡡��  : pskvNode      �����ڵ�
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __tshellVarDelete (__PTSHELL_VAR  pskvNode)
{
    REGISTER PLW_LIST_LINE       *pplineHashHeader = LW_NULL;
    REGISTER INT                  iHashVal;
    
    __TTINY_SHELL_LOCK();                                               /*  �������                    */
    /*
     *  �ӹ���������ɾ��
     */
    _List_Line_Del(&pskvNode->SV_lineManage, 
                   &_G_plineTSVarHeader);                               /*  ��������ͷ                */
    
    /*
     *  �ӹ�ϣ����ɾ��
     */
    iHashVal = __hashHorner(pskvNode->SV_pcVarName, 
                            LW_CFG_SHELL_VAR_HASH_SIZE);                /*  ȷ��һ��ɢ�е�λ��          */
     
    pplineHashHeader = &_G_plineTSVarHeaderHashTbl[iHashVal];
    
    _List_Line_Del(&pskvNode->SV_lineHash, 
                   pplineHashHeader);                                   /*  ɾ��                        */

    __TTINY_SHELL_UNLOCK();                                             /*  �ͷ���Դ                    */
    
    __TSHELL_RUN_HOOK(pskvNode->SV_pcVarName);                          /*  ���ûص�                    */
    
    if (pskvNode->SV_ulRefCnt == 0) {                                   /*  �б�ֱ�����ù��ڴ�          */
        __SHEAP_FREE(pskvNode->SV_pcVarValue);
    }
    __SHEAP_FREE(pskvNode->SV_pcVarName);
    __SHEAP_FREE(pskvNode);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellVarDeleteByName
** ��������: �� ttiny shell ϵͳ��ɾ��һ������ (����Ϊ����)
** �䡡��  : pcVarName     ������
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __tshellVarDeleteByName (CPCHAR  pcVarName)
{
    REGISTER PLW_LIST_LINE        plineHash;
    REGISTER PLW_LIST_LINE       *pplineHashHeader = LW_NULL;
    REGISTER __PTSHELL_VAR        pskvNode = LW_NULL;                   /*  �����ڵ�                    */
    REGISTER INT                  iHashVal;

    iHashVal = __hashHorner(pcVarName, LW_CFG_SHELL_VAR_HASH_SIZE);     /*  ȷ��һ��ɢ�е�λ��          */
    
    __TTINY_SHELL_LOCK();                                               /*  �������                    */
    plineHash = _G_plineTSVarHeaderHashTbl[iHashVal];
    for (;
         plineHash != LW_NULL;
         plineHash  = _list_line_get_next(plineHash)) {
    
        pskvNode = _LIST_ENTRY(plineHash, __TSHELL_VAR,
                               SV_lineHash);
    
        if (lib_strcmp(pcVarName, pskvNode->SV_pcVarName) == 0) {       /*  ��������ͬ                  */
            break;
        }
    }
    
    if (plineHash == LW_NULL) {
        __TTINY_SHELL_UNLOCK();                                         /*  �ͷ���Դ                    */
        _ErrorHandle(ERROR_TSHELL_EVAR);
        return  (ERROR_TSHELL_EVAR);                                    /*  û���ҵ�����                */
    }
    
    /*
     *  �ӹ���������ɾ��
     */
    _List_Line_Del(&pskvNode->SV_lineManage, 
                   &_G_plineTSVarHeader);                               /*  ��������ͷ                */
                   
    pplineHashHeader = &_G_plineTSVarHeaderHashTbl[iHashVal];
    
    _List_Line_Del(&pskvNode->SV_lineHash, 
                   pplineHashHeader);                                   /*  ɾ��                        */
    
    __TTINY_SHELL_UNLOCK();                                             /*  �ͷ���Դ                    */
    
    __TSHELL_RUN_HOOK(pskvNode->SV_pcVarName);                          /*  ���ûص�                    */
    
    if (pskvNode->SV_ulRefCnt == 0) {                                   /*  �б�ֱ�����ù��ڴ�          */
        __SHEAP_FREE(pskvNode->SV_pcVarValue);
    }
    __SHEAP_FREE(pskvNode->SV_pcVarName);
    __SHEAP_FREE(pskvNode);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellVarFind
** ��������: �� ttiny shell ϵͳ����һ������.
** �䡡��  : pcVarName     ������
**           ppskvNode     �ؼ��ֽڵ�˫ָ��
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __tshellVarFind (char  *pcVarName, __PTSHELL_VAR   *ppskvNode)
{
    REGISTER PLW_LIST_LINE        plineHash;
    REGISTER __PTSHELL_VAR        pskvNode = LW_NULL;                   /*  �����ڵ�                    */
    REGISTER INT                  iHashVal;

    iHashVal = __hashHorner(pcVarName, LW_CFG_SHELL_VAR_HASH_SIZE);     /*  ȷ��һ��ɢ�е�λ��          */
    
    __TTINY_SHELL_LOCK();                                               /*  �������                    */
    plineHash = _G_plineTSVarHeaderHashTbl[iHashVal];
    for (;
         plineHash != LW_NULL;
         plineHash  = _list_line_get_next(plineHash)) {
    
        pskvNode = _LIST_ENTRY(plineHash, __TSHELL_VAR,
                               SV_lineHash);
    
        if (lib_strcmp(pcVarName, pskvNode->SV_pcVarName) == 0) {       /*  ��������ͬ                  */
            break;
        }
    }
    
    if (plineHash == LW_NULL) {
        __TTINY_SHELL_UNLOCK();                                         /*  �ͷ���Դ                    */
        _ErrorHandle(ERROR_TSHELL_EVAR);
        return  (ERROR_TSHELL_EVAR);                                    /*  û���ҵ�����                */
    }
    
    *ppskvNode = pskvNode;
    
    __TTINY_SHELL_UNLOCK();                                             /*  �ͷ���Դ                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellVarList
** ��������: ��������е����б������ƿ��ַ
** �䡡��  : pskvNodeStart   ��ʼ�ڵ��ַ, NULL ��ʾ��ͷ��ʼ
**           ppskvNode[]     �ڵ��б�
**           iMaxCounter     �б��п��Դ�ŵ����ڵ�����
** �䡡��  : ��ʵ��ȡ�Ľڵ�����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __tshellVarList (__PTSHELL_VAR   pskvNodeStart,
                        __PTSHELL_VAR   ppskvNode[],
                        INT             iMaxCounter)
{
    REGISTER INT                  i = 0;
    
    REGISTER PLW_LIST_LINE        plineNode;
    REGISTER __PTSHELL_VAR        pskvNode = pskvNodeStart;             /*  �����ڵ�                    */
    
    __TTINY_SHELL_LOCK();                                               /*  �������                    */
    if (pskvNode == LW_NULL) {
        plineNode = _G_plineTSVarHeader;
    } else {
        plineNode = _list_line_get_next(&pskvNode->SV_lineManage);
    }
    
    for (;
         plineNode != LW_NULL;
         plineNode  = _list_line_get_next(plineNode)) {                 /*  ��ʼ����                    */
        
        pskvNode = _LIST_ENTRY(plineNode, __TSHELL_VAR,
                               SV_lineManage);
                               
        ppskvNode[i++] = pskvNode;
        
        if (i >= iMaxCounter) {                                         /*  �Ѿ���������                */
            break;
        }
    }
    __TTINY_SHELL_UNLOCK();                                             /*  �ͷ���Դ                    */
    
    return  ((ULONG)i);
}
/*********************************************************************************************************
** ��������: __tshellVarNum
** ��������: ��� shell ��������
** �䡡��  : NONE
** �䡡��  : ��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __tshellVarNum (VOID)
{
    REGISTER PLW_LIST_LINE        plineNode;
             INT                  iNum = 0;
    
    __TTINY_SHELL_LOCK();                                               /*  �������                    */
    for (plineNode  = _G_plineTSVarHeader;
         plineNode != LW_NULL;
         plineNode  = _list_line_get_next(plineNode)) {                 /*  ��ʼ����                    */
        iNum++;
    }
    __TTINY_SHELL_UNLOCK();                                             /*  �ͷ���Դ                    */
    
    return  (iNum);
}
/*********************************************************************************************************
** ��������: __tshellVarDup
** ��������: dup shell ����
** �䡡��  : pfuncStrdup       �ڴ���亯��
**           ppcEvn            dup Ŀ��
**           ulMax             ������
** �䡡��  : dup ����
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : Han.hui �Ե�ǰ��� shell �����������յķ��������Ǻ�����, ��Ϊ shell �лص����û����̵ĺ���.
*********************************************************************************************************/
INT   __tshellVarDup (PVOID (*pfuncMalloc)(size_t stSize), PCHAR  ppcEvn[], ULONG  ulMax)
{
             INT            iDupNum = 0;
    REGISTER PLW_LIST_LINE  plineNode;
    REGISTER __PTSHELL_VAR  pskvNode;
             PCHAR          pcLine;
    
    __TTINY_SHELL_LOCK();                                               /*  �������                    */
    for (plineNode  = _G_plineTSVarHeader;
         plineNode != LW_NULL;
         plineNode  = _list_line_get_next(plineNode)) {                 /*  ��ʼ����                    */
        
        size_t  stLen;
        size_t  stNameLen;
        size_t  stValueLen;
        
        pskvNode = _LIST_ENTRY(plineNode, __TSHELL_VAR,
                               SV_lineManage);
        if (iDupNum < ulMax) {
            stNameLen  = lib_strlen(pskvNode->SV_pcVarName);
            stValueLen = lib_strlen(pskvNode->SV_pcVarValue);
            stLen      = stNameLen + stValueLen + 2;
            
            LW_SOFUNC_PREPARE(pfuncMalloc);
            pcLine = (PCHAR)pfuncMalloc(stLen);
            if (pcLine == LW_NULL) {
                break;
            }
            
            lib_strcpy(pcLine, pskvNode->SV_pcVarName);
            lib_strcpy(&pcLine[stNameLen], "=");
            lib_strcpy(&pcLine[stNameLen + 1], pskvNode->SV_pcVarValue);
            
            ppcEvn[iDupNum] = pcLine;
            
            iDupNum++;
        
        } else {
            break;
        }
    }
    __TTINY_SHELL_UNLOCK();                                             /*  �ͷ���Դ                    */
    
    return  (iDupNum);
}
/*********************************************************************************************************
** ��������: __tshellVarSave
** ��������: �����еĻ����������浽�ļ�.
** �䡡��  : pcFileName     ��Ҫ������ļ��� (�������, ���򴴽�, һ��Ϊ "/etc/profile")
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __tshellVarSave (CPCHAR  pcFileName)
{
             FILE                *pfile;
    REGISTER PLW_LIST_LINE        plineNode;
    REGISTER __PTSHELL_VAR        pskvNode;                             /*  �����ڵ�                    */
    
    pfile = fopen(pcFileName, "w");
    if (pfile == LW_NULL) {
        return  (PX_ERROR);
    }
    
    fprintf(pfile, "#sylixos environment variables profile.\n");
    __TTINY_SHELL_LOCK();                                               /*  �������                    */
    for (plineNode  = _G_plineTSVarHeader;
         plineNode != LW_NULL;
         plineNode  = _list_line_get_next(plineNode)) {                 /*  ��ʼ����                    */
         
        pskvNode = _LIST_ENTRY(plineNode, __TSHELL_VAR,
                               SV_lineManage);
                               
        if ((lib_strcmp(pskvNode->SV_pcVarName, "SYSTEM")    == 0) ||
            (lib_strcmp(pskvNode->SV_pcVarName, "VERSION")   == 0) ||
            (lib_strcmp(pskvNode->SV_pcVarName, "LICENSE")   == 0) ||
            (lib_strcmp(pskvNode->SV_pcVarName, "TMPDIR")    == 0) ||
            (lib_strcmp(pskvNode->SV_pcVarName, "KERN_FLOAT") == 0)) {  /*  ��Щ������������          */
            continue;
        }

        /*
         *  TODO:����û���жϱ�����ֵ�к��� " �����.
         */
        fprintf(pfile, "%s=\"%s\"\n", pskvNode->SV_pcVarName, pskvNode->SV_pcVarValue);
    }
    __TTINY_SHELL_UNLOCK();                                             /*  �ͷ���Դ                    */
    
    fclose(pfile);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellVarLoad
** ��������: �� profile �ļ��ж�ȡ���л���������ֵ (һ��Ϊ "/etc/profile")
** �䡡��  : pcFileName     profile �ļ���
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __tshellVarLoad (CPCHAR  pcFileName)
{
    CHAR                 cLineBuffer[LW_CFG_SHELL_MAX_COMMANDLEN + 1];
    PCHAR                pcCmd;
    FILE                *pfile;
             
    pfile = fopen(pcFileName, "r");
    if (pfile == LW_NULL) {
        return  (PX_ERROR);
    }
    
    do {
        pcCmd = fgets(cLineBuffer, LW_CFG_SHELL_MAX_COMMANDLEN, 
                      pfile);                                           /*  ���һ��ָ��                */
        if (pcCmd) {
            __tshellExec(pcCmd, LW_NULL);                               /*  ������ֵ                    */
        }
    } while (pcCmd);
    
    fclose(pfile);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellVarGetRt
** ��������: ���һ��������ֵ
** �䡡��  : pcVarName       ������
**           pcVarValue      ������ֵ˫ָ��
** �䡡��  : ����ֵ�ĳ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __tshellVarGetRt (CPCHAR       pcVarName, 
                       PCHAR        pcVarValue,
                       INT          iMaxLen)
{
    REGISTER PLW_LIST_LINE        plineHash;
    REGISTER __PTSHELL_VAR        pskvNode = LW_NULL;                   /*  �����ڵ�                    */
    REGISTER INT                  iHashVal;

    iHashVal = __hashHorner(pcVarName, LW_CFG_SHELL_VAR_HASH_SIZE);     /*  ȷ��һ��ɢ�е�λ��          */
    
    __TTINY_SHELL_LOCK();                                               /*  �������                    */
    plineHash = _G_plineTSVarHeaderHashTbl[iHashVal];
    for (;
         plineHash != LW_NULL;
         plineHash  = _list_line_get_next(plineHash)) {
    
        pskvNode = _LIST_ENTRY(plineHash, __TSHELL_VAR,
                               SV_lineHash);
    
        if (lib_strcmp(pcVarName, pskvNode->SV_pcVarName) == 0) {       /*  ��������ͬ                  */
            break;
        }
    }
    
    if (plineHash == LW_NULL) {
        __TTINY_SHELL_UNLOCK();                                         /*  �ͷ���Դ                    */
        _ErrorHandle(ERROR_TSHELL_EVAR);
        return  (PX_ERROR);                                             /*  û���ҵ�����                */
    }
    
    lib_strlcpy(pcVarValue, pskvNode->SV_pcVarValue, iMaxLen);
    
    __TTINY_SHELL_UNLOCK();                                             /*  �ͷ���Դ                    */

    return  ((INT)lib_strlen(pcVarValue));
}
/*********************************************************************************************************
** ��������: __tshellVarGet
** ��������: ���һ��������ֵ
** �䡡��  : pcVarName       ������
**           ppcVarValue     ������ֵ˫ָ��
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __tshellVarGet (CPCHAR  pcVarName, PCHAR  *ppcVarValue)
{
    REGISTER PLW_LIST_LINE        plineHash;
    REGISTER __PTSHELL_VAR        pskvNode = LW_NULL;                   /*  �����ڵ�                    */
    REGISTER INT                  iHashVal;

    iHashVal = __hashHorner(pcVarName, LW_CFG_SHELL_VAR_HASH_SIZE);     /*  ȷ��һ��ɢ�е�λ��          */
    
    __TTINY_SHELL_LOCK();                                               /*  �������                    */
    plineHash = _G_plineTSVarHeaderHashTbl[iHashVal];
    for (;
         plineHash != LW_NULL;
         plineHash  = _list_line_get_next(plineHash)) {
    
        pskvNode = _LIST_ENTRY(plineHash, __TSHELL_VAR,
                               SV_lineHash);
    
        if (lib_strcmp(pcVarName, pskvNode->SV_pcVarName) == 0) {       /*  ��������ͬ                  */
            break;
        }
    }
    
    if (plineHash == LW_NULL) {
        __TTINY_SHELL_UNLOCK();                                         /*  �ͷ���Դ                    */
        _ErrorHandle(ERROR_TSHELL_EVAR);
        return  (ERROR_TSHELL_EVAR);                                    /*  û���ҵ�����                */
    }
    
    if (ppcVarValue) {
        *ppcVarValue = pskvNode->SV_pcVarValue;
        if (pskvNode->SV_ulRefCnt != __ARCH_ULONG_MAX) {
            pskvNode->SV_ulRefCnt++;                                    /*  ���ü���                    */
        }
    }
    __TTINY_SHELL_UNLOCK();                                             /*  �ͷ���Դ                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellVarSet
** ��������: ����һ��������ֵ
** �䡡��  : pcVarName       ������
**           pcVarValue      ������ֵ
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __tshellVarSet (CPCHAR       pcVarName, CPCHAR       pcVarValue, INT  iIsOverwrite)
{
    REGISTER PLW_LIST_LINE        plineHash;
    REGISTER __PTSHELL_VAR        pskvNode = LW_NULL;                   /*  �����ڵ�                    */
    REGISTER INT                  iHashVal;
    REGISTER size_t               stValueStrLen;
    REGISTER PCHAR                pcNewBuffer;

    iHashVal = __hashHorner(pcVarName, LW_CFG_SHELL_VAR_HASH_SIZE);     /*  ȷ��һ��ɢ�е�λ��          */
    
    __TTINY_SHELL_LOCK();                                               /*  �������                    */
    plineHash = _G_plineTSVarHeaderHashTbl[iHashVal];
    for (;
         plineHash != LW_NULL;
         plineHash  = _list_line_get_next(plineHash)) {
    
        pskvNode = _LIST_ENTRY(plineHash, __TSHELL_VAR,
                               SV_lineHash);
    
        if (lib_strcmp(pcVarName, pskvNode->SV_pcVarName) == 0) {       /*  ��������ͬ                  */
            break;
        }
    }
    
    if (plineHash == LW_NULL) {
        __TTINY_SHELL_UNLOCK();                                         /*  �ͷ���Դ                    */
        _ErrorHandle(ERROR_TSHELL_EVAR);
        return  (ERROR_TSHELL_EVAR);                                    /*  û���ҵ�����                */
    }
    
    if ((iIsOverwrite == 0) && 
        (lib_strlen(pskvNode->SV_pcVarValue) > 0)) {                    /*  ���ܱ���д                  */
        __TTINY_SHELL_UNLOCK();                                         /*  �ͷ���Դ                    */
        _ErrorHandle(ERROR_TSHELL_CANNT_OVERWRITE);
        return  (ERROR_TSHELL_CANNT_OVERWRITE);
    }
    
    stValueStrLen = lib_strlen(pcVarValue);
    
    pcNewBuffer = (PCHAR)__SHEAP_ALLOC(stValueStrLen + 1);              /*  �����µĻ�����              */
    if (!pcNewBuffer) {
        __TTINY_SHELL_UNLOCK();                                         /*  �ͷ���Դ                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (ERROR_SYSTEM_LOW_MEMORY);                              /*  ȱ���ڴ�                    */
    }
    lib_strcpy(pcNewBuffer, pcVarValue);
    
    if (pskvNode->SV_ulRefCnt == 0) {                                   /*  �б�ֱ�����ù��ڴ�          */
        __SHEAP_FREE(pskvNode->SV_pcVarValue);
    }
    pskvNode->SV_pcVarValue = pcNewBuffer;                              /*  �����µı���ֵ����          */
    
    __TTINY_SHELL_UNLOCK();                                             /*  �ͷ���Դ                    */

    __TSHELL_RUN_HOOK(pcVarName);                                       /*  ���ø�д�ص�                */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellVarDefine
** ��������: ͨ���� ttiny shell ϵͳ���һ������,(�˺������� API_TShellExec ����, �������ǿ�д��)
** �䡡��  : pcCmd         shell ָ��
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __tshellVarDefine (PCHAR  pcCmd)
{
    REGISTER PCHAR      pcTemp  = pcCmd;
    REGISTER PCHAR      pcTemp2 = pcCmd;
    
    REGISTER PCHAR      pcVarName  = (PCHAR)pcCmd;
    REGISTER PCHAR      pcVarValue = LW_NULL;
             PCHAR      pcTokenNext;
             
    REGISTER ULONG      ulError;
    
    for (; *pcTemp != PX_EOS; pcTemp++) {                               /*  ɨ�踳ֵ����                */
        if (*pcTemp == '=') {
            break;
        }
    }
    
    if ((*pcTemp == PX_EOS) || (pcTemp == pcCmd)) {                     /*  �Ǳ�����ֵ�����������      */
        _ErrorHandle(ERROR_TSHELL_CMDNOTFUND);
        return  (ERROR_TSHELL_CMDNOTFUND);
    }
    
    do {
        if (((*pcTemp2 >= 'a') && (*pcTemp2 <= 'z')) ||
            ((*pcTemp2 >= 'A') && (*pcTemp2 <= 'Z')) ||
            ((*pcTemp2 >= '0') && (*pcTemp2 <= '9')) ||
            ((*pcTemp2 == '_'))) {                                      /*  �Ϸ��������ַ�              */
            pcTemp2++;
        } else {
            break;
        }
    } while (pcTemp2 != pcTemp);                                        /*  ��ʼ���������Ƿ�Ϸ�      */
    
    if (pcTemp2 != pcTemp) {                                            /*  ���������зǷ��ַ�          */
        _ErrorHandle(ERROR_TSHELL_EVAR);
        return  (ERROR_TSHELL_EVAR);
    }
    
    *pcTemp    = PX_EOS;                                                /*  ����������                  */
    pcVarValue = pcTemp + 1;                                            /*  ��ñ�����ֵ�ַ���          */
    
    if ((*pcVarValue == ' ') ||
        (*pcVarValue == '\t')) {                                        /*  �������������ֵ�ִ�        */
        _ErrorHandle(ERROR_TSHELL_EKEYWORD);
        return  (ERROR_TSHELL_EKEYWORD);
    }
    
    __tshellStrGetToken(pcVarValue, &pcTokenNext);                      /*  ��ø�ֵ�ִ�                */
    __tshellStrDelTransChar(pcVarValue, pcVarValue);                    /*  ɾ��ת���ַ�                */
    
    ulError = __tshellVarSet(pcVarName, pcVarValue, 1);                 /*  ���ñ�����ֵ                */
    if (ulError == ERROR_TSHELL_EVAR) {
        ulError = __tshellVarAdd(pcVarName, pcVarValue, 
                                 lib_strlen(pcVarName));                /*  �����±���                  */
    }
    
    return  (ulError);
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
