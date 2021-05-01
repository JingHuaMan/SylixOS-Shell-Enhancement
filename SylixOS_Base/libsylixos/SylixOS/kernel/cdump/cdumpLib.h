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
** ��   ��   ��: cdumpLib.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 12 �� 01 ��
**
** ��        ��: ϵͳ/Ӧ�ñ�����Ϣ��¼.
*********************************************************************************************************/

#ifndef __CDUMP_LIB_H
#define __CDUMP_LIB_H

/*********************************************************************************************************
  �궨��
*********************************************************************************************************/

#define LW_CDUMP_MAGIC_0    0xab
#define LW_CDUMP_MAGIC_1    0x56
#define LW_CDUMP_MAGIC_2    0xef
#define LW_CDUMP_MAGIC_3    0x33
#define LW_CDUMP_MAGIC_LEN  4

/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/

VOID  _CrashDumpAbortStkOf(addr_t  ulRetAddr, addr_t  ulAbortAddr, CPCHAR  pcInfo, PLW_CLASS_TCB  ptcb);
VOID  _CrashDumpAbortFatal(addr_t  ulRetAddr, addr_t  ulAbortAddr, CPCHAR  pcInfo);
VOID  _CrashDumpAbortKernel(LW_OBJECT_HANDLE   ulOwner, 
                            CPCHAR             pcKernelFunc, 
                            PVOID              pvCtx,
                            CPCHAR             pcInfo, 
                            CPCHAR             pcTail);
VOID  _CrashDumpAbortAccess(PVOID  pcCtx, CPCHAR  pcInfo);
                            
VOID  _CrashDumpSet(PVOID  pvCdump, size_t  stSize);
PVOID _CrashDumpGet(size_t  *pstSize);

#endif                                                                  /*  __CDUMP_LIB_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
