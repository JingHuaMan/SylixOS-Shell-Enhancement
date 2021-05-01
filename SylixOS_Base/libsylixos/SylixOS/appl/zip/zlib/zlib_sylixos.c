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
** ��   ��   ��: zlib_sylixos.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 01 �� 16 ��
**
** ��        ��: ����ϵͳ zlib ���Խӿ�.

** BUG:
2010.02.08  ʹ��ͬ������ģʽִ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  �������� (ע��, zlib �Զ�ջ�����ķǳ���, ����ʹ��ʱһ��Ҫ�Ӵ��ջ)
*********************************************************************************************************/
int minigzip_main(int argc, char **argv);
int zlib_main(int argc, char* argv[]);
/*********************************************************************************************************
** ��������: luaShellInit
** ��������: ��ʼ�� zlib shell �ӿ�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  zlibShellInit (VOID)
{
#if LW_CFG_SHELL_EN > 0
    /*
     *  ע�� lua ��������������.
     */
    API_TShellKeywordAddEx("gzip",  minigzip_main, LW_OPTION_KEYWORD_SYNCBG);
    API_TShellFormatAdd("gzip",     " [-c] [-d] [-f] [-h] [-r] [-1 to -9] [files...]");
    API_TShellHelpAdd("gzip",       "Usage:  gzip [-d] [-f] [-h] [-r] [-1 to -9] [files...]\n"
                                    "       -c : write to standard output\n"
                                    "       -d : decompress\n"
                                    "       -f : compress with Z_FILTERED\n"
                                    "       -h : compress with Z_HUFFMAN_ONLY\n"
                                    "       -r : compress with Z_RLE\n"
                                    "       -1 to -9 : compression level\n");
    
    API_TShellKeywordAddEx("zlib", zlib_main, LW_OPTION_KEYWORD_SYNCBG);
    API_TShellFormatAdd("zlib",    " [output.gz  [input.gz]]");
    API_TShellHelpAdd("zlib",      "zlib test.");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
