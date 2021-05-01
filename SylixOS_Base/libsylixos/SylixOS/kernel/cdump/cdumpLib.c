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
** ��   ��   ��: cdumpLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 12 �� 01 ��
**
** ��        ��: ϵͳ/Ӧ�ñ�����Ϣ��¼.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if (LW_CFG_CDUMP_EN > 0) && (LW_CFG_DEVICE_EN > 0)
/*********************************************************************************************************
  ������Ϣ
*********************************************************************************************************/
static PVOID  _K_pvCrashDumpBuffer = (PVOID)PX_ERROR;
static size_t _K_stCrashDumpSize   = LW_CFG_CDUMP_BUF_SIZE;
/*********************************************************************************************************
  ���Ϣ����
*********************************************************************************************************/
#define LW_CDUMP_BUF_SIZE   (_K_stCrashDumpSize)
#define LW_CDUMP_MAX_LEN    (LW_CDUMP_BUF_SIZE - 1)
/*********************************************************************************************************
** ��������: _CrashDumpAbortStkOf
** ��������: ��ջ���������Ϣ��¼.
** �䡡��  : ulRetAddr     �쳣���� PC ��ַ
**           ulAbortAddr   �쳣��ַ
**           pcInfo        �쳣��Ϣ
**           ptcb          �쳣����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _CrashDumpAbortStkOf (addr_t  ulRetAddr, addr_t  ulAbortAddr, CPCHAR  pcInfo, PLW_CLASS_TCB  ptcb)
{
    PCHAR   pcCdump = (PCHAR)_K_pvCrashDumpBuffer;
    size_t  stOft   = 4;
    
    if (!pcCdump || (pcCdump == (PCHAR)PX_ERROR)) {
        return;
    }
    
    pcCdump[0] = LW_CDUMP_MAGIC_0;
    pcCdump[1] = LW_CDUMP_MAGIC_1;
    pcCdump[2] = LW_CDUMP_MAGIC_2;
    pcCdump[3] = LW_CDUMP_MAGIC_3;
    
    stOft = bnprintf(pcCdump, LW_CDUMP_MAX_LEN, stOft, 
                     "FATAL ERROR: thread %lx[%s] stack overflow. "
                     "ret_addr: 0x%08lx abt_addr: 0x%08lx abt_type: %s\n"
                     "rebooting...\n",
                     ptcb->TCB_ulId, ptcb->TCB_cThreadName,
                     ulRetAddr, ulAbortAddr, pcInfo);
    
    pcCdump[stOft] = PX_EOS;
}
/*********************************************************************************************************
** ��������: _CrashDumpAbortFatal
** ��������: ������Ϣ��¼.
** �䡡��  : ulRetAddr     �쳣���� PC ��ַ
**           ulAbortAddr   �쳣��ַ
**           pcInfo        �쳣��Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _CrashDumpAbortFatal (addr_t  ulRetAddr, addr_t  ulAbortAddr, CPCHAR  pcInfo)
{
    PCHAR   pcCdump = (PCHAR)_K_pvCrashDumpBuffer;
    size_t  stOft   = 4;
    
    if (!pcCdump || (pcCdump == (PCHAR)PX_ERROR)) {
        return;
    }
    
    pcCdump[0] = LW_CDUMP_MAGIC_0;
    pcCdump[1] = LW_CDUMP_MAGIC_1;
    pcCdump[2] = LW_CDUMP_MAGIC_2;
    pcCdump[3] = LW_CDUMP_MAGIC_3;
    
    stOft = bnprintf(pcCdump, LW_CDUMP_MAX_LEN, stOft, 
                     "FATAL ERROR: abort occur in exception mode. "
                     "ret_addr: 0x%08lx abt_addr: 0x%08lx abt_type: %s\n"
                     "rebooting...\n",
                     ulRetAddr, ulAbortAddr, pcInfo);
    
    pcCdump[stOft] = PX_EOS;
}
/*********************************************************************************************************
** ��������: _CrashDumpAbortKernel
** ��������: �ں˱�����Ϣ��¼.
** �䡡��  : ulOwner       ռ���ں˵�����
**           pcKernelFunc  �����ں˵ĺ���
**           pvCtx         �쳣��Ϣ�ṹ
**           pcInfo        �쳣��Ϣ
**           pcTail        ������Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _CrashDumpAbortKernel (LW_OBJECT_HANDLE   ulOwner, 
                             CPCHAR             pcKernelFunc, 
                             PVOID              pvCtx,
                             CPCHAR             pcInfo, 
                             CPCHAR             pcTail)
{
    PLW_VMM_ABORT_CTX  pabtctx = (PLW_VMM_ABORT_CTX)pvCtx;
    PCHAR              pcCdump = (PCHAR)_K_pvCrashDumpBuffer;
    size_t             stOft;
    
    if (!pcCdump || (pcCdump == (PCHAR)PX_ERROR)) {
        return;
    }
    
    lib_bzero(pcCdump, LW_CDUMP_BUF_SIZE);
    
    pcCdump[0] = LW_CDUMP_MAGIC_0;
    pcCdump[1] = LW_CDUMP_MAGIC_1;
    pcCdump[2] = LW_CDUMP_MAGIC_2;
    pcCdump[3] = LW_CDUMP_MAGIC_3;
    
    archTaskCtxPrint(&pcCdump[4], (LW_CDUMP_MAX_LEN - LW_CDUMP_MAGIC_LEN), 
                     &pabtctx->ABTCTX_archRegCtx);
                     
    stOft = lib_strlen(pcCdump);
    
    stOft = bnprintf(pcCdump, LW_CDUMP_MAX_LEN, stOft, 
                     "FATAL ERROR: abort in kernel status. "
                     "kowner: 0x%08lx, kfunc: %s, "
                     "ret_addr: 0x%08lx abt_addr: 0x%08lx, abt_type: %s, %s.\n",
                     ulOwner, pcKernelFunc, 
                     pabtctx->ABTCTX_ulRetAddr,
                     pabtctx->ABTCTX_ulAbortAddr, pcInfo, pcTail);
}
/*********************************************************************************************************
** ��������: _CrashDumpAbortAccess
** ��������: ������Ϣ��¼.
** �䡡��  : pvCtx            �쳣��Ϣ�ṹ
**           pcInfo           �쳣��Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _CrashDumpAbortAccess (PVOID  pvCtx, CPCHAR  pcInfo)
{
    PLW_VMM_ABORT_CTX  pabtctx = (PLW_VMM_ABORT_CTX)pvCtx;
    PCHAR              pcCdump = (PCHAR)_K_pvCrashDumpBuffer;
    size_t             stOft;
    
    if (!pcCdump || (pcCdump == (PCHAR)PX_ERROR)) {
        return;
    }
    
    lib_bzero(pcCdump, LW_CDUMP_BUF_SIZE);
    
    pcCdump[0] = LW_CDUMP_MAGIC_0;
    pcCdump[1] = LW_CDUMP_MAGIC_1;
    pcCdump[2] = LW_CDUMP_MAGIC_2;
    pcCdump[3] = LW_CDUMP_MAGIC_3;
    
    archTaskCtxPrint(&pcCdump[4], (LW_CDUMP_MAX_LEN - LW_CDUMP_MAGIC_LEN), 
                     &pabtctx->ABTCTX_archRegCtx);
    
    stOft = lib_strlen(pcCdump);
    
    switch (__ABTCTX_ABORT_TYPE(pabtctx)) {
    
    case LW_VMM_ABORT_TYPE_UNDEF:
        stOft = bnprintf(pcCdump, LW_CDUMP_MAX_LEN, stOft, 
                         "UNDEF ERROR: abort in thread %lx[%s]. "
                         "ret_addr: 0x%08lx abt_addr: 0x%08lx, abt_type: %s.\n",
                         pabtctx->ABTCTX_ptcb->TCB_ulId,
                         pabtctx->ABTCTX_ptcb->TCB_cThreadName,
                         pabtctx->ABTCTX_ulRetAddr,
                         pabtctx->ABTCTX_ulAbortAddr, pcInfo);
        break;
    
    case LW_VMM_ABORT_TYPE_FPE:
        stOft = bnprintf(pcCdump, LW_CDUMP_MAX_LEN, stOft, 
                         "FPU ERROR: abort in thread %lx[%s]. "
                         "ret_addr: 0x%08lx abt_addr: 0x%08lx, abt_type: %s.\n",
                         pabtctx->ABTCTX_ptcb->TCB_ulId,
                         pabtctx->ABTCTX_ptcb->TCB_cThreadName,
                         pabtctx->ABTCTX_ulRetAddr,
                         pabtctx->ABTCTX_ulAbortAddr, pcInfo);
        break;
        
    default:
        stOft = bnprintf(pcCdump, LW_CDUMP_MAX_LEN, stOft, 
                         "ACCESS ERROR: abort in thread %lx[%s]. "
                         "ret_addr: 0x%08lx abt_addr: 0x%08lx, abt_type: %s.\n",
                         pabtctx->ABTCTX_ptcb->TCB_ulId,
                         pabtctx->ABTCTX_ptcb->TCB_cThreadName,
                         pabtctx->ABTCTX_ulRetAddr,
                         pabtctx->ABTCTX_ulAbortAddr, pcInfo);
        break;
    }
    
    API_BacktracePrint(&pcCdump[stOft], (LW_CDUMP_MAX_LEN - stOft), LW_CFG_CDUMP_CALL_STACK_DEPTH);
}
/*********************************************************************************************************
** ��������: _CrashDumpSet
** ��������: ����ϵͳ/Ӧ�ñ�����Ϣ��¼λ��.
** �䡡��  : pvCdump           �����ַ
**           stSize            �����С
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _CrashDumpSet (PVOID  pvCdump, size_t  stSize)
{
    _K_pvCrashDumpBuffer = pvCdump;
    _K_stCrashDumpSize   = stSize;
}
/*********************************************************************************************************
** ��������: _CrashDumpGet
** ��������: ��ȡϵͳ/Ӧ�ñ�����Ϣ��¼λ��.
** �䡡��  : pstSize           �����С
** �䡡��  : �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PVOID  _CrashDumpGet (size_t  *pstSize)
{
    *pstSize = _K_stCrashDumpSize;
    
    return  (_K_pvCrashDumpBuffer);
}

#endif                                                                  /*  LW_CFG_CDUMP_EN > 0         */
                                                                        /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
