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
** ��   ��   ��: ttinyVarLib.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 07 �� 27 ��
**
** ��        ��: һ����С�͵� shell ϵͳ�ı����������ڲ�ͷ�ļ�.
*********************************************************************************************************/

#ifndef __TTINYVARLIB_H
#define __TTINYVARLIB_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

/*********************************************************************************************************
  VAR �ṹ
  ע�� : SV_ulRefCnt ���ü�������ͨ�� __tshellVarGet() ֱ�ӻ�� SV_pcVarValue �Ĵ���, 
         SV_ulRefCnt ��Ϊ��, �����ô˻�������ʱ���ͷ��ϵĻ����� (�м��м�!)
*********************************************************************************************************/

typedef struct {
    LW_LIST_LINE             SV_lineManage;                             /*  ������˫����                */
    LW_LIST_LINE             SV_lineHash;                               /*  ��ϣ��������                */

    PCHAR                    SV_pcVarName;                              /*  ������                      */
    PCHAR                    SV_pcVarValue;                             /*  ������ֵ                    */
    
    ULONG                    SV_ulRefCnt;                               /*  ���ü���                    */
} __TSHELL_VAR;
typedef __TSHELL_VAR        *__PTSHELL_VAR;                             /*  ָ������                    */

/*********************************************************************************************************
  �ڲ���������
*********************************************************************************************************/

ULONG  __tshellVarAdd(CPCHAR  pcVarName, CPCHAR  pcVarValue, size_t  stNameStrLen);
ULONG  __tshellVarDelete(__PTSHELL_VAR  pskvNode);
ULONG  __tshellVarDeleteByName(CPCHAR  pcVarName);
ULONG  __tshellVarFind(char  *pcVarName, __PTSHELL_VAR   *ppskvNode);
ULONG  __tshellVarList(__PTSHELL_VAR   pskvNodeStart,
                       __PTSHELL_VAR   ppskvNode[],
                       INT             iMaxCounter);
INT    __tshellVarGetRt(CPCHAR  pcVarName, 
                        PCHAR   pcVarValue,
                        INT     iMaxLen);
ULONG  __tshellVarGet(CPCHAR  pcVarName, PCHAR  *ppcVarValue);
ULONG  __tshellVarSet(CPCHAR  pcVarName, CPCHAR       pcVarValue, INT  iIsOverwrite);
ULONG  __tshellVarDefine(PCHAR  pcCmd);

INT    __tshellVarSave(CPCHAR  pcFileName);
INT    __tshellVarLoad(CPCHAR  pcFileName);

/*********************************************************************************************************
  ���̲���ʹ�����к�����ʼ������������
*********************************************************************************************************/

INT    __tshellVarNum(VOID);
INT    __tshellVarDup(PVOID (*pfuncMalloc)(size_t stSize), PCHAR  ppcEvn[], ULONG  ulMax);

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
#endif                                                                  /*  __TTINYVARLIB_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
