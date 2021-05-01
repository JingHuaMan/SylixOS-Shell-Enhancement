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
** ��   ��   ��: gpioLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 11 �� 29 ��
**
** ��        ��: GPIO (ͨ������/���) �ܽŲ���ģ��.
**
** BUG:
2014.05.25  ���� get flags ����.
2017.12.29  ���� API_GpioGetIrq() ������ GPIO ����һ·�жϵľ�������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_GPIO_EN > 0
/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/
static LW_LIST_LINE_HEADER          _G_plineGpioChips;                  /*  ������������                */
static LW_GPIO_DESC                 _G_gdesc[LW_CFG_MAX_GPIOS];         /*  ÿ���ܽ�������              */
static LW_SPINLOCK_DEFINE          (_G_slGpio);
/*********************************************************************************************************
  �ڲ�������
*********************************************************************************************************/
#define GPIO_LOCK(pintreg)          LW_SPIN_LOCK_QUICK(&_G_slGpio, (pintreg))
#define GPIO_UNLOCK(intreg)         LW_SPIN_UNLOCK_QUICK(&_G_slGpio, (intreg))

#define GPIO_CHIP_HWGPIO(pgdesc)    ((pgdesc) - &(pgdesc)->GD_pgcChip->GC_gdDesc[0])
#define GPIO_TO_DESC(gpio)          (&_G_gdesc[(gpio)])
#define DESC_TO_GPIO(pgdesc)        ((pgdesc)->GD_pgcChip->GC_uiBase + GPIO_CHIP_HWGPIO(pgdesc))

#define GPIO_IS_VALID(gpio)         ((gpio) < LW_CFG_MAX_GPIOS)
/*********************************************************************************************************
** ��������: __gpioGetDirection
** ��������: ���ָ�� GPIO ����
** �䡡��  : pgdesc    GPIO �ܽ�������
** �䡡��  : 0: ���� 1: ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __gpioGetDirection (PLW_GPIO_DESC pgdesc)
{
    INT            iError;
    PLW_GPIO_CHIP  pgchip = pgdesc->GD_pgcChip;
    
    if (!pgchip->GC_pfuncGetDirection) {
        return  (PX_ERROR);
    }
    
    iError = pgchip->GC_pfuncGetDirection(pgchip, GPIO_CHIP_HWGPIO(pgdesc));
    if (iError > 0) {
        pgdesc->GD_ulFlags |= LW_GPIODF_IS_OUT;
        return  (1);
    
    } else if (iError == 0) {
        pgdesc->GD_ulFlags &= ~LW_GPIODF_IS_OUT;
        return  (0);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __gpioGetDesc
** ��������: ͨ�� GPIO �Ż��ָ�� GPIO ������
** �䡡��  : uiGpio        GPIO ��
**           bRequested    �Ƿ�������
** �䡡��  : GPIO �ܽ�������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_GPIO_DESC  __gpioGetDesc (UINT  uiGpio, BOOL  bRequested)
{
    PLW_GPIO_DESC   pgdesc;
    
    if (!GPIO_IS_VALID(uiGpio)) {
        return  (LW_NULL);
    }
    
    pgdesc = GPIO_TO_DESC(uiGpio);
    if (bRequested && !(pgdesc->GD_ulFlags & LW_GPIODF_REQUESTED)) {
        return  (LW_NULL);
    }
    
    return  (pgdesc);
}
/*********************************************************************************************************
** ��������: __gpioSetValueOpenDrain
** ��������: ����һ�� OPEN DRAIN ���Ե� GPIO
** �䡡��  : pgdesc    GPIO �ܽ�������
**           iValue    ���õ�ֵ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __gpioSetValueOpenDrain (PLW_GPIO_DESC pgdesc, INT iValue)
{
    INT             iError;
    PLW_GPIO_CHIP   pgchip = pgdesc->GD_pgcChip;
    
    if (iValue) {
        iError = pgchip->GC_pfuncDirectionInput(pgchip, GPIO_CHIP_HWGPIO(pgdesc));
        if (iError == ERROR_NONE) {
            pgdesc->GD_ulFlags &= ~LW_GPIODF_IS_OUT;
        }
    
    } else {
        iError = pgchip->GC_pfuncDirectionOutput(pgchip, GPIO_CHIP_HWGPIO(pgdesc), 0);
        if (iError == ERROR_NONE) {
            pgdesc->GD_ulFlags |= LW_GPIODF_IS_OUT;
        }
    }
}
/*********************************************************************************************************
** ��������: __gpioSetValueOpenSource
** ��������: ����һ�� OPEN SOURCE ���Ե� GPIO
** �䡡��  : pgdesc    GPIO �ܽ�������
**           iValue    ���õ�ֵ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __gpioSetValueOpenSource (PLW_GPIO_DESC pgdesc, INT iValue)
{
    INT             iError;
    PLW_GPIO_CHIP   pgchip = pgdesc->GD_pgcChip;
    
    if (iValue) {
        iError = pgchip->GC_pfuncDirectionOutput(pgchip, GPIO_CHIP_HWGPIO(pgdesc), 1);
        if (iError == ERROR_NONE) {
            pgdesc->GD_ulFlags |= LW_GPIODF_IS_OUT;
        }
    
    } else {
        iError = pgchip->GC_pfuncDirectionInput(pgchip, GPIO_CHIP_HWGPIO(pgdesc));
        if (iError == ERROR_NONE) {
            pgdesc->GD_ulFlags &= ~LW_GPIODF_IS_OUT;
        }
    }
}
/*********************************************************************************************************
** ��������: _GpioInit
** ��������: ��ʼ�� GPIO ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_GpioInit (VOID)
{
    LW_SPIN_INIT(&_G_slGpio);
}
/*********************************************************************************************************
** ��������: API_GpioChipAdd
** ��������: ����һ�� GPIO оƬ����
** �䡡��  : pgchip    GPIO ����
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GpioChipAdd (PLW_GPIO_CHIP pgchip)
{
    INTREG         iregInterLevel;
    UINT           i;
    PLW_LIST_LINE  plineTemp;
    PLW_LIST_LINE  plinePrev;
    PLW_GPIO_CHIP  pgchipTemp;
    
    if (!pgchip) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pgchip->GC_ulVerMagic != LW_GPIO_VER_MAGIC) {                   /*  ������ƥ��                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, 
                     "GPIO driver version not matching to current system.\r\n");
        _ErrorHandle(EFTYPE);
        return  (PX_ERROR);
    }
    
    if ((!GPIO_IS_VALID(pgchip->GC_uiBase)) || 
        (!GPIO_IS_VALID(pgchip->GC_uiBase + pgchip->GC_uiNGpios - 1))) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    GPIO_LOCK(&iregInterLevel);                                         /*  ���� GPIO                   */
    for (plineTemp  = _G_plineGpioChips;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pgchipTemp = _LIST_ENTRY(plineTemp, LW_GPIO_CHIP, GC_lineManage);
        if (pgchipTemp->GC_uiBase >= (pgchip->GC_uiBase + pgchip->GC_uiNGpios)) {
            break;
        }
    }
    if (plineTemp) {
        plinePrev = _list_line_get_prev(plineTemp);
        if (plinePrev) {
            pgchipTemp = _LIST_ENTRY(plinePrev, LW_GPIO_CHIP, GC_lineManage);
            if ((pgchipTemp->GC_uiBase + pgchipTemp->GC_uiNGpios) > pgchip->GC_uiBase) {
                GPIO_UNLOCK(iregInterLevel);                            /*  ���� GPIO                   */
                _DebugHandle(__ERRORMESSAGE_LEVEL, 
                             "GPIO integer space overlap, cannot add chip.\r\n");
                _ErrorHandle(EBUSY);
                return  (PX_ERROR);
            }
        }
        _List_Line_Add_Left(&pgchip->GC_lineManage, plineTemp);
    
    } else if (_G_plineGpioChips) {                                     /*  �¼���� base ���          */
        _List_Line_Add_Right(&pgchip->GC_lineManage, &pgchipTemp->GC_lineManage);
    
    } else {
        _List_Line_Add_Ahead(&pgchip->GC_lineManage, &_G_plineGpioChips);
    }
    
    pgchip->GC_gdDesc = &_G_gdesc[pgchip->GC_uiBase];
    for (i = 0; i < pgchip->GC_uiNGpios; i++) {
        PLW_GPIO_DESC pgdesc = &pgchip->GC_gdDesc[i];
        pgdesc->GD_pgcChip = pgchip;
        
        if (pgchip->GC_pfuncDirectionInput == LW_NULL) {                /*  û�����빦��                */
            pgdesc->GD_ulFlags = LW_GPIODF_IS_OUT;
        
        } else {
            pgdesc->GD_ulFlags = 0ul;                                   /*  Ĭ��Ϊ����̬(оƬ���������)*/
        }
    }
    GPIO_UNLOCK(iregInterLevel);                                        /*  ���� GPIO                   */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_GpioChipDelete
** ��������: ɾ��һ�� GPIO оƬ����
** �䡡��  : pgchip    GPIO ����
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GpioChipDelete (PLW_GPIO_CHIP pgchip)
{
    INTREG  iregInterLevel;
    UINT    i;

    if (!pgchip) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    GPIO_LOCK(&iregInterLevel);                                         /*  ���� GPIO                   */
    for (i = 0; i < pgchip->GC_uiNGpios; i++) {
        if (pgchip->GC_gdDesc[i].GD_ulFlags & LW_GPIODF_REQUESTED) {
            GPIO_UNLOCK(iregInterLevel);                                /*  ���� GPIO                   */
            _ErrorHandle(EBUSY);
            return  (PX_ERROR);
        }
    }
    
    for (i = 0; i < pgchip->GC_uiNGpios; i++) {
        pgchip->GC_gdDesc[i].GD_pgcChip = LW_NULL;
    }
    
    _List_Line_Del(&pgchip->GC_lineManage, &_G_plineGpioChips);
    GPIO_UNLOCK(iregInterLevel);                                        /*  ���� GPIO                   */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_GpioChipFind
** ��������: ��ѯһ�� GPIO оƬ����
** �䡡��  : pvData        ƥ�亯������
**           pfuncMatch    ��ѯƥ�亯�� (�˺������� LW_TRUE ��ʾ�ҵ�)
** �䡡��  : ��ѯ���������ṹ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PLW_GPIO_CHIP  API_GpioChipFind (PVOID pvData, BOOL (*pfuncMatch)(PLW_GPIO_CHIP pgchip, PVOID  pvData))
{
    INTREG         iregInterLevel;
    PLW_GPIO_CHIP  pgchip;
    PLW_LIST_LINE  plineTemp;
    
    if (!pfuncMatch) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    GPIO_LOCK(&iregInterLevel);                                         /*  ���� GPIO                   */
    for (plineTemp  = _G_plineGpioChips;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pgchip = _LIST_ENTRY(plineTemp, LW_GPIO_CHIP, GC_lineManage);
        if (pfuncMatch(pgchip, pvData)) {
            GPIO_UNLOCK(iregInterLevel);                                /*  ���� GPIO                   */
            return  (pgchip);
        }
    }
    GPIO_UNLOCK(iregInterLevel);                                        /*  ���� GPIO                   */
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_GpioIsValid
** ��������: GPIO ���Ƿ���Ч
** �䡡��  : uiGpio        GPIO ��
** �䡡��  : 1: ��Ч 0:��Ч
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GpioIsValid (UINT uiGpio)
{
    return  (GPIO_IS_VALID(uiGpio));
}
/*********************************************************************************************************
** ��������: API_GpioHasDrv
** ��������: GPIO �Ƿ��ж�Ӧ����������
** �䡡��  : uiGpio        GPIO ��
** �䡡��  : 1: �� 0:��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GpioHasDrv (UINT uiGpio)
{
    PLW_GPIO_DESC   pgdesc;
    
    pgdesc = GPIO_TO_DESC(uiGpio);
    
    return  (pgdesc->GD_pgcChip ? 1 : 0);
}
/*********************************************************************************************************
** ��������: API_GpioRequest
** ��������: ����ʹ��һ�� GPIO
** �䡡��  : uiGpio        ���� GPIO ��
**           pcLabel       ��ǩ
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GpioRequest (UINT uiGpio, CPCHAR pcLabel)
{
    INTREG          iregInterLevel;
    INT             iError;
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;
    
    if (!GPIO_IS_VALID(uiGpio)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pgdesc = GPIO_TO_DESC(uiGpio);
    
    GPIO_LOCK(&iregInterLevel);                                         /*  ���� GPIO                   */
    pgchip = pgdesc->GD_pgcChip;
    if (!pgchip) {
        GPIO_UNLOCK(iregInterLevel);                                    /*  ���� GPIO                   */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pgdesc->GD_ulFlags & LW_GPIODF_REQUESTED) {                     /*  �Ѿ���ʹ����                */
        GPIO_UNLOCK(iregInterLevel);                                    /*  ���� GPIO                   */
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }
    
    pgdesc->GD_ulFlags |= LW_GPIODF_REQUESTED;
    pgdesc->GD_pcLabel  = pcLabel ? pcLabel : "?";
    
    if (pgchip->GC_pfuncRequest) {
        GPIO_UNLOCK(iregInterLevel);                                    /*  ���� GPIO                   */
        iError = pgchip->GC_pfuncRequest(pgchip, GPIO_CHIP_HWGPIO(pgdesc));
        GPIO_LOCK(&iregInterLevel);                                     /*  ���� GPIO                   */
        
        if (iError < ERROR_NONE) {
            pgdesc->GD_ulFlags &= ~LW_GPIODF_REQUESTED;
            pgdesc->GD_pcLabel  = LW_NULL;
            GPIO_UNLOCK(iregInterLevel);                                /*  ���� GPIO                   */
            return  (iError);
        }
    }
    
    if (pgchip->GC_pfuncGetDirection) {
        GPIO_UNLOCK(iregInterLevel);                                    /*  ���� GPIO                   */
        __gpioGetDirection(pgdesc);
    
    } else {
        GPIO_UNLOCK(iregInterLevel);                                    /*  ���� GPIO                   */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_GpioRequestOne
** ��������: ����ʹ��һ�� GPIO
** �䡡��  : uiGpio        ���� GPIO ��
**           ulFlags       ��Ҫ���õ�����
**           pcLabel       ��ǩ
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GpioRequestOne (UINT uiGpio, ULONG ulFlags, CPCHAR pcLabel)
{
    INT             iError;
    PLW_GPIO_DESC   pgdesc;
    
    iError = API_GpioRequest(uiGpio, pcLabel);
    if (iError < ERROR_NONE) {
        return  (iError);
    }
    
    pgdesc = GPIO_TO_DESC(uiGpio);
    
    if (ulFlags & LW_GPIOF_OPEN_DRAIN) {
        pgdesc->GD_ulFlags |= LW_GPIODF_OPEN_DRAIN;
    }
    
    if (ulFlags & LW_GPIOF_OPEN_SOURCE) {
        pgdesc->GD_ulFlags |= LW_GPIODF_OPEN_SOURCE;
    }
    
    if (ulFlags & LW_GPIOF_DIR_IN) {
        iError = API_GpioDirectionInput(uiGpio);
    
    } else {
        iError = API_GpioDirectionOutput(uiGpio, (ulFlags & LW_GPIOF_INIT_HIGH) ? 1 : 0);
    }
    
    if (iError < ERROR_NONE) {
        API_GpioFree(uiGpio);
        return  (iError);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_GpioFree
** ��������: �ͷ�ʹ��һ�� GPIO
** �䡡��  : uiGpio        GPIO ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_GpioFree (UINT uiGpio)
{
    INTREG          iregInterLevel;
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;
    
    pgdesc = __gpioGetDesc(uiGpio, LW_TRUE);
    if (!pgdesc) {
        return;
    }
    
    GPIO_LOCK(&iregInterLevel);                                         /*  ���� GPIO                   */
    pgchip = pgdesc->GD_pgcChip;
    if (pgchip && (pgdesc->GD_ulFlags & LW_GPIODF_REQUESTED)) {
        if (pgchip->GC_pfuncFree) {
            GPIO_UNLOCK(iregInterLevel);                                /*  ���� GPIO                   */
            pgchip->GC_pfuncFree(pgchip, GPIO_CHIP_HWGPIO(pgdesc));
            GPIO_LOCK(&iregInterLevel);                                 /*  ���� GPIO                   */
        }
        
        pgdesc->GD_ulFlags &= ~(LW_GPIODF_REQUESTED | LW_GPIODF_TRIGGER_MASK);
        pgdesc->GD_pcLabel  = LW_NULL;
    }
    GPIO_UNLOCK(iregInterLevel);                                        /*  ���� GPIO                   */
}
/*********************************************************************************************************
** ��������: API_GpioRequestArray
** ��������: ����ʹ��һ�� GPIO
** �䡡��  : pgArray       GPIO ����
**           stNum         ����Ԫ�ظ���
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GpioRequestArray (PLW_GPIO pgArray, size_t stNum)
{
    INT     i;
    INT     iError;
    
    if (!pgArray || !stNum) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    for (i = 0; i < stNum; i++, pgArray++) {
        iError = API_GpioRequestOne(pgArray->G_ulGpio, pgArray->G_ulFlags, pgArray->G_pcLabel);
        if (iError < ERROR_NONE) {
            goto    __error_handle;
        }
    }
    
    return  (ERROR_NONE);
    
__error_handle:
    while (i) {
        pgArray--;
        i--;
        API_GpioFree(pgArray->G_ulGpio);
    }
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: API_GpioFreeArray
** ��������: �ͷ�һ�� GPIO
** �䡡��  : pgArray       GPIO ����
**           stNum         ����Ԫ�ظ���
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_GpioFreeArray (PLW_GPIO pgArray, size_t stNum)
{
    if (!pgArray) {
        return;
    }

    while (stNum) {
        API_GpioFree(pgArray->G_ulGpio);
        pgArray++;
        stNum--;
    }
}
/*********************************************************************************************************
** ��������: API_GpioGetFlags
** ��������: ��ȡһ�� GPIO flags
** �䡡��  : uiGpio        ���� GPIO ��
**           pulFlags      ��ȡ�� flags
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GpioGetFlags (UINT uiGpio, ULONG *pulFlags)
{
    PLW_GPIO_DESC   pgdesc;

    if (!pulFlags) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pgdesc = __gpioGetDesc(uiGpio, LW_TRUE);
    if (!pgdesc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    *pulFlags = pgdesc->GD_ulFlags;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_GpioOpenDrain
** ��������: ����һ�� GPIO OPEN DRAIN ����
** �䡡��  : uiGpio        ���� GPIO ��
**           bOpenDrain    ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_GpioOpenDrain (UINT uiGpio, BOOL bOpenDrain)
{
    PLW_GPIO_DESC   pgdesc;

    pgdesc = __gpioGetDesc(uiGpio, LW_TRUE);
    if (!pgdesc) {
        return;
    }
    
    if (bOpenDrain) {
        pgdesc->GD_ulFlags |= LW_GPIODF_OPEN_DRAIN;
    
    } else {
        pgdesc->GD_ulFlags &= ~LW_GPIODF_OPEN_DRAIN;
    }
}
/*********************************************************************************************************
** ��������: API_GpioOpenSource
** ��������: ����һ�� GPIO OPEN SOURCE ����
** �䡡��  : uiGpio        ���� GPIO ��
**           bOpenSource   ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_GpioOpenSource (UINT uiGpio, BOOL bOpenSource)
{
    PLW_GPIO_DESC   pgdesc;

    pgdesc = __gpioGetDesc(uiGpio, LW_TRUE);
    if (!pgdesc) {
        return;
    }
    
    if (bOpenSource) {
        pgdesc->GD_ulFlags |= LW_GPIODF_OPEN_SOURCE;
    
    } else {
        pgdesc->GD_ulFlags &= ~LW_GPIODF_OPEN_SOURCE;
    }
}
/*********************************************************************************************************
** ��������: API_GpioSetDebounce
** ��������: ����ָ�� GPIO ȥ����ʱ�����
** �䡡��  : uiGpio        GPIO ��
**           uiDebounce    ȥ����ʱ�����
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GpioSetDebounce (UINT uiGpio, UINT uiDebounce)
{
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;

    pgdesc = __gpioGetDesc(uiGpio, LW_TRUE);
    if (!pgdesc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pgchip = pgdesc->GD_pgcChip;
    if (!pgchip || !pgchip->GC_pfuncSetDebounce) {
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    
    return  (pgchip->GC_pfuncSetDebounce(pgchip, GPIO_CHIP_HWGPIO(pgdesc), uiDebounce));
}
/*********************************************************************************************************
** ��������: API_GpioSetPull
** ��������: ����ָ�� GPIO ����������
** �䡡��  : uiGpio        GPIO ��
**           uiType        ���������� 0: ��· 1: ���� pull up 2: ���� pull down
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GpioSetPull (UINT uiGpio, UINT uiType)
{
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;

    pgdesc = __gpioGetDesc(uiGpio, LW_TRUE);
    if (!pgdesc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pgchip = pgdesc->GD_pgcChip;
    if (!pgchip || !pgchip->GC_pfuncSetPull) {
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    
    return  (pgchip->GC_pfuncSetPull(pgchip, GPIO_CHIP_HWGPIO(pgdesc), uiType));
}
/*********************************************************************************************************
** ��������: API_GpioDirectionInput
** ��������: ����ָ�� GPIO Ϊ����ģʽ
** �䡡��  : uiGpio        GPIO ��
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GpioDirectionInput (UINT uiGpio)
{
    INT             iError;
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;

    pgdesc = __gpioGetDesc(uiGpio, LW_TRUE);
    if (!pgdesc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pgchip = pgdesc->GD_pgcChip;
    if (!pgchip || !pgchip->GC_pfuncGet || !pgchip->GC_pfuncDirectionInput) {
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    
    iError = pgchip->GC_pfuncDirectionInput(pgchip, GPIO_CHIP_HWGPIO(pgdesc));
    if (iError == ERROR_NONE) {
        pgdesc->GD_ulFlags &= ~LW_GPIODF_IS_OUT;
    }
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: API_GpioDirectionOutput
** ��������: ����ָ�� GPIO Ϊ���ģʽ
** �䡡��  : uiGpio        GPIO ��
**           iValue        1: �ߵ�ƽ 0: �͵�ƽ
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GpioDirectionOutput (UINT uiGpio, INT iValue)
{
    INT             iError;
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;

    pgdesc = __gpioGetDesc(uiGpio, LW_TRUE);
    if (!pgdesc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pgchip = pgdesc->GD_pgcChip;
    if (!pgchip || !pgchip->GC_pfuncSet || !pgchip->GC_pfuncDirectionOutput) {
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    
    iError = pgchip->GC_pfuncDirectionOutput(pgchip, GPIO_CHIP_HWGPIO(pgdesc), iValue);
    if (iError == ERROR_NONE) {
        pgdesc->GD_ulFlags |= LW_GPIODF_IS_OUT;
    }
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: API_GpioGetValue
** ��������: ��ȡָ�� GPIO ��ֵ
** �䡡��  : uiGpio        GPIO ��
** �䡡��  : GPIO ��ǰ��ƽ״̬ 1: �ߵ�ƽ 0: �͵�ƽ
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �˺��������κβ�����Ч���ж�, �����û����뱣֤ uiGpio �Ѿ�����ɹ�, ��������������������ȷ.

                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_GpioGetValue (UINT uiGpio)
{
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;
    
    pgdesc = GPIO_TO_DESC(uiGpio);                                      /*  �����κμ��, �ӿ��ٶ�      */
    pgchip = pgdesc->GD_pgcChip;
    
    return  (pgchip->GC_pfuncGet(pgchip, GPIO_CHIP_HWGPIO(pgdesc)));
}
/*********************************************************************************************************
** ��������: API_GpioSetValue
** ��������: ����ָ�� GPIO ��ֵ
** �䡡��  : uiGpio        GPIO ��
**           iValue        1: �ߵ�ƽ 0: �͵�ƽ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �˺��������κβ�����Ч���ж�, �����û����뱣֤ uiGpio �Ѿ�����ɹ�, ��������������������ȷ.

                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_GpioSetValue (UINT uiGpio, INT iValue)
{
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;
    
    pgdesc = GPIO_TO_DESC(uiGpio);                                      /*  �����κμ��, �ӿ��ٶ�      */
    pgchip = pgdesc->GD_pgcChip;
    
    if (pgdesc->GD_ulFlags & LW_GPIODF_OPEN_DRAIN) {
        __gpioSetValueOpenDrain(pgdesc, iValue);
    
    } else if (pgdesc->GD_ulFlags & LW_GPIODF_OPEN_SOURCE) {
        __gpioSetValueOpenSource(pgdesc, iValue);
    
    } else {
        pgchip->GC_pfuncSet(pgchip, GPIO_CHIP_HWGPIO(pgdesc), iValue);
    }
}
/*********************************************************************************************************
** ��������: API_GpioGetIrq
** ��������: ����ָ�� GPIO �ŷ��ض�Ӧ�� IRQ ��
** �䡡��  : uiGpio        GPIO ��
**           bIsLevel      �Ƿ�Ϊ��ƽ����
**           uiType        ���Ϊ��ƽ����, 1 ��ʾ�ߵ�ƽ����, 0 ��ʾ�͵�ƽ����
**                         ���Ϊ���ش���, 1 ��ʾ�����ش���, 0 ��ʾ�½��ش���, 2 ��ʾ˫���ش���
** �䡡��  : IRQ ��, ���󷵻� LW_VECTOR_INVALID
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_GpioGetIrq (UINT uiGpio, BOOL bIsLevel, UINT uiType)
{
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;

    pgdesc = __gpioGetDesc(uiGpio, LW_TRUE);
    if (!pgdesc) {
        _ErrorHandle(EINVAL);
        return  (LW_VECTOR_INVALID);
    }
    
    pgchip = pgdesc->GD_pgcChip;
    
    if (pgchip->GC_pfuncGetIrq) {
        return  (pgchip->GC_pfuncGetIrq(pgchip, GPIO_CHIP_HWGPIO(pgdesc), bIsLevel, uiType));
    
    } else {
        _ErrorHandle(ENXIO);
        return  (LW_VECTOR_INVALID);
    }
}
/*********************************************************************************************************
** ��������: API_GpioSetupIrq
** ��������: ����ָ�� GPIO ��������Ӧ���ⲿ�ж�, �����ض�Ӧ�� IRQ ��
** �䡡��  : uiGpio        GPIO ��
**           bIsLevel      �Ƿ�Ϊ��ƽ����
**           uiType        ���Ϊ��ƽ����, 1 ��ʾ�ߵ�ƽ����, 0 ��ʾ�͵�ƽ����
**                         ���Ϊ���ش���, 1 ��ʾ�����ش���, 0 ��ʾ�½��ش���, 2 ��ʾ˫���ش���
** �䡡��  : IRQ ��, ���󷵻� LW_VECTOR_INVALID
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_GpioSetupIrq (UINT uiGpio, BOOL bIsLevel, UINT uiType)
{
    ULONG           ulVector;
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;

    pgdesc = __gpioGetDesc(uiGpio, LW_TRUE);
    if (!pgdesc) {
        _ErrorHandle(EINVAL);
        return  (LW_VECTOR_INVALID);
    }
    
    pgchip = pgdesc->GD_pgcChip;
    
    if (pgchip->GC_pfuncSetupIrq) {
        ulVector = pgchip->GC_pfuncSetupIrq(pgchip, GPIO_CHIP_HWGPIO(pgdesc), bIsLevel, uiType);
        if (ulVector != LW_VECTOR_INVALID) {                            /*  �ⲿ�ж����óɹ�            */
            if (bIsLevel) {
                pgdesc->GD_ulFlags |= LW_GPIODF_TRIG_LEVEL;
            }
            if (uiType == 0) {
                pgdesc->GD_ulFlags |= LW_GPIODF_TRIG_FALL;
            
            } else if (uiType == 1) {
                pgdesc->GD_ulFlags |= LW_GPIODF_TRIG_RISE;
                
            } else if (uiType == 2) {
                pgdesc->GD_ulFlags |= (LW_GPIODF_TRIG_FALL | LW_GPIODF_TRIG_RISE);
            }
        }
        return  (ulVector);
    
    } else {
        _ErrorHandle(ENXIO);
        return  (LW_VECTOR_INVALID);
    }
}
/*********************************************************************************************************
** ��������: API_GpioClearIrq
** ��������: GPIO Ϊ�ⲿ�ж�����ģʽʱ, ���ж�������������ж��������
** �䡡��  : uiGpio        GPIO ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �˺��������κβ�����Ч���ж�, �����û����뱣֤ uiGpio �Ѿ�����ɹ�, ��������������������ȷ.

                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_GpioClearIrq (UINT uiGpio)
{
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;
    
    pgdesc = GPIO_TO_DESC(uiGpio);                                      /*  �����κμ��, �ӿ��ٶ�      */
    pgchip = pgdesc->GD_pgcChip;
    
    if (pgchip->GC_pfuncClearIrq) {
        pgchip->GC_pfuncClearIrq(pgchip, GPIO_CHIP_HWGPIO(pgdesc));
    }
}
/*********************************************************************************************************
** ��������: API_GpioSvrIrq
** ��������: GPIO Ϊ�ⲿ�ж�����ģʽʱ, �жϵ�ǰ�Ƿ�Ϊָ���� GPIO �ж�
** �䡡��  : uiGpio        GPIO ��
** �䡡��  : LW_IRQ_HANDLED ��ʾ��ǰ�ж���ָ�� GPIO �������ж�
**           LW_IRQ_NONE    ��ʾ��ǰ�жϲ���ָ�� GPIO �������ж�
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �˺��������κβ�����Ч���ж�, �����û����뱣֤ uiGpio �Ѿ�����ɹ�, ��������������������ȷ.

                                           API ����
*********************************************************************************************************/
LW_API  
irqreturn_t  API_GpioSvrIrq (UINT uiGpio)
{
    irqreturn_t     irqret = LW_IRQ_NONE;
    PLW_GPIO_DESC   pgdesc;
    PLW_GPIO_CHIP   pgchip;
    
    pgdesc = GPIO_TO_DESC(uiGpio);                                      /*  �����κμ��, �ӿ��ٶ�      */
    pgchip = pgdesc->GD_pgcChip;
    
    if (pgchip->GC_pfuncSvrIrq) {
        irqret = pgchip->GC_pfuncSvrIrq(pgchip, GPIO_CHIP_HWGPIO(pgdesc));
    }
    
    return  (irqret);
}

#endif                                                                  /*  LW_CFG_GPIO_EN > 0          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
