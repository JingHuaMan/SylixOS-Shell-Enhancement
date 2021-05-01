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
** ��   ��   ��: elf_arch.h
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2010 �� 04 �� 17 ��
**
** ��        ��: ����ϵ�ṹ��ص� elf �ļ��ض�λ�ӿ�
*********************************************************************************************************/

#ifndef __ELF_ARCH_H
#define __ELF_ARCH_H

/*********************************************************************************************************
  ��ϵ�ṹ��ص��ض�λ�����ӿ�����
*********************************************************************************************************/

INT  archElfRGetJmpBuffItemLen(PVOID  pmodule);                         /*  ��ȡ��ת�����С            */

INT  archElfRelocateRela(PVOID       pmodule,
                         Elf_Rela   *prela,
                         Elf_Sym    *psym,
                         Elf_Addr    addrSymVal,
                         PCHAR       pcTargetSec,
                         PCHAR       pcBuffer,
                         size_t      stBuffLen);                        /*  RELA ���ض�λ               */

INT  archElfRelocateRel(PVOID        pmodule,
                        Elf_Rel     *prel,
                        Elf_Sym     *psym,
                        Elf_Addr     addrSymVal,
                        PCHAR        pcTargetSec,
                        PCHAR        pcBuffer,
                        size_t       stBuffLen);                        /*  REL ���ض�λ                */

INT  archElfGotInit(PVOID  pmodule);                                    /*  ��ʼ�� GOT ��               */

#if defined(LW_CFG_CPU_ARCH_C6X)
INT  archElfDSBTRemove(PVOID  pmodule);
#endif                                                                  /*  LW_CFG_CPU_ARCH_C6X         */

#endif                                                                  /*  __ELF_ARCH_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
