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
** ��   ��   ��: KernelObject.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 14 ��
**
** ��        ��: �ں˶���.

** BUG:
2012.03.30  �������ں˶�������ϲ�������ļ�.
2012.12.07  API_ObjectIsGlobal() ʹ����Դ�������ж�.
2015.09.19  �����ں˶�����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_OBJECT_SHARE_EN > 0
/*********************************************************************************************************
  �ں˶������
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE            OBJS_lineManage;
    UINT64                  OBJS_u64Key;
    LW_OBJECT_HANDLE        OBJS_ulHandle;
} LW_OBJECT_SHARE;
typedef LW_OBJECT_SHARE    *PLW_OBJECT_SHARE;
/*********************************************************************************************************
  �ں˶���������
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  _K_plineObjShare;
#endif                                                                  /*  LW_CFG_OBJECT_SHARE_EN > 0  */
/*********************************************************************************************************
** ��������: API_ObjectGetClass
** ��������: ��ö�������
** �䡡��  : 
** �䡡��  : CLASS
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
UINT8  API_ObjectGetClass (LW_OBJECT_HANDLE  ulId)
{
    REGISTER UINT8    ucClass;

#if LW_CFG_ARG_CHK_EN > 0
    if (!ulId) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);
        return  (0);
    }
#endif
    
    ucClass = (UINT8)_ObjectGetClass(ulId);
    
    return  (ucClass);
}
/*********************************************************************************************************
** ��������: API_ObjectIsGlobal
** ��������: ��ö����Ƿ�Ϊȫ�ֶ���
** �䡡��  : 
** �䡡��  : Ϊȫ�ֶ��󣬷��أ�LW_TRUE  ���򣬷��أ�LW_FALSE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
BOOL  API_ObjectIsGlobal (LW_OBJECT_HANDLE  ulId)
{
#if LW_CFG_MODULELOADER_EN > 0
    return  (__resHandleIsGlobal(ulId));
#else
    return  (LW_TRUE);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
}
/*********************************************************************************************************
** ��������: API_ObjectGetNode
** ��������: ��ö������ڵĴ�������
** �䡡��  : 
** �䡡��  : ��������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ObjectGetNode (LW_OBJECT_HANDLE  ulId)
{
    REGISTER ULONG    ulNode;
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!ulId) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);
        return  ((unsigned)(PX_ERROR));
    }
#endif

    ulNode = _ObjectGetNode(ulId);
    
    return  (ulNode);
}
/*********************************************************************************************************
** ��������: API_ObjectGetIndex
** ��������: ��ö��󻺳����ڵ�ַ
** �䡡��  : 
** �䡡��  : ��ö��󻺳����ڵ�ַ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ObjectGetIndex (LW_OBJECT_HANDLE  ulId)
{
    REGISTER ULONG    ulIndex;
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!ulId) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);
        return  ((unsigned)(PX_ERROR));
    }
#endif

    ulIndex = _ObjectGetIndex(ulId);
    
    return  (ulIndex);
}
/*********************************************************************************************************
** ��������: API_ObjectShareAdd
** ��������: ��һ���ں˶���ע�ᵽ�������
** �䡡��  : ulId      �ں˶��� ID
**           u64Key    key
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_OBJECT_SHARE_EN > 0

LW_API  
ULONG  API_ObjectShareAdd (LW_OBJECT_HANDLE  ulId, UINT64  u64Key)
{
    PLW_OBJECT_SHARE    pobjsNew;
    PLW_OBJECT_SHARE    pobjsFind;
    PLW_LIST_LINE       pline;
    ULONG               ulError = ERROR_NONE;
    
    if (ulId == LW_OBJECT_HANDLE_INVALID) {
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    pobjsNew = (PLW_OBJECT_SHARE)__KHEAP_ALLOC(sizeof(LW_OBJECT_SHARE));
    if (pobjsNew == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);
        return  (ERROR_KERNEL_LOW_MEMORY);
    }
    pobjsNew->OBJS_u64Key   = u64Key;
    pobjsNew->OBJS_ulHandle = ulId;
    
    __KERNEL_ENTER();
    for (pline  = _K_plineObjShare;
         pline != LW_NULL;
         pline  = _list_line_get_next(pline)) {
        
        pobjsFind = _LIST_ENTRY(pline, LW_OBJECT_SHARE, OBJS_lineManage);
        if (pobjsFind->OBJS_u64Key == u64Key) {
            ulError = ERROR_KERNEL_KEY_CONFLICT;
            break;
        }
    }
    if (pline == LW_NULL) {
        _List_Line_Add_Ahead(&pobjsNew->OBJS_lineManage, &_K_plineObjShare);
    }
    __KERNEL_EXIT();
    
    _ErrorHandle(ulError);
    return  (ulError);
}
/*********************************************************************************************************
** ��������: API_ObjectShareDelete
** ��������: ��һ���ں˶���ӹ������ɾ��
** �䡡��  : u64Key        key
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ObjectShareDelete (UINT64  u64Key)
{
    PLW_OBJECT_SHARE    pobjsFind;
    PLW_LIST_LINE       pline;
    
    __KERNEL_ENTER();
    for (pline  = _K_plineObjShare;
         pline != LW_NULL;
         pline  = _list_line_get_next(pline)) {
        
        pobjsFind = _LIST_ENTRY(pline, LW_OBJECT_SHARE, OBJS_lineManage);
        if (pobjsFind->OBJS_u64Key == u64Key) {
            break;
        }
    }
    if (pline) {
        _List_Line_Del(&pobjsFind->OBJS_lineManage, &_K_plineObjShare);
    
    } else {
        pobjsFind = LW_NULL;
    }
    __KERNEL_EXIT();
    
    if (pobjsFind) {
        __KHEAP_FREE(pobjsFind);
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
}
/*********************************************************************************************************
** ��������: API_ObjectShareFind
** ��������: ��ö�������
** �䡡��  : u64Key        key
** �䡡��  : ��ѯ�����ں˶���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
LW_OBJECT_HANDLE  API_ObjectShareFind (UINT64  u64Key)
{
    PLW_OBJECT_SHARE    pobjsFind;
    PLW_LIST_LINE       pline;
    LW_OBJECT_HANDLE    ulRet = LW_OBJECT_HANDLE_INVALID;
    
    __KERNEL_ENTER();
    for (pline  = _K_plineObjShare;
         pline != LW_NULL;
         pline  = _list_line_get_next(pline)) {
        
        pobjsFind = _LIST_ENTRY(pline, LW_OBJECT_SHARE, OBJS_lineManage);
        if (pobjsFind->OBJS_u64Key == u64Key) {
            ulRet = pobjsFind->OBJS_ulHandle;
            break;
        }
    }
    __KERNEL_EXIT();
    
    if (ulRet == LW_OBJECT_HANDLE_INVALID) {
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
    }
    
    return  (ulRet);
}

#endif                                                                  /*  LW_CFG_OBJECT_SHARE_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
