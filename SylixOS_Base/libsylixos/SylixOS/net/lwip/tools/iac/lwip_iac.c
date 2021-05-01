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
** ��   ��   ��: lwip_iac.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 07 �� 13 ��
**
** ��        ��: lwip iac �����.

** BUG:
2013.06.09  ʹ�� -fsigned-char ʱ, ���ִ���, ����Ƚ�ʱ��Ҫʹ���޷��űȽ�
2014.10.22  ���� __inetIacFilter() �жϴ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_TELNET_EN > 0)
#include "lwip_iac.h"
/*********************************************************************************************************
** ��������: __inetIacMakeFrame
** ��������: ����һ�� IAC ���ݰ� (��ǰ���ܽ�Ϊ��, δ�����ܻ���ǿ)
** �䡡��  : iCommand      ����
**           iOpt          ѡ���ʶ, PX_ERROR ��ʾû��ѡ���ʶ
**           pcBuffer      ������ (���� 3 ���ֽ�)
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __inetIacMakeFrame (INT  iCommand, INT  iOpt, PCHAR  pcBuffer)
{
    if (pcBuffer) {
        pcBuffer[0] = (CHAR)LW_IAC_IAC;
        pcBuffer[1] = (CHAR)iCommand;
        if (iOpt != PX_ERROR) {                                         /*   PX_ERROR ��ʾû��ѡ���ʶ  */
            pcBuffer[2] = (CHAR)iOpt;
            return  (3);
        } else {
            return  (2);
        }
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __inetIacSend
** ��������: ����һ�� IAC ���ݰ�
** �䡡��  : iFd           �ļ�
**           iCommand      ����
**           iOpt          ѡ���ʶ
** �䡡��  : ERROR or send len
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __inetIacSend (INT  iFd, INT  iCommand, INT  iOpt)
{
    CHAR    cSendBuffer[3];
    INT     iLen;
    
    iLen = __inetIacMakeFrame(iCommand, iOpt, cSendBuffer);
    if (iLen < 0) {
        return  (PX_ERROR);
    }
    
    return  ((INT)write(iFd, cSendBuffer, (size_t)iLen));
}
/*********************************************************************************************************
** ��������: __inetIacFilter
** ��������: IAC ���ݰ������˲���
** �䡡��  : pcBuffer          ���ջ���
**           iLen              ���峤��
**           piPtyLen          pty ��Ҫ���յ����ݳ���
**           piProcessLen      ���ִ�����ֽ��� (���ڱ��ν��յ� IAC �ֶβ�����, 
                                                 �п��ܻ�ʣ�༸���ֽڲ�����)
**           pfunc             IAC �ص�����
**           pvArg             IAC �ص�����
** �䡡��  : pty data buffer
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PCHAR  __inetIacFilter (PCHAR        pcBuffer, 
                        INT          iLen, 
                        INT         *piPtyLen, 
                        INT         *piProcessLen, 
                        FUNCPTR      pfunc,
                        PVOID        pvArg)
{
    REGISTER PUCHAR     pucTemp = (PUCHAR)pcBuffer;
    REGISTER PUCHAR     pucEnd  = (PUCHAR)(pcBuffer + iLen);
    REGISTER PUCHAR     pucOut  = (PUCHAR)pcBuffer;                     /*  ��Ҫ���ظ� pty ������       */
    
    for (; pucTemp < pucEnd; pucTemp++) {
        if (*pucTemp != LW_IAC_IAC) {                                   /*  �� IAC ת��                 */
            *pucOut++ = *pucTemp;
        } else {
            if ((pucTemp + 2) <= pucEnd) {
                if (pfunc) {
                    pucTemp += pfunc(pvArg, pucTemp, pucEnd);           /*  �ص�                        */
                    pucTemp--;                                          /*  ����forѭ�������++         */
                } else {
                    pucTemp += 1;                                       /*  ���� IAC �ֶ�               */
                }
            } else {
                break;                                                  /*  ���� IAC ������           */
            }
        }
    }
    
    if (piPtyLen) {
        *piPtyLen = (INT)(pucOut - (PUCHAR)pcBuffer);                   /*  pty ��Ҫ����ĳ���          */
    }
    if (piProcessLen) {
        *piProcessLen = (INT)(pucTemp - (PUCHAR)pcBuffer);              /*  ���ִ�����                */
    }
    
    return  (pcBuffer);                                                 /*  �������׵�ַ                */
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_TELNET_EN > 0    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
