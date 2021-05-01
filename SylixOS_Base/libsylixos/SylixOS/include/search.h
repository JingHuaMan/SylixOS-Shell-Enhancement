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
** ��   ��   ��: search.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 09 �� 21 ��
**
** ��        ��: ������.
*********************************************************************************************************/

#ifndef __SEARCH_H
#define __SEARCH_H

#include <sys/cdefs.h>
#include <stddef.h>

typedef struct entry {
	char *key;
	void *data;
} ENTRY;

typedef enum {
	FIND, ENTER
} ACTION;

typedef enum {
	preorder,
	postorder,
	endorder,
	leaf
} VISIT;

__BEGIN_DECLS
int      hcreate(size_t);
void	 hdestroy(void);
ENTRY	*hsearch(ENTRY, ACTION);

void	*lfind(const void *, const void *, size_t *, size_t,
		      int (*)(const void *, const void *));
void	*lsearch(const void *, void *, size_t *, size_t,
		      int (*)(const void *, const void *));
void	 insque(void *, void *);
void	 remque(void *);

void	*tdelete(const void * __restrict, void ** __restrict,
		      int (*)(const void *, const void *));
void	*tfind(const void *, void * const *,
		      int (*)(const void *, const void *));
void	*tsearch(const void *, void **, 
		      int (*)(const void *, const void *));
void	 twalk(const void *, void (*)(const void *, VISIT, int));
__END_DECLS

#endif                                                                  /*  __SEARCH_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
