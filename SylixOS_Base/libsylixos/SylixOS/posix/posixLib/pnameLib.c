/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: pnameLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 30 ��
**
** ��        ��: posix �ڲ���������.

** BUG:
2010.01.07  ����֧�� proc �ļ��Ľӿ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/posixLib.h"                                        /*  �Ѱ�������ϵͳͷ�ļ�        */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
/*********************************************************************************************************
  hash ����
*********************************************************************************************************/
INT  __hashHorner(CPCHAR  pcKeyword, INT  iTableSize);                  /*  ���ɶ���ʽ                  */

LW_LIST_LINE_HEADER          _G_plinePxNameNodeHash[__PX_NAME_NODE_HASH_SIZE];
UINT                         _G_uiNamedNodeCounter = 0;
/*********************************************************************************************************
** ��������: __pxnameSeach
** ��������: �� posix ����ϵͳ��ѯָ���Ľڵ�.
** �䡡��  : pcName        ����
**           iHash         ��ϣ�����
** �䡡��  : �ҵ��Ľڵ�ָ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
__PX_NAME_NODE  *__pxnameSeach (CPCHAR  pcName, INT  iHash)
{
    PLW_LIST_LINE      plineTemp;
    __PX_NAME_NODE    *pxnode;
    
    if (iHash < 0) {
        iHash = __hashHorner(pcName, __PX_NAME_NODE_HASH_SIZE);
    }
    
    for (plineTemp  = _G_plinePxNameNodeHash[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pxnode = _LIST_ENTRY(plineTemp, __PX_NAME_NODE, PXNODE_lineManage);
        if (lib_strcmp(pxnode->PXNODE_pcName, pcName) == 0) {
            break;
        }
    }
    
    if (plineTemp) {
        return  (pxnode);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: __pxnameAdd
** ��������: ����һ�����ֽڵ�.
** �䡡��  : pxnode        ���ֽڵ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __pxnameAdd (__PX_NAME_NODE  *pxnode)
{
    INT                iHash;
    
    if (pxnode == LW_NULL) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    iHash = __hashHorner(pxnode->PXNODE_pcName, __PX_NAME_NODE_HASH_SIZE);
                                                                        /*  ȷ�� hash �����            */
    if (__pxnameSeach(pxnode->PXNODE_pcName, iHash)) {
        errno = EEXIST;                                                 /*  �Ѿ����ڴ˽ڵ�              */
        return  (PX_ERROR);
    }

    API_AtomicSet(0, &pxnode->PXNODE_atomic);                           /*  ��ʼ��������                */
    
    _List_Line_Add_Ahead(&pxnode->PXNODE_lineManage,
                         &_G_plinePxNameNodeHash[iHash]);               /*  ���� hash ��                */
    _G_uiNamedNodeCounter++;
                         
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __pxnameDel
** ��������: ɾ��һ�����ֽڵ�.
** �䡡��  : pcName            ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __pxnameDel (CPCHAR  pcName)
{
    __PX_NAME_NODE    *pxnode;
    INT                iHash;

    if (pcName == LW_NULL) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    iHash = __hashHorner(pcName, __PX_NAME_NODE_HASH_SIZE);             /*  ȷ�� hash �����            */
    
    pxnode = __pxnameSeach(pcName, iHash);
    if (pxnode == LW_NULL) {
        errno = ENOENT;                                                 /*  û�ж�Ӧ�ڵ�                */
        return  (PX_ERROR);
    }
    
    _List_Line_Del(&pxnode->PXNODE_lineManage,
                   &_G_plinePxNameNodeHash[iHash]);                     /*  �� hash ����ɾ��            */
    _G_uiNamedNodeCounter--;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __pxnameDelByNode
** ��������: ɾ��һ�����ֽڵ�.
** �䡡��  : pxnode            ���ֽڵ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __pxnameDelByNode (__PX_NAME_NODE  *pxnode)
{
    INT  iHash;

    if (pxnode == LW_NULL) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    iHash = __hashHorner(pxnode->PXNODE_pcName, 
                         __PX_NAME_NODE_HASH_SIZE);                     /*  ȷ�� hash �����            */
    
    _List_Line_Del(&pxnode->PXNODE_lineManage,
                   &_G_plinePxNameNodeHash[iHash]);                     /*  �� hash ����ɾ��            */
    _G_uiNamedNodeCounter--;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __pxnameGet
** ��������: ��ȡһ�����ֽڵ�.
** �䡡��  : pcName            ����
**           ppvData           �ڵ�����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __pxnameGet (CPCHAR  pcName, PVOID  *ppvData)
{
    __PX_NAME_NODE    *pxnode;

    if ((pcName == LW_NULL) || (ppvData == LW_NULL)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    pxnode = __pxnameSeach(pcName, -1);
    if (pxnode == LW_NULL) {
        errno = ENOENT;                                                 /*  û�ж�Ӧ�ڵ�                */
        return  (PX_ERROR);
    }
    
    *ppvData = pxnode->PXNODE_pvData;
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
