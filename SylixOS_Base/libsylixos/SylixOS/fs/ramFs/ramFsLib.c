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
** ��   ��   ��: ramFsLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 05 �� 24 ��
**
** ��        ��: �ڴ��ļ�ϵͳ�ڲ�����.
**
** BUG:
2015.11.25  ���� ramFs seek ������Ҵ���.
2017.12.27  ���� __ram_move() ���� POSIX �淶.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0 && LW_CFG_RAMFS_EN > 0
#include "ramFsLib.h"
/*********************************************************************************************************
  ������
*********************************************************************************************************/
#define RAM_FOOT(blk1, blk2)    ((blk1 > blk2) ? (blk1 - blk2) : (blk2 - blk1))
/*********************************************************************************************************
** ��������: __ram_open
** ��������: ramfs ��һ���ļ�
** �䡡��  : pramfs           �ļ�ϵͳ
**           pcName           �ļ���
**           ppramnFather     ���޷��ҵ��ڵ�ʱ������ӽ���һ��,
                              ��Ѱ�ҵ��ڵ�ʱ���游ϵ�ڵ�. 
                              LW_NULL ��ʾ��
             pbRoot           �Ƿ�Ϊ���ڵ�
**           pbLast           ��ƥ��ʧ��ʱ, �Ƿ������һ���ļ�ƥ��ʧ��
**           ppcTail          ������������ļ�, ָ�������ļ����·��
** �䡡��  : �򿪽��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PRAM_NODE  __ram_open (PRAM_VOLUME  pramfs,
                       CPCHAR       pcName,
                       PRAM_NODE   *ppramnFather,
                       BOOL        *pbRoot,
                       BOOL        *pbLast,
                       PCHAR       *ppcTail)
{
    CHAR                pcTempName[MAX_FILENAME_LENGTH];
    PCHAR               pcNext;
    PCHAR               pcNode;
    
    PRAM_NODE           pramn;
    PRAM_NODE           pramnTemp;
    
    PLW_LIST_LINE       plineTemp;
    PLW_LIST_LINE       plineHeader;                                    /*  ��ǰĿ¼ͷ                  */
    
    if (ppramnFather == LW_NULL) {
        ppramnFather = &pramnTemp;                                      /*  ��ʱ����                    */
    }
    *ppramnFather = LW_NULL;
    
    if (*pcName == PX_ROOT) {                                           /*  ���Ը�����                  */
        lib_strlcpy(pcTempName, (pcName + 1), PATH_MAX);
    } else {
        lib_strlcpy(pcTempName, pcName, PATH_MAX);
    }
    
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
    plineHeader = pramfs->RAMFS_plineSon;                               /*  �Ӹ�Ŀ¼��ʼ����            */
    
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
            
            pramn = _LIST_ENTRY(plineTemp, RAM_NODE, RAMN_lineBrother);
            if (S_ISLNK(pramn->RAMN_mode)) {                            /*  �����ļ�                    */
                if (lib_strcmp(pramn->RAMN_pcName, pcNode) == 0) {
                    goto    __find_ok;                                  /*  �ҵ�����                    */
                }
            
            } else if (S_ISDIR(pramn->RAMN_mode)) {
                if (lib_strcmp(pramn->RAMN_pcName, pcNode) == 0) {      /*  �Ѿ��ҵ�һ��Ŀ¼            */
                    break;
                }
                
            } else {
                if (lib_strcmp(pramn->RAMN_pcName, pcNode) == 0) {
                    if (pcNext) {                                       /*  �������¼�, �������ΪĿ¼  */
                        goto    __find_error;                           /*  ����Ŀ¼ֱ�Ӵ���            */
                    }
                    break;
                }
            }
        }
        if (plineTemp == LW_NULL) {                                     /*  �޷���������                */
            goto    __find_error;
        }
        
        *ppramnFather = pramn;                                          /*  �ӵ�ǰ�ڵ㿪ʼ����          */
        plineHeader   = pramn->RAMN_plineSon;                           /*  �ӵ�һ�����ӿ�ʼ            */
        
    } while (pcNext);                                                   /*  �������¼�Ŀ¼              */
    
__find_ok:
    *ppramnFather = pramn->RAMN_pramnFather;                            /*  ��ϵ�ڵ�                    */
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
    return  (pramn);
    
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
** ��������: __ram_maken
** ��������: ramfs ����һ���ļ�
** �䡡��  : pramfs           �ļ�ϵͳ
**           pcName           �ļ���
**           pramnFather      ����, NULL ��ʾ��Ŀ¼
**           mode             mode_t
**           pcLink           ���Ϊ�����ļ�, ����ָ������Ŀ��.
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PRAM_NODE  __ram_maken (PRAM_VOLUME  pramfs,
                        CPCHAR       pcName,
                        PRAM_NODE    pramnFather,
                        mode_t       mode,
                        CPCHAR       pcLink)
{
    PRAM_NODE   pramn = (PRAM_NODE)__SHEAP_ALLOC(sizeof(RAM_NODE));
    CPCHAR      pcFileName;
    
    if (pramn == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }
    lib_bzero(pramn, sizeof(RAM_NODE));
    
    pcFileName = lib_rindex(pcName, PX_DIVIDER);
    if (pcFileName) {
        pcFileName++;
    } else {
        pcFileName = pcName;
    }
    
    pramn->RAMN_pcName = (PCHAR)__SHEAP_ALLOC(lib_strlen(pcFileName) + 1);
    if (pramn->RAMN_pcName == LW_NULL) {
        __SHEAP_FREE(pramn);
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }
    lib_strcpy(pramn->RAMN_pcName, pcFileName);
    
    if (S_ISLNK(mode)) {
        pramn->RAMN_pcLink = (PCHAR)__SHEAP_ALLOC(lib_strlen(pcLink) + 1);
        if (pramn->RAMN_pcLink == LW_NULL) {
            __SHEAP_FREE(pramn->RAMN_pcName);
            __SHEAP_FREE(pramn);
            _ErrorHandle(ENOMEM);
            return  (LW_NULL);
        }
        lib_strcpy(pramn->RAMN_pcLink, pcLink);
        
    } else {
        if ((mode & S_IFMT) == 0) {
            mode |= S_IFREG;
        }
        pramn->RAMN_pcLink = LW_NULL;
    }
    
    pramn->RAMN_pramnFather = pramnFather;
    pramn->RAMN_pramfs      = pramfs;
    pramn->RAMN_mode        = mode;
    pramn->RAMN_timeCreate  = lib_time(LW_NULL);
    pramn->RAMN_timeAccess  = pramn->RAMN_timeCreate;
    pramn->RAMN_timeChange  = pramn->RAMN_timeCreate;
    pramn->RAMN_uid         = getuid();
    pramn->RAMN_gid         = getgid();
    pramn->RAMN_prambCookie = LW_NULL;
    
    if (pramnFather) {
        _List_Line_Add_Ahead(&pramn->RAMN_lineBrother, 
                             &pramnFather->RAMN_plineSon);
    } else {
        _List_Line_Add_Ahead(&pramn->RAMN_lineBrother, 
                             &pramfs->RAMFS_plineSon);
    }
    
    return  (pramn);
}
/*********************************************************************************************************
** ��������: __ram_unlink
** ��������: ramfs ɾ��һ���ļ�
** �䡡��  : pramn            �ļ��ڵ�
** �䡡��  : ɾ�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __ram_unlink (PRAM_NODE  pramn)
{
    PRAM_VOLUME     pramfs;
    PRAM_NODE       pramnFather;
    PLW_LIST_LINE   plineTemp;
    
    if (S_ISDIR(pramn->RAMN_mode) && pramn->RAMN_plineSon) {
        _ErrorHandle(ENOTEMPTY);
        return  (PX_ERROR);
    }
    
    pramfs      = pramn->RAMN_pramfs;
    pramnFather = pramn->RAMN_pramnFather;
    
    if (pramnFather) {
        _List_Line_Del(&pramn->RAMN_lineBrother, 
                       &pramnFather->RAMN_plineSon);
    } else {
        _List_Line_Del(&pramn->RAMN_lineBrother, 
                       &pramfs->RAMFS_plineSon);
    }
    
    while (pramn->RAMN_plineBStart) {
        plineTemp = pramn->RAMN_plineBStart;
        _List_Line_Del(plineTemp, &pramn->RAMN_plineBStart);
        
        __RAM_BFREE(plineTemp);
        pramfs->RAMFS_ulCurBlk--;
        pramn->RAMN_ulCnt--;
    }
    
    if (S_ISLNK(pramn->RAMN_mode)) {
        __SHEAP_FREE(pramn->RAMN_pcLink);
    }
    __SHEAP_FREE(pramn->RAMN_pcName);
    __SHEAP_FREE(pramn);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ram_truncate
** ��������: ramfs �ض�һ���ļ�
** �䡡��  : pramn            �ļ��ڵ�
**           stOft            �׶ε�
** �䡡��  : ���̽��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __ram_truncate (PRAM_NODE  pramn, size_t  stOft)
{
    size_t          stTemp = 0;
    PLW_LIST_LINE   plineTemp;
    PLW_LIST_LINE   plineDel;
    PRAM_VOLUME     pramfs = pramn->RAMN_pramfs;
    
    for (plineTemp  = pramn->RAMN_plineBStart;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        if (stOft <= stTemp) {                                          /*  �Ӵ˿鿪ʼɾ��              */
            break;
        } else {
            stTemp += __RAM_BDATASIZE;                                  /*  �ļ�����ָ����ǰ�ƶ�        */
        }
    }
    
    if (plineTemp) {                                                    /*  ��Ҫ�ض�                    */
        if (plineTemp == pramn->RAMN_plineBStart) {                     /*  �ӵ�һ���鿪ʼɾ��          */
            pramn->RAMN_plineBStart = LW_NULL;
            pramn->RAMN_plineBEnd   = LW_NULL;
        
        } else if (plineTemp == pramn->RAMN_plineBEnd) {                /*  ɾ��������                  */
            pramn->RAMN_plineBEnd = _list_line_get_prev(plineTemp);
        }
        
        do {
            plineDel  = plineTemp;
            plineTemp = _list_line_get_next(plineTemp);
            _List_Line_Del(plineDel, &pramn->RAMN_plineBStart);
            
            __RAM_BFREE(plineTemp);
            pramfs->RAMFS_ulCurBlk--;
            pramn->RAMN_ulCnt--;
        } while (plineTemp);
        
        pramn->RAMN_prambCookie = LW_NULL;                              /*  cookie δ֪                 */
    }

    pramn->RAMN_stSize  = stOft;                                        /*  ��¼�µ��ļ���С            */
    pramn->RAMN_stVSize = stOft;
}
/*********************************************************************************************************
** ��������: __ram_automem
** ��������: ramfs ������Ҫ�����ļ�������
** �䡡��  : pramn            �ļ��ڵ�
**           ulNBlk           ���󻺳�����
**           stStart          �ڴ˷�Χ�ڲ�����
**           stLen
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ram_automem (PRAM_NODE  pramn, ULONG  ulNBlk, size_t  stStart, size_t  stLen)
{
    ULONG           i;
    ULONG           ulNeedAllocBlk;
    PRAM_VOLUME     pramfs = pramn->RAMN_pramfs;
    size_t          stCur;
    
    if (ulNBlk <= pramn->RAMN_ulCnt) {                                  /*  ʵ����Ŀ����                */
        return  (ERROR_NONE);
    
    } else {                                                            /*  ��Ҫ���ӿ�                  */
        PRAM_BUFFER     pramb;
        
        ulNeedAllocBlk = ulNBlk - pramn->RAMN_ulCnt;
        if ((ulNeedAllocBlk + pramfs->RAMFS_ulCurBlk) > 
            pramfs->RAMFS_ulMaxBlk) {                                   /*  �����ļ�ϵͳ��С����        */
            _ErrorHandle(ENOSPC);
            return  (PX_ERROR);
        }
        
        stCur = (size_t)pramn->RAMN_ulCnt * __RAM_BDATASIZE;
        
        for (i = 0; i < ulNeedAllocBlk; i++) {
            pramb = (PRAM_BUFFER)__RAM_BALLOC(__RAM_BSIZE);
            if (pramb == LW_NULL) {
                _ErrorHandle(ENOSPC);
                return  (PX_ERROR);
            }
            
            if (((stCur < stStart) && ((stCur + __RAM_BDATASIZE) <= stStart)) ||
                ((stCur > stStart) && (stCur >= (stStart + stLen)))) {  /*  �ж��������Ƿ�Ҫ����        */
                lib_bzero(pramb->RAMB_ucData, __RAM_BDATASIZE);
            }
            
            if (pramn->RAMN_plineBEnd) {                                /*  ����ĩβ��                  */
                _List_Line_Add_Right(&pramb->RAMB_lineManage,
                                     pramn->RAMN_plineBEnd);
                pramn->RAMN_plineBEnd = &pramb->RAMB_lineManage;
            
            } else {                                                    /*  û�л���                    */
                _List_Line_Add_Ahead(&pramb->RAMB_lineManage,
                                     &pramn->RAMN_plineBStart);
                pramn->RAMN_plineBEnd = pramn->RAMN_plineBStart;
            }
            
            pramfs->RAMFS_ulCurBlk++;
            pramn->RAMN_ulCnt++;
        }
        
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: __ram_automem
** ��������: ramfs ������Ҫ�����ļ�������
** �䡡��  : pramn            �ļ��ڵ�
**           ulNBlk           ���󻺳�����
**           stStart          �ڴ˷�Χ�ڲ�����
**           stLen
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __ram_increase (PRAM_NODE  pramn, size_t  stNewSize)
{
    if (pramn->RAMN_stSize < stNewSize) {
        pramn->RAMN_stSize = stNewSize;
        if (pramn->RAMN_stVSize < pramn->RAMN_stSize) {
            pramn->RAMN_stVSize = pramn->RAMN_stSize;
        }
    }
}
/*********************************************************************************************************
** ��������: __ram_getbuf
** ��������: ramfs ��ȡ�ļ�����
** �䡡��  : pramn            �ļ��ڵ�
**           stOft            ƫ����
**           pstBlkOft        ��ȡ�� block �ڲ�ƫ��
** �䡡��  : ��ȡ�Ļ�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PRAM_BUFFER  __ram_getbuf (PRAM_NODE  pramn, size_t  stOft, size_t  *pstBlkOft)
{
#define RAM_BCHK_VALID(x)   if (x == LW_NULL) { _ErrorHandle(ENOSPC); return  (LW_NULL); }

    PLW_LIST_LINE   plineTemp;
    ULONG           i;
    ULONG           ulFoot = __ARCH_ULONG_MAX;
    ULONG           ulBlkIndex;

    if (pramn->RAMN_plineBStart == LW_NULL) {
        _ErrorHandle(ENOSPC);
        return  (LW_NULL);
    }
    
    ulBlkIndex = (ULONG)(stOft / __RAM_BDATASIZE);
    *pstBlkOft = (ULONG)(stOft % __RAM_BDATASIZE);
    
    if (pramn->RAMN_prambCookie) {                                      /*  ���ȼ�� cookie             */
        ulFoot = RAM_FOOT(pramn->RAMN_ulCookie, ulBlkIndex);
        if (ulFoot == 0ul) {
            return  (pramn->RAMN_prambCookie);
        }
    }
    
    if (ulFoot < ulBlkIndex) {                                          /*  ͨ�� cookie ���Ҹ�����      */
        plineTemp = (PLW_LIST_LINE)pramn->RAMN_prambCookie;
        if (pramn->RAMN_ulCookie < ulBlkIndex) {
            for (i = 0; i < ulFoot; i++) {
                plineTemp = _list_line_get_next(plineTemp);
                RAM_BCHK_VALID(plineTemp);
            }
        } else {
            for (i = 0; i < ulFoot; i++) {
                plineTemp = _list_line_get_prev(plineTemp);
                RAM_BCHK_VALID(plineTemp);
            }
        }
    } else {                                                            /*  ��Ҫͨ�� start end ��ѯ     */
        REGISTER ULONG  ulForward  = ulBlkIndex;
        REGISTER ULONG  ulBackward = pramn->RAMN_ulCnt - ulBlkIndex - 1;
        
        if (ulForward <= ulBackward) {                                  /*  ͨ�� start ָ����Ҹ���     */
            plineTemp = pramn->RAMN_plineBStart;
            for (i = 0; i < ulForward; i++) {
                plineTemp = _list_line_get_next(plineTemp);
                RAM_BCHK_VALID(plineTemp);
            }
        } else {                                                        /*  ͨ�� end ָ����Ҹ���       */
            plineTemp = pramn->RAMN_plineBEnd;
            for (i = 0; i < ulBackward; i++) {
                plineTemp = _list_line_get_prev(plineTemp);
                RAM_BCHK_VALID(plineTemp);
            }
        }
    }
    
    pramn->RAMN_prambCookie = (PRAM_BUFFER)plineTemp;
    pramn->RAMN_ulCookie    = ulBlkIndex;
    return  (pramn->RAMN_prambCookie);
}
/*********************************************************************************************************
** ��������: __ram_getbuf_next
** ��������: ramfs ͨ�� cookie ��ȡ�¸�����
** �䡡��  : pramn            �ļ��ڵ�
** �䡡��  : ��ȡ�Ļ�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PRAM_BUFFER  __ram_getbuf_next (PRAM_NODE  pramn)
{
#define RAM_COOKIE_NEXT(cookie) (PRAM_BUFFER)_list_line_get_next(&(cookie)->RAMB_lineManage)

    if (pramn == LW_NULL) {
        return  (LW_NULL);
    }

    if (pramn->RAMN_prambCookie) {
        pramn->RAMN_prambCookie = RAM_COOKIE_NEXT(pramn->RAMN_prambCookie);
        pramn->RAMN_ulCookie++;
        return  (pramn->RAMN_prambCookie);
    
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: __ram_read
** ��������: ramfs ��ȡ�ļ�����
** �䡡��  : pramn            �ļ��ڵ�
**           pvBuffer         ������
**           stSize           ��������С
**           stOft            ƫ����
** �䡡��  : ��ȡ���ֽ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ssize_t  __ram_read (PRAM_NODE  pramn, PVOID  pvBuffer, size_t  stSize, size_t  stOft)
{
    PRAM_BUFFER     pramb;
    UINT8          *pucDest = (UINT8 *)pvBuffer;
    size_t          stDataLeft;
    size_t          stNBytes;
    size_t          stRead  = 0;
    size_t          stStart;
    
    if (pramn->RAMN_stVSize <= stOft) {                                 /*  �Ѿ����ļ�ĩβ              */
        return  (0);
    }
    
    stDataLeft = pramn->RAMN_stVSize - stOft;                           /*  ����ʣ��������              */
    stNBytes   = __MIN(stDataLeft, stSize);
    
    pramb = __ram_getbuf(pramn, stOft, &stStart);
    do {
        if (pramb == LW_NULL) {                                         /*  ��Ҫ��� 0 (POSIX)          */
            lib_bzero(pucDest, stNBytes);
            stRead += stNBytes;
            break;
        
        } else {
            size_t  stBufSize = (__RAM_BDATASIZE - stStart);
            if (stBufSize >= stNBytes) {
                lib_memcpy(pucDest, &pramb->RAMB_ucData[stStart], stNBytes);
                stRead += stNBytes;
                break;
            
            } else {
                lib_memcpy(pucDest, &pramb->RAMB_ucData[stStart], stBufSize);
                pucDest  += stBufSize;
                stRead   += stBufSize;
                stNBytes -= stBufSize;
                stStart   = 0;                                          /*  �´ο�����ͷ��ʼ           */
            }
        }
        pramb = __ram_getbuf_next(pramn);
    } while (stNBytes);
    
    return  ((ssize_t)stRead);
}
/*********************************************************************************************************
** ��������: __ram_write
** ��������: ramfs д���ļ�����
** �䡡��  : pramn            �ļ��ڵ�
**           pvBuffer         ������
**           stNBytes         ��Ҫ��ȡ�Ĵ�С
**           stOft            ƫ����
** �䡡��  : ��ȡ���ֽ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ssize_t  __ram_write (PRAM_NODE  pramn, CPVOID  pvBuffer, size_t  stNBytes, size_t  stOft)
{
    PRAM_BUFFER     pramb;
    UINT8          *pucDest = (UINT8 *)pvBuffer;
    size_t          stEnd   = stOft + stNBytes;
    size_t          stWrite = 0;
    size_t          stStart;
    ULONG           ulNBlk;
    
    ulNBlk = (ULONG)(stEnd / __RAM_BDATASIZE);
    if (stEnd % __RAM_BDATASIZE) {
        ulNBlk++;
    }
    
    if (__ram_automem(pramn, ulNBlk, stOft, stNBytes)) {
        return  (0);                                                    /*  û���ڴ�ռ�ɹ�����        */
    }
    
    pramb = __ram_getbuf(pramn, stOft, &stStart);
    while (pramb && stNBytes) {
        size_t  stBufSize = (__RAM_BDATASIZE - stStart);
        if (stBufSize >= stNBytes) {
            lib_memcpy(&pramb->RAMB_ucData[stStart], pucDest, stNBytes);
            stWrite += stNBytes;
            break;
        
        } else {
            lib_memcpy(&pramb->RAMB_ucData[stStart], pucDest, stBufSize);
            pucDest  += stBufSize;
            stWrite  += stBufSize;
            stNBytes -= stBufSize;
            stStart   = 0;                                              /*  �´δ� 0 ��ʼ               */
        }
        pramb = __ram_getbuf_next(pramn);
    }
    
    if (pramn->RAMN_stSize < (stOft + stWrite)) {
        pramn->RAMN_stSize = (stOft + stWrite);
        if (pramn->RAMN_stVSize < pramn->RAMN_stSize) {
            pramn->RAMN_stVSize = pramn->RAMN_stSize;
        }
    }
    
    return  ((ssize_t)stWrite);
}
/*********************************************************************************************************
** ��������: __ram_mount
** ��������: ramfs ����
** �䡡��  : pramfs           �ļ�ϵͳ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __ram_mount (PRAM_VOLUME  pramfs)
{
}
/*********************************************************************************************************
** ��������: __ram_unlink_dir
** ��������: ramfs ɾ��һ��Ŀ¼
** �䡡��  : plineDir         Ŀ¼ͷ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __ram_unlink_dir (LW_LIST_LINE_HEADER plineDir)
{
    PLW_LIST_LINE   plineTemp;
    PRAM_NODE       pramn;
    
    while (plineDir) {
        plineTemp = plineDir;
        plineDir  = _list_line_get_next(plineDir);
        pramn     = _LIST_ENTRY(plineTemp, RAM_NODE, RAMN_lineBrother);
        if (S_ISDIR(pramn->RAMN_mode) && pramn->RAMN_plineSon) {        /*  Ŀ¼�ļ�                    */
            __ram_unlink_dir(pramn->RAMN_plineSon);                     /*  �ݹ�ɾ����Ŀ¼              */
        }
        __ram_unlink(pramn);
    }
}
/*********************************************************************************************************
** ��������: __ram_ummount
** ��������: ramfs ж��
** �䡡��  : pramfs           �ļ�ϵͳ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __ram_unmount (PRAM_VOLUME  pramfs)
{
    __ram_unlink_dir(pramfs->RAMFS_plineSon);
}
/*********************************************************************************************************
** ��������: __ram_close
** ��������: ramfs �ر�һ���ļ�
** �䡡��  : pramn            �ļ��ڵ�
**           iFlag            ���ļ�ʱ�ķ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __ram_close (PRAM_NODE  pramn, INT  iFlag)
{
    pramn->RAMN_timeAccess = lib_time(LW_NULL);

    if (pramn->RAMN_bChanged && ((iFlag & O_ACCMODE) != O_RDONLY)) {
        pramn->RAMN_timeChange = pramn->RAMN_timeAccess;
    }
}
/*********************************************************************************************************
** ��������: __ram_move_check
** ��������: ramfs ���ڶ����ڵ��Ƿ�Ϊ��һ���ڵ������
** �䡡��  : pramn1        ��һ���ڵ�
**           pramn2        �ڶ����ڵ�
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ram_move_check (PRAM_NODE  pramn1, PRAM_NODE  pramn2)
{
    do {
        if (pramn1 == pramn2) {
            return  (PX_ERROR);
        }
        pramn2 = pramn2->RAMN_pramnFather;
    } while (pramn2);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ram_move
** ��������: ramfs �ƶ�����������һ���ļ�
** �䡡��  : pramn            �ļ��ڵ�
**           pcNewName        �µ�����
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __ram_move (PRAM_NODE  pramn, PCHAR  pcNewName)
{
    INT         iRet;
    PRAM_VOLUME pramfs;
    PRAM_NODE   pramnTemp;
    PRAM_NODE   pramnFather;
    PRAM_NODE   pramnNewFather;
    BOOL        bRoot;
    BOOL        bLast;
    PCHAR       pcTail;
    PCHAR       pcTemp;
    PCHAR       pcFileName;
    
    pramfs      = pramn->RAMN_pramfs;
    pramnFather = pramn->RAMN_pramnFather;
    
    pramnTemp = __ram_open(pramfs, pcNewName, &pramnNewFather, &bRoot, &bLast, &pcTail);
    if (!pramnTemp && (bRoot || (bLast == LW_FALSE))) {                 /*  ������ָ�������û��Ŀ¼    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pramn == pramnTemp) {                                           /*  ��ͬ                        */
        return  (ERROR_NONE);
    }
    
    if (S_ISDIR(pramn->RAMN_mode) && pramnNewFather) {
        if (__ram_move_check(pramn, pramnNewFather)) {                  /*  ���Ŀ¼�Ϸ���              */
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
    }
    
    pcFileName = lib_rindex(pcNewName, PX_DIVIDER);
    if (pcFileName) {
        pcFileName++;
    } else {
        pcFileName = pcNewName;
    }
    
    pcTemp = (PCHAR)__SHEAP_ALLOC(lib_strlen(pcFileName) + 1);          /*  Ԥ�������ֻ���              */
    if (pcTemp == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    lib_strcpy(pcTemp, pcFileName);
    
    if (pramnTemp) {
        if (!S_ISDIR(pramn->RAMN_mode) && S_ISDIR(pramnTemp->RAMN_mode)) {
            __SHEAP_FREE(pcTemp);
            _ErrorHandle(EISDIR);
            return  (PX_ERROR);
        }
        if (S_ISDIR(pramn->RAMN_mode) && !S_ISDIR(pramnTemp->RAMN_mode)) {
            __SHEAP_FREE(pcTemp);
            _ErrorHandle(ENOTDIR);
            return  (PX_ERROR);
        }
        
        iRet = __ram_unlink(pramnTemp);                                 /*  ɾ��Ŀ��                    */
        if (iRet) {
            __SHEAP_FREE(pcTemp);
            return  (PX_ERROR);
        }
    }
    
    if (pramnFather != pramnNewFather) {                                /*  Ŀ¼�����ı�                */
        if (pramnFather) {
            _List_Line_Del(&pramn->RAMN_lineBrother, 
                           &pramnFather->RAMN_plineSon);
        } else {
            _List_Line_Del(&pramn->RAMN_lineBrother, 
                           &pramfs->RAMFS_plineSon);
        }
        if (pramnNewFather) {
            _List_Line_Add_Ahead(&pramn->RAMN_lineBrother, 
                                 &pramnNewFather->RAMN_plineSon);
        } else {
            _List_Line_Add_Ahead(&pramn->RAMN_lineBrother, 
                                 &pramfs->RAMFS_plineSon);
        }
    }
    
    __SHEAP_FREE(pramn->RAMN_pcName);                                   /*  �ͷ�������                  */
    pramn->RAMN_pcName = pcTemp;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ram_stat
** ��������: ramfs ����ļ� stat
** �䡡��  : pramn            �ļ��ڵ�
**           pramfs           �ļ�ϵͳ
**           pstat            ��õ� stat
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __ram_stat (PRAM_NODE  pramn, PRAM_VOLUME  pramfs, struct stat  *pstat)
{
    if (pramn) {
        pstat->st_dev     = LW_DEV_MAKE_STDEV(&pramfs->RAMFS_devhdrHdr);
        pstat->st_ino     = (ino_t)pramn;
        pstat->st_mode    = pramn->RAMN_mode;
        pstat->st_nlink   = 1;
        pstat->st_uid     = pramn->RAMN_uid;
        pstat->st_gid     = pramn->RAMN_gid;
        pstat->st_rdev    = 1;
        pstat->st_size    = (off_t)pramn->RAMN_stSize;
        pstat->st_atime   = pramn->RAMN_timeAccess;
        pstat->st_mtime   = pramn->RAMN_timeChange;
        pstat->st_ctime   = pramn->RAMN_timeCreate;
        pstat->st_blksize = __RAM_BSIZE;
        pstat->st_blocks  = (blkcnt_t)pramn->RAMN_ulCnt;
    
    } else {
        pstat->st_dev     = LW_DEV_MAKE_STDEV(&pramfs->RAMFS_devhdrHdr);
        pstat->st_ino     = (ino_t)0;
        pstat->st_mode    = pramfs->RAMFS_mode;
        pstat->st_nlink   = 1;
        pstat->st_uid     = pramfs->RAMFS_uid;
        pstat->st_gid     = pramfs->RAMFS_gid;
        pstat->st_rdev    = 1;
        pstat->st_size    = 0;
        pstat->st_atime   = pramfs->RAMFS_time;
        pstat->st_mtime   = pramfs->RAMFS_time;
        pstat->st_ctime   = pramfs->RAMFS_time;
        pstat->st_blksize = __RAM_BSIZE;
        pstat->st_blocks  = 0;
    }
    
    pstat->st_resv1 = LW_NULL;
    pstat->st_resv2 = LW_NULL;
    pstat->st_resv3 = LW_NULL;
}
/*********************************************************************************************************
** ��������: __ram_statfs
** ��������: ramfs ����ļ� stat
** �䡡��  : pramfs           �ļ�ϵͳ
**           pstatfs          ��õ� statfs
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __ram_statfs (PRAM_VOLUME  pramfs, struct statfs  *pstatfs)
{
    pstatfs->f_type   = TMPFS_MAGIC;
    pstatfs->f_bsize  = __RAM_BSIZE;
    pstatfs->f_blocks = pramfs->RAMFS_ulMaxBlk;
    pstatfs->f_bfree  = pramfs->RAMFS_ulMaxBlk - pramfs->RAMFS_ulCurBlk;
    pstatfs->f_bavail = 1;
    
    pstatfs->f_files  = 0;
    pstatfs->f_ffree  = 0;
    
#if LW_CFG_CPU_WORD_LENGHT == 64
    pstatfs->f_fsid.val[0] = (int32_t)((addr_t)pramfs >> 32);
    pstatfs->f_fsid.val[1] = (int32_t)((addr_t)pramfs & 0xffffffff);
#else
    pstatfs->f_fsid.val[0] = (int32_t)pramfs;
    pstatfs->f_fsid.val[1] = 0;
#endif
    
    pstatfs->f_flag    = 0;
    pstatfs->f_namelen = PATH_MAX;
}

#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
                                                                        /*  LW_CFG_RAMFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
