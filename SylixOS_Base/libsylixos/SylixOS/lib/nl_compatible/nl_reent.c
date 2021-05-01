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
** ��   ��   ��: nl_reent.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 08 �� 15 ��
**
** ��        ��: newlib fio ���ݲ�. (SylixOS �� VxWorks ����, ����ʹ���Լ��� libc ��)
                 �ܶ� gcc ʹ�� newlib ��Ϊ libc, �����Ŀ�Ҳ�������� newlib, ���� libstdc++ ��, 
                 SylixOS Ҫ��ʹ����Щ��, ������ṩһ�� newlib reent ���ݵĽӿ�.
                 
2012.11.09  lib_nlreent_init() ��������Ļ�������.
2014.11.08  ���� __getreent() ����, ֧�� newlib ʹ�� __DYNAMIC_REENT__ ����֧�� SMP ϵͳ.
2015.06.23  �����ڴ�ʹ����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
#include "../libc/stdio/lib_file.h"
/*********************************************************************************************************
  newlib compatible reent
*********************************************************************************************************/
typedef struct __nl_reent {
    /*
     *  ._errno ��ʹ��, Ϊ�˶����Ƽ���, �������ռλ������.
     *  ��Ϊ newlib errno �Ķ���Ϊ *__errno() �� SylixOS ��ͬ, �������ʹ���� errno ��һ���ᶨλ�� SylixOS
     *  �е� __errno() ����.
     */
    int         _errno_notuse;                                          /*  not use!                    */
    FILE       *_stdin, *_stdout, *_stderr;                             /*  ������׼�ļ�                */
} __NL_REENT;

typedef struct __lw_reent {
    FILE        _file[3];                                               /*  ������׼�ļ�                */
    /*
     *  ���±���Ϊ newlib ���ݲ���.
     */
    __NL_REENT  _nl_com;
} __LW_REENT;
/*********************************************************************************************************
  newlib compatible reent for all thread
*********************************************************************************************************/
static __LW_REENT _G_lwreentTbl[LW_CFG_MAX_THREADS];                    /*  ÿ������ӵ��һ�� reent    */
/*********************************************************************************************************
  newlib compatible reent (�˷�����֧�� SMP ϵͳ)
*********************************************************************************************************/
struct __nl_reent *LW_WEAK _impure_ptr;                                 /*  ��ǰ newlib reent ������    */
/*********************************************************************************************************
** ��������: __nlreent_swtich_hook
** ��������: newlib ʹ�� __DYNAMIC_REENT__ ����ʱ����֧�� SMP ϵͳ, �����ṩ��Ҫ����.
** �䡡��  : NONE
** �䡡��  : newlib �߳�������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
struct __nl_reent *__getreent (VOID)
{
    REGISTER PLW_CLASS_TCB   ptcbCur;
    REGISTER __LW_REENT     *plwreent;

    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    plwreent = &_G_lwreentTbl[ptcbCur->TCB_usIndex];
    
    return  (&plwreent->_nl_com);
}
/*********************************************************************************************************
** ��������: __nlreent_swtich_hook
** ��������: nl reent ������� hook
** �䡡��  : ulOldThread   �����������������߳�
**           ulNewThread   ��Ҫ���е��߳�.
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �����ڵ���ģʽ.
*********************************************************************************************************/
static VOID  __nlreent_swtich_hook (LW_OBJECT_HANDLE  ulOldThread, LW_OBJECT_HANDLE  ulNewThread)
{
    REGISTER __LW_REENT  *plwreent;
    
    plwreent    = &_G_lwreentTbl[_ObjectGetIndex(ulNewThread)];
    _impure_ptr = &plwreent->_nl_com;
}
/*********************************************************************************************************
** ��������: __nlreent_delete_hook
** ��������: nl reent ����ɾ�� hook
** �䡡��  : ulThread      �߳�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __nlreent_delete_hook (LW_OBJECT_HANDLE  ulThread)
{
    INT          i;
    __LW_REENT  *plwreent = &_G_lwreentTbl[_ObjectGetIndex(ulThread)];
    __NL_REENT  *pnlreent = &plwreent->_nl_com;
    
    /*
     *  ע��, �� stdin stdout �� stderr ���ض�����, �Ͳ��ùر���, ����ֻ�ر���ԭʼ������ std �ļ�.
     */
    for (i = 0; i < 3; i++) {
        if (plwreent->_file[i]._flags) {
            fclose_ex(&plwreent->_file[i], LW_TRUE, LW_FALSE);
        }
    }
    
    pnlreent->_stdin  = LW_NULL;
    pnlreent->_stdout = LW_NULL;
    pnlreent->_stderr = LW_NULL;
}
/*********************************************************************************************************
** ��������: lib_nlreent_init
** ��������: ��ʼ��ָ���̵߳� nl reent �ṹ
** �䡡��  : ulThread      �߳� ID
** �䡡��  : newlib ���� reent �ṹ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  lib_nlreent_init (LW_OBJECT_HANDLE  ulThread)
{
    static BOOL  bSwpHook = LW_FALSE;
    static BOOL  bDelHook = LW_FALSE;
    
    __LW_REENT  *plwreent = &_G_lwreentTbl[_ObjectGetIndex(ulThread)];
    __NL_REENT  *pnlreent = &plwreent->_nl_com;
    
    if (bSwpHook == LW_FALSE) {
        if (API_SystemHookAdd(__nlreent_swtich_hook, LW_OPTION_THREAD_SWAP_HOOK) == ERROR_NONE) {
            bSwpHook = LW_TRUE;
        }
    }
    
    if (bDelHook == LW_FALSE) {
        if (API_SystemHookAdd(__nlreent_delete_hook, LW_OPTION_THREAD_DELETE_HOOK) == ERROR_NONE) {
            bDelHook = LW_TRUE;
        }
    }
    
    pnlreent->_stdin  = &plwreent->_file[STDIN_FILENO];
    pnlreent->_stdout = &plwreent->_file[STDOUT_FILENO];
    pnlreent->_stderr = &plwreent->_file[STDERR_FILENO];
    
    __lib_newfile(pnlreent->_stdin);                                    /* ��׼�ļ���ʼ���������ڴ���� */
    __lib_newfile(pnlreent->_stdout);
    __lib_newfile(pnlreent->_stderr);
    
    /*
     *  stdin init flags
     */
    pnlreent->_stdin->_flags = __SRD;
#if LW_CFG_FIO_STDIN_LINE_EN > 0
    pnlreent->_stdin->_flags |= __SLBF;
#endif                                                                  /* LW_CFG_FIO_STDIN_LINE_EN     */

    /*
     *  stdout init flags
     */
    pnlreent->_stdout->_flags = __SWR;
#if LW_CFG_FIO_STDIN_LINE_EN > 0
    pnlreent->_stdout->_flags |= __SLBF;
#endif                                                                  /* LW_CFG_FIO_STDIN_LINE_EN     */

    /*
     *  stderr init flags
     */
    pnlreent->_stderr->_flags = __SWR;
#if LW_CFG_FIO_STDERR_LINE_EN > 0
    pnlreent->_stderr->_flags |= __SNBF;
#endif                                                                  /* LW_CFG_FIO_STDERR_LINE_EN    */

    pnlreent->_stdin->_file  = STDIN_FILENO;
    pnlreent->_stdout->_file = STDOUT_FILENO;
    pnlreent->_stderr->_file = STDERR_FILENO;
}
/*********************************************************************************************************
** ��������: lib_nlreent_stdfile
** ��������: ��ȡ��ǰ�̵߳� stdfile �ṹ
** �䡡��  : FileNo        �ļ���, 0, 1, 2
** �䡡��  : stdfile ָ���ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
FILE **lib_nlreent_stdfile (INT  FileNo)
{
    REGISTER __LW_REENT    *plwreent;
    REGISTER __NL_REENT    *pnlreent;
    REGISTER PLW_CLASS_TCB  ptcbCur;

    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        return  (LW_NULL);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    plwreent = &_G_lwreentTbl[_ObjectGetIndex(ptcbCur->TCB_ulId)];
    pnlreent = &plwreent->_nl_com;
    
    switch (FileNo) {
    
    case STDIN_FILENO:
        return  (&pnlreent->_stdin);
        
    case STDOUT_FILENO:
        return  (&pnlreent->_stdout);
        
    case STDERR_FILENO:
        return  (&pnlreent->_stderr);
        
    default:
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: lib_nlreent_static
** ��������: ��ȡ��ǰ�߳����õľ�̬ stdfile �ṹ
** �䡡��  : files[] ������׼�ļ�ָ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  lib_nlreent_static (FILE *files[])
{
    REGISTER __LW_REENT    *plwreent;
    REGISTER PLW_CLASS_TCB  ptcbCur;

    if (!files) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        return  (PX_ERROR);
    }

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    plwreent = &_G_lwreentTbl[_ObjectGetIndex(ptcbCur->TCB_ulId)];
    files[0] = &plwreent->_file[0];
    files[1] = &plwreent->_file[1];
    files[2] = &plwreent->_file[2];

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_FIO_LIB_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
