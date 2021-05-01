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
** ��   ��   ��: ttinyShellModemCmd.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 07 �� 28 ��
**
** ��        ��: ϵͳ modem ��������.

** BUG:
2013.06.10  ���� -fsigned-char .
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
#include "../SylixOS/shell/ttinyShell/ttinyShell.h"
#include "../SylixOS/shell/ttinyShell/ttinyShellLib.h"
#include "../SylixOS/shell/ttinyVar/ttinyVarLib.h"
/*********************************************************************************************************
  XMODE ������
*********************************************************************************************************/
#define __LW_XMODEM_SOH             0x01
#define __LW_XMODEM_EOT             0x04
#define __LW_XMODEM_ACK             0x06
#define __LW_XMODEM_NAK             0x15
#define __LW_XMODEM_CAN             0x18
#define __LW_XMODEM_PAD             0x1A                                /*  ���ݲ�ȫ�����              */
/*********************************************************************************************************
  XMODE �����ֲ���
*********************************************************************************************************/
#define __LW_XMODEM_SEND_CMD(c)     {                               \
                                        CHAR    cCmd = c;           \
                                        write(STD_OUT, &cCmd, 1);   \
                                    }
/*********************************************************************************************************
  XMODE ����
*********************************************************************************************************/
#define __LW_XMODEM_DATA_LEN        128                                 /*  ���ݿ��С                  */
#define __LW_XMODEM_PACKET_LEN      (__LW_XMODEM_DATA_LEN + 4)          /*  ���ݰ���С                  */
#define __LW_XMODEM_TX_TIMEOUT      3                                   /*  ������Ϊ��ʱʱ��ĵ�λ      */
#define __LW_XMODEM_RX_TIMEOUT      3                                   /*  ������Ϊ��ʱʱ��ĵ�λ      */
/*********************************************************************************************************
** ��������: __tshellXmodemCleanup
** ��������: Xmodem �������� control-C ʱ���������
** �䡡��  : iFile     �����ļ�������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __tshellXmodemCleanup (INT  iFile)
{
    struct timeval  tmval = {2, 0};
           CHAR     cTemp[16];
    
    if (iFile >= 0) {
        close(iFile);
    }
    
    /*
     *  ����һЩ�ն���������ļ�����ʱ, ���������� __LW_XMODEM_CAN 0x18 ����, 
     *  ���Ի����ϵͳת��Ϊ OPT_TERMINAL ģʽʱ�� ctrl+x �����ͻ, ���ϵͳ����,
     *  ����������Ҫ�ȴ� 2s ��ƽ��ʱ��.
     */
    while (waitread(STD_IN, &tmval) == 1) {
        if (read(STD_IN, cTemp, sizeof(cTemp)) <= 0) {
            break;
        }
    }
    
    ioctl(STD_IN, FIOSETOPTIONS, OPT_TERMINAL);
    ioctl(STD_OUT, FIOSETOPTIONS, OPT_TERMINAL);
    
    ioctl(STD_IN, FIORFLUSH);                                           /*  �������������              */
}
/*********************************************************************************************************
** ��������: __tshellFsCmdXmodems
** ��������: ϵͳ���� "xmodems" (���͸� remote һ���ļ�)
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellFsCmdXmodems (INT  iArgC, PCHAR  ppcArgV[])
{
    INT             i;
    INT             j;
    
    BOOL            bIsEot = LW_FALSE;
    BOOL            bStart = LW_FALSE;
    
    INT             iFile;
    INT             iRetVal;
    UCHAR           ucRead;
    UCHAR           ucTemp[__LW_XMODEM_PACKET_LEN] = {__LW_XMODEM_SOH};
    UCHAR           ucSeq = 1;
    UCHAR           ucChkSum;
    
    ssize_t         sstRecvNum;
    ssize_t         sstReadNum;
    
    fd_set          fdsetRead;
    struct timeval  timevalTO;
    
    
    if (iArgC != 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    iFile = open(ppcArgV[1], O_RDONLY);
    if (iFile < 0) {
        fprintf(stderr, "can not open source file!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    API_ThreadCleanupPush(__tshellXmodemCleanup, (PVOID)(LONG)iFile);   /*  �����������                */
    
    ioctl(STD_IN, FIOSETOPTIONS, OPT_RAW);                              /*  ����׼�ļ���Ϊ raw ģʽ     */
    ioctl(STD_OUT, FIOSETOPTIONS, OPT_RAW);                             /*  ����׼�ļ���Ϊ raw ģʽ     */
    
    /*
     *  ������һ�����ݰ�
     */
    sstReadNum = read(iFile, &ucTemp[3], __LW_XMODEM_DATA_LEN);
    if (sstReadNum <= 0) {
        __LW_XMODEM_SEND_CMD(__LW_XMODEM_EOT);                          /*  �������                    */
        API_ThreadCleanupPop(LW_TRUE);
        printf("0 bytes read from source file!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    for (j = (INT)sstReadNum; j < __LW_XMODEM_DATA_LEN; j++) {          /*  ��ȫ����                    */
        ucTemp[j + 3] = __LW_XMODEM_PAD;
    }
    
    ucTemp[1] = (UCHAR)(ucSeq);                                         /*  ���к�                      */
    ucTemp[2] = (UCHAR)(~ucSeq);
    
    ucChkSum = 0;
    for (j = 3; j < (__LW_XMODEM_DATA_LEN + 3); j++) {
        ucChkSum = (UCHAR)(ucChkSum + ucTemp[j]);                       /*  ����У���                  */
    }
    ucTemp[__LW_XMODEM_DATA_LEN + 3] = ucChkSum;
    
    /*
     *  ��ʼѭ�������ļ�
     */
    FD_ZERO(&fdsetRead);
    /*
     *  ��ʱ����Ĭ��Ϊ 20 ��
     */
    for (i = 0; i < 20; i++) {
        FD_SET(STD_IN, &fdsetRead);
        timevalTO.tv_sec  = __LW_XMODEM_TX_TIMEOUT;
        timevalTO.tv_usec = 0;
        iRetVal = select(1, &fdsetRead, LW_NULL, LW_NULL, &timevalTO);  /*  �ȴ��ɶ�                    */
        if (iRetVal != 1) {
            if (bIsEot) {
                if (bStart) {
                    write(STD_OUT, ucTemp, __LW_XMODEM_PACKET_LEN);     /*  �ط�����                    */
                }
            }
            continue;                                                   /*  ���ճ�ʱ                    */
        }
        i = 0;                                                          /*  ���ó�ʱ����                */
        
        sstRecvNum = read(STD_IN, &ucRead, 1);
        if (sstRecvNum <= 0) {
            API_ThreadCleanupPop(LW_TRUE);
            fprintf(stderr, "standard in device error!\n");
            return  (PX_ERROR);
        }
        
        if (ucRead == __LW_XMODEM_CAN) {                                /*  ���ս���                    */
            break;
        
        } else if (ucRead == __LW_XMODEM_NAK) {                         /*  ��Ҫ�ط�                    */
            write(STD_OUT, ucTemp, __LW_XMODEM_PACKET_LEN);
            bStart = LW_TRUE;
        
        } else if (ucRead == __LW_XMODEM_ACK) {
            ucSeq++;
            if (ucSeq > 255) {
                ucSeq = 1;
            }
            bStart = LW_TRUE;
            
            if (bIsEot) {
                break;                                                  /*  ���ͽ���                    */
            
            } else {
                sstReadNum = read(iFile, &ucTemp[3], __LW_XMODEM_DATA_LEN);
                if (sstReadNum <= 0) {
                    bIsEot = LW_TRUE;
                    __LW_XMODEM_SEND_CMD(__LW_XMODEM_EOT);              /*  ���ͽ���                    */
                
                } else {
                    /*
                     *  ��ȫ����
                     */
                    for (j = (INT)sstReadNum; j < __LW_XMODEM_DATA_LEN; j++) {
                        ucTemp[j + 3] = __LW_XMODEM_PAD;
                    }
                    
                    ucTemp[1] = (UCHAR)(ucSeq);                         /*  ���к�                      */
                    ucTemp[2] = (UCHAR)(~ucSeq);
                    
                    ucChkSum = 0;
                    for (j = 3; j < (__LW_XMODEM_DATA_LEN + 3); j++) {
                        ucChkSum = (UCHAR)(ucChkSum + ucTemp[j]);       /*  ����У���                  */
                    }
                    ucTemp[__LW_XMODEM_DATA_LEN + 3] = ucChkSum;
                    
                    write(STD_OUT, ucTemp, __LW_XMODEM_PACKET_LEN);     /*  ��������                    */
                }
            }
        }
    }

    API_ThreadCleanupPop(LW_TRUE);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellFsCmdXmodemr
** ��������: ϵͳ���� "xmodemr" (�� remote ����һ���ļ�)
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellFsCmdXmodemr (INT  iArgC, PCHAR  ppcArgV[])
{
    INT             i;
    INT             j;
    
    INT             iFile;
    INT             iRetVal;
    UCHAR           ucTemp[__LW_XMODEM_PACKET_LEN];
    UCHAR           ucSeq = 1;
    UCHAR           ucChkSum;
    
    ssize_t         sstRecvNum;
    ssize_t         sstWriteNum;
    size_t          stTotalNum = 0;
    
    fd_set          fdsetRead;
    struct timeval  timevalTO;
    
    
    if (iArgC != 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    /*
     *  ����ļ��Ƿ����
     */
    iFile = open(ppcArgV[1], O_RDONLY);
    if (iFile >= 0) {
        close(iFile);                                                   /*  �ر��ļ�                    */
        
__re_select:
        printf("destination file is exist, overwrite? (Y/N)\n");
        read(0, ucTemp, __LW_XMODEM_DATA_LEN);
        if ((ucTemp[0] == 'N') ||
            (ucTemp[0] == 'n')) {                                       /*  ������                      */
            return  (ERROR_NONE);
        
        } else if ((ucTemp[0] != 'Y') &&
                   (ucTemp[0] != 'y')) {                                /*  ѡ�����                    */
            goto    __re_select;
        }
    }
    
    iFile = open(ppcArgV[1], (O_WRONLY | O_CREAT | O_TRUNC), 0666);     /*  �����ļ�                    */
    if (iFile < 0) {
        fprintf(stderr, "can not open destination file!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    API_ThreadCleanupPush(__tshellXmodemCleanup, (PVOID)(LONG)iFile);   /*  �����������                */
    
    ioctl(STD_IN, FIOSETOPTIONS, OPT_RAW);                              /*  ����׼�ļ���Ϊ raw ģʽ     */
    ioctl(STD_OUT, FIOSETOPTIONS, OPT_RAW);                             /*  ����׼�ļ���Ϊ raw ģʽ     */
    
    __LW_XMODEM_SEND_CMD(__LW_XMODEM_NAK);                              /*  ��������                    */
    
    FD_ZERO(&fdsetRead);
    /*
     *  ��ʱ����Ĭ��Ϊ 20 ��
     */
    for (i = 0; i < 20; i++) {
        FD_SET(STD_IN, &fdsetRead);
        timevalTO.tv_sec  = __LW_XMODEM_RX_TIMEOUT;
        timevalTO.tv_usec = 0;
        iRetVal = select(1, &fdsetRead, LW_NULL, LW_NULL, &timevalTO);  /*  �ȴ��ɶ�                    */
        if (iRetVal != 1) {
            stTotalNum = 0;                                             /*  ����ѽ��յ�����            */
            __LW_XMODEM_SEND_CMD(__LW_XMODEM_NAK);                      /*  ��������������ݰ�          */
            continue;                                                   /*  ���ճ�ʱ                    */
        }
        i = 0;                                                          /*  ���ó�ʱ����                */
    
        sstRecvNum = read(STD_IN, &ucTemp[stTotalNum], __LW_XMODEM_PACKET_LEN - stTotalNum);
        if (sstRecvNum <= 0) {
            API_ThreadCleanupPop(LW_TRUE);
            fprintf(stderr, "standard in device error!\n");
            return  (PX_ERROR);
        }
        if (ucTemp[0] == __LW_XMODEM_EOT) {                             /*  ���ս���                    */
            __LW_XMODEM_SEND_CMD(__LW_XMODEM_ACK);
            break;
        }
        stTotalNum += (size_t)sstRecvNum;
        if (stTotalNum < __LW_XMODEM_PACKET_LEN) {                      /*  ���ݰ�������                */
            continue;
        } else {
            stTotalNum = 0;                                             /*  �Ѿ��ӵ�һ�����������ݰ�    */
        }
        
        /*
         *  ��ʼ�ж����ݰ���ȷ��
         */
        if (ucTemp[1] != ucSeq) {                                       /*  ���к��Ƿ����              */
            if (ucTemp[1] == (ucSeq - 1)) {
                __LW_XMODEM_SEND_CMD(__LW_XMODEM_ACK);                  /*  ��Ҫ��һ������              */
            } else {
                __LW_XMODEM_SEND_CMD(__LW_XMODEM_CAN);                  /*  ����ͨ��                    */
                API_ThreadCleanupPop(LW_TRUE);
                fprintf(stderr, "sequence number error!\n");
                return  (PX_ERROR);
            }
            continue;
        }
        
        if (~ucTemp[1] == ucTemp[2]) {                                  /*  ���к�У�����              */
            __LW_XMODEM_SEND_CMD(__LW_XMODEM_NAK);
            continue;
        }
        
        ucChkSum = 0;
        for (j = 3; j < (__LW_XMODEM_DATA_LEN + 3); j++) {
            ucChkSum = (UCHAR)(ucChkSum + ucTemp[j]);                   /*  ����У���                  */
        }
        
        if (ucTemp[__LW_XMODEM_DATA_LEN + 3] != ucChkSum) {             /*  У�����                    */
            __LW_XMODEM_SEND_CMD(__LW_XMODEM_NAK);
            continue;
        }
        
        /*
         *  ������д��Ŀ���ļ�
         */
        sstWriteNum = write(iFile, &ucTemp[3], __LW_XMODEM_DATA_LEN);
        if (sstWriteNum != __LW_XMODEM_DATA_LEN) {
            INT     iErrNo = errno;
            __LW_XMODEM_SEND_CMD(__LW_XMODEM_CAN);                      /*  ����ͨ��                    */
            API_ThreadCleanupPop(LW_TRUE);
            fprintf(stderr, "write file error %s!\n", lib_strerror(iErrNo));
            return  (PX_ERROR);
        }
        
        ucSeq++;
        __LW_XMODEM_SEND_CMD(__LW_XMODEM_ACK);                          /*  ��Ҫ��һ������              */
    }
    
    API_ThreadCleanupPop(LW_TRUE);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellModemCmdInit
** ��������: ��ʼ�� modem ������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __tshellModemCmdInit (VOID)
{
    API_TShellKeywordAdd("xmodems", __tshellFsCmdXmodems);
    API_TShellFormatAdd("xmodems", " file path");
    API_TShellHelpAdd("xmodems", "send a file use xmodem protocol.\n");
    
    API_TShellKeywordAdd("xmodemr", __tshellFsCmdXmodemr);
    API_TShellFormatAdd("xmodemr", " file path");
    API_TShellHelpAdd("xmodemr", "receive a file use xmodem protocol.\n");
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
