/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: mman.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 08 �� 12 ��
**
** ��        ��: POSIX IEEE Std 1003.1, 2004 Edition sys/mman.h

** BUG:
2011.03.04  ��� vmmMalloc û�гɹ���ת��ʹ�� __SHEAP_ALLOC().
2011.05.16  mprotect() ʹ�� vmmSetFlag() �ӿڲ���.
            ֧�ַ� REG �ļ������ļ� mmap() ����.
2011.05.17  ֧�� mmap() ��׼, ���ȷ�������ռ�, ����ȱҳ�ж�ʱ, ���������ڴ�.
2011.07.27  �� mmap() ȱҳ�ж�����ļ�����û�дﵽһҳʱ, ��Ҫ��� 0 .
            ֧���豸�ļ��� mmap() ����.
2011.08.03  ����ļ� stat ʹ�� fstat api.
2011.12.09  mmap() ��亯��ʹ�� iosRead() ���.
            ���� mmapShow() ����, �鿴 mmap() �ڴ����.
2012.01.10  ����� MAP_ANONYMOUS ��֧��.
2012.08.16  ֱ��ʹ�� pread/pwrite �����ļ�.
2012.10.19  mmap() �� munmap() ������ļ����������õĴ���.
2012.12.07  �� mmap ������Դ������.
2012.12.22  ӳ��ʱ, ������ǽ���ӳ��� pid ���ж�Ϊ������Դ, ���ﲻ���ٶ��ļ�������, 
            ��Ϊ��ǰ���ļ����������Ѳ����Ǵ������̵��ļ���������. �������������ص��ļ�������.
2012.12.27  mmap ����� reg �ļ�, ҲҪ������һ�¶�Ӧ�ļ�ϵͳ�� mmap ����, �����������ȱҳ���.
            ����� shm_open shm_unlink ��֧��.
2012.12.30  mmapShow() ������ʾ�ļ���, ������� pid ����ʾ.
2013.01.02  ����ӳ��ʱ��Ҫ���������� unmap ����.
2013.01.12  munmap �������ͬһ����, �򲻲����ļ��������Ͷ�Ӧ����.
2013.03.12  ���� mmap64.
2013.03.16  mmap ����豸������֧����Ҳ����ӳ��.
2013.03.17  mmap ���� MAP_SHARED ��־�����Ƿ�ʹ�� CACHE.
2013.09.13  ֧�� MAP_SHARED.
            msync() ��������ҳ���ҿ�дʱ�Ż�д.
2013.12.21  ֧�� PROT_EXEC.
2014.04.30  ����� mremap() ��֧��.
            ���� VMM ʱ����ʹ�� HEAP ���з���, �����㷨һ����.
2014.05.01  ���� mremap() ����ʱ�� errno.
2014.07.15  ��д������ mmap ��Դ�ͷź���.
2015.06.01  ʹ���µ� vmmMmap ϵ�к���ʵ�� mmap �ӿ�.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "unistd.h"
#include "../include/px_mman.h"                                         /*  �Ѱ�������ϵͳͷ�ļ�        */
#include "../include/posixLib.h"                                        /*  posix �ڲ�������            */
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if (LW_CFG_POSIX_EN > 0) && (LW_CFG_DEVICE_EN > 0)
/*********************************************************************************************************
** ��������: mlock
** ��������: ����ָ���ڴ��ַ�ռ䲻���л�ҳ����.
** �䡡��  : pvAddr        ��ʼ��ַ
**           stLen         �ڴ�ռ䳤��
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  mlock (const void  *pvAddr, size_t  stLen)
{
    (VOID)pvAddr;
    (VOID)stLen;
    
    if (geteuid() != 0) {
        errno = EACCES;
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: munlock
** ��������: ����ָ���ڴ��ַ�ռ�, ������л�ҳ����.
** �䡡��  : pvAddr        ��ʼ��ַ
**           stLen         �ڴ�ռ䳤��
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  munlock (const void  *pvAddr, size_t  stLen)
{
    (VOID)pvAddr;
    (VOID)stLen;
    
    if (geteuid() != 0) {
        errno = EACCES;
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mlockall
** ��������: �������̿ռ䲻���л�ҳ����.
** �䡡��  : iFlag         ����ѡ�� MCL_CURRENT / MCL_FUTURE
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  mlockall (int  iFlag)
{
    (VOID)iFlag;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: munlockall
** ��������: �������̿ռ�, ������л�ҳ����.
** �䡡��  : NONE
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  munlockall (void)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mprotect
** ��������: ���ý�����ָ���ĵ�ַ�� page flag
** �䡡��  : pvAddr        ��ʼ��ַ
**           stLen         ����
**           iProt         �µ�����
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  mprotect (void  *pvAddr, size_t  stLen, int  iProt)
{
#if LW_CFG_VMM_EN > 0
    ULONG   ulFlag = LW_VMM_FLAG_READ;
    
    if (!ALIGNED(pvAddr, LW_CFG_VMM_PAGE_SIZE) || stLen == 0) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (iProt & (~(PROT_READ | PROT_WRITE | PROT_EXEC))) {
        errno = ENOTSUP;
        return  (PX_ERROR);
    }
    
    if (iProt) {
        if (iProt & PROT_WRITE) {
            ulFlag |= LW_VMM_FLAG_RDWR;                                 /*  �ɶ�д                      */
        }
        if (iProt & PROT_EXEC) {
            ulFlag |= LW_VMM_FLAG_EXEC;                                 /*  ��ִ��                      */
        }
    } else {
        ulFlag = LW_VMM_FLAG_FAIL;                                      /*  ���������                  */
    }
    
    return  (API_VmmMProtect(pvAddr, stLen, ulFlag));                   /*  ���������µ� flag           */
    
#else
    return  (ERROR_NONE);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: mmap
** ��������: �ڴ��ļ�ӳ�亯��, ���: http://www.opengroup.org/onlinepubs/000095399/functions/mmap.html
** �䡡��  : pvAddr        ��ʼ��ַ (�������Ϊ NULL, ϵͳ���Զ������ڴ�)
**           stLen         ӳ�䳤��
**           iProt         ҳ������
**           iFlag         ӳ���־
**           iFd           �ļ�������
**           off           �ļ�ƫ����
** �䡡��  : �ļ�ӳ�����ڴ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
void  *mmap (void  *pvAddr, size_t  stLen, int  iProt, int  iFlag, int  iFd, off_t  off)
{
#if LW_CFG_VMM_EN > 0
    PVOID   pvRet;
    INT     iFlags;
    ULONG   ulFlag = LW_VMM_FLAG_FAIL;
    
    if (iProt & PROT_READ) {                                            /*  �ɶ�                        */
        ulFlag |= LW_VMM_FLAG_READ;
    }
    if (iProt & PROT_WRITE) {
        ulFlag |= LW_VMM_FLAG_RDWR;                                     /*  ��д                        */
    }
    if (iProt & PROT_EXEC) {
        ulFlag |= LW_VMM_FLAG_EXEC;                                     /*  ��ִ��                      */
    }
    
    if (iFlag & MAP_FIXED) {                                            /*  ��֧�� FIX                  */
        errno = ENOTSUP;
        return  (MAP_FAILED);
    }
    if (iFlag & MAP_ANONYMOUS) {
        iFd = PX_ERROR;
    }
    
    if ((iFlag & MAP_SHARED) && ((iFlag & MAP_PRIVATE) == 0)) {
        iFlags = LW_VMM_SHARED_CHANGE;
    
    } else if ((iFlag & MAP_PRIVATE) && ((iFlag & MAP_SHARED) == 0)) {
        iFlags = LW_VMM_PRIVATE_CHANGE;
    
    } else {
        errno = EINVAL;
        return  (MAP_FAILED);
    }
    
    if ((iFlag & MAP_ANONYMOUS) && (pvAddr == LW_NULL)) {
        if (iFlag & MAP_CONTIG) {
            iFlags |= LW_VMM_PHY_PREALLOC;
            ulFlag |= LW_VMM_FLAG_PHY_CONTINUOUS;                           /*  �����ڴ�����                */

        } else if (iFlag & MAP_PREALLOC) {
            iFlags |= LW_VMM_PHY_PREALLOC;                                  /*  �ڴ����Ԥ����              */
        }
    }

    pvRet = API_VmmMmap(pvAddr, stLen, iFlags, ulFlag, iFd, off);
    
    return  ((pvRet != LW_VMM_MAP_FAILED) ? pvRet : MAP_FAILED);
#else
    errno = ENOSYS;
    return  (MAP_FAILED);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: mmap64 (sylixos �ڲ� off_t ������� 64bit)
** ��������: �ڴ��ļ�ӳ�亯��, ���: http://www.opengroup.org/onlinepubs/000095399/functions/mmap.html
** �䡡��  : pvAddr        ��ʼ��ַ (�������Ϊ NULL, ϵͳ���Զ������ڴ�)
**           stLen         ӳ�䳤��
**           iProt         ҳ������
**           iFlag         ӳ���־
**           iFd           �ļ�������
**           off           �ļ�ƫ����
** �䡡��  : �ļ�ӳ�����ڴ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����� REG �ļ�, Ҳ����ʹ����������� mmap. �����ڴ��豸�� mmap ֱ�ӻ����ӳ��.
                                           API ����
*********************************************************************************************************/
LW_API  
void  *mmap64 (void  *pvAddr, size_t  stLen, int  iProt, int  iFlag, int  iFd, off64_t  off)
{
    return  (mmap(pvAddr, stLen, iProt, iFlag, iFd, (off_t)off));
}
/*********************************************************************************************************
** ��������: mremap
** ��������: ���������ڴ�����Ĵ�С, ���: http://man7.org/linux/man-pages/man2/mremap.2.html
** �䡡��  : pvAddr        �Ѿ�����������ڴ��ַ
**           stOldSize     ��ǰ���ڴ������С
**           stNewSize     ��Ҫ���õ��ڴ������´�С
**           iFlag         ӳ���־     MREMAP_MAYMOVE \ MREMAP_FIXED
** �䡡��  : �������ú���ڴ������ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
void  *mremap (void *pvAddr, size_t stOldSize, size_t stNewSize, int iFlag, ...)
{
#if LW_CFG_VMM_EN > 0
    PVOID   pvRet;
    INT     iMoveEn;

    if (iFlag & MREMAP_FIXED) {
        errno = ENOTSUP;
        return  (MAP_FAILED);
    }
    
    iMoveEn = (iFlag & MREMAP_MAYMOVE) ? 1 : 0;
    
    pvRet = API_VmmMremap(pvAddr, stOldSize, stNewSize, iMoveEn);
    
    return  ((pvRet != LW_VMM_MAP_FAILED) ? pvRet : MAP_FAILED);
#else
    errno = ENOSYS;
    return  (MAP_FAILED);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: munmap
** ��������: ȡ���ڴ��ļ�ӳ��
** �䡡��  : pvAddr        ��ʼ��ַ
**           stLen         ӳ�䳤��
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  munmap (void  *pvAddr, size_t  stLen)
{
#if LW_CFG_VMM_EN > 0
    return  (API_VmmMunmap(pvAddr, stLen));
#else
    errno = ENOSYS;
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: msync
** ��������: ���ڴ���ӳ����ļ����ݻ�д�ļ�
** �䡡��  : pvAddr        ��ʼ��ַ
**           stLen         ӳ�䳤��
**           iFlag         ��д����
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  msync (void  *pvAddr, size_t  stLen, int  iFlag)
{
#if LW_CFG_VMM_EN > 0
    INT     iInval = (iFlag & MS_INVALIDATE) ? 1 : 0;
    
    return  (API_VmmMsync(pvAddr, stLen, iInval));
#else
    errno = ENOSYS;
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: shm_open
** ��������: The posix_madvise() function shall advise the implementation on the expected behavior of the 
             application with respect to the data in the memory starting at address addr, and continuing 
             for len bytes. The implementation may use this information to optimize handling of the 
             specified data. The posix_madvise() function shall have no effect on the semantics of 
             access to memory in the specified range, although it may affect the performance of access.
** �䡡��  : addr      address
**           len       length
**           advice    advice
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  posix_madvise (void *addr, size_t len, int advice)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: shm_open
** ��������: establishes a connection between a shared memory object and a file descriptor.
** �䡡��  : name      file name
**           oflag     create flag like open()
**           mode      create mode like open()
** �䡡��  : filedesc
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  shm_open (const char *name, int oflag, mode_t mode)
{
    CHAR    cFullName[MAX_FILENAME_LENGTH] = "/dev/shm/";
    
    if (!name || (*name == PX_EOS)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (name[0] == PX_ROOT) {
        lib_strlcat(cFullName, &name[1], MAX_FILENAME_LENGTH);
    } else {
        lib_strlcat(cFullName, name, MAX_FILENAME_LENGTH);
    }
    
    return  (open(cFullName, oflag, mode));
}
/*********************************************************************************************************
** ��������: shm_unlink
** ��������: removes the name of the shared memory object named by the string pointed to by name.
** �䡡��  : name      file name
** �䡡��  : filedesc
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  shm_unlink (const char *name)
{
    CHAR    cFullName[MAX_FILENAME_LENGTH] = "/dev/shm/";
    
    if (!name || (*name == PX_EOS)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (name[0] == PX_ROOT) {
        lib_strlcat(cFullName, &name[1], MAX_FILENAME_LENGTH);
    } else {
        lib_strlcat(cFullName, name, MAX_FILENAME_LENGTH);
    }
    
    return  (unlink(cFullName));
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
