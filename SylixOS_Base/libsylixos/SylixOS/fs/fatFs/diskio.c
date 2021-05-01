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
** ��   ��   ��: diskio.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 09 �� 26 ��
**
** ��        ��: FAT �ļ�ϵͳ�� BLOCK �豸�ӿ�.
**
** BUG:
2014.12.31  ֧�� ff10c �ӿ�.
2015.03.24  ������ƻ�ȡ����״̬�ӿ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "diskio.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
PLW_BLK_DEV     __blockIoDevGet(INT  iIndex);
INT             __blockIoDevRead(INT     iIndex, 
                                 VOID   *pvBuffer, 
                                 ULONG   ulStartSector, 
                                 ULONG   ulSectorCount);
INT             __blockIoDevWrite(INT     iIndex, 
                                  VOID   *pvBuffer, 
                                  ULONG   ulStartSector, 
                                  ULONG   ulSectorCount);
INT             __blockIoDevReset(INT     iIndex);
INT             __blockIoDevStatus(INT     iIndex);
INT             __blockIoDevIoctl(INT  iIndex, INT  iCmd, LONG  lArg);
/*********************************************************************************************************
** ��������: disk_initialize
** ��������: ��ʼ�����豸
** �䡡��  : ucDriver      �����
** �䡡��  : VOID
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
DSTATUS  disk_initialize (BYTE  ucDriver)
{
    REGISTER INT    iError = __blockIoDevIoctl((INT)ucDriver, FIODISKINIT, 0);
    
    if (iError < 0) {
        return  ((DSTATUS)(STA_NOINIT | STA_NODISK));
    
    } else {
        return  ((DSTATUS)ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: disk_status
** ��������: ��ÿ��豸״̬
** �䡡��  : ucDriver      �����
** �䡡��  : ERROR_NONE ��ʾ���豸��ǰ����, ������ʾ������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
DSTATUS  disk_status (BYTE  ucDriver)
{
    DSTATUS     dstat = 0;
    PLW_BLK_DEV pblk  = __blockIoDevGet((INT)ucDriver);

    if (!pblk) {
        return  (STA_NODISK);
    }
    
    if ((pblk->BLKD_iFlag & O_ACCMODE) == O_RDONLY) {
        dstat |= STA_PROTECT;
    }
    
    dstat |= (DSTATUS)__blockIoDevStatus((INT)ucDriver);
    
    return  (dstat);
}
/*********************************************************************************************************
** ��������: disk_status
** ��������: ��һ�����豸
** �䡡��  : ucDriver          �����
**           ucBuffer          ������
**           dwSectorNumber    ��ʼ������
**           uiSectorCount     ��������
** �䡡��  : DRESULT
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
DRESULT disk_read (BYTE  ucDriver, BYTE  *ucBuffer, DWORD   dwSectorNumber, UINT  uiSectorCount)
{
    REGISTER INT    iError;
    
    iError = __blockIoDevRead((INT)ucDriver, 
                              (PVOID)ucBuffer, 
                              (ULONG)dwSectorNumber,
                              (ULONG)uiSectorCount);
    if (iError >= ERROR_NONE) {
        return  (RES_OK);
    
    } else {
        return  (RES_ERROR);
    }
}
/*********************************************************************************************************
** ��������: disk_write
** ��������: дһ�����豸
** �䡡��  : ucDriver          �����
**           ucBuffer          ������
**           dwSectorNumber    ��ʼ������
**           uiSectorCount     ��������
** �䡡��  : DRESULT
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
DRESULT disk_write (BYTE  ucDriver, const BYTE  *ucBuffer, DWORD   dwSectorNumber, UINT  uiSectorCount)
{
    REGISTER INT    iError;
    
    iError = __blockIoDevWrite((INT)ucDriver, 
                               (PVOID)ucBuffer, 
                               (ULONG)dwSectorNumber,
                               (ULONG)uiSectorCount);
    if (iError >= ERROR_NONE) {
        return  (RES_OK);
    
    } else {
        return  (RES_ERROR);
    }
}
/*********************************************************************************************************
** ��������: disk_write
** ��������: дһ�����豸
** �䡡��  : ucDriver          �����
**           ucCmd             ����
**           pvArg             ����
** �䡡��  : DRESULT
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
DRESULT  disk_ioctl (BYTE  ucDriver, BYTE ucCmd, void  *pvArg)
{
    REGISTER INT            iError;
             ULONG          ulArgLong;
             LW_BLK_RANGE   blkrange;
    
    switch (ucCmd) {                                                    /*  ת������                    */
    
    case CTRL_SYNC:
        return  (RES_OK);                                               /*  ע��, Ŀǰ�����������      */
    
    case GET_SECTOR_COUNT:                                              /*  �������������              */
        if (__blockIoDevIoctl((INT)ucDriver, 
                              LW_BLKD_GET_SECNUM, 
                              (LONG)&ulArgLong)) {
            return  (RES_ERROR);
        
        } else {
            *(DWORD *)pvArg = (DWORD)ulArgLong;                         /*  FatFs set 32 bit arg        */
        }
        return  (RES_OK);
        
    case GET_SECTOR_SIZE:                                               /*  ���������С                */
        if (__blockIoDevIoctl((INT)ucDriver, 
                              LW_BLKD_GET_SECSIZE, 
                              (LONG)&ulArgLong)) {
            return  (RES_ERROR);
        
        } else {
            *(WORD *)pvArg = (WORD)ulArgLong;                           /*  FatFs set 16 bit arg        */
        }
        return  (RES_OK);
        
    case GET_BLOCK_SIZE:                                                /*  ��ÿ��С                  */
        if (__blockIoDevIoctl((INT)ucDriver, 
                              LW_BLKD_GET_BLKSIZE, 
                              (LONG)&ulArgLong)) {
            return  (RES_ERROR);
        
        } else {
            *(DWORD *)pvArg = (DWORD)ulArgLong;                         /*  FatFs set 32 bit arg        */
        }
        return  (RES_OK);
        
    case CTRL_TRIM:                                                     /*  ATA �ͷ�����                */
        ucCmd = FIOTRIM;
        blkrange.BLKR_ulStartSector = (ULONG)(((DWORD *)pvArg)[0]);
        blkrange.BLKR_ulEndSector   = (ULONG)(((DWORD *)pvArg)[1]);
        pvArg = (PVOID)&blkrange;
        break;
        
    case CTRL_POWER:                                                    /*  ��Դ����                    */
        ucCmd = LW_BLKD_CTRL_POWER;
        break;
        
    case CTRL_LOCK:                                                     /*  �����豸                    */
        ucCmd = LW_BLKD_CTRL_LOCK;
        break;
        
    case CTRL_EJECT:                                                    /*  �����豸                    */
        ucCmd = LW_BLKD_CTRL_EJECT;
        break;
    }
    
    iError = __blockIoDevIoctl((INT)ucDriver, (INT)ucCmd, (LONG)pvArg);
    if (iError >= ERROR_NONE) {
        return  (RES_OK);
    
    } else {
        return  (RES_ERROR);
    }
}

#endif                                                                  /*  LW_CFG_MAX_VOLUMES          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
