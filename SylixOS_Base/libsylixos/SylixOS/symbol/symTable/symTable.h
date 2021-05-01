/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: symTable.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 02 �� 26 ��
**
** ��        ��: ϵͳ���ű������ (Ϊ����װ�����ṩ����).
** ע        ��: ������ϵͳȫ�ַ��ű����.
*********************************************************************************************************/

#ifndef __SYMTABLE_H
#define __SYMTABLE_H

/*********************************************************************************************************
  symbol flag
*********************************************************************************************************/
#define LW_SYMBOL_FLAG_STATIC           0x80000000                      /*  ����ɾ���ľ�̬����          */
#define LW_SYMBOL_FLAG_WEAK             0x40000000                      /*  ������                      */

#define LW_SYMBOL_IS_STATIC(flag)       ((flag) & LW_SYMBOL_FLAG_STATIC)
#define LW_SYMBOL_IS_WEAK(flag)         ((flag) & LW_SYMBOL_FLAG_WEAK)

#define LW_SYMBOL_FLAG_REN              0x00000001                      /*  �ɶ�����                    */
#define LW_SYMBOL_FLAG_WEN              0x00000002                      /*  ��д����                    */
#define LW_SYMBOL_FLAG_XEN              0x00000004                      /*  ��ִ�з���                  */

#define LW_SYMBOL_IS_REN(flag)          ((flag) & LW_SYMBOL_FLAG_REN)
#define LW_SYMBOL_IS_WEN(flag)          ((flag) & LW_SYMBOL_FLAG_WEN)
#define LW_SYMBOL_IS_XEN(flag)          ((flag) & LW_SYMBOL_FLAG_XEN)
/*********************************************************************************************************
  symbol 
*********************************************************************************************************/
typedef struct lw_symbol {
    LW_LIST_LINE         SYM_lineManage;                                /*  ��������                    */
    PCHAR                SYM_pcName;                                    /*  ������                      */
    caddr_t              SYM_pcAddr;                                    /*  ��Ӧ���ڴ��ַ              */
    INT                  SYM_iFlag;                                     /*  flag                        */
} LW_SYMBOL;
typedef LW_SYMBOL       *PLW_SYMBOL;
/*********************************************************************************************************
  symbol api
  delete ��������ʹ��, �������������, �û�������ʹ�ûص������е� PLW_SYMBOL �ṹ.
*********************************************************************************************************/
LW_API VOID         API_SymbolInit(VOID);
LW_API INT          API_SymbolAddStatic(PLW_SYMBOL  psymbol, INT  iNum);/*  ��̬���ű�װ              */
LW_API INT          API_SymbolAdd(CPCHAR  pcName, caddr_t  pcAddr, INT  iFlag);
LW_API INT          API_SymbolDelete(CPCHAR  pcName, INT  iFlag);
LW_API PVOID        API_SymbolFind(CPCHAR  pcName, INT  iFlag);
LW_API VOID         API_SymbolTraverse(BOOL (*pfuncCb)(PVOID, PLW_SYMBOL), PVOID  pvArg);

#define symbolInit          API_SymbolInit
#define symbolAddStatic     API_SymbolAddStatic
#define symbolAdd           API_SymbolAdd
#define symbolDelete        API_SymbolDelete
#define symbolFind          API_SymbolFind
#define symbolTraverse      API_SymbolTraverse

/*********************************************************************************************************
  API_SymbolAddStatic ˵��: (һ��ֻ����ϵͳʹ��ģ��װ����ʱ, ��ʹ�ô˹���!)
  
  �˺������� BSP ��ʼ��ʱʹ��! 
  
  API_SymbolAddStatic �� sylixos �������߲�����Ӧ�Ĵ������, ��������� doc/SYMBOL �ļ�.
*********************************************************************************************************/

#endif                                                                  /*  __SYMTABLE_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
