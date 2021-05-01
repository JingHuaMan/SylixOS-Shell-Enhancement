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
** ��   ��   ��: diskCacheSync.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 03 �� 23 ��
**
** ��        ��: ���̸��ٻ��������������д����.
**
** BUG:
2016.12.21  ���ļ�ϵͳ��д�߳����.
2020.06.11  ʹ��ͳһ��д����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "diskCacheLib.h"
#include "diskCache.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKCACHE_EN > 0)
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
extern INT  __diskCacheIoctl(PLW_DISKCACHE_CB   pdiskcDiskCache, INT  iCmd, LONG  lArg);
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
LW_OBJECT_HANDLE        _G_ulDiskCacheListLock  = 0ul;                  /*  ������                      */
PLW_LIST_LINE           _G_plineDiskCacheHeader = LW_NULL;              /*  ����ͷ                      */
/*********************************************************************************************************
  ������
*********************************************************************************************************/
#define __LW_DISKCACHE_LIST_LOCK()      \
        API_SemaphoreMPend(_G_ulDiskCacheListLock, LW_OPTION_WAIT_INFINITE)
#define __LW_DISKCACHE_LIST_UNLOCK()    \
        API_SemaphoreMPost(_G_ulDiskCacheListLock)
/*********************************************************************************************************
** ��������: __diskCacheSync
** ��������: ���̸��ٻ��������������д����
** �䡡��  : pvArg             ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __diskCacheSync (PVOID  pvArg)
{
    PLW_DISKCACHE_CB   pdiskcDiskCache;
    PLW_LIST_LINE      plineCache;
    ULONG              ulNSector = LW_CFG_DISKCACHE_MINSECTOR;
    
    (VOID)pvArg;
    
    __LW_DISKCACHE_LIST_LOCK();
    for (plineCache  = _G_plineDiskCacheHeader;
         plineCache != LW_NULL;
         plineCache  = _list_line_get_next(plineCache)) {               /*  �������д��̻���            */

        pdiskcDiskCache = _LIST_ENTRY(plineCache,
                                      LW_DISKCACHE_CB,
                                      DISKC_lineManage);                /*  ��д����                    */
        if (pdiskcDiskCache->DISKC_ulDirtyCounter) {
            __diskCacheIoctl(pdiskcDiskCache,
                             LW_BLKD_DISKCACHE_RAMFLUSH, ulNSector);
        }
    }
    __LW_DISKCACHE_LIST_UNLOCK();
}
/*********************************************************************************************************
** ��������: __diskCacheListAdd
** ��������: �� DISK CACHE ������д��
** �䡡��  : pdiskcDiskCache    ���� CACHE ���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __diskCacheListAdd (PLW_DISKCACHE_CB   pdiskcDiskCache)
{
    __LW_DISKCACHE_LIST_LOCK();
    _List_Line_Add_Ahead(&pdiskcDiskCache->DISKC_lineManage, &_G_plineDiskCacheHeader);
    __LW_DISKCACHE_LIST_UNLOCK();
}
/*********************************************************************************************************
** ��������: __diskCacheListDel
** ��������: �ӻ�д���н� DISK CACHE �Ƴ�
** �䡡��  : pdiskcDiskCache    ���� CACHE ���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __diskCacheListDel (PLW_DISKCACHE_CB   pdiskcDiskCache)
{
    __LW_DISKCACHE_LIST_LOCK();
    _List_Line_Del(&pdiskcDiskCache->DISKC_lineManage, &_G_plineDiskCacheHeader);
    __LW_DISKCACHE_LIST_UNLOCK();
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKCACHE_EN > 0)   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
