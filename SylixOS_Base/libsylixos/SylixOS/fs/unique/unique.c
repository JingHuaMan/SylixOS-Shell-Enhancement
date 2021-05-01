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
** ��   ��   ��: unique.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 01 �� 08 ��
**
** ��        ��: Ϊû�� inode ���кŵ��ļ�ϵͳ�ṩ���к�֧��, ���� FAT SMB ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_FATFS_EN > 0)
#include "unique.h"
/*********************************************************************************************************
  inode unique pool
*********************************************************************************************************/
#define UNIQ_INO_IS_BUSY(index, arr)    \
        (((arr)[((index) >> 3)] >> ((index) & (8 - 1))) & 0x01)
#define SET_UNIQ_INO_BUSY(index, arr)   \
        ((arr)[((index) >> 3)] |= (0x01 << ((index) & (8 - 1))))
#define SET_UNIQ_INO_FREE(index, arr)   \
        ((arr)[((index) >> 3)] &= (~(0x01 << ((index) & (8 - 1)))))
/*********************************************************************************************************
** ��������: API_FsUniqueCreate
** ��������: ����һ�� unique �ķ�����
** �䡡��  : stSize        unique �ش�С
**           ulResvNo      �������ֵ����ֵ
** �䡡��  : LW_UNIQUE_POOL �ṹָ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
PLW_UNIQUE_POOL  API_FsUniqueCreate (size_t  stSize, ULONG  ulResvNo)
{
    PLW_UNIQUE_POOL     punip = (PLW_UNIQUE_POOL)__SHEAP_ALLOC(sizeof(LW_UNIQUE_POOL));
    
    if (punip == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    
    punip->UNIP_pcArray = (PCHAR)__SHEAP_ALLOC(stSize);
    if (punip->UNIP_pcArray == LW_NULL) {
        __SHEAP_FREE(punip);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    lib_bzero(punip->UNIP_pcArray, stSize);
    
    if (ulResvNo == 0) {
        ulResvNo =  1;
    }
    
    punip->UNIP_ulIndex  = 0;
    punip->UNIP_stSize   = stSize;
    punip->UNIP_ulResvNo = ulResvNo;
    
    return  (punip);
}
/*********************************************************************************************************
** ��������: API_FsUniqueDelete
** ��������: ɾ��һ�� unique �ķ�����
** �䡡��  : punip         UNIQUE_POOL �ṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
VOID  API_FsUniqueDelete (PLW_UNIQUE_POOL  punip)
{
    if (punip) {
        if (punip->UNIP_pcArray) {
            __SHEAP_FREE(punip->UNIP_pcArray);
        }
        __SHEAP_FREE(punip);
    }
}
/*********************************************************************************************************
** ��������: API_FsUniqueAlloc
** ��������: ����һ�� unique ��
** �䡡��  : punip         UNIQUE_POOL �ṹ
** �䡡��  : ����� unique ��
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ﲻ��ʹ�� realloc ����, ��� realloc ʧ�ܻᶪʧ��ǰ����� inode
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_FsUniqueAlloc (PLW_UNIQUE_POOL  punip)
{
    INT     i;
    BOOL    bGet = LW_FALSE;
    PCHAR   pcNewArray;
    size_t  stNewSize;
    size_t  stBitSize = punip->UNIP_stSize << 3;
    
    while (!bGet) {
        for (i = 0; i < stBitSize; i++) {
            if (!UNIQ_INO_IS_BUSY(punip->UNIP_ulIndex, punip->UNIP_pcArray)) {
                SET_UNIQ_INO_BUSY(punip->UNIP_ulIndex, punip->UNIP_pcArray);
                return (punip->UNIP_ulResvNo + punip->UNIP_ulIndex);
            }
            punip->UNIP_ulIndex++;
            if (punip->UNIP_ulIndex >= stBitSize) {
                punip->UNIP_ulIndex = 0;
            }
        }
        
        if ((punip->UNIP_stSize << 1) < (0x0FFFFFFF - punip->UNIP_ulResvNo)) {
            stNewSize  = punip->UNIP_stSize << 1;
            pcNewArray = (PCHAR)__SHEAP_ALLOC(stNewSize);               /*  �������� unique ������    */
            if (pcNewArray == LW_NULL) {
                bGet = LW_TRUE;
            
            } else {
                lib_memcpy(pcNewArray, punip->UNIP_pcArray, punip->UNIP_stSize);
                __SHEAP_FREE(punip->UNIP_pcArray);
                punip->UNIP_pcArray = pcNewArray;
                punip->UNIP_stSize  = stNewSize;
                stBitSize = stNewSize << 3;
            }
        } else {
            bGet = LW_TRUE;
        }
    }
    
    return  (0);                                                        /*  ����ʧ��                    */
}
/*********************************************************************************************************
** ��������: API_FsUniqueFree
** ��������: ����һ�� unique ��
** �䡡��  : punip         UNIQUE_POOL �ṹ
**           ulNo          inode ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
VOID  API_FsUniqueFree (PLW_UNIQUE_POOL  punip, ULONG  ulNo)
{
    SET_UNIQ_INO_FREE((ulNo - punip->UNIP_ulResvNo), punip->UNIP_pcArray);
}
/*********************************************************************************************************
** ��������: API_FsUniqueIsVal
** ��������: �ж�һ�� unique �ŷ�Χ�Ƿ�Ϸ�
** �䡡��  : punip         UNIQUE_POOL �ṹ
**           ulNo          inode ��
** �䡡��  : �Ƿ�Ϸ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
BOOL  API_FsUniqueIsVal (PLW_UNIQUE_POOL  punip, ULONG  ulNo)
{
    return  (ulNo >= punip->UNIP_ulResvNo);
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_FATFS_EN > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
