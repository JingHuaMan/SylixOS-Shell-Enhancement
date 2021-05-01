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
** ��   ��   ��: sysconf.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 12 �� 10 ��
**
** ��        ��: ���� posix sysconf ��. ��ȡ��ǰϵͳ������Ϣ.
**
** BUG:
2014.08.15  _SC_NPROCESSORS_ONLN ����Ϊ��ǰ���� CPU ����.
2015.05.25  ����� _SC_PHYS_PAGES _SC_AVPHYS_PAGES ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "unistd.h"
#include "limits.h"
#include "mqueue.h"
#include "sys/statvfs.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
/*********************************************************************************************************
** ��������: sysconf
** ��������: get configurable system variables
** �䡡��  : name          represents the system variable to be queried
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
long  sysconf (int name)
{
    switch (name) {
    
    case _SC_AIO_LISTIO_MAX:
        return  (__ARCH_LONG_MAX);
        
    case _SC_AIO_MAX:
        return  (__ARCH_LONG_MAX);
        
    case _SC_ASYNCHRONOUS_IO:
        return  (1);
        
    case _SC_ARG_MAX:
        return  (LW_CFG_SHELL_MAX_KEYWORDLEN);
        
    case _SC_CLK_TCK:
        return  (LW_TICK_HZ);
        
    case _SC_DELAYTIMER_MAX:
        return  (__ARCH_INT_MAX);
        
    case _SC_IOV_MAX:
        return  (__ARCH_LONG_MAX);
        
    case _SC_LINE_MAX:
        return  (LW_CFG_SHELL_MAX_COMMANDLEN);
    
    case _SC_CPUTIME:
        return  (CLOCK_PROCESS_CPUTIME_ID);
        
    case _SC_LOGIN_NAME_MAX:
        return  (PATH_MAX);
        
    case _SC_MONOTONIC_CLOCK:
        return  (CLOCK_MONOTONIC);
        
    case _SC_MQ_OPEN_MAX:
        return  (LW_CFG_MAX_EVENTS / 3);
        
    case _SC_MQ_PRIO_MAX:
        return  (MQ_PRIO_MAX);
        
    case _SC_OPEN_MAX:
        return  (LW_CFG_MAX_FILES);
        
    case _SC_PAGESIZE:
        return  (LW_CFG_VMM_PAGE_SIZE);
        
    case _SC_PRIORITY_SCHEDULING:
        return  (1);
        
#if LW_CFG_SIGNAL_EN > 0
    case _SC_RTSIG_MAX:
        return  (SIGRTMAX - SIGRTMIN);
#endif
        
    case _SC_REALTIME_SIGNALS:
        return  (1);
        
    case _SC_SEM_NSEMS_MAX:
        return  (LW_CFG_MAX_EVENTS);
        
    case _SC_SEM_VALUE_MAX:
        return  (__ARCH_UINT_MAX);
        
    case _SC_SEMAPHORES:
        return  (1);
        
    case _SC_SIGQUEUE_MAX:
        return  (LW_CFG_MAX_MSGQUEUES);
        
    case _SC_TIMER_MAX:
        return  (LW_CFG_MAX_TIMERS);
        
    case _SC_TIMERS:
        return  (1);
        
    case _SC_TZNAME_MAX:
        return  (10);                                                   /*  __TIMEZONE_BUFFER_SIZE      */
        
    case _SC_VERSION:
        return  (_POSIX_VERSION);
        
    case _SC_NPROCESSORS_CONF:
        return  (LW_NCPUS);
        
    case _SC_NPROCESSORS_ONLN:
#if LW_CFG_SMP_EN > 0
        {
            ULONG   i;
            int     iCnt = 0;
            LW_CPU_FOREACH_ACTIVE (i) {
                iCnt++;
            }
            return  (iCnt);
        }
#else
        return  (LW_NCPUS);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
        
    case _SC_THREAD_CPUTIME:
        return  (CLOCK_THREAD_CPUTIME_ID);
    
    case _SC_THREAD_DESTRUCTOR_ITERATIONS:
        return  (__ARCH_LONG_MAX);
        
    case _SC_THREAD_KEYS_MAX:
        return  (__ARCH_LONG_MAX);
        
    case _SC_THREAD_STACK_MIN:
        return  (LW_CFG_PTHREAD_DEFAULT_STK_SIZE);
        
    case _SC_THREAD_THREADS_MAX:
        return  (LW_CFG_MAX_THREADS);
        
    case _SC_THREADS:
        return  (1);
        
    case _SC_THREAD_ATTR_STACKADDR:
        return  (1);
        
    case _SC_THREAD_ATTR_STACKSIZE:
        return  (1);
        
    case _SC_THREAD_PRIORITY_SCHEDULING:
        return  (1);
        
    case _SC_THREAD_PRIO_INHERIT:
        return  (1);
        
    case _SC_THREAD_PRIO_PROTECT:
        return  (1);
        
    case _SC_TTY_NAME_MAX:
        return  (PATH_MAX);
        
    case _SC_SYNCHRONIZED_IO:
        return  (1);
        
    case _SC_FSYNC:
        return  (1);
        
    case _SC_MAPPED_FILES:
        return  (1);
        
#if LW_CFG_VMM_EN > 0
    case _SC_PHYS_PAGES:
        {
            INT     i, iPages = 0;
            size_t  stPhySize;
            LW_MMU_PHYSICAL_DESC  phydesc[2];

            for (i = 0; i < LW_CFG_VMM_ZONE_NUM; i++) {
                if (API_VmmZoneStatus((ULONG)i, LW_NULL, &stPhySize, 
                                      LW_NULL, LW_NULL, LW_NULL) == ERROR_NONE) {
                    iPages += (INT)(stPhySize >> LW_CFG_VMM_PAGE_SHIFT);
                }
            }
            API_VmmPhysicalKernelDesc(&phydesc[0], &phydesc[1]);
            iPages += (INT)(phydesc[0].PHYD_stSize >> LW_CFG_VMM_PAGE_SHIFT);
            iPages += (INT)(phydesc[1].PHYD_stSize >> LW_CFG_VMM_PAGE_SHIFT);
            return  (iPages);
        }
        
    case _SC_AVPHYS_PAGES:
        {
            INT     i, iFreePages = 0;
            ULONG   ulFreePages;
            for (i = 0; i < LW_CFG_VMM_ZONE_NUM; i++) {
                if (API_VmmZoneStatus((ULONG)i, LW_NULL, LW_NULL, 
                                      LW_NULL, &ulFreePages, LW_NULL) == ERROR_NONE) {
                    iFreePages += (INT)ulFreePages;
                }
            }
            return  (iFreePages);
        }
#endif
        
    case _SC_KERNS_PAGES:
        {
            size_t stGet, stTotal = 0;

            if (API_KernelHeapInfo(LW_OPTION_HEAP_KERNEL, &stGet, LW_NULL,
                                   LW_NULL, LW_NULL, LW_NULL) == ERROR_NONE) {
                stTotal += stGet;
            }
            if (API_KernelHeapInfo(LW_OPTION_HEAP_SYSTEM, &stGet, LW_NULL,
                                   LW_NULL, LW_NULL, LW_NULL) == ERROR_NONE) {
                stTotal += stGet;
            }
            return  ((int)(stTotal >> LW_CFG_VMM_PAGE_SHIFT));
        }

    case _SC_AVKERNS_PAGES:
        {
            size_t stGet, stFree = 0;

            if (API_KernelHeapInfo(LW_OPTION_HEAP_KERNEL, LW_NULL, LW_NULL,
                                   LW_NULL, &stGet, LW_NULL) == ERROR_NONE) {
                stFree += stGet;
            }
            if (API_KernelHeapInfo(LW_OPTION_HEAP_SYSTEM, LW_NULL, LW_NULL,
                                   LW_NULL, &stGet, LW_NULL) == ERROR_NONE) {
                stFree += stGet;
            }
            return  ((int)(stFree >> LW_CFG_VMM_PAGE_SHIFT));
        }

    default:
        errno = EINVAL;
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: fpathconf
** ��������: get configurable pathname variables
**           fd            file descriptor
** �䡡��  : name          represents the system variable to be queried
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
long  fpathconf (int fd, int name)
{
    struct statvfs statvfsBuf;
    
    switch (name) {
    
    case _PC_LINK_MAX:
        return  (_S_iIoMaxLinkLevels);
        
    case _PC_MAX_CANON:
        return  (MAX_CANON);
        
    case _PC_MAX_INPUT:
        return  (MAX_INPUT);
        
    case _PC_NAME_MAX:
        return  (NAME_MAX);
        
    case _PC_PATH_MAX:
        return  (PATH_MAX);
        
    case _PC_PIPE_BUF:
        return  (PIPE_BUF);
        
    case _PC_CHOWN_RESTRICTED:
        return  (_POSIX_CHOWN_RESTRICTED);
    
    case _PC_NO_TRUNC:
        return  (_POSIX_NO_TRUNC);
    
    case _PC_VDISABLE:
        return  (_POSIX_VDISABLE);
    
    case _PC_SYNC_IO:
        return  (OPEN_MAX);
    
    case _PC_ASYNC_IO:
        return  (AIO_MAX);
    
    case _PC_PRIO_IO:
        return  (0);
    
    case _PC_SOCK_MAXBUF:
        return  (-1);
    
    case _PC_FILESIZEBITS:
        return  (FILESIZEBITS);
    
    case _PC_REC_INCR_XFER_SIZE:
        return  (-1);
    
    case _PC_REC_MAX_XFER_SIZE:
        return  (-1);
    
    case _PC_REC_MIN_XFER_SIZE:
        if (fstatvfs(fd, &statvfsBuf) < 0) {
            return  (-1);
        }
        return  (statvfsBuf.f_bsize);
    
    case _PC_REC_XFER_ALIGN:                                            /*  same as _PC_ALLOC_SIZE_MIN  */
    case _PC_ALLOC_SIZE_MIN:
        if (fstatvfs(fd, &statvfsBuf) < 0) {
            return  (-1);
        }
        return  (statvfsBuf.f_frsize);
    
    case _PC_SYMLINK_MAX:
        return  (_S_iIoMaxLinkLevels);
    
    case _PC_2_SYMLINKS:
        return  (_POSIX2_SYMLINKS);
    
    default:
        errno = EINVAL;
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: pathconf
** ��������: get configurable pathname variables
** �䡡��  : path          pathname
**           name          represents the system variable to be queried
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
long  pathconf (const char *path, int name)
{
    int     iFd;
    long    lRet;
    
    iFd = open(path, O_RDONLY);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    lRet = fpathconf(iFd, name);
    
    close(iFd);
    
    return  (lRet);
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
