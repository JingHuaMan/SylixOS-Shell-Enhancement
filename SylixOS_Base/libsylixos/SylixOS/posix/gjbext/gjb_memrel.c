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
** ��   ��   ��: gjb_memrel.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2016 �� 04 �� 13 ��
**
** ��        ��: GJB7714 ��չ�ӿ��ڴ������ز���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "unistd.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if (LW_CFG_POSIX_EN > 0) && (LW_CFG_GJB7714_EN > 0)
#include "../include/px_gjbext.h"
/*********************************************************************************************************
** ��������: heap_mem_init
** ��������: ��ʼ��ϵͳ�ڴ����.
** �䡡��  : flag      FIRST_FIT_ALLOCATION or BUDDY_ALLOCATION
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  heap_mem_init (int flag)
{
    (VOID)flag;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mem_findmax
** ��������: ���ϵͳ�ڴ���������������С.
** �䡡��  : flag      FIRST_FIT_ALLOCATION or BUDDY_ALLOCATION
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  mem_findmax (void)
{
    size_t  stMax;
    
    if (getpid() > 0) {
        return  (LW_CFG_MB_SIZE);
    
    } else {
        stMax = _HeapGetMax(_K_pheapSystem);
    }
    
    return  ((int)stMax);
}
/*********************************************************************************************************
** ��������: mem_getinfo
** ��������: ���ϵͳ�ڴ���Ϣ.
** �䡡��  : info      �ڴ���Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  mem_getinfo (struct meminfo *info)
{
    if (!info) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

    if (getpid() > 0) {
        errno = EACCES;
        return  (PX_ERROR);
    
    } else {
        __KERNEL_ENTER();
        info->segment = _K_pheapSystem->HEAP_ulSegmentCounter;
        info->used    = _K_pheapSystem->HEAP_stUsedByteSize;
        info->maxused = _K_pheapSystem->HEAP_stMaxUsedByteSize;
        info->free    = _K_pheapSystem->HEAP_stFreeByteSize;
        __KERNEL_EXIT();
        
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: mem_show
** ��������: ��ʾϵͳ�ڴ���Ϣ.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
void  mem_show (void)
{
    lib_system("free");
}
/*********************************************************************************************************
** ��������: mpart_module_init
** ��������: ��ʼ���ڴ����ģ��.
** �䡡��  : max_parts     ���ֿ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  mpart_module_init (int  max_parts)
{
    if (max_parts > LW_CFG_MAX_REGIONS) {
        errno = ENOSPC;
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mpart_create
** ��������: ����һ���ڴ����.
** �䡡��  : addr      �ڴ����ַ
**           size      �ڴ������С
**           mid       �ڴ� ID
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  mpart_create (char *addr, size_t  size, mpart_id *mid)
{
    if (!addr || !size || !mid) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (API_RegionCreate("GJB mpart", addr, size, 
                         LW_OPTION_DEFAULT, mid)) {
        return  (ERROR_NONE);
    }
    
    errno = ENOSPC;
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: mpart_delete
** ��������: ɾ��һ���ڴ����.
** �䡡��  : mid       �ڴ� ID
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  mpart_delete (mpart_id mid)
{
    ULONG   ulError;
    
    ulError = API_RegionDelete(&mid);
    if (ulError) {
        if (ulError == ERROR_REGION_USED) {
            errno = EBUSY;
            return  (PX_ERROR);
        }
        
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mpart_addmem
** ��������: ���ڴ��������ڴ�.
** �䡡��  : mid       �ڴ� ID
**           addr      �ڴ��ַ
**           size      �ڴ��С
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  mpart_addmem (mpart_id mid, char *addr, size_t  size)
{
    if (!addr || !size) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (API_RegionAddMem(mid, addr, size)) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mpart_alloc
** ��������: �����ڴ�.
** �䡡��  : mid       �ڴ� ID
**           size      ��С
** �䡡��  : ����������ڴ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
void  *mpart_alloc (mpart_id mid, size_t  size)
{
    void  *ret;

    if (!size) {
        errno = EINVAL;
        return  (LW_NULL);
    }
    
    ret = API_RegionGet(mid, size);
    if (!ret) {
        if (errno == ERROR_REGION_NOMEM) {
            errno =  ENOMEM;
        }
    }
    
    return  (ret);
}
/*********************************************************************************************************
** ��������: mpart_memalign
** ��������: ��������ڴ�.
** �䡡��  : mid       �ڴ� ID
**           alignment �����С
**           size      ��С
** �䡡��  : ����������ڴ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
void  *mpart_memalign (mpart_id mid, size_t  alignment, size_t  size)
{
    void  *ret;

    if (!size) {
        errno = EINVAL;
        return  (LW_NULL);
    }
    
    ret = API_RegionGetAlign(mid, size, alignment);
    if (!ret) {
        if (errno == ERROR_REGION_NOMEM) {
            errno =  ENOMEM;
        }
    }
    
    return  (ret);
}
/*********************************************************************************************************
** ��������: mpart_realloc
** ��������: ���·����ڴ�.
** �䡡��  : mid       �ڴ� ID
**           addr      �ڴ�
**           size      ��С
** �䡡��  : ����������ڴ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
void  *mpart_realloc (mpart_id mid, char *addr, size_t  size)
{
    void  *ret;

    if (!size) {
        errno = EINVAL;
        return  (LW_NULL);
    }
    
    ret = API_RegionReget(mid, addr, size);
    if (!ret) {
        if (errno == ERROR_REGION_NOMEM) {
            errno =  ENOMEM;
        }
    }
    
    return  (ret);
}
/*********************************************************************************************************
** ��������: mpart_free
** ��������: �ͷ��ڴ�.
** �䡡��  : mid       �ڴ� ID
**           addr      �ڴ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  mpart_free (mpart_id mid, char *addr)
{
    void  *ret;

    if (!addr) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

    ret = API_RegionPut(mid, addr);
    if (ret) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: mpart_findmaxfree
** ��������: ����ڴ������зֶ�.
** �䡡��  : mid       �ڴ� ID
** �䡡��  : �����зֶ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  mpart_findmaxfree (mpart_id mid)
{
    size_t  max;

    if (API_RegionGetMax(mid, &max)) {
        return  (PX_ERROR);
    }
    
    return  ((int)max);
}
/*********************************************************************************************************
** ��������: mpart_getinfo
** ��������: ����ڴ���Ϣ.
** �䡡��  : mid       �ڴ� ID
**           pi        �ڴ�ֶ���Ϣ
** �䡡��  : �����зֶ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  mpart_getinfo (mpart_id mid, struct partinfo  *pi)
{
    if (!pi) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (API_RegionStatus(mid, LW_NULL, &pi->segment, 
                         &pi->used, &pi->free, &pi->maxused)) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_GJB7714_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
