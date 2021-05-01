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
** ��   ��   ��: sysHookListLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 04 �� 01 ��
**
** ��        ��: ϵͳ���Ӻ������ڲ���

** BUG
2007.11.07  _SysCreateHook() ���뽨��ʱ�� option ѡ��.
2007.11.13  ʹ���������������������ȫ��װ.
2007.11.18  ����ע��.
2008.03.02  ����ϵͳ���������ص�.
2008.03.10  ʹ�ð�ȫ���ƵĻص�������.
2009.04.09  �޸Ļص�����.
2010.08.03  ÿ���ص����ƿ�ʹ�ö����� spinlock.
2012.09.23  ��ʼ��ʱ��������ϵͳ�ص�, ���ǵ��û���һ�ε��� hook add ����ʱ�ٰ�װ.
2013.03.16  ������̴�����ɾ���ص�.
2013.12.12  �ж� hook ����������Ƕ�ײ�������.
2014.01.07  �������� hook ��������.
2015.05.15  ��ͬ�� hook ʹ�ò�ͬ����.
2016.06.26  ���� hook Ϊ�������, ͬʱ delete hook ʱ�������ն�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#define  __SYSHOOKLIST_MAIN_FILE
#include "sysHookList.h"
/*********************************************************************************************************
  HOOK ������ģ�� (spinlock �汾)
*********************************************************************************************************/
#define __HOOK_TEMPLATE_SPIN(phookcb, param) \
        do { \
            INT             i; \
            INTREG          iregInterLevel; \
            LW_HOOK_FUNC    pfuncHook; \
             \
            LW_SPIN_LOCK_QUICK(&((phookcb)->HOOKCB_slHook), &iregInterLevel); \
            for (i = (phookcb->HOOKCB_uiCnt - 1); i >= 0; i--) { \
                if ((phookcb)->HOOKCB_pfuncnode[i]) { \
                    pfuncHook = (phookcb)->HOOKCB_pfuncnode[i]->FUNCNODE_hookfunc; \
                    LW_SPIN_UNLOCK_QUICK(&((phookcb)->HOOKCB_slHook), iregInterLevel); \
                    pfuncHook param; \
                    LW_SPIN_LOCK_QUICK(&((phookcb)->HOOKCB_slHook), &iregInterLevel); \
                } \
            } \
            LW_SPIN_UNLOCK_QUICK(&((phookcb)->HOOKCB_slHook), iregInterLevel); \
        } while (0)
/*********************************************************************************************************
  HOOK ������ģ�� (������)
*********************************************************************************************************/
#define __HOOK_TEMPLATE(phookcb, param) \
        do { \
            INT             i; \
            LW_HOOK_FUNC    pfuncHook; \
             \
            for (i = (phookcb->HOOKCB_uiCnt - 1); i >= 0; i--) { \
                if ((phookcb)->HOOKCB_pfuncnode[i]) { \
                    pfuncHook = (phookcb)->HOOKCB_pfuncnode[i]->FUNCNODE_hookfunc; \
                    pfuncHook param; \
                } \
            } \
        } while (0)
/*********************************************************************************************************
** ��������: _HookAdjTemplate
** ��������: ����ģ��, ɾ���ն���
** �䡡��  : phookcb       �ص����ƿ�
**           iHole         ɾ���Ŀն�λ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _HookAdjTemplate (PLW_HOOK_CB  phookcb, INT  iHole)
{
    INT  i;
    
    for (i = (iHole + 1); i < LW_SYS_HOOK_SIZE; i++) {
        if (phookcb->HOOKCB_pfuncnode[i]) {
            phookcb->HOOKCB_pfuncnode[i - 1] = phookcb->HOOKCB_pfuncnode[i];
            phookcb->HOOKCB_pfuncnode[i]     = LW_NULL;
        } else {
            break;
        }
    }
}
/*********************************************************************************************************
** ��������: _HookAddTemplate
** ��������: HOOK ����һ������ģ��
** �䡡��  : phookcb       �ص����ƿ�
**           pfuncnode     ������½ڵ�
** �䡡��  : �Ƿ����ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _HookAddTemplate (PLW_HOOK_CB  phookcb, PLW_FUNC_NODE  pfuncnode)
{
    INT      i;
    INTREG   iregInterLevel;
    
    LW_SPIN_LOCK_QUICK(&phookcb->HOOKCB_slHook, &iregInterLevel);
    for (i = 0; i < LW_SYS_HOOK_SIZE; i++) {
        if (phookcb->HOOKCB_pfuncnode[i] == LW_NULL) {
            phookcb->HOOKCB_pfuncnode[i] =  pfuncnode;
            phookcb->HOOKCB_uiCnt++;
            break;
        }
    }
    LW_SPIN_UNLOCK_QUICK(&phookcb->HOOKCB_slHook, iregInterLevel);
    
    return  ((i < LW_SYS_HOOK_SIZE) ? ERROR_NONE : PX_ERROR);
}
/*********************************************************************************************************
** ��������: _HookDelTemplate
** ��������: ���񴴽� HOOK ɾ��һ������
** �䡡��  : pfunc         ��Ҫɾ���ĵ���
**           bEmpty        ɾ�����Ƿ��Ѿ�û���κνڵ�
** �䡡��  : �Ѿ����������Ƴ��Ľڵ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_FUNC_NODE  _HookDelTemplate (PLW_HOOK_CB  phookcb, LW_HOOK_FUNC  hookfunc, BOOL  *bEmpty)
{
    INT             i;
    INTREG          iregInterLevel;
    PLW_FUNC_NODE   pfuncnode;
    
    *bEmpty = LW_FALSE;
    
    LW_SPIN_LOCK_QUICK(&phookcb->HOOKCB_slHook, &iregInterLevel);
    for (i = 0; i < LW_SYS_HOOK_SIZE; i++) {
        if (phookcb->HOOKCB_pfuncnode[i]) {
            if (phookcb->HOOKCB_pfuncnode[i]->FUNCNODE_hookfunc == hookfunc) {
                pfuncnode = phookcb->HOOKCB_pfuncnode[i];
                phookcb->HOOKCB_pfuncnode[i] = LW_NULL;
                phookcb->HOOKCB_uiCnt--;
                if (!phookcb->HOOKCB_uiCnt) {
                    *bEmpty = LW_TRUE;
                } else {
                    _HookAdjTemplate(phookcb, i);
                }
                break;
            }
        }
    }
    LW_SPIN_UNLOCK_QUICK(&phookcb->HOOKCB_slHook, iregInterLevel);
    
    return  ((i < LW_SYS_HOOK_SIZE) ? pfuncnode : LW_NULL);
}
/*********************************************************************************************************
** ��������: _CreateHookCall
** ��������: ���񴴽� HOOK ����
** �䡡��  : ulId                      �߳� Id
             ulOption                  ����ѡ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _CreateHookCall (LW_OBJECT_HANDLE  ulId, ULONG  ulOption)
{
    __HOOK_TEMPLATE_SPIN(HOOK_T_CREATE, (ulId, ulOption));
}
/*********************************************************************************************************
** ��������: _DeleteHookCall
** ��������: ����ɾ�� HOOK ����
** �䡡��  : ulId                      �߳� Id
**           pvReturnVal               �̷߳���ֵ
**           ptcb                      �߳� TCB
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _DeleteHookCall (LW_OBJECT_HANDLE  ulId, PVOID  pvReturnVal, PLW_CLASS_TCB  ptcb)
{
    __HOOK_TEMPLATE_SPIN(HOOK_T_DELETE, (ulId, pvReturnVal, ptcb));
}
/*********************************************************************************************************
** ��������: _SwapHookCall
** ��������: �����л� HOOK ����
** �䡡��  : hOldThread        ���߳�
**           hNewThread        ���߳�
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _SwapHookCall (LW_OBJECT_HANDLE   hOldThread, LW_OBJECT_HANDLE   hNewThread)
{
    __HOOK_TEMPLATE(HOOK_T_SWAP, (hOldThread, hNewThread));
}
/*********************************************************************************************************
** ��������: _TickHookCall
** ��������: TICK HOOK ����
** �䡡��  : i64Tick   ��ǰ tick
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _TickHookCall (INT64   i64Tick)
{
    __HOOK_TEMPLATE(HOOK_T_TICK, (i64Tick));
}
/*********************************************************************************************************
** ��������: _InitHookCall
** ��������: �̳߳�ʼ�� HOOK ����
** �䡡��  : ulId                      �߳� Id
**           ptcb                      �߳� TCB
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _InitHookCall (LW_OBJECT_HANDLE  ulId, PLW_CLASS_TCB  ptcb)
{
    __HOOK_TEMPLATE_SPIN(HOOK_T_INIT, (ulId, ptcb));
}
/*********************************************************************************************************
** ��������: _IdleHookCall
** ��������: �����߳� HOOK ����
** �䡡��  : ulCPUId                   CPU ID
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _IdleHookCall (ULONG  ulCPUId)
{
    __HOOK_TEMPLATE(HOOK_T_IDLE, (ulCPUId));
}
/*********************************************************************************************************
** ��������: _InitBeginHookCall
** ��������: ϵͳ��ʼ����ʼ HOOK ����
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _InitBeginHookCall (VOID)
{
    __HOOK_TEMPLATE(HOOK_T_INITBEGIN, ());
}
/*********************************************************************************************************
** ��������: _InitEndHookCall
** ��������: ϵͳ��ʼ������ HOOK ����
** �䡡��  : iError                    ����ϵͳ��ʼ���Ƿ���ִ���   0 �޴���   1 ����
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _InitEndHookCall (INT  iError)
{
    __HOOK_TEMPLATE(HOOK_T_INITEND, (iError));
}
/*********************************************************************************************************
** ��������: _RebootHookCall
** ��������: ϵͳ���� HOOK ����
** �䡡��  : iRebootType                ϵͳ������������
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _RebootHookCall (INT  iRebootType)
{
    __HOOK_TEMPLATE_SPIN(HOOK_T_REBOOT, (iRebootType));
}
/*********************************************************************************************************
** ��������: _WatchDogHookCall
** ��������: �߳̿��Ź� HOOK ����
** �䡡��  : ulId                      �߳� Id
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _WatchDogHookCall (LW_OBJECT_HANDLE  ulId)
{
    __HOOK_TEMPLATE(HOOK_T_WATCHDOG, (ulId));
}
/*********************************************************************************************************
** ��������: _ObjCreateHookCall
** ��������: �����ں˶��� HOOK ����
** �䡡��  : ulId                      �߳� Id
**           ulOption                  ����ѡ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _ObjCreateHookCall (LW_OBJECT_HANDLE  ulId, ULONG  ulOption)
{
    __HOOK_TEMPLATE_SPIN(HOOK_T_OBJCREATE, (ulId, ulOption));
}
/*********************************************************************************************************
** ��������: _ObjDeleteHookCall
** ��������: ɾ���ں˶��� HOOK ����
** �䡡��  : ulId                      �߳� Id
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _ObjDeleteHookCall (LW_OBJECT_HANDLE  ulId)
{
    __HOOK_TEMPLATE_SPIN(HOOK_T_OBJDELETE, (ulId));
}
/*********************************************************************************************************
** ��������: _FdCreateHookCall
** ��������: �ļ����������� HOOK ����
** �䡡��  : iFd                       �ļ�������
**           pid                       ����id
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _FdCreateHookCall (INT iFd, pid_t  pid)
{
    __HOOK_TEMPLATE_SPIN(HOOK_T_FDCREATE, (iFd, pid));
}
/*********************************************************************************************************
** ��������: _FdDeleteHookCall
** ��������: �ļ�������ɾ�� HOOK ����
** �䡡��  : iFd                       �ļ�������
**           pid                       ����id
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _FdDeleteHookCall (INT iFd, pid_t  pid)
{
    __HOOK_TEMPLATE_SPIN(HOOK_T_FDDELETE, (iFd, pid));
}
/*********************************************************************************************************
** ��������: _IdleEnterHookCall
** ��������: CPU �������ģʽ HOOK ����
** �䡡��  : ulIdEnterFrom             ���ĸ��߳̽��� idle
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _IdleEnterHookCall (LW_OBJECT_HANDLE  ulIdEnterFrom)
{
    __HOOK_TEMPLATE(HOOK_T_IDLEENTER, (ulIdEnterFrom));
}
/*********************************************************************************************************
** ��������: _IdleExitHookCall
** ��������: CPU �˳�����ģʽ HOOK ����
** �䡡��  : ulIdExitTo                �˳� idle �߳̽����ĸ��߳�
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _IdleExitHookCall (LW_OBJECT_HANDLE  ulIdExitTo)
{
    __HOOK_TEMPLATE(HOOK_T_IDLEEXIT, (ulIdExitTo));
}
/*********************************************************************************************************
** ��������: _IntEnterHookCall
** ��������: CPU �����ж�(�쳣)ģʽ HOOK ����
** �䡡��  : ulVector      �ж�����
**           ulNesting     ��ǰǶ�ײ���
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _IntEnterHookCall (ULONG  ulVector, ULONG  ulNesting)
{
    __HOOK_TEMPLATE(HOOK_T_INTENTER, (ulVector, ulNesting));
}
/*********************************************************************************************************
** ��������: _IntExitHookCall
** ��������: CPU �˳��ж�(�쳣)ģʽ HOOK ����
** �䡡��  : ulVector      �ж�����
**           ulNesting     ��ǰǶ�ײ���
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _IntExitHookCall (ULONG  ulVector, ULONG  ulNesting)
{
    __HOOK_TEMPLATE(HOOK_T_INTEXIT, (ulVector, ulNesting));
}
/*********************************************************************************************************
** ��������: _StkOverflowHookCall
** ��������: ϵͳ��ջ��� HOOK ����
** �䡡��  : pid                       ���� id
**           ulId                      �߳� id
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _StkOverflowHookCall (pid_t  pid, LW_OBJECT_HANDLE  ulId)
{
    __HOOK_TEMPLATE(HOOK_T_STKOF, (pid, ulId));
}
/*********************************************************************************************************
** ��������: _FatalErrorHookCall
** ��������: ϵͳ�������� HOOK ����
** �䡡��  : pid                       ���� id
**           ulId                      �߳� id
**           pinfo                     �ź���Ϣ
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _FatalErrorHookCall (pid_t  pid, LW_OBJECT_HANDLE  ulId, struct siginfo *psiginfo)
{
    __HOOK_TEMPLATE_SPIN(HOOK_T_FATALERR, (pid, ulId, psiginfo));
}
/*********************************************************************************************************
** ��������: _VpCreateHookCall
** ��������: ���̽��� HOOK ����
** �䡡��  : pid                       ���� id
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _VpCreateHookCall (pid_t pid)
{
    __HOOK_TEMPLATE_SPIN(HOOK_T_VPCREATE, (pid));
}
/*********************************************************************************************************
** ��������: _VpDeleteHookCall
** ��������: ����ɾ�� HOOK ����
** �䡡��  : pid                       ���� id
**           iExitCode                 ���̷���ֵ
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _VpDeleteHookCall (pid_t pid, INT iExitCode)
{
    __HOOK_TEMPLATE_SPIN(HOOK_T_VPDELETE, (pid, iExitCode));
}
/*********************************************************************************************************
** ��������: _HookListInit
** ��������: ��ʼ�� HOOK ����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID _HookListInit (VOID)
{
    LW_SPIN_INIT(&HOOK_T_CREATE->HOOKCB_slHook);
    HOOK_T_CREATE->HOOKCB_pfuncCall = _CreateHookCall;
    
    LW_SPIN_INIT(&HOOK_T_DELETE->HOOKCB_slHook);
    HOOK_T_DELETE->HOOKCB_pfuncCall = _DeleteHookCall;
    
    LW_SPIN_INIT(&HOOK_T_SWAP->HOOKCB_slHook);
    HOOK_T_SWAP->HOOKCB_pfuncCall = _SwapHookCall;
    
    LW_SPIN_INIT(&HOOK_T_TICK->HOOKCB_slHook);
    HOOK_T_TICK->HOOKCB_pfuncCall = _TickHookCall;
    
    LW_SPIN_INIT(&HOOK_T_INIT->HOOKCB_slHook);
    HOOK_T_INIT->HOOKCB_pfuncCall = _InitHookCall;
    
    LW_SPIN_INIT(&HOOK_T_IDLE->HOOKCB_slHook);
    HOOK_T_IDLE->HOOKCB_pfuncCall = _IdleHookCall;
    
    LW_SPIN_INIT(&HOOK_T_INITBEGIN->HOOKCB_slHook);
    HOOK_T_INITBEGIN->HOOKCB_pfuncCall = _InitBeginHookCall;
    
    LW_SPIN_INIT(&HOOK_T_INITEND->HOOKCB_slHook);
    HOOK_T_INITEND->HOOKCB_pfuncCall = _InitEndHookCall;
    
    LW_SPIN_INIT(&HOOK_T_REBOOT->HOOKCB_slHook);
    HOOK_T_REBOOT->HOOKCB_pfuncCall = _RebootHookCall;
    
    LW_SPIN_INIT(&HOOK_T_WATCHDOG->HOOKCB_slHook);
    HOOK_T_WATCHDOG->HOOKCB_pfuncCall = _WatchDogHookCall;
    
    LW_SPIN_INIT(&HOOK_T_OBJCREATE->HOOKCB_slHook);
    HOOK_T_OBJCREATE->HOOKCB_pfuncCall = _ObjCreateHookCall;
    
    LW_SPIN_INIT(&HOOK_T_OBJDELETE->HOOKCB_slHook);
    HOOK_T_OBJDELETE->HOOKCB_pfuncCall = _ObjDeleteHookCall;
    
    LW_SPIN_INIT(&HOOK_T_FDCREATE->HOOKCB_slHook);
    HOOK_T_FDCREATE->HOOKCB_pfuncCall = _FdCreateHookCall;
    
    LW_SPIN_INIT(&HOOK_T_FDDELETE->HOOKCB_slHook);
    HOOK_T_FDDELETE->HOOKCB_pfuncCall = _FdDeleteHookCall;
    
    LW_SPIN_INIT(&HOOK_T_IDLEENTER->HOOKCB_slHook);
    HOOK_T_IDLEENTER->HOOKCB_pfuncCall = _IdleEnterHookCall;
    
    LW_SPIN_INIT(&HOOK_T_IDLEEXIT->HOOKCB_slHook);
    HOOK_T_IDLEEXIT->HOOKCB_pfuncCall = _IdleExitHookCall;
    
    LW_SPIN_INIT(&HOOK_T_INTENTER->HOOKCB_slHook);
    HOOK_T_INTENTER->HOOKCB_pfuncCall = _IntEnterHookCall;
    
    LW_SPIN_INIT(&HOOK_T_INTEXIT->HOOKCB_slHook);
    HOOK_T_INTEXIT->HOOKCB_pfuncCall = _IntExitHookCall;
    
    LW_SPIN_INIT(&HOOK_T_STKOF->HOOKCB_slHook);
    HOOK_T_STKOF->HOOKCB_pfuncCall = _StkOverflowHookCall;
    
    LW_SPIN_INIT(&HOOK_T_FATALERR->HOOKCB_slHook);
    HOOK_T_FATALERR->HOOKCB_pfuncCall = _FatalErrorHookCall;
    
    LW_SPIN_INIT(&HOOK_T_VPCREATE->HOOKCB_slHook);
    HOOK_T_VPCREATE->HOOKCB_pfuncCall = _VpCreateHookCall;
    
    LW_SPIN_INIT(&HOOK_T_VPDELETE->HOOKCB_slHook);
    HOOK_T_VPDELETE->HOOKCB_pfuncCall = _VpDeleteHookCall;
    
    HOOK_F_ADD = _HookAddTemplate;
    HOOK_F_DEL = _HookDelTemplate;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
