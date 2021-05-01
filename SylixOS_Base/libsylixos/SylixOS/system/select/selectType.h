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
** ��   ��   ��: selectType.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 11 �� 07 ��
**
** ��        ��:  IO ϵͳ select ��ϵͳ���ݽṹ����.

** BUG
2008.03.16  LW_HANDLE ��Ϊ LW_OBJECT_HANDLE.
*********************************************************************************************************/

#ifndef __SELECTTYPE_H
#define __SELECTTYPE_H

/*********************************************************************************************************
  �ȴ��ڵ�����
*********************************************************************************************************/

typedef enum {
    SELREAD,                                                            /*  ������                      */
    SELWRITE,                                                           /*  д����                      */
    SELEXCEPT                                                           /*  �쳣����                    */
} LW_SEL_TYPE;                                                          /*  �ȴ�����                    */

#define LW_SEL_TYPE_FLAG_READ   0x01
#define LW_SEL_TYPE_FLAG_WRITE  0x02
#define LW_SEL_TYPE_FLAG_EXCEPT 0x04

/*********************************************************************************************************
  �ȴ�����ڵ�.
*********************************************************************************************************/

typedef struct {
    LW_LIST_LINE            SELWUN_lineManage;                          /*  ��������                    */
    UINT32                  SELWUN_uiFlags;
#define LW_SELWUN_FLAG_DFREE    0x1                                     /*  �˽ڵ㲻��Ҫ�ͷ�            */

#define LW_SELWUN_CLEAR_DFREE(node) \
        ((node)->SELWUN_uiFlags &= ~LW_SELWUN_FLAG_DFREE)
#define LW_SELWUN_SET_DFREE(node)   \
        ((node)->SELWUN_uiFlags |= LW_SELWUN_FLAG_DFREE)
#define LW_SELWUN_IS_DFREE(node)    \
        ((node)->SELWUN_uiFlags & LW_SELWUN_FLAG_DFREE)

#define LW_SELWUN_FLAG_READY   0x2                                      /*  �˽ڵ�����׼����            */

#define LW_SELWUN_CLEAR_READY(node) \
        ((node)->SELWUN_uiFlags &= ~LW_SELWUN_FLAG_READY)
#define LW_SELWUN_SET_READY(node)   \
        ((node)->SELWUN_uiFlags |= LW_SELWUN_FLAG_READY)
#define LW_SELWUN_IS_READY(node)    \
        ((node)->SELWUN_uiFlags & LW_SELWUN_FLAG_READY)

    LW_OBJECT_HANDLE        SELWUN_hThreadId;                           /*  �����ڵ���߳̾��          */
    INT                     SELWUN_iFd;                                 /*  ���ӵ���ļ�������          */
    LW_SEL_TYPE             SELWUN_seltypType;                          /*  �ȴ�����                    */
} LW_SEL_WAKEUPNODE;
typedef LW_SEL_WAKEUPNODE  *PLW_SEL_WAKEUPNODE;

/*********************************************************************************************************
  �ȴ�����ͷ���ƽṹ, ֧�� select() �����������ļ�����ӵ�����½ṹ, ���� socket �ļ�.
*********************************************************************************************************/

typedef struct {
    LW_OBJECT_HANDLE        SELWUL_hListLock;                           /*  ������                      */
    LW_SEL_WAKEUPNODE       SELWUL_selwunFrist;                         /*  ͨ�������,ֻ����һ���ڵ�   */
                                                                        /*  ʹ������������Ա�������  */
                                                                        /*  ��̬�ڴ����                */
    PLW_LIST_LINE           SELWUL_plineHeader;                         /*  �ȴ�����ͷ                  */
    UINT                    SELWUL_ulWakeCounter;                       /*  �ȴ�������                  */
} LW_SEL_WAKEUPLIST;
typedef LW_SEL_WAKEUPLIST  *PLW_SEL_WAKEUPLIST;

/*********************************************************************************************************
  ÿ���̶߳�ӵ�е�˽�нṹ
*********************************************************************************************************/

typedef struct {
    LW_OBJECT_HANDLE        SELCTX_hSembWakeup;                         /*  �����ź���                  */
    BOOL                    SELCTX_bPendedOnSelect;                     /*  �Ƿ������� select() ��      */
    BOOL                    SELCTX_bBadFd;                              /*  �ļ�����������              */
    
    fd_set                  SELCTX_fdsetOrigReadFds;                    /*  ԭʼ�Ķ��ļ���              */
    fd_set                  SELCTX_fdsetOrigWriteFds;                   /*  ԭʼ��д�ļ���              */
    fd_set                  SELCTX_fdsetOrigExceptFds;                  /*  ԭʼ���쳣�ļ���            */
    
    INT                     SELCTX_iWidth;                              /*  select() ��һ������         */
} LW_SEL_CONTEXT;
typedef LW_SEL_CONTEXT     *PLW_SEL_CONTEXT;

/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/

INT   __selDoIoctls(fd_set  *pfdset, fd_set  *pfdsetUpdate,
                    INT  iFdSetWidth, INT  iFunc,
                    PLW_SEL_WAKEUPNODE pselwun, BOOL  bStopOnErr);

#endif                                                                  /*  __SELECTLIB_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
