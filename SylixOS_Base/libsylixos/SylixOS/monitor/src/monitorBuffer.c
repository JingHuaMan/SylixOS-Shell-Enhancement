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
** ��   ��   ��: monitorBuffer.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 08 �� 17 ��
**
** ��        ��: SylixOS �ں��¼����������������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MONITOR_EN > 0
/*********************************************************************************************************
  �¼��������궨��
*********************************************************************************************************/
#define MONITOR_BUFFER_MIN_SIZE             (8 * 1024)                  /*  ���ٴ�С                    */
#define MONITOR_BUFFER_UPDATE_SIZE          (4 * 1024)                  /*  ���пռ�С�ڴ˽��ϴ�        */
/*********************************************************************************************************
  �¼��������ṹ
*********************************************************************************************************/
typedef struct {
    PUCHAR              MB_pucBuffer;                                   /*  �������׵�ַ                */
    size_t              MB_stSize;                                      /*  ��������С                  */
    size_t              MB_stLeft;                                      /*  ʣ��ռ��С                */
    
    PUCHAR              MB_pucPut;                                      /*  д��ָ��                    */
    PUCHAR              MB_pucGet;                                      /*  ����ָ��                    */
    
    LW_SPINLOCK_DEFINE (MB_slLock);                                     /*  spinlock                    */
    
    LW_OBJECT_HANDLE    MB_hReadSync;                                   /*  ��ͬ��                      */
} MONITOR_BUFFER;
typedef MONITOR_BUFFER *PMONITOR_BUFFER;
/*********************************************************************************************************
** ��������: __monitorBufferCreate
** ��������: ����һ����ظ��ٽڵ��¼�������
** �䡡��  : stSize            ��������С
** �䡡��  : �¼����������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PVOID  __monitorBufferCreate (size_t  stSize)
{
    PMONITOR_BUFFER  pmb;
    
    if (stSize < MONITOR_BUFFER_MIN_SIZE) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (LW_NULL);
    }
    
    pmb = (PMONITOR_BUFFER)__SHEAP_ALLOC(sizeof(MONITOR_BUFFER) + stSize);
    if (!pmb) {
        _ErrorHandle(ERROR_MONITOR_ENOMEM);
        return  (LW_NULL);
    }
    
    pmb->MB_hReadSync = API_SemaphoreBCreate("mb_sync", LW_FALSE, LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (pmb->MB_hReadSync == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(pmb);
        return  (LW_NULL);
    }
    
    pmb->MB_pucBuffer = (PUCHAR)pmb + sizeof(MONITOR_BUFFER);
    pmb->MB_stSize    = stSize;
    pmb->MB_stLeft    = stSize;
    
    pmb->MB_pucPut    = pmb->MB_pucBuffer;
    pmb->MB_pucGet    = pmb->MB_pucBuffer;
    
    LW_SPIN_INIT(&pmb->MB_slLock);
    
    return  ((PVOID)pmb);
}
/*********************************************************************************************************
** ��������: __monitorBufferDelete
** ��������: ɾ��һ����ظ��ٽڵ��¼�������
** �䡡��  : pvMonitorBuffer   �¼�������
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __monitorBufferDelete (PVOID  pvMonitorBuffer)
{
    PMONITOR_BUFFER  pmb = (PMONITOR_BUFFER)pvMonitorBuffer;

    if (!pmb) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    LW_KERNEL_JOB_DEL(2, (VOIDFUNCPTR)API_SemaphoreBPost, (PVOID)pmb->MB_hReadSync,
                      LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    
    API_SemaphoreBDelete(&pmb->MB_hReadSync);
    
    __SHEAP_FREE(pmb);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __monitorBufferPut
** ��������: ���ظ��ٽڵ��¼�����������һ���¼���Ϣ
** �䡡��  : pvMonitorBuffer   �¼�������
**           pvEvent           �¼���Ϣ
** �䡡��  : д��ĳ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  __monitorBufferPut (PVOID      pvMonitorBuffer,
                             CPVOID     pvEvent)
{
    INTREG           iregInterLevel;
    PMONITOR_BUFFER  pmb = (PMONITOR_BUFFER)pvMonitorBuffer;
    PUCHAR           pucLen = (PUCHAR)pvEvent;
    
    size_t           stLen;
    size_t           stSize;
    
    BOOL             bUpdate;
    
    stSize = (size_t)((pucLen[0] << 8) + pucLen[1]);                    /*  ǰ���ֽ�Ϊ��˳�����Ϣ      */
    if (stSize > MONITOR_EVENT_MAX_SIZE) {
        return  (0);
    }

    LW_SPIN_LOCK_QUICK(&pmb->MB_slLock, &iregInterLevel);
    if (pmb->MB_stLeft < stSize) {
        LW_SPIN_UNLOCK_QUICK(&pmb->MB_slLock, iregInterLevel);
        _ErrorHandle(ERROR_MONITOR_ENOSPC);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "buffer full.\r\n");
        return  (0);
    }
    
    if ((pmb->MB_stLeft > MONITOR_BUFFER_UPDATE_SIZE) &&
        ((pmb->MB_stLeft - stSize) < MONITOR_BUFFER_UPDATE_SIZE)) {
        bUpdate = LW_TRUE;
    } else {
        bUpdate = LW_FALSE;
    }
    
    stLen = (pmb->MB_pucBuffer + pmb->MB_stSize) - pmb->MB_pucPut;
    if (stLen >= stSize) {
        lib_memcpy(pmb->MB_pucPut, pvEvent, stSize);
        
        if (stLen > stSize) {
            pmb->MB_pucPut += stSize;
        
        } else {
            pmb->MB_pucPut = pmb->MB_pucBuffer;
        }
        
    } else {
        lib_memcpy(pmb->MB_pucPut, pvEvent, stLen);
        pmb->MB_pucPut  = pmb->MB_pucBuffer;
        
        lib_memcpy(pmb->MB_pucPut, (PUCHAR)pvEvent + stLen, stSize - stLen);
        pmb->MB_pucPut += (stSize - stLen);
    }
    
    pmb->MB_stLeft -= stSize;
    
    LW_SPIN_UNLOCK_QUICK(&pmb->MB_slLock, iregInterLevel);
    
    if (bUpdate) {
        LW_KERNEL_JOB_ADD((VOIDFUNCPTR)API_SemaphoreBPost, (PVOID)pmb->MB_hReadSync,
                          LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    }

    return  ((ssize_t)stSize);
}
/*********************************************************************************************************
** ��������: __monitorBufferGet
** ��������: �Ӽ�ظ��ٽڵ��¼���������ȡһ���¼���Ϣ
** �䡡��  : pvMonitorBuffer   �¼�������
**           pvEvent           �¼���Ϣ
**           stBufferSize      ����������
**           ulTimeout         ��ʱʱ��
** �䡡��  : ��ȡ���¼�ʵ�ʳ���
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �ӻ�������ȡ�¼�ʱ���Ȳ鿴�������Ƿ�����Ϣ, ���û��, ��ȴ��ź���. ��Ϊ������װ���¼�ʱ��
             ���������ͷ��ź���֪ͨ��ȡ�߳�, ���ǵȵ�����������ʱ��֪ͨ, ����¼����ٵ�����¿��ܻ�����
             �¼��ϴ��ӳٽϾõ����, ���������ڵȴ��ź�����ʱ��, ��һ����ͼ��ȡ�¼���Ϣ, �ӿ���¼����ϴ�
             �ٶ�.
*********************************************************************************************************/
ssize_t  __monitorBufferGet (PVOID        pvMonitorBuffer,
                             PVOID        pvEvent,
                             size_t       stBufferSize,
                             ULONG        ulTimeout)
{
    INTREG           iregInterLevel;
    PMONITOR_BUFFER  pmb = (PMONITOR_BUFFER)pvMonitorBuffer;
    
    size_t           stSize;
    size_t           stLen;
    
    ULONG            ulError;
    BOOL             bTimeout = LW_FALSE;
    
    do {
        LW_SPIN_LOCK_QUICK(&pmb->MB_slLock, &iregInterLevel);
        stLen = pmb->MB_stSize - pmb->MB_stLeft;
        if (stLen) {
            break;
        }
        LW_SPIN_UNLOCK_QUICK(&pmb->MB_slLock, iregInterLevel);
        
        if (bTimeout) {
            return  (0);
        }
        
        ulError = API_SemaphoreBPend(pmb->MB_hReadSync, ulTimeout);
        if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
            bTimeout = LW_TRUE;
        
        } else if (ulError) {
            return  (PX_ERROR);
        }
    } while (1);
    
    {
        UCHAR   ucHigh = pmb->MB_pucGet[0];
        UCHAR   ucLow;
        
        if ((pmb->MB_pucGet - pmb->MB_pucBuffer) >= pmb->MB_stSize) {
            ucLow = pmb->MB_pucBuffer[0];
        } else {
            ucLow = pmb->MB_pucGet[1];
        }
        
        stSize = (size_t)((ucHigh << 8) + ucLow);
    }
    
    if (stSize > stBufferSize) {
        LW_SPIN_UNLOCK_QUICK(&pmb->MB_slLock, iregInterLevel);
        _ErrorHandle(ERROR_MONITOR_EMSGSIZE);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "message too big.\r\n");
        return  (0);
    }
    
    stLen = (pmb->MB_pucBuffer + pmb->MB_stSize) - pmb->MB_pucGet;
    if (stLen >= stSize) {
        lib_memcpy(pvEvent, pmb->MB_pucGet, stSize);
        
        if (stLen > stSize) {
            pmb->MB_pucGet += stSize;
        
        } else {
            pmb->MB_pucGet = pmb->MB_pucBuffer;
        }
        
    } else {
        lib_memcpy(pvEvent, pmb->MB_pucGet, stLen);
        pmb->MB_pucGet  = pmb->MB_pucBuffer;
        
        lib_memcpy((PUCHAR)pvEvent + stLen, pmb->MB_pucGet, stSize - stLen);
        pmb->MB_pucGet += (stSize - stLen);
    }
    
    pmb->MB_stLeft += stSize;
    
    LW_SPIN_UNLOCK_QUICK(&pmb->MB_slLock, iregInterLevel);

    return  ((ssize_t)stSize);
}
/*********************************************************************************************************
** ��������: __monitorBufferFlush
** ��������: ��ռ�ظ��ٽڵ��¼�������
** �䡡��  : pvMonitorBuffer   �¼�������
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __monitorBufferFlush (PVOID  pvMonitorBuffer)
{
    INTREG           iregInterLevel;
    PMONITOR_BUFFER  pmb = (PMONITOR_BUFFER)pvMonitorBuffer;

    API_SemaphoreBClear(pmb->MB_hReadSync);
    
    LW_SPIN_LOCK_QUICK(&pmb->MB_slLock, &iregInterLevel);
    
    pmb->MB_stLeft = pmb->MB_stSize;
    pmb->MB_pucPut = pmb->MB_pucBuffer;
    pmb->MB_pucGet = pmb->MB_pucBuffer;
    
    LW_SPIN_UNLOCK_QUICK(&pmb->MB_slLock, iregInterLevel);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __monitorBufferStatus
** ��������: ��ȡ��ظ��ٽڵ��¼�������״̬
** �䡡��  : pvMonitorBuffer   �¼�������
**           pstLeft           ʣ��ռ��С
**           pstNextSize       ��һ����Ч�¼�����
** �䡡��  : ERROR or NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __monitorBufferStatus (PVOID  pvMonitorBuffer,
                              size_t *pstLeft,
                              size_t *pstNextSize)
{
    INTREG           iregInterLevel;
    PMONITOR_BUFFER  pmb = (PMONITOR_BUFFER)pvMonitorBuffer;
    
    LW_SPIN_LOCK_QUICK(&pmb->MB_slLock, &iregInterLevel);
    
    if (pstLeft) {
        *pstLeft = pmb->MB_stLeft;
    }
    
    if (pstNextSize) {
        if (pmb->MB_stLeft != pmb->MB_stSize) {
            UCHAR   ucHigh = pmb->MB_pucGet[0];
            UCHAR   ucLow;
            
            if ((pmb->MB_pucGet - pmb->MB_pucBuffer) >= pmb->MB_stSize) {
                ucLow = pmb->MB_pucBuffer[0];
            } else {
                ucLow = pmb->MB_pucGet[1];
            }
            
            *pstNextSize = (size_t)((ucHigh << 8) + ucLow);
        
        } else {
            *pstNextSize = 0;
        }
    }
    
    LW_SPIN_UNLOCK_QUICK(&pmb->MB_slLock, iregInterLevel);

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
