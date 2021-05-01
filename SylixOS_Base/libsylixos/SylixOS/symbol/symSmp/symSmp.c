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
** ��   ��   ��: symSmp.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2018 �� 12 �� 11 ��
**
** ��        ��: ������� SMP/UP ���ݷ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_SYMBOL_EN > 0
/*********************************************************************************************************
  bsp ����. 
*********************************************************************************************************/
#define __LW_SYMBOL_ITEM__STR(s)            #s
#define __LW_SYMBOL_ITEM_STR(s)             __LW_SYMBOL_ITEM__STR(s)
#define __LW_SYMBOL_ITEM_FUNC(pcName)                                       \
        {   {LW_NULL, LW_NULL},                                             \
            __LW_SYMBOL_ITEM_STR(pcName), (caddr_t)pcName,                  \
    	    LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN \
        },
static LW_SYMBOL    _G_symSmp[] = {
/*********************************************************************************************************
  SMP
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
    {   {LW_NULL, LW_NULL}, "_UpSpinInit", (caddr_t)_SmpSpinInit,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_UpSpinLock", (caddr_t)_SmpSpinLock,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_UpSpinTryLock", (caddr_t)_SmpSpinTryLock,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_UpSpinUnlock", (caddr_t)_SmpSpinUnlock,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_UpSpinLockIrq", (caddr_t)_SmpSpinLockIrq,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_UpSpinTryLockIrq", (caddr_t)_SmpSpinTryLockIrq,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_UpSpinUnlockIrq", (caddr_t)_SmpSpinUnlockIrq,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_UpSpinLockIrqQuick", (caddr_t)_SmpSpinLockIrqQuick,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_UpSpinUnlockIrqQuick", (caddr_t)_SmpSpinUnlockIrqQuick,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_UpSpinLockTask", (caddr_t)_SmpSpinLockTask,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_UpSpinTryLockTask", (caddr_t)_SmpSpinTryLockTask,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_UpSpinUnlockTask", (caddr_t)_SmpSpinUnlockTask,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_UpSpinLockRaw", (caddr_t)_SmpSpinLockRaw,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_UpSpinTryLockRaw", (caddr_t)_SmpSpinTryLockRaw,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_UpSpinUnlockRaw", (caddr_t)_SmpSpinUnlockRaw,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
#else
/*********************************************************************************************************
  UP
*********************************************************************************************************/
    {   {LW_NULL, LW_NULL}, "_SmpSpinInit", (caddr_t)_UpSpinInit,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_SmpSpinLock", (caddr_t)_UpSpinLock,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_SmpSpinTryLock", (caddr_t)_UpSpinTryLock,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_SmpSpinUnlock", (caddr_t)_UpSpinUnlock,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_SmpSpinLockIrq", (caddr_t)_UpSpinLockIrq,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_SmpSpinTryLockIrq", (caddr_t)_UpSpinTryLockIrq,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_SmpSpinUnlockIrq", (caddr_t)_UpSpinUnlockIrq,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_SmpSpinLockIrqQuick", (caddr_t)_UpSpinLockIrqQuick,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_SmpSpinUnlockIrqQuick", (caddr_t)_UpSpinUnlockIrqQuick,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_SmpSpinLockTask", (caddr_t)_UpSpinLockTask,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_SmpSpinTryLockTask", (caddr_t)_UpSpinTryLockTask,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_SmpSpinUnlockTask", (caddr_t)_UpSpinUnlockTask,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_SmpSpinLockRaw", (caddr_t)_UpSpinLockRaw,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_SmpSpinTryLockRaw", (caddr_t)_UpSpinTryLockRaw,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_SmpSpinUnlockRaw", (caddr_t)_UpSpinUnlockRaw,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
#endif
};
/*********************************************************************************************************
** ��������: __symbolAddSmp
** ��������: ����ű������ SMP ���� 
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __symbolAddSmp (VOID)
{
    return  (API_SymbolAddStatic(_G_symSmp, (sizeof(_G_symSmp) / sizeof(LW_SYMBOL))));
}

#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
