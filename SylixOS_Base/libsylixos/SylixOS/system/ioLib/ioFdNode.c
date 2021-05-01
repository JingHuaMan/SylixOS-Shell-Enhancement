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
** ��   ��   ��: ioFdNode.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 01 �� 06 ��
**
** ��        ��: ��� NEW_1 ���豸������ fd_node ����
*********************************************************************************************************/
#define  __SYLIXOS_STDARG
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
** ��������: API_IosFdNodeAdd
** ��������: ���һ�� fd_node (���ͬһ���豸�Ѿ���һ���ظ��� inode ������ֻ��������)
** �䡡��  : pplineHeader  ͬһ�豸 fd_node ����
**           dev           �豸������
**           inode64       �ļ� inode (ͬһ�豸 inode ����Ψһ)
**           iFlags        ��ѡ��
**           mode          �ļ� mode
**           uid           �����û� id
**           gid           ������ id
**           oftSize       �ļ���ǰ��С
**           pvFile        ����˽����Ϣ
**           pbIsNew       ���ظ����������֪�Ƿ�Ϊ�½��ڵ㻹��
** �䡡��  : fd_node
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �˺����̲߳���ȫ, ��Ҫ���豸����ִ��
                                           API ����
*********************************************************************************************************/
LW_API 
PLW_FD_NODE  API_IosFdNodeAdd (LW_LIST_LINE_HEADER  *pplineHeader,
                               dev_t                 dev,
                               ino64_t               inode64,
                               INT                   iFlags,
                               mode_t                mode,
                               uid_t                 uid,
                               gid_t                 gid,
                               off_t                 oftSize,
                               PVOID                 pvFile,
                               BOOL                 *pbIsNew)
{
    PLW_LIST_LINE   plineTemp;
    PLW_FD_NODE     pfdnode;
    
    for (plineTemp  = *pplineHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  �����Ѿ��򿪵��ļ�          */
         
        pfdnode = _LIST_ENTRY(plineTemp, LW_FD_NODE, FDNODE_lineManage);
        if ((pfdnode->FDNODE_dev     == dev) &&
            (pfdnode->FDNODE_inode64 == inode64)) {                     /*  �ظ���                    */
            
            if (pfdnode->FDNODE_ulLock &&
                (iFlags & (O_WRONLY | O_RDWR | O_TRUNC))) {
                _ErrorHandle(EBUSY);                                    /*  �ļ�������                  */
                return  (LW_NULL);
            }
            
            if (pfdnode->FDNODE_bRemove && (iFlags & O_CREAT)) {        /*  ɾ�����ٴ���                */
                pfdnode->FDNODE_bRemove = LW_FALSE;                     /*  ��� Remove ��־            */
            }

            pfdnode->FDNODE_ulRef++;
            if (pbIsNew) {
                *pbIsNew = LW_FALSE;                                    /*  ֻ�������÷Ǵ���            */
            }
            return  (pfdnode);
        }
    }
    
    pfdnode = (PLW_FD_NODE)__SHEAP_ALLOC(sizeof(LW_FD_NODE));
    if (pfdnode == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    lib_bzero(pfdnode, sizeof(LW_FD_NODE));
    
    pfdnode->FDNODE_ulSem = API_SemaphoreBCreate("fd_node_lock", LW_TRUE, 
                                                 LW_OPTION_OBJECT_GLOBAL |
                                                 LW_OPTION_WAIT_PRIORITY, LW_NULL);
    if (pfdnode->FDNODE_ulSem == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(pfdnode);
        return  (LW_NULL);
    }
    
    pfdnode->FDNODE_dev     = dev;
    pfdnode->FDNODE_inode64 = inode64;
    pfdnode->FDNODE_mode    = mode;
    pfdnode->FDNODE_uid     = uid;
    pfdnode->FDNODE_gid     = gid;
    pfdnode->FDNODE_oftSize = oftSize;
    pfdnode->FDNODE_pvFile  = pvFile;
    pfdnode->FDNODE_bRemove = LW_FALSE;
    pfdnode->FDNODE_ulLock  = 0;                                        /*  û������                    */
    pfdnode->FDNODE_ulRef   = 1;                                        /*  ��ʼ������Ϊ 1              */
    
    if (pbIsNew) {
        *pbIsNew = LW_TRUE;
    }
    
    _List_Line_Add_Ahead(&pfdnode->FDNODE_lineManage,
                         pplineHeader);
    
    return  (pfdnode);
}
/*********************************************************************************************************
** ��������: API_IosFdNodeDec
** ��������: ɾ��һ�� fd_node (������ò�Ϊ 1 ��ֻ�� -- )
** �䡡��  : pplineHeader  ͬһ�豸 fd_node ����
**           pfdnode       fd_node
**           bRemove       �Ƿ���Ҫɾ���ļ�.
** �䡡��  : -1 : ����
**            0 : ����ɾ��
**           >0 : �������ú����������
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �˺����̲߳���ȫ, ��Ҫ���豸����ִ��
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_IosFdNodeDec (LW_LIST_LINE_HEADER  *pplineHeader,
                       PLW_FD_NODE           pfdnode,
                       BOOL                 *bRemove)
{
    if (!pfdnode) {
        return  (PX_ERROR);
    }

    pfdnode->FDNODE_ulRef--;
    if (pfdnode->FDNODE_ulRef) {
        if (bRemove) {
            *bRemove = LW_FALSE;
        }
        return  ((INT)pfdnode->FDNODE_ulRef);                           /*  ���������ļ����ü�������    */
    }
    
    _FdLockfClearFdNode(pfdnode);                                       /*  ɾ�����м�¼��              */
    
    _List_Line_Del(&pfdnode->FDNODE_lineManage,
                   pplineHeader);
    
    API_SemaphoreBDelete(&pfdnode->FDNODE_ulSem);
    
    if (bRemove) {
        *bRemove = pfdnode->FDNODE_bRemove;
    }
    
    __SHEAP_FREE(pfdnode);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_IosFdNodeFind
** ��������: ����һ�� fd_node 
** �䡡��  : plineHeader   ͬһ�豸 fd_node ����
**           dev           �豸������
**           inode64       �ļ� inode (ͬһ�豸 inode ����Ψһ)
** �䡡��  : fd_node
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �˺����̲߳���ȫ, ��Ҫ���豸����ִ��
                                           API ����
*********************************************************************************************************/
LW_API 
PLW_FD_NODE  API_IosFdNodeFind (LW_LIST_LINE_HEADER  plineHeader, dev_t  dev, ino64_t  inode64)
{
    PLW_LIST_LINE   plineTemp;
    PLW_FD_NODE     pfdnode;
    
    for (plineTemp  = plineHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  �����Ѿ��򿪵��ļ�          */
         
        pfdnode = _LIST_ENTRY(plineTemp, LW_FD_NODE, FDNODE_lineManage);
        if ((pfdnode->FDNODE_dev     == dev) &&
            (pfdnode->FDNODE_inode64 == inode64)) {
            return  (pfdnode);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_IosFdNodeLock
** ��������: ����һ�� fd_node ���ļ��ر�ǰ������д, ������ɾ��, �ر��ļ����Զ����.
** �䡡��  : pfdnode       fd_node
** �䡡��  : NONE
** ȫ�ֱ���: -1 : ����
**            0 : ����ɾ��
** ����ģ��: 
** ע  ��  : 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_IosFdNodeLock (PLW_FD_NODE  pfdnode)
{
    if (!pfdnode) {
        return  (PX_ERROR);
    }

    pfdnode->FDNODE_ulLock = 1;
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
