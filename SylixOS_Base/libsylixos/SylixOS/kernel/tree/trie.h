/*********************************************************************************************************
**
** ��   ��   ��: trie.h
**
** ��   ��   ��: ��껽�
**
** �ļ���������: 2021 �� 5 �� 17 ��
**
** ��        ��: ǰ׺��, �ڵ�����Ϊascii���256���ַ�(�ɸ�)��
*********************************************************************************************************/

#ifndef LIBSYLIXOS_SYLIXOS_KERNEL_TREE_TRIE_H_
#define LIBSYLIXOS_SYLIXOS_KERNEL_TREE_TRIE_H_

#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include <string.h>
#include <stdio.h>

/* Ŀǰֻ֧��ascii��, ������256λ(���֮��Ҫ�ĵĻ�����Ҫ��most_frequently_used_char������)�� */
#define TRIE_CHILD_LENGTH  256
/*********************************************************************************************************
 ǰ׺������, ��֧��ascii�롣
*********************************************************************************************************/
typedef struct __trie_node {
    unsigned            isValid:1;                      /*  child�Ƿ񱻳�ʼ��      */
    unsigned            isEnd:1;                        /*  �Ƿ���ĳ��ָ��Ľ�β     */
    unsigned            frequence:15;                   /*  �˽ڵ㱻��ѯ��Ƶ��         */
    unsigned            maxFrequence:15;                /*  �ӽڵ������ʹ��Ƶ��     */
    char                mostFrequentlyUsedChar;         /*  �ʹ���ӽڵ�                */
    struct __trie_node *child;                          /*  �类��ʼ��, ����һ����Ϊ256��ָ������     */
} LW_TRIE_NODE;
typedef LW_TRIE_NODE    *PLW_TRIE_NODE;

/*********************************************************************************************************
  ǰ׺���ڵ�ṹ��ʼ��
*********************************************************************************************************/
#define INITIALIZE_TRIE_NODE(trieNode) do {                \
            (trieNode)->isValid = LW_FALSE;                \
            (trieNode)->isEnd = LW_FALSE;                  \
            (trieNode)->frequence = 0;                     \
            (trieNode)->maxFrequence = 0;                  \
        } while (0)
/*********************************************************************************************************
  ǰ׺������
*********************************************************************************************************/
PLW_TRIE_NODE   __trieGetRoot(VOID);
VOID            __trieNodeValidate(PLW_TRIE_NODE trieNode);
VOID            __trieInsert(PLW_TRIE_NODE trieNode, PCHAR sentence, int n);
PCHAR           __trieSearch(PLW_TRIE_NODE trieNode, PCHAR sentence, int n);
VOID            __trieDelete(PLW_TRIE_NODE trieNode);
VOID            __trieToFile(PLW_TRIE_NODE trieNode, FILE *file);
PLW_TRIE_NODE   __trieFromFile(FILE *file);

#endif /* LIBSYLIXOS_SYLIXOS_KERNEL_TREE_TRIE_H_ */
