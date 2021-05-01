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
** ��   ��   ��: lwip_ping.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 05 �� 06 ��
**
** ��        ��: lwip ping ����. (���Դ����Դ�� contrib)

** BUG:
2009.05.16  icmp У�����.
2009.05.20  ����� TTL ѡ���֧��.
2009.05.21  ����� timeout ��֧��.
2009.05.27  ����� ^C ��֧��.
2009.07.03  ���� GCC ����.
2009.09.03  ������ API_INetPing ���һ�εȴ�ʱ��.
2009.09.24  ʹ�� inet_ntoa_r() �滻 inet_ntoa().
2009.09.25  ���� gethostbyname() ���ص�ַ���Ƶķ���.
2009.11.11  ʹ�����µķ��ͷ���, ֮ǰ���� connect().
2010.05.29  __inetPingSend() ʹ�� sendto �������ݰ�. �Աܹ�һ�� lwip �� bug(#29979).
2011.03.11  ʹ�� getaddrinfo() ������������.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_PING_EN > 0)
#include "lwip/icmp.h"
#include "lwip/prot/icmp.h"
#include "lwip/raw.h"
#include "lwip/inet.h"
#include "lwip/inet_chksum.h"
#include "lwip/netdb.h"
#include "sys/socket.h"
/*********************************************************************************************************
** ��������: __inetPingPrepare
** ��������: ���� ping ��
** �䡡��  : icmphdrEcho   ����
**           iDataSize     ���ݴ�С
**           pusSeqRecv    ��Ҫ�жϵ� seq
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __inetPingPrepare (struct  icmp_echo_hdr   *icmphdrEcho, 
                                INT   iDataSize, UINT16  *pusSeqRecv)
{
    static u16_t    usSeqNum = 1;
    REGISTER INT    i;
    
    SYS_ARCH_DECL_PROTECT(x);

    ICMPH_TYPE_SET(icmphdrEcho, ICMP_ECHO);
    ICMPH_CODE_SET(icmphdrEcho, 0);
    
    *pusSeqRecv = usSeqNum;
    
    icmphdrEcho->chksum = 0;
    icmphdrEcho->id     = 0xAFAF;                                       /*  ID                          */
    icmphdrEcho->seqno  = htons(usSeqNum);
    
    /*
     *  �������
     */
    for(i = 0; i < iDataSize; i++) {
        ((PCHAR)icmphdrEcho)[sizeof(struct icmp_echo_hdr) + i] = (CHAR)(i % 256);
    }
    
#if CHECKSUM_GEN_ICMP
    icmphdrEcho->chksum = inet_chksum(icmphdrEcho, (u16_t)(iDataSize + sizeof(struct icmp_echo_hdr)));
#endif                                                                  /*  CHECKSUM_GEN_ICMP           */
    
    SYS_ARCH_PROTECT(x);
    usSeqNum++;
    SYS_ARCH_UNPROTECT(x);
}
/*********************************************************************************************************
** ��������: __inetPingSend
** ��������: ���� ping ��
** �䡡��  : iSock         �׽���
**           inaddr        Ŀ�� ip ��ַ.
**           iDataSize     ���ݴ�С
**           pusSeqRecv    ��Ҫ�жϵ� seq
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __inetPingSend (INT  iSock, struct in_addr  inaddr, INT  iDataSize, UINT16  *pusSeqRecv)
{
    REGISTER size_t                stPingSize = sizeof(struct icmp_echo_hdr) + iDataSize;
             struct icmp_echo_hdr *icmphdrEcho;
             ssize_t               sstError;
             struct sockaddr_in    sockaddrin;
             
    icmphdrEcho = (struct icmp_echo_hdr *)__SHEAP_ALLOC(stPingSize);
    if (icmphdrEcho == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory\r\n");
        return  (PX_ERROR);
    }
    
    __inetPingPrepare(icmphdrEcho, iDataSize, pusSeqRecv);
    
    sockaddrin.sin_len    = sizeof(struct sockaddr_in);
    sockaddrin.sin_family = AF_INET;
    sockaddrin.sin_port   = 0;
    sockaddrin.sin_addr   = inaddr;
    
    sstError = sendto(iSock, icmphdrEcho, stPingSize, 0, 
                      (const struct sockaddr *)&sockaddrin, 
                      sizeof(struct sockaddr_in));
                         
    __SHEAP_FREE(icmphdrEcho);
    
    return ((sstError > 0) ? ERR_OK : ERR_VAL);
}
/*********************************************************************************************************
** ��������: __inetPingRecv
** ��������: ���� ping ��
** �䡡��  : iSock         socket
**           usSeqRecv     ��Ҫ�жϵ� seq
**           piTTL         ���յ��� TTL
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __inetPingRecv (INT  iSock, UINT16  usSeqRecv, INT  *piTTL)
{
             CHAR                   cBuffer[512];
             
             INT                    iCnt = 20;                          /*  Ĭ�������յ����ݰ���      */
    REGISTER ssize_t                sstLen;
             INT                    iAddLen = sizeof(struct sockaddr_in);
             struct sockaddr_in     sockaddrinFrom;
             
             struct ip_hdr         *iphdrFrom;
             struct icmp_echo_hdr  *icmphdrFrom;
            
    while ((sstLen = recvfrom(iSock, cBuffer, sizeof(cBuffer), 0, 
                              (struct sockaddr *)&sockaddrinFrom, (socklen_t *)&iAddLen)) > 0) {
        
        if (sstLen >= (sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr))) {
            
            iphdrFrom   = (struct ip_hdr *)cBuffer;
            icmphdrFrom = (struct icmp_echo_hdr *)(cBuffer + (IPH_HL(iphdrFrom) * 4));
            if (ICMPH_TYPE(icmphdrFrom) == ICMP_ER) {
                if ((icmphdrFrom->id == 0xAFAF) && (icmphdrFrom->seqno == htons(usSeqRecv))) {
                    *piTTL = 0;
                    *piTTL = (u8_t)IPH_TTL(iphdrFrom);
                    return  (ERROR_NONE);
                } else {
                    iCnt--;
                }
            }

        } else {
            iCnt--;
        }
        
        if (iCnt < 0) {                                                 /*  ���յ���������ݰ�̫��      */
            break;                                                      /*  �˳�                        */
        }
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __inetPingCleanup
** ��������: ���� ping ��Դ.
** �䡡��  : iSock         �׽���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __inetPingCleanup (INT  iSock)
{
    close(iSock);
}
/*********************************************************************************************************
** ��������: API_INetPing
** ��������: internet ping
** �䡡��  : pinaddr       Ŀ�� ip ��ַ.
**           iTimes        ����
**           iDataSize     ���ݴ�С
**           iTimeout      ��ʱʱ��
**           iTTL          IP TTL
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_INetPing (struct in_addr  *pinaddr, INT  iTimes, INT  iDataSize, INT  iTimeout, INT  iTTL)
{
             CHAR       cInetAddr[INET_ADDRSTRLEN];                     /*  IP ��ַ����                 */

    REGISTER INT        iSock;
             INT        i;
             UINT16     usSeqRecv = 0;
             
             INT        iSuc = 0;
             INT        iTTLRecv;                                       /*  ���յ� TTL ֵ               */

    struct timespec     tvTime1;
    struct timespec     tvTime2;
    
    struct sockaddr_in  sockaddrinTo;
    
             UINT64     ullUSec;
             UINT64     ullUSecMin = 0xffffffffffffffffull;
             UINT64     ullUSecMax = 0ull;
             UINT64     ullUSecAvr = 0ull;
    
    if ((iDataSize >= (64 * LW_CFG_KB_SIZE)) || (iDataSize < 0)) {      /*  0 - 64KB                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if ((iTTL < 1) || (iTTL > 255)) {                                   /*  1 - 255                     */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    sockaddrinTo.sin_len    = sizeof(struct sockaddr_in);
    sockaddrinTo.sin_family = AF_INET;
    sockaddrinTo.sin_addr   = *pinaddr;
    
    iSock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);                    /*  ����ԭ�����е� push         */
    if (iSock < 0) {
        return  (PX_ERROR);
    }
    API_ThreadCleanupPush(__inetPingCleanup, (PVOID)(LONG)iSock);       /*  �����������                */
    
    setsockopt(iSock, SOL_SOCKET, SO_RCVTIMEO, &iTimeout, sizeof(INT));
    setsockopt(iSock, IPPROTO_IP, IP_TTL, &iTTL, sizeof(INT));
    
    connect(iSock, (struct sockaddr *)&sockaddrinTo, 
            sizeof(struct sockaddr_in));                                /*  �趨Ŀ��                    */
    
    printf("Pinging %s\n\n", inet_ntoa_r(*pinaddr, cInetAddr, sizeof(cInetAddr)));
    
    for (i = 0; ;) {
        if (__inetPingSend(iSock, *pinaddr, iDataSize, &usSeqRecv) < 0) { 
                                                                        /*  ���� icmp ���ݰ�            */
            fprintf(stderr, "error: %s.\n", lib_strerror(errno));
            
            i++;
            if (i >= iTimes) {
                break;
            }
            API_TimeSSleep(1);                                          /*  �ȴ� 1 S                    */
            continue;
        
        } else {
            i++;                                                        /*  ���ʹ��� ++                 */
        }
        
        lib_clock_gettime(CLOCK_MONOTONIC, &tvTime1);
        if (__inetPingRecv(iSock, usSeqRecv, &iTTLRecv) < 0) {          /*  ���� icmp ���ݰ�            */
            printf("Request time out.\n");                              /*  timeout                     */
            if (i >= iTimes) {
                break;                                                  /*  ping ����                   */
            }
        
        } else {
            lib_clock_gettime(CLOCK_MONOTONIC, &tvTime2);
            if (__timespecLeftTime(&tvTime1, &tvTime2)) {
                __timespecSub(&tvTime2, &tvTime1);
            } else {
                tvTime2.tv_sec  = 0;
                tvTime2.tv_nsec = 0;
            }
            
            ullUSec = (UINT64)(tvTime2.tv_sec * 1000000)
                    + (UINT64)(tvTime2.tv_nsec / 1000);
            
            printf("Reply from %s: bytes=%d time=%llu.%03llums TTL=%d\n", 
                   inet_ntoa_r(*pinaddr, cInetAddr, sizeof(cInetAddr)),
                   iDataSize, (ullUSec / 1000), (ullUSec % 1000), iTTLRecv);
        
            iSuc++;
            
            ullUSecAvr += ullUSec;
            ullUSecMax  = (ullUSecMax > ullUSec) ? ullUSecMax : ullUSec;
            ullUSecMin  = (ullUSecMin < ullUSec) ? ullUSecMin : ullUSec;
            
            if (i >= iTimes) {
                break;                                                  /*  ping ����                   */
            } else {
                API_TimeSSleep(1);                                      /*  �ȴ� 1 S                    */
            }
        }
    }
    API_ThreadCleanupPop(LW_TRUE);                                      /*  ping ���                   */
    
    /*
     *  ��ӡ�ܽ���Ϣ
     */
    printf("\nPing statistics for %s:\n", inet_ntoa_r(*pinaddr, cInetAddr, sizeof(cInetAddr)));
    printf("    Packets: Send = %d, Received = %d, Lost = %d(%d%% loss),\n", 
                  iTimes, iSuc, (iTimes - iSuc), (((iTimes - iSuc) * 100) / iTimes));
    
    if (iSuc == 0) {                                                    /*  û��һ�γɹ�                */
        return  (PX_ERROR);
    }
    
    ullUSecAvr /= iSuc;
    
    printf("Approximate round trip times in milli-seconds:\n");
    printf("    Minimum = %llu.%03llums, Maximum = %llu.%03llums, Average = %llu.%03llums\r\n\r\n",
           (ullUSecMin / 1000), (ullUSecMin % 1000), 
           (ullUSecMax / 1000), (ullUSecMax % 1000), 
           (ullUSecAvr / 1000), (ullUSecAvr % 1000));
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellPing
** ��������: ping ����
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static INT  __tshellPing (INT  iArgC, PCHAR  *ppcArgV)
{
    struct addrinfo      hints;
    struct addrinfo     *phints = LW_NULL;

    struct in_addr  inaddr;
           CHAR     cInetAddr[INET_ADDRSTRLEN];                         /*  IP ��ַ����                 */

           INT      iTimes    = 4;
           INT      iDataSize = 32;
           INT      iTimeout  = 3000;
           INT      iTTL      = 255;

    if (iArgC <= 1) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    /*
     *  ��������
     */
    if (iArgC >= 4) {
        REGISTER INT    i;
        
        for (i = 2; i < iArgC; i += 2) {
            if (lib_strcmp(ppcArgV[i], "-n") == 0) {
                sscanf(ppcArgV[i + 1], "%i", &iTimes);                  /*  ��ô���                    */
            
            } else if (lib_strcmp(ppcArgV[i], "-l") == 0) {
                sscanf(ppcArgV[i + 1], "%i", &iDataSize);               /*  ������ݴ�С                */
                if ((iDataSize > (65000 - sizeof(struct icmp_echo_hdr))) ||
                    (iDataSize < 1)) {
                    fprintf(stderr, "data size error!\n");
                    return  (-ERROR_TSHELL_EPARAM);
                }
                
            } else if (lib_strcmp(ppcArgV[i], "-i") == 0) {             /*  ��� TTL ��ֵ               */
                sscanf(ppcArgV[i + 1], "%i", &iTTL);
                if ((iTTL < 1) || (iTTL > 255)) {
                    fprintf(stderr, "TTL error!\n");
                    return  (-ERROR_TSHELL_EPARAM);
                }
            
            } else if (lib_strcmp(ppcArgV[i], "-w") == 0) {             /*  ��� timeout ��ֵ           */
                sscanf(ppcArgV[i + 1], "%i", &iTimeout);
                if ((iTimeout < 1) || (iTimeout > 60000)) {
                    fprintf(stderr, "timeout error!\n");
                    return  (-ERROR_TSHELL_EPARAM);
                }
                
            } else {
                fprintf(stderr, "arguments error!\n");                   /*  ��������                    */
                return  (-ERROR_TSHELL_EPARAM);
            }
        }
    }
    
    /*
     *  ������ַ
     */
    if (!inet_aton(ppcArgV[1], &inaddr)) {
        printf("Execute a DNS query...\n");
        
        {
            INT  iOptionNoAbort;
            INT  iOption;
            
            ioctl(STD_IN, FIOGETOPTIONS, &iOption);
            iOptionNoAbort = (iOption & ~OPT_ABORT);
            ioctl(STD_IN, FIOSETOPTIONS, iOptionNoAbort);               /*  ������ control-C ����       */
            
            lib_bzero(&hints, sizeof(struct addrinfo));
            hints.ai_family = AF_INET;                                  /*  ���� IPv4 ��ַ              */
            hints.ai_flags  = AI_CANONNAME;
            getaddrinfo(ppcArgV[1], LW_NULL, &hints, &phints);          /*  ��������                    */
        
            ioctl(STD_IN, FIOSETOPTIONS, iOption);                      /*  �ظ�ԭ��״̬                */
        }
        
        if (phints == LW_NULL) {
            printf("Pinging request could not find host %s ."
                   "Please check the name and try again.\n\n", ppcArgV[1]);
            return  (-ERROR_TSHELL_EPARAM);
        
        } else {
            if (phints->ai_addr->sa_family == AF_INET) {                /*  ��������ַ                */
                inaddr = ((struct sockaddr_in *)(phints->ai_addr))->sin_addr;
                freeaddrinfo(phints);
            
            } else {
                freeaddrinfo(phints);
                printf("Ping only support AF_INET domain!\n");
                return  (-EAFNOSUPPORT);
            }
            printf("Pinging %s [%s]\n\n", ppcArgV[1], inet_ntoa_r(inaddr, cInetAddr, sizeof(cInetAddr)));
        }
    }
    
    return  (API_INetPing(&inaddr, iTimes, iDataSize, iTimeout, iTTL));
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** ��������: API_INetPingInit
** ��������: ��ʼ�� ping ����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_INetPingInit (VOID)
{
#if LW_CFG_SHELL_EN > 0
    API_TShellKeywordAdd("ping", __tshellPing);
    API_TShellFormatAdd("ping", " ip/hostname [-l datalen] [-n times] [-i ttl] [-w timeout]");
    API_TShellHelpAdd("ping",   "ping tool\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_PING_EN > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
