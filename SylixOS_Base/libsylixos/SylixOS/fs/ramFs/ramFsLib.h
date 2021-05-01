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
** ��   ��   ��: ramFsLib.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 05 �� 24 ��
**
** ��        ��: �ڴ��ļ�ϵͳ�ڲ�����.
*********************************************************************************************************/

#ifndef __RAMFSLIB_H
#define __RAMFSLIB_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0 && LW_CFG_RAMFS_EN > 0

/*********************************************************************************************************
  ���·���ִ��Ƿ�Ϊ��Ŀ¼����ֱ��ָ���豸
*********************************************************************************************************/

#define __STR_IS_ROOT(pcName)           ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))

/*********************************************************************************************************
  �ڴ�����
*********************************************************************************************************/

#if (LW_CFG_RAMFS_VMM_EN > 0) && (LW_CFG_VMM_EN > 0)
#define __RAM_BSIZE_SHIFT               LW_CFG_VMM_PAGE_SHIFT           /*  page size                   */
#else
#define __RAM_BSIZE_SHIFT               11                              /*  (1 << 11) = 2048 (blk size) */
#endif                                                                  /*  LW_CFG_RAMFS_VMM_EN         */

/*********************************************************************************************************
  һ���ڴ��Ƭ��С
*********************************************************************************************************/

#define __RAM_BSIZE                     (1 << __RAM_BSIZE_SHIFT)
#define __RAM_BSIZE_MASK                ((1 << __RAM_BSIZE_SHIFT) - 1)
#define __RAM_BDATASIZE                 (__RAM_BSIZE - (sizeof(PVOID) * 2))

/*********************************************************************************************************
  �ļ������ڴ����
*********************************************************************************************************/

#if (LW_CFG_RAMFS_VMM_EN > 0) && (LW_CFG_VMM_EN > 0)
#define __RAM_BALLOC(size)              API_VmmMalloc(size);
#define __RAM_BFREE(ptr)                API_VmmFree(ptr);
#else
#define __RAM_BALLOC(size)              __SHEAP_ALLOC(size)
#define __RAM_BFREE(ptr)                __SHEAP_FREE(ptr)
#endif                                                                  /*  LW_CFG_RAMFS_VMM_EN         */

/*********************************************************************************************************
  ����
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR          RAMFS_devhdrHdr;                                /*  ramfs �ļ�ϵͳ�豸ͷ        */
    LW_OBJECT_HANDLE    RAMFS_hVolLock;                                 /*  �������                    */
    LW_LIST_LINE_HEADER RAMFS_plineFdNodeHeader;                        /*  fd_node ����                */
    LW_LIST_LINE_HEADER RAMFS_plineSon;                                 /*  ��������                    */
    
    BOOL                RAMFS_bForceDelete;                             /*  �Ƿ�����ǿ��ж�ؾ�          */
    BOOL                RAMFS_bValid;
    
    uid_t               RAMFS_uid;                                      /*  �û� id                     */
    gid_t               RAMFS_gid;                                      /*  ��   id                     */
    mode_t              RAMFS_mode;                                     /*  �ļ� mode                   */
    time_t              RAMFS_time;                                     /*  ����ʱ��                    */
    ULONG               RAMFS_ulCurBlk;                                 /*  ��ǰ�����ڴ��С            */
    ULONG               RAMFS_ulMaxBlk;                                 /*  ����ڴ�������              */
} RAM_VOLUME;
typedef RAM_VOLUME     *PRAM_VOLUME;

typedef struct {
    LW_LIST_LINE        RAMB_lineManage;                                /*  ��������                    */
    BYTE                RAMB_ucData[__RAM_BDATASIZE];                   /*  �ڴ��                      */
} RAM_BUFFER;
typedef RAM_BUFFER     *PRAM_BUFFER;

typedef struct ramfs_node {
    LW_LIST_LINE        RAMN_lineBrother;                               /*  �ֵܽڵ�����                */
    struct ramfs_node  *RAMN_pramnFather;                               /*  ����ָ��                    */
    PLW_LIST_LINE       RAMN_plineSon;                                  /*  �ӽڵ�����                  */
    PRAM_VOLUME         RAMN_pramfs;                                    /*  �ļ�ϵͳ                    */
    
    BOOL                RAMN_bChanged;                                  /*  �ļ������Ƿ����            */
    mode_t              RAMN_mode;                                      /*  �ļ� mode                   */
    time_t              RAMN_timeCreate;                                /*  ����ʱ��                    */
    time_t              RAMN_timeAccess;                                /*  ������ʱ��                */
    time_t              RAMN_timeChange;                                /*  ����޸�ʱ��                */
    
    size_t              RAMN_stSize;                                    /*  ��ǰ�ļ���С (���ܴ��ڻ���) */
    size_t              RAMN_stVSize;                                   /*  lseek ���������С          */
    
    uid_t               RAMN_uid;                                       /*  �û� id                     */
    gid_t               RAMN_gid;                                       /*  ��   id                     */
    PCHAR               RAMN_pcName;                                    /*  �ļ�����                    */
    PCHAR               RAMN_pcLink;                                    /*  ����Ŀ��                    */
    
    PLW_LIST_LINE       RAMN_plineBStart;                               /*  �ļ�ͷ                      */
    PLW_LIST_LINE       RAMN_plineBEnd;                                 /*  �ļ�β                      */
    ULONG               RAMN_ulCnt;                                     /*  �ļ����ݿ�����              */
    
    PRAM_BUFFER         RAMN_prambCookie;                               /*  �ļ� cookie                 */
    ULONG               RAMN_ulCookie;                                  /*  �ļ� cookie �±�            */
} RAM_NODE;
typedef RAM_NODE       *PRAM_NODE;

/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/
PRAM_NODE   __ram_open(PRAM_VOLUME  pramfs,
                       CPCHAR       pcName,
                       PRAM_NODE   *ppramnFather,
                       BOOL        *pbRoot,
                       BOOL        *pbLast,
                       PCHAR       *ppcTail);
PRAM_NODE   __ram_maken(PRAM_VOLUME  pramfs,
                        CPCHAR       pcName,
                        PRAM_NODE    pramnFather,
                        mode_t       mode,
                        CPCHAR       pcLink);
INT         __ram_unlink(PRAM_NODE  pramn);
VOID        __ram_truncate(PRAM_NODE  pramn, size_t  stOft);
VOID        __ram_increase(PRAM_NODE  pramn, size_t  stNewSize);
ssize_t     __ram_read(PRAM_NODE  pramn, PVOID  pvBuffer, size_t  stSize, size_t  stOft);
ssize_t     __ram_write(PRAM_NODE  pramn, CPVOID  pvBuffer, size_t  stNBytes, size_t  stOft);
VOID        __ram_mount(PRAM_VOLUME  pramfs);
VOID        __ram_unmount(PRAM_VOLUME  pramfs);
VOID        __ram_close(PRAM_NODE  pramn, INT iFlag);
INT         __ram_move(PRAM_NODE  pramn, PCHAR  pcNewName);
VOID        __ram_stat(PRAM_NODE  pramn, PRAM_VOLUME  pramfs, struct stat  *pstat);
VOID        __ram_statfs(PRAM_VOLUME  pramfs, struct statfs  *pstatfs);
                       
#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
                                                                        /*  LW_CFG_RAMFS_EN > 0         */
#endif                                                                  /*  __RAMFSLIB_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
