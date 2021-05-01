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
** ��   ��   ��: lib_file.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 09 �� 04 ��
**
** ��        ��: ���׼ C ��֧��, ��׼�ļ���������ȡ����

** BUG:
2009.02.15  ��׼�ļ��н��� stdin ��Ҫ������.
2009.05.28  ɾ���̱߳�׼�ļ�ʱ, ��Ҫ����� TCB ָ������, ��ֹ ThreadRestart ����.
2009.07.11  �ڴ�����׼�ļ�ʱ�������ɾ�����Ӻ���(����ʲô����).
2011.07.20  �ļ� FILE �ڴ����� malloc ����, ���������˳�ʱ, ���ͷŵ��ڴ�.
2012.12.07  �� FILE ������Դ������.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "lib_stdio.h"
#include "local.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
/*********************************************************************************************************
** ��������: __lib_newfile
** ��������: ����һ���ļ��ṹ
** �䡡��  : pfFile        �ļ�ָ�� (���Ϊ NULL ������)
** �䡡��  : �ļ�ָ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
FILE  *__lib_newfile (FILE  *pfFile)
{
    BOOL    bNeedRes = LW_TRUE;

    if (pfFile == LW_NULL) { 
        pfFile = (FILE *)lib_malloc(sizeof(FILE));                      /* �����ں��ļ�ָ��             */
        if (!pfFile) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (LW_NULL);
        }
    
    } else {
        bNeedRes = LW_FALSE;                                            /* ��׼ IO �ļ�                 */
    }
    
    lib_bzero(pfFile, sizeof(FILE));
    
    pfFile->_p        = LW_NULL;                                        /* no current pointer           */
    pfFile->_r        = 0;
    pfFile->_w        = 0;                                              /* nothing to read or write     */
    pfFile->_flags    = 1;                                              /* caller sets real flags       */
    pfFile->_file     = -1;                                             /* no file                      */
    pfFile->_bf._base = LW_NULL;                                        /* no buffer                    */
    pfFile->_bf._size = 0;
    pfFile->_lbfsize  = 0;                                              /* not line buffered            */
    pfFile->_ub._base = LW_NULL;                                        /* no ungetc buffer             */
    pfFile->_ub._size = 0;
    pfFile->_lb._base = LW_NULL;                                        /* no line buffer               */
    pfFile->_lb._size = 0;
    pfFile->_blksize  = 0;
    pfFile->_offset   = 0;
    pfFile->_cookie   = (void *)pfFile;
    
    pfFile->_close = __sclose;
    pfFile->_read  = __sread;
    pfFile->_seek  = __sseek;
    pfFile->_write = __swrite;
    
    if (bNeedRes) {
        __resAddRawHook(&pfFile->resraw, (VOIDFUNCPTR)fclose_ex, 
                        pfFile, (PVOID)LW_FALSE, (PVOID)LW_TRUE, 0, 0, 0);
    }
    
    return  (pfFile);
}
/*********************************************************************************************************
** ��������: __lib_delfile
** ��������: ɾ��һ���ļ��ṹ
** �䡡��  : �ļ�ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __lib_delfile (FILE  *pfFile)
{
    __resDelRawHook(&pfFile->resraw);

    if (pfFile) {
        lib_free(pfFile);
    }
}
/*********************************************************************************************************
** ��������: __lib_stdin
** ��������: ��õ�ǰ stdin �ļ�ָ��
** �䡡��  : NONE
** �䡡��  : �ļ�ָ��ĵ�ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
FILE  **__lib_stdin (VOID)
{
    return  (lib_nlreent_stdfile(STDIN_FILENO));
}
/*********************************************************************************************************
** ��������: __lib_stdout
** ��������: ��õ�ǰ stdout �ļ�ָ��
** �䡡��  : NONE
** �䡡��  : �ļ�ָ��ĵ�ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
FILE  **__lib_stdout (VOID)
{
    return  (lib_nlreent_stdfile(STDOUT_FILENO));
}
/*********************************************************************************************************
** ��������: __lib_stderr
** ��������: ��õ�ǰ stderr �ļ�ָ��
** �䡡��  : NONE
** �䡡��  : �ļ�ָ��ĵ�ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
FILE  **__lib_stderr (VOID)
{
    return  (lib_nlreent_stdfile(STDERR_FILENO));
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_FIO_LIB_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
