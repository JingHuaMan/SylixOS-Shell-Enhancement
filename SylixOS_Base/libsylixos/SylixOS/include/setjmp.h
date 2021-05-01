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
** ��   ��   ��: setjmp.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 10 �� 23 ��
**
** ��        ��: ���� C ��.
*********************************************************************************************************/

#ifndef __SETJMP_H
#define __SETJMP_H

#ifndef __SYLIXOS_H
#include <SylixOS.h>
#endif

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

__BEGIN_NAMESPACE_STD

struct __lw_jmp_buf_tag {
    LW_STACK    __saved_regs[ARCH_JMP_BUF_WORD_SIZE];                   /*  REGs + SP                   */
    sigset_t    __saved_mask;
    int         __mask_was_saved;
};

typedef struct __lw_jmp_buf_tag   jmp_buf[1];
typedef struct __lw_jmp_buf_tag   sigjmp_buf[1];

void   longjmp(jmp_buf, int);
void   siglongjmp(sigjmp_buf, int);
void   _longjmp(jmp_buf, int);

int    setjmp(jmp_buf);
int    sigsetjmp(sigjmp_buf, int);
int    _setjmp(jmp_buf);

__END_NAMESPACE_STD

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __SETJMP_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
