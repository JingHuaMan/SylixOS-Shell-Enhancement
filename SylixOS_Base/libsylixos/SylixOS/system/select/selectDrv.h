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
** ��   ��   ��: selectDrv.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 11 �� 07 ��
**
** ��        ��:  IO ϵͳ select ��ϵͳ����������Ҫʹ�õĺ�������.

** ע��:�����ʹ�����º���,��Щ��������ʹ�κ��豸����������֧�� select() ��������.

** BUG
2007.12.11 �����˴��󼤻�Ĵ���.
*********************************************************************************************************/

#ifndef __SELECTDRV_H 
#define __SELECTDRV_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0)

LW_API VOID         API_SelWakeupListInit(PLW_SEL_WAKEUPLIST  pselwulList);
LW_API VOID         API_SelWakeupListTerm(PLW_SEL_WAKEUPLIST  pselwulList);
LW_API UINT         API_SelWakeupListLen(PLW_SEL_WAKEUPLIST  pselwulList);

LW_API LW_SEL_TYPE  API_SelWakeupType(PLW_SEL_WAKEUPNODE   pselwunNode);
LW_API VOID         API_SelWakeup(PLW_SEL_WAKEUPNODE   pselwunNode);
LW_API VOID         API_SelWakeupError(PLW_SEL_WAKEUPNODE   pselwunNode);

#if LW_CFG_SEMFD_EN > 0
LW_API VOID         API_SelWakeupFifo(PLW_SEL_WAKEUPLIST  pselwulList, LW_SEL_TYPE  seltyp);
LW_API VOID         API_SelWakeupPrio(PLW_SEL_WAKEUPLIST  pselwulList, LW_SEL_TYPE  seltyp);
#endif                                                                  /*  LW_CFG_SEMFD_EN > 0         */

LW_API VOID         API_SelWakeupAll(PLW_SEL_WAKEUPLIST   pselwulList, LW_SEL_TYPE  seltyp);
LW_API VOID         API_SelWakeupAllByFlags(PLW_SEL_WAKEUPLIST  pselwulList, UINT  uiFlags);
LW_API VOID         API_SelWakeupTerm(PLW_SEL_WAKEUPLIST   pselwulList);

LW_API INT          API_SelNodeAdd(PLW_SEL_WAKEUPLIST   pselwulList, 
                                   PLW_SEL_WAKEUPNODE   pselwunNode);
LW_API INT          API_SelNodeDelete(PLW_SEL_WAKEUPLIST   pselwulList, 
                                      PLW_SEL_WAKEUPNODE   pselwunDelete);

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SELECT_EN > 0)      */
/*********************************************************************************************************
  ��������ʹ�ú�
*********************************************************************************************************/

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0)

#define SEL_WAKE_UP_LIST_INIT(pselwulList)      API_SelWakeupListInit(pselwulList)
#define SEL_WAKE_UP_LIST_TERM(pselwulList)      API_SelWakeupListTerm(pselwulList)
#define SEL_WAKE_UP_LIST_LEN(pselwulList)       API_SelWakeupListLen(pselwulList)

#define SEL_WAKE_UP_TYPE(pselwunNode)           API_SelWakeupType(pselwunNode)
#define SEL_WAKE_UP(pselwunNode)                API_SelWakeup(pselwunNode)
#define SEL_WAKE_UP_ERROR(pselwunNode)          API_SelWakeupError(pselwunNode)

#define SEL_WAKE_UP_ALL(pselwulList, seltyp)                \
        API_SelWakeupAll(pselwulList, seltyp)
        
#define SEL_WAKE_UP_ALL_BY_FLAGS(pselwulList, uiFlags)      \
        API_SelWakeupAllByFlags(pselwulList, uiFlags)
        
#if LW_CFG_SEMFD_EN > 0
#define SEL_WAKE_UP_FIFO(pselwulList, seltyp)               \
        API_SelWakeupFifo(pselwulList, seltyp)

#define SEL_WAKE_UP_PRIO(pselwulList, seltyp)               \
        API_SelWakeupPrio(pselwulList, seltyp)
#endif                                                                  /*  LW_CFG_SEMFD_EN > 0         */

#define SEL_WAKE_UP_TERM(pselwulList)                       \
        API_SelWakeupTerm(pselwulList)

#define SEL_WAKE_NODE_ADD(pselwulList, pselwunNode)         \
        API_SelNodeAdd(pselwulList, pselwunNode)
        
#define SEL_WAKE_NODE_DELETE(pselwulList, pselwunDelete)    \
        API_SelNodeDelete(pselwulList, pselwunDelete)
        
#else                                                                   /*  �ü��� select() ��          */

static LW_INLINE VOID SEL_WAKE_UP_LIST_INIT(PLW_SEL_WAKEUPLIST  pselwulList)
{
    return;
}
static LW_INLINE VOID SEL_WAKE_UP_LIST_TERM(PLW_SEL_WAKEUPLIST  pselwulList)
{
    return;
}
static LW_INLINE UINT SEL_WAKE_UP_LIST_LEN(PLW_SEL_WAKEUPLIST  pselwulList)
{
    return  (0);
}
static LW_INLINE LW_SEL_TYPE SEL_WAKE_UP_TYPE(PLW_SEL_WAKEUPNODE   pselwunNode)
{
    return  (SELEXCEPT);
}
static LW_INLINE VOID SEL_WAKE_UP(PLW_SEL_WAKEUPNODE   pselwunNode)
{
    return;
}
static LW_INLINE VOID SEL_WAKE_UP_ERROR(PLW_SEL_WAKEUPNODE   pselwunNode)
{
    return;
}
static LW_INLINE VOID SEL_WAKE_UP_ALL(PLW_SEL_WAKEUPLIST   pselwulList, LW_SEL_TYPE  seltyp)
{
    return;
}
static LW_INLINE VOID SEL_WAKE_UP_ALL_BY_FLAGS(PLW_SEL_WAKEUPLIST   pselwulList, UINT  uiFlags)
{
    return;
}
static LW_INLINE VOID SEL_WAKE_UP_TERM(PLW_SEL_WAKEUPLIST   pselwulList)
{
    return;
}
static LW_INLINE INT  SEL_WAKE_NODE_ADD(PLW_SEL_WAKEUPLIST   pselwulList, 
                                        PLW_SEL_WAKEUPNODE   pselwunNode)
{
    return  (PX_ERROR);
}
static LW_INLINE INT  SEL_WAKE_NODE_DELETE(PLW_SEL_WAKEUPLIST   pselwulList, 
                                           PLW_SEL_WAKEUPNODE   pselwunDelete)
{
    return  (PX_ERROR);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SELECT_EN > 0)      */
#endif                                                                  /*  __SELECTDRV_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
