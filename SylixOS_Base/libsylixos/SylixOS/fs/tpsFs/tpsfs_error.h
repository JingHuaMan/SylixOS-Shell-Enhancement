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
** ��   ��   ��: tpsfs_error.h
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2015 �� 9 �� 21 ��
**
** ��        ��: tpsfs ������

** BUG:
*********************************************************************************************************/

#ifndef __TPSFS_ERROR_H
#define __TPSFS_ERROR_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0

typedef enum {
    TPS_ERR_NONE = 0,

    TPS_ERR_PARAM_NULL,                                                 /* ����Ϊ��                     */
    TPS_ERR_PARAM,                                                      /* ��������                     */
    TPS_ERR_ALLOC,                                                      /* �ڴ�������                 */
    TPS_ERR_FORMAT,                                                     /* �ļ�ϵͳ��ʽ����             */

    TPS_ERR_SECTOR_SIZE,                                                /* ������С����                 */
    TPS_ERR_BLOCK_SIZE,                                                 /* ������С����                 */
    TPS_ERR_READSECTOR,                                                 /* ����������                   */
    TPS_ERR_WRITESECTOR,                                                /* д��������                   */
    TPS_ERR_DEV_SYNC,                                                   /* ����ͬ������                 */

    TPS_ERR_WALKPATH,                                                   /* ����·��ʧ��                 */
    TPS_ERR_OP_NOT_SUPPORT,                                             /* ��֧�ָò���                 */

    TPS_ERR_CHECK_LOG,                                                  /* ��־������                 */
    TPS_ERR_COMLETE_LOG,                                                /* ����ļ�ϵͳһ���Դ���       */

    TPS_ERR_INODE_GET,                                                  /* ��ȡinode����                */
    TPS_ERR_INODE_OPEN ,                                                /* ��inode����                */
    TPS_ERR_FORMAT_SETTING,                                             /* ��ʽ�����ô���               */
    TPS_ERR_INODE_CREAT,                                                /* ����inode����                */
    TPS_ERR_UNMOUNT_INUSE,                                              /* ж������ʹ�õľ�             */
    TPS_ERR_INODE_ADD_BLK,                                              /* ��ӿ鵽inode����            */
    TPS_ERR_INODE_WRITE,                                                /* д�ļ�����                   */
    TPS_ERR_INODE_READ,                                                 /* ���ļ�����                   */
    TPS_ERR_INODE_DELETED,                                              /* ɾ��inode����                */
    TPS_ERR_INODE_TRUNC,                                                /* �ض�inode����                */
    TPS_ERR_INODE_SIZE,                                                 /* inode��С����                */
    TPS_ERR_INODE_SYNC,                                                 /* ͬ��inode����                */
    TPS_ERR_INODE_SERIAL,                                               /* ���к�inode����              */
    TPS_ERR_INODE_HASHNOEMPTY,                                          /* Ŀ¼�ǿ�                     */
    TPS_ERR_INODE_BUFF,                                                 /* inode����������              */

    TPS_ERR_BTREE_INIT,                                                 /* ��ʼ��b+tree                 */
    TPS_ERR_BTREE_INSERT,                                               /* ����鵽b+tree����           */
    TPS_ERR_BTREE_TRUNC,                                                /* �ض�b+tree����               */
    TPS_ERR_BTREE_ALLOC,                                                /* ����b+tree�ڵ����           */
    TPS_ERR_BTREE_FREE,                                                 /* �ͷ�b+tree�ڵ����           */
    TPS_ERR_BTREE_FREE_ND,                                              /* �ͷ�b+tree�ڵ㵽�黺�����   */
    TPS_ERR_BTREE_KEY_AREADY_EXIT,                                      /* ��ֵ��ͻ                     */
    TPS_ERR_BTREE_PUT_NODE,                                             /* дb+tree�ڵ㵽���̴���       */
    TPS_ERR_BTREE_GET_NODE,                                             /* �Ӵ��̻�ȡb+tree�ڵ����     */
    TPS_ERR_BTREE_INSERT_NODE,                                          /* ����ڵ㵽b+tree����         */
    TPS_ERR_BTREE_NODE_TYPE,                                            /* �ڵ����ʹ���                 */
    TPS_ERR_BTREE_NODE_NOT_EXIST,                                       /* ָ����ֵ�ڵ㲻����           */
    TPS_ERR_BTREE_BLOCK_COUNT,                                          /* ��������������               */
    TPS_ERR_BTREE_NODE_OVERLAP,                                         /* �������ص�                   */
    TPS_ERR_BTREE_NODE_REMOVE,                                          /* ɾ��b+tree�ڵ����           */
    TPS_ERR_BTREE_KEY_NOTFOUND,                                         /* ����ָ����ֵ����             */
    TPS_ERR_BTREE_UPDATE_KEY,                                           /* ���¼�ֵ��                   */
    TPS_ERR_BTREE_DISK_SPACE,                                           /* ���̿ռ䲻��                 */
    TPS_ERR_BTREE_NODE_MAGIC,                                           /* BTREE �ڵ� magic����         */
    TPS_ERR_BTREE_OVERFLOW,                                             /* BTREE�ڵ�������            */

    TPS_ERR_CHECK_NAME,                                                 /* �ļ�·��������             */

    TPS_ERR_TRANS_ALLOC,                                                /* �����������                 */
    TPS_ERR_TRANS_WRITE,                                                /* ����д��������               */
    TPS_ERR_TRANS_COMMIT,                                               /* �ύ�������                 */
    TPS_ERR_TRANS_CHECK,                                                /* �����������                 */
    TPS_ERR_TRANS_OVERFLOW,                                             /* �����ڴ����                 */
    TPS_ERR_TRANS_NEED_COMMIT,                                          /* ������Ҫ�ύ                 */
    TPS_TRAN_INIT_SIZE,                                                 /* ��ʼ�����������С����       */
    TPS_ERR_TRANS_COMMIT_FAULT,                                         /* �����ύ���̳���             */
    TPS_ERR_TRANS_ROLLBK_FAULT,                                         /* �ع�������̳���             */
    TPS_ERR_TRANS_READ,                                                 /* �����ȡʧ��                 */

    TPS_ERR_ENTRY_NOT_EXIST,                                            /* �ļ�������                   */
    TPS_ERR_ENTRY_AREADY_EXIST,                                         /* �ļ��Ѵ���                   */
    TPS_ERR_ENTRY_CREATE,                                               /* ����Ŀ¼�����               */
    TPS_ERR_ENTRY_FIND,                                                 /* ����Ŀ¼�����               */
    TPS_ERR_ENTRY_REMOVE,                                               /* ɾ��Ŀ¼�����               */
    TPS_ERR_ENTRY_UNEQUAL,                                              /* Ŀ¼�������                 */

    TPS_ERR_BUF_READ,                                                   /* �����̴���                   */
    TPS_ERR_BUF_WRITE,                                                  /* д���̴���                   */
    TPS_ERR_BUF_SYNC,                                                   /* ͬ�����̴���                 */
    TPS_ERR_BUF_TRIM,                                                   /* ���մ��̿����               */
    TPS_ERR_SEEK_DEV,                                                   /* seek�����豸����             */
    TPS_ERR_READ_DEV,                                                   /* �������豸����               */
    TPS_ERR_WRITE_DEV,                                                  /* д�����豸����               */

    TPS_ERR_BP_INIT,                                                    /* ��ʼ���黺����д���         */
    TPS_ERR_BP_READ,                                                    /* ���黺����д���             */
    TPS_ERR_BP_WRITE,                                                   /* д�黺����д���             */
    TPS_ERR_BP_ALLOC,                                                   /* �ӿ黺����з�������       */
    TPS_ERR_BP_FREE,                                                    /* �ͷſ鵽�黺����д���       */
    TPS_ERR_BP_ADJUST,                                                  /* �����黺����д�С����       */

    TPS_ERR_DIR_MK,                                                     /* ����Ŀ¼����                 */

    TPS_ERR_HASH_EXIST,                                                 /* hash�ڵ��Ѵ���               */
    TPS_ERR_HASH_TOOLONG_NAME,                                          /* �����hash�ڵ��е����ֹ���   */
    TPS_ERR_HASH_INSERT,                                                /* ����hash�ڵ�ʧ��             */
    TPS_ERR_HASH_NOT_EXIST,                                             /* hash�ڵ㲻����               */
    TPS_ERR_HASH_REMOVE,                                                /* ɾ��hash�ڵ�ʧ��             */

    TPS_ERR_UNEXPECT                                                    /* λ�ô���                     */
} TPS_RESULT;

#endif                                                                  /* LW_CFG_TPSFS_EN > 0          */
#endif                                                                  /* __TPSFS_ERROR_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
