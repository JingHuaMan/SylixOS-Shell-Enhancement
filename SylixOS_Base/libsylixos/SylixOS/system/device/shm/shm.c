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
** ��   ��   ��: shm.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 12 �� 27 ��
**
** ��        ��: �����ڴ��豸. 
                 ���豸�µ��ļ�ֻ��ͨ��Ψһһ�� truncat �����ı��ļ���С, Ȼ��ͨ�� mmap ����, ����ʹ��
                 read / write ���з���.
** BUG:
2013.03.13  ��һ�� mmap ʱ���������ڴ�, ���һ�� munmap ʱ�ͷ������ڴ�.
2013.03.16  ����ʹ�� DMA �ڴ�, ת��ʹ��ͨ�������ڴ�.
2013.03.17  __shmMmap() ֱ��ʹ�� mmap ȷ����ӳ�� flag.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
#include "../SylixOS/fs/include/fs_fs.h"                                /*  ��Ҫ���ļ�ϵͳʱ��          */
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0 && LW_CFG_SHM_DEVICE_EN > 0 && LW_CFG_VMM_EN > 0 
/*********************************************************************************************************
  �ļ��ڵ�
*********************************************************************************************************/
typedef struct lw_shm_node {
    LW_LIST_LINE                 SHMN_lineBrother;                      /*  �ֵܽڵ�                    */
    struct lw_shm_node          *SHMN_pshmnFather;                      /*  ��ϵ�ڵ�                    */
    PLW_LIST_LINE                SHMN_plineSon;                         /*  ���ӽڵ�                    */
    PCHAR                        SHMN_pcName;                           /*  �ڵ���                      */
    INT                          SHMN_iOpenNum;                         /*  �򿪵Ĵ���                  */
    off_t                        SHMN_oftSize;                          /*  �ļ���С                    */
    PVOID                        SHMN_pvPhyMem;                         /*  �����ڴ�, ����ʱ����������  */
#define SHMN_pvLink              SHMN_pvPhyMem
    ULONG                        SHMN_ulMapCnt;                         /*  ӳ�������                  */
    mode_t                       SHMN_mode;                             /*  �ڵ�����                    */
    time_t                       SHMN_time;                             /*  �ڵ�ʱ��, һ��Ϊ��ǰʱ��    */
    uid_t                        SHMN_uid;
    gid_t                        SHMN_gid;
} LW_SHM_NODE;
typedef LW_SHM_NODE             *PLW_SHM_NODE;
/*********************************************************************************************************
  shm��
*********************************************************************************************************/
typedef struct lw_shm_root {
    PLW_LIST_LINE                SHMR_plineSon;                         /*  ָ���һ������              */
    size_t                       SHMR_stMemUsed;                        /*  �ڴ�������                  */
    time_t                       SHMR_time;                             /*  ����ʱ��                    */
} LW_SHM_ROOT;
typedef LW_SHM_ROOT             *PLW_SHM_ROOT;
/*********************************************************************************************************
  �豸�ṹ
*********************************************************************************************************/
static LW_DEV_HDR                _G_devhdrShm;                          /*  �����ڴ��豸                */
/*********************************************************************************************************
  ����ȫ�ֱ���
*********************************************************************************************************/
static LW_SHM_ROOT               _G_shmrRoot;
static LW_OBJECT_HANDLE          _G_ulShmLock;                          /*  procFs ������               */
static INT                       _G_iShmDrvNum = PX_ERROR;
/*********************************************************************************************************
  ����ȫ�ֱ���
*********************************************************************************************************/
#define __LW_SHM_LOCK()          API_SemaphoreMPend(_G_ulShmLock, LW_OPTION_WAIT_INFINITE)
#define __LW_SHM_UNLOCK()        API_SemaphoreMPost(_G_ulShmLock)
/*********************************************************************************************************
  ���·���ִ��Ƿ�Ϊ��Ŀ¼����ֱ��ָ���豸
*********************************************************************************************************/
#define __STR_IS_ROOT(pcName)    ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))
/*********************************************************************************************************
  �ڵ������ڴ����
*********************************************************************************************************/
static INT  __shmPhymemAlloc(PLW_SHM_NODE  pshmn);
static INT  __shmPhymemFree(PLW_SHM_NODE  pshmn);
/*********************************************************************************************************
** ��������: __shmFindNode
** ��������: �����ڴ��豸����һ���ڵ�
** �䡡��  : pcName            �ڵ��� (����� procfs ����ʼ)
**           ppshmnFather      ���޷��ҵ��ڵ�ʱ������ӽ���һ��,
                               ��Ѱ�ҵ��ڵ�ʱ���游ϵ�ڵ�. 
                               LW_NULL ��ʾ��
**           pbRoot            �ڵ����Ƿ�ָ����ڵ�.
**           pbLast            ��ƥ��ʧ��ʱ, �Ƿ������һ���ļ�ƥ��ʧ��
**           ppcTail           ����β��, ���������豸��������ʱ, tail ָ������β.
** �䡡��  : �ڵ�, LW_NULL ��ʾ���ڵ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_SHM_NODE  __shmFindNode (CPCHAR            pcName, 
                                    PLW_SHM_NODE     *ppshmnFather, 
                                    BOOL             *pbRoot,
                                    BOOL             *pbLast,
                                    PCHAR            *ppcTail)
{
    static CHAR         pcTempName[MAX_FILENAME_LENGTH];
    PCHAR               pcNext;
    PCHAR               pcNode;
    
    PLW_SHM_NODE        pshmn;
    PLW_SHM_NODE        pshmnTemp;
    
    PLW_LIST_LINE       plineTemp;
    PLW_LIST_LINE       plineHeader;                                    /*  ��ǰĿ¼ͷ                  */
    
    if (pcName == LW_NULL) {
__param_error:
        if (pbRoot) {
            *pbRoot = LW_FALSE;                                         /*  pcName ��Ϊ��               */
        }
        if (pbLast) {
            *pbLast = LW_FALSE;
        }
        return  (LW_NULL);
    }
    
    if (ppshmnFather == LW_NULL) {
        ppshmnFather = &pshmnTemp;                                      /*  ��ʱ����                    */
    }
    *ppshmnFather = LW_NULL;
    
    if (*pcName == PX_ROOT) {                                           /*  ���Ը�����                  */
        lib_strlcpy(pcTempName, (pcName + 1), PATH_MAX);
    } else {
        goto    __param_error;
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
    plineHeader = _G_shmrRoot.SHMR_plineSon;                            /*  �Ӹ�Ŀ¼��ʼ����            */
    
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
             
            pshmn = _LIST_ENTRY(plineTemp, LW_SHM_NODE, SHMN_lineBrother);
            if (S_ISLNK(pshmn->SHMN_mode)) {                            /*  �����ļ�                    */
                if (lib_strcmp(pshmn->SHMN_pcName, pcNode) == 0) {
                    goto    __find_ok;                                  /*  �ҵ�����                    */
                }
            
            } else if (S_ISSOCK(pshmn->SHMN_mode) || S_ISREG(pshmn->SHMN_mode)) {
                if (lib_strcmp(pshmn->SHMN_pcName, pcNode) == 0) {
                    if (pcNext) {                                       /*  �������¼�, �������ΪĿ¼  */
                        goto    __find_error;                           /*  ����Ŀ¼ֱ�Ӵ���            */
                    }
                    break;
                }
            
            } else {                                                    /*  Ŀ¼�ڵ�                    */
                if (lib_strcmp(pshmn->SHMN_pcName, pcNode) == 0) {
                    break;
                }
            }
        }
        if (plineTemp == LW_NULL) {                                     /*  �޷���������                */
            goto    __find_error;
        }
        
        *ppshmnFather = pshmn;                                          /*  �ӵ�ǰ�ڵ㿪ʼ����          */
        plineHeader   = pshmn->SHMN_plineSon;                           /*  �ӵ�һ�����ӿ�ʼ            */
        
    } while (pcNext);                                                   /*  �������¼�Ŀ¼              */
    
__find_ok:
    *ppshmnFather = pshmn->SHMN_pshmnFather;                            /*  ��ϵ�ڵ�                    */
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
    return  (pshmn);
    
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
** ��������: __shmMakeNode
** ��������: �����ڴ��豸����һ���ڵ�
** �䡡��  : pcName        ȫ�� (�Ӹ���ʼ)
**           iFlags        ��ʽ
**           iMode         mode_t
**           pcLink        ����������ļ�, ��������Ŀ��
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __shmMakeNode (CPCHAR  pcName, INT  iFlags, INT  iMode, CPCHAR  pcLink)
{
    PLW_SHM_NODE     pshmn       = LW_NULL;
    PLW_SHM_NODE     pshmnFather = LW_NULL;
    BOOL             bIsRoot     = LW_FALSE;
    BOOL             bLast       = LW_FALSE;
    PCHAR            pcTail      = LW_NULL;
    
    size_t           stAllocSize;
    INT              iError = PX_ERROR;
    PLW_SHM_NODE     pshmnNew;
    CPCHAR           pcLastName;
    size_t           stLen;
    
    if ((pcName == LW_NULL) || (*pcName != PX_ROOT)) {                  /*  ·����                      */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (S_ISLNK(iMode) && pcLink == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if ((iMode & S_IFMT) == 0) {
        iMode |= S_IFREG;                                               /*  Ĭ��Ϊ REG �ļ�             */
    }
    
    pcLastName = lib_rindex(pcName, PX_DIVIDER);
    if (pcLastName == LW_NULL) {
        pcLastName = pcName;
    } else {
        pcLastName++;                                                   /*  ָ���ļ����ĵ�һ���ڵ�      */
    }
    
    stLen = lib_strlen(pcLastName);
    if (stLen == 0) {
        _ErrorHandle(EINVAL);                                           /*  socket �ļ�������           */
        return  (PX_ERROR);
    }
    
    stLen++;                                                            /*  Ԥ�� \0 �ռ�                */
    stAllocSize = (size_t)(sizeof(LW_SHM_NODE) + stLen);
    pshmnNew    = (PLW_SHM_NODE)__SHEAP_ALLOC(stAllocSize);
    if (!pshmnNew) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    pshmnNew->SHMN_pcName = (PCHAR)pshmnNew + sizeof(LW_SHM_NODE);
    lib_strcpy(pshmnNew->SHMN_pcName, pcLastName);
    
    if (S_ISLNK(iMode)) {
        pshmnNew->SHMN_pvLink = __SHEAP_ALLOC(lib_strlen(pcLink) + 1);
        if (pshmnNew->SHMN_pvLink == LW_NULL) {
            __SHEAP_FREE(pshmnNew);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        lib_strcpy((PCHAR)pshmnNew->SHMN_pvLink, pcLink);
    } else {
        pshmnNew->SHMN_pvPhyMem = LW_NULL;
    }
    
    pshmnNew->SHMN_plineSon = LW_NULL;                                  /*  û���κζ���                */
    pshmnNew->SHMN_iOpenNum = 0;
    pshmnNew->SHMN_oftSize  = 0;
    pshmnNew->SHMN_ulMapCnt = 0;                                        /*  û��ӳ��                    */
    pshmnNew->SHMN_mode     = iMode;
    pshmnNew->SHMN_time     = lib_time(LW_NULL);                        /*  �� UTC ʱ����Ϊʱ���׼     */
    pshmnNew->SHMN_uid      = getuid();
    pshmnNew->SHMN_gid      = getgid();
    
    __LW_SHM_LOCK();
    pshmn = __shmFindNode(pcName, &pshmnFather, 
                          &bIsRoot, &bLast, &pcTail);                   /*  ��ѯ�豸                    */
    if (pshmn) {
        if ((pcTail && *pcTail == PX_ROOT) && 
            S_ISLNK(pshmn->SHMN_mode)) {
            _ErrorHandle(ERROR_IOS_FILE_SYMLINK);                       /*  ���ڵ�Ϊ symlink ����       */
        } else {
            _ErrorHandle(ERROR_IOS_DUPLICATE_DEVICE_NAME);              /*  �豸����                    */
        }
    } else {
        if (pshmnFather && !S_ISDIR(pshmnFather->SHMN_mode)) {          /*  ��ϵ�ڵ㲻ΪĿ¼            */
            _ErrorHandle(ENOENT);
        
        } else if (bLast == LW_FALSE) {                                 /*  ȱ���м�Ŀ¼��              */
            _ErrorHandle(ENOENT);                                       /*  XXX errno ?                 */
        
        } else {
            if (pshmnFather) {
                pshmnNew->SHMN_pshmnFather = pshmnFather;
                _List_Line_Add_Ahead(&pshmnNew->SHMN_lineBrother,
                                     &pshmnFather->SHMN_plineSon);      /*  ����ָ��������              */
            } else {
                pshmnNew->SHMN_pshmnFather = LW_NULL;
                _List_Line_Add_Ahead(&pshmnNew->SHMN_lineBrother,
                                     &_G_shmrRoot.SHMR_plineSon);       /*  ������ڵ�                  */
            }
            iError = ERROR_NONE;
        }
    }
    __LW_SHM_UNLOCK();                                                  /*  ���������ڴ��豸            */
    
    if (iError) {
        if (S_ISLNK(iMode)) {
            __SHEAP_FREE(pshmnNew->SHMN_pvLink);                        /*  �ͷ������ļ�����            */
        }
        __SHEAP_FREE(pshmnNew);                                         /*  �ͷ��ڴ�                    */
    }
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __shmRemoveNode
** ��������: �����ڴ��豸ɾ��һ���ڵ�, 
** �䡡��  : pcName        �ڵ���
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __shmRemoveNode (CPCHAR  pcName)
{
    PLW_SHM_NODE    pshmnFather = LW_NULL;
    PLW_SHM_NODE    pshmn;
    BOOL            bIsRoot = LW_FALSE;
    INT             iError = PX_ERROR;
    
    if (pcName == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    /*
     *  �豸���⴦��
     */
    if ((pcName[0] == PX_EOS) || 
        (lib_strcmp(pcName, PX_STR_ROOT) == 0)) {                       /*  �����Ƴ����豸              */
        return  (ERROR_NONE);
    }
    
    __LW_SHM_LOCK();
    pshmn = __shmFindNode(pcName, &pshmnFather, 
                          &bIsRoot, LW_NULL, LW_NULL);                  /*  ��ѯ�豸                    */
    if (pshmn) {
        if (pshmn->SHMN_plineSon) {
            _ErrorHandle(ENOTEMPTY);                                    /*  ��Ϊ��                      */
        } else if (pshmn->SHMN_iOpenNum) {
            _ErrorHandle(EBUSY);                                        /*  �ڵ�û�йر�                */
        } else {
            if (pshmnFather == LW_NULL) {
                _List_Line_Del(&pshmn->SHMN_lineBrother,
                               &_G_shmrRoot.SHMR_plineSon);             /*  �Ӹ��ڵ�ж��                */
            } else {
                _List_Line_Del(&pshmn->SHMN_lineBrother,
                               &pshmnFather->SHMN_plineSon);            /*  �Ӹ��ڵ�ж��                */
            }
            iError = ERROR_NONE;
        }
    }
    __LW_SHM_UNLOCK();                                                  /*  ���������ڴ��豸            */
    
    if (iError == ERROR_NONE) {
        if (S_ISLNK(pshmn->SHMN_mode)) {
            __SHEAP_FREE(pshmn->SHMN_pvLink);                           /*  �ͷ������ļ�����            */
        } else if (pshmn->SHMN_pvPhyMem) {
            __shmPhymemFree(pshmn);                                     /*  �ͷŹ���������ڴ�          */
        }
        __SHEAP_FREE(pshmn);                                            /*  �ͷ��ڴ�                    */
    }
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __shmPhymemAlloc
** ��������: �����ڴ��豸�ڵ㴴�������ڴ�, 
** �䡡��  : pshmn               �ļ��ڵ�
**           oftSize             �ļ���С
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __shmPhymemAlloc (PLW_SHM_NODE  pshmn)
{
    REGISTER ULONG  ulPageNum = (ULONG) (pshmn->SHMN_oftSize >> LW_CFG_VMM_PAGE_SHIFT);
    REGISTER size_t stExcess  = (size_t)(pshmn->SHMN_oftSize & ~LW_CFG_VMM_PAGE_MASK);
    
             size_t stRealSize;
             
    if (stExcess) {
        ulPageNum++;
    }
    
    stRealSize = (size_t)(ulPageNum << LW_CFG_VMM_PAGE_SHIFT);
    if (stRealSize == 0) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pshmn->SHMN_pvPhyMem = API_VmmPhyAlloc(stRealSize);
    if (pshmn->SHMN_pvPhyMem == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    
    _G_shmrRoot.SHMR_stMemUsed += (size_t)pshmn->SHMN_oftSize;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __shmPhymemFree
** ��������: �����ڴ��豸�ڵ��ͷ������ڴ�, 
** �䡡��  : pshmn               �ļ��ڵ�
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __shmPhymemFree (PLW_SHM_NODE  pshmn)
{
    if (pshmn->SHMN_pvPhyMem) {
        API_VmmPhyFree(pshmn->SHMN_pvPhyMem);                           /*  �ͷŹ���������ڴ�          */
        _G_shmrRoot.SHMR_stMemUsed -= (size_t)pshmn->SHMN_oftSize;
        pshmn->SHMN_pvPhyMem = LW_NULL;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __shmOpen
** ��������: �����ڴ��豸 open ����
** �䡡��  : pdevhdr          shm�豸
**           pcName           �ļ���
**           iFlags           ��ʽ
**           iMode            mode_t
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LONG  __shmOpen (LW_DEV_HDR     *pdevhdr,
                        PCHAR           pcName,
                        INT             iFlags,
                        INT             iMode)
{
    PLW_SHM_NODE     pshmn       = LW_NULL;
    PLW_SHM_NODE     pshmnFather = LW_NULL;
    BOOL             bIsRoot;
    PCHAR            pcTail      = LW_NULL;
    INT              iError;
    
    if (__STR_IS_ROOT(pcName)) {
        LW_DEV_INC_USE_COUNT(&_G_devhdrShm);                            /*  ���¼�����                  */
        return  ((LONG)LW_NULL);
    }
    
    if (iFlags & O_CREAT) {                                             /*  ���ﲻ���� socket �ļ�      */
        if (__fsCheckFileName(pcName)) {
            _ErrorHandle(ENOENT);
            return  (PX_ERROR);
        }
        if (S_ISFIFO(iMode) || 
            S_ISBLK(iMode)  ||
            S_ISCHR(iMode)) {
            _ErrorHandle(ERROR_IO_DISK_NOT_PRESENT);                    /*  ��֧��������Щ��ʽ          */
            return  (PX_ERROR);
        }
    }
    
    if (iFlags & O_TRUNC) {                                             /*  ������򿪽ض�              */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
    if (iFlags & O_CREAT) {                                             /*  ����Ŀ¼���ļ�              */
        iError = __shmMakeNode(pcName, iFlags, iMode, LW_NULL);
        if ((iError != ERROR_NONE) && (iFlags & O_EXCL)) {
            return  (PX_ERROR);                                         /*  �޷�����                    */
        }
    }
    
    __LW_SHM_LOCK();
    pshmn = __shmFindNode(pcName, &pshmnFather, &bIsRoot, LW_NULL, &pcTail);
    if (pshmn) {
        pshmn->SHMN_iOpenNum++;
        if (!S_ISLNK(pshmn->SHMN_mode)) {                               /*  ���������ļ�                */
            __LW_SHM_UNLOCK();                                          /*  ���������ڴ��豸            */
            LW_DEV_INC_USE_COUNT(&_G_devhdrShm);                        /*  ���¼�����                  */
            return  ((LONG)pshmn);
        }
    } else {
        __LW_SHM_UNLOCK();                                              /*  ���������ڴ��豸            */
        return  (PX_ERROR);
    }
    __LW_SHM_UNLOCK();                                                  /*  ���������ڴ��豸            */
    
    if (pshmn) {                                                        /*  �����ļ�����                */
        INT     iFollowLinkType;
        PCHAR   pcSymfile = pcTail - lib_strlen(pshmn->SHMN_pcName) - 1;
        PCHAR   pcPrefix;
        
        if (*pcSymfile != PX_DIVIDER) {
            pcSymfile--;
        }
        if (pcSymfile == pcName) {
            pcPrefix = LW_NULL;                                         /*  û��ǰ׺                    */
        } else {
            pcPrefix = pcName;
            *pcSymfile = PX_EOS;
        }
        if (pcTail && lib_strlen(pcTail)) {
            iFollowLinkType = FOLLOW_LINK_TAIL;                         /*  ����Ŀ���ڲ��ļ�            */
        } else {
            iFollowLinkType = FOLLOW_LINK_FILE;                         /*  �����ļ�����                */
        }
        
        iError = _PathBuildLink(pcName, MAX_FILENAME_LENGTH, 
                                LW_NULL, pcPrefix, (CPCHAR)pshmn->SHMN_pvLink, pcTail);
                                
        __LW_SHM_LOCK();                                                /*  ���������ڴ��豸            */
        pshmn->SHMN_iOpenNum--;
        __LW_SHM_UNLOCK();                                              /*  ���������ڴ��豸            */
        
        if (iError) {
            return  (PX_ERROR);                                         /*  �޷�����������Ŀ��Ŀ¼      */
        } else {
            return  (iFollowLinkType);
        }
    }
    
    LW_DEV_INC_USE_COUNT(&_G_devhdrShm);                                /*  ���¼�����                  */
    
    return  ((LONG)pshmn);
}
/*********************************************************************************************************
** ��������: __shmRemove
** ��������: �����ڴ��豸 remove ����
** �䡡��  : pdevhdr
**           pcName           �ļ���
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __shmRemove (PLW_DEV_HDR     pdevhdr,
                         PCHAR           pcName)
{
    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        PLW_SHM_NODE    pshmnFather;
        PLW_SHM_NODE    pshmn;
        BOOL            bIsRoot;
        PCHAR           pcTail = LW_NULL;
        
        __LW_SHM_LOCK();                                                /*  ���������ڴ��豸            */
        pshmn = __shmFindNode(pcName, &pshmnFather, &bIsRoot, LW_NULL, &pcTail);
        if (pshmn) {
            if (S_ISLNK(pshmn->SHMN_mode)) {                            /*  �����ļ�                    */
                size_t  stLenTail = 0;
                
                if (pcTail) {
                    stLenTail = lib_strlen(pcTail);                     /*  ȷ�� tail ����              */
                }
                
                /*
                 *  û��β��, ������Ҫɾ�������ļ�����, ����ɾ�������ļ�����.
                 *  ��ôֱ��ʹ�� __shmRemoveNode ɾ������.
                 */
                if (stLenTail) {
                    PCHAR   pcSymfile = pcTail - lib_strlen(pshmn->SHMN_pcName) - 1;
                    PCHAR   pcPrefix;
                    
                    if (*pcSymfile != PX_DIVIDER) {
                        pcSymfile--;
                    }
                    if (pcSymfile == pcName) {
                        pcPrefix = LW_NULL;                             /*  û��ǰ׺                    */
                    } else {
                        pcPrefix = pcName;
                        *pcSymfile = PX_EOS;
                    }
                    
                    if (_PathBuildLink(pcName, MAX_FILENAME_LENGTH, 
                                       LW_NULL, pcPrefix, 
                                       (CPCHAR)pshmn->SHMN_pvLink, 
                                       pcTail) < ERROR_NONE) {
                        __LW_SHM_UNLOCK();                              /*  ���������ڴ��豸            */
                        return  (PX_ERROR);                             /*  �޷�����������Ŀ��Ŀ¼      */
                    
                    } else {
                        __LW_SHM_UNLOCK();                              /*  ���������ڴ��豸            */
                        return  (FOLLOW_LINK_TAIL);                     /*  �����ļ��ڲ��ļ�            */
                    }
                }
            }
        }
        __LW_SHM_UNLOCK();                                              /*  ���������ڴ��豸            */
    
        return  (__shmRemoveNode(pcName));
    }
}
/*********************************************************************************************************
** ��������: __shmClose
** ��������: �����ڴ��豸 close ����
** �䡡��  : pshmn             �ļ��ڵ�
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __shmClose (PLW_SHM_NODE    pshmn)
{
    __LW_SHM_LOCK();                                                    /*  ���������ڴ��豸            */
    if (pshmn) {
        pshmn->SHMN_iOpenNum--;
    }
    __LW_SHM_UNLOCK();                                                  /*  ���������ڴ��豸            */
    
    LW_DEV_DEC_USE_COUNT(&_G_devhdrShm);                                /*  ���¼�����                  */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __shmRead
** ��������: �����ڴ��豸 read ����
** �䡡��  : pshmn            �ļ��ڵ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __shmRead (PLW_SHM_NODE  pshmn,
                           PCHAR         pcBuffer, 
                           size_t        stMaxBytes)
{
    _ErrorHandle(ENOSYS);
    return  (0);
}
/*********************************************************************************************************
** ��������: __shmPRead
** ��������: �����ڴ��豸 pread ����
** �䡡��  : pshmn         �ļ��ڵ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oftPos        λ��
** �䡡��  : ʵ�ʶ�ȡ�ĸ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __shmPRead (PLW_SHM_NODE  pshmn,
                            PCHAR         pcBuffer, 
                            size_t        stMaxBytes,
                            off_t         oftPos)
{
    _ErrorHandle(ENOSYS);
    return  (0);
}
/*********************************************************************************************************
** ��������: __shmWrite
** ��������: �����ڴ��豸 write ����
** �䡡��  : pshmn            �ļ��ڵ�
**           pcBuffer         ������
**           stNBytes         ��Ҫд�������
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __shmWrite (PLW_SHM_NODE pshmn,
                            PCHAR        pcBuffer, 
                            size_t       stNBytes)
{
    _ErrorHandle(ENOSYS);
    return  (0);
}
/*********************************************************************************************************
** ��������: __shmPWrite
** ��������: �����ڴ��豸 pwrite ����
** �䡡��  : pshmn         �ļ��ڵ�
**           pcBuffer      ������
**           stBytes       ��Ҫд����ֽ���
**           oftPos        λ��
** �䡡��  : ʵ��д��ĸ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __shmPWrite (PLW_SHM_NODE pshmn, 
                             PCHAR        pcBuffer, 
                             size_t       stBytes,
                             off_t        oftPos)
{
    _ErrorHandle(ENOSYS);
    return  (0);
}
/*********************************************************************************************************
** ��������: __shmStatGet
** ��������: �����ڴ��豸����ļ�״̬������
** �䡡��  : pshmn               �ļ��ڵ�
**           pstat               stat �ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __shmStatGet (PLW_SHM_NODE  pshmn, struct stat *pstat)
{
    if (pstat == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pshmn) {
        pstat->st_dev     = LW_DEV_MAKE_STDEV(&_G_devhdrShm);
        pstat->st_ino     = (ino_t)pshmn;
        pstat->st_mode    = pshmn->SHMN_mode;
        pstat->st_nlink   = 1;
        pstat->st_uid     = pshmn->SHMN_uid;
        pstat->st_gid     = pshmn->SHMN_gid;
        pstat->st_rdev    = 1;
        if (S_ISLNK(pshmn->SHMN_mode)) {
            pstat->st_size = lib_strlen((CPCHAR)pshmn->SHMN_pvLink);
        } else {
            pstat->st_size = pshmn->SHMN_oftSize;
        }
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
        pstat->st_atime = pshmn->SHMN_time;                             /*  �ڵ㴴����׼ʱ��            */
        pstat->st_mtime = pshmn->SHMN_time;
        pstat->st_ctime = pshmn->SHMN_time;
    
    } else {
        pstat->st_dev     = LW_DEV_MAKE_STDEV(&_G_devhdrShm);
        pstat->st_ino     = (ino_t)0;                                   /*  ��Ŀ¼                      */
        pstat->st_mode    = (0666 | S_IFDIR);                           /*  Ĭ������                    */
        pstat->st_nlink   = 1;
        pstat->st_uid     = 0;
        pstat->st_gid     = 0;
        pstat->st_rdev    = 1;
        pstat->st_size    = 0;
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
        pstat->st_atime   = _G_shmrRoot.SHMR_time;                      /*  �ڵ㴴����׼ʱ��            */
        pstat->st_mtime   = _G_shmrRoot.SHMR_time;
        pstat->st_ctime   = _G_shmrRoot.SHMR_time;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __shmLStatGet
** ��������: �����ڴ��豸����ļ�״̬������ (����������ļ����ȡ�����ļ�������)
** �䡡��  : pshmn               �ļ��ڵ�
**           pcName              �ļ���
**           pstat               stat �ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __shmLStatGet (LW_DEV_HDR *pdevhdr, PCHAR  pcName, struct stat *pstat)
{
    PLW_SHM_NODE    pshmnFather;
    PLW_SHM_NODE    pshmn;
    BOOL            bIsRoot;
    PCHAR           pcTail = LW_NULL;
    
    if (pcName == LW_NULL) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    __LW_SHM_LOCK();                                                    /*  ���������ڴ��豸            */
    pshmn = __shmFindNode(pcName, &pshmnFather, &bIsRoot, LW_NULL, &pcTail);
                                                                        /*  ��ѯ�豸                    */
    if (pshmn) {                                                        /*  һ������ FOLLOW_LINK_TAIL   */
        pstat->st_dev     = LW_DEV_MAKE_STDEV(&_G_devhdrShm);
        pstat->st_ino     = (ino_t)pshmn;
        pstat->st_mode    = pshmn->SHMN_mode;
        pstat->st_nlink   = 1;
        pstat->st_uid     = pshmn->SHMN_uid;
        pstat->st_gid     = pshmn->SHMN_gid;
        pstat->st_rdev    = 1;
        if (S_ISLNK(pshmn->SHMN_mode)) {
            pstat->st_size = lib_strlen((CPCHAR)pshmn->SHMN_pvLink);
        } else {
            pstat->st_size = pshmn->SHMN_oftSize;
        }
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
        pstat->st_atime = pshmn->SHMN_time;                             /*  �ڵ㴴����׼ʱ��            */
        pstat->st_mtime = pshmn->SHMN_time;
        pstat->st_ctime = pshmn->SHMN_time;
        
        __LW_SHM_UNLOCK();                                              /*  ���������ڴ��豸            */
        
        return  (ERROR_NONE);
        
    } else {
        __LW_SHM_UNLOCK();                                              /*  ���������ڴ��豸            */
        
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __shmStatfsGet
** ��������: �����ڴ��豸����ļ�ϵͳ״̬������
** �䡡��  : pshmn               �ļ��ڵ�
**           pstatfs             statfs �ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __shmStatfsGet (PLW_SHM_NODE  pshmn, struct statfs *pstatfs)
{
    if (pstatfs) {
        pstatfs->f_type   = 0;
        pstatfs->f_bsize  = (long)_G_shmrRoot.SHMR_stMemUsed;           /*  �ļ�ϵͳ�ڴ�ʹ����          */
        pstatfs->f_blocks = 1;
        pstatfs->f_bfree  = 0;
        pstatfs->f_bavail = 1;
        
        pstatfs->f_files  = 0;
        pstatfs->f_ffree  = 0;
        
#if LW_CFG_CPU_WORD_LENGHT == 64
        pstatfs->f_fsid.val[0] = (int32_t)((addr_t)&_G_devhdrShm >> 32);
        pstatfs->f_fsid.val[1] = (int32_t)((addr_t)&_G_devhdrShm & 0xffffffff);
#else
        pstatfs->f_fsid.val[0] = (int32_t)&_G_devhdrShm;
        pstatfs->f_fsid.val[1] = 0;
#endif
        
        pstatfs->f_flag    = 0;
        pstatfs->f_namelen = PATH_MAX;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __shmReadDir
** ��������: �����ڴ��豸���ָ��Ŀ¼��Ϣ
** �䡡��  : pshmn               �ļ��ڵ�
**           dir                 Ŀ¼�ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __shmReadDir (PLW_SHM_NODE  pshmn, DIR  *dir)
{
             INT                i;
             INT                iError = ERROR_NONE;
    REGISTER LONG               iStart;
             PLW_LIST_LINE      plineTemp;
             
             PLW_LIST_LINE      plineHeader;
             PLW_SHM_NODE       pshmnTemp;
             
    if (!dir) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __LW_SHM_LOCK();                                                    /*  ���������ڴ��豸            */
    if (pshmn == LW_NULL) {
        plineHeader = _G_shmrRoot.SHMR_plineSon;
    } else {
        plineHeader = pshmn->SHMN_plineSon;
    }
    
    iStart = dir->dir_pos;

    for ((plineTemp  = plineHeader), (i = 0); 
         (plineTemp != LW_NULL) && (i < iStart); 
         (plineTemp  = _list_line_get_next(plineTemp)), (i++));         /*  ����                        */
    
    if (plineTemp == LW_NULL) {
        _ErrorHandle(ENOENT);
        iError = PX_ERROR;                                              /*  û�ж���Ľڵ�              */
    
    } else {
        pshmnTemp = _LIST_ENTRY(plineTemp, LW_SHM_NODE, SHMN_lineBrother);
        dir->dir_pos++;
        /*
         *  �����ļ���
         */
        lib_strlcpy(dir->dir_dirent.d_name, 
                    pshmnTemp->SHMN_pcName, 
                    sizeof(dir->dir_dirent.d_name));                    /*  �����ļ���                  */
                    
        dir->dir_dirent.d_type = IFTODT(pshmnTemp->SHMN_mode);
        dir->dir_dirent.d_shortname[0] = PX_EOS;
    }
    __LW_SHM_UNLOCK();                                                  /*  ���������ڴ��豸            */
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __shmTruncate
** ��������: �����ڴ��豸�����ļ���С
** �䡡��  : pshmn               �ļ��ڵ�
**           oftSize             �ļ���С
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ÿ���ļ�ֻ������һ��.
*********************************************************************************************************/
static INT  __shmTruncate (PLW_SHM_NODE  pshmn, off_t  oftSize)
{
    __LW_SHM_LOCK();                                                    /*  ���������ڴ��豸            */
    if (!pshmn) {
        __LW_SHM_UNLOCK();                                              /*  ���������ڴ��豸            */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    if (!S_ISREG(pshmn->SHMN_mode)) {
        __LW_SHM_UNLOCK();                                              /*  ���������ڴ��豸            */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (pshmn->SHMN_oftSize == oftSize) {
        __LW_SHM_UNLOCK();                                              /*  ���������ڴ��豸            */
        return  (ERROR_NONE);
    }
    if (pshmn->SHMN_pvPhyMem) {                                         /*  ����ӳ�乤����, ���ܸı��С*/
        __LW_SHM_UNLOCK();                                              /*  ���������ڴ��豸            */
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    pshmn->SHMN_oftSize = oftSize;                                      /*  ��¼�µ��ļ���С            */
    __LW_SHM_UNLOCK();                                                  /*  ���������ڴ��豸            */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __shmMmap
** ��������: �����ڴ��豸�����ڴ�ռ�ӳ��
** �䡡��  : pshmn               �ļ��ڵ�
**           pdmap               ����ռ���Ϣ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT   __shmMmap (PLW_SHM_NODE  pshmn, PLW_DEV_MMAP_AREA  pdmap)
{
    addr_t   ulPhysical;

    __LW_SHM_LOCK();                                                    /*  ���������ڴ��豸            */
    if (!pshmn) {
        __LW_SHM_UNLOCK();                                              /*  ���������ڴ��豸            */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    if (!S_ISREG(pshmn->SHMN_mode)) {
        __LW_SHM_UNLOCK();                                              /*  ���������ڴ��豸            */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (pshmn->SHMN_oftSize == 0) {
        __LW_SHM_UNLOCK();                                              /*  ���������ڴ��豸            */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (pshmn->SHMN_pvPhyMem == LW_NULL) {
        if (__shmPhymemAlloc(pshmn) < ERROR_NONE) {                     /*  ���������ڴ�                */
            __LW_SHM_UNLOCK();                                          /*  ���������ڴ��豸            */
            return  (PX_ERROR);
        }
    }
    
    pshmn->SHMN_ulMapCnt++;                                             /*  mmap ����++                 */

    ulPhysical  = (addr_t)pshmn->SHMN_pvPhyMem;
    ulPhysical += (addr_t)(pdmap->DMAP_offPages << LW_CFG_VMM_PAGE_SHIFT);                                   
    
    if (API_VmmRemapArea(pdmap->DMAP_pvAddr, (PVOID)ulPhysical, 
                         pdmap->DMAP_stLen, pdmap->DMAP_ulFlag,         /*  ֱ��ʹ�� mmap ָ���� flag   */
                         LW_NULL, LW_NULL)) {                           /*  �������ڴ�ӳ�䵽�����ڴ�    */
        if (pshmn->SHMN_ulMapCnt > 1) {
            pshmn->SHMN_ulMapCnt--;
        } else {
            pshmn->SHMN_ulMapCnt = 0;
            __shmPhymemFree(pshmn);                                     /*  �ͷ������ڴ�                */
        }
        __LW_SHM_UNLOCK();                                              /*  ���������ڴ��豸            */
        return  (PX_ERROR);
    }
    __LW_SHM_UNLOCK();                                                  /*  ���������ڴ��豸            */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __shmUnmap
** ��������: �����ڴ��豸�����ڴ�ռ���ӳ��
** �䡡��  : pshmn               �ļ��ڵ�
**           pdmap               ����ռ���Ϣ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT   __shmUnmap (PLW_SHM_NODE  pshmn, PLW_DEV_MMAP_AREA  pdmap)
{
    __LW_SHM_LOCK();                                                    /*  ���������ڴ��豸            */
    if (!pshmn) {
        __LW_SHM_UNLOCK();                                              /*  ���������ڴ��豸            */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    if (!S_ISREG(pshmn->SHMN_mode)) {
        __LW_SHM_UNLOCK();                                              /*  ���������ڴ��豸            */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (pshmn->SHMN_oftSize == 0) {
        __LW_SHM_UNLOCK();                                              /*  ���������ڴ��豸            */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (pshmn->SHMN_pvPhyMem) {
        if (pshmn->SHMN_ulMapCnt > 1) {
            pshmn->SHMN_ulMapCnt--;
        } else {
            pshmn->SHMN_ulMapCnt = 0;
            __shmPhymemFree(pshmn);                                     /*  �������κ�ӳ��, �ͷ������ڴ�*/
        }
    }
    __LW_SHM_UNLOCK();                                                  /*  ���������ڴ��豸            */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __shmLseek
** ��������: �����ڴ��豸 lseek ����
** �䡡��  : pshmn            �ļ��ڵ�
**           oftOffset        ƫ����
**           iWhence          ��λ��׼
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static off_t  __shmLseek (PLW_SHM_NODE  pshmn,
                          off_t         oftOffset, 
                          INT           iWhence)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __shmIoctl
** ��������: �����ڴ��豸 ioctl ����
** �䡡��  : pshmn              �ļ��ڵ�
**           request,           ����
**           arg                �������
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __shmIoctl (PLW_SHM_NODE  pshmn,
                        INT           iRequest,
                        LONG          lArg)
{
    off_t   oftSize;

    switch (iRequest) {
    
    case FIOFSTATGET:                                                   /*  ����ļ�״̬                */
        return  (__shmStatGet(pshmn, (struct stat *)lArg));
        
    case FIOFSTATFSGET:                                                 /*  ����ļ�ϵͳ״̬            */
        return  (__shmStatfsGet(pshmn, (struct statfs *)lArg));
    
    case FIOREADDIR:                                                    /*  ��ȡһ��Ŀ¼��Ϣ            */
        return  (__shmReadDir(pshmn, (DIR *)lArg));
    
    case FIOTRUNC:                                                      /*  �����ļ���С                */
        oftSize = *(off_t *)lArg;
        return  (__shmTruncate(pshmn, oftSize));
    
    case FIOSYNC:                                                       /*  ���ļ������д              */
    case FIODATASYNC:
    case FIOFLUSH:
        return  (ERROR_NONE);
        
    case FIOFSTYPE:                                                     /*  ����ļ�ϵͳ����            */
        *(PCHAR *)lArg = "Share Memory FileSystem";
        return  (ERROR_NONE);
        
    default:
        _ErrorHandle(ENOSYS);
        break;
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __shmSymlink
** ��������: �����ڴ��豸 symlink ����
** �䡡��  : pdevhdr            
**           pcName             �����������ļ�
**           pcLinkDst          ����Ŀ��
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __shmSymlink (PLW_DEV_HDR     pdevhdr,
                          PCHAR           pcName,
                          CPCHAR          pcLinkDst)
{
    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }
    
    return  (__shmMakeNode(pcName, O_CREAT | O_EXCL, DEFAULT_FILE_PERM | S_IFLNK, pcLinkDst));
}
/*********************************************************************************************************
** ��������: __shmReadlink
** ��������: �����ڴ��豸 read link ����
** �䡡��  : pdevhdr                       �豸ͷ
**           pcName                        ����ԭʼ�ļ���
**           pcLinkDst                     ����Ŀ���ļ���
**           stMaxSize                     �����С
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __shmReadlink (PLW_DEV_HDR    pdevhdr,
                               PCHAR          pcName,
                               PCHAR          pcLinkDst,
                               size_t         stMaxSize)
{
    PLW_SHM_NODE    pshmnFather = LW_NULL;
    PLW_SHM_NODE    pshmn;
    BOOL            bIsRoot = LW_FALSE;
    ssize_t         sstRet  = PX_ERROR;

    if ((pcName == LW_NULL) || (pcLinkDst == LW_NULL) || (stMaxSize == 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __LW_SHM_LOCK();                                                    /*  ���������ڴ��豸            */
    pshmn = __shmFindNode(pcName, &pshmnFather, 
                          &bIsRoot, LW_NULL, LW_NULL);                  /*  ��ѯ�豸                    */
    if (pshmn) {
        if (S_ISLNK(pshmn->SHMN_mode)) {
            size_t  stLen = lib_strlen((CPCHAR)pshmn->SHMN_pvLink);
            lib_strncpy(pcLinkDst, 
                        (CPCHAR)pshmn->SHMN_pvLink, stMaxSize);         /*  ������������                */
            if (stLen > stMaxSize) {
                stLen = stMaxSize;                                      /*  ������Ч�ֽ���              */
            }
            sstRet = (ssize_t)stLen;
        
        } else {
            _ErrorHandle(ENOENT);
        }
    }
    __LW_SHM_UNLOCK();                                                  /*  ���������ڴ��豸            */
    
    return  (sstRet);
}
/*********************************************************************************************************
** ��������: API_ShmDrvInstall
** ��������: ��װ�����ڴ���������
** �䡡��  : NONE
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_ShmDrvInstall (VOID)
{
    struct file_operations     fileop;

    if (_G_iShmDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    if (_G_ulShmLock == LW_OBJECT_HANDLE_INVALID) {
        _G_ulShmLock = API_SemaphoreMCreate("shm_lock", LW_PRIO_DEF_CEILING, 
                                            LW_OPTION_WAIT_PRIORITY |
                                            LW_OPTION_INHERIT_PRIORITY |
                                            LW_OPTION_DELETE_SAFE |
                                            LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }
    
    lib_bzero(&fileop, sizeof(struct file_operations));
    
    fileop.owner       = THIS_MODULE;
    fileop.fo_create   = __shmOpen;
    fileop.fo_release  = __shmRemove;
    fileop.fo_open     = __shmOpen;
    fileop.fo_close    = __shmClose;
    fileop.fo_read     = __shmRead;
    fileop.fo_read_ex  = __shmPRead;
    fileop.fo_write    = __shmWrite;
    fileop.fo_write_ex = __shmPWrite;
    fileop.fo_lstat    = __shmLStatGet;
    fileop.fo_ioctl    = __shmIoctl;
    fileop.fo_lseek    = __shmLseek;
    fileop.fo_symlink  = __shmSymlink;
    fileop.fo_readlink = __shmReadlink;
    fileop.fo_mmap     = __shmMmap;
    fileop.fo_unmap    = __shmUnmap;
    
    _G_iShmDrvNum = iosDrvInstallEx(&fileop);
     
    DRIVER_LICENSE(_G_iShmDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iShmDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iShmDrvNum, "share memory driver.");
    
    return  ((_G_iShmDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** ��������: API_ShmDevCreate
** ��������: ��װ�����ڴ��豸
** �䡡��  : NONE
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_ShmDevCreate (VOID)
{
    static BOOL     bIsInit = LW_FALSE;
    
    if (bIsInit) {
        return  (ERROR_NONE);
    }

    if (_G_iShmDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    if (iosDevAddEx(&_G_devhdrShm, "/dev/shm", _G_iShmDrvNum, DT_DIR) != ERROR_NONE) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    bIsInit = LW_TRUE;
    
    lib_time(&_G_shmrRoot.SHMR_time);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
                                                                        /*  LW_CFG_SHM_DEVICE_EN        */
                                                                        /*  LW_CFG_VMM_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
