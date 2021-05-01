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
** ��   ��   ��: lib_memcmp.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ��

** BUG:
2013.06.09  memcmp ��Ҫʹ�� unsigned char �Ƚϴ�С.
*********************************************************************************************************/
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: lib_memcmp
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT    lib_memcmp (CPVOID  pvMem1, CPVOID  pvMem2, size_t  stCount)
{
    REGISTER PUCHAR     pucMem1Reg = (PUCHAR)pvMem1;
    REGISTER PUCHAR     pucMem2Reg = (PUCHAR)pvMem2;
    REGISTER INT        i;
    
    for (i = 0; i < stCount; i++) {
        if (*pucMem1Reg != *pucMem2Reg) {
            break;
        }
        pucMem1Reg++;
        pucMem2Reg++;
    }
    if (i >= stCount) {
        return  (0);
    }
    
    if (*pucMem1Reg > *pucMem2Reg) {
        return  (1);
    } else {
        return  (-1);
    }
}
/*********************************************************************************************************
** ��������: lib_bcmp
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT    lib_bcmp (CPVOID  pvMem1, CPVOID  pvMem2, size_t  stCount)
{
    REGISTER PUCHAR      pucMem1Reg = (PUCHAR)pvMem1;
    REGISTER PUCHAR      pucMem2Reg = (PUCHAR)pvMem2;
    REGISTER INT        i;
    
    for (i = 0; i < stCount; i++) {
        if (*pucMem1Reg != *pucMem2Reg) {
            break;
        }
        pucMem1Reg++;
        pucMem2Reg++;
    }
    if (i >= stCount) {
        return  (0);
    }
    
    if (*pucMem1Reg > *pucMem2Reg) {
        return  (1);
    } else {
        return  (-1);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
