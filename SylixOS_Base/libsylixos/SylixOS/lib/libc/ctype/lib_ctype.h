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
** ��   ��   ��: lib_ctype.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 02 �� 07 ��
**
** ��        ��: ������
*********************************************************************************************************/

#ifndef __LIB_CTYPE_H
#define __LIB_CTYPE_H

extern const unsigned char	*_ctype_;

int     lib_isalnum(int);
int     lib_isalpha(int);
int     lib_iscntrl(int);
int     lib_isdigit(int);
int     lib_isgraph(int);
int     lib_islower(int);
int     lib_isprint(int);
int     lib_ispunct(int);
int     lib_isspace(int);
int     lib_isupper(int);
int     lib_isxdigit(int);
int     lib_isascii(int);
int     lib_toascii(int);
int     lib_isblank(int);

#endif                                                                  /*  __LIB_CTYPE_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
