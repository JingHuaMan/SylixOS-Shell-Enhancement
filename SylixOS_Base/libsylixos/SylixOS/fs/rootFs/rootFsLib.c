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
** ��   ��   ��: rootFsLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 03 ��
**
** ��        ��: root �ļ�ϵͳ�ڲ��� (�ڰ�װ�����豸֮ǰ, ���밲װ rootFs ������ root �豸, 
                                      �����������豸�Ĺҽӵ�).
                                      
** BUG:
2010.08.10  �����ڴ治��ʱ�Ĵ��� BUG.
2010.09.10  �����������ӽڵ�ʱ�� mode ���� BUG.
2011.03.22  API_RootFsMakeNode() ���������ļ�Ӧ�þ��� S_IFLNK ����.
2011.05.16  �����ڵ�ʱ, �����ϵ�ڵ�Ϊ��������, �򷵻� errno == ERROR_IOS_FILE_SYMLINK.
2011.05.19  ���� dev match ʱ�Ը��ļ�ϵͳĿ¼���ļ��жϴ�������.
2011.06.11  make node ���� opt ѡ��, ����Խڵ�ʱ��̳� rootfs ʱ���֧��.
2011.08.11  __rootFsDevMatch() �����ƥ��ʧ��, ����ļ����ǴӸ����ſ�ʼ��, �򷵻ظ��豸.
2012.09.25  ֧�� LW_ROOTFS_NODE_TYPE_SOCK �����ļ�.
2012.11.09  __rootFsReadNode() ������Ч�ֽڳ���.
2012.12.20  ���� API_RootFsMakeNode() ���� socket �ļ�����.
2013.03.16  ����� reg �ļ���֧��.
            API_RootFsMakeNode ���� mode ����.
2013.08.13  ������ļ������ļ�¼.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PATH_VXWORKS == 0)
#include "rootFsLib.h"
#include "rootFs.h"
/*********************************************************************************************************
  rootfs ȫ�ֱ���
*********************************************************************************************************/
LW_ROOTFS_ROOT      _G_rfsrRoot;                                        /*  rootFs ��                   */
/*********************************************************************************************************
** ��������: __rootFsFindNode
** ��������: rootfs ����һ���ڵ�
** �䡡��  : pcName            �ڵ��� (����Ӹ����� / ��ʼ)
**           pprfsnFather      ���޷��ҵ��ڵ�ʱ��Ϊ�ӽ���һ��, 
                               ���ҵ��ڵ�ʱ�����ҵ��ڵ�ĸ�ϵ�ڵ�. 
                               LW_NULL ��ʾ�ҵ���Ϊ��
**           pbRoot            �ڵ����Ƿ�ָ����ڵ�.
**           pbLast            ��ƥ��ʧ��ʱ, �Ƿ������һ���ļ�ƥ��ʧ��
                               ����: /a/b/c Ŀ¼a��b��ƥ��ɹ�, ��ʣcû�гɹ��� pbLast Ϊ LW_TRUE.
**           ppcTail           ����β��, ���������豸��������ʱ, tail ָ������β.
** �䡡��  : �ڵ�, LW_NULL ��ʾ���ڵ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_ROOTFS_NODE  __rootFsFindNode (CPCHAR            pcName, 
                                   PLW_ROOTFS_NODE  *pprfsnFather, 
                                   BOOL             *pbRoot,
                                   BOOL             *pbLast,
                                   PCHAR            *ppcTail)
{
    /*
     *  ���е��ô˺����ĵ����߾�Ҫ��֤�˺����� LW_ROOTFS_LOCK() ����, 
     *  �˺����ڲ�ʹ�õ�·������Ϊȫ�ֱ���.
     */
    static CHAR         pcTempName[MAX_FILENAME_LENGTH];
    PCHAR               pcNext;
    PCHAR               pcNode;
    
    PLW_ROOTFS_NODE     prfsn;
    PLW_ROOTFS_NODE     prfsnTemp;
    
    PLW_LIST_LINE       plineTemp;
    PLW_LIST_LINE       plineHeader;                                    /*  ��ǰĿ¼ͷ                  */
    
    if (pprfsnFather == LW_NULL) {
        pprfsnFather = &prfsnTemp;                                      /*  ��ʱ����                    */
    }
    *pprfsnFather = LW_NULL;
    
    if (*pcName == PX_ROOT) {                                           /*  ���Ը�����                  */
        lib_strlcpy(pcTempName, (pcName + 1), PATH_MAX);
    
    } else {
        if (pbRoot) {
            *pbRoot = LW_FALSE;                                         /*  pcName ��Ϊ��               */
        }
        if (pbLast) {
            *pbLast = LW_FALSE;
        }
        return  (LW_NULL);
    }
    
    /*
     *  �ж��ļ����Ƿ�Ϊ��
     */
    if (pcTempName[0] == PX_EOS) {
        if (pbRoot) {
            *pbRoot = LW_TRUE;                                          /*  pcName Ϊ��                 */
        }
        if (pbLast) {
            *pbLast = LW_FALSE;
        }
        return  (LW_NULL);
    
    } else {
        if (pbRoot) {
            *pbRoot = LW_FALSE;                                         /*  pcName ��Ϊ��               */
        }
    }
    
    pcNext      = pcTempName;
    plineHeader = _G_rfsrRoot.RFSR_plineSon;                            /*  �Ӹ�Ŀ¼��ʼ����            */
    
    /*
     *  ����ʹ��ѭ�� (����ݹ�!)
     */
    do {
        pcNode = pcNext;
        pcNext = lib_index(pcNode, PX_DIVIDER);                         /*  �ƶ����¼�Ŀ¼              */
        if (pcNext) {                                                   /*  �Ƿ���Խ�����һ��          */
            *pcNext = PX_EOS;
            pcNext++;                                                   /*  ��һ���ָ��                */
        }
        
        for (plineTemp  = plineHeader;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
             
            prfsn = _LIST_ENTRY(plineTemp, LW_ROOTFS_NODE, RFSN_lineBrother);
            if (prfsn->RFSN_iNodeType == LW_ROOTFS_NODE_TYPE_DEV) {     /*  �˽ڵ�Ϊ�豸                */
                /*
                 *  ���Ƚ��豸������
                 */
                PCHAR       pcDevName = lib_rindex(prfsn->RFSN_rfsnv.RFSNV_pdevhdr->DEVHDR_pcName, 
                                                   PX_DIVIDER);
                if (pcDevName == LW_NULL) {
                    pcDevName =  prfsn->RFSN_rfsnv.RFSNV_pdevhdr->DEVHDR_pcName;
                } else {
                    pcDevName++;                                        /*  ���� /                      */
                }
                
                if (lib_strcmp(pcDevName, pcNode) == 0) {               /*  �Ƚ��豸��                  */
                    goto    __find_ok;                                  /*  �ҵ��豸                    */
                }
            
            } else if (prfsn->RFSN_iNodeType == 
                       LW_ROOTFS_NODE_TYPE_LNK) {                       /*  �˽ڵ�Ϊ����                */
                if (lib_strcmp(prfsn->RFSN_rfsnv.RFSNV_pcName,
                               pcNode) == 0) {
                    goto    __find_ok;                                  /*  �ҵ�����                    */
                }
            
            } else if (prfsn->RFSN_iNodeType == 
                       LW_ROOTFS_NODE_TYPE_SOCK) {                      /*  �˽ڵ�Ϊ socket �ļ�        */
                if (lib_strcmp(prfsn->RFSN_rfsnv.RFSNV_pcName,
                               pcNode) == 0) {
                    if (pcNext) {                                       /*  �������¼�, �������ΪĿ¼  */
                        goto    __find_error;                           /*  ����Ŀ¼ֱ�Ӵ���            */
                    }
                    break;
                }
            } else {                                                    /*  �˽ڵ�ΪĿ¼                */
                if (lib_strcmp(prfsn->RFSN_rfsnv.RFSNV_pcName,
                               pcNode) == 0) {
                    break;
                }
            }
        }
        if (plineTemp == LW_NULL) {                                     /*  �޷���������                */
            goto    __find_error;
        }
        
        *pprfsnFather = prfsn;                                          /*  �ӵ�ǰ�ڵ㿪ʼ����          */
        plineHeader   = prfsn->RFSN_plineSon;                           /*  �ӵ�һ�����ӿ�ʼ            */
        
    } while (pcNext);                                                   /*  �������¼�Ŀ¼              */
    
__find_ok:
    *pprfsnFather = prfsn->RFSN_prfsnFather;                            /*  ��ϵ�ڵ�                    */
    /*
     *  ���� tail ��λ��.
     */
    if (ppcTail) {
        if (pcNext) {
            INT   iTail = pcNext - pcTempName;
            *ppcTail = (PCHAR)pcName + iTail;                           /*  ָ��û�б������ / �ַ�     */
        } else {
            *ppcTail = (PCHAR)pcName + lib_strlen(pcName);              /*  ָ����ĩβ                  */
        }
    }
    return  (prfsn);
    
__find_error:
    if (pbLast) {
        if (pcNext == LW_NULL) {                                        /*  ���һ������ʧ��            */
            *pbLast = LW_TRUE;
        } else {
            *pbLast = LW_FALSE;
        }
    }
    return  (LW_NULL);                                                  /*  �޷��ҵ��ڵ�                */
}
/*********************************************************************************************************
** ��������: __rootFsDevMatch
** ��������: rootfs ƥ��һ���豸 (�豸������� / ���ſ�ʼ)
** �䡡��  : pcName            �豸��
** �䡡��  : �豸�ڵ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_DEV_HDR  __rootFsDevMatch (CPCHAR  pcName)
{
    PLW_ROOTFS_NODE    prfsnFather;
    PLW_ROOTFS_NODE    prfsn;
    BOOL               bIsRoot;
    PLW_DEV_HDR        pdevhdrFind = LW_NULL;
    
    if ((pcName == LW_NULL) || (pcName[0] != PX_ROOT)) {                /*  ����豸����Ч��            */
        return  (LW_NULL);
    }
    
    /*
     *  ���豸���⴦��
     */
    if (lib_strcmp(pcName, PX_STR_ROOT) == 0) {                         /*  Ѱ�� root �豸              */
        if (_G_devhdrRoot.DEVHDR_usDrvNum) {
            return  (&_G_devhdrRoot);                                   /*  �����װ�˸��豸            */
        } else {
            return  (LW_NULL);                                          /*  û�а�װ���豸              */
        }
    }
    
    /*
     *  pcName ���������ǴӸ�Ŀ¼���ſ�ʼ���豸��
     */
    __LW_ROOTFS_LOCK();                                                 /*  ���� rootfs                 */
    prfsn = __rootFsFindNode(pcName, &prfsnFather, &bIsRoot, LW_NULL, LW_NULL);
                                                                        /*  ��ѯ�豸                    */
    if (prfsn) {
        if (prfsn->RFSN_iNodeType == LW_ROOTFS_NODE_TYPE_DEV) {         /*  ���ҵ��豸                  */
            pdevhdrFind = prfsn->RFSN_rfsnv.RFSNV_pdevhdr;
        } else {
            pdevhdrFind = &_G_devhdrRoot;                               /*  ���ҵ����豸�µ�Ŀ¼������  */
        }
    
    } else if (prfsnFather) {                                           /*  û���ҵ��豸, ���ҵ���ϵ�ڵ�*/
        if (prfsnFather->RFSN_iNodeType == LW_ROOTFS_NODE_TYPE_DEV) {   /*  �и�ϵ�ڵ����, ��Ϊ�豸��  */
            /*
             *  �ж������Ƿ���� tail. �����豸Ϊ: /dev/temp �� pcName Ϊ /dev/temp/aaa/bbb/...
             */
            pdevhdrFind = prfsnFather->RFSN_rfsnv.RFSNV_pdevhdr;        /*  ʹ�ø�ϵ�豸                */
        
        } else if (prfsnFather->RFSN_iNodeType == LW_ROOTFS_NODE_TYPE_DIR) {
            pdevhdrFind = &_G_devhdrRoot;                               /*  ���ΪĿ¼, ��Ϊ���ļ�ϵͳ  */
        }
    
    } else if (pcName[0] == PX_ROOT) {                                  /*  �Ӹ�Ŀ¼��ʼ                */
        if (_G_devhdrRoot.DEVHDR_usDrvNum) {
            pdevhdrFind = &_G_devhdrRoot;
        }
    }
    __LW_ROOTFS_UNLOCK();                                               /*  ���� rootfs                 */
    
    return  (pdevhdrFind);
}
/*********************************************************************************************************
** ��������: __rootFsReadNode
** ��������: rootfs ��ȡһ���ڵ�, (�ڵ�ֻ�������ӽڵ�) 
** �䡡��  : pcName        �ڵ���
**           pcBuffer      ���ݻ���
**           stSize        �����С
** �䡡��  : ��ȡ�Ĵ�С
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  __rootFsReadNode (CPCHAR  pcName, PCHAR  pcBuffer, size_t  stSize)
{
    PLW_ROOTFS_NODE    prfsnFather = LW_NULL;
    PLW_ROOTFS_NODE    prfsn;
    BOOL               bIsRoot = LW_FALSE;
    CHAR               cFullPathName[MAX_FILENAME_LENGTH];
    
    ssize_t            sstRet  = PX_ERROR;
    
    if ((pcName == LW_NULL)   || 
        (pcBuffer == LW_NULL) || 
        (stSize == 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    /*
     *  ���豸���⴦��
     */
    if ((pcName[0] == PX_EOS) || 
        (lib_strcmp(pcName, PX_STR_ROOT) == 0)) {                       /*  ���ܶ�ȡ���豸              */
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    if (_PathCat(_PathGetDef(), pcName, cFullPathName) != ERROR_NONE) { /*  ��ôӸ�Ŀ¼��ʼ��·��      */
        _ErrorHandle(ENAMETOOLONG);
        return  (PX_ERROR);
    }
    
    __LW_ROOTFS_LOCK();                                                 /*  ���� rootfs                 */
    prfsn = __rootFsFindNode(cFullPathName, &prfsnFather, 
                             &bIsRoot, LW_NULL, LW_NULL);               /*  ��ѯ�豸                    */
    if (prfsn) {
        if (prfsn->RFSN_iNodeType == LW_ROOTFS_NODE_TYPE_LNK) {
            size_t  stLen = lib_strlen(prfsn->RFSN_pcLink);
            lib_strncpy(pcBuffer, prfsn->RFSN_pcLink, stSize);          /*  ������������                */
            if (stLen > stSize) {
                stLen = stSize;                                         /*  ������Ч�ֽ���              */
            }
            sstRet = (ssize_t)stLen;
            
        } else {
            _ErrorHandle(ENOENT);
        }
    }
    __LW_ROOTFS_UNLOCK();                                               /*  ���� rootfs                 */
    
    return  (sstRet);
}
/*********************************************************************************************************
** ��������: API_RootFsMakeNode
** ��������: rootfs ����һ���ڵ�
** �䡡��  : pcName        ȫ�� (�Ӹ���ʼ)
**           iNodeType     �ڵ�����     Ϊ�豸ʱ pvValue Ϊ PLW_DEV_HDR ָ��
                                        ΪĿ¼ʱ pvValue Ϊ LW_NULL ָ��
                                        Ϊ����ʱ pvValue Ϊ ����·��ָ��
             iNodeOpt      �ڵ�ѡ��
             iMode         mode_t
**           pvValue       ����
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_RootFsMakeNode (CPCHAR  pcName, INT  iNodeType, INT  iNodeOpt, INT  iMode, PVOID  pvValue)
{
    PLW_ROOTFS_NODE    prfsnFather = LW_FALSE;
    PLW_ROOTFS_NODE    prfsn;
    BOOL               bIsRoot = LW_FALSE;
    BOOL               bLast = LW_FALSE;
    CHAR               cFullPathName[MAX_FILENAME_LENGTH];
    PCHAR              pcTail = LW_NULL;
    
    size_t             stAllocSize;
    INT                iError = PX_ERROR;
    PLW_ROOTFS_NODE    prfsnNew;

    iMode &= ~S_IFMT;                                                   /*  ȥ��������Ϣ                */
    
    if ((pcName == LW_NULL) || (*pcName != PX_ROOT)) {                  /*  ·����                      */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (iNodeType > LW_ROOTFS_NODE_TYPE_REG) {                          /*  ���ڵ�����                */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if ((iNodeType != LW_ROOTFS_NODE_TYPE_DIR) &&
        (iNodeType != LW_ROOTFS_NODE_TYPE_SOCK) &&
        (iNodeType != LW_ROOTFS_NODE_TYPE_REG) &&
        (pvValue == LW_NULL)) {                                         /*  ��Ҫ����������              */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    /*
     *  ���豸���⴦��
     */
    if (lib_strcmp(pcName, PX_STR_ROOT) == 0) {                         /*  ��Ӹ��豸����              */
        return  (ERROR_NONE);
    }
    
    if (_PathCat(_PathGetDef(), pcName, cFullPathName) != ERROR_NONE) { /*  ��ôӸ�Ŀ¼��ʼ��·��      */
        _ErrorHandle(ENAMETOOLONG);
        return  (PX_ERROR);
    }
    
    /*
     *  �����������ͽڵ��ڴ�
     */
    if (iNodeType == LW_ROOTFS_NODE_TYPE_DIR) {                         /*  Ŀ¼��                      */
        /*
         *  Ŀ¼�ڵ�����
         */
        PCHAR   pcDirName = lib_rindex(cFullPathName, PX_DIVIDER);      /*  ����ļ���, һ����Ϊ NULL   */
        size_t  stLen;
        
        pcDirName++;                                                    /*  ָ���ļ����ĵ�һ���ڵ�      */
        stLen = lib_strlen(pcDirName);
        if (stLen == 0) {
            _ErrorHandle(EINVAL);                                       /*  Ŀ¼�ļ�������              */
            return  (PX_ERROR);
        }
        /*
         *  �����ڴ��Ƭ�˷�, �������һ��Ƭ�ڴ�.
         */
        stLen++;                                                        /*  Ԥ�� \0 �ռ�                */
        stAllocSize = sizeof(LW_ROOTFS_NODE) + stLen;
        prfsnNew = (PLW_ROOTFS_NODE)__SHEAP_ALLOC(stAllocSize);         /*  ����ڵ��ڴ�                */
        if (prfsnNew) {
            prfsnNew->RFSN_rfsnv.RFSNV_pcName = (PCHAR)prfsnNew + sizeof(LW_ROOTFS_NODE);
            lib_strcpy(prfsnNew->RFSN_rfsnv.RFSNV_pcName, pcDirName);
            
            prfsnNew->RFSN_pcLink = LW_NULL;                            /*  �������ļ�                  */
            prfsnNew->RFSN_mode   = (iMode | S_IFDIR);
        }
    } else if (iNodeType == LW_ROOTFS_NODE_TYPE_DEV) {                  /*  �豸��                      */
        /*
         *  �豸�ڵ�����
         */
        stAllocSize = sizeof(LW_ROOTFS_NODE);
        prfsnNew = (PLW_ROOTFS_NODE)__SHEAP_ALLOC(sizeof(LW_ROOTFS_NODE));
                                                                        /*  ����ڵ��ڴ�                */
        if (prfsnNew) {
            prfsnNew->RFSN_rfsnv.RFSNV_pdevhdr = (PLW_DEV_HDR)pvValue;  /*  �����豸ͷ                  */
        
            prfsnNew->RFSN_pcLink = LW_NULL;                            /*  �������ļ�                  */
            prfsnNew->RFSN_mode   = iMode;
        }
        
    } else if ((iNodeType == LW_ROOTFS_NODE_TYPE_SOCK) ||
               (iNodeType == LW_ROOTFS_NODE_TYPE_REG)) {                /*  socket or reg file                 */
        /*
         *  AF_UNIX �ļ�
         */
        PCHAR   pcSockName = lib_rindex(cFullPathName, PX_DIVIDER);     /*  ����ļ���, һ����Ϊ NULL   */
        size_t  stLen;
        
        if (pcSockName == LW_NULL) {
            pcSockName = cFullPathName;
        } else {
            pcSockName++;                                               /*  ָ���ļ����ĵ�һ���ڵ�      */
        }
        
        stLen = lib_strlen(pcSockName);
        if (stLen == 0) {
            _ErrorHandle(EINVAL);                                       /*  socket �ļ�������           */
            return  (PX_ERROR);
        }
        /*
         *  �����ڴ��Ƭ�˷�, �������һ��Ƭ�ڴ�.
         */
        stLen++;                                                        /*  Ԥ�� \0 �ռ�                */
        stAllocSize = sizeof(LW_ROOTFS_NODE) + stLen;
        prfsnNew = (PLW_ROOTFS_NODE)__SHEAP_ALLOC(stAllocSize);         /*  ����ڵ��ڴ�                */
        if (prfsnNew) {
            prfsnNew->RFSN_rfsnv.RFSNV_pcName = (PCHAR)prfsnNew + sizeof(LW_ROOTFS_NODE);
            lib_strcpy(prfsnNew->RFSN_rfsnv.RFSNV_pcName, pcSockName);
            
            prfsnNew->RFSN_pcLink = LW_NULL;                            /*  �������ļ�                  */
            if (iNodeType == LW_ROOTFS_NODE_TYPE_SOCK) {
                prfsnNew->RFSN_mode = (iMode | S_IFSOCK);               /*  socket Ĭ��Ϊ 0777          */
            } else {
                prfsnNew->RFSN_mode = (iMode | S_IFREG);
            }
        }
    } else {                                                            /*  �����ļ�                    */
        /*
         *  ���ӽڵ�����
         */
        PCHAR   pcDirName = lib_rindex(cFullPathName, PX_DIVIDER);      /*  ����ļ���, һ����Ϊ NULL   */
        size_t  stLen;
        size_t  stLinkLen = lib_strlen((PCHAR)pvValue);
        
        pcDirName++;                                                    /*  ָ���ļ����ĵ�һ���ڵ�      */
        stLen = lib_strlen(pcDirName);
        if ((stLen == 0) || (stLinkLen == 0)) {
            _ErrorHandle(EINVAL);                                       /*  �ļ�������                  */
            return  (PX_ERROR);
        }
        /*
         *  �����ڴ��Ƭ�˷�, �������һ��Ƭ�ڴ�.
         */
        stAllocSize = sizeof(LW_ROOTFS_NODE) + stLen;
        prfsnNew = (PLW_ROOTFS_NODE)__SHEAP_ALLOC(stAllocSize);         /*  ����ڵ��ڴ�                */
        if (prfsnNew) {
            prfsnNew->RFSN_rfsnv.RFSNV_pcName = (PCHAR)prfsnNew + sizeof(LW_ROOTFS_NODE);
            lib_strcpy(prfsnNew->RFSN_rfsnv.RFSNV_pcName, pcDirName);
        
            stLinkLen++;                                                /*  Ԥ�� \0 ��λ��              */
            prfsnNew->RFSN_pcLink = (PCHAR)__SHEAP_ALLOC(stLinkLen);
            if (prfsnNew->RFSN_pcLink) {
                lib_strcpy(prfsnNew->RFSN_pcLink, (PCHAR)pvValue);      /*  ������������                */
                stAllocSize += stLinkLen;
            
            } else {
                __SHEAP_FREE(prfsnNew);                                 /*  ȱ���ڴ�                    */
                prfsnNew = LW_NULL;
            }
        }
        
        /*
         *  �������ļ�������Ϊ�����Ӷ���һ��.
         */
        if (prfsnNew) {
            struct stat   statBuf;
            
            if (stat((PCHAR)pvValue, &statBuf) < ERROR_NONE) {
                prfsnNew->RFSN_mode  = (iMode | S_IFLNK);
            } else {
                prfsnNew->RFSN_mode  = (statBuf.st_mode & ~S_IFMT);     /*  ���� file type ��Ϣ         */
                prfsnNew->RFSN_mode |= S_IFLNK;
            }
        }
    }
    
    if (prfsnNew == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    prfsnNew->RFSN_plineSon    = LW_NULL;                               /*  û���κζ���                */
    prfsnNew->RFSN_iOpenNum    = 0;
    prfsnNew->RFSN_iNodeType   = iNodeType;
    prfsnNew->RFSN_stAllocSize = stAllocSize;
    
    prfsnNew->RFSN_uid = getuid();
    prfsnNew->RFSN_gid = getgid();
    
    if (iNodeOpt & LW_ROOTFS_NODE_OPT_ROOTFS_TIME) {
        prfsnNew->RFSN_time = (time_t)(PX_ERROR);                       /*  �̳� rootfs ʱ��            */
    } else {
        prfsnNew->RFSN_time = lib_time(LW_NULL);                        /*  �� UTC ʱ����Ϊʱ���׼     */
    }
    
    __LW_ROOTFS_LOCK();                                                 /*  ���� rootfs                 */
    prfsn = __rootFsFindNode(cFullPathName, &prfsnFather, 
                             &bIsRoot, &bLast, &pcTail);                /*  ��ѯ�豸                    */
    if (prfsn) {
        if ((pcTail && *pcTail == PX_ROOT) &&
            (prfsn->RFSN_iNodeType == LW_ROOTFS_NODE_TYPE_LNK)) {
            _ErrorHandle(ERROR_IOS_FILE_SYMLINK);                       /*  ���ڵ�Ϊ symlink ����       */
        } else {
            _ErrorHandle(ERROR_IOS_DUPLICATE_DEVICE_NAME);              /*  �豸����                    */
        }
    } else {
        if (prfsnFather && prfsnFather->RFSN_iNodeType) {               /*  ��ϵ�ڵ㲻ΪĿ¼            */
            /*
             *  �˽ڵ�ĸ��ױ���Ϊһ��Ŀ¼!
             */
            _ErrorHandle(ENOENT);
        
        } else if (bLast == LW_FALSE) {                                 /*  ȱ���м�Ŀ¼��              */
            _ErrorHandle(ENOENT);                                       /*  XXX errno ?                 */
        
        } else {
            if (prfsnFather) {
                prfsnNew->RFSN_prfsnFather = prfsnFather;
                _List_Line_Add_Ahead(&prfsnNew->RFSN_lineBrother,
                                     &prfsnFather->RFSN_plineSon);      /*  ����ָ��������              */
            } else {
                prfsnNew->RFSN_prfsnFather = LW_NULL;
                _List_Line_Add_Ahead(&prfsnNew->RFSN_lineBrother,
                                     &_G_rfsrRoot.RFSR_plineSon);       /*  ������ڵ�                  */
            }
            iError = ERROR_NONE;
        }
    }
    __LW_ROOTFS_UNLOCK();                                               /*  ���� rootfs                 */
    
    if (iError) {
        if (prfsnNew->RFSN_pcLink) {
            __SHEAP_FREE(prfsnNew->RFSN_pcLink);                        /*  �ͷ������ļ�����            */
        }
        __SHEAP_FREE(prfsnNew);                                         /*  �ͷ��ڴ�                    */
    } else {
        _G_rfsrRoot.RFSR_stMemUsed += stAllocSize;                      /*  �����ڴ�ʹ����              */
        _G_rfsrRoot.RFSR_ulFiles   += 1;                                /*  ����һ���ļ�                */
    }
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: API_RootFsRemoveNode
** ��������: rootfs ɾ��һ���ڵ�, 
** �䡡��  : pcName        �ڵ���
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_RootFsRemoveNode (CPCHAR  pcName)
{
    PLW_ROOTFS_NODE    prfsnFather = LW_NULL;
    PLW_ROOTFS_NODE    prfsn;
    BOOL               bIsRoot = LW_FALSE;
    CHAR               cFullPathName[MAX_FILENAME_LENGTH];
    
    INT                iError = PX_ERROR;
    
    if (pcName == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    /*
     *  ���豸���⴦��
     */
    if ((pcName[0] == PX_EOS) || 
        (lib_strcmp(pcName, PX_STR_ROOT) == 0)) {                       /*  �����Ƴ����豸              */
        return  (ERROR_NONE);
    }
    
    if (_PathCat(_PathGetDef(), pcName, cFullPathName) != ERROR_NONE) { /*  ��ôӸ�Ŀ¼��ʼ��·��      */
        _ErrorHandle(ENAMETOOLONG);
        return  (PX_ERROR);
    }
    
    __LW_ROOTFS_LOCK();                                                 /*  ���� rootfs                 */
    prfsn = __rootFsFindNode(cFullPathName, &prfsnFather, 
                             &bIsRoot, LW_NULL, LW_NULL);               /*  ��ѯ�豸                    */
    if (prfsn) {
        if (prfsn->RFSN_plineSon) {
            _ErrorHandle(ENOTEMPTY);                                    /*  ��Ϊ��                      */
        } else if (prfsn->RFSN_iOpenNum) {
            _ErrorHandle(EBUSY);                                        /*  �ڵ�û�йر�                */
        } else {
            if (prfsnFather == LW_NULL) {
                _List_Line_Del(&prfsn->RFSN_lineBrother,
                               &_G_rfsrRoot.RFSR_plineSon);             /*  �Ӹ��ڵ�ж��                */
            } else {
                _List_Line_Del(&prfsn->RFSN_lineBrother,
                               &prfsnFather->RFSN_plineSon);            /*  �Ӹ��ڵ�ж��                */
            }
            iError = ERROR_NONE;
        }
    
    } else {
        _ErrorHandle(ENOENT);                                           /*  û�ж�Ӧ�Ľڵ�              */
    }
    __LW_ROOTFS_UNLOCK();                                               /*  ���� rootfs                 */
    
    if (iError == ERROR_NONE) {
        _G_rfsrRoot.RFSR_stMemUsed -= prfsn->RFSN_stAllocSize;
        _G_rfsrRoot.RFSR_ulFiles   -= 1;                                /*  ����һ���ļ�                */
        if (prfsn->RFSN_pcLink) {
            __SHEAP_FREE(prfsn->RFSN_pcLink);                           /*  �ͷ������ļ�����            */
        }
        __SHEAP_FREE(prfsn);                                            /*  �ͷ��ڴ�                    */
    }
    
    return  (iError);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_PATH_VXWORKS == 0    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
