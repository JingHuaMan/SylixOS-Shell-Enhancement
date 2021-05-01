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
** ��   ��   ��: pthread_key.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 30 ��
**
** ��        ��: pthread ˽�����ݼ��ݿ�.

** BUG:
2012.12.07  ������Դ����ڵ�.
2013.05.01  If successful, the pthread_key_*() function shall store the newly created key value at *key 
            and shall return zero. Otherwise, an error number shall be returned to indicate the error.
2013.05.02  ���� destructor ���õĲ�����ʱ��.
2014.12.09  ǿ�Ʊ�ɱ���Ľ��̲�ִ�� desturtors.
2017.03.15  ʹ�� hash ��, ��߲�ѯ�ٶ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/px_pthread.h"                                      /*  �Ѱ�������ϵͳͷ�ļ�        */
#include "../include/posixLib.h"                                        /*  posix �ڲ�������            */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
/*********************************************************************************************************
  �������
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  key HASH
*********************************************************************************************************/
#define __PX_KEY_THREAD_HASH_SIZE   16
#define __PX_KEY_THREAD_HASH_MASK   (__PX_KEY_THREAD_HASH_SIZE - 1)
#define __PX_KEY_THREAD_HASH(t)     ((INT)(t) & __PX_KEY_THREAD_HASH_MASK)
/*********************************************************************************************************
  key ˽����������
*********************************************************************************************************/
typedef struct {
    LW_LIST_RING            PKEYN_ringManage;                           /*  ���� key ������             */
    long                    PKEYN_lId;                                  /*  key id                      */
    void                  (*PKEYN_pfuncDestructor)(void *);             /*  destructor                  */
    LW_LIST_LINE_HEADER     PKEYN_plineKeyHeader[__PX_KEY_THREAD_HASH_SIZE];
                                                                        /*  �����߳�˽������ָ��        */
    LW_OBJECT_HANDLE        PKEYN_ulMutex;                              /*  ������                      */
    LW_RESOURCE_RAW         PKEYN_resraw;                               /*  ��Դ����ڵ�                */
} __PX_KEY_NODE;

static LW_LIST_RING_HEADER  _G_pringKeyHeader;                          /*  ���е� key ������           */

#define __PX_KEY_LOCK(pkeyn)        API_SemaphoreMPend(pkeyn->PKEYN_ulMutex, LW_OPTION_WAIT_INFINITE)
#define __PX_KEY_UNLOCK(pkeyn)      API_SemaphoreMPost(pkeyn->PKEYN_ulMutex)
/*********************************************************************************************************
  Э��ɾ���ص���������
*********************************************************************************************************/
static BOOL  _G_bKeyDelHookAdd = LW_FALSE;
static VOID  __pthreadDataDeleteByThread(LW_OBJECT_HANDLE  ulId, PVOID  pvRetVal, PLW_CLASS_TCB  ptcbDel);
/*********************************************************************************************************
** ��������: __pthreadKeyOnce
** ��������: ���� POSIX �߳�
** �䡡��  : lId           ��id
**           pvData        ��ʼ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __pthreadKeyOnce (VOID)
{
    API_SystemHookAdd(__pthreadDataDeleteByThread, LW_OPTION_THREAD_DELETE_HOOK);
}
/*********************************************************************************************************
** ��������: __pthreadDataSet
** ��������: ����ָ�� key �ڵ�ǰ�߳��ڲ����ݽڵ�. (���򴴽�)
** �䡡��  : lId           ��id
**           pvData        ��ʼ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __pthreadDataSet (long  lId, const void  *pvData)
{
    REGISTER INT         iHash;
    __PX_KEY_NODE       *pkeyn = (__PX_KEY_NODE *)lId;
    __PX_KEY_DATA       *pkeyd;
    PLW_CLASS_TCB        ptcbCur;
    LW_OBJECT_HANDLE     ulMe;
    PLW_LIST_LINE        plineTemp;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    ulMe = ptcbCur->TCB_ulId;
    
    /*
     *  �����Ƿ��Ѿ����������˽������
     */
    iHash = __PX_KEY_THREAD_HASH(ulMe);

    __PX_KEY_LOCK(pkeyn);                                               /*  ��ס key ��                 */
    for (plineTemp  = pkeyn->PKEYN_plineKeyHeader[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pkeyd = (__PX_KEY_DATA *)plineTemp;                             /*  ������ KEY DATA �ĵ�һ��Ԫ��*/
        if (pkeyd->PKEYD_ulOwner == ulMe) {                             /*  �ҵ���Ӧ��ǰ�̵߳�����      */
            pkeyd->PKEYD_pvData = (void *)pvData;
            break;
        }
    }
    __PX_KEY_UNLOCK(pkeyn);                                             /*  ���� key ��                 */

    if (plineTemp) {
        return  (ERROR_NONE);                                           /*  �Ѿ��ҵ��˶�Ӧ�Ľڵ�        */
    }
    
    /*
     *  ���û���ҵ�, ����Ҫ�½�˽������
     */
    pkeyd = (__PX_KEY_DATA  *)__SHEAP_ALLOC(sizeof(__PX_KEY_DATA));     /*  û�нڵ�, ��Ҫ�½�          */
    if (pkeyd == LW_NULL) {
        errno = ENOMEM;
        return  (ENOMEM);
    }
    pkeyd->PKEYD_lId     = lId;                                         /*  ͨ�� id ������� key        */
    pkeyd->PKEYD_pvData  = (void *)pvData;
    pkeyd->PKEYD_ulOwner = ulMe;                                        /*  ��¼�߳� ID                 */
    
    __PX_KEY_LOCK(pkeyn);                                               /*  ��ס key ��                 */
    _List_Line_Add_Ahead(&pkeyd->PKEYD_lineManage,
                         &pkeyn->PKEYN_plineKeyHeader[iHash]);          /*  �����Ӧ key ������         */
    __PX_KEY_UNLOCK(pkeyn);                                             /*  ���� key ��                 */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __pthreadDataGet
** ��������: ��ȡָ�� key �ڵ�ǰ�߳��ڲ����ݽڵ�.
** �䡡��  : lId           ��id
**           ppvData       ��ʼ����(����)
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __pthreadDataGet (long  lId, void  **ppvData)
{
    REGISTER INT         iHash;
    __PX_KEY_NODE       *pkeyn = (__PX_KEY_NODE *)lId;
    __PX_KEY_DATA       *pkeyd;
    LW_OBJECT_HANDLE     ulMe  = API_ThreadIdSelf();
    PLW_LIST_LINE        plineTemp;
    
    if (ppvData == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *ppvData = LW_NULL;
    
    iHash = __PX_KEY_THREAD_HASH(ulMe);

    __PX_KEY_LOCK(pkeyn);                                               /*  ��ס key ��                 */
    for (plineTemp  = pkeyn->PKEYN_plineKeyHeader[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pkeyd = (__PX_KEY_DATA *)plineTemp;                             /*  ������ KEY DATA �ĵ�һ��Ԫ��*/
        if (pkeyd->PKEYD_ulOwner == ulMe) {                             /*  �ҵ���Ӧ��ǰ�̵߳�����      */
            *ppvData = pkeyd->PKEYD_pvData;
            break;
        }
    }
    __PX_KEY_UNLOCK(pkeyn);                                             /*  ���� key ��                 */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __pthreadDataDeleteByKey
** ��������: ɾ��ָ�� key �����������ݽڵ�.
** �䡡��  : pkeyn        KEY NODE
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __pthreadDataDeleteByKey (__PX_KEY_NODE  *pkeyn)
{
    __PX_KEY_DATA       *pkeyd;
    INT                  i;
    
    /*
     *  key ��ɾ��, ��Ҫ���� key ����˽�����ݱ�, ɾ������˽������
     */
    __PX_KEY_LOCK(pkeyn);                                               /*  ��ס key ��                 */
    for (i = 0; i < __PX_KEY_THREAD_HASH_SIZE; i++) {
        while (pkeyn->PKEYN_plineKeyHeader[i]) {
            pkeyd = (__PX_KEY_DATA *)pkeyn->PKEYN_plineKeyHeader[i];    /*  ������ KEY DATA �ĵ�һ��Ԫ��*/

            _List_Line_Del(&pkeyd->PKEYD_lineManage,
                           &pkeyn->PKEYN_plineKeyHeader[i]);            /*  ��������ɾ��                */

            __SHEAP_FREE(pkeyd);                                        /*  �ͷ��߳�˽�������ڴ�        */
        }
    }
    __PX_KEY_UNLOCK(pkeyn);                                             /*  ���� key ��                 */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __pthreadDataDeleteByThread
** ��������: ɾ�������뵱ǰ�߳���ص��ڲ����ݽڵ�.
** �䡡��  : ulId      ��ɾ�������� (0 ��������� destroy, ���������߳� ID)
**           pvRetVal  ���񷵻�ֵ
**           ptcbDel   ��ɾ��������ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __pthreadDataDeleteByThread (LW_OBJECT_HANDLE  ulId, PVOID  pvRetVal, PLW_CLASS_TCB  ptcbDel)
{
    REGISTER INT         iHash;
    __PX_KEY_NODE       *pkeyn;
    __PX_KEY_DATA       *pkeyd;
    PLW_LIST_RING        pringTempK;
    PLW_LIST_LINE        plineTempD;
    VOIDFUNCPTR          pfuncDestructor;
    
    PVOID                pvPrevValue;
    BOOL                 bCall = LW_TRUE;
    
#if LW_CFG_MODULELOADER_EN > 0
    LW_LD_VPROC         *pvprocDel;

    pvprocDel = __LW_VP_GET_TCB_PROC(ptcbDel);
    if (pvprocDel && pvprocDel->VP_bImmediatelyTerm) {                  /*  ���̲���Ҫִ�� destructor   */
        bCall = LW_FALSE;
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */

    if (ulId == 0ul) {
        ulId  = ptcbDel->TCB_ulId;
        bCall = LW_FALSE;                                               /*  ������ destroy              */
    }

    /*
     *  �߳�ɾ��, ��Ҫ�������� key ����˽�����ݱ�, ɾ���뱾�߳���ص�˽������
     */
    iHash = __PX_KEY_THREAD_HASH(ulId);

__re_check:
    __PX_LOCK();                                                        /*  ��ס posix ��               */
    pringTempK = _G_pringKeyHeader;
    if (pringTempK != LW_NULL) {
        do {
            pkeyn = (__PX_KEY_NODE *)pringTempK;
            
            plineTempD = pkeyn->PKEYN_plineKeyHeader[iHash];            /*  ���� key ���ڵ����нڵ�     */
            while (plineTempD) {
                pkeyd = (__PX_KEY_DATA *)plineTempD;
                plineTempD = _list_line_get_next(plineTempD);

                if (pkeyd->PKEYD_ulOwner == ulId) {                     /*  �Ƿ�Ϊ��ǰ�߳����ݽڵ�      */
                    if (pkeyn->PKEYN_pfuncDestructor &&
                        pkeyd->PKEYD_pvData) {                          /*  ��Ҫ���� destructor         */
                        pvPrevValue = pkeyd->PKEYD_pvData;
                        pkeyd->PKEYD_pvData = LW_NULL;                  /*  �´β��ٵ��� destructor     */
                        pfuncDestructor = pkeyn->PKEYN_pfuncDestructor;
                        __PX_UNLOCK();                                  /*  ���� posix ��               */

                        if (pfuncDestructor && bCall) {                 /*  ����ɾ������                */
                            LW_SOFUNC_PREPARE(pfuncDestructor);
                            pfuncDestructor(pvPrevValue);
                        }
                        goto    __re_check;                             /*  ���¼��                    */
                    }
                    
                    _List_Line_Del(&pkeyd->PKEYD_lineManage,
                                   &pkeyn->PKEYN_plineKeyHeader[iHash]);/*  ��������ɾ��                */
                    __SHEAP_FREE(pkeyd);                                /*  �ͷ��߳�˽�������ڴ�        */
                }
            }

            pringTempK = _list_ring_get_next(pringTempK);
        } while (pringTempK != _G_pringKeyHeader);
    }
    __PX_UNLOCK();                                                      /*  ���� posix ��               */
}
/*********************************************************************************************************
** ��������: _PthreadKeyCleanup
** ��������: ɾ�������뵱ǰ�߳���ص��ڲ����ݽڵ�.
** �䡡��  : pkey          �� (����)
**           bDestroy      �Ƿ����ɾ������
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ���̻����ڴ�ռ�֮ǰ��Ԥ�ȵ��ô˺���.
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

VOID  _PthreadKeyCleanup (PLW_CLASS_TCB  ptcbDel, BOOL  bDestroy)
{
    LW_OBJECT_HANDLE  ulId = bDestroy ? ptcbDel->TCB_ulId : 0ul;

    __pthreadDataDeleteByThread(ulId, LW_NULL, ptcbDel);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** ��������: pthread_key_create
** ��������: ����һ�����ݼ�.
** �䡡��  : pkey          �� (����)
**           fdestructor   ɾ������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : key �ź�������Ϊ LW_OPTION_OBJECT_GLOBAL ����Ϊ key �Ѿ�ʹ����ԭʼ��Դ���л���.
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_key_create (pthread_key_t  *pkey, void (*fdestructor)(void *))
{
    INT              i;
    __PX_KEY_NODE   *pkeyn;
    
    if (pkey == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    API_ThreadOnce(&_G_bKeyDelHookAdd, __pthreadKeyOnce);               /*  ��װ�߳�ɾ���ص�            */
    
    pkeyn = (__PX_KEY_NODE *)__SHEAP_ALLOC(sizeof(__PX_KEY_NODE));      /*  �����ڵ��ڴ�                */
    if (pkeyn == LW_NULL) {
        errno = ENOMEM;
        return  (ENOMEM);
    }
    pkeyn->PKEYN_lId             = (long)pkeyn;
    pkeyn->PKEYN_pfuncDestructor = fdestructor;
    pkeyn->PKEYN_ulMutex         = API_SemaphoreMCreate("pxkey", LW_PRIO_DEF_CEILING, 
                                            LW_OPTION_WAIT_PRIORITY |
                                            LW_OPTION_INHERIT_PRIORITY |
                                            LW_OPTION_DELETE_SAFE |
                                            LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (pkeyn->PKEYN_ulMutex == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(pkeyn);
        errno = EAGAIN;
        return  (EAGAIN);
    }
    
    for (i = 0; i < __PX_KEY_THREAD_HASH_SIZE; i++) {
        pkeyn->PKEYN_plineKeyHeader[i] = LW_NULL;
    }

    __PX_LOCK();                                                        /*  ��ס posix ��               */
#if defined(__GNUC__) && (__GNUC__ > 4)                                 /*  �߰汾 GCC, OpenMP ����     */
    _List_Ring_Add_Last(&pkeyn->PKEYN_ringManage,
                        &_G_pringKeyHeader);                            /*  ���� key ������             */
#else
    _List_Ring_Add_Ahead(&pkeyn->PKEYN_ringManage,
                         &_G_pringKeyHeader);                           /*  ���� key ������             */
#endif
    __PX_UNLOCK();                                                      /*  ���� posix ��               */
    
    __resAddRawHook(&pkeyn->PKEYN_resraw, (VOIDFUNCPTR)pthread_key_delete, 
                    pkeyn, 0, 0, 0, 0, 0);                              /*  ������Դ������              */
    
    *pkey = (pthread_key_t)pkeyn;                                       /*  ���ڴ��ַ����Ϊ id         */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_key_delete
** ��������: ɾ��һ�����ݼ�. (ע��, ɾ������������ô���ʱ��װ����������)
** �䡡��  : key          ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_key_delete (pthread_key_t  key)
{
    __PX_KEY_NODE   *pkeyn = (__PX_KEY_NODE *)key;

    if (key == 0) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthreadDataDeleteByKey(pkeyn);                                    /*  ɾ��������� key ��ص����� */
    
    __PX_LOCK();                                                        /*  ��ס posix ��               */
    _List_Ring_Del(&pkeyn->PKEYN_ringManage,
                   &_G_pringKeyHeader);                                 /*  �� key ��������ɾ��         */
    __PX_UNLOCK();                                                      /*  ���� posix ��               */
    
    API_SemaphoreMDelete(&pkeyn->PKEYN_ulMutex);
    
    __resDelRawHook(&pkeyn->PKEYN_resraw);
    
    __SHEAP_FREE(pkeyn);                                                /*  �ͷ� key                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_setspecific
** ��������: �趨һ�����ݼ�ָ����ǰ�̵߳�˽������.
** �䡡��  : key          ��
**           pvalue       ֵ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_setspecific (pthread_key_t  key, const void  *pvalue)
{
    if (key == 0) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (__pthreadDataSet(key, pvalue));
}
/*********************************************************************************************************
** ��������: pthread_getspecific
** ��������: ��ȡһ�����ݼ�ָ����ǰ�̵߳�˽������.
** �䡡��  : key          ��
**           pvalue       ֵ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
void *pthread_getspecific (pthread_key_t  key)
{
    void   *pvalue = LW_NULL;

    if (key == 0) {
        errno = EINVAL;
        return  (LW_NULL);
    }
    
    __pthreadDataGet(key, &pvalue);
    
    return  (pvalue);
}
/*********************************************************************************************************
** ��������: pthread_key_cleanup_np
** ��������: ɾ����ǰ�߳����� key (Σ�ղ���)
** �䡡��  : destroy   �Ƿ�������ٺ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
void  pthread_key_cleanup_np (int destroy)
{
    LW_OBJECT_HANDLE  ulId;
    PLW_CLASS_TCB     ptcbCur;

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    ulId = destroy ? ptcbCur->TCB_ulId : 0ul;

    __pthreadDataDeleteByThread(ulId, LW_NULL, ptcbCur);
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
