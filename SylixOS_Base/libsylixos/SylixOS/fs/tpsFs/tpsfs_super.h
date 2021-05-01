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
** ��   ��   ��: tpsfs_super.h
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2015 �� 9 �� 21 ��
**
** ��        ��: tpsfs ������ṹ����

** BUG:
*********************************************************************************************************/

#ifndef __TPSFS_SUPER_H
#define __TPSFS_SUPER_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0

/*********************************************************************************************************
  magic ��ֵ����
*********************************************************************************************************/

#define TPS_MAGIC_SUPER_BLOCK1   0xad348878
#define TPS_MAGIC_SUPER_BLOCK2   0xad348879

/*********************************************************************************************************
  �ļ�ϵͳ���� id
*********************************************************************************************************/

#define TPS_TYPE_NUMBER         0x9c

/*********************************************************************************************************
  �汾
*********************************************************************************************************/

#define TPS_CUR_VERSION         2
#define TPS_VER_SURPORT_HASHDIR 2

/*********************************************************************************************************
  super block ��������
*********************************************************************************************************/

#define TPS_MIN_LOG_SIZE        (512 * 1024)                            /* ��С��־��С                 */
#define TPS_SUPER_BLOCK_SECTOR  0                                       /* ������������                 */
#define TPS_SUPER_BLOCK_NUM     0                                       /* �������                     */
#define TPS_SPACE_MNG_INUM      1                                       /* �ռ����inode��              */
#define TPS_ROOT_INUM           2                                       /* ��inode��                    */
#define TPS_BPSTART_BLK         3                                       /* �黺����ʼ                   */
#define TPS_BPSTART_CNT         7                                       /* �黺�����Ŀ                 */
#define TPS_DATASTART_BLK       10                                      /* ���ݿ���ʼ                   */
#define TPS_MIN_BLK_SIZE        4096                                    /* ��С���С����               */
#define TPS_INODE_CACHE_LMT     1024

/*********************************************************************************************************
  super block ���ر�ʶ
*********************************************************************************************************/

#define TPS_MOUNT_FLAG_WRITE    0x2                                     /* ��д                         */
#define TPS_MOUNT_FLAG_READ     0x1                                     /* �ɶ�                         */
#define TPS_TRANS_FAULT         0x4                                     /* �������̬���ļ�ϵͳ���ɷ��� */

/*********************************************************************************************************
  super block �ṹ
*********************************************************************************************************/

typedef struct tps_super_block {
    UINT                    SB_uiMagic;                                 /* magic��ֵ                    */
    UINT                    SB_uiVersion;                               /* �汾                         */
    
    UINT                    SB_uiSectorSize;                            /* ���豸��������С             */
    UINT                    SB_uiSectorShift;
    UINT                    SB_uiSectorMask;
    UINT                    SB_uiSecPerBlk;                             /* ÿ��������                   */
    
    UINT                    SB_uiBlkSize;                               /* ���С                       */
    UINT                    SB_uiBlkShift;
    UINT                    SB_uiBlkMask;
    
    UINT                    SB_uiFlags;                                 /* ���ر�ʶ                     */
    UINT64                  SB_ui64Generation;                          /* ��ʶһ�θ�ʽ������ϵͳ�޸�   */
    UINT64                  SB_ui64TotalBlkCnt;                         /* �ܿ���                       */
    UINT64                  SB_ui64DataStartBlk;                        /* ���ݿ���ʼ                   */
    UINT64                  SB_ui64DataBlkCnt;                          /* ���ݿ�����                   */
    UINT64                  SB_ui64LogStartBlk;                         /* ��־����ʼ                   */
    UINT64                  SB_ui64LogBlkCnt;                           /* ��־������                   */
    UINT64                  SB_ui64BPStartBlk;                          /* btree�黺�����ʼ��          */
    UINT64                  SB_ui64BPBlkCnt;                            /* btree�黺��������          */
    
    TPS_INUM                SB_inumSpaceMng;                            /* �ռ����inode��              */
    TPS_INUM                SB_inumRoot;                                /* �ļ�ϵͳ��inode��            */
    TPS_INUM                SB_inumDeleted;                             /* ��ɾ���ļ��б�               */

    PTPS_DEV                SB_dev;                                     /* �豸����ָ��                 */
    struct tps_inode       *SB_pinodeSpaceMng;                          /* �ռ����inode                */
    struct tps_inode       *SB_pinodeRoot;                              /* �ļ�ϵͳ��inode              */
    struct tps_inode       *SB_pinodeDeleted;                           /* �Ѿ�ɾ�����ļ�               */
    struct tps_inode       *SB_pinodeOpenList;                          /* �Դ��ļ�����               */
    UINT                    SB_uiInodeOpenCnt;                          /* ��ǰ���ļ���               */

    PUCHAR                  SB_pucSectorBuf;                            /* ����ҳ�滺����               */

    struct tps_blk_pool    *SB_pbp;                                     /* btree�黺������              */

    struct tps_trans_sb    *SB_ptranssb;                                /* ����ϵͳ������               */
} TPS_SUPER_BLOCK;
typedef TPS_SUPER_BLOCK    *PTPS_SUPER_BLOCK;

/*********************************************************************************************************
  super block ����
*********************************************************************************************************/
#ifdef __cplusplus 
extern "C" { 
#endif 
                                                                        /* ����tpsfs�ļ�ϵͳ            */
errno_t tpsFsMount(PTPS_DEV pdev, UINT uiFlags, PTPS_SUPER_BLOCK *ppsb);
                                                                        /* ж��tpsfs�ļ�ϵͳ            */
errno_t tpsFsUnmount(PTPS_SUPER_BLOCK pSB);
                                                                        /* ��ʽ��tpsfs�ļ�ϵͳ          */
errno_t tpsFsFormat(PTPS_DEV pdev, UINT uiBlkSize);

#ifdef __cplusplus 
}
#endif 

#endif                                                                  /* LW_CFG_TPSFS_EN > 0          */
#endif                                                                  /* __TPSFS_SUPER_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
