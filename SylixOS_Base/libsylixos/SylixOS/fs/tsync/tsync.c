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
** ��   ��   ��: tsync.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2020 �� 06 �� 11 ��
**
** ��        ��: �ļ�ϵͳ������д����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && ((LW_CFG_DISKCACHE_EN > 0) || (LW_CFG_YAFFS_EN > 0))
/*********************************************************************************************************
  �ص�����
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE        TSYNC_lineManage;
    VOIDFUNCPTR         TSYNC_pfuncSync;
    PVOID               TSYNC_pvArg;
} LW_TSYNC_NODE;
typedef LW_TSYNC_NODE  *PLW_TSYNC_NODE;
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static PLW_LIST_LINE    _G_plineTSyncHeader = LW_NULL;                  /*  ����ͷ                      */
static BOOL             _G_bTSyncOnce       = LW_FALSE;
/*********************************************************************************************************
  �����߳�����
*********************************************************************************************************/
static LW_CLASS_THREADATTR  _G_threadattrTSyncAttr = {
    LW_NULL,
    LW_CFG_THREAD_DEFAULT_GUARD_SIZE,
    LW_CFG_THREAD_DISKCACHE_STK_SIZE,
    LW_PRIO_T_CACHE,
    LW_CFG_TSYNC_OPTION | LW_OPTION_OBJECT_GLOBAL,                      /*  �ں�ȫ���߳�                */
    LW_NULL,
    LW_NULL,
};
/*********************************************************************************************************
** ��������: __tsyncThread
** ��������: �ļ�ϵͳ������д����
** �䡡��  : pvArg             ��������
** �䡡��  : NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PVOID  __tsyncThread (PVOID  pvArg)
{
    PLW_LIST_LINE   plineCache;
    PLW_TSYNC_NODE  ptsync;

    for (;;) {
        API_TimeSSleep(LW_CFG_TSYNC_PERIOD);                            /*  ������ʱ                    */

        for (plineCache  = _G_plineTSyncHeader;
             plineCache != LW_NULL;
             plineCache  = _list_line_get_next(plineCache)) {

            ptsync = _LIST_ENTRY(plineCache, LW_TSYNC_NODE, TSYNC_lineManage);
            ptsync->TSYNC_pfuncSync(ptsync->TSYNC_pvArg);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __tsyncInit
** ��������: �����ļ�ϵͳ������д����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __tsyncInit (VOID)
{
    LW_OBJECT_HANDLE  ulTSync;

    ulTSync = API_ThreadCreate("t_sync", __tsyncThread, &_G_threadattrTSyncAttr, LW_NULL);
    if (!ulTSync) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "t_sync thread create error.\r\n");
    }
}
/*********************************************************************************************************
** ��������: API_TSyncAdd
** ��������: ��װ�ļ�ϵͳ������д����ص�����
** �䡡��  : pfuncSync         ��д�ص�����
**           pvArg             ��д�ص�����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_TSyncAdd (VOIDFUNCPTR  pfuncSync, PVOID  pvArg)
{
    PLW_TSYNC_NODE  ptsync;

    if (!pfuncSync) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    API_ThreadOnce(&_G_bTSyncOnce, __tsyncInit);                        /*  ���� t_sync ����            */

    ptsync = (PLW_TSYNC_NODE)__SHEAP_ALLOC(sizeof(LW_TSYNC_NODE));
    if (!ptsync) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    ptsync->TSYNC_pfuncSync = pfuncSync;
    ptsync->TSYNC_pvArg     = pvArg;

    __KERNEL_ENTER();
    _List_Line_Add_Ahead(&ptsync->TSYNC_lineManage, &_G_plineTSyncHeader);
    __KERNEL_EXIT();

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKCACHE_EN > 0) ||*/
                                                                        /*  (LW_CFG_YAFFS_EN > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
