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
** ��   ��   ��: tpsfs_dir.h
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2015 �� 9 �� 21 ��
**
** ��        ��: tpsfs Ŀ¼����

** BUG:
*********************************************************************************************************/

#ifndef __TPSFS_DIR_H
#define __TPSFS_DIR_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0

/*********************************************************************************************************
  ����entry��С�����ļ������ȶ���������ΪTPS_ENTRY_SIZE��������
*********************************************************************************************************/

#define TPS_ENTRY_ITEM_SIZE     256
#define TPS_ENTRY_ITEM_SHIFT    8
#define TPS_ENTRY_ITEM_MASK     0xff

#define TPS_ENTRY_MAGIC_NUM     0x00df87ad

/*********************************************************************************************************
  entry �ṹ
*********************************************************************************************************/

typedef struct tps_entry {
    struct tps_entry   *ENTRY_pnext;                                    /* entry����                    */
    PTPS_SUPER_BLOCK    ENTRY_psb;                                      /* ������                       */
    TPS_INUM            ENTRY_inumDir;                                  /* ��Ŀ¼inode��                */
    PTPS_INODE          ENTRY_pinode;                                   /* �ļ�inodeָ��                */
    UINT                ENTRY_uiLen;                                    /* entry���̽ṹ����            */
    UINT                ENTRY_uiMagic;                                  /* entryģ��                    */
    TPS_INUM            ENTRY_inum;                                     /* �ļ���                       */
    BOOL                ENTRY_bInHash;                                  /* entry�Ƿ���hash����          */
    TPS_OFF_T           ENTRY_offset;                                   /* �ļ���Ŀ¼�ļ��е�ƫ��       */
    CHAR                ENTRY_pcName[1];                                /* �ļ���                       */
} TPS_ENTRY;
typedef TPS_ENTRY      *PTPS_ENTRY;

/*********************************************************************************************************
  entry ����
*********************************************************************************************************/
#ifdef __cplusplus 
extern "C" { 
#endif 
                                                                        /* ����entry                    */
TPS_RESULT tpsFsCreateEntry(PTPS_TRANS ptrans, PTPS_INODE pinodeDir,
                            CPCHAR pcFileName, TPS_INUM inum);
                                                                        /* ����entry                    */
TPS_RESULT tpsFsFindEntry(PTPS_INODE pinodeDir, CPCHAR pcFileName, PTPS_ENTRY *ppentry);
                                                                        /* �ͷ�entry�ڴ�ָ��            */
TPS_RESULT tpsFsEntryFree(PTPS_ENTRY pentry);
                                                                        /* ɾ��entry                    */
TPS_RESULT tpsFsEntryRemove(PTPS_TRANS ptrans, PTPS_ENTRY pentry);
                                                                        /* ��ָ��ƫ�ƿ�ʼ��ȡentry      */
TPS_RESULT tpsFsEntryRead(PTPS_INODE pinodeDir, BOOL bHash, TPS_OFF_T off, PTPS_ENTRY *ppentry);
                                                                        /* ��ȡ���һ��Ŀ¼�����λ��   */
TPS_SIZE_T tpsFsGetDirSize (PTPS_INODE pinodeDir);

#ifdef __cplusplus 
}
#endif 

#endif                                                                  /* LW_CFG_TPSFS_EN > 0          */
#endif                                                                  /* __TPSFS_DIR_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
