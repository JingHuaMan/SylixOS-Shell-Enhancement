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
** ��   ��   ��: lwip_fix.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 05 �� 06 ��
**
** ��        ��: lwip SylixOS ��ֲ. ע��: ������ֲ��ĺ���ֻ������ lwip �ڲ�ʹ��!

** BUG:
2009.05.20  �����߳�Ӧ�þ��а�ȫ����.
2009.05.21  �����Ķ̳�ʱ�Ĵ���.
2009.05.27  ϵͳ֧�� read abort ����.
2009.05.28  timeout ���ʹ�� ptcb ��Ϊ����.
2009.06.04  �� sys_mbox_post �������ʱ, ��Ҫ��ӡ log.
2009.06.19  �ڴ����������ʱ, ��Ҫ��ӡ _DebugHandle().
2009.07.04  �� fprintf(stderr, ...); һЩ��������ֱ���� panic ��� (Ŀǰ����û������).
2009.07.28  sio_open() ʱ�ļ��ȴ�ʱ���ʼ��Ϊ���޳�.
2009.08.26  ����ͳ����Ϣ����.
2009.08.29  ���� sio_tryread() ��֧�� lwip 1.4.0
2009.09.04  ʹ�ö�ֵ�ź���, �����ź��������������!
            lwip Ӧ�ò�ʹ����Ϣ����, ����Ҫ����ĺ���ͨ����Ϣ�ķ�ʽ���ݵ� tcpip ����, Ȼ��ȴ����(�ź���)
            ����ĳЩ����������ź����в���, ֱ�Ӿͻ᷵��, ��Ϊ��Ϣ�Ǿֲ�����, ���� tcpip ���ȼ�����Ӧ��ʱ,
            �յ���Ϣʱ, ��Ϣ�Ѿ���ȫ����...
2009.10.29  sys_now �������������.
2009.11.23  timeout ���ƿ���� lwip �ڴ�����ȡ.
2010.01.04  lwip 1.4.0 ���ϰ汾ʵ�����Լ��� timeout ��, ������ҪΪÿ���߳��ṩ timeouts.
2010.02.22  ���� lwip ʵ���µ� mutex ���ƺ��µĲ���.
2010.04.12  ����ͳһ�� ip_input_hook() ����.
2011.06.18  sys_now() ֱ��ʹ�� 64 λϵͳʱ��.
2011.06.23  ���� aodv_ip_input_hook() ����.
2012.08.23  ���� ppp_link_status_hook(), ppp ��������ֱ��ʹ�ô˺�����Ϊ״̬�ص�.
2012.09.28  sys_mbox_post() ������� mbox ����, �ȴ������Ϊ 10ms.
            sys_mbox_post() ����һ���µķ�ʽ, ��������д����.
2013.04.23  sys_thread_new() ����ע��.
2013.09.04  �Ż�����˳�򲢼��� lwip ����ר�õ��������.
2014.07.01  SIO �������е��ļ�������Ϊ�ں��ļ�����������.
2016.07.21  ʹ��֧�ִ�������������Ϣ����, ������Ҫ����ͬ���ź���.
2020.05.10  ֧�� SEM_PER_THREAD ����ģʽ.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_PANIC
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "lwip_config.h"
#include "lwip_fix.h"
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/sio.h"
#include "lwip/netdb.h"
#include "lwip/stats.h"
#include "lwip/init.h"
#include "lwip/pbuf.h"
#include "netif/aodvif.h"
#if LW_CFG_NET_ROUTER > 0
#include "net/route.h"
#include "route/ip4_route.h"
#include "route/ip6_route.h"
#if LW_CFG_NET_BALANCING > 0
#include "net/sroute.h"
#include "balancing/ip4_sroute.h"
#include "balancing/ip6_sroute.h"
#endif                                                                  /*  LW_CFG_NET_BALANCING > 0    */
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */
#if PPP_SUPPORT > 0 || PPPOE_SUPPORT > 0
#include "net/if.h"
#include "netif/ppp/pppapi.h"
#endif                                                                  /*  PPP_SUPPORT > 0 ||          */
                                                                        /*  PPPOE_SUPPORT > 0           */
/*********************************************************************************************************
  �汾�ж� (����!!! �Ͱ汾 lwip ϵͳ������֧��)
*********************************************************************************************************/
#if __SYLIXOS_MAKEVER(LWIP_VERSION_MAJOR, LWIP_VERSION_MINOR, LWIP_VERSION_REVISION) <= __SYLIXOS_MAKEVER(1, 4, 0)
#error lwip version too old!
#endif                                                                  /*  LWIP 1.4.0 ���°汾         */
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_SPINLOCK_CA_DEFINE_CACHE_ALIGN(_G_slcaLwip);                  /*  ���������                  */
static LW_OBJECT_HANDLE                  _G_hTcpipMbox;
static UINT32                            _G_uiAppTryLost;
/*********************************************************************************************************
  net safe
*********************************************************************************************************/
#if LW_CFG_NET_SAFE > 0
#define LW_CFG_NET_SAFE_LAZY  1                                         /*  Lazy create test            */
#if LW_CFG_NET_SAFE_LAZY == 0
static void sys_thread_sem_init(LW_OBJECT_HANDLE  id);
#endif                                                                  /*  LW_CFG_NET_SAFE_LAZY == 0   */
static void sys_thread_sem_fini(LW_OBJECT_HANDLE  id);
#endif                                                                  /*  LW_CFG_NET_SAFE > 0         */
/*********************************************************************************************************
** ��������: sys_init
** ��������: ϵͳ�ӿڳ�ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_init (void)
{
    LW_SPIN_INIT(&_G_slcaLwip.SLCA_sl);                                 /*  ��ʼ������ؼ�����������    */

#if LW_CFG_NET_SAFE > 0
#if LW_CFG_NET_SAFE_LAZY == 0
    API_SystemHookAdd(sys_thread_sem_init, LW_OPTION_THREAD_CREATE_HOOK);
#endif                                                                  /*  LW_CFG_NET_SAFE_LAZY == 0   */
    API_SystemHookAdd(sys_thread_sem_fini, LW_OPTION_THREAD_DELETE_HOOK);
#endif                                                                  /*  LW_CFG_NET_SAFE > 0         */
}
/*********************************************************************************************************
** ��������: sys_arch_protect
** ��������: ϵͳ�ӿ� SYS_ARCH_PROTECT
** �䡡��  : pireg     �жϵȼ�״̬
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_arch_protect (INTREG  *pireg)
{
    LW_SPIN_LOCK_QUICK(&_G_slcaLwip.SLCA_sl, pireg);
}
/*********************************************************************************************************
** ��������: sys_arch_unprotect
** ��������: ϵͳ�ӿ� SYS_ARCH_UNPROTECT
** �䡡��  : ireg      �жϵȼ�״̬
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_arch_unprotect (INTREG  ireg)
{
    LW_SPIN_UNLOCK_QUICK(&_G_slcaLwip.SLCA_sl, ireg);
}
/*********************************************************************************************************
** ��������: sys_assert_print
** ��������: ϵͳ����ʧ�ܴ�ӡ
** �䡡��  : msg       ʧ����Ϣ
**           func      ��������
**           file      �����ļ�
**           line      �����б�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_assert_print (const char *msg, const char *func, const char *file, int line)
{
    char  *fname;
    
    _PathLastName(file, &fname);                                        /*  ֻ��ʾ�ļ���                */
    
    _PrintFormat("[NET] Assert: %s func: %s() file: %s line: %d\r\n", msg, func, fname, line);
}
/*********************************************************************************************************
** ��������: sys_error_print
** ��������: ϵͳ�����ӡ
** �䡡��  : msg       ʧ����Ϣ
**           func      ��������
**           file      �����ļ�
**           line      �����б�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_error_print (const char *msg, const char *func, const char *file, int line)
{
    char  *fname;
    
    _PathLastName(file, &fname);                                        /*  ֻ��ʾ�ļ���                */
    
    _PrintFormat("[NET] Error: %s func: %s() file: %s line: %d\r\n", msg, func, fname, line);
}
/*********************************************************************************************************
** ��������: lwip_platform_memcpy
** ��������: lwip ר���ڴ濽��
** �䡡��  : pvDest    Ŀ��
**           pvSrc     Դ
**           stCount   ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
LW_WEAK PVOID  lwip_platform_memcpy (PVOID  pvDest, CPVOID  pvSrc, size_t  stCount)
{
    return  (lib_memcpy(pvDest, pvSrc, stCount));
}
/*********************************************************************************************************
** ��������: lwip_platform_smemcpy
** ��������: lwip ר�ð�ȫ�ڴ濽��
** �䡡��  : pvDest    Ŀ��
**           pvSrc     Դ
**           stCount   ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
LW_WEAK PVOID  lwip_platform_smemcpy (PVOID  pvDest, CPVOID  pvSrc, size_t  stCount)
{
    return  (lib_memcpy(pvDest, pvSrc, stCount));
}
/*********************************************************************************************************
** ��������: sys_mutex_new
** ��������: ����һ�� lwip ������
** �䡡��  : pmutex    �����Ļ�����
** �䡡��  : ������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
err_t  sys_mutex_new (sys_mutex_t *pmutex)
{
    SYS_ARCH_DECL_PROTECT(lev);
    LW_OBJECT_HANDLE    hMutex;
    ULONG               ulOpt = LW_OPTION_INHERIT_PRIORITY
                              | LW_OPTION_DELETE_SAFE
                              | LW_OPTION_OBJECT_DEBUG_UNPEND
                              | LW_OPTION_OBJECT_GLOBAL;
                   
    if (!LW_KERN_NET_LOCK_FIFO_GET()) {
        ulOpt |= LW_OPTION_WAIT_PRIORITY;
    }
    
    hMutex = API_SemaphoreMCreate("net_mutex", LW_PRIO_DEF_CEILING, ulOpt, LW_NULL);
    if (hMutex == LW_OBJECT_HANDLE_INVALID) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create net mutex.\r\n");
        SYS_STATS_INC(mutex.err);
        return  (ERR_MEM);
    
    } else {
        if (pmutex) {
            *pmutex = hMutex;
        }
        SYS_STATS_INC(mutex.used);
        
        SYS_ARCH_PROTECT(lev);
        if (lwip_stats.sys.mutex.used > lwip_stats.sys.mutex.max) {
            lwip_stats.sys.mutex.max = lwip_stats.sys.mutex.used;
        }
        SYS_ARCH_UNPROTECT(lev);
        return  (ERR_OK);
    }
}
/*********************************************************************************************************
** ��������: sys_mutex_free
** ��������: ɾ��һ�� lwip ������
** �䡡��  : pmutex      ������
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_mutex_free (sys_mutex_t *pmutex)
{
    API_SemaphoreMDelete(pmutex);
    SYS_STATS_DEC(mutex.used);
}
/*********************************************************************************************************
** ��������: sys_mutex_lock
** ��������: ����һ�� lwip ������
** �䡡��  : pmutex      ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_mutex_lock (sys_mutex_t  *pmutex)
{
    if (pmutex) {
        API_SemaphoreMPend(*pmutex, LW_OPTION_WAIT_INFINITE);
    }
}
/*********************************************************************************************************
** ��������: sys_mutex_unlock
** ��������: ����һ�� lwip ������
** �䡡��  : pmutex      ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_mutex_unlock (sys_mutex_t  *pmutex)
{
    if (pmutex) {
        API_SemaphoreMPost(*pmutex);
    }
}
/*********************************************************************************************************
** ��������: sys_mutex_valid
** ��������: ���һ�� lwip �������Ƿ���Ч
** �䡡��  : pmutex      ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int  sys_mutex_valid (sys_mutex_t  *pmutex)
{
    if (pmutex) {
        if (*pmutex) {
            return  (1);
        }
    }
    
    return  (0);
}
/*********************************************************************************************************
** ��������: sys_mutex_set_invalid
** ��������: ��һ�� lwip ����������Ϊ��Ч
** �䡡��  : pmutex      ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_mutex_set_invalid (sys_mutex_t *pmutex)
{
    if (pmutex) {
        *pmutex = SYS_MUTEX_NULL;
    }
}
/*********************************************************************************************************
** ��������: sys_sem_new
** ��������: ����һ�� lwip �ź���
** �䡡��  : psem      �������ź���
**           count     ��ʼ����ֵ
** �䡡��  : ������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
err_t  sys_sem_new (sys_sem_t  *psem, u8_t  count)
{
    SYS_ARCH_DECL_PROTECT(lev);
    LW_OBJECT_HANDLE    hSemaphore = API_SemaphoreCCreate("net_sem", (ULONG)count, 0x1, 
                                                          LW_OPTION_WAIT_FIFO |
                                                          LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (hSemaphore == LW_OBJECT_HANDLE_INVALID) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create net sem.\r\n");
        SYS_STATS_INC(sem.err);
        return  (ERR_MEM);
    
    } else {
        if (psem) {
            *psem = hSemaphore;
        }
        SYS_STATS_INC(sem.used);
        
        SYS_ARCH_PROTECT(lev);
        if (lwip_stats.sys.sem.used > lwip_stats.sys.sem.max) {
            lwip_stats.sys.sem.max = lwip_stats.sys.sem.used;
        }
        SYS_ARCH_UNPROTECT(lev);
        return  (ERR_OK);
    }
}
/*********************************************************************************************************
** ��������: sys_sem_free
** ��������: ɾ��һ�� lwip �ź���
** �䡡��  : psem   �ź������ָ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_sem_free (sys_sem_t  *psem)
{
    API_SemaphoreCDelete(psem);
    SYS_STATS_DEC(sem.used);
}
/*********************************************************************************************************
** ��������: sys_sem_signal
** ��������: ����һ�� lwip �ź���
** �䡡��  : psem   �ź������ָ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_sem_signal (sys_sem_t *psem)
{
    if (psem) {
        API_SemaphoreCPost(*psem);
    }
}
/*********************************************************************************************************
** ��������: sys_arch_sem_wait
** ��������: �ȴ�һ�� lwip �ź���
** �䡡��  : psem   �ź������ָ��
** �䡡��  : �ȴ�ʱ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
u32_t  sys_arch_sem_wait (sys_sem_t *psem, u32_t timeout)
{
    ULONG       ulError;
    ULONG       ulTimeout = (ULONG)((timeout * LW_TICK_HZ) / 1000);
                                                                        /*  תΪ TICK ��                */
    ULONG       ulOldTime;
    ULONG       ulNowTime;
    
    if (psem == LW_NULL) {
        return  (SYS_ARCH_TIMEOUT);
    }
    
    __KERNEL_TIME_GET(ulOldTime, ULONG);
    
    if (timeout == 0) {
        ulTimeout =  LW_OPTION_WAIT_INFINITE;
    
    } else if (ulTimeout == 0) {
        ulTimeout = 1;                                                  /*  ������Ҫһ����������        */
    }
    
    ulError = API_SemaphoreCPend(*psem, ulTimeout);
    if (ulError) {
        return  (SYS_ARCH_TIMEOUT);
    
    } else {
        __KERNEL_TIME_GET(ulNowTime, ULONG);
        
        ulNowTime = (ulNowTime >= ulOldTime) 
                  ? (ulNowTime -  ulOldTime) 
                  : (__ARCH_ULONG_MAX - ulOldTime + ulNowTime + 1);     /*  ���� TICK ʱ��              */
    
        timeout = (u32_t)((ulNowTime * 1000) / LW_TICK_HZ);             /*  תΪ������                  */
        
        return  (timeout);
    }
}
/*********************************************************************************************************
** ��������: sys_sem_valid
** ��������: ��� lwip �ź����������Ƿ� > 0
** �䡡��  : psem   �ź������ָ��
** �䡡��  : 1: ��Ч 0: ��Ч
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int  sys_sem_valid (sys_sem_t *psem)
{
    if (psem) {
        if (*psem) {
            return  (1);
        }
    }
    
    return  (0);
}
/*********************************************************************************************************
** ��������: sys_sem_set_invalid
** ��������: ��� lwip �ź���������.
** �䡡��  : psem   �ź������ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_sem_set_invalid (sys_sem_t *psem)
{
    if (psem) {
        *psem = SYS_SEM_NULL;
    }
}
/*********************************************************************************************************
** ��������: sys_mbox_new
** ��������: ����һ�� lwip ͨ������
** �䡡��  : pmbox     ��Ҫ�����������
**           size      ��С(����)
** �䡡��  : ������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
err_t  sys_mbox_new (sys_mbox_t *pmbox, INT  size)
{
    ULONG               ulMsgCnt;
    LW_OBJECT_HANDLE    hMsgQueue;

    SYS_ARCH_DECL_PROTECT(lev);
    
    if (size > 0) {
        ulMsgCnt = size;
    
    } else {
        ulMsgCnt = LWIP_MSGQUEUE_SIZE;
    }
    
    hMsgQueue = API_MsgQueueCreate("net_msg", 
                                   ulMsgCnt, 
                                   sizeof(PVOID), 
                                   LW_OPTION_WAIT_FIFO |
                                   LW_OPTION_OBJECT_GLOBAL,
                                   LW_NULL);
    if (hMsgQueue == LW_OBJECT_HANDLE_INVALID) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create net msgqueue.\r\n");
        SYS_STATS_INC(mbox.err);
        return  (ERR_MEM);
    
    } else {
        if (pmbox) {
            *pmbox = hMsgQueue;
        }
        SYS_STATS_INC(mbox.used);
        
        SYS_ARCH_PROTECT(lev);
        if (lwip_stats.sys.mbox.used > lwip_stats.sys.mbox.max) {
            lwip_stats.sys.mbox.max = lwip_stats.sys.mbox.used;
        }
        SYS_ARCH_UNPROTECT(lev);
        
        if (TCPIP_MBOX_SIZE == size) {
            _G_hTcpipMbox = hMsgQueue;
        }
        return  (ERR_OK);
    }
}
/*********************************************************************************************************
** ��������: sys_mbox_free
** ��������: �ͷ�һ�� lwip ͨ������
** �䡡��  : pmbox  ������ָ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_mbox_free (sys_mbox_t *pmbox)
{
    if (*pmbox) {
        API_MsgQueueDelete(pmbox);
        SYS_STATS_DEC(mbox.used);
    }
}
/*********************************************************************************************************
** ��������: sys_mbox_post
** ��������: ����һ��������Ϣ, һ����֤�ɹ�
** �䡡��  : pmbox  ������ָ��
**           msg    ��Ϣ
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_mbox_post (sys_mbox_t *pmbox, void *msg)
{
    ULONG   ulError;

    if (pmbox == LW_NULL) {
        return;
    }
    
    ulError = API_MsgQueueSend2(*pmbox, &msg, sizeof(PVOID), LW_OPTION_WAIT_INFINITE);
    if (ulError) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "mbox send error: %s.\r\n", lib_strerror((INT)ulError));
    }
}
/*********************************************************************************************************
** ��������: sys_mbox_post_prio
** ��������: ����һ��������Ϣ, һ����֤�ɹ�
** �䡡��  : pmbox  ������ָ��
**           msg    ��Ϣ
**           prio   7 high ~ 0 low
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_mbox_post_prio (sys_mbox_t *pmbox, void *msg, u8_t prio)
{
    ULONG   ulError;
    ULONG   ulOption;

    if (pmbox == LW_NULL) {
        return;
    }
    
    if (prio) {
        prio     = (prio > 7) ? (0) : (7 - prio);
        ulOption = LW_OPTION_URGENT | (ULONG)(prio << 4);
        ulError = API_MsgQueueSendEx2(*pmbox, &msg, sizeof(PVOID), LW_OPTION_WAIT_INFINITE, ulOption);
        
    } else {
        ulError = API_MsgQueueSend2(*pmbox, &msg, sizeof(PVOID), LW_OPTION_WAIT_INFINITE);
    }
    
    if (ulError) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "mbox send error: %s.\r\n", lib_strerror((INT)ulError));
    }
}
/*********************************************************************************************************
** ��������: sys_mbox_trypost
** ��������: ����һ��������Ϣ(�����˳�)
** �䡡��  : pmbox  ������ָ��
**           msg    ��Ϣ
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
err_t  sys_mbox_trypost (sys_mbox_t *pmbox, void *msg)
{
    ULONG   ulError;
    
    if (pmbox == LW_NULL) {
        return  (ERR_MEM);
    }
    
    ulError = API_MsgQueueSend(*pmbox, &msg, sizeof(PVOID));
    if (ulError == ERROR_NONE) {                                        /*  ���ͳɹ�                    */
        return  (ERR_OK);
    }

#if LW_CFG_LWIP_DEBUG > 0
      else if (ulError != ERROR_MSGQUEUE_FULL) {
        panic("[NET] Panic: sys_mbox_trypost() msgqueue error: %s\n", lib_strerror(errno));
        return  (ERR_MEM);
    }
#endif                                                                  /*  LW_CFG_LWIP_DEBUG > 0       */
    
      else {
        if (*pmbox != _G_hTcpipMbox) {
            _G_uiAppTryLost++;
        }
        return  (ERR_MEM);
    }
}
/*********************************************************************************************************
** ��������: sys_mbox_trypost_prio
** ��������: ������Ϣ���ȼ�����һ��������Ϣ(�����˳�)
** �䡡��  : pmbox  ������ָ��
**           msg    ��Ϣ
**           prio   7 high ~ 0 low
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_LWIP_IPQOS > 0

err_t  sys_mbox_trypost_prio (sys_mbox_t *pmbox, void *msg, u8_t prio)
{
    ULONG   ulError;
    ULONG   ulOption;
    
    if (pmbox == LW_NULL) {
        return  (ERR_MEM);
    }
    
    if (prio) {
        prio     = (prio > 7) ? (0) : (7 - prio);
        ulOption = LW_OPTION_URGENT | (ULONG)(prio << 4);
        ulError  = API_MsgQueueSendEx(*pmbox, &msg, sizeof(PVOID), ulOption);
    
    } else {
        ulError = API_MsgQueueSend(*pmbox, &msg, sizeof(PVOID));
    }
    
    if (ulError == ERROR_NONE) {                                        /*  ���ͳɹ�                    */
        return  (ERR_OK);
    
    }

#if LW_CFG_LWIP_DEBUG > 0
      else if (ulError != ERROR_MSGQUEUE_FULL) {
        panic("[NET] Panic: sys_mbox_trypost_prio() msgqueue error: %s\n", lib_strerror(errno));
        return  (ERR_MEM);
    }
#endif                                                                  /*  LW_CFG_LWIP_DEBUG > 0       */
    
      else {
        if (*pmbox != _G_hTcpipMbox) {
            _G_uiAppTryLost++;
        }
        return  (ERR_MEM);
    }
}

#endif                                                                  /*  LW_CFG_LWIP_IPQOS > 0       */
/*********************************************************************************************************
** ��������: sys_mbox_trypost_fromisr
** ��������: �ж��з���һ��������Ϣ(�����˳�)
** �䡡��  : pmbox  ������ָ��
**           msg    ��Ϣ
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
err_t sys_mbox_trypost_fromisr (sys_mbox_t *pmbox, void *msg)
{
    ULONG   ulError;
    
    if (pmbox == LW_NULL) {
        return  (ERR_MEM);
    }
    
    ulError = API_MsgQueueSend(*pmbox, &msg, sizeof(PVOID));
    if (ulError == ERROR_NONE) {                                        /*  ���ͳɹ�                    */
        return  (ERR_OK);
    }
    
    return  (ERR_MEM);
}
/*********************************************************************************************************
** ��������: sys_mbox_trypost_stat
** ��������: ��ȡӦ�ò� trypost ��ʧ����
** �䡡��  : NONE
** �䡡��  : Ӧ�ò� trypost ��ʧ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
u32_t sys_mbox_trypost_stat (void)
{
    return  (_G_uiAppTryLost);
}
/*********************************************************************************************************
** ��������: sys_arch_mbox_fetch
** ��������: ����һ��������Ϣ
** �䡡��  : pmbox  ������ָ��
**           msg        ��Ϣ
**           timeout    ��ʱʱ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
u32_t   sys_arch_mbox_fetch (sys_mbox_t *pmbox, void  **msg, u32_t  timeout)
{
    ULONG       ulError;
    ULONG       ulTimeout = (ULONG)((timeout * LW_TICK_HZ) / 1000);
                                                                        /*  תΪ TICK ��                */
    size_t      stMsgLen  = 0;
    PVOID       pvMsg;
    
    ULONG       ulOldTime;
    ULONG       ulNowTime;
    
    if (pmbox == LW_NULL) {
        if (msg) {
            *msg = LW_NULL;                                             /*  No Message                  */
        }
        return  (SYS_ARCH_TIMEOUT);
    }
    
    __KERNEL_TIME_GET(ulOldTime, ULONG);
    
    if (timeout == 0) {
        ulTimeout =  LW_OPTION_WAIT_INFINITE;
    
    } else if (ulTimeout == 0) {
        ulTimeout = 1;                                                  /*  ������Ҫһ����������        */
    }
    
    ulError = API_MsgQueueReceive(*pmbox, &pvMsg, sizeof(PVOID), &stMsgLen, ulTimeout);
    if (ulError) {
        if (msg) {
            *msg = LW_NULL;                                             /*  No Message                  */
        }
        return  (SYS_ARCH_TIMEOUT);
    
    } else {
        __KERNEL_TIME_GET(ulNowTime, ULONG);
        
        ulNowTime = (ulNowTime >= ulOldTime) 
                  ? (ulNowTime -  ulOldTime) 
                  : (__ARCH_ULONG_MAX - ulOldTime + ulNowTime + 1);     /*  ���� TICK ʱ��              */
    
        timeout = (u32_t)((ulNowTime * 1000) / LW_TICK_HZ);             /*  תΪ������                  */
        if (msg) {
            *msg = pvMsg;                                               /*  ��Ҫ������Ϣ                */
        }
        
        return  (timeout);
    }
}
/*********************************************************************************************************
** ��������: sys_arch_mbox_tryfetch
** ��������: ����������һ��������Ϣ
** �䡡��  : pmbox  ������ָ��
**           msg    ��Ϣ
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
u32_t   sys_arch_mbox_tryfetch (sys_mbox_t *pmbox, void  **msg)
{
    ULONG       ulError;
    size_t      stMsgLen = 0;
    PVOID       pvMsg;
    
    if (pmbox == LW_NULL) {
        return  (SYS_MBOX_EMPTY);
    }
    
    ulError = API_MsgQueueTryReceive(*pmbox, &pvMsg, sizeof(PVOID), &stMsgLen);
    if (ulError) {
        return  (SYS_MBOX_EMPTY);
    
    } else {
        if (msg) {
            *msg = pvMsg;                                               /*  ��Ҫ������Ϣ                */
        }
        
        return  (ERR_OK);
    }
}
/*********************************************************************************************************
** ��������: sys_mbox_valid
** ��������: ��� lwip �����Ƿ���Ч
** �䡡��  : pmbox    ������ָ��
** �䡡��  : 1: ��Ч 0: ��Ч
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int  sys_mbox_valid (sys_mbox_t *pmbox)
{
    if (pmbox) {
        if (*pmbox) {
            return  (1);
        }
    }
    
    return  (0);
}
/*********************************************************************************************************
** ��������: sys_mbox_set_invalid
** ��������: ��� lwip ���������е���Ϣ
** �䡡��  : pmbox    ������ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_mbox_set_invalid (sys_mbox_t *pmbox)
{
    if (pmbox) {
        *pmbox = SYS_MBOX_NULL;
    }
}
/*********************************************************************************************************
  thread
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: sys_thread_hostent
** ��������: ���̰߳�ȫ�ķ���һ�� hostent �Ŀ���
** �䡡��  : phostent      hostent ��Ϣ
** �䡡��  : �̰߳�ȫ�Ŀ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_DNS

struct hostent  *sys_thread_hostent (struct hostent  *phostent)
{
#define __LW_MAX_HOSTENT    20

    static ip_addr_t        ipaddrBuffer[__LW_MAX_HOSTENT];
    static ip_addr_t       *pipaddrBuffer[__LW_MAX_HOSTENT];
    static struct hostent   hostentBuffer[__LW_MAX_HOSTENT];
    static int              iIndex    = 0;
    static char            *pcAliases = LW_NULL;
    
           ip_addr_t      **ppipaddrBuffer = &pipaddrBuffer[iIndex];
           struct hostent  *phostentRet    = &hostentBuffer[iIndex];


    pipaddrBuffer[iIndex] = &ipaddrBuffer[iIndex];
    ipaddrBuffer[iIndex]  = (*(ip_addr_t *)phostent->h_addr);
    
    iIndex++;
    if (iIndex >= __LW_MAX_HOSTENT) {
        iIndex =  0;
    }
    
    phostentRet->h_name      = phostent->h_name;                        /*  h_name ����Ҫ����           */
    phostentRet->h_aliases   = &pcAliases;                              /*  LW_NULL                     */
    phostentRet->h_addrtype  = phostent->h_addrtype;                    /*  AF_INET                     */
    phostentRet->h_length    = phostent->h_length;                      /*  sizeof(struct ip_addr)      */
    phostentRet->h_addr_list = (char **)ppipaddrBuffer;                 /*  ip address                  */

    return  (phostentRet);
}

#endif                                                                  /*  LWIP_DNS                    */
/*********************************************************************************************************
** ��������: sys_thread_new
** ��������: ����һ���߳�
** �䡡��  : name           �߳���
**           thread         �߳�ָ��
**           arg            ��ڲ���
**           stacksize      ��ջ��С
**           prio           ���ȼ�
** �䡡��  : �߳̾��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
sys_thread_t  sys_thread_new (const char *name, lwip_thread_fn thread, 
                              void *arg, int  stacksize, int prio)
{
    LW_CLASS_THREADATTR     threadattr;
    LW_OBJECT_HANDLE        hThread;
    
    if (stacksize < LW_CFG_LWIP_DEF_STK_SIZE) {
        stacksize = LW_CFG_LWIP_DEF_STK_SIZE;                           /*  ��С��ջ                    */
    }
    if (prio < LW_PRIO_T_NETPROTO) {
        prio = LW_PRIO_T_NETPROTO;                                      /*  ���ȼ�����������Э��ջ      */
    }
    if (prio > LW_PRIO_LOW) {
        prio = LW_PRIO_LOW;
    }
    
    API_ThreadAttrBuild(&threadattr,
                        stacksize,
                        (UINT8)prio,
                        (LW_OPTION_THREAD_STK_CHK |
                         LW_OPTION_THREAD_SAFE |
                         LW_OPTION_OBJECT_GLOBAL |
                         LW_OPTION_THREAD_DETACHED),
                        arg);
                                                   
    hThread = API_ThreadInit(name, (PTHREAD_START_ROUTINE)thread, &threadattr, LW_NULL);
    if (hThread == LW_OBJECT_HANDLE_INVALID) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create lwip thread.\r\n");
        return  (hThread);
    }
    
    /*
     *  ������Լ���һЩ����Ĵ���...
     */
    API_ThreadStart(hThread);

    return  (hThread);
}
/*********************************************************************************************************
  net safe
*********************************************************************************************************/
#if LW_CFG_NET_SAFE > 0
/*********************************************************************************************************
** ��������: sys_thread_sem_get
** ��������: ��ȡ��ǰ���� sem
** �䡡��  : NONE
** �䡡��  : �ź���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
sys_sem_t *sys_thread_sem_get (void)
{
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR_SAFE(ptcbCur);

#if LW_CFG_NET_SAFE_LAZY > 0
    if (ptcbCur->TCB_ulNetSem == LW_OBJECT_HANDLE_INVALID) {
        ptcbCur->TCB_ulNetSem = API_SemaphoreCCreate("net_tsem", 0, 1,
                                                     LW_OPTION_WAIT_FIFO |
                                                     LW_OPTION_OBJECT_GLOBAL, LW_NULL);
        if (ptcbCur->TCB_ulNetSem == LW_OBJECT_HANDLE_INVALID) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create net tsem.\r\n");
        }
    }
#endif

    return  (&ptcbCur->TCB_ulNetSem);
}
/*********************************************************************************************************
** ��������: sys_thread_sem_init
** ��������: �������� sem
** �䡡��  : NONE
** �䡡��  : ϵͳʱ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_NET_SAFE_LAZY == 0

static void sys_thread_sem_init (LW_OBJECT_HANDLE  id)
{
    PLW_CLASS_TCB   ptcb = __GET_TCB_FROM_INDEX(_ObjectGetIndex(id));

    if (ptcb->TCB_ulNetSem == LW_OBJECT_HANDLE_INVALID) {
        ptcb->TCB_ulNetSem = API_SemaphoreCCreate("net_tsem", 0, 1,
                                                  LW_OPTION_WAIT_FIFO |
                                                  LW_OPTION_OBJECT_GLOBAL, LW_NULL);
        if (ptcb->TCB_ulNetSem == LW_OBJECT_HANDLE_INVALID) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create net tsem.\r\n");
        }
    }
}

#endif                                                                  /*  LW_CFG_NET_SAFE_LAZY == 0   */
/*********************************************************************************************************
** ��������: sys_thread_sem_fini
** ��������: �ͷ����� sem
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static void sys_thread_sem_fini (LW_OBJECT_HANDLE  id)
{
    PLW_CLASS_TCB   ptcb = __GET_TCB_FROM_INDEX(_ObjectGetIndex(id));

    if (ptcb->TCB_ulNetSem) {
        API_SemaphoreCDelete(&ptcb->TCB_ulNetSem);
    }
}

#endif                                                                  /*  LW_CFG_NET_SAFE             */
/*********************************************************************************************************
  time
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: sys_jiffies
** ��������: ����ϵͳʱ��
** �䡡��  : NONE
** �䡡��  : ϵͳʱ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
u32_t  sys_jiffies (void)
{
    return  ((u32_t)API_TimeGet());
}
/*********************************************************************************************************
** ��������: sys_arch_msleep
** ��������: �ӳ� ms 
** �䡡��  : ms  ��Ҫ�ӳٵ� ms
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_arch_msleep (u32_t ms)
{
    API_TimeMSleep((ULONG)ms);
}
/*********************************************************************************************************
** ��������: sys_now
** ��������: ���ص�ǰʱ�� (��λ : ms) 
** �䡡��  : NONE
** �䡡��  : ��ǰʱ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
u32_t  sys_now (void)
{
    UINT64  ullTemp = (UINT64)API_TimeGet64();                          /*  ʹ�� 64 λ, �������        */
    
    ullTemp = ((ullTemp * 1000) / LW_TICK_HZ);                          /*  �任Ϊ�������              */
    
    return  ((u32_t)(ullTemp % ((u32_t)~0)));                           /*  ����                        */
}
/*********************************************************************************************************
  PPP/SLIP
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: sio_open
** ��������: sylixos sio open
** �䡡��  : port      COM �˿�
** �䡡��  : �ļ�������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
sio_fd_t  sio_open (u8_t  port)
{
    CHAR    cNameBuffer[64];
    int     iFd;
    
    snprintf(cNameBuffer, sizeof(cNameBuffer), 
             "%s%d", LWIP_SYLIXOS_TTY_NAME, port);                      /*  �ϳ��ļ���                  */
    
    __KERNEL_SPACE_ENTER();                                             /*  �ں��ļ�������              */
    iFd = open(cNameBuffer, O_RDWR);
    if (iFd < 0) {
        __KERNEL_SPACE_EXIT();
        fprintf(stderr, "sio_open() can not open file : \"%s\".\n", cNameBuffer);
        return  (0);                                                    /*  �޷����ļ�                */
    }
    ioctl(iFd, FIOOPTIONS, OPT_RAW);                                    /*  ����ԭʼģʽ                */
    ioctl(iFd, FIORTIMEOUT, LW_NULL);                                   /*  ���޳��ȴ�ʱ��              */
    __KERNEL_SPACE_EXIT();
    
    return  ((sio_fd_t)(long)iFd);
}
/*********************************************************************************************************
** ��������: sio_send
** ��������: sylixos sio send
** �䡡��  : data      ����
**           fd        �ļ�������
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sio_send (u8_t  data, sio_fd_t  fd)
{
    __KERNEL_SPACE_ENTER();                                             /*  �ں��ļ�������              */
    write((int)(long)fd, (const void *)&data, 1);
    __KERNEL_SPACE_EXIT();
}
/*********************************************************************************************************
** ��������: sio_recv
** ��������: sylixos sio recv
** �䡡��  : fd        �ļ�������
** �䡡��  : ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
u8_t  sio_recv (sio_fd_t  fd)
{
    char  data;
    
    __KERNEL_SPACE_ENTER();                                             /*  �ں��ļ�������              */
    read((int)(long)fd, (void *)&data, 1);
    __KERNEL_SPACE_EXIT();
    
    return  ((u8_t)data);
}
/*********************************************************************************************************
** ��������: sio_read
** ��������: sylixos sio read
** �䡡��  : fd        �ļ�������
**           buffer    ������
**           num       ����
** �䡡��  : ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
u32_t  sio_read (sio_fd_t  fd, u8_t *buffer, u32_t  num)
{
    ssize_t  sstReadNum;
    
    __KERNEL_SPACE_ENTER();
    sstReadNum = read((int)(long)fd, (void *)buffer, (size_t)num);
    __KERNEL_SPACE_EXIT();

    if (sstReadNum < 0) {
        return  (0);
    
    } else {
        return  ((u32_t)sstReadNum);
    }
}
/*********************************************************************************************************
** ��������: sio_tryread
** ��������: sylixos sio try read (if no data is available and never blocks)
** �䡡��  : fd        �ļ�������
**           buffer    ������
**           num       ����
** �䡡��  : ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
u32_t  sio_tryread (sio_fd_t  fd, u8_t *buffer, u32_t  num)
{
    INT    iNRead = 0;
    u32_t  uiRead = 0;

    __KERNEL_SPACE_ENTER();
    if (ioctl((int)(long)fd, FIONREAD, &iNRead)) {
        __KERNEL_SPACE_EXIT();
        return  (0);
    }
    
    if (iNRead > 0) {
        uiRead = (u32_t)read((int)(long)fd, (void *)buffer, num);
    }
    __KERNEL_SPACE_EXIT();
    
    return  (uiRead);
}
/*********************************************************************************************************
** ��������: sio_write
** ��������: sylixos sio write
** �䡡��  : fd        �ļ�������
**           buffer    ������
**           num       ����
** �䡡��  : ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
u32_t  sio_write (sio_fd_t  fd, u8_t *buffer, u32_t  num)
{
    ssize_t  ssWriteNum;
    
    __KERNEL_SPACE_ENTER();
    ssWriteNum = write((int)(long)fd, (const void *)buffer, (size_t)num);
    __KERNEL_SPACE_EXIT();
    
    if (ssWriteNum < 0) {
        return  (0);
    
    } else {
        return  ((u32_t)ssWriteNum);
    }
}
/*********************************************************************************************************
** ��������: sio_read_abort
** ��������: sylixos sio read abort
** �䡡��  : fd        �ļ�������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sio_read_abort (sio_fd_t  fd)
{
    __KERNEL_SPACE_ENTER();
    ioctl((int)(long)fd, FIOWAITABORT, OPT_RABORT);                     /*  ���һ��������              */
    __KERNEL_SPACE_EXIT();
}
/*********************************************************************************************************
  ip extern 
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: ip_input_hook
** ��������: sylixos ip input hook
** �䡡��  : p        pbuf
**           pnetif   net interface
** �䡡��  : �Ƿ� eaten
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int ip_input_hook (struct pbuf *p, struct netif *pnetif)
{
#if LW_CFG_NET_LOGINBL_EN > 0
    extern INT  loginbl_input_hook(struct pbuf *p, struct netif *inp);

    return  (loginbl_input_hook(p, pnetif));
#else
    return  (0);                                                        /*  do not eaten packet         */
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
}
/*********************************************************************************************************
** ��������: ip_input_hook
** ��������: sylixos ipv6 input hook
** �䡡��  : p        pbuf
**           pnetif   net interface
** �䡡��  : �Ƿ� eaten
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int ip6_input_hook (struct pbuf *p, struct netif *pnetif)
{
    return  (0);                                                        /*  do not eaten packet         */
}
/*********************************************************************************************************
  User Route Hook
*********************************************************************************************************/
extern struct netif *lwip_ip_route_hook(int ip_type, const void *src, const void *dest);
/*********************************************************************************************************
** ��������: ip_route_src_hook
** ��������: sylixos ip route hook
** �䡡��  : pipsrc   source address
**           pipdest  destination route netif
** �䡡��  : netif
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
struct netif *ip_route_src_hook (const ip4_addr_t *pipsrc, const ip4_addr_t *pipdest)
{
#if LW_CFG_NET_ROUTER > 0
    struct netif  *netif;
    
    netif = lwip_ip_route_hook(4, pipsrc, pipdest);                     /*  IP_HOOK_V4 == 4            */
    if (netif) {
        return  (netif);
    }

#if LW_CFG_NET_BALANCING > 0
    netif = srt_route_search_hook(pipsrc, pipdest);                     /*  source route first          */
#else
    netif = LW_NULL;
#endif
    
    if (netif == LW_NULL) {
        netif = rt_route_search_hook(pipsrc, pipdest);
    }

    return  ((PVOID)netif);
    
#else
    return  (LW_NULL);
#endif                                                                  /*  LW_CFG_NET_ROUTER           */
}
/*********************************************************************************************************
** ��������: ip_route_default_hook
** ��������: sylixos ip route hook
** �䡡��  : pipsrc   source address
**           pipdest  destination route netif
** �䡡��  : netif
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
struct netif *ip_route_default_hook (const ip4_addr_t *pipsrc, const ip4_addr_t *pipdest)
{
#if LW_CFG_NET_ROUTER > 0
#if LW_CFG_NET_BALANCING > 0
    return  (srt_route_default_hook(pipsrc, pipdest));
#else
    return  (LW_NULL);
#endif

#else
    return  (LW_NULL);
#endif                                                                  /*  LW_CFG_NET_ROUTER           */
}
/*********************************************************************************************************
** ��������: ip_gw_hook
** ��������: sylixos ip route gw hook
** �䡡��  : pnetif  netif
**           pipdest destination
** �䡡��  : netif
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ip4_addr_t *ip_gw_hook (struct netif *pnetif, const ip4_addr_t *pipdest)
{
#if LW_CFG_NET_ROUTER > 0
    return  (rt_route_gateway_hook(pnetif, pipdest));
#else
    return  (LW_NULL);
#endif                                                                  /*  LW_CFG_NET_ROUTER           */
}
/*********************************************************************************************************
** ��������: ip6_route_src_hook
** ��������: sylixos ip route hook
** �䡡��  : pip6src     source address
**           pip6dest    destination route netif
** �䡡��  : netif
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6 > 0

struct netif *ip6_route_src_hook (const ip6_addr_t *pip6src, const ip6_addr_t *pip6dest)
{
#if LW_CFG_NET_ROUTER > 0
    struct netif  *netif;
    
    netif = lwip_ip_route_hook(6, pip6src, pip6dest);                   /*  IP_HOOK_V6 == 6            */
    if (netif) {
        return  (netif);
    }

#if LW_CFG_NET_BALANCING > 0
    netif = srt6_route_search_hook(pip6src, pip6dest);                  /*  source route first          */
#else
    netif = LW_NULL;
#endif
    
    if (netif == LW_NULL) {
        netif = rt6_route_search_hook(pip6src, pip6dest);
    }

    return  ((PVOID)netif);
    
#else
    return  (LW_NULL);
#endif                                                                  /*  LW_CFG_NET_ROUTER           */
}
/*********************************************************************************************************
** ��������: ip6_route_default_hook
** ��������: sylixos ip route hook
** �䡡��  : pip6src     source address
**           pip6dest    destination route netif
** �䡡��  : netif
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
struct netif *ip6_route_default_hook (const ip6_addr_t *pip6src, const ip6_addr_t *pip6dest)
{
#if LW_CFG_NET_ROUTER > 0
#if LW_CFG_NET_BALANCING > 0
    return  (srt6_route_default_hook(pip6src, pip6dest));
#else
    return  (LW_NULL);
#endif

#else
    return  (LW_NULL);
#endif                                                                  /*  LW_CFG_NET_ROUTER           */
}
/*********************************************************************************************************
** ��������: ip6_gw_hook
** ��������: sylixos ip6 route gw hook
** �䡡��  : dest  destination route netif
** �䡡��  : netif
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ip6_addr_t *ip6_gw_hook (struct netif *pnetif, const ip6_addr_t *pip6dest)
{
#if LW_CFG_NET_ROUTER > 0
    return  (rt6_route_gateway_hook(pnetif, pip6dest));
#else
    return  (LW_NULL);
#endif                                                                  /*  LW_CFG_NET_ROUTER           */
}

#endif                                                                  /*  LWIP_IPV6 > 0               */
/*********************************************************************************************************
** ��������: link_input_hook
** ��������: sylixos link input hook (û���� CORELOCK ����)
** �䡡��  : p             pbuf
**           pnetif        net interface
** �䡡��  : �Ƿ� eaten
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int link_input_hook (struct pbuf *p, struct netif *pnetif)
{
extern INT packet_link_input(struct pbuf *p, struct netif *inp, BOOL bOutgo);

    if (!netif_is_up(pnetif)) {
        return  (1);                                                    /*  û��ʹ�ܵ�����������        */
    }

    return  (packet_link_input(p, pnetif, LW_FALSE));
}
/*********************************************************************************************************
** ��������: link_output_hook
** ��������: sylixos link output hook (�� CORELOCK ����)
** �䡡��  : p             pbuf
**           pnetif        net interface
** �䡡��  : �Ƿ� eaten
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int link_output_hook (struct pbuf *p, struct netif *pnetif)
{
extern INT packet_link_input(struct pbuf *p, struct netif *inp, BOOL bOutgo);

    return  (packet_link_input(p, pnetif, LW_TRUE));
}
/*********************************************************************************************************
** ��������: htonl
** ��������: inet htonl
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
uint32_t htonl (uint32_t x)
{
#if BYTE_ORDER == LITTLE_ENDIAN
    return  PP_HTONL(x);
#else
    return  x;
#endif
}
/*********************************************************************************************************
** ��������: ntohl
** ��������: inet ntohl
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
uint32_t ntohl (uint32_t x)
{
#if BYTE_ORDER == LITTLE_ENDIAN
    return  PP_HTONL(x);
#else
    return  x;
#endif
}
/*********************************************************************************************************
** ��������: htons
** ��������: inet htons
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
uint16_t htons (uint16_t x)
{
#if BYTE_ORDER == LITTLE_ENDIAN
    return  PP_HTONS(x);
#else
    return  x;
#endif
}
/*********************************************************************************************************
** ��������: ntohs
** ��������: inet ntohs
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
uint16_t ntohs (uint16_t x)
{
#if BYTE_ORDER == LITTLE_ENDIAN
    return  PP_HTONS(x);
#else
    return  x;
#endif
}
/*********************************************************************************************************
** ��������: inet_addr
** ��������: ipaddr_addr(cp)
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
in_addr_t  inet_addr (const char *cp)
{
    return  (ipaddr_addr(cp));
}
/*********************************************************************************************************
** ��������: inet_aton
** ��������: ip4addr_aton(cp, (ip4_addr_t*)addr)
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
int  inet_aton (const char *cp, struct in_addr *addr)
{
    return  (ip4addr_aton(cp, (ip4_addr_t*)addr));
}
/*********************************************************************************************************
** ��������: inet_ntoa
** ��������: ip4addr_ntoa((const ip4_addr_t*)&(addr))
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
char  *inet_ntoa (struct in_addr addr)
{
    return  (ip4addr_ntoa((const ip4_addr_t*)&(addr)));
}
/*********************************************************************************************************
** ��������: inet_ntoa_r
** ��������: ip4addr_ntoa_r((const ip4_addr_t*)&(addr), buf, buflen)
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
char  *inet_ntoa_r (struct in_addr addr, char *buf, int buflen)
{
    return  (ip4addr_ntoa_r((const ip4_addr_t*)&(addr), buf, buflen));
}
/*********************************************************************************************************
** ��������: inet6_aton
** ��������: ip6addr_aton(cp, (ip6_addr_t*)addr)
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
#if LWIP_IPV6

int  inet6_aton (const char *cp, struct in6_addr *addr)
{
    return  (ip6addr_aton(cp, (ip6_addr_t*)addr));
}
/*********************************************************************************************************
** ��������: inet6_ntoa
** ��������: ip6addr_ntoa((const ip6_addr_t*)&(addr))
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
char  *inet6_ntoa (struct in6_addr addr)
{
    return  (ip6addr_ntoa((const ip6_addr_t*)&(addr)));
}
/*********************************************************************************************************
** ��������: inet6_ntoa_r
** ��������: ip6addr_ntoa_r((const ip6_addr_t*)&(addr), buf, buflen)
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
char  *inet6_ntoa_r (struct in6_addr addr, char *buf, int buflen)
{
    return  (ip6addr_ntoa_r((const ip6_addr_t*)&(addr), buf, buflen));
}

#endif                                                                  /*  LWIP_IPV6                   */
#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
