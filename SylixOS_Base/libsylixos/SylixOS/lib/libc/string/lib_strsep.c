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
** ��   ��   ��: lib_strsep.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 08 �� 22 ��
**
** ��        ��: ��
*********************************************************************************************************/
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: lib_strsep
** ��������: Split a string into tokens
** �䡡��  : s     The string to be searched
**           ct    The characters to search for
** �䡡��  : It returns empty tokens, too, behaving exactly like the libc function
             of that name. In fact, it was stolen from glibc2 and de-fancy-fied.
             Same semantics, slimmer shape. ;)
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
char *lib_strsep (char **s, const char *ct)
{
    char *sbegin = *s;
    char *end;

    if (sbegin == LW_NULL) {
        return  (NULL);
    }

    end = lib_strpbrk(sbegin, ct);
    if (end) {
        *end++ = '\0';
    }
    *s = end;

    return  (sbegin);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
