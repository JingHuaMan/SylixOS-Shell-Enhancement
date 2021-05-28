/*********************************************************************************************************
**
** 文   件   名: trie.h
**
** 创   建   人: 李昊锦
**
** 文件创建日期: 2021 年 5 月 17 日
**
** 描        述: 前缀树, 节点内容为ascii码的256个字符(可改)。
*********************************************************************************************************/

#ifndef LIBSYLIXOS_SYLIXOS_KERNEL_TREE_TRIE_H_
#define LIBSYLIXOS_SYLIXOS_KERNEL_TREE_TRIE_H_

#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include <string.h>
#include <stdio.h>

/* 目前只支持ascii码, 所以是256位(如果之后要改的话还需要改most_frequently_used_char的声明)。 */
#define TRIE_CHILD_LENGTH  256
/*********************************************************************************************************
 前缀树类型, 仅支持ascii码。
*********************************************************************************************************/
typedef struct __trie_node {
    unsigned            isValid:1;                      /*  child是否被初始化      */
    unsigned            isEnd:1;                        /*  是否是某个指令的结尾     */
    unsigned            frequence:15;                   /*  此节点被查询的频率         */
    unsigned            maxFrequence:15;                /*  子节点中最大使用频率     */
    char                mostFrequentlyUsedChar;         /*  最常使用子节点                */
    struct __trie_node *child;                          /*  如被初始化, 则是一个长为256的指针数组     */
} LW_TRIE_NODE;
typedef LW_TRIE_NODE    *PLW_TRIE_NODE;

/*********************************************************************************************************
  前缀树节点结构初始化
*********************************************************************************************************/
#define INITIALIZE_TRIE_NODE(trieNode) do {                \
            (trieNode)->isValid = LW_FALSE;                \
            (trieNode)->isEnd = LW_FALSE;                  \
            (trieNode)->frequence = 0;                     \
            (trieNode)->maxFrequence = 0;                  \
        } while (0)
/*********************************************************************************************************
  前缀树操作
*********************************************************************************************************/
PLW_TRIE_NODE   __trieGetRoot(VOID);
VOID            __trieNodeValidate(PLW_TRIE_NODE trieNode);
VOID            __trieInsert(PLW_TRIE_NODE trieNode, PCHAR sentence, int n);
PCHAR           __trieSearch(PLW_TRIE_NODE trieNode, PCHAR sentence, int n);
VOID            __trieDelete(PLW_TRIE_NODE trieNode);
VOID            __trieToFile(PLW_TRIE_NODE trieNode, FILE *file);
PLW_TRIE_NODE   __trieFromFile(FILE *file);

#endif /* LIBSYLIXOS_SYLIXOS_KERNEL_TREE_TRIE_H_ */
