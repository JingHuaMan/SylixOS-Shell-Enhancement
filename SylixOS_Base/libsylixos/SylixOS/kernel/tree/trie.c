/*********************************************************************************************************
**
** ��   ��   ��: trie.c
**
** ��   ��   ��: ��껽�
**
** �ļ���������: 2021 �� 5 �� 17 ��
**
** ��        ��: ǰ׺��, �ڵ�����Ϊascii���256���ַ�(�ɸ�)��
*********************************************************************************************************/

#include "../SylixOS/kernel/tree/trie.h"

/*********************************************************************************************************
  ��������
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
** ��������: __trieGetRoot
** ��������: ��ʼ��ǰ׺����
** �䡡��  : NONE
** �䡡��  : ������ǰ׺����
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __trieNodeValidate
** ��������: ʹһ��ǰ׺���ڵ���Ч
** �䡡��  : trieNode ����Ч�ڵ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __trieInsert
** ��������: ��һ���ַ�������ǰ׺����
** �䡡��  : trieNode ��ǰ����λ��
**        sentence  ��ǰ�����ַ���
**        n         �ַ�������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID __trieInsert(PLW_TRIE_NODE trieNode, PCHAR sentence, int n)
{
    trieNode->frequence = trieNode->frequence ==
            32767 ? 32767 : trieNode->frequence + 1;                   /*  ʹ��Ƶ������                 */

    if (!trieNode->isValid) {                                         /*  ����Ľڵ��Ѿ������� */
        __trieNodeValidate(trieNode);
    }

    if (n == 0) {                                                       /*  �������                       */
        trieNode->isEnd = LW_TRUE;
        return;
    }

    __trieInsert(trieNode->child + *sentence, sentence + 1, n - 1);   /*  �ݹ����                        */

    if ((trieNode->child + *sentence)->frequence >= trieNode->maxFrequence) {
        trieNode->maxFrequence = (trieNode->child + *sentence)->frequence;
        trieNode->mostFrequentlyUsedChar = *sentence;
    }
}
/*********************************************************************************************************
** ��������: __trieSearch
** ��������: �ҳ�ǰ׺����ƥ�䵱ǰ�ַ����ĺ�׺�ַ���
** �䡡��  : trieNode ��ǰ����λ��
**        sentence  ��ǰ�����ַ���
**        n         �ַ�������
** �䡡��  : �������ƥ���ַ����򷵻�����һ����׺, ����������򷵻ؿ�ָ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PCHAR __trieSearch(PLW_TRIE_NODE trieNode, PCHAR sentence, int n)
{
    if (!trieNode->isValid) {                                                 /*  δ�ҵ�                                     */
        return LW_NULL;
    }

    if (n == 0) {                                                               /*  ���ҵ�                                     */
        INT bufferLen = 10, current_len = 0;
        PCHAR buffer = (PCHAR)__SHEAP_ALLOC(bufferLen * sizeof(CHAR));
        while (1) {
            if (current_len >= bufferLen - 1) {                                /*  ������ڵ�buffer������  */
                ENLARGE_LIST(buffer, bufferLen, CHAR, PCHAR);                  /*  ��������                                 */
            }
            if (!trieNode->isEnd) {                                           /*  �Ƿ������һ���ڵ�              */
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

    return __trieSearch(trieNode->child + *sentence, sentence + 1, n - 1);   /*  �ݹ����                                   */
}
/*********************************************************************************************************
** ��������: __recursiveDeleteTrie
** ��������: �ݹ�شӶ���ɾ��ǰ׺��
** �䡡��  : trieNode ǰ׺��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __trieDelete
** ��������: �ݹ�شӶ���ɾ��ǰ׺��
** �䡡��  : trieNode ǰ׺����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID __trieDelete(PLW_TRIE_NODE trieNode)
{
    __recursiveDeleteTrie(trieNode);
    __SHEAP_FREE(trieNode);
}
/*********************************************************************************************************
** ��������: __trieFromFile
** ��������: ���ļ��ж���ǰ׺��
** �䡡��  : file      ��ʷ��¼�ļ�
** �䡡��  : ������ǰ׺����
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __trieToFile
** ��������: ǰ׺��д���ļ�
** �䡡��  : trieNode ǰ׺����
**        file     ��ʷ��¼�ļ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
