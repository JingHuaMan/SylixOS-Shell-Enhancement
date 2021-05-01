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
** ��   ��   ��: getopt_var.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 08 �� 16 ��
**
** ��        ��: ���� POSIX �� getopt ���� argc argv ������׼��.(ȫ�ֱ�������)

** BUG:
2009.12.15  �޸�ע��.
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
*********************************************************************************************************/
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
#include "getopt_var.h"
#include "getopt.h"
/*********************************************************************************************************
  ע��: ���� posix ���ݲ���ϵͳ�ṩ�� getopt() �� getopt_long() �Ƕ��̲߳������뺯��, ��Ϊ����������ֻ��
        Ϊ�˷������̵���ڲ�������Ƶ�. ���ڶ��̻߳�������Ҫ��������������, ���������ȫ�ֱ����ı���ʹ����
        ���� errno �ķ���.
*********************************************************************************************************/
/*********************************************************************************************************
  ARGOPT ������
*********************************************************************************************************/
typedef struct {
    INT     SAO_iOptErr;                                                /*  ������Ϣ                    */
    INT     SAO_iOptInd;                                                /*  argv�ĵ�ǰ����ֵ            */
    INT     SAO_iOptOpt;                                                /*  ��Чѡ���ַ�                */
    INT     SAO_iOptReset;                                              /*  ��λѡ��                    */
    PCHAR   SAO_pcOptArg;                                               /*  ��ǰѡ������ִ�            */
    /*
     *  �����ڲ�ʹ��
     */
    CHAR    SAO_cEMsg[1];
    PCHAR   SAO_pcEMsg;
    PCHAR   SAO_pcPlace;
    INT     SAO_iNonoptStart;
    INT     SAO_iNonoptEnd;
} __TSHELL_ARGOPT;
typedef __TSHELL_ARGOPT     *__PTSHELL_ARGOPT;
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static const __TSHELL_ARGOPT _G_saoStd = {1, 1, '?', 0, LW_NULL, {0}, LW_NULL, LW_NULL, -1, -1}; 
                                                                        /*  ��׼��ʼ��ģ��              */
static       __TSHELL_ARGOPT _G_saoTmp = {1, 1, '?', 0, LW_NULL, {0}, LW_NULL, LW_NULL, -1, -1}; 
                                                                        /*  �޷������ڴ�ʹ�ô˱���      */
/*********************************************************************************************************
  �����
*********************************************************************************************************/
#define __TTINY_SHELL_GET_ARTOPT(ptcb)              (__PTSHELL_ARGOPT)(ptcb->TCB_shc.SHC_ulGetOptCtx)
#define __TTINY_SHELL_SET_ARTOPT(ptcb, pvAddr)      ptcb->TCB_shc.SHC_ulGetOptCtx = (addr_t)pvAddr
/*********************************************************************************************************
** ��������: __tshellOptNonoptEnd
** ��������: ��� ARGOPT �������е� SAO_INonoptEnd ��ַ
** �䡡��  : NONE
** �䡡��  : SAO_INonoptEnd ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  *__tshellOptNonoptEnd (VOID)
{
    REGISTER INT                *piNonoptEnd;
    REGISTER __PTSHELL_ARGOPT    psaoGet;
             PLW_CLASS_TCB       ptcbCur;

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    __TTINY_SHELL_LOCK();                                               /*  shell ����                  */
    if (LW_NULL == __TTINY_SHELL_GET_ARTOPT(ptcbCur)) {
        psaoGet =  (__PTSHELL_ARGOPT)__SHEAP_ALLOC(sizeof(__TSHELL_ARGOPT));
        if (!psaoGet) {
            _G_saoTmp.SAO_pcPlace = _G_saoTmp.SAO_cEMsg;
            _G_saoTmp.SAO_pcEMsg  = _G_saoTmp.SAO_cEMsg;
            piNonoptEnd = &_G_saoTmp.SAO_iNonoptEnd;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "\"int nonopt_end\" system low memory.\r\n");
        } else {
            __TTINY_SHELL_SET_ARTOPT(ptcbCur, psaoGet);
            *psaoGet = _G_saoStd;
            psaoGet->SAO_pcPlace = psaoGet->SAO_cEMsg;
            psaoGet->SAO_pcEMsg  = psaoGet->SAO_cEMsg;
            piNonoptEnd = &psaoGet->SAO_iNonoptEnd;
        }
    } else {
        psaoGet  = __TTINY_SHELL_GET_ARTOPT(ptcbCur);
        piNonoptEnd = &psaoGet->SAO_iNonoptEnd;
    }
    __TTINY_SHELL_UNLOCK();                                             /*  �������                    */
    
    return  (piNonoptEnd);
}
/*********************************************************************************************************
** ��������: __tshellOptNonoptStart
** ��������: ��� ARGOPT �������е� SAO_iNonoptStart ��ַ
** �䡡��  : NONE
** �䡡��  : SAO_iNonoptStart ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  *__tshellOptNonoptStart (VOID)
{
    REGISTER INT                *piNonoptStart;
    REGISTER __PTSHELL_ARGOPT    psaoGet;
             PLW_CLASS_TCB       ptcbCur;
             
    LW_TCB_GET_CUR_SAFE(ptcbCur);

    __TTINY_SHELL_LOCK();                                               /*  shell ����                  */
    if (LW_NULL == __TTINY_SHELL_GET_ARTOPT(ptcbCur)) {
        psaoGet =  (__PTSHELL_ARGOPT)__SHEAP_ALLOC(sizeof(__TSHELL_ARGOPT));
        if (!psaoGet) {
            _G_saoTmp.SAO_pcPlace = _G_saoTmp.SAO_cEMsg;
            _G_saoTmp.SAO_pcEMsg  = _G_saoTmp.SAO_cEMsg;
            piNonoptStart = &_G_saoTmp.SAO_iNonoptStart;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "\"int nonopt_start\" system low memory.\r\n");
        } else {
            __TTINY_SHELL_SET_ARTOPT(ptcbCur, psaoGet);
            *psaoGet = _G_saoStd;
            psaoGet->SAO_pcPlace = psaoGet->SAO_cEMsg;
            psaoGet->SAO_pcEMsg  = psaoGet->SAO_cEMsg;
            piNonoptStart = &psaoGet->SAO_iNonoptStart;
        }
    } else {
        psaoGet  = __TTINY_SHELL_GET_ARTOPT(ptcbCur);
        piNonoptStart = &psaoGet->SAO_iNonoptStart;
    }
    __TTINY_SHELL_UNLOCK();                                             /*  �������                    */
    
    return  (piNonoptStart);
}
/*********************************************************************************************************
** ��������: __tshellOptPlace
** ��������: ��� ARGOPT �������е� SAO_pcPlace ��ַ
** �䡡��  : NONE
** �䡡��  : SAO_pcPlace ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PCHAR   *__tshellOptPlace (VOID)
{
    REGISTER PCHAR              *ppcPlace;
    REGISTER __PTSHELL_ARGOPT    psaoGet;
             PLW_CLASS_TCB       ptcbCur;
             
    LW_TCB_GET_CUR_SAFE(ptcbCur);

    __TTINY_SHELL_LOCK();                                               /*  shell ����                  */
    if (LW_NULL == __TTINY_SHELL_GET_ARTOPT(ptcbCur)) {
        psaoGet =  (__PTSHELL_ARGOPT)__SHEAP_ALLOC(sizeof(__TSHELL_ARGOPT));
        if (!psaoGet) {
            _G_saoTmp.SAO_pcPlace = _G_saoTmp.SAO_cEMsg;
            _G_saoTmp.SAO_pcEMsg  = _G_saoTmp.SAO_cEMsg;
            ppcPlace = &_G_saoTmp.SAO_pcPlace;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "\"int place\" system low memory.\r\n");
        } else {
            __TTINY_SHELL_SET_ARTOPT(ptcbCur, psaoGet);
            *psaoGet  = _G_saoStd;
            psaoGet->SAO_pcPlace = psaoGet->SAO_cEMsg;
            psaoGet->SAO_pcEMsg  = psaoGet->SAO_cEMsg;
            ppcPlace = &psaoGet->SAO_pcPlace;
        }
    } else {
        psaoGet  = __TTINY_SHELL_GET_ARTOPT(ptcbCur);
        ppcPlace = &psaoGet->SAO_pcPlace;
    }
    __TTINY_SHELL_UNLOCK();                                             /*  �������                    */
    
    return  (ppcPlace);
}
/*********************************************************************************************************
** ��������: __tshellOptEMsg
** ��������: ��� ARGOPT �������е� SAO_cEMsg ��ַ
** �䡡��  : NONE
** �䡡��  : SAO_cEMsg ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PCHAR  *__tshellOptEMsg (VOID)
{
    REGISTER PCHAR              *ppcEMsg;
    REGISTER __PTSHELL_ARGOPT    psaoGet;
             PLW_CLASS_TCB       ptcbCur;
             
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    __TTINY_SHELL_LOCK();                                               /*  shell ����                  */
    if (LW_NULL == __TTINY_SHELL_GET_ARTOPT(ptcbCur)) {
        psaoGet =  (__PTSHELL_ARGOPT)__SHEAP_ALLOC(sizeof(__TSHELL_ARGOPT));
        if (!psaoGet) {
            _G_saoTmp.SAO_pcPlace = _G_saoTmp.SAO_cEMsg;
            _G_saoTmp.SAO_pcEMsg  = _G_saoTmp.SAO_cEMsg;
            ppcEMsg = &_G_saoTmp.SAO_pcEMsg;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "\"char  EMSG[1]\" system low memory.\r\n");
        } else {
            __TTINY_SHELL_SET_ARTOPT(ptcbCur, psaoGet);
            *psaoGet = _G_saoStd;
            psaoGet->SAO_pcPlace = psaoGet->SAO_cEMsg;
            psaoGet->SAO_pcEMsg  = psaoGet->SAO_cEMsg;
            ppcEMsg  = &psaoGet->SAO_pcEMsg;
        }
    } else {
        psaoGet = __TTINY_SHELL_GET_ARTOPT(ptcbCur);
        ppcEMsg = &psaoGet->SAO_pcEMsg;
    }
    __TTINY_SHELL_UNLOCK();                                             /*  �������                    */
    
    return  (ppcEMsg);
}
/*********************************************************************************************************
** ��������: API_TShellOptErr
** ��������: �൱�� int opterr �������.
**           ����ʱ��getopt()����Ϊ����Чѡ��͡�ȱ�ٲ���ѡ�������������Ϣ.
** �䡡��  : NONE
** �䡡��  : getopt ����������Ӧ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  *API_TShellOptErr (VOID)
{
    REGISTER INT                *piOptErr;
    REGISTER __PTSHELL_ARGOPT    psaoGet;
             PLW_CLASS_TCB       ptcbCur;
             
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    __TTINY_SHELL_LOCK();                                               /*  shell ����                  */
    if (LW_NULL == __TTINY_SHELL_GET_ARTOPT(ptcbCur)) {
        psaoGet =  (__PTSHELL_ARGOPT)__SHEAP_ALLOC(sizeof(__TSHELL_ARGOPT));
        if (!psaoGet) {
            _G_saoTmp.SAO_pcPlace = _G_saoTmp.SAO_cEMsg;
            _G_saoTmp.SAO_pcEMsg  = _G_saoTmp.SAO_cEMsg;
            piOptErr = &_G_saoTmp.SAO_iOptErr;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "\"int opterr\" system low memory.\r\n");
        } else {
            __TTINY_SHELL_SET_ARTOPT(ptcbCur, psaoGet);
            *psaoGet = _G_saoStd;
            psaoGet->SAO_pcPlace = psaoGet->SAO_cEMsg;
            psaoGet->SAO_pcEMsg  = psaoGet->SAO_cEMsg;
            piOptErr = &psaoGet->SAO_iOptErr;
        }
    } else {
        psaoGet  = __TTINY_SHELL_GET_ARTOPT(ptcbCur);
        piOptErr = &psaoGet->SAO_iOptErr;
    }
    __TTINY_SHELL_UNLOCK();                                             /*  �������                    */
    
    return  (piOptErr);
}
/*********************************************************************************************************
** ��������: API_TShellOptInd
** ��������: �൱�� int optind �������.
**           argv�ĵ�ǰ����ֵ����getopt()��whileѭ����ʹ��ʱ��ѭ��������ʣ�µ��ִ���Ϊ��������
**           ��argv[optind]��argv[argc-1]�п����ҵ���
** �䡡��  : NONE
** �䡡��  : getopt ����������Ӧ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  *API_TShellOptInd (VOID)
{
    REGISTER INT                *piOptInd;
    REGISTER __PTSHELL_ARGOPT    psaoGet;
             PLW_CLASS_TCB       ptcbCur;
             
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    __TTINY_SHELL_LOCK();                                               /*  shell ����                  */
    if (LW_NULL == __TTINY_SHELL_GET_ARTOPT(ptcbCur)) {
        psaoGet =  (__PTSHELL_ARGOPT)__SHEAP_ALLOC(sizeof(__TSHELL_ARGOPT));
        if (!psaoGet) {
            _G_saoTmp.SAO_pcPlace = _G_saoTmp.SAO_cEMsg;
            _G_saoTmp.SAO_pcEMsg  = _G_saoTmp.SAO_cEMsg;
            piOptInd = &_G_saoTmp.SAO_iOptInd;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "\"int optind\" system low memory.\r\n");
        } else {
            __TTINY_SHELL_SET_ARTOPT(ptcbCur, psaoGet);
            *psaoGet = _G_saoStd;
            psaoGet->SAO_pcPlace = psaoGet->SAO_cEMsg;
            psaoGet->SAO_pcEMsg  = psaoGet->SAO_cEMsg;
            piOptInd = &psaoGet->SAO_iOptInd;
        }
    } else {
        psaoGet  = __TTINY_SHELL_GET_ARTOPT(ptcbCur);
        piOptInd = &psaoGet->SAO_iOptInd;
    }
    __TTINY_SHELL_UNLOCK();                                             /*  �������                    */
    
    return  (piOptInd);
}
/*********************************************************************************************************
** ��������: API_TShellOptOpt
** ��������: �൱�� int optopt �������.
**           ��������Чѡ���ַ�֮ʱ��getopt()�����򷵻�'?'�ַ����򷵻�':'�ַ���
**           ����optopt�����������ֵ���Чѡ���ַ���
** �䡡��  : NONE
** �䡡��  : getopt ����������Ӧ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  *API_TShellOptOpt (VOID)
{
    REGISTER INT                *piOptOpt;
    REGISTER __PTSHELL_ARGOPT    psaoGet;
             PLW_CLASS_TCB       ptcbCur;
             
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    __TTINY_SHELL_LOCK();                                               /*  shell ����                  */
    if (LW_NULL == __TTINY_SHELL_GET_ARTOPT(ptcbCur)) {
        psaoGet =  (__PTSHELL_ARGOPT)__SHEAP_ALLOC(sizeof(__TSHELL_ARGOPT));
        if (!psaoGet) {
            _G_saoTmp.SAO_pcPlace = _G_saoTmp.SAO_cEMsg;
            _G_saoTmp.SAO_pcEMsg  = _G_saoTmp.SAO_cEMsg;
            piOptOpt = &_G_saoTmp.SAO_iOptOpt;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "\"int optopt\" system low memory.\r\n");
        } else {
            __TTINY_SHELL_SET_ARTOPT(ptcbCur, psaoGet);
            *psaoGet = _G_saoStd;
            psaoGet->SAO_pcPlace = psaoGet->SAO_cEMsg;
            psaoGet->SAO_pcEMsg  = psaoGet->SAO_cEMsg;
            piOptOpt = &psaoGet->SAO_iOptOpt;
        }
    } else {
        psaoGet  = __TTINY_SHELL_GET_ARTOPT(ptcbCur);
        piOptOpt = &psaoGet->SAO_iOptOpt;
    }
    __TTINY_SHELL_UNLOCK();                                             /*  �������                    */
    
    return  (piOptOpt);
}
/*********************************************************************************************************
** ��������: API_TShellOptReset
** ��������: �൱�� int optreset �������.
**           ���˱���Ϊ 1 ʱ, ��ʾ�ӵ�һ��������ʼ����.
** �䡡��  : NONE
** �䡡��  : getopt ����������Ӧ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  *API_TShellOptReset (VOID)
{
    REGISTER INT                *piOptReset;
    REGISTER __PTSHELL_ARGOPT    psaoGet;
             PLW_CLASS_TCB       ptcbCur;
             
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    __TTINY_SHELL_LOCK();                                               /*  shell ����                  */
    if (LW_NULL == __TTINY_SHELL_GET_ARTOPT(ptcbCur)) {
        psaoGet =  (__PTSHELL_ARGOPT)__SHEAP_ALLOC(sizeof(__TSHELL_ARGOPT));
        if (!psaoGet) {
            _G_saoTmp.SAO_pcPlace = _G_saoTmp.SAO_cEMsg;
            _G_saoTmp.SAO_pcEMsg  = _G_saoTmp.SAO_cEMsg;
            piOptReset = &_G_saoTmp.SAO_iOptReset;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "\"int optopt\" system low memory.\r\n");
        } else {
            __TTINY_SHELL_SET_ARTOPT(ptcbCur, psaoGet);
            *psaoGet = _G_saoStd;
            psaoGet->SAO_pcPlace = psaoGet->SAO_cEMsg;
            psaoGet->SAO_pcEMsg  = psaoGet->SAO_cEMsg;
            piOptReset = &psaoGet->SAO_iOptReset;
        }
    } else {
        psaoGet  = __TTINY_SHELL_GET_ARTOPT(ptcbCur);
        piOptReset = &psaoGet->SAO_iOptReset;
    }
    __TTINY_SHELL_UNLOCK();                                             /*  �������                    */
    
    return  (piOptReset);
}
/*********************************************************************************************************
** ��������: API_TShellOptArg
** ��������: �൱�� int optarg �������.
**           ��ǰѡ������ִ� (�����).
** �䡡��  : NONE
** �䡡��  : getopt ����������Ӧ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PCHAR   *API_TShellOptArg (VOID)
{
    REGISTER PCHAR              *ppcOptArg;
    REGISTER __PTSHELL_ARGOPT    psaoGet;
             PLW_CLASS_TCB       ptcbCur;
             
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    __TTINY_SHELL_LOCK();                                               /*  shell ����                  */
    if (LW_NULL == __TTINY_SHELL_GET_ARTOPT(ptcbCur)) {
        psaoGet =  (__PTSHELL_ARGOPT)__SHEAP_ALLOC(sizeof(__TSHELL_ARGOPT));
        if (!psaoGet) {
            _G_saoTmp.SAO_pcPlace = _G_saoTmp.SAO_cEMsg;
            _G_saoTmp.SAO_pcEMsg  = _G_saoTmp.SAO_cEMsg;
            ppcOptArg = &_G_saoTmp.SAO_pcOptArg;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "\"int optarg\" system low memory.\r\n");
        } else {
            __TTINY_SHELL_SET_ARTOPT(ptcbCur, psaoGet);
            *psaoGet  = _G_saoStd;
            psaoGet->SAO_pcPlace = psaoGet->SAO_cEMsg;
            psaoGet->SAO_pcEMsg  = psaoGet->SAO_cEMsg;
            ppcOptArg = &psaoGet->SAO_pcOptArg;
        }
    } else {
        psaoGet  = __TTINY_SHELL_GET_ARTOPT(ptcbCur);
        ppcOptArg = &psaoGet->SAO_pcOptArg;
    }
    __TTINY_SHELL_UNLOCK();                                             /*  �������                    */
    
    return  (ppcOptArg);
}
/*********************************************************************************************************
** ��������: API_TShellOptFree
** ��������: ���е��ù� getopt ���� getopt_long ������, ������Ҫ������������ͷ���ʱ�ڴ�,
**           �Ӷ���������ڴ�й¶.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_TShellOptFree (VOID)
{
    PLW_CLASS_TCB       ptcbCur;
             
    LW_TCB_GET_CUR_SAFE(ptcbCur);

    __TTINY_SHELL_LOCK();                                               /*  shell ����                  */
    if (__TTINY_SHELL_GET_ARTOPT(ptcbCur)) {
        __SHEAP_FREE(__TTINY_SHELL_GET_ARTOPT(ptcbCur));
        __TTINY_SHELL_SET_ARTOPT(ptcbCur, LW_NULL);
    }
    __TTINY_SHELL_UNLOCK();                                             /*  �������                    */
}
/*********************************************************************************************************
** ��������: __tShellOptDeleteHook
** ��������: �߳�ɾ�� hook.
** �䡡��  : ulId          �߳� ID
**           pvReturnVal   �̷߳���ֵ
**           ptcb          �߳̿��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __tShellOptDeleteHook (LW_OBJECT_HANDLE  ulId, 
                             PVOID             pvReturnVal, 
                             PLW_CLASS_TCB     ptcb)
{
    __TTINY_SHELL_LOCK();                                               /*  shell ����                  */
    if (__TTINY_SHELL_GET_ARTOPT(ptcb)) {
        __SHEAP_FREE(__TTINY_SHELL_GET_ARTOPT(ptcb));
        __TTINY_SHELL_SET_ARTOPT(ptcb, LW_NULL);                        /*  ��ֹ���������              */
    }
    __TTINY_SHELL_UNLOCK();                                             /*  �������                    */
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
