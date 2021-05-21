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
** ��������: __trie_node_validate
** ��������: ʹһ��ǰ׺���ڵ���Ч
** �䡡��  : trie_node ����Ч�ڵ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __trie_insert
** ��������: ��һ���ַ�������ǰ׺����
** �䡡��  : trie_node ��ǰ����λ��
**        sentence  ��ǰ�����ַ���
**        n         �ַ�������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID __trie_insert(PLW_TRIE_NODE trie_node, PCHAR sentence, int n)
{
    trie_node->frequence = trie_node->frequence ==
            65535 ? 65535 : trie_node->frequence + 1;                   /*  ʹ��Ƶ������                 */

    if (n == 0) {                                                       /*  �������                       */
        trie_node->is_end = LW_TRUE;
        return;
    }

    if (!trie_node->is_valid) {                                         /*  ����Ľڵ��Ѿ������� */
        __trie_node_validate(trie_node);
    }

    __trie_insert(trie_node->child + *sentence, sentence + 1, n - 1);   /*  �ݹ����                        */

    if ((trie_node->child + *sentence)->frequence >= trie_node->max_frequence) {
        trie_node->max_frequence = (trie_node->child + *sentence)->frequence;
        trie_node->most_frequently_used_char = *sentence;
    }
}
/*********************************************************************************************************
** ��������: __trie_search
** ��������: �ҳ�ǰ׺����ƥ�䵱ǰ�ַ����ĺ�׺�ַ���
** �䡡��  : trie_node ��ǰ����λ��
**        sentence  ��ǰ�����ַ���
**        n         �ַ�������
** �䡡��  : �������ƥ���ַ����򷵻�����һ����׺, ����������򷵻ؿ�ָ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PCHAR __trie_search(PLW_TRIE_NODE trie_node, PCHAR sentence, int n)
{
    if (!trie_node->is_valid) {                                                 /*  δ�ҵ�                             */
        return LW_NULL;
    }

    if (n == 0) {                                                               /*  ���ҵ�                            */
        INT buffer_len = 10, current_len = 0;
        PCHAR buffer = (PCHAR)__SHEAP_ALLOC(buffer_len * sizeof(CHAR));
        while (1) {
            if (current_len >= buffer_len - 1) {                                /*  ������ڵ�buffer������  */
                PCHAR new_buffer = (PCHAR)__SHEAP_ALLOC(
                        3 * buffer_len / 2 * sizeof(CHAR));                     /*  ����1.5��                     */
                memcpy(new_buffer, buffer, (buffer_len - 1) * sizeof(CHAR));
                __SHEAP_FREE(buffer);
                buffer = new_buffer;
                buffer_len = 3 * buffer_len / 2;
            }
            if (!trie_node->is_end) {                                           /*  �Ƿ������һ���ڵ�       */
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

    return __trie_search(trie_node->child + *sentence, sentence + 1, n - 1);   /*  �ݹ����                        */
}
