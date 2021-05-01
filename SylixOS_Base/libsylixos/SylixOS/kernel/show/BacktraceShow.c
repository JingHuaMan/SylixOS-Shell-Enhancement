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
** ��   ��   ��: BacktraceShow.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2016 �� 11 �� 12 ��
**
** ��        ��: ��ʾ����ջ��Ϣ.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_FIO_LIB_EN > 0
/*********************************************************************************************************
  loader
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "dlfcn.h"
#include "../SylixOS/loader/include/loader_lib.h"
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  backtrace ��������
*********************************************************************************************************/
extern int backtrace(void **array, int size);
/*********************************************************************************************************
** ��������: API_BacktraceShow
** ��������: ��ʾ����ջ��Ϣ.
** �䡡��  : iFd       ����ļ�
**           iMaxDepth ����ջ���
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
VOID  API_BacktraceShow (INT  iFd, INT  iMaxDepth)
{
#ifdef __GNUC__
#define SHOWSTACK_SIZE  100

    PVOID   pvFrame[SHOWSTACK_SIZE];
    INT     i, iCnt;
    
    if (iMaxDepth > SHOWSTACK_SIZE) {
        iMaxDepth = SHOWSTACK_SIZE;
    
    } else if (iMaxDepth < 0) {
        iMaxDepth = 1;
    }
    
    iCnt = backtrace(pvFrame, iMaxDepth);
    if (iCnt > 0) {
#if LW_CFG_MODULELOADER_EN > 0
        Dl_info         dlinfo;
        LW_LD_VPROC    *pvproc = vprocGetCur();
    
        for (i = 0; i < iCnt; i++) {
            if ((API_ModuleAddr(pvFrame[i], &dlinfo, pvproc) == ERROR_NONE) &&
                (dlinfo.dli_sname)) {
                PVOID   pvBaseAddr;

                pvBaseAddr = dlinfo.dli_fbase ? \
                             ((LW_LD_EXEC_MODULE *)dlinfo.dli_fbase)->EMOD_pvBaseAddr : LW_NULL;
                if (iFd >= 0) {
                    fdprintf(iFd, "[%02d] %p (%s@%p+0x%x %s+%zu)\n",
                             iCnt - i,
                             pvFrame[i],
                             dlinfo.dli_fname,
                             pvBaseAddr,
                             pvFrame[i] - pvBaseAddr,
                             dlinfo.dli_sname,
                             ((size_t)pvFrame[i] - (size_t)dlinfo.dli_saddr));

                } else {
                    _DebugFormat(__PRINTMESSAGE_LEVEL, "[%02d] %p (%s@%p+0x%x %s+%zu)\r\n",
                             iCnt - i,
                             pvFrame[i],
                             dlinfo.dli_fname,
                             pvBaseAddr,
                             pvFrame[i] - pvBaseAddr,
                             dlinfo.dli_sname,
                             ((size_t)pvFrame[i] - (size_t)dlinfo.dli_saddr));
                }

            } else {
                if (iFd >= 0) {
                    fdprintf(iFd, "[%02d] %p (<unknown>)\n", iCnt - i, pvFrame[i]);
                
                } else {
                    _DebugFormat(__PRINTMESSAGE_LEVEL, "[%02d] %p (<unknown>)\r\n", iCnt - i, pvFrame[i]);
                }
            }
        }
#else
        for (i = 0; i < iCnt; i++) {
            if (iFd >= 0) {
                fdprintf(iFd, "[%02d] %p\n", iCnt - i, pvFrame[i]);
                
            } else {
                _DebugFormat(__PRINTMESSAGE_LEVEL, "[%02d] %p\r\n", iCnt - i, pvFrame[i]);
            }
        }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    }
#endif                                                                  /*  !__GNUC__                   */
}
/*********************************************************************************************************
** ��������: API_BacktracePrint
** ��������: ��ӡ����ջ��Ϣ������.
** �䡡��  : pvBuffer  ����λ��
**           stSize    �����С
**           iMaxDepth ����ջ���
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
VOID  API_BacktracePrint (PVOID  pvBuffer, size_t  stSize, INT  iMaxDepth)
{
#ifdef __GNUC__
#define SHOWSTACK_SIZE  100

    PVOID   pvFrame[SHOWSTACK_SIZE];
    INT     i, iCnt;
    size_t  stOft = 0;
    
    if (iMaxDepth > SHOWSTACK_SIZE) {
        iMaxDepth = SHOWSTACK_SIZE;
    
    } else if (iMaxDepth < 0) {
        iMaxDepth = 1;
    }
    
    iCnt = backtrace(pvFrame, iMaxDepth);
    if (iCnt > 0) {
#if LW_CFG_MODULELOADER_EN > 0
        Dl_info         dlinfo;
        LW_LD_VPROC    *pvproc = vprocGetCur();
    
        for (i = 0; i < iCnt; i++) {
            if ((API_ModuleAddr(pvFrame[i], &dlinfo, pvproc) == ERROR_NONE) &&
                (dlinfo.dli_sname)) {
                PVOID   pvBaseAddr;

                pvBaseAddr = dlinfo.dli_fbase ? \
                             ((LW_LD_EXEC_MODULE *)dlinfo.dli_fbase)->EMOD_pvBaseAddr : LW_NULL;

                stOft = bnprintf(pvBuffer, stSize, stOft, "[%02d] %p (%s@%p+0x%x %s+%zu)\n",
                        iCnt - i,
                        pvFrame[i],
                        dlinfo.dli_fname,
                        pvBaseAddr,
                        pvFrame[i] - pvBaseAddr,
                        dlinfo.dli_sname,
                        ((size_t)pvFrame[i] - (size_t)dlinfo.dli_saddr));

            } else {
                stOft = bnprintf(pvBuffer, stSize, stOft, "[%02d] %p (<unknown>)\n", iCnt - i, pvFrame[i]);
            }
        }
#else
        for (i = 0; i < iCnt; i++) {
            stOft = bnprintf(pvBuffer, stSize, stOft, "[%02d] %p\n", iCnt - i, pvFrame[i]);
        }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    }
#endif                                                                  /*  !__GNUC__                   */
}

#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
