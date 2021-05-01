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
** ��   ��   ��: PartitionShow.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 04 �� 01 ��
**
** ��        ��: ��ʾָ�����ڴ������Ϣ.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_FIO_LIB_EN > 0
#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)
/*********************************************************************************************************
** ��������: API_PartitionShow
** ��������: ��ʾָ�����ڴ������Ϣ
** �䡡��  : ulId      �ڴ�������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
VOID   API_PartitionShow (LW_OBJECT_HANDLE  ulId)
{
    CHAR        cPartitionName[LW_CFG_OBJECT_NAME_SIZE];
    
    ULONG       ulBlockCounter;
    ULONG       ulFreeBlockCounter;
    size_t      stBlockByteSize;
    
    if (API_PartitionGetName(ulId, cPartitionName)) {
        return;
    }
    
    if (API_PartitionStatus(ulId,
                            &ulBlockCounter,
                            &ulFreeBlockCounter,
                            &stBlockByteSize)) {
        return;
    }
    
    printf("partition show >>\n\n");
    printf("partition name           : %s\n",    cPartitionName);
    printf("partition block number   : %11lu\n", ulBlockCounter);
    printf("partition free block     : %11lu\n", ulFreeBlockCounter);
    printf("partition per block size : %11zd\n", stBlockByteSize);
}

#endif                                                                  /*  LW_CFG_PARTITION_EN > 0     */
                                                                        /*  LW_CFG_MAX_PARTITIONS > 0   */
#endif                                                                  /*  LW_CFG_FIO_LIB_EN           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
