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
  数组扩张
*********************************************************************************************************/
#define INITIAL_BUFFER_SIZE     10
#define ENLARGE_LIST(array_list, original_length_element, element_type, pointer_type) do {              \
            pointer_type new_buffer = (pointer_type)__SHEAP_ALLOC(                                      \
                                      3 * original_length_element / 2 * sizeof(element_type));          \
            memcpy(new_buffer, array_list, original_length_element * sizeof(element_type));             \
            __SHEAP_FREE(array_list);                                                                   \
            array_list = new_buffer;                                                                    \
            original_length_element = 3 * original_length_element / 2;                                  \
        } while (0)
/*********************************************************************************************************
** 函数名称: __trieGetRoot
** 功能描述: 初始化前缀树根
** 输　入  : NONE
** 输　出  : 新生成前缀树根
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PLW_TRIE_NODE __trieGetRoot(VOID)
{
    PLW_TRIE_NODE root;

    root = (PLW_TRIE_NODE)__SHEAP_ALLOC(TRIE_CHILD_LENGTH * sizeof(LW_TRIE_NODE));
    INITIALIZE_TRIE_NODE(root);
    __trieNodeValidate(root);

    return root;
}
/*********************************************************************************************************
** 函数名称: __trieNodeValidate
** 功能描述: 使一个前缀树节点生效
** 输　入  : trieNode 待生效节点
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID __trieNodeValidate(PLW_TRIE_NODE trieNode)
{
    trieNode->isValid = LW_TRUE;
    trieNode->child = (PLW_TRIE_NODE)__SHEAP_ALLOC(TRIE_CHILD_LENGTH * sizeof(LW_TRIE_NODE));
    PLW_TRIE_NODE temp = trieNode->child;
    int i;
    for (i = 0; i < TRIE_CHILD_LENGTH; i++) {
        INITIALIZE_TRIE_NODE(temp);
        temp++;
    }
}
/*********************************************************************************************************
** 函数名称: __trieInsert
** 功能描述: 将一个字符串插入前缀树中
** 输　入  : trieNode 当前插入位置
**        sentence  当前插入字符串
**        n         字符串长度
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID __trieInsert(PLW_TRIE_NODE trieNode, PCHAR sentence, int n)
{
    trieNode->frequence = trieNode->frequence ==
            32767 ? 32767 : trieNode->frequence + 1;                   /*  使用频率增加                 */

    if (!trieNode->isValid) {                                         /*  插入的节点已经到底了 */
        __trieNodeValidate(trieNode);
    }

    if (n == 0) {                                                       /*  插入完成                       */
        trieNode->isEnd = LW_TRUE;
        return;
    }

    __trieInsert(trieNode->child + *sentence, sentence + 1, n - 1);   /*  递归插入                        */

    if ((trieNode->child + *sentence)->frequence >= trieNode->maxFrequence) {
        trieNode->maxFrequence = (trieNode->child + *sentence)->frequence;
        trieNode->mostFrequentlyUsedChar = *sentence;
    }
}
/*********************************************************************************************************
** 函数名称: __trieSearch
** 功能描述: 找出前缀树中匹配当前字符串的后缀字符串
** 输　入  : trieNode 当前搜索位置
**        sentence  当前搜索字符串
**        n         字符串长度
** 输　出  : 如果存在匹配字符串则返回其中一个后缀, 如果不存在则返回空指针
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PCHAR __trieSearch(PLW_TRIE_NODE trieNode, PCHAR sentence, int n)
{
    if (!trieNode->isValid) {                                                 /*  未找到                                     */
        return LW_NULL;
    }

    if (n == 0) {                                                               /*  已找到                                     */
        INT bufferLen = 10, current_len = 0;
        PCHAR buffer = (PCHAR)__SHEAP_ALLOC(bufferLen * sizeof(CHAR));
        while (1) {
            if (current_len >= bufferLen - 1) {                                /*  如果现在的buffer不够长  */
                ENLARGE_LIST(buffer, bufferLen, CHAR, PCHAR);                  /*  扩张数组                                 */
            }
            if (!trieNode->isEnd) {                                           /*  是否是最后一个节点              */
                *(buffer + current_len++) = trieNode->mostFrequentlyUsedChar;
                trieNode = trieNode->child +
                        trieNode->mostFrequentlyUsedChar;
            } else {
                break;
            }
        }
        *(buffer + current_len) = '\0';
        return buffer;
    }

    return __trieSearch(trieNode->child + *sentence, sentence + 1, n - 1);   /*  递归查找                                   */
}
/*********************************************************************************************************
** 函数名称: __recursiveDeleteTrie
** 功能描述: 递归地从堆中删除前缀树
** 输　入  : trieNode 前缀树
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __recursiveDeleteTrie(PLW_TRIE_NODE trieNode)
{
    int i;
    for (i = 0; i < TRIE_CHILD_LENGTH; i++) {
        if ((trieNode->child + i)->isValid) {
            __recursiveDeleteTrie(trieNode->child + i);
        }
    }
    __SHEAP_FREE(trieNode->child);
}
/*********************************************************************************************************
** 函数名称: __trieDelete
** 功能描述: 递归地从堆中删除前缀树
** 输　入  : trieNode 前缀树根
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID __trieDelete(PLW_TRIE_NODE trieNode)
{
    __recursiveDeleteTrie(trieNode);
    __SHEAP_FREE(trieNode);
}
/*********************************************************************************************************
** 函数名称: __trieFromFile
** 功能描述: 从文件中读出前缀树
** 输　入  : file      历史记录文件
** 输　出  : 新生成前缀树根
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PLW_TRIE_NODE __trieFromFile(FILE *file)
{

    PLW_TRIE_NODE root;
    root = __trieGetRoot();

    BOOL turn = 0;
    CHAR currentPos;
    INT  pointer, iReturn;
    INT  sizes[2] = {INITIAL_BUFFER_SIZE, INITIAL_BUFFER_SIZE};
    INT  nums[2] = {1, 0};
    PLW_TRIE_NODE *lists[2];

    lists[0] = (PLW_TRIE_NODE *)__SHEAP_ALLOC(INITIAL_BUFFER_SIZE * sizeof(PLW_TRIE_NODE));
    lists[1] = (PLW_TRIE_NODE *)__SHEAP_ALLOC(INITIAL_BUFFER_SIZE * sizeof(PLW_TRIE_NODE));
    *lists[0] = root;

    CHAR buffer[6];

    while (nums[turn]) {
        for (pointer = 0; pointer < nums[turn]; pointer++) {
            currentPos = fgetc(file);
            if (currentPos == EOF) {
                return root;
            }

            while (currentPos) {
                __trieNodeValidate(lists[turn][pointer]->child + currentPos);
                iReturn = fread(buffer, sizeof(CHAR), 6, file);
                if (iReturn != 6) {
                    _DebugHandle(__ERRORMESSAGE_LEVEL, "History recording file has been edited abnormally.\r\n");
                    _ErrorHandle(ERROR_IOS_FILE_NOT_SUP);
                    return root;
                }
                (lists[turn][pointer]->child + currentPos)->isEnd = buffer[0];
                (lists[turn][pointer]->child + currentPos)->frequence =
                        buffer[1] + (((int) buffer[2]) << 8);
                (lists[turn][pointer]->child + currentPos)->maxFrequence =
                        buffer[3] + (((int) buffer[4]) << 8);
                (lists[turn][pointer]->child + currentPos)->mostFrequentlyUsedChar = buffer[5];
                if (nums[!turn] == sizes[!turn]) {
                    ENLARGE_LIST(lists[!turn], sizes[!turn], PLW_TRIE_NODE, PLW_TRIE_NODE *);
                }
                lists[!turn][nums[!turn]++] = lists[turn][pointer]->child + currentPos;

                currentPos = fgetc(file);
                if (currentPos == EOF) {
                    return root;
                }
            }
        }
        nums[turn] = 0;
        turn = !turn;
    }

    return root;
}
/*********************************************************************************************************
** 函数名称: __trieToFile
** 功能描述: 前缀树写入文件
** 输　入  : trieNode 前缀树根
**        file     历史记录文件
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID __trieToFile(PLW_TRIE_NODE trieNode, FILE *file)
{
    BOOL turn = 0;
    INT  pointer, index;
    INT  sizes[2] = {INITIAL_BUFFER_SIZE, INITIAL_BUFFER_SIZE};
    INT  nums[2] = {1, 0};
    PLW_TRIE_NODE *lists[2];
    PLW_TRIE_NODE  currentNode;

    lists[0] = (PLW_TRIE_NODE *)__SHEAP_ALLOC(INITIAL_BUFFER_SIZE * sizeof(PLW_TRIE_NODE));
    lists[1] = (PLW_TRIE_NODE *)__SHEAP_ALLOC(INITIAL_BUFFER_SIZE * sizeof(PLW_TRIE_NODE));
    *lists[0] = trieNode;

    CHAR node_info[7];

    while (nums[turn]) {
        for (pointer = 0; pointer < nums[turn]; pointer++) {
            currentNode = lists[turn][pointer];
            for (index = 0; index < TRIE_CHILD_LENGTH; index++) {
                if ((currentNode->child + index)->isValid) {
                    if (nums[!turn] == sizes[!turn]) {
                        ENLARGE_LIST(lists[!turn], sizes[!turn], PLW_TRIE_NODE, PLW_TRIE_NODE *);
                    }
                    lists[!turn][nums[!turn]++] = currentNode->child + index;
                    node_info[0] = index;
                    node_info[1] = (currentNode->child + index)->isEnd;
                    node_info[2] = (currentNode->child + index)->frequence & 0xFF;
                    node_info[3] = (currentNode->child + index)->frequence >> 8;
                    node_info[4] = (currentNode->child + index)->maxFrequence & 0xFF;
                    node_info[5] = (currentNode->child + index)->maxFrequence >> 8;
                    node_info[6] = (currentNode->child + index)->mostFrequentlyUsedChar;
                    fwrite(node_info, sizeof(CHAR), 7, file);
                }
            }
            fputc(0, file);
        }
        nums[turn] = 0;
        turn = !turn;
    }
}
