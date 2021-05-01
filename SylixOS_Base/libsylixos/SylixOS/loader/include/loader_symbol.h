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
** ��   ��   ��: loader_symbol.h
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2010 �� 04 �� 17 ��
**
** ��        ��: ���Ź���

** BUG:
2011.02.20  �Ľ����ű����, ����ֲ���ȫ�ֵ�����. (����)
*********************************************************************************************************/

#ifndef __LOADER_SYMBOL_H
#define __LOADER_SYMBOL_H

/*********************************************************************************************************
  ��������
*********************************************************************************************************/
#define LW_LD_SYM_ANY       0                                           /*  �κ����͵ķ���              */
#define LW_LD_SYM_FUNCTION  (LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN)   /*  ��������                    */
#define LW_LD_SYM_DATA      (LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_WEN)   /*  ���ݷ���                    */

#define LW_LD_VER_SYM       "__sylixos_version"                         /*  �汾����                    */
#define LW_LD_DEF_VER       "0.0.0"                                     /*  Ĭ�ϰ汾                    */
#define LW_LD_VERIFY_VER(pcModuleName, pcVersion, ulType)   \
        __moduleVerifyVersion(pcModuleName, pcVersion, ulType)

/*********************************************************************************************************
  �汾�������
*********************************************************************************************************/

INT __moduleVerifyVersion(CPCHAR  pcModuleName, 
                          CPCHAR  pcVersion, 
                          ULONG   ulType);                              /*  ģ��ϵͳ�汾�뵱ǰϵͳƥ��  */

/*********************************************************************************************************
  ���Ų���
*********************************************************************************************************/

INT __moduleFindSym(LW_LD_EXEC_MODULE  *pmodule,
                    CPCHAR              pcSymName, 
                    ULONG              *pulSymVal, 
                    BOOL               *pbWeak, INT iFlag);             /*  ����ģ���ڷ���              */

INT __moduleSymGetValue(LW_LD_EXEC_MODULE  *pmodule,
                        BOOL                bIsWeak,
                        CPCHAR              pcSymName,
                        addr_t             *pulSymVal,
                        INT                 iFlag);                     /*  ��ȫ��, ����, �����������  */

INT __moduleExportSymbol(LW_LD_EXEC_MODULE  *pmodule,
                         CPCHAR              pcSymName,
                         addr_t              ulSymVal,
                         INT                 iFlag,
                         ULONG               ulAllSymCnt,
                         ULONG               ulCurSymNum);              /*  ��������                    */

VOID __moduleTraverseSym(LW_LD_EXEC_MODULE  *pmodule, 
                         BOOL (*pfuncCb)(PVOID, PLW_SYMBOL, LW_LD_EXEC_MODULE *), 
                         PVOID  pvArg);                                 /*  ����ģ���������з���        */

INT __moduleDeleteAllSymbol(LW_LD_EXEC_MODULE *pmodule);
                                                                        /*  ɾ������                    */
                                                                        
size_t __moduleSymbolBufferSize(LW_LD_EXEC_MODULE *pmodule);            /*  ��ȡ���ű����С          */

#endif                                                                  /*  __LOADER_SYMBOL_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
