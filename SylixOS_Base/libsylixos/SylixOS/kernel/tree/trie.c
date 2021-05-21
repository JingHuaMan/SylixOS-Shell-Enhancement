/*********************************************************************************************************
**
** 文   件   名: trie.c
**
** 创   建   人: 李昊锦
**
** 文件创建日期: 2021 年 5 月 17 日
**
** 描        述: 前缀树, 节点内容为ascii码的256个字符(可改)。
*********************************************************************************************************/

#include "../SylixOS/kernel/tree/trie.h"

/*********************************************************************************************************
** 函数名称: __trie_node_validate
** 功能描述: 使一个前缀树节点生效
** 输　入  : trie_node 待生效节点
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID __trie_node_validate(PLW_TRIE_NODE trie_node)
{
    trie_node->is_valid = LW_TRUE;
    trie_node->child = (PLW_TRIE_NODE)__SHEAP_ALLOC(TRIE_CHILD_LENGTH * sizeof(LW_TRIE_NODE));
    PLW_TRIE_NODE temp = trie_node->child;
    int i;
    for (i = 0; i < TRIE_CHILD_LENGTH; i++) {
        INITIALIZE_TRIE_NODE(temp);
        temp++;
    }
}
/*********************************************************************************************************
** 函数名称: __trie_insert
** 功能描述: 将一个字符串插入前缀树中
** 输　入  : trie_node 当前插入位置
**        sentence  当前插入字符串
**        n         字符串长度
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID __trie_insert(PLW_TRIE_NODE trie_node, PCHAR sentence, int n)
{
    trie_node->frequence = trie_node->frequence ==
            65535 ? 65535 : trie_node->frequence + 1;                   /*  使用频率增加                 */

    if (n == 0) {                                                       /*  插入完成                       */
        trie_node->is_end = LW_TRUE;
        return;
    }

    if (!trie_node->is_valid) {                                         /*  插入的节点已经到底了 */
        __trie_node_validate(trie_node);
    }

    __trie_insert(trie_node->child + *sentence, sentence + 1, n - 1);   /*  递归插入                        */

    if ((trie_node->child + *sentence)->frequence >= trie_node->max_frequence) {
        trie_node->max_frequence = (trie_node->child + *sentence)->frequence;
        trie_node->most_frequently_used_char = *sentence;
    }
}
/*********************************************************************************************************
** 函数名称: __trie_search
** 功能描述: 找出前缀树中匹配当前字符串的后缀字符串
** 输　入  : trie_node 当前搜索位置
**        sentence  当前搜索字符串
**        n         字符串长度
** 输　出  : 如果存在匹配字符串则返回其中一个后缀, 如果不存在则返回空指针
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PCHAR __trie_search(PLW_TRIE_NODE trie_node, PCHAR sentence, int n)
{
    if (!trie_node->is_valid) {                                                 /*  未找到                             */
        return LW_NULL;
    }

    if (n == 0) {                                                               /*  已找到                            */
        INT buffer_len = 10, current_len = 0;
        PCHAR buffer = (PCHAR)__SHEAP_ALLOC(buffer_len * sizeof(CHAR));
        while (1) {
            if (current_len >= buffer_len - 1) {                                /*  如果现在的buffer不够长  */
                PCHAR new_buffer = (PCHAR)__SHEAP_ALLOC(
                        3 * buffer_len / 2 * sizeof(CHAR));                     /*  扩大1.5倍                     */
                memcpy(new_buffer, buffer, (buffer_len - 1) * sizeof(CHAR));
                __SHEAP_FREE(buffer);
                buffer = new_buffer;
                buffer_len = 3 * buffer_len / 2;
            }
            if (!trie_node->is_end) {                                           /*  是否是最后一个节点       */
                *(buffer + current_len++) = trie_node->most_frequently_used_char;
                trie_node = trie_node->child +
                        trie_node->most_frequently_used_char;
            } else {
                break;
            }
        }
        *(buffer + current_len) = '\0';
        return buffer;
    }

    return __trie_search(trie_node->child + *sentence, sentence + 1, n - 1);   /*  递归查找                        */
}
