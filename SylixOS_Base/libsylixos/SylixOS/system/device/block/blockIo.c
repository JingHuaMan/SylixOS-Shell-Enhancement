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
** ��   ��   ��: blockIo.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 09 �� 26 ��
**
** ��        ��: ϵͳ���豸�ײ�ӿ�.

** BUG:
2009.03.21  �߼��豸û������λ, FIOUNMOUNT & ��Դ���� & ����������������.
            ����д�������ش���ʱ, ͬʱ�豸Ϊ���ƶ��豸, ��ʱϵͳ�����²㷢�� FIOCANCLE ����, DISK CACHE 
            �յ�������Ὣ CACHE ����Ϊ��λ״̬, ��ֹ����������豸.
2009.03.27  �� O_RDONLY ���ж��д���.
2009.04.17  LW_BLKD_GET_SECSIZE �ڲ�ʹ�� WORD ����(16bit).
2009.07.15  LW_BLKD_GET_SECSIZE ioctl ʱҲҪע�� WORD ����.
2009.10.28  ��������ʹ�� atomic ԭ��������.
2009.11.03  ���ƶ��豸һ����������ý�ʸı�. �����������д. �ȴ����´���.
2010.03.09  ���дʱ������ִ���, ��Ҫ��ӡ _DebugHandle();
2010.05.05  �����������豸ʱ�Բ����������ж�. ���豸��λ�ͻ�ȡ״̬�����Ǳ���ķ���.
2015.03.24  __blockIoDevGet() ����Ҫ�ź�������, �������ϲ��ļ�ϵͳ�е�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static PLW_BLK_DEV          _G_pblkdBLockIoTbl[LW_CFG_MAX_VOLUMES];
static LW_OBJECT_HANDLE     _G_hBLockIoTblLock;
/*********************************************************************************************************
  �����
*********************************************************************************************************/
#define __BLOCKIO_DEV_LOCK()        API_SemaphoreBPend(_G_hBLockIoTblLock, LW_OPTION_WAIT_INFINITE)
#define __BLOCKIO_DEV_UNLOCK()      API_SemaphoreBPost(_G_hBLockIoTblLock);
/*********************************************************************************************************
** ��������: __blockIoDevInit
** ��������: ��ʼ�����豸������
** �䡡��  : VOID
** �䡡��  : VOID
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __blockIoDevInit (VOID)
{
    REGISTER INT        i;
    
    for (i = 0; i < LW_CFG_MAX_VOLUMES; i++) {
        _G_pblkdBLockIoTbl[i] = LW_NULL;
    }
    
    _G_hBLockIoTblLock = API_SemaphoreBCreate("blk_lock", LW_TRUE, LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (!_G_hBLockIoTblLock) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "couldn't create block io lock.\r\n");
    }
}
/*********************************************************************************************************
** ��������: __blockIoDevCreate
** ��������: �������㴴��һ�����豸����
** �䡡��  : pblkdNew      �µĿ��豸
** �䡡��  : -1:           ���豸����
**           -2:           �ռ�����
**           ����:         �豸������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __blockIoDevCreate (PLW_BLK_DEV  pblkdNew)
{
    REGISTER INT        i;

    if (!pblkdNew) {
        return  (PX_ERROR);
    }
    
    if ((pblkdNew->BLKD_pfuncBlkRd    == LW_NULL) ||
        (pblkdNew->BLKD_pfuncBlkIoctl == LW_NULL)) {                    /*  ����ȱ�ٻ�����������        */
        return  (PX_ERROR);
    }
    
    if (((pblkdNew->BLKD_iFlag & O_WRONLY) ||
         (pblkdNew->BLKD_iFlag & O_RDWR)) &&
         (pblkdNew->BLKD_pfuncBlkWrt == LW_NULL)) {                     /*  ��д���ʲ���ȱ��д����      */
        return  (PX_ERROR);
    }
    
    if (pblkdNew->BLKD_iRetry < 1) {
        return  (PX_ERROR);
    }
    
    __BLOCKIO_DEV_LOCK();
    for (i = 0; i < LW_CFG_MAX_VOLUMES; i++) {
        if (_G_pblkdBLockIoTbl[i] == LW_NULL) {
            _G_pblkdBLockIoTbl[i] =  pblkdNew;
            break;
        }
    }
    __BLOCKIO_DEV_UNLOCK();
    
    if (i >= LW_CFG_MAX_VOLUMES) {
        return  (-2);
    
    } else {
        return  (i);
    }
}
/*********************************************************************************************************
** ��������: __blockIoDevDelete
** ��������: ��������ɾ��һ�����豸����
** �䡡��  : iIndex        ���豸
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __blockIoDevDelete (INT  iIndex)
{
    if ((iIndex >= LW_CFG_MAX_VOLUMES) ||
        (iIndex <  0)) {
        return;
    }
    
    __BLOCKIO_DEV_LOCK();
    _G_pblkdBLockIoTbl[iIndex] = LW_NULL;
    __BLOCKIO_DEV_UNLOCK();
}
/*********************************************************************************************************
** ��������: __blockIoDevGet
** ��������: ͨ����������������ȡһ�������豸���ƿ�
** �䡡��  : iIndex        ���豸
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_BLK_DEV  __blockIoDevGet (INT  iIndex)
{
    REGISTER PLW_BLK_DEV    pblkd;

    if ((iIndex >= LW_CFG_MAX_VOLUMES) ||
        (iIndex <  0)) {
        return  (LW_NULL);
    }
    
    pblkd = _G_pblkdBLockIoTbl[iIndex];
    
    return  (pblkd);
}
/*********************************************************************************************************
** ��������: __blockIoDevRead
** ��������: ���������ȡһ�����豸
** �䡡��  : iIndex        ���豸
**           pvBuffer      ������
**           ulStartSector  ��ʼ����
**           ulSectorCount  ��������
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __blockIoDevRead (INT     iIndex, 
                       VOID   *pvBuffer, 
                       ULONG   ulStartSector, 
                       ULONG   ulSectorCount)
{
    REGISTER PLW_BLK_DEV  pblkd = _G_pblkdBLockIoTbl[iIndex];
    REGISTER INT          i = 0;
    
    if (pblkd->BLKD_iFlag & O_WRONLY) {
        return  (PX_ERROR);
    }
    
    if (pblkd->BLKD_bRemovable && pblkd->BLKD_bDiskChange) {            /*  �豸�ı䲻�ɲ���            */
        return  (PX_ERROR);
    }
    
    if (pblkd->BLKD_pfuncBlkRd(pblkd,
                               pvBuffer, 
                               ulStartSector, 
                               ulSectorCount) < 0) {
                               
        for (i = 0; i < pblkd->BLKD_iRetry; i++) {
            if (pblkd->BLKD_bRemovable) {
                if (pblkd->BLKD_pfuncBlkStatusChk) {
                    if (pblkd->BLKD_pfuncBlkStatusChk(pblkd) != ERROR_NONE) {
                        continue;                                       /*  �豸״̬����, ���¼��      */
                    }
                }
            }
            if (pblkd->BLKD_pfuncBlkRd(pblkd,
                                       pvBuffer, 
                                       ulStartSector, 
                                       ulSectorCount) >= 0) {
                break;
            }
        }
    }
    
    if (i >= pblkd->BLKD_iRetry) {
        if (pblkd->BLKD_bRemovable) {
            pblkd->BLKD_pfuncBlkIoctl(pblkd, FIOCANCEL, 0);
        }
        _DebugFormat(__ERRORMESSAGE_LEVEL, "can not read block: blk %d sector %lu [%ld].\r\n",
                     iIndex, ulStartSector, ulSectorCount);
        return  (PX_ERROR);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: __blockIoDevWrite
** ��������: ��������д��һ�����豸
** �䡡��  : iIndex        ���豸
**           pvBuffer      ������
**           ulStartSector  ��ʼ����
**           ulSectorCount  ��������
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __blockIoDevWrite (INT     iIndex, 
                        VOID   *pvBuffer, 
                        ULONG   ulStartSector, 
                        ULONG   ulSectorCount)
{
    REGISTER PLW_BLK_DEV  pblkd = _G_pblkdBLockIoTbl[iIndex];
    REGISTER INT          i = 0;
    
    if (pblkd->BLKD_iFlag == O_RDONLY) {
        return  (PX_ERROR);
    }
    
    if (pblkd->BLKD_bRemovable && pblkd->BLKD_bDiskChange) {            /*  �豸�ı䲻�ɲ���            */
        return  (PX_ERROR);
    }
    
    if (pblkd->BLKD_pfuncBlkWrt(pblkd,
                                pvBuffer, 
                                ulStartSector, 
                                ulSectorCount) < 0) {
                               
        for (i = 0; i < pblkd->BLKD_iRetry; i++) {
            if (pblkd->BLKD_bRemovable) {
                if (pblkd->BLKD_pfuncBlkStatusChk) {
                    if (pblkd->BLKD_pfuncBlkStatusChk(pblkd) != ERROR_NONE) {
                        continue;                                       /*  �豸״̬����, ���¼��      */
                    }
                }
            }
            if (pblkd->BLKD_pfuncBlkWrt(pblkd,
                                        pvBuffer, 
                                        ulStartSector, 
                                        ulSectorCount) >= 0) {
                break;
            }
        }
    }
    
    if (i >= pblkd->BLKD_iRetry) {
        if (pblkd->BLKD_bRemovable) {
            pblkd->BLKD_pfuncBlkIoctl(pblkd, FIOCANCEL, 0);
        }
        _DebugFormat(__ERRORMESSAGE_LEVEL, "can not write block: blk %d sector %lu [%ld].\r\n",
                     iIndex, ulStartSector, ulSectorCount);
        return  (PX_ERROR);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: __blockIoDevIoctl
** ��������: ���豸ִ��ָ������
** �䡡��  : iIndex        ���豸
**           pvBuffer      ������
**           iStartSector  ��ʼ����
**           iSectorCount  ��������
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __blockIoDevIoctl (INT  iIndex, INT  iCmd, LONG  lArg)
{
             INTREG       iregInterLevel;
    REGISTER PLW_BLK_DEV  pblkd = _G_pblkdBLockIoTbl[iIndex];
             PLW_BLK_DEV  pblkdPhy;                                     /*  �����豸                    */
             
    switch (iCmd) {
    
    case FIOUNMOUNT:                                                    /*  ж�ش���                    */
        if (pblkd->BLKD_pvLink) {
            pblkdPhy = (PLW_BLK_DEV)pblkd->BLKD_pvLink;
            __LW_ATOMIC_LOCK(iregInterLevel);
            if (pblkdPhy->BLKD_uiLinkCounter == 1) {                    /*  �����豸����ǰһ�ι���      */
                pblkdPhy->BLKD_uiInitCounter  = 0;                      /*  ���������                  */
                pblkdPhy->BLKD_uiPowerCounter = 0;
                __LW_ATOMIC_UNLOCK(iregInterLevel);
                break;
            } else {
                __LW_ATOMIC_UNLOCK(iregInterLevel);
            }
        } else {                                                        /*  ���豸Ϊ�����豸            */
            break;
        }
        return  (ERROR_NONE);                                           /*  ��ι����߼��豸����˲���  */
        
    case FIODISKINIT:
        if (pblkd->BLKD_pvLink) {
            pblkdPhy = (PLW_BLK_DEV)pblkd->BLKD_pvLink;
            __LW_ATOMIC_LOCK(iregInterLevel);
            pblkdPhy->BLKD_uiInitCounter++;                             /*  ���ӳ�ʼ������              */
            if (pblkdPhy->BLKD_uiInitCounter == 1) {                    /*  �����豸û�г�ʼ��          */
                __LW_ATOMIC_UNLOCK(iregInterLevel);
                break;
            } else {
                __LW_ATOMIC_UNLOCK(iregInterLevel);
            }
        } else {                                                        /*  ���豸Ϊ�����豸            */
            break;
        }
        return  (ERROR_NONE);                                           /*  ��γ�ʼ���߼��豸����˲���*/
    
    case LW_BLKD_GET_SECNUM:
        if (pblkd->BLKD_ulNSector > 0) {
            *(ULONG *)lArg = pblkd->BLKD_ulNSector;                     /*  ULONG ����                  */
            return  (ERROR_NONE);
        }
        break;
        
    case LW_BLKD_GET_SECSIZE:
        if (pblkd->BLKD_ulBytesPerSector > 0) {
            *(ULONG *)lArg = pblkd->BLKD_ulBytesPerSector;              /*  ULONG ����                  */
            return  (ERROR_NONE);
        }
        break;
        
    case LW_BLKD_GET_BLKSIZE:
        if (pblkd->BLKD_ulBytesPerBlock > 0) {
            *(ULONG *)lArg = pblkd->BLKD_ulBytesPerBlock;               /*  ULONG ����                  */
            return  (ERROR_NONE);
        }
        break;
        
    case LW_BLKD_CTRL_POWER:
        if (lArg == LW_BLKD_POWER_ON) {                                 /*  �򿪵�Դ                    */
            if (pblkd->BLKD_pvLink) {
                pblkdPhy = (PLW_BLK_DEV)pblkd->BLKD_pvLink;
                __LW_ATOMIC_LOCK(iregInterLevel);
                pblkdPhy->BLKD_uiPowerCounter++;                        /*  ���ӵ�Դ�򿪴���            */
                if (pblkdPhy->BLKD_uiPowerCounter == 1) {
                    __LW_ATOMIC_UNLOCK(iregInterLevel);
                    break;
                } else {
                    __LW_ATOMIC_UNLOCK(iregInterLevel);
                }
            } else {                                                    /*  �����豸ֱ�Ӵ�            */
                break;
            }
        } else {                                                        /*  �Ͽ���Դ                    */
            if (pblkd->BLKD_pvLink) {
                pblkdPhy = (PLW_BLK_DEV)pblkd->BLKD_pvLink;
                if (pblkdPhy->BLKD_uiPowerCounter) {
                    __LW_ATOMIC_LOCK(iregInterLevel);
                    pblkdPhy->BLKD_uiPowerCounter--;                    /*  ���ٵ�Դ�򿪴���            */
                    if (pblkdPhy->BLKD_uiPowerCounter == 0) {
                        __LW_ATOMIC_UNLOCK(iregInterLevel);
                        break;
                    } else {
                        __LW_ATOMIC_UNLOCK(iregInterLevel);
                    }
                }
            } else {
                break;
            }
        }
        return  (ERROR_NONE);                                           /*  ����˲���                  */
    
    case LW_BLKD_CTRL_EJECT:
        if (pblkd->BLKD_bRemovable == LW_TRUE) {                        /*  �����Ƴ�                    */
            if (pblkd->BLKD_pvLink) {
                pblkdPhy = (PLW_BLK_DEV)pblkd->BLKD_pvLink;
                if (pblkdPhy->BLKD_uiLinkCounter == 1) {                /*  �����豸����ǰһ�ι���      */
                    break;
                }
            } else {
                break;
            }
        }
        return  (ERROR_NONE);                                           /*  ����˲���                  */
    }
    
    return  (pblkd->BLKD_pfuncBlkIoctl(pblkd, iCmd, lArg));
}
/*********************************************************************************************************
** ��������: __blockIoDevReset
** ��������: ���豸��λ
** �䡡��  : iIndex        ���豸
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __blockIoDevReset (INT     iIndex)
{
    REGISTER PLW_BLK_DEV  pblkd = _G_pblkdBLockIoTbl[iIndex];
    REGISTER INT          i = 0;

    if (pblkd->BLKD_iLogic) {                                           /*  �߼��豸��λΪ��            */
        return  (ERROR_NONE);
    }
    
    if (pblkd->BLKD_pfuncBlkReset == LW_NULL) {
        return  (ERROR_NONE);
    }

    if (pblkd->BLKD_pfuncBlkReset(pblkd) < 0) {
        for (i = 0; i < pblkd->BLKD_iRetry; i++) {
            if (pblkd->BLKD_pfuncBlkReset(pblkd) >= 0) {
                break;
            }
        }
    }
    
    if (i >= pblkd->BLKD_iRetry) {
        return  (PX_ERROR);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: __blockIoDevStatus
** ��������: ��ÿ��豸״̬
** �䡡��  : iIndex        ���豸
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __blockIoDevStatus (INT     iIndex)
{
    REGISTER PLW_BLK_DEV  pblkd = _G_pblkdBLockIoTbl[iIndex];
    
    if (pblkd->BLKD_pfuncBlkStatusChk) {
        return  (pblkd->BLKD_pfuncBlkStatusChk(pblkd));
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: __blockIoDevIsLogic
** ��������: ���Կ��豸�Ƿ�Ϊ�߼��豸
** �䡡��  : iIndex        ���豸
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __blockIoDevIsLogic (INT     iIndex)
{
    REGISTER PLW_BLK_DEV  pblkd = _G_pblkdBLockIoTbl[iIndex];
    
    return  (pblkd->BLKD_iLogic);
}
/*********************************************************************************************************
** ��������: __blockIoDevFlag
** ��������: ��ÿ��豸 iFlag ��־λ
** �䡡��  : iIndex        ���豸
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __blockIoDevFlag (INT     iIndex)
{
    REGISTER PLW_BLK_DEV  pblkd = _G_pblkdBLockIoTbl[iIndex];
    
    return  (pblkd->BLKD_iFlag);
}

#endif                                                                  /*  LW_CFG_MAX_VOLUMES          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
