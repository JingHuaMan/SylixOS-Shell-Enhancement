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
** ��   ��   ��: regexp.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 09 �� 17 ��
**
** ��        ��: ������ʽ֧�ֿ�, ��Ҫ�ⲿ��֧��.
*********************************************************************************************************/

#ifndef __REGEXP_H
#define __REGEXP_H

/*
 * Definitions etc. for regexp(3) routines.
 *
 * Caveat:  this is V8 regexp(3) [actually, a reimplementation thereof],
 * not the System V one.
 */
#define NSUBEXP  10

typedef struct regexp {
	char *startp[NSUBEXP];
	char *endp[NSUBEXP];
	char regstart;		/* Internal use only. */
	char reganch;		/* Internal use only. */
	char *regmust;		/* Internal use only. */
	int regmlen;		/* Internal use only. */
	char program[1];	/* Unwarranted chumminess with compiler. */
} regexp;

#include <sys/cdefs.h>

__BEGIN_DECLS
regexp *regcomp(const char *);
int     regexec(const  regexp *, const char *);
void    regsub(const  regexp *, const char *, char *);
void    regerror(const char *);
__END_DECLS

#endif                                                                  /*  __REGEXP_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
