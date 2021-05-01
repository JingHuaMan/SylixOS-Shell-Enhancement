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
** ��   ��   ��: RegionShow.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 04 �� 01 ��
**
** ��        ��: ��ʾָ�����ڴ����Ϣ.

** BUG:
2009.04.04  ����ʹ����������Ϣ.
2009.11.13  ������ʾ��ʽ. �� vmm ���.
2014.05.27  �޸� heap �ڴ泬�� 40MB ��ʾ������ϵ�������.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if (LW_CFG_FIO_LIB_EN > 0) && (LW_CFG_REGION_EN > 0) && (LW_CFG_MAX_REGIONS > 0)
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static const CHAR   _G_cHeapInfoHdr[] = "\n\
     HEAP         TOTAL      USED     MAX USED  SEGMENT USED\n\
-------------- ---------- ---------- ---------- ------- ----\n";
/*********************************************************************************************************
** ��������: API_RegionShow
** ��������: ��ʾָ�����ڴ����Ϣ
** �䡡��  : ulId      �ڴ�ؾ�� 0: ��ʾ��ʾ�ں˶Ѻ�ϵͳ����Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
VOID   API_RegionShow (LW_OBJECT_HANDLE  ulId)
{
    CHAR        cRegionName[LW_CFG_OBJECT_NAME_SIZE];

    size_t      stByteSize;
    ULONG       ulSegmentCounter;
    size_t      stUsedByteSize;
    size_t      stMaxUsedByteSize;
    
    printf("heap show >>\n");
    printf(_G_cHeapInfoHdr);                                            /*  ��ӡ��ӭ��Ϣ                */
    
    if (ulId == 0) {
        API_KernelHeapInfo(LW_OPTION_HEAP_KERNEL, 
                           &stByteSize,
                           &ulSegmentCounter,
                           &stUsedByteSize,
                           LW_NULL,
                           &stMaxUsedByteSize);
        
        if (API_KernelHeapInfo(LW_OPTION_HEAP_SYSTEM, LW_NULL, LW_NULL, 
                               LW_NULL, LW_NULL, LW_NULL)) {            /*  ����Ƿ���� system ��      */
            printf("%-14s %8zuKB %8zuKB %8zuKB %7ld %3zd%%\n", "kersys",
                   stByteSize / LW_CFG_KB_SIZE, 
                   stUsedByteSize  / LW_CFG_KB_SIZE, 
                   stMaxUsedByteSize / LW_CFG_KB_SIZE,
                   ulSegmentCounter, (stUsedByteSize / (stByteSize / 100)));
        
        } else {
            printf("%-14s %8zuKB %8zuKB %8zuKB %7ld %3zd%%\n", "kernel",
                   stByteSize / LW_CFG_KB_SIZE, 
                   stUsedByteSize  / LW_CFG_KB_SIZE, 
                   stMaxUsedByteSize / LW_CFG_KB_SIZE,
                   ulSegmentCounter, (stUsedByteSize / (stByteSize / 100)));
                   
            API_KernelHeapInfo(LW_OPTION_HEAP_SYSTEM, 
                               &stByteSize,
                               &ulSegmentCounter,
                               &stUsedByteSize,
                               LW_NULL,
                               &stMaxUsedByteSize);
                               
            printf("%-14s %8zuKB %8zuKB %8zuKB %7ld %3zd%%\n", "system",
                   stByteSize / LW_CFG_KB_SIZE, 
                   stUsedByteSize  / LW_CFG_KB_SIZE, 
                   stMaxUsedByteSize / LW_CFG_KB_SIZE,
                   ulSegmentCounter, (stUsedByteSize / (stByteSize / 100)));
        }
    
    } else {
        if (API_RegionGetName(ulId, cRegionName)) {
            return;
        }
        if (API_RegionStatus(ulId, 
                             &stByteSize,
                             &ulSegmentCounter,
                             &stUsedByteSize,
                             LW_NULL,
                             &stMaxUsedByteSize)) {
            return;
        }
        printf("%-14s %8zuKB %8zuKB %8zuKB %7ld %3zd%%\n", cRegionName,
               stByteSize / LW_CFG_KB_SIZE, 
               stUsedByteSize  / LW_CFG_KB_SIZE, 
               stMaxUsedByteSize / LW_CFG_KB_SIZE,
               ulSegmentCounter, (stUsedByteSize / (stByteSize / 100)));
    }
    
    printf("\n");
}

#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
                                                                        /*  LW_CFG_REGION_EN > 0        */
                                                                        /*  LW_CFG_MAX_REGIONS > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
