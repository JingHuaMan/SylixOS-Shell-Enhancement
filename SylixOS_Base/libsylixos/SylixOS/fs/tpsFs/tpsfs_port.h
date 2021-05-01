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
** ��   ��   ��: tpsfs_port.h
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2015 �� 9 �� 21 ��
**
** ��        ��: tpsfs ��ֲ��

** BUG:
*********************************************************************************************************/

#ifndef __TPSFS_PORT_H
#define __TPSFS_PORT_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0

#include "endian.h"

/*********************************************************************************************************
  ��׼ʱ���ȡ
*********************************************************************************************************/

#define TPS_UTC_TIME()      lib_time(LW_NULL);

/*********************************************************************************************************
  �豸�ṹ����ֲ����ʵ��
*********************************************************************************************************/

typedef struct tps_dev {
    UINT       (*DEV_SectorSize)(struct tps_dev *pdev);                 /* ��ȡ������С                 */
    UINT64     (*DEV_SectorCnt)(struct tps_dev *pdev);                  /* ��ȡ��������                 */
    INT        (*DEV_ReadSector)(struct tps_dev *pdev,
                                 PUCHAR pucBuf,
                                 UINT64 ui64StartSector,
                                 UINT64 uiSectorCnt);                   /* ������                       */
    INT        (*DEV_WriteSector)(struct tps_dev *pdev,
                                  PUCHAR pucBuf,
                                  UINT64 ui64StartSector,
                                  UINT64 uiSectorCnt,
                                  BOOL bSync);                          /* д����                       */
    INT        (*DEV_Sync)(struct tps_dev *pdev, 
                           UINT64 ui64StartSector, 
                           UINT64 uiSectorCnt);                         /* ͬ������                     */
    INT        (*DEV_Trim)(struct tps_dev *pdev,
                           UINT64 ui64StartSector,
                           UINT64 uiSectorCnt);                         /* Trim����                     */
    PVOID        DEV_pvPriv;                                            /* ˽�г�Ա                     */
} TPS_DEV;
typedef TPS_DEV *PTPS_DEV;

/*********************************************************************************************************
  ��С��ת����tpsfs ����С�˴洢
*********************************************************************************************************/

#define TPS_LE32_TO_CPU(pos, val)   {   val = le32dec(pos); pos += sizeof(UINT32);    }
#define TPS_LE64_TO_CPU(pos, val)   {   val = le64dec(pos); pos += sizeof(UINT64);    }
#define TPS_LE32_TO_CPU_VAL(pos)    le32dec(pos)
#define TPS_LE64_TO_CPU_VAL(pos)    le64dec(pos)

#define TPS_CPU_TO_LE32(pos, val)   {   le32enc(pos, val); pos += sizeof(UINT32);    }
#define TPS_CPU_TO_LE64(pos, val)   {   le64enc(pos, val); pos += sizeof(UINT64);    }

#define TPS_CPU_TO_IBLK             TPS_CPU_TO_LE64
#define TPS_IBLK_TO_CPU             TPS_LE64_TO_CPU
#define TPS_IBLK_TO_CPU_VAL         TPS_LE64_TO_CPU_VAL

/*********************************************************************************************************
  �ڴ����
*********************************************************************************************************/

#define TPS_ALLOC(sz)               __SHEAP_ALLOC(((size_t)(sz)))
#define TPS_FREE(p)                 __SHEAP_FREE((p))

/*********************************************************************************************************
  ���봦��
*********************************************************************************************************/

#define TPS_ROUND_UP(x, align)      (TPS_SIZE_T)(((TPS_SIZE_T)(x) +  (align - 1)) & ~(align - 1))
#define TPS_ROUND_DOWN(x, align)    (TPS_SIZE_T)( (TPS_SIZE_T)(x) & ~(align - 1))
#define TPS_ALIGNED(x, align)       (((TPS_SIZE_T)(x) & (align - 1)) == 0)

#endif                                                                  /* LW_CFG_TPSFS_EN > 0          */
#endif                                                                  /* __TPSFS_PORT_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
