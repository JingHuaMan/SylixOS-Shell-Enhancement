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
** ��   ��   ��: af_unix.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 12 �� 18 ��
**
** ��        ��: AF_UNIX ֧��

** BUG:
2012.12.27  UNIX �涨 len ���Ȳ����� sun_path �е� '\0' �ַ�.
2012.12.28  ���봫���ļ��������ʹ��ݽ���֤��Ĺ���.
2012.12.29  ����ʱ��� flags û�� MSG_NOSIGNAL �������������Ӳ��ɴ�Ĺܵ���Ҫ���� SIGPIPE �ź�.
2012.12.31  ʹ��С�ڴ滺����ʱ���ȴ��ڴ���з���.
2013.01.16  unix_sendto2 ���û�д�����, ��ȷ����ȫ����������˳�.
2013.01.20  Ϊ���� unix ��׼, connect ���ʹ�� NBIO ģʽ, �������������û����Ӧ�� errno Ϊ EINPROGRESS.
            ���� BSD socket �淶, �ύ select ����ʱ, ����Ҫ���� SO_ERROR �����ж� NBIO ����ȷ���.
2013.03.29  ���� unix_listen ����ֵ����.
            ���� unix_get/setsockopt ȱ��һ�� break;
2013.04.01  ���� __unix_have_event ����ֵ����.
2013.04.30  ��ȷ connect ��������Ϊ.
2013.06.21  ����������� unix ���ƿ�Ĺ���.
2013.09.06  ���� ECONNREFUSED �� ECONNRESET ��ʹ��.
            [ECONNREFUSED]
            The target address was not listening for connections or refused the connection request.
            [ECONNRESET]
            Remote host reset the connection request.
            unix_accept() ��û���㹻�ڴ�ʱ, ��Ҫ�ܾ����еȴ�������.
2013.11.17  ֧�� SOCK_SEQPACKET ��������.
2013.11.21  �����µķ����źŽӿ�.
2014.10.16  __unixFind() ����� listen ״̬ unix �׽��ֵ�����.
2015.01.01  ���� __unixFind() �� DGRAM �����жϴ���.
2015.01.08  SOCK_DGRAM û��һ�ν���������, ʣ�µ�������Ҫ����.
2016.12.14  accept connect ʹ�ò�ͬ�������ź���.
2017.08.01  �����й������ž����� 90 ���������, ףԸ�����������.
            AF_UNIX ֧�ֶ��̲߳��ж�д.
2017.08.31  shutdown д��, Զ�̶��л����������ݴ������.
2019.01.09  ���� __unixUpdateWriter() �жϴ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_NET_UNIX_EN > 0
#include "limits.h"
#include "sys/socket.h"
#include "sys/un.h"
#include "af_unix.h"
#include "lwip/mem.h"
#include "af_unix_msg.h"
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
extern void  __socketEnotify(void *file, LW_SEL_TYPE type, INT  iSoErr);
/*********************************************************************************************************
  ������
*********************************************************************************************************/
#define __AF_UNIX_ADDROFFSET        offsetof(struct sockaddr_un, sun_path)
#define __AF_UNIX_DEF_FLAG          0777
#define __AF_UNIX_DEF_BUFSIZE       (LW_CFG_KB_SIZE * 64)               /*  Ĭ��Ϊ 64K ���ջ���         */
#define __AF_UNIX_DEF_BUFMAX        (LW_CFG_KB_SIZE * 256)              /*  Ĭ��Ϊ 256K ���ջ���        */
#define __AF_UNIX_DEF_BUFMIN        (LW_CFG_KB_SIZE * 8)                /*  ��С���ջ����С            */

#ifdef __SYLIXOS_LITE
#define __AF_UNIX_PIPE_BUF          (LW_CFG_KB_SIZE * 2)                /*  һ��ԭ�Ӳ��������ݴ�С      */
#define __AF_UNIX_PIPE_BUF_SHIFT    11                                  /*  1 << 11 == 2K               */
#else
#define __AF_UNIX_PIPE_BUF          (LW_CFG_KB_SIZE * 8)                /*  һ��ԭ�Ӳ��������ݴ�С      */
#define __AF_UNIX_PIPE_BUF_SHIFT    13                                  /*  1 << 13 == 8K               */
#endif

#define __AF_UNIX_PART_256          LW_CFG_AF_UNIX_256_POOLS            /*  256 �ֽ��ڴ������          */
#define __AF_UNIX_PART_512          LW_CFG_AF_UNIX_512_POOLS            /*  512 �ֽ��ڴ������          */
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_LIST_LINE_HEADER          _G_plineAfUnix;
static LW_OBJECT_HANDLE             _G_hAfUnixMutex;

#define __AF_UNIX_LOCK()            API_SemaphoreMPend(_G_hAfUnixMutex, LW_OPTION_WAIT_INFINITE)
#define __AF_UNIX_UNLOCK()          API_SemaphoreMPost(_G_hAfUnixMutex)

static LW_OBJECT_HANDLE             _G_hAfUnixPart256;
static LW_OBJECT_HANDLE             _G_hAfUnixPart512;

#define __AF_UNIX_PART_256_SIZE     (__AF_UNIX_PART_256 * (256 / sizeof(LW_STACK)))
#define __AF_UNIX_PART_512_SIZE     (__AF_UNIX_PART_512 * (512 / sizeof(LW_STACK)))

static LW_STACK                    *_G_pstkUnixPart256;
static LW_STACK                    *_G_pstkUnixPart512;
/*********************************************************************************************************
  �����
  
  connect ���������� UNIX_hCanWrite
  accept  ���������� UNIX_hCanRead
*********************************************************************************************************/
#define __AF_UNIX_WCONN(pafunix)    API_SemaphoreBPend(pafunix->UNIX_hCanWrite, \
                                                       pafunix->UNIX_ulConnTimeout)
#define __AF_UNIX_SCONN(pafunix)    API_SemaphoreBPost(pafunix->UNIX_hCanWrite)
#define __AF_UNIX_CCONN(pafunix)    API_SemaphoreBClear(pafunix->UNIX_hCanWrite)
                                                       
#define __AF_UNIX_WACCE(pafunix)    API_SemaphoreBPend(pafunix->UNIX_hCanRead, \
                                                       LW_OPTION_WAIT_INFINITE)
#define __AF_UNIX_SACCE(pafunix)    API_SemaphoreBPost(pafunix->UNIX_hCanRead)
#define __AF_UNIX_CACCE(pafunix)    API_SemaphoreBClear(pafunix->UNIX_hCanRead)
                                                       
#define __AF_UNIX_WREAD(pafunix)    API_SemaphoreBPend(pafunix->UNIX_hCanRead, \
                                                       pafunix->UNIX_ulRecvTimeout)
#define __AF_UNIX_SREAD(pafunix)    API_SemaphoreBPost(pafunix->UNIX_hCanRead)
#define __AF_UNIX_CREAD(pafunix)    API_SemaphoreBClear(pafunix->UNIX_hCanRead)
                                                       
#define __AF_UNIX_WWRITE(pafunix)   API_SemaphoreBPend(pafunix->UNIX_hCanWrite, \
                                                       pafunix->UNIX_ulSendTimeout)
#define __AF_UNIX_SWRITE(pafunix)   API_SemaphoreBPost(pafunix->UNIX_hCanWrite)
#define __AF_UNIX_CWRITE(pafunix)   API_SemaphoreBClear(pafunix->UNIX_hCanWrite)
/*********************************************************************************************************
  �ȴ��ж�
*********************************************************************************************************/
#define __AF_UNIX_IS_NBIO(pafunix, flags)   \
        ((pafunix->UNIX_iFlag & O_NONBLOCK) || (flags & MSG_DONTWAIT))
        
#define __AF_UNIX_TYPE(pafunix)     (pafunix->UNIX_iType)
/*********************************************************************************************************
** ��������: __unixBufAlloc
** ��������: �����ڴ�
** �䡡��  : stLen                 ����
** �䡡��  : ������������Ϣ�ڵ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PVOID  __unixBufAlloc (size_t  stLen)
{
    PVOID   pvMem;

    if (stLen <= 256) {
        pvMem = API_PartitionGet(_G_hAfUnixPart256);
        if (pvMem) {
            return  (pvMem);
        }
    }
    if (stLen <= 512) {
        pvMem = API_PartitionGet(_G_hAfUnixPart512);
        if (pvMem) {
            return  (pvMem);
        }
    }
    
    return  (mem_malloc(stLen));
}
/*********************************************************************************************************
** ��������: __unixBufFree
** ��������: �����ڴ�
** �䡡��  : pvMem                 �ڴ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __unixBufFree (PVOID  pvMem)
{
    if (((PLW_STACK)pvMem >= &_G_pstkUnixPart256[0]) && 
        ((PLW_STACK)pvMem <  &_G_pstkUnixPart256[__AF_UNIX_PART_256_SIZE])) {
        API_PartitionPut(_G_hAfUnixPart256, pvMem);
    
    } else if (((PLW_STACK)pvMem >= &_G_pstkUnixPart512[0]) && 
               ((PLW_STACK)pvMem <  &_G_pstkUnixPart512[__AF_UNIX_PART_512_SIZE])) {
        API_PartitionPut(_G_hAfUnixPart512, pvMem);
    
    } else {
        mem_free(pvMem);
    }
}
/*********************************************************************************************************
** ��������: __unixCreateMsg
** ��������: ����һ����Ϣ�ڵ� (�����������Ͳ���Ҫ������ַ��Ϣ, �ɽ�ʡ�ռ�, ����ٶ�)
** �䡡��  : pafunixSender         ���ͷ�
**           pafunixRecver         ���շ�
**           pvMsg                 ��Ϣ
**           stLen                 ��Ϣ����
**           pvMsgEx               ��չ��Ϣ
**           uiLenEx               ��չ��Ϣ����
** �䡡��  : ������������Ϣ�ڵ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static AF_UNIX_N  *__unixCreateMsg (AF_UNIX_T  *pafunixSender, 
                                    AF_UNIX_T  *pafunixRecver,
                                    CPVOID  pvMsg, size_t  stLen,
                                    CPVOID  pvMsgEx, socklen_t  uiLenEx)
{
    AF_UNIX_N  *pafunixmsg;
    size_t      stPathLen;
    INT         iError;
    
    if (__AF_UNIX_TYPE(pafunixSender) == SOCK_DGRAM) {                 /*  SOCK_DGRAM ��Ҫ������ַ��Ϣ  */
        stPathLen = lib_strlen(pafunixSender->UNIX_cFile);
    } else {
        stPathLen = 0;
    }
    
    pafunixmsg = (AF_UNIX_N *)__unixBufAlloc(sizeof(AF_UNIX_N) + stPathLen + stLen);
    if (pafunixmsg == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }
    
    pafunixmsg->UNIM_pcMsg = (PCHAR)pafunixmsg + sizeof(AF_UNIX_N) + stPathLen;
    lib_memcpy(pafunixmsg->UNIM_pcMsg, pvMsg, stLen);
    
    if (stPathLen > 0) {
        lib_strcpy(pafunixmsg->UNIM_cPath, pafunixSender->UNIX_cFile);
    } else {
        pafunixmsg->UNIM_cPath[0] = PX_EOS;
    }
    
    pafunixmsg->UNIM_stLen    = stLen;
    pafunixmsg->UNIM_stOffset = 0;
    
    if (pvMsgEx && uiLenEx) {                                           /*  �Ƿ���Ҫ��չ��Ϣ            */
        AF_UNIX_NEX  *punie = (AF_UNIX_NEX *)__unixBufAlloc(sizeof(AF_UNIX_NEX) + uiLenEx);
        if (punie == LW_NULL) {
            __unixBufFree(pafunixmsg);
            _ErrorHandle(ENOMEM);
            return  (LW_NULL);
        }
        punie->UNIE_pcMsgEx     = (PCHAR)punie + sizeof(AF_UNIX_NEX);
        punie->UNIE_uiLenEx     = uiLenEx;
        punie->UNIE_pid         = __PROC_GET_PID_CUR();                 /*  ��¼���ͷ��� pid            */
        punie->UNIE_bValid      = LW_TRUE;                              /*  ��չ��Ϣ��Ч�ɱ�����        */
        punie->UNIE_bNeedUnProc = LW_TRUE;                              /*  ��Ҫ unproc ����            */
        lib_memcpy(punie->UNIE_pcMsgEx, pvMsgEx, uiLenEx);              /*  ������չ��Ϣ                */
        
        iError = __unix_smsg_proc(pafunixRecver,
                                  punie->UNIE_pcMsgEx, 
                                  punie->UNIE_uiLenEx,
                                  punie->UNIE_pid);                     /*  ��չ��Ϣ����ǰԤ����        */
        if (iError < ERROR_NONE) {                                      /*  �������, �� proc �ڲ��Ѵ���*/
            __unixBufFree(punie);                                       /*  ��ص� errno                */
            __unixBufFree(pafunixmsg);
            return  (LW_NULL);
        }
        pafunixmsg->UNIM_punie = punie;
    } else {
        pafunixmsg->UNIM_punie = LW_NULL;
    }
    
    return  (pafunixmsg);
}
/*********************************************************************************************************
** ��������: __unixDeleteMsg
** ��������: ɾ��һ����Ϣ�ڵ�
** �䡡��  : pafunixmsg            ��Ϣ�ڵ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __unixDeleteMsg (AF_UNIX_N  *pafunixmsg)
{
    if (pafunixmsg->UNIM_punie) {
        AF_UNIX_NEX  *punie = pafunixmsg->UNIM_punie;
        if (punie->UNIE_bNeedUnProc) {                                  /*  �Ƿ���Ҫ UnProc ����        */
            __unix_smsg_unproc(punie->UNIE_pcMsgEx,
                               punie->UNIE_uiLenEx,
                               punie->UNIE_pid);                        /*  �ظ���ǰ��״̬              */
        }
        __unixBufFree(punie);                                           /*  �ͷ���չ��Ϣ�ڴ�            */
    }
    __unixBufFree(pafunixmsg);
}
/*********************************************************************************************************
** ��������: __unixDeleteMsg
** ��������: ɾ��һ�� unix �ڵ��������Ϣ�ڵ�
** �䡡��  : pafunix               unix �ڵ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __unixDeleteAllMsg (AF_UNIX_T  *pafunix)
{
    AF_UNIX_N       *pafunixmsg;
    AF_UNIX_Q       *pafunixq;
    
    pafunixq = &pafunix->UNIX_unixq;
    
    while (pafunixq->UNIQ_pmonoHeader) {
        pafunixmsg = (AF_UNIX_N *)pafunixq->UNIQ_pmonoHeader;
        _list_mono_allocate_seq(&pafunixq->UNIQ_pmonoHeader,
                                &pafunixq->UNIQ_pmonoTail);
        __unixDeleteMsg(pafunixmsg);
    }
    
    pafunixq->UNIQ_stTotal = 0;
}
/*********************************************************************************************************
** ��������: __unixCanWrite
** ��������: �ж��Ƿ������ָ���Ľ��սڵ㷢������
** �䡡��  : pafunixRecver         ���շ�
** �䡡��  : �Ƿ��д
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  __unixCanWrite (AF_UNIX_T  *pafunixRecver)
{
    size_t      stFreeBuf;
    
    if (pafunixRecver->UNIX_iStatus == __AF_UNIX_STATUS_LISTEN) {       /*  CONNECT ״̬�²���д        */
        return  (LW_FALSE);
    }
    
    if (pafunixRecver->UNIX_stMaxBufSize > 
        pafunixRecver->UNIX_unixq.UNIQ_stTotal) {
        stFreeBuf = pafunixRecver->UNIX_stMaxBufSize
                  - pafunixRecver->UNIX_unixq.UNIQ_stTotal;             /*  ��öԷ�ʣ�໺���С        */
    } else {
        stFreeBuf = 0;
    }
    if (stFreeBuf < __AF_UNIX_PIPE_BUF) {                               /*  �Է���������                */
        return  (LW_FALSE);
    
    } else {
        return  (LW_TRUE);                                              /*  ���Է���                    */
    }
}
/*********************************************************************************************************
** ��������: __unixCanRead
** ��������: ��ǰ�ڵ��Ƿ�ɶ�
** �䡡��  : pafunix           unix �ڵ�
**           flags             MSG_PEEK or MSG_WAITALL 
**           stLen             ����� MSG_WAITALL ��Ҫ�жϳ���.
** �䡡��  : �Ƿ�ɶ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  __unixCanRead (AF_UNIX_T  *pafunix, INT  flags, size_t  stLen)
{
    AF_UNIX_Q       *pafunixq;
    
    pafunixq = &pafunix->UNIX_unixq;
    
    if (pafunixq->UNIQ_pmonoHeader == LW_NULL) {                        /*  û����Ϣ�ɽ���              */
        return  (LW_FALSE);
    }
    
    if (flags & MSG_WAITALL) {
        if (pafunixq->UNIQ_stTotal < stLen) {                           /*  ���ݲ����޷�����            */
            return  (LW_FALSE);
        }
    }
    
    return  (LW_TRUE);
}
/*********************************************************************************************************
** ��������: __unixCanAccept
** ��������: �鿴�������ӵĶ������Ƿ��еȴ��Ľڵ�
** �䡡��  : pafunix               ���ƿ�
** �䡡��  : �ȴ����ӵĽڵ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  __unixCanAccept (AF_UNIX_T  *pafunix)
{
    if (pafunix->UNIX_pringConnect == LW_NULL) {
        return  (LW_FALSE);
    
    } else {
        return  (LW_TRUE);
    }
}
/*********************************************************************************************************
** ��������: __unixSendtoMsg
** ��������: ��һ�� unix �ڵ㷢��һ����Ϣ
** �䡡��  : pafunixSender         ���ͷ�
**           pafunixRecver         ���շ�
**           pvMsg                 ��Ϣ
**           stLen                 ��Ϣ����
**           pvMsgEx               ��չ��Ϣ
**           uiLenEx               ��չ��Ϣ����
**           flags                 Ŀǰδʹ��
** �䡡��  : ���͵��ֽ���
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �˺�����෢�� __AF_UNIX_PIPE_BUF ���ֽ�.
*********************************************************************************************************/
static ssize_t  __unixSendtoMsg (AF_UNIX_T  *pafunixSender, AF_UNIX_T  *pafunixRecver, 
                                 CPVOID  pvMsg, size_t  stLen, 
                                 CPVOID  pvMsgEx, socklen_t  uiLenEx, 
                                 INT  flags)
{
    AF_UNIX_N  *pafunixmsg;
    AF_UNIX_Q  *pafunixq;
    
    if (__AF_UNIX_TYPE(pafunixSender) == SOCK_STREAM) {
        if (stLen > __AF_UNIX_PIPE_BUF) {
            stLen = __AF_UNIX_PIPE_BUF;                                 /*  һ�η������ PIPE_BUF       */
        }

    } else {                                                            /*  ��Ҫԭ��Ͷ��                */
        if (stLen > pafunixRecver->UNIX_stMaxBufSize) {                 /*  ��󲻵ó������ջ����С    */
            stLen = pafunixRecver->UNIX_stMaxBufSize;
        }
    }
    
    pafunixmsg = __unixCreateMsg(pafunixSender, pafunixRecver, 
                                 pvMsg, stLen, pvMsgEx, uiLenEx);
    if (pafunixmsg == LW_NULL) {                                        /*  �����Ѿ������˶�Ӧ�� errno  */
        return  (PX_ERROR);
    }
    
    pafunixq = &pafunixRecver->UNIX_unixq;
    
    _list_mono_free_seq(&pafunixq->UNIQ_pmonoHeader,
                        &pafunixq->UNIQ_pmonoTail,
                        &pafunixmsg->UNIM_monoManage);                  /*  ͨ�� mono free ����������� */
    
    pafunixq->UNIQ_stTotal += stLen;                                    /*  ���»������е�������        */
    
    return  ((ssize_t)stLen);
}
/*********************************************************************************************************
** ��������: __unixRecvfromMsg
** ��������: ��һ�� unix �ڵ����һ����Ϣ
** �䡡��  : pafunixRecver         ���շ�
**           pcMsg                 ��Ϣ
**           stLen                 ��Ϣ����
**           pvMsgEx               ��չ��Ϣ
**           puiLenEx              ��չ��Ϣ���� (������Ҫ������Ϣ�������Ĵ�С, ����ֵΪ������չ��Ϣ�ĳ���)
**           flags                 MSG_PEEK (MSG_WAITALL ����� recvfrom2 ֧��) 
**           from                  Զ�̵�ַ����
**           fromlen               Զ�̵�ַ����
**           msg_flags             ���� flags
** �䡡��  : ���յ��ֽ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __unixRecvfromMsg (AF_UNIX_T  *pafunixRecver, 
                                   PVOID pvMsg, size_t  stLen, 
                                   PVOID pvMsgEx, socklen_t  *puiLenEx, INT  flags,
                                   struct sockaddr_un *from, socklen_t *fromlen,
                                   INT  *msg_flags)
{
    AF_UNIX_NEX     *punie;
    AF_UNIX_N       *pafunixmsg;
    AF_UNIX_Q       *pafunixq;
    size_t           stMsgLen;
    size_t           stPathLen;
    size_t           stBufPathLen;
    
    pafunixq = &pafunixRecver->UNIX_unixq;
    
    if (pafunixq->UNIQ_pmonoHeader == LW_NULL) {                        /*  û����Ϣ�ɽ���              */
        return  (0);
    }
    
    pafunixmsg = (AF_UNIX_N *)pafunixq->UNIQ_pmonoHeader;               /*  ���һ����Ҫ���յ���Ϣ���ƿ�*/
    
    /*
     *  ������Ϣ���ͷ��ĵ�ַ.
     */
    if (from && fromlen && (*fromlen > __AF_UNIX_ADDROFFSET)) {         /*  �Ƿ���Ҫ��ȡ��ַ��Ϣ        */
        stBufPathLen = (*fromlen - __AF_UNIX_ADDROFFSET);
        if (__AF_UNIX_TYPE(pafunixRecver) == SOCK_DGRAM) {              /*  ������������Ҫ��ÿ�����л�ȡ*/
            stPathLen = lib_strlen(pafunixmsg->UNIM_cPath);
            if (stPathLen > stBufPathLen) {
                stPathLen = stBufPathLen;
                if (msg_flags) {
                    (*msg_flags) |= MSG_CTRUNC;
                }
            }
            lib_strncpy(from->sun_path, pafunixmsg->UNIM_cPath, stBufPathLen);
        
        } else {                                                        /*  ����������Ҫ�ӶԵȽڵ��ȡ  */
            AF_UNIX_T  *pafunixPeer = pafunixRecver->UNIX_pafunxPeer;
            if (pafunixPeer) {
                stPathLen = lib_strlen(pafunixPeer->UNIX_cFile);
                if (stPathLen > stBufPathLen) {
                    stPathLen = stBufPathLen;
                    if (msg_flags) {
                        (*msg_flags) |= MSG_CTRUNC;
                    }
                }
                lib_strncpy(from->sun_path, pafunixPeer->UNIX_cFile, stBufPathLen);
            
            } else {                                                    /*  �����Ѿ��ж�, ����ʣ������  */
                stPathLen = 0;
                if (stBufPathLen > 0) {
                    from->sun_path[0] = PX_EOS;
                }
            }
        }
        
        from->sun_family = AF_UNIX;
        from->sun_len    = (uint8_t)(__AF_UNIX_ADDROFFSET + stPathLen); /*  ������ \0 ���ܳ���          */
        *fromlen = from->sun_len;
    }
    
    /*
     *  ���Ƚ�����չ��Ϣ
     */
    if (pvMsgEx && puiLenEx && *puiLenEx) {                             /*  ��Ҫ������չ��Ϣ            */
        punie = pafunixmsg->UNIM_punie;
        if (punie && punie->UNIE_bValid) {                              /*  ������չ���ݲ�����Ч        */
            if (punie->UNIE_bNeedUnProc) {
                __unix_rmsg_proc(punie->UNIE_pcMsgEx,
                                 punie->UNIE_uiLenEx,
                                 punie->UNIE_pid, flags);
                punie->UNIE_bNeedUnProc = LW_FALSE;                     /*  �������                    */
            }
            if (*puiLenEx > punie->UNIE_uiLenEx) {
                *puiLenEx = punie->UNIE_uiLenEx;
            }
            lib_memcpy(pvMsgEx, punie->UNIE_pcMsgEx, *puiLenEx);        /*  ������չ��Ϣ                */
        } else {
            *puiLenEx = 0;                                              /*  û���������                */
        }
    } else if (puiLenEx) {
        *puiLenEx = 0;
    }
    
    /*
     *  ����������Ϣ
     */
    stMsgLen = pafunixmsg->UNIM_stLen - pafunixmsg->UNIM_stOffset;      /*  ������Ϣ�ڵ�����Ϣ�ĳ���    */
    if (stLen > stMsgLen) {                                             /*  ���Ի�ȡȫ����Ϣ            */
        stLen = stMsgLen;
        if ((__AF_UNIX_TYPE(pafunixRecver) != SOCK_STREAM) && msg_flags) {
            (*msg_flags) |= MSG_EOR;
        }
    
    } else {
        if ((__AF_UNIX_TYPE(pafunixRecver) == SOCK_DGRAM) && msg_flags) {
            (*msg_flags) |= MSG_TRUNC;
        }
    }
                                                                        /*  ��������                    */
    lib_memcpy(pvMsg, &pafunixmsg->UNIM_pcMsg[pafunixmsg->UNIM_stOffset], stLen);
    
    if (flags & MSG_PEEK) {                                             /*  ����Ԥ��, ��ɾ������        */
        return  ((ssize_t)stLen);
    }
    
    if ((stLen == stMsgLen) || 
        (pafunixRecver->UNIX_iType == SOCK_DGRAM)) {                    /*  �Ѿ���ȡ��ϻ��� DGRAM ��   */
        _list_mono_allocate_seq(&pafunixq->UNIQ_pmonoHeader,
                                &pafunixq->UNIQ_pmonoTail);             /*  ����Ϣ�Ӷ�����ɾ��          */
        __unixDeleteMsg(pafunixmsg);                                    /*  �ͷ���Ϣ�ڵ�                */
        
        pafunixq->UNIQ_stTotal -= stMsgLen;                             /*  ���������ֽ���              */
        
    } else {
        if (pafunixmsg->UNIM_punie) {
            pafunixmsg->UNIM_punie->UNIE_bValid = LW_FALSE;             /*  ��չ��Ϣֻ�ܽ���һ��        */
        }
        
        pafunixmsg->UNIM_stOffset += stLen;                             /*  ��ǰ����ָ��                */
        pafunixq->UNIQ_stTotal    -= stLen;                             /*  ���ٵ��ζ�ȡ�ֽ���          */
    }
    
    return  ((ssize_t)stLen);
}
/*********************************************************************************************************
** ��������: __unixUpdateReader
** ��������: ��һ�� unix �ڵ㷢�Ϳɶ�
** �䡡��  : pafunix               ���սڵ�
**           iSoErr                ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __unixUpdateReader (AF_UNIX_T  *pafunix, INT  iSoErr)
{
    __AF_UNIX_SREAD(pafunix);
    
    __socketEnotify(pafunix->UNIX_sockFile, SELREAD, iSoErr);           /*  ���� select �ɶ�            */
}
/*********************************************************************************************************
** ��������: __unixUpdateWriter
** ��������: ��һ�� unix �ڵ㷢�Ϳ�д
** �䡡��  : pafunix                ���ͽڵ�
**           iSoErr                 ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __unixUpdateWriter (AF_UNIX_T  *pafunix, INT  iSoErr)
{
    AF_UNIX_T  *pafunixPeer;

    if (pafunix->UNIX_stMaxBufSize > pafunix->UNIX_unixq.UNIQ_stTotal) {
        if ((pafunix->UNIX_stMaxBufSize -
             pafunix->UNIX_unixq.UNIQ_stTotal) >= __AF_UNIX_PIPE_BUF) { /*  ��֤д��ԭ����              */
            __AF_UNIX_SWRITE(pafunix);

            pafunixPeer = pafunix->UNIX_pafunxPeer;                     /*  �������Զ�̶Եȷ�          */
            if (pafunixPeer) {
                __socketEnotify(pafunixPeer->UNIX_sockFile, SELWRITE, iSoErr);
            }                                                           /*  Զ�̶Եȷ� select ��д      */
        }
    }
}
/*********************************************************************************************************
** ��������: __unixUpdateExcept
** ��������: ��һ�� unix �ڵ㷢���쳣
** �䡡��  : pafunix               �ڵ�
**           iSoErr                ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __unixUpdateExcept (AF_UNIX_T  *pafunix, INT  iSoErr)
{
    AF_UNIX_T  *pafunixPeer;

    __AF_UNIX_SREAD(pafunix);
    
    __socketEnotify(pafunix->UNIX_sockFile, SELREAD, iSoErr);           /*  ���� select �ɶ�            */
    
    __AF_UNIX_SWRITE(pafunix);
    
    pafunixPeer = pafunix->UNIX_pafunxPeer;                             /*  �������Զ�̶Եȷ�          */
    if (pafunixPeer) {
        __socketEnotify(pafunixPeer->UNIX_sockFile, SELWRITE, iSoErr);  /*  Զ�̶Եȷ� select ��д      */
    }
    
    __socketEnotify(pafunix->UNIX_sockFile, SELEXCEPT, iSoErr);         /*  ���� select ���쳣          */
}
/*********************************************************************************************************
** ��������: __unixUpdateConnecter
** ��������: �����������ӵ� unix �ڵ� (select ֻҪ���������Ŀ�д�Ϳ�����)
** �䡡��  : pafunixConn         �������ӵĽڵ�
**           iSoErr              ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __unixUpdateConnecter (AF_UNIX_T  *pafunixConn, INT  iSoErr)
{
    __AF_UNIX_SCONN(pafunixConn);
    
    __socketEnotify(pafunixConn->UNIX_sockFile, SELWRITE, iSoErr);      /*  �������ӵĽڵ� select ��д  */
}
/*********************************************************************************************************
** ��������: __unixUpdateAccept
** ��������: ���� accept �ڵ�, ��ʾ�нڵ���������
** �䡡��  : pafunixAcce         accept �ڵ�
**           iSoErr              ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __unixUpdateAccept (AF_UNIX_T  *pafunixAcce, INT  iSoErr)
{
    __AF_UNIX_SACCE(pafunixAcce);
    
    __socketEnotify(pafunixAcce->UNIX_sockFile, SELREAD, iSoErr);       /*  accept �ڵ� select �ɶ�     */
}
/*********************************************************************************************************
** ��������: __unixShutdownR
** ��������: �رձ��ض��Ĺ���
** �䡡��  : pafunix               �ڵ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __unixShutdownR (AF_UNIX_T  *pafunix)
{
    AF_UNIX_T  *pafunixPeer = pafunix->UNIX_pafunxPeer;
    
    if ((pafunix->UNIX_iShutDFlag & __AF_UNIX_SHUTD_R) == 0) {
        pafunix->UNIX_iShutDFlag |= __AF_UNIX_SHUTD_R;                  /*  �Ҳ����ٶ�                  */
        if (pafunixPeer) {
            pafunixPeer->UNIX_iShutDFlag |= __AF_UNIX_SHUTD_W;          /*  Զ�̲�����д                */
            __unixUpdateWriter(pafunixPeer, ESHUTDOWN);                 /*  ����Զ�̽ڵ�ȴ�д          */
        }
        __unixDeleteAllMsg(pafunix);                                    /*  ɾ��û�н��յ���Ϣ          */
        __unixUpdateReader(pafunix, ENOTCONN);                          /*  �����ҵĶ��ȴ�              */
    }
}
/*********************************************************************************************************
** ��������: __unixShutdownW
** ��������: �رձ���д�Ĺ���
** �䡡��  : pafunix               �ڵ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ﲢ�����������, ���Է���ʣ�����ݵĻ���.
*********************************************************************************************************/
static VOID  __unixShutdownW (AF_UNIX_T  *pafunix)
{
    AF_UNIX_T  *pafunixPeer = pafunix->UNIX_pafunxPeer;
    
    if ((pafunix->UNIX_iShutDFlag & __AF_UNIX_SHUTD_W) == 0) {
        pafunix->UNIX_iShutDFlag |= __AF_UNIX_SHUTD_W;                  /*  �Ҳ�����д                  */
        if (pafunixPeer) {
            pafunixPeer->UNIX_iShutDFlag |= __AF_UNIX_SHUTD_R;          /*  Զ�̲����ٶ�                */
            __unixUpdateReader(pafunixPeer, ENOTCONN);                  /*  ����Զ�̽ڵ�ȴ���          */
        }
        __unixUpdateWriter(pafunix, ESHUTDOWN);                         /*  �����ҵĵȴ�д              */
    }
}
/*********************************************************************************************************
** ��������: __unixCreate
** ��������: ����һ�� af_unix ���ƿ�
** �䡡��  : iType                 SOCK_STREAM / SOCK_DGRAM / SOCK_SEQPACKET
** �䡡��  : ���ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static AF_UNIX_T  *__unixCreate (INT  iType)
{
    AF_UNIX_T   *pafunix;

    pafunix = (AF_UNIX_T *)__SHEAP_ALLOC(sizeof(AF_UNIX_T));
    if (pafunix == LW_NULL) {
        return  (LW_NULL);
    }
    lib_bzero(pafunix, sizeof(AF_UNIX_T));
    
    pafunix->UNIX_iReuse       = 0;
    pafunix->UNIX_iFlag        = O_RDWR;
    pafunix->UNIX_iType        = iType;
    pafunix->UNIX_iStatus      = __AF_UNIX_STATUS_NONE;
    pafunix->UNIX_iShutDFlag   = 0;
    pafunix->UNIX_iBacklog     = 1;
    pafunix->UNIX_pafunxPeer   = LW_NULL;
    pafunix->UNIX_stMaxBufSize = __AF_UNIX_DEF_BUFSIZE;
    
    pafunix->UNIX_ulSendTimeout = LW_OPTION_WAIT_INFINITE;
    pafunix->UNIX_ulRecvTimeout = LW_OPTION_WAIT_INFINITE;
    pafunix->UNIX_ulConnTimeout = LW_OPTION_WAIT_INFINITE;
    
    pafunix->UNIX_hCanRead = API_SemaphoreBCreate("unix_rlock", LW_FALSE, 
                                                  LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (pafunix->UNIX_hCanRead == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(pafunix);
        return  (LW_NULL);
    }
    
    pafunix->UNIX_hCanWrite = API_SemaphoreBCreate("unix_wlock", LW_FALSE, 
                                                   LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (pafunix->UNIX_hCanWrite == LW_OBJECT_HANDLE_INVALID) {
        API_SemaphoreBDelete(&pafunix->UNIX_hCanRead);
        __SHEAP_FREE(pafunix);
        return  (LW_NULL);
    }
    
    __AF_UNIX_LOCK();
    _List_Line_Add_Ahead(&pafunix->UNIX_lineManage, &_G_plineAfUnix);
    __AF_UNIX_UNLOCK();
    
    return  (pafunix);
}
/*********************************************************************************************************
** ��������: __unixDelete
** ��������: ɾ��һ�� af_unix ���ƿ� (�������п��ƿ�, ɾ�� UNIX_pafunxPeer)
** �䡡��  : pafunix               ���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __unixDelete (AF_UNIX_T  *pafunix)
{
    AF_UNIX_T       *pafunixTemp;
    PLW_LIST_LINE    plineTemp;
    
    __AF_UNIX_LOCK();
    __unixDeleteAllMsg(pafunix);                                        /*  ɾ������δ���յ���Ϣ        */
    for (plineTemp  = _G_plineAfUnix;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
    
        pafunixTemp = (AF_UNIX_T *)plineTemp;
        if (pafunixTemp->UNIX_pafunxPeer == pafunix) {
            pafunixTemp->UNIX_pafunxPeer = LW_NULL;
        }
    }
    _List_Line_Del(&pafunix->UNIX_lineManage, &_G_plineAfUnix);
    __AF_UNIX_UNLOCK();
    
    API_SemaphoreBDelete(&pafunix->UNIX_hCanRead);
    API_SemaphoreBDelete(&pafunix->UNIX_hCanWrite);
    
    __SHEAP_FREE(pafunix);
}
/*********************************************************************************************************
** ��������: __unixFind
** ��������: ��ѯһ���ڵ�
** �䡡��  : pcPath                ��ѯһ���ڵ�
**           iType                 ����
**           bListen               �Ƿ��ѯһ�� listen �ڵ�
** �䡡��  : pafunix
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static AF_UNIX_T  *__unixFind (CPCHAR  pcPath, INT  iType, BOOL  bListen)
{
    AF_UNIX_T       *pafunixTemp;
    PLW_LIST_LINE    plineTemp;
    
    for (plineTemp  = _G_plineAfUnix;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
    
        pafunixTemp = (AF_UNIX_T *)plineTemp;
        if ((__AF_UNIX_TYPE(pafunixTemp) == iType) &&
            (lib_strcmp(pafunixTemp->UNIX_cFile, pcPath) == 0)) {
            if ((iType == SOCK_DGRAM) || !bListen ||
                (pafunixTemp->UNIX_iStatus == __AF_UNIX_STATUS_LISTEN)) {
                return  (pafunixTemp);
            }
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __unixConnect
** ��������: pafunix �������ӵ� pafunixAcce
** �䡡��  : pafunix               ���ƿ�
**           pafunixAcce           ���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __unixConnect (AF_UNIX_T  *pafunix, AF_UNIX_T  *pafunixAcce)
{
    pafunix->UNIX_iStatus = __AF_UNIX_STATUS_CONNECT;
    pafunix->UNIX_pafunxPeer = pafunixAcce;

    _List_Ring_Add_Last(&pafunix->UNIX_ringConnect,
                        &pafunixAcce->UNIX_pringConnect);
                        
    pafunixAcce->UNIX_iConnNum++;
}
/*********************************************************************************************************
** ��������: __unixUnconnect
** ��������: ���������Ӷ������˳���
** �䡡��  : pafunixConn             �������ӵĿ��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __unixUnconnect (AF_UNIX_T  *pafunixConn)
{
    AF_UNIX_T  *pafunixAcce = pafunixConn->UNIX_pafunxPeer;
    
    pafunixConn->UNIX_iStatus = __AF_UNIX_STATUS_NONE;
    
    if (pafunixAcce) {
        _List_Ring_Del(&pafunixConn->UNIX_ringConnect,
                       &pafunixAcce->UNIX_pringConnect);
                       
        pafunixAcce->UNIX_iConnNum--;
        
        pafunixConn->UNIX_pafunxPeer = LW_NULL;
    }
}
/*********************************************************************************************************
** ��������: __unixAccept
** ��������: ���������ӵĶ�����ȡ��һ��
** �䡡��  : pafunix               ���ƿ�
** �䡡��  : �ȴ����ӵĽڵ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static AF_UNIX_T  *__unixAccept (AF_UNIX_T  *pafunix)
{
    AF_UNIX_T      *pafunixConn;
    PLW_LIST_RING   pringConn;
    
    if (pafunix->UNIX_pringConnect == LW_NULL) {
        return  (LW_NULL);
    }
    
    pringConn = pafunix->UNIX_pringConnect;                             /*  �ӵȴ���ͷ��ȡ              */
    
    pafunixConn = _LIST_ENTRY(pringConn, AF_UNIX_T, UNIX_ringConnect);
    
    _List_Ring_Del(&pafunixConn->UNIX_ringConnect,
                   &pafunix->UNIX_pringConnect);                        /*  �ӵȴ����ӱ���ɾ��          */
    
    pafunix->UNIX_iConnNum--;
    
    pafunixConn->UNIX_pafunxPeer = LW_NULL;
    
    return  (pafunixConn);
}
/*********************************************************************************************************
** ��������: __unixRefuseAll
** ��������: �������ӵĶ���ɾ�����еȴ����ӽڵ� (unix_close ʱ����)
** �䡡��  : pafunix               ���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __unixRefuseAll (AF_UNIX_T  *pafunix)
{
    AF_UNIX_T      *pafunixConn;
    PLW_LIST_RING   pringConn;
    
    while (pafunix->UNIX_pringConnect) {
        pringConn = pafunix->UNIX_pringConnect;                         /*  �ӵȴ���ͷ��ȡ              */
        
        pafunixConn = _LIST_ENTRY(pringConn, AF_UNIX_T, UNIX_ringConnect);
        pafunixConn->UNIX_iStatus = __AF_UNIX_STATUS_NONE;
        pafunixConn->UNIX_pafunxPeer = LW_NULL;
        
        _List_Ring_Del(&pafunixConn->UNIX_ringConnect,
                       &pafunix->UNIX_pringConnect);                    /*  �ӵȴ����ӱ���ɾ��          */
        
        __unixUpdateConnecter(pafunixConn, ECONNREFUSED);               /*  ֪ͨ�������ӵ��߳�          */
    }
    pafunix->UNIX_iConnNum = 0;
}
/*********************************************************************************************************
** ��������: __unixMsToTicks
** ��������: ����ת��Ϊ ticks
** �䡡��  : ulMs        ����
** �䡡��  : tick
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ULONG  __unixMsToTicks (ULONG  ulMs)
{
    ULONG   ulTicks;
    
    if (ulMs == 0) {
        ulTicks = LW_OPTION_WAIT_INFINITE;
    } else {
        ulTicks = LW_MSECOND_TO_TICK_1(ulMs);
    }
    
    return  (ulTicks);
}
/*********************************************************************************************************
** ��������: __unixTvToTicks
** ��������: timeval ת��Ϊ ticks
** �䡡��  : ptv       ʱ��
** �䡡��  : tick
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ULONG  __unixTvToTicks (struct timeval  *ptv)
{
    ULONG   ulTicks;

    if ((ptv->tv_sec == 0) && (ptv->tv_usec == 0)) {
        return  (LW_OPTION_WAIT_INFINITE);
    }
    
    ulTicks = __timevalToTick(ptv);
    if (ulTicks == 0) {
        ulTicks = 1;
    }
    
    return  (ulTicks);
}
/*********************************************************************************************************
** ��������: __unixTicksToMs
** ��������: ticks ת��Ϊ����
** �䡡��  : ulTicks       tick
** �䡡��  : ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ULONG  __unixTicksToMs (ULONG  ulTicks)
{
    ULONG  ulMs;
    
    if (ulTicks == LW_OPTION_WAIT_INFINITE) {
        ulMs = 0;
    } else {
        ulMs = (ulTicks * 1000) / LW_TICK_HZ;
    }
    
    return  (ulMs);
}
/*********************************************************************************************************
** ��������: __unixTicksToTv
** ��������: ticks ת��Ϊ timeval
** �䡡��  : ulTicks       tick
** �䡡��  : timeval
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID __unixTicksToTv (ULONG  ulTicks, struct timeval *ptv)
{
    if (ulTicks == LW_OPTION_WAIT_INFINITE) {
        ptv->tv_sec  = 0;
        ptv->tv_usec = 0;
    } else {
        __tickToTimeval(ulTicks, ptv);
    }
}
/*********************************************************************************************************
** ��������: __unixSignalNotify
** ��������: ��Ҫ���ѵĹܵ���Ҫ���� SIGPIPE �ź�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID __unixSignalNotify (INT  iFlag)
{
#if LW_CFG_SIGNAL_EN > 0
    sigevent_t      sigeventPipe;
    
    if ((iFlag & MSG_NOSIGNAL) == 0) {                                  /*  û�� MSG_NOSIGNAL ��־      */
        sigeventPipe.sigev_signo           = SIGPIPE;
        sigeventPipe.sigev_value.sival_ptr = LW_NULL;
        sigeventPipe.sigev_notify          = SIGEV_SIGNAL;
    
        _doSigEvent(API_ThreadIdSelf(), &sigeventPipe, SI_MESGQ);       /*  ���� SIGPIPE �ź�           */
    }
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
}
/*********************************************************************************************************
** ��������: unix_init
** ��������: ��ʼ�� unix ��Э��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  unix_init (VOID)
{
    _G_hAfUnixMutex = API_SemaphoreMCreate("afunix_lock", LW_PRIO_DEF_CEILING, 
                                           LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                           LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                           LW_NULL);
                                           
    _G_pstkUnixPart256 = (LW_STACK *)__SHEAP_ALLOC(__AF_UNIX_PART_256 * 256);
    _BugHandle(!_G_pstkUnixPart256, LW_TRUE, "AF_UNIX buffer create error!\r\n");
    
    _G_pstkUnixPart512 = (LW_STACK *)__SHEAP_ALLOC(__AF_UNIX_PART_512 * 512);
    _BugHandle(!_G_pstkUnixPart512, LW_TRUE, "AF_UNIX buffer create error!\r\n");
    
    _G_hAfUnixPart256 = API_PartitionCreate("unix_256", _G_pstkUnixPart256, __AF_UNIX_PART_256,
                                            256, LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _G_hAfUnixPart512 = API_PartitionCreate("unix_512", _G_pstkUnixPart512, __AF_UNIX_PART_512,
                                            512, LW_OPTION_OBJECT_GLOBAL, LW_NULL);
}
/*********************************************************************************************************
** ��������: unix_traversal
** ��������: �������� af_unix ���ƿ�
** �䡡��  : pfunc                ��������
**           pvArg[0 ~ 5]         ������������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  unix_traversal (VOIDFUNCPTR    pfunc, 
                      PVOID          pvArg0,
                      PVOID          pvArg1,
                      PVOID          pvArg2,
                      PVOID          pvArg3,
                      PVOID          pvArg4,
                      PVOID          pvArg5)
{
    AF_UNIX_T       *pafunixTemp;
    PLW_LIST_LINE    plineTemp;
    
    __AF_UNIX_LOCK();
    for (plineTemp  = _G_plineAfUnix;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pafunixTemp = (AF_UNIX_T *)plineTemp;
        pfunc(pafunixTemp, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5);
    }
    __AF_UNIX_UNLOCK();
}
/*********************************************************************************************************
** ��������: unix_socket
** ��������: unix socket
** �䡡��  : iDomain        ��, ������ AF_UNIX
**           iType          SOCK_STREAM / SOCK_DGRAM / SOCK_SEQPACKET
**           iProtocol      Э��
** �䡡��  : unix socket
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
AF_UNIX_T  *unix_socket (INT  iDomain, INT  iType, INT  iProtocol)
{
    AF_UNIX_T   *pafunix;
    
    if (iDomain != AF_UNIX) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    if ((iType != SOCK_STREAM) && 
        (iType != SOCK_DGRAM)  &&
        (iType != SOCK_SEQPACKET)) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    pafunix = __unixCreate(iType);
    if (pafunix == LW_NULL) {
        _ErrorHandle(ENOMEM);
    }
    
    return  (pafunix);
}
/*********************************************************************************************************
** ��������: unix_bind
** ��������: bind
** �䡡��  : pafunix   unix file
**           name      address
**           namelen   address len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  unix_bind (AF_UNIX_T  *pafunix, const struct sockaddr *name, socklen_t namelen)
{
    struct sockaddr_un  *paddrun = (struct sockaddr_un *)name;
           INT           iFd;
           INT           iPathLen;
           INT           iSockType;
           AF_UNIX_T    *pafunixFind;
           CHAR          cPath[MAX_FILENAME_LENGTH];
    
    if (paddrun == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iPathLen = (namelen - __AF_UNIX_ADDROFFSET);
    if (iPathLen < 1) {                                                 /*  sockaddr ��û��·��         */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (iPathLen > PATH_MAX) {                                          /*  ·������                    */
        _ErrorHandle(ENAMETOOLONG);
        return  (PX_ERROR);
    }
    
    lib_strncpy(cPath, paddrun->sun_path, iPathLen);
    cPath[iPathLen] = PX_EOS;
    
    iFd = open(cPath, O_CREAT | O_RDWR, __AF_UNIX_DEF_FLAG | S_IFSOCK); /*  ���� socket �ļ�            */
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    __AF_UNIX_LOCK();
    API_IosFdGetName(iFd, cPath, MAX_FILENAME_LENGTH);                  /*  �������·��                */
    iSockType = __AF_UNIX_TYPE(pafunix);
    pafunixFind = __unixFind(cPath, iSockType, 
                             (iSockType == SOCK_DGRAM) ?
                             LW_FALSE : LW_TRUE);
    if (pafunixFind) {
        __AF_UNIX_UNLOCK();
        close(iFd);
        _ErrorHandle(EADDRINUSE);                                       /*  �������ظ���ַ��          */
        return  (PX_ERROR);
    }
    lib_strcpy(pafunix->UNIX_cFile, cPath);
    __AF_UNIX_UNLOCK();
    
    close(iFd);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: unix_listen
** ��������: listen
** �䡡��  : pafunix   unix file
**           backlog   back log num
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  unix_listen (AF_UNIX_T  *pafunix, INT  backlog)
{
    if (backlog < 0) {
        backlog = 1;
    }
    
    if (__AF_UNIX_TYPE(pafunix) == SOCK_DGRAM) {                        /*  ����Ϊ���������� socket     */
        _ErrorHandle(EOPNOTSUPP);
        return  (PX_ERROR);
    }
    
    if (pafunix->UNIX_iStatus != __AF_UNIX_STATUS_NONE) {               /*  ����� NONE ģʽ��ʼ listen */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __AF_UNIX_LOCK();
    if (pafunix->UNIX_cFile[0] == PX_EOS) {
        __AF_UNIX_UNLOCK();
        _ErrorHandle(EDESTADDRREQ);                                     /*  û�а󶨵�ַ                */
        return  (PX_ERROR);
    }
    
    pafunix->UNIX_iBacklog = backlog;
    pafunix->UNIX_iStatus  = __AF_UNIX_STATUS_LISTEN;
    __AF_UNIX_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: unix_accept
** ��������: accept
** �䡡��  : pafunix   unix file
**           addr      address
**           addrlen   address len
** �䡡��  : unix socket
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
AF_UNIX_T  *unix_accept (AF_UNIX_T  *pafunix, struct sockaddr *addr, socklen_t *addrlen)
{
    struct sockaddr_un  *paddrun = (struct sockaddr_un *)addr;
    AF_UNIX_T           *pafunixConn;
    AF_UNIX_T           *pafunixNew;

    if (__AF_UNIX_TYPE(pafunix) == SOCK_DGRAM) {                        /*  ����Ϊ���������� socket     */
        _ErrorHandle(EOPNOTSUPP);
        return  (LW_NULL);
    }
    
    if (pafunix->UNIX_iStatus != __AF_UNIX_STATUS_LISTEN) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    __AF_UNIX_LOCK();
    do {
        if (__unixCanAccept(pafunix)) {                                 /*  �Ƿ���� accept             */
            __AF_UNIX_UNLOCK();
            pafunixNew = __unixCreate(__AF_UNIX_TYPE(pafunix));         /*  �̳�ԭ��������              */
            if (pafunixNew == LW_NULL) {
                __AF_UNIX_LOCK();
                __unixRefuseAll(pafunix);                               /*  û���㹻�ڴ�, �ܾ�����      */
                __AF_UNIX_UNLOCK();
                _ErrorHandle(ENOMEM);
                return  (LW_NULL);
            }
            __AF_UNIX_LOCK();
            
            pafunixConn = __unixAccept(pafunix);                        /*  ���һ�����ڵȴ�������      */
            if (pafunixConn) {
                pafunixConn->UNIX_iStatus    = __AF_UNIX_STATUS_ESTAB;
                pafunixConn->UNIX_pafunxPeer = pafunixNew;
                pafunixNew->UNIX_iStatus     = __AF_UNIX_STATUS_ESTAB;
                pafunixNew->UNIX_pafunxPeer  = pafunixConn;
                pafunixNew->UNIX_iPassCred   = pafunix->UNIX_iPassCred; /*  �̳� SO_PASSCRED ѡ��       */
                lib_strcpy(pafunixNew->UNIX_cFile, pafunix->UNIX_cFile);/*  �̳� accept ��ַ            */
                
                if (paddrun && addrlen && (*addrlen > __AF_UNIX_ADDROFFSET)) {
                    size_t  stPathLen    = lib_strlen(pafunixConn->UNIX_cFile);
                    size_t  stBufPathLen = (*addrlen - __AF_UNIX_ADDROFFSET);
                    if (stPathLen > stBufPathLen) {
                        stPathLen = stBufPathLen;
                    }
                    lib_strncpy(paddrun->sun_path, pafunixConn->UNIX_cFile, 
                                stBufPathLen);                          /*  ��¼�������ӽڵ����Ϣ      */
                    paddrun->sun_family = AF_UNIX;
                    paddrun->sun_len = (uint8_t)(__AF_UNIX_ADDROFFSET + stPathLen);
                    *addrlen = paddrun->sun_len;                        /*  ���Ȳ����� \0               */
                }
                
                __unixUpdateConnecter(pafunixConn, ERROR_NONE);         /*  �����������ӵ��߳�          */
                
                if (__AF_UNIX_IS_NBIO(pafunixConn, 0)) {                /*  ������Ϊ��������ʽ          */
                    __AF_UNIX_CCONN(pafunixConn);                       /*  ����ź���, ��ʼ����������;*/
                }
                break;
            
            } else {                                                    /*  û��ȡ��Զ������            */
                __AF_UNIX_UNLOCK();
                __unixDelete(pafunixNew);
                __AF_UNIX_LOCK();
            }
        }
        
        if (__AF_UNIX_IS_NBIO(pafunix, 0)) {                            /*  ������ IO                   */
            __AF_UNIX_UNLOCK();
            _ErrorHandle(EWOULDBLOCK);                                  /*  ��Ҫ���¶�                  */
            return  (LW_NULL);
        }
        
        __AF_UNIX_UNLOCK();
        
        __AF_UNIX_WACCE(pafunix);                                       /*  �ȴ���������                */
        
        __AF_UNIX_LOCK();
    } while (1);
    __AF_UNIX_UNLOCK();

    return  (pafunixNew);
}
/*********************************************************************************************************
** ��������: unix_connect
** ��������: connect
** �䡡��  : pafunix   unix file
**           name      address
**           namelen   address len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  unix_connect (AF_UNIX_T  *pafunix, const struct sockaddr *name, socklen_t namelen)
{
    struct sockaddr_un  *paddrun = (struct sockaddr_un *)name;
    AF_UNIX_T           *pafunixAcce;
    CHAR                 cPath[MAX_FILENAME_LENGTH];

    if (paddrun == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (namelen <= __AF_UNIX_ADDROFFSET) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    _PathGetFull(cPath, MAX_FILENAME_LENGTH, paddrun->sun_path);        /*  �������·��                */
    
    __AF_UNIX_LOCK();
    if (pafunix->UNIX_iStatus == __AF_UNIX_STATUS_CONNECT) {
        if (__AF_UNIX_IS_NBIO(pafunix, 0)) {
            __AF_UNIX_UNLOCK();
            _ErrorHandle(EINPROGRESS);                                  /*  ����ִ�� connect ����       */
        } else {
            __AF_UNIX_UNLOCK();
            _ErrorHandle(EALREADY);
        }
        return  (PX_ERROR);
    }
    
    if (pafunix->UNIX_iStatus != __AF_UNIX_STATUS_NONE) {
        __AF_UNIX_UNLOCK();
        _ErrorHandle(EISCONN);                                          /*  �Ѿ�������                  */
        return  (PX_ERROR);
    }
    
    pafunixAcce = __unixFind(cPath, __AF_UNIX_TYPE(pafunix), LW_TRUE);  /*  ��ѯ����Ŀ��                */
    if (pafunixAcce == LW_NULL) {
        __AF_UNIX_UNLOCK();
        _ErrorHandle(ECONNRESET);                                       /*  û��Ŀ�� ECONNRESET         */
        return  (PX_ERROR);
    }
    
    if (__AF_UNIX_TYPE(pafunix) == SOCK_DGRAM) {                        /*  ��������������              */
        pafunix->UNIX_pafunxPeer = pafunixAcce;                         /*  ����Զ�̽ڵ�                */
        
    } else {                                                            /*  ������������                */
        if ((pafunixAcce->UNIX_iStatus != __AF_UNIX_STATUS_LISTEN) ||   /*  �Է�û���� listen ģʽ      */
            (pafunixAcce->UNIX_iConnNum > pafunixAcce->UNIX_iBacklog)) {/*  �Է��Ѿ������������        */
            __AF_UNIX_UNLOCK();
            _ErrorHandle(ECONNREFUSED);                                 /*  �޷�����Ŀ�� ECONNREFUSED   */
            return  (PX_ERROR);
        }
        
        __AF_UNIX_CCONN(pafunix);
         
        __unixConnect(pafunix, pafunixAcce);                            /*  ����Զ�̽ڵ�                */
        
        __unixUpdateAccept(pafunixAcce, ERROR_NONE);                    /*  ���� accept                 */
        
        if (__AF_UNIX_IS_NBIO(pafunix, 0)) {                            /*  ������ IO                   */
            __AF_UNIX_UNLOCK();
            _ErrorHandle(EINPROGRESS);                                  /*  ��Ҫ�û��ȴ����ӽ��        */
            return  (PX_ERROR);
        }
        __AF_UNIX_UNLOCK();
        
        __AF_UNIX_WCONN(pafunix);                                       /*  �ȴ� accept ȷ�ϴ���Ϣ      */
        
        __AF_UNIX_LOCK();
        if (pafunix->UNIX_iStatus == __AF_UNIX_STATUS_CONNECT) {
            __unixUnconnect(pafunix);                                   /*  �������                    */
            __AF_UNIX_UNLOCK();
            _ErrorHandle(ETIMEDOUT);                                    /*  connect ��ʱ                */
            return  (PX_ERROR);
        
        } else if (pafunix->UNIX_iStatus == __AF_UNIX_STATUS_NONE) {
            __AF_UNIX_UNLOCK();
            _ErrorHandle(ECONNREFUSED);                                 /*  connect ���ܾ�              */
            return  (PX_ERROR);
        }
    }
    __AF_UNIX_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: unix_connect2
** ��������: connect two unix socket
** �䡡��  : pafunix0   unix file
**           pafunix1   unix file
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  unix_connect2 (AF_UNIX_T  *pafunix0, AF_UNIX_T  *pafunix1)
{
    __AF_UNIX_LOCK();
    pafunix0->UNIX_pafunxPeer = pafunix1;
    pafunix1->UNIX_pafunxPeer = pafunix0;
    
    if ((__AF_UNIX_TYPE(pafunix0) == SOCK_STREAM) ||
        (__AF_UNIX_TYPE(pafunix0) == SOCK_SEQPACKET)) {
        pafunix0->UNIX_iStatus = __AF_UNIX_STATUS_ESTAB;
        pafunix1->UNIX_iStatus = __AF_UNIX_STATUS_ESTAB;
    }
    __AF_UNIX_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: unix_recvfrom2
** ��������: recvfrom (������չ��Ϣ���չ���)
** �䡡��  : pafunix   unix file
**           mem       buffer
**           len       buffer len
**           mem_ex    control msg
**           plen_ex   control msg len
**           pid       sender pid
**           flags     flag
**           from      packet from
**           fromlen   name len
**           msg_flags returned flags 
** �䡡��  : NUM
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����߳�ͬʱ��ͬһ�� socket ����ʹ�ò�ͬ�� flag ����ֻ����߼��Դ���, Ŀǰ��û��������Ƶ�
             Ӧ��.
*********************************************************************************************************/
static ssize_t  unix_recvfrom2 (AF_UNIX_T  *pafunix, 
                                void *mem, size_t len, 
                                void *mem_ex, socklen_t *plen_ex, int flags,
                                struct sockaddr *from, socklen_t *fromlen, int *msg_flags)
{
    ssize_t     sstTotal = 0;
    ULONG       ulError;
    BOOL        bNeedUpdateWriter;
    
    if (!mem || !len) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if ((flags & MSG_PEEK) && (flags & MSG_WAITALL)) {                  /*  Ŀǰ���ǲ������            */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __AF_UNIX_LOCK();
    do {
        if ((__AF_UNIX_TYPE(pafunix) == SOCK_STREAM) ||
            (__AF_UNIX_TYPE(pafunix) == SOCK_SEQPACKET)) {              /*  ������������                */
            if (pafunix->UNIX_iStatus != __AF_UNIX_STATUS_ESTAB) {
                __AF_UNIX_UNLOCK();
                _ErrorHandle(ENOTCONN);                                 /*  û������                    */
                return  (PX_ERROR);
            }
        }
        
        if (__unixCanRead(pafunix, flags, len)) {                       /*  ���Խ���                    */
            PCHAR   pcRecvMem = (PCHAR)mem;                             /*  ��ǰ����ָ��                */
            ssize_t sstReadNum;                                         /*  ���ν������ݳ���            */
            
            if (__unixCanWrite(pafunix)) {
                bNeedUpdateWriter = LW_FALSE;                           /*  �����Ϳ���д����Ҫ����      */
            } else {
                bNeedUpdateWriter = LW_TRUE;                            /*  ��Ҫ����д����              */
            }
            
__recv_more:
            sstReadNum = __unixRecvfromMsg(pafunix,
                                           (PVOID)pcRecvMem, 
                                           (size_t)(len - sstTotal), 
                                           mem_ex, plen_ex, flags, 
                                           (struct sockaddr_un *)from, 
                                           fromlen, msg_flags);         /*  �ӻ�����ȡһ����Ϣ          */
            pcRecvMem += sstReadNum;
            sstTotal  += sstReadNum;
            
            if (__AF_UNIX_TYPE(pafunix) == SOCK_STREAM) {               /*  �ַ�������                  */
                if (sstTotal < len) {                                   /*  ���пռ�                    */
                    if ((flags & MSG_WAITALL) ||                        /*  ��Ҫװ�����ջ�����          */
                        (pafunix->UNIX_unixq.UNIQ_pmonoHeader)) {       /*  �������ݿ��Խ���            */
                        goto    __recv_more;
                    }
                }
            }
            break;                                                      /*  ��������ѭ��                */
        
        } else {
#if LW_CFG_NET_UNIX_MULTI_EN > 0
            if (__unixCanRead(pafunix, 0, 0)) {
                __AF_UNIX_CREAD(pafunix);                               /*  �ɽ���, ��������ȴ��ź���  */
            }
#endif
            
            if ((__AF_UNIX_TYPE(pafunix) == SOCK_STREAM) ||
                (__AF_UNIX_TYPE(pafunix) == SOCK_SEQPACKET)) {          /*  ������������                */
                if (pafunix->UNIX_iShutDFlag & __AF_UNIX_SHUTD_R) {     /*  �ѱ��رն�                  */
                    __AF_UNIX_UNLOCK();
                    _ErrorHandle(ENOTCONN);                             /*  �����Ѿ��ر�                */
                    return  (sstTotal);
                }
                if (pafunix->UNIX_pafunxPeer == LW_NULL) {
                    __AF_UNIX_UNLOCK();
                    _ErrorHandle(ECONNRESET);                           /*  û������                    */
                    return  (PX_ERROR);
                }
            }
        }
        
        if (__AF_UNIX_IS_NBIO(pafunix, flags)) {                        /*  ������ IO                   */
            __AF_UNIX_UNLOCK();
            _ErrorHandle(EWOULDBLOCK);                                  /*  ��Ҫ���¶�                  */
            return  (sstTotal);
        }
        
        __AF_UNIX_UNLOCK();
        ulError = __AF_UNIX_WREAD(pafunix);                             /*  �ȴ�����                    */
        if (ulError) {
            _ErrorHandle(EWOULDBLOCK);                                  /*  �ȴ���ʱ                    */
            return  (sstTotal);
        }
        __AF_UNIX_LOCK();
    } while (1);
    
    if (bNeedUpdateWriter && (sstTotal > 0)) {
        __unixUpdateWriter(pafunix, ERROR_NONE);                        /*  update writer               */
    }

#if LW_CFG_NET_UNIX_MULTI_EN > 0
    if (__unixCanRead(pafunix, 0, 0)) {
        __AF_UNIX_SREAD(pafunix);                                       /*  other reader can read       */
    }
#endif
    __AF_UNIX_UNLOCK();
    
    return  (sstTotal);
}
/*********************************************************************************************************
** ��������: unix_recvmsg
** ��������: recvmsg
** �䡡��  : pafunix   unix file
**           msg       ��Ϣ
**           flags     flag
** �䡡��  : NUM ���ݳ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  unix_recvmsg (AF_UNIX_T  *pafunix, struct msghdr *msg, int flags)
{
    ssize_t     sstRecvLen;

    if (!msg) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    msg->msg_flags = 0;
    
    if (msg->msg_iovlen == 1) {
        sstRecvLen = unix_recvfrom2(pafunix, msg->msg_iov->iov_base, msg->msg_iov->iov_len, 
                                    msg->msg_control, &msg->msg_controllen, flags, 
                                    (struct sockaddr *)msg->msg_name, &msg->msg_namelen,
                                    &msg->msg_flags);
    
    } else {
        struct iovec    liovec, *msg_iov;
        int             msg_iovlen;
        ssize_t         sstRecvCnt;
        unsigned int    i, totalsize;
        char           *lbuf;
        char           *temp;
        
        msg_iov    = msg->msg_iov;
        msg_iovlen = msg->msg_iovlen;
        
        for (i = 0, totalsize = 0; i < msg_iovlen; i++) {
            if ((msg_iov[i].iov_len == 0) || (msg_iov[i].iov_base == LW_NULL)) {
                _ErrorHandle(EINVAL);
                return  (PX_ERROR);
            }
            totalsize += (unsigned int)msg_iov[i].iov_len;
        }
        
        /*
         * TODO: �ڴ˹����б� kill ���ڴ�й©����.
         */
        lbuf = (char *)__unixBufAlloc(totalsize);
        if (lbuf == LW_NULL) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        
        liovec.iov_base = (PVOID)lbuf;
        liovec.iov_len  = (size_t)totalsize;
        
        sstRecvLen = unix_recvfrom2(pafunix, liovec.iov_base, liovec.iov_len, 
                                    msg->msg_control, &msg->msg_controllen, flags, 
                                    (struct sockaddr *)msg->msg_name, &msg->msg_namelen,
                                    &msg->msg_flags);
        
        sstRecvCnt = sstRecvLen;
        temp       = lbuf;
        for (i = 0; sstRecvCnt > 0 && i < msg_iovlen; i++) {
            size_t   qty = (size_t)((sstRecvCnt > msg_iov[i].iov_len) ? msg_iov[i].iov_len : sstRecvCnt);
            lib_memcpy(msg_iov[i].iov_base, temp, qty);
            temp += qty;
            sstRecvCnt -= qty;
        }
        
        __unixBufFree(lbuf);
    }
    
    return  (sstRecvLen);
}
/*********************************************************************************************************
** ��������: unix_recvfrom
** ��������: recvfrom
** �䡡��  : pafunix   unix file
**           mem       buffer
**           len       buffer len
**           flags     flag
**           from      packet from
**           fromlen   name len
** �䡡��  : NUM
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  unix_recvfrom (AF_UNIX_T  *pafunix, void *mem, size_t len, int flags,
                        struct sockaddr *from, socklen_t *fromlen)
{
    return  (unix_recvfrom2(pafunix, mem, len, LW_NULL, LW_NULL, flags, from, fromlen, LW_NULL));
}
/*********************************************************************************************************
** ��������: unix_recv
** ��������: recv
** �䡡��  : pafunix   unix file
**           mem       buffer
**           len       buffer len
**           flags     flag
** �䡡��  : NUM
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  unix_recv (AF_UNIX_T  *pafunix, void *mem, size_t len, int flags)
{
    return  (unix_recvfrom2(pafunix, mem, len, LW_NULL, LW_NULL, flags, LW_NULL, LW_NULL, LW_NULL));
}
/*********************************************************************************************************
** ��������: unix_sendto2
** ��������: sendto (������չ��Ϣ���չ���)
** �䡡��  : pafunix   unix file
**           data      send buffer
**           size      send len
**           data_ex   control msg
**           size_ex   control msg len
**           flags     flag
**           to        packet to
**           tolen     name len
** �䡡��  : NUM
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  unix_sendto2 (AF_UNIX_T  *pafunix, const void *data, size_t size, 
                              const void *data_ex, socklen_t size_ex, int flags,
                              const struct sockaddr *to, socklen_t tolen)
{
    BOOL        bHaveTo  = LW_FALSE;
    size_t      stLeft   = size;
    ssize_t     sstTotal = 0;
    ssize_t     sstWriteNum;                                            /*  ���η��ͳ���                */
    ULONG       ulError;
    
    CHAR        cPath[MAX_FILENAME_LENGTH];
    AF_UNIX_T  *pafunixRecver;
    
    CPCHAR      pcSendMem         = (CPCHAR)data;                       /*  ��ǰ��������ָ��            */
    BOOL        bNeedUpdateReader = LW_FALSE;
    
    if (!data || !size) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__AF_UNIX_TYPE(pafunix) == SOCK_DGRAM) {                        /*  DGRAM                       */
        if (to && tolen && (tolen > __AF_UNIX_ADDROFFSET)) {            /*  �Ƿ��е�ַ��Ϣ              */
            _PathGetFull(cPath, MAX_FILENAME_LENGTH, ((struct sockaddr_un *)to)->sun_path);
            bHaveTo = LW_TRUE;
        }
    }
    
    __AF_UNIX_LOCK();
    do {
        if ((__AF_UNIX_TYPE(pafunix) == SOCK_STREAM) ||
            (__AF_UNIX_TYPE(pafunix) == SOCK_SEQPACKET)) {              /*  ������������                */
            if (pafunix->UNIX_iShutDFlag & __AF_UNIX_SHUTD_W) {
                __AF_UNIX_UNLOCK();
                __unixSignalNotify(flags);
                _ErrorHandle(ESHUTDOWN);                                /*  �����Ѿ��ر�                */
                return  (sstTotal);
            }
            if (pafunix->UNIX_iStatus != __AF_UNIX_STATUS_ESTAB) {
                __AF_UNIX_UNLOCK();
                __unixSignalNotify(flags);
                _ErrorHandle(ENOTCONN);                                 /*  û������                    */
                return  (PX_ERROR);
            }
        }
        
        if (bHaveTo) {                                                  /*  �Ƿ��е�ַ��Ϣ              */
            pafunixRecver = __unixFind(cPath, __AF_UNIX_TYPE(pafunix), LW_FALSE);
        
        } else {
            pafunixRecver = pafunix->UNIX_pafunxPeer;
        }
    
        if (pafunixRecver == LW_NULL) {
            if (__AF_UNIX_TYPE(pafunix) == SOCK_DGRAM) {
                __AF_UNIX_UNLOCK();
                _ErrorHandle(EHOSTUNREACH);                             /*  û��Ŀ��                    */
            } else {
                __AF_UNIX_UNLOCK();
                __unixSignalNotify(flags);
                _ErrorHandle(ECONNRESET);                               /*  �����Ѿ��ж�                */
            }
            return  (sstTotal);
        }
        
__try_send:
        if (__unixCanWrite(pafunixRecver)) {                            /*  ���Է���                    */
            sstWriteNum = __unixSendtoMsg(pafunix, pafunixRecver,
                                          pcSendMem, stLeft,
                                          data_ex, size_ex, flags);
            if (sstWriteNum <= 0) {
                break;                                                  /*  ʧ�����ڲ��Ѿ������� errno  */
            }

            bNeedUpdateReader = LW_TRUE;                                /*  ��Ҫ֪ͨ����                */

            pcSendMem += sstWriteNum;
            sstTotal  += sstWriteNum;
            stLeft    -= sstWriteNum;

            data_ex = LW_NULL;                                          /*  �������ֻ����һ��          */
            size_ex = 0;

            if (sstTotal >= size) {
                break;                                                  /*  �������                    */
            } else {
                goto    __try_send;
            }

        } 
#if LW_CFG_NET_UNIX_MULTI_EN > 0
          else {
            __AF_UNIX_CWRITE(pafunixRecver);                            /*  ����д, �������д�ź���    */
        }
#endif
        
        if (bNeedUpdateReader) {
            bNeedUpdateReader = LW_FALSE;
            __unixUpdateReader(pafunixRecver, ERROR_NONE);              /*  update remote reader        */
        }
        
        if (__AF_UNIX_IS_NBIO(pafunix, flags)) {                        /*  ������ IO                   */
            __AF_UNIX_UNLOCK();
            _ErrorHandle(EWOULDBLOCK);                                  /*  ��Ҫ���¶�                  */
            return  (sstTotal);
        }
    
        __AF_UNIX_UNLOCK();
        ulError = __AF_UNIX_WWRITE(pafunixRecver);                      /*  �ȴ���д                    */
        if (ulError) {
            if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
                _ErrorHandle(EWOULDBLOCK);                              /*  �ȴ���ʱ                    */
            } else {
                _ErrorHandle(ECONNABORTED);
            }
            return  (sstTotal);
        }
        __AF_UNIX_LOCK();
    } while (1);
    
    if (bNeedUpdateReader) {
        __unixUpdateReader(pafunixRecver, ERROR_NONE);                  /*  update remote reader        */
    }
    
#if LW_CFG_NET_UNIX_MULTI_EN > 0
    if (__unixCanWrite(pafunixRecver)) {
        __AF_UNIX_SWRITE(pafunixRecver);                                /*  other writer can write      */
    }
#endif
    __AF_UNIX_UNLOCK();
    
    return  (sstTotal);
}
/*********************************************************************************************************
** ��������: unix_sendmsg
** ��������: sendmsg
** �䡡��  : pafunix   unix file
**           msg       ��Ϣ
**           flags     flag
** �䡡��  : NUM ���ݳ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  unix_sendmsg (AF_UNIX_T  *pafunix, const struct msghdr *msg, int flags)
{
    ssize_t     sstSendLen;

    if (!msg) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (msg->msg_iovlen == 1) {
        sstSendLen = unix_sendto2(pafunix, msg->msg_iov->iov_base, msg->msg_iov->iov_len, 
                                  msg->msg_control, msg->msg_controllen, flags, 
                                  (const struct sockaddr *)msg->msg_name, msg->msg_namelen);
                                  
    } else {
        struct iovec    liovec,*msg_iov;
        int             msg_iovlen;
        unsigned int    i, totalsize;
        ssize_t         size;
        char           *lbuf;
        char           *temp;
        
        msg_iov    = msg->msg_iov;
        msg_iovlen = msg->msg_iovlen;
        
        for (i = 0, totalsize = 0; i < msg_iovlen; i++) {
            if ((msg_iov[i].iov_len == 0) || (msg_iov[i].iov_base == LW_NULL)) {
                _ErrorHandle(EINVAL);
                return  (PX_ERROR);
            }
            totalsize += (unsigned int)msg_iov[i].iov_len;
        }
        
        /*
         * TODO: �ڴ˹����б� kill ���ڴ�й©����.
         */
        lbuf = (char *)__unixBufAlloc(totalsize);
        if (lbuf == LW_NULL) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        
        liovec.iov_base = (PVOID)lbuf;
        liovec.iov_len  = (size_t)totalsize;
        
        size = totalsize;
        
        temp = lbuf;
        for (i = 0; size > 0 && i < msg_iovlen; i++) {
            int     qty = msg_iov[i].iov_len;
            lib_memcpy(temp, msg_iov[i].iov_base, qty);
            temp += qty;
            size -= qty;
        }
        
        sstSendLen = unix_sendto2(pafunix, liovec.iov_base, liovec.iov_len, 
                                  msg->msg_control, msg->msg_controllen, flags, 
                                  (const struct sockaddr *)msg->msg_name, msg->msg_namelen);
                           
        __unixBufFree(lbuf);
    }
    
    return  (sstSendLen);
}
/*********************************************************************************************************
** ��������: unix_sendto
** ��������: sendto
** �䡡��  : pafunix   unix file
**           data      send buffer
**           size      send len
**           flags     flag
**           to        packet to
**           tolen     name len
** �䡡��  : NUM
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  unix_sendto (AF_UNIX_T  *pafunix, const void *data, size_t size, int flags,
                      const struct sockaddr *to, socklen_t tolen)
{
    return  (unix_sendto2(pafunix, data, size, LW_NULL, 0, flags, to, tolen));
}
/*********************************************************************************************************
** ��������: unix_send
** ��������: send
** �䡡��  : pafunix   unix file
**           data      send buffer
**           size      send len
**           flags     flag
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  unix_send (AF_UNIX_T  *pafunix, const void *data, size_t size, int flags)
{
    return  (unix_sendto2(pafunix, data, size, LW_NULL, 0, flags, LW_NULL, 0));
}
/*********************************************************************************************************
** ��������: unix_close
** ��������: close 
** �䡡��  : pafunix   unix file
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  unix_close (AF_UNIX_T  *pafunix)
{
    AF_UNIX_T  *pafunixPeer;

    __AF_UNIX_LOCK();
    if ((__AF_UNIX_TYPE(pafunix) == SOCK_STREAM) ||
        (__AF_UNIX_TYPE(pafunix) == SOCK_SEQPACKET)) {                  /*  ������������                */
        if (pafunix->UNIX_iStatus == __AF_UNIX_STATUS_CONNECT) {        /*  ��������                    */
            __unixUnconnect(pafunix);                                   /*  �������                    */
        } else if (pafunix->UNIX_iStatus == __AF_UNIX_STATUS_LISTEN) {  /*  �ȴ����� unix ����          */
            __unixRefuseAll(pafunix);                                   /*  �ܾ�������������            */
        }
        pafunixPeer = pafunix->UNIX_pafunxPeer;
        if (pafunixPeer) {
            /*
             *  ����������ӶԵȽڵ�, ���ｫ����Է�, ������ӹ�ϵ, �����ı�Է���״̬,
             *  ����ɾ���Է�û�н�����ȫ��ʣ������, �������ʣ������, �Է����Լ�������, 
             *  ���Է�һ������, �ٴν��վͻ��յ� ECONNABORTED ����.
             */
            pafunixPeer->UNIX_pafunxPeer = LW_NULL;                     /*  ����Է������ӹ�ϵ          */
            __unixUpdateExcept(pafunixPeer, ECONNRESET);                /*  ������ܼ���Է��˳��ȴ�    */
        }
        pafunix->UNIX_iStatus    = __AF_UNIX_STATUS_NONE;
        pafunix->UNIX_pafunxPeer = LW_NULL;                             /*  ����������ӹ�ϵ            */
        __unixUpdateExcept(pafunix, ENOTCONN);
    }
    __AF_UNIX_UNLOCK();
    
    __unixDelete(pafunix);                                              /*  ɾ��ͬʱ��������ջ���      */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: unix_shutdown
** ��������: shutdown
** �䡡��  : pafunix   unix file
**           how       SHUT_RD  SHUT_WR  SHUT_RDWR
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  unix_shutdown (AF_UNIX_T  *pafunix, int how)
{
    if ((how != SHUT_RD) && (how != SHUT_WR) && (how != SHUT_RDWR)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __AF_UNIX_LOCK();
    if ((__AF_UNIX_TYPE(pafunix) == SOCK_STREAM) ||
        (__AF_UNIX_TYPE(pafunix) == SOCK_SEQPACKET)) {                  /*  ������������                */
        if ((pafunix->UNIX_iStatus == __AF_UNIX_STATUS_NONE)   ||
            (pafunix->UNIX_iStatus == __AF_UNIX_STATUS_LISTEN) ||
            (pafunix->UNIX_iStatus == __AF_UNIX_STATUS_CONNECT)) {      /*  ����������״̬��������      */
            __AF_UNIX_UNLOCK();
            return  (ERROR_NONE);
        }
        
        if (how == SHUT_RD) {                                           /*  �رձ��ض�                  */
            __unixShutdownR(pafunix);
            
        } else if (how == SHUT_WR) {                                    /*  �رձ���д                  */
            __unixShutdownW(pafunix);
            
        } else {                                                        /*  �رձ��ض�д                */
            __unixShutdownR(pafunix);
            __unixShutdownW(pafunix);
        }
    } else {
        __AF_UNIX_UNLOCK();
        _ErrorHandle(EOPNOTSUPP);
        return  (PX_ERROR);
    }
    __AF_UNIX_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: unix_getsockname
** ��������: getsockname
** �䡡��  : pafunix   unix file
**           name      address
**           namelen   address len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  unix_getsockname (AF_UNIX_T  *pafunix, struct sockaddr *name, socklen_t *namelen)
{
    struct sockaddr_un  *paddrun = (struct sockaddr_un *)name;
    size_t               stPathLen;
    size_t               stBufPathLen;

    if (paddrun && namelen && (*namelen > __AF_UNIX_ADDROFFSET)) {
        __AF_UNIX_LOCK();
        stPathLen    = lib_strlen(pafunix->UNIX_cFile);
        stBufPathLen = (*namelen - __AF_UNIX_ADDROFFSET);
        if (stPathLen > stBufPathLen) {
            stPathLen = stBufPathLen;
        }
        lib_strncpy(paddrun->sun_path, pafunix->UNIX_cFile, stBufPathLen);
        __AF_UNIX_UNLOCK();
        paddrun->sun_family = AF_UNIX;
        paddrun->sun_len = (uint8_t)(__AF_UNIX_ADDROFFSET + stPathLen); /*  ������ \0 ���ܳ���          */
        *namelen = paddrun->sun_len;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: unix_getpeername
** ��������: getpeername
** �䡡��  : pafunix   unix file
**           name      address
**           namelen   address len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  unix_getpeername (AF_UNIX_T  *pafunix, struct sockaddr *name, socklen_t *namelen)
{
    struct sockaddr_un  *paddrun = (struct sockaddr_un *)name;
    AF_UNIX_T           *pafunixPeer;
    size_t               stPathLen;
    size_t               stBufPathLen;
    
    if (paddrun && namelen && (*namelen > __AF_UNIX_ADDROFFSET)) {
        __AF_UNIX_LOCK();
        stBufPathLen = (*namelen - __AF_UNIX_ADDROFFSET);
        pafunixPeer = pafunix->UNIX_pafunxPeer;
        if (pafunixPeer) {
            stPathLen = lib_strlen(pafunixPeer->UNIX_cFile);
            if (stPathLen > stBufPathLen) {
                stPathLen = stBufPathLen;
            }
            lib_strncpy(paddrun->sun_path, pafunixPeer->UNIX_cFile, stBufPathLen);
        } else {
            stPathLen = 0;
            if (stBufPathLen > 0) {
                paddrun->sun_path[0] = PX_EOS;
            }
        }
        __AF_UNIX_UNLOCK();
        paddrun->sun_family = AF_UNIX;
        paddrun->sun_len = (uint8_t)(__AF_UNIX_ADDROFFSET + stPathLen); /*  ������ \0 ���ܳ���          */
        *namelen = paddrun->sun_len;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: unix_setsockopt
** ��������: setsockopt
** �䡡��  : pafunix   unix file
**           level     level
**           optname   option
**           optval    option value
**           optlen    option value len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  unix_setsockopt (AF_UNIX_T  *pafunix, int level, int optname, const void *optval, socklen_t optlen)
{
    INT     iRet = PX_ERROR;
    
    if (!optval || optlen < sizeof(INT)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __AF_UNIX_LOCK();
    switch (level) {
    
    case SOL_SOCKET:
        switch (optname) {
        
        case SO_REUSEADDR:
            pafunix->UNIX_iReuse = *(INT *)optval;
            iRet = ERROR_NONE;
            break;

        case SO_LINGER:
            if (optlen < sizeof(struct linger)) {
                _ErrorHandle(EINVAL);
            } else {
                pafunix->UNIX_linger = *(struct linger *)optval;
                iRet = ERROR_NONE;
            }
            break;
        
        case SO_RCVBUF:
            pafunix->UNIX_stMaxBufSize = *(INT *)optval;
            if (pafunix->UNIX_stMaxBufSize < __AF_UNIX_DEF_BUFMIN) {
                pafunix->UNIX_stMaxBufSize = __AF_UNIX_DEF_BUFMIN;
            } else if (pafunix->UNIX_stMaxBufSize > __AF_UNIX_DEF_BUFMAX) {
                pafunix->UNIX_stMaxBufSize = __AF_UNIX_DEF_BUFMAX;
            }
            iRet = ERROR_NONE;
            break;
            
        case SO_SNDTIMEO:
            if (optlen == sizeof(struct timeval)) {
                pafunix->UNIX_ulSendTimeout = __unixTvToTicks((struct timeval *)optval);
            } else {
                pafunix->UNIX_ulSendTimeout = __unixMsToTicks(*(INT *)optval);
            }
            iRet = ERROR_NONE;
            break;
            
        case SO_RCVTIMEO:
            if (optlen == sizeof(struct timeval)) {
                pafunix->UNIX_ulRecvTimeout = __unixTvToTicks((struct timeval *)optval);
            } else {
                pafunix->UNIX_ulRecvTimeout = __unixMsToTicks(*(INT *)optval);
            }
            iRet = ERROR_NONE;
            break;
        
        case SO_CONTIMEO:
            if (optlen == sizeof(struct timeval)) {
                pafunix->UNIX_ulConnTimeout = __unixTvToTicks((struct timeval *)optval);
            } else {
                pafunix->UNIX_ulConnTimeout = __unixMsToTicks(*(INT *)optval);
            }
            iRet = ERROR_NONE;
            break;
        
        case SO_DONTLINGER:
            if (*(INT *)optval) {
                pafunix->UNIX_linger.l_onoff = 0;
            } else {
                pafunix->UNIX_linger.l_onoff = 1;
            }
            iRet = ERROR_NONE;
            break;
            
        case SO_PASSCRED:
            pafunix->UNIX_iPassCred = *(INT *)optval;
            iRet = ERROR_NONE;
            break;
            
        default:
            _ErrorHandle(ENOSYS);
            break;
        }
        break;
        
    default:
        _ErrorHandle(ENOPROTOOPT);
        break;
    }
    __AF_UNIX_UNLOCK();
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: unix_getsockopt
** ��������: getsockopt
** �䡡��  : pafunix   unix file
**           level     level
**           optname   option
**           optval    option value
**           optlen    option value len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  unix_getsockopt (AF_UNIX_T  *pafunix, int level, int optname, void *optval, socklen_t *optlen)
{
    INT     iRet = PX_ERROR;
    
    if (!optval || *optlen < sizeof(INT)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __AF_UNIX_LOCK();
    switch (level) {
    
    case SOL_SOCKET:
        switch (optname) {
        
        case SO_ACCEPTCONN:
            if ((__AF_UNIX_TYPE(pafunix) == SOCK_STREAM) ||
                (__AF_UNIX_TYPE(pafunix) == SOCK_SEQPACKET)) {
                if (pafunix->UNIX_iStatus == __AF_UNIX_STATUS_LISTEN) {
                    *(INT *)optval = 1;
                } else {
                    *(INT *)optval = 0;
                }
                iRet = ERROR_NONE;
            } else {
                _ErrorHandle(ENOTSUP);
            }
            break;
        
        case SO_REUSEADDR:
            *(INT *)optval = pafunix->UNIX_iReuse;
            iRet = ERROR_NONE;
            break;

        case SO_LINGER:
            if (*optlen < sizeof(struct linger)) {
                _ErrorHandle(EINVAL);
            } else {
                *(struct linger *)optval = pafunix->UNIX_linger;
                iRet = ERROR_NONE;
            }
            break;
        
        case SO_RCVBUF:
            *(INT *)optval = pafunix->UNIX_stMaxBufSize;
            iRet = ERROR_NONE;
            break;
            
        case SO_SNDTIMEO:
            if (*optlen == sizeof(struct timeval)) {
                __unixTicksToTv(pafunix->UNIX_ulSendTimeout, (struct timeval *)optval);
            } else {
                *(INT *)optval = (INT)__unixTicksToMs(pafunix->UNIX_ulSendTimeout);
            }
            iRet = ERROR_NONE;
            break;
        
        case SO_RCVTIMEO:
            if (*optlen == sizeof(struct timeval)) {
                __unixTicksToTv(pafunix->UNIX_ulRecvTimeout, (struct timeval *)optval);
            } else {
                *(INT *)optval = (INT)__unixTicksToMs(pafunix->UNIX_ulRecvTimeout);
            }
            iRet = ERROR_NONE;
            break;
        
        case SO_TYPE:
            *(INT *)optval = (INT)__AF_UNIX_TYPE(pafunix);
            iRet = ERROR_NONE;
            break;
        
        case SO_CONTIMEO:
            if (*optlen == sizeof(struct timeval)) {
                __unixTicksToTv(pafunix->UNIX_ulConnTimeout, (struct timeval *)optval);
            } else {
                *(INT *)optval = (INT)__unixTicksToMs(pafunix->UNIX_ulConnTimeout);
            }
            iRet = ERROR_NONE;
            break;
            
        case SO_PASSCRED:
            *(INT *)optval = pafunix->UNIX_iPassCred;
            iRet = ERROR_NONE;
            break;
        
        default:
            _ErrorHandle(ENOSYS);
            break;
        }
        break;
        
    default:
        _ErrorHandle(ENOPROTOOPT);
        break;
    }
    __AF_UNIX_UNLOCK();
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: unix_ioctl
** ��������: ioctl
** �䡡��  : pafunix   unix file
**           iCmd      ����
**           pvArg     ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  unix_ioctl (AF_UNIX_T  *pafunix, INT  iCmd, PVOID  pvArg)
{
    INT     iRet = ERROR_NONE;
    
    switch (iCmd) {
    
    case FIOGETFL:
        if (pvArg) {
            *(INT *)pvArg = pafunix->UNIX_iFlag;
        }
        break;
        
    case FIOSETFL:
        if ((INT)(LONG)pvArg & O_NONBLOCK) {
            pafunix->UNIX_iFlag |= O_NONBLOCK;
        } else {
            pafunix->UNIX_iFlag &= ~O_NONBLOCK;
        }
        break;
        
    case FIONREAD:
        if (pvArg) {
            *(INT *)pvArg = (INT)pafunix->UNIX_unixq.UNIQ_stTotal;
        }
        break;
        
    case FIONBIO:
        if (pvArg && *(INT *)pvArg) {
            pafunix->UNIX_iFlag |= O_NONBLOCK;
        } else {
            pafunix->UNIX_iFlag &= ~O_NONBLOCK;
        }
        break;
        
    default:
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
        break;
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: __unix_have_event
** ��������: ����Ӧ�Ŀ��ƿ��Ƿ���ָ�����¼�
** �䡡��  : pafunix   unix file
**           type      �¼�����
**           piSoErr   ����ȴ����¼���Ч����� SO_ERROR
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
** ˵  ��  :

SELREAD:
    1>.�׽ӿ������ݿɶ�
    2>.�����ӵĶ���һ��ر� (Ҳ���ǽ�����FIN��TCP����). ���������׽ӿڽ��ж�������������������0
    3>.���׽ӿ���һ�������׽ӿ�������ɵ���������Ϊ0.
    4>.������һ���׽ӿڴ�����������������׽ӿڵĶ�������������������-1, ������errno.
       ����ͨ������ SO_ERROR ѡ����� getsockopt �������.
       
SELWRITE:
    1>.�׽ӿ��п�����д�Ŀռ�.
    2>.�����ӵ�д��һ��رգ����������׽ӿڽ���д����������SIGPIPE�ź�.
    3>.���׽ӿ�ʹ�÷������ķ�ʽconnect�������ӣ����������Ѿ��첽����������connect�Ѿ���ʧ�ܸ���.
    4>.������һ���׽ӿڴ��������.
    
SELEXCEPT
    1>.�������ӵ��׽ӿ�û������.
*********************************************************************************************************/
int __unix_have_event (AF_UNIX_T *pafunix, int type, int  *piSoErr)
{
    INT     iEvent = 0;

    switch (type) {

    case SELREAD:                                                       /*  �Ƿ�ɶ�                    */
        __AF_UNIX_LOCK();
        if ((__AF_UNIX_TYPE(pafunix) == SOCK_STREAM) ||
            (__AF_UNIX_TYPE(pafunix) == SOCK_SEQPACKET)) {
            switch (pafunix->UNIX_iStatus) {
            
            case __AF_UNIX_STATUS_NONE:
                *piSoErr = ENOTCONN;                                    /*  û������                    */
                iEvent   = 1;
                break;
                
            case __AF_UNIX_STATUS_LISTEN:
                if (__unixCanAccept(pafunix)) {                         /*  ���� accept                 */
                    *piSoErr = ERROR_NONE;
                    iEvent   = 1;
                }
                break;
                
            case __AF_UNIX_STATUS_CONNECT:
                *piSoErr = ENOTCONN;                                    /*  �������ӹ�����              */
                iEvent   = 1;
                break;
                
            case __AF_UNIX_STATUS_ESTAB:
                if (pafunix->UNIX_pafunxPeer == LW_NULL) {
                    *piSoErr = ECONNRESET;                              /*  �����Ѿ��ж�                */
                    iEvent   = 1;
                
                } else if (__unixCanRead(pafunix, 0, 0)) {              /*  �ɶ�                        */
                    *piSoErr = ERROR_NONE;
                    iEvent   = 1;
                
                } else if (pafunix->UNIX_iShutDFlag & __AF_UNIX_SHUTD_R) {
                    *piSoErr = ENOTCONN;                                /*  ���Ѿ���ֹͣ��              */
                    iEvent   = 1;
                }
                break;
                
            default:
                *piSoErr = ENOTCONN;                                    /*  �������е�����              */
                iEvent   = 1;
                break;
            }
        } else {
            if (__unixCanRead(pafunix, 0, 0)) {                         /*  �ɶ�                        */
                *piSoErr = ERROR_NONE;
                iEvent   = 1;
            }
        }
        __AF_UNIX_UNLOCK();
        break;

    case SELWRITE:                                                      /*  �Ƿ��д                    */
        __AF_UNIX_LOCK();                                               /*  ��ֹ peer ��ɾ��            */
        if ((__AF_UNIX_TYPE(pafunix) == SOCK_STREAM) ||
            (__AF_UNIX_TYPE(pafunix) == SOCK_SEQPACKET)) {
            switch (pafunix->UNIX_iStatus) {
            
            case __AF_UNIX_STATUS_NONE:
            case __AF_UNIX_STATUS_LISTEN:
                *piSoErr = ENOTCONN;                                    /*  û������                    */
                iEvent   = 1;
                break;
                
            case __AF_UNIX_STATUS_CONNECT:
                break;                                                  /*  ��Ҫ�ȴ�                    */
            
            case __AF_UNIX_STATUS_ESTAB:
                if (pafunix->UNIX_pafunxPeer == LW_NULL) {
                    *piSoErr = ECONNRESET;                              /*  �����Ѿ��ж�                */
                    iEvent   = 1;
                
                } else if (pafunix->UNIX_iShutDFlag & __AF_UNIX_SHUTD_W) {
                    *piSoErr = ESHUTDOWN;                               /*  д�Ѿ���ֹͣ��              */
                    iEvent   = 1;
                
                } else if (__unixCanWrite(pafunix->UNIX_pafunxPeer)) {  /*  ���Ŀ���Ƿ��д            */
                    *piSoErr = ERROR_NONE;
                    iEvent   = 1;
                }
                break;
                
            default:
                *piSoErr = ENOTCONN;                                    /*  �������е�����              */
                iEvent   = 1;
                break;
            }
        } else {
            if (pafunix->UNIX_pafunxPeer) {                             /*  �������Զ�̶�              */
                if (__unixCanWrite(pafunix->UNIX_pafunxPeer)) {         /*  ���Ŀ���Ƿ��д            */
                    *piSoErr = ERROR_NONE;
                    iEvent   = 1;
                }
            
            } else {
                *piSoErr = ERROR_NONE;
                iEvent   = 1;                                           /*  û��Ŀ��� DGRAM            */
            }
        }
        __AF_UNIX_UNLOCK();
        break;
        
    case SELEXCEPT:                                                     /*  �Ƿ��쳣                    */
        __AF_UNIX_LOCK();
        if ((__AF_UNIX_TYPE(pafunix) == SOCK_STREAM) ||
            (__AF_UNIX_TYPE(pafunix) == SOCK_SEQPACKET)) {
            if (pafunix->UNIX_iStatus == __AF_UNIX_STATUS_ESTAB) {      /*  estab ״̬��������          */
                if (pafunix->UNIX_pafunxPeer == LW_NULL) {
                    iEvent = 1;                                         /*  ������ SO_ERROR �̳�֮ǰ��ֵ*/
                }
            }
        }
        __AF_UNIX_UNLOCK();
        break;
    }
    
    return  (iEvent);
}
/*********************************************************************************************************
** ��������: __unix_set_sockfile
** ��������: ���ö�Ӧ�� socket �ļ�
** �䡡��  : pafunix   unix file
**           file      �ļ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  __unix_set_sockfile (AF_UNIX_T *pafunix, void *file)
{
    pafunix->UNIX_sockFile = file;
}

#endif                                                                  /*  LW_CFG_NET_EN               */
                                                                        /*  LW_CFG_NET_UNIX_EN > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
