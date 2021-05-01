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
** ��   ��   ��: loader_vpstat.c
**
** ��   ��   ��: Han.hui (����)
**
** �ļ���������: 2019 �� 06 �� 03 ��
**
** ��        ��: ��������ͳ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../include/loader_lib.h"
#include "../include/loader_symbol.h"
/*********************************************************************************************************
  ���̿��ƿ�
*********************************************************************************************************/
extern LW_LD_VPROC  *_G_pvprocTable[LW_CFG_MAX_THREADS];
/*********************************************************************************************************
** ��������: vprocMemInfo
** ��������: ��ý����ڴ�ʹ�����
** �䡡��  : pid           ���̺�
**           pstStatic     ��̬�ڴ��С
**           pstHeap       �ڴ������
**           pstMmap       mmap ��С
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  vprocMemInfoNoLock (LW_LD_VPROC  *pvproc, size_t  *pstStatic, size_t  *pstHeap, size_t  *pstMmap)
{
    size_t        stStatic = 0;
    size_t        stHeap   = 0;
    size_t        stMmap   = 0;

    INT                 i, iNum;
    ULONG               ulPages;
    BOOL                bStart;
    LW_LIST_RING       *pringTemp;
    LW_LD_EXEC_MODULE  *pmodTemp;
    PVOID               pvVmem[LW_LD_VMEM_MAX];

#if LW_CFG_VMM_EN == 0
    PLW_CLASS_HEAP      pheapVpPatch;
#endif                                                                  /*  LW_CFG_VMM_EN == 0          */

    if (!pvproc) {
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }

    LW_VP_LOCK(pvproc);
    for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
         pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
         pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {

        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);

#if LW_CFG_VMM_EN > 0
        ulPages = 0;
        if (API_VmmPCountInArea(pmodTemp->EMOD_pvBaseAddr, &ulPages) == ERROR_NONE) {
            stStatic += (size_t)(ulPages << LW_CFG_VMM_PAGE_SHIFT);
        }
#else
        stStatic += pmodTemp->EMOD_stLen;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
        stStatic += __moduleSymbolBufferSize(pmodTemp);
    }

    if (stStatic) {                                                     /*  ���ٴ���һ��ģ��            */
        pmodTemp = _LIST_ENTRY(pvproc->VP_ringModules, LW_LD_EXEC_MODULE, EMOD_ringModules);
        ulPages  = 0;

#if LW_CFG_VMM_EN > 0
        iNum = __moduleVpPatchVmem(pmodTemp, pvVmem, LW_LD_VMEM_MAX);
        if (iNum > 0) {
            for (i = 0; i < iNum; i++) {
                if (API_VmmPCountInArea(pvVmem[i],
                                        &ulPages) == ERROR_NONE) {
                    stHeap += (size_t)(ulPages << LW_CFG_VMM_PAGE_SHIFT);
                }
            }
        }
#else
        pheapVpPatch = (PLW_CLASS_HEAP)__moduleVpPatchHeap(pmodTemp);
        if (pheapVpPatch) {                                             /*  ��� vp ����˽�� heap       */
            stHeap += (size_t)(pheapVpPatch->HEAP_stTotalByteSize);
        }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
    }
    LW_VP_UNLOCK(pvproc);

#if LW_CFG_VMM_EN > 0
    API_VmmMmapPCount(pvproc->VP_pid, &stMmap);                         /*  ���� mmap �ڴ�ʵ��������    */
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    if (pstStatic) {
        *pstStatic = stStatic;
    }

    if (pstHeap) {
        *pstHeap = stHeap;
    }

    if (pstMmap) {
        *pstMmap = stMmap;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: vprocMemInfo
** ��������: ��ý����ڴ�ʹ�����
** �䡡��  : pid           ���̺�
**           pstStatic     ��̬�ڴ��С
**           pstHeap       �ڴ������
**           pstMmap       mmap ��С
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  vprocMemInfo (pid_t  pid, size_t  *pstStatic, size_t  *pstHeap, size_t  *pstMmap)
{
    INT           iRet;
    LW_LD_VPROC  *pvproc;

    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (!pvproc) {
        LW_LD_UNLOCK();
        return  (PX_ERROR);
    }

    iRet = vprocMemInfoNoLock(pvproc, pstStatic, pstHeap, pstMmap);
    LW_LD_UNLOCK();

    return  (iRet);
}
/*********************************************************************************************************
** ��������: vprocListGet
** ��������: ��ý����б�
** �䡡��  : pidTable      ���̺ű�
**           uiMaxCnt      �������
** �䡡��  : ��ȡ�Ľ��̸���
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  vprocListGet (pid_t  pidTable[], UINT  uiMaxCnt)
{
    INT  i, iNum = 0;

    if (!pidTable || !uiMaxCnt) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    LW_LD_LOCK();
    for (i = 0; i < LW_CFG_MAX_THREADS; i++) {
        if (_G_pvprocTable[i]) {
            pidTable[iNum] = _G_pvprocTable[i]->VP_pid;
            iNum++;
            if (iNum >= uiMaxCnt) {
                break;
            }
        }
    }
    LW_LD_UNLOCK();

    return  (iNum);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
