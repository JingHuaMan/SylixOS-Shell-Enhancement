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
** ��   ��   ��: gpioLib.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 11 �� 29 ��
**
** ��        ��: GPIO (ͨ������/���) �ܽŲ���ģ��.
*********************************************************************************************************/

#ifndef __GPIOLIB_H
#define __GPIOLIB_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_GPIO_EN > 0

/*********************************************************************************************************
  GPIO ������
  
  GC_pfuncRequest  
  ��ʾ����һ�� GPIO �������û���������, �����Դ����ȵ�, �����Ϊ LW_NULL
  
  GC_pfuncFree
  �ͷ�һ�����ڱ�ʹ�õ� GPIO, �����ǰ���ж�ģʽ��, �����ж����빦��.
  
  GC_pfuncGetDirection
  ��õ�ǰ GPIO ����, 1 ��ʾ���, 0 ��ʾ����
  
  GC_pfuncDirectionInput
  ���� GPIO Ϊ����ģʽ (�����ǰ���ж�ģʽ��, �����ж����빦��)
  
  GC_pfuncGet
  ��� GPIO ����ֵ
  
  GC_pfuncDirectionOutput
  ���� GPIO Ϊ���ģʽ (�����ǰ���ж�ģʽ��, �����ж����빦��)
  
  GC_pfuncSetDebounce
  ���� GPIO ȥ��������
  
  GC_pfuncSetPull
  ���� GPIO ����������������, 0: ��· 1: ���� pull up 2: ���� pull down
  
  GC_pfuncSet
  ���� GPIO ���ֵ
  
  GC_pfuncGetIrq
  ��ȡ GPIO IRQ ������, ������ GPIO �ж�, ������ȡ IRQ ��
  bIsLevel 1: ��ƽ���� 0:���ش���, uiType 1:�����ش��� 0:�½��ش��� 2:˫���ش���
  
  GC_pfuncSetupIrq
  ���� GPIO Ϊ�ⲿ�ж������, ͬʱ���ض�Ӧ�� IRQ ������
  bIsLevel 1: ��ƽ���� 0:���ش���, uiType 1:�����ش��� 0:�½��ش��� 2:˫���ش���
  
  GC_pfuncClearIrq
  GPIO Ϊ�ⲿ�ж�����ģʽʱ, �����жϺ�, ���ж�������������ж��������.
  
  GC_pfuncSvrIrq
  GPIO �����ⲿ�ж�ʱ����ô˺���, ����Ǳ� GPIO �������ж�, �򷵻� LW_IRQ_HANDLED
  �������, �򷵻� LW_IRQ_NONE.
  
  ���Ϻ��� uiOffset ����Ϊ��� GC_uiBase ��ƫ����, GPIO ����������Ҫͨ������ֵȷ����Ӧ��Ӳ���Ĵ���.
  ע��: ��ͬ������ GC_pfuncGetIrq, GC_pfuncSetupIrq ����ֵ������ͬ.
*********************************************************************************************************/

struct lw_gpio_desc;
typedef struct lw_gpio_chip {
    CPCHAR                  GC_pcLabel;
    LW_LIST_LINE            GC_lineManage;
    ULONG                   GC_ulVerMagic;
#define LW_GPIO_VER_MAGIC   0xfffffff1
    
    INT                   (*GC_pfuncRequest)(struct lw_gpio_chip *pgchip, UINT uiOffset);
    VOID                  (*GC_pfuncFree)(struct lw_gpio_chip *pgchip, UINT uiOffset);
    INT                   (*GC_pfuncGetDirection)(struct lw_gpio_chip *pgchip, UINT uiOffset);
    INT                   (*GC_pfuncDirectionInput)(struct lw_gpio_chip *pgchip, UINT uiOffset);
    INT                   (*GC_pfuncGet)(struct lw_gpio_chip *pgchip, UINT uiOffset);
    INT                   (*GC_pfuncDirectionOutput)(struct lw_gpio_chip *pgchip, UINT uiOffset, 
                                                     INT iValue);
    INT                   (*GC_pfuncSetDebounce)(struct lw_gpio_chip *pgchip, UINT uiOffset, 
                                                 UINT uiDebounce);
    INT                   (*GC_pfuncSetPull)(struct lw_gpio_chip *pgchip, UINT uiOffset, UINT uiType);
    VOID                  (*GC_pfuncSet)(struct lw_gpio_chip *pgchip, UINT uiOffset, INT iValue);
    ULONG                 (*GC_pfuncGetIrq)(struct lw_gpio_chip *pgchip, UINT uiOffset,
                                            BOOL bIsLevel, UINT uiType);
    ULONG                 (*GC_pfuncSetupIrq)(struct lw_gpio_chip *pgchip, UINT uiOffset,
                                              BOOL bIsLevel, UINT uiType);
    VOID                  (*GC_pfuncClearIrq)(struct lw_gpio_chip *pgchip, UINT uiOffset);
    irqreturn_t           (*GC_pfuncSvrIrq)(struct lw_gpio_chip *pgchip, UINT uiOffset);
    
    UINT                    GC_uiBase;
    UINT                    GC_uiNGpios;
    struct lw_gpio_desc    *GC_gdDesc;
    
    ULONG                   GC_ulPad[16];                               /*  ����δ����չ                */
} LW_GPIO_CHIP;
typedef LW_GPIO_CHIP       *PLW_GPIO_CHIP;

/*********************************************************************************************************
  GPIO �ܽ������� (�ڲ�ʹ��)
*********************************************************************************************************/

typedef struct lw_gpio_desc {
    PLW_GPIO_CHIP           GD_pgcChip;
    ULONG                   GD_ulFlags;
#define LW_GPIODF_REQUESTED             0x0001
#define LW_GPIODF_IS_OUT                0x0002
#define LW_GPIODF_TRIG_FALL             0x0004
#define LW_GPIODF_TRIG_RISE             0x0008
#define LW_GPIODF_TRIG_LEVEL            0x0010
#define LW_GPIODF_OPEN_DRAIN            0x0020
#define LW_GPIODF_OPEN_SOURCE           0x0040

#define LW_GPIODF_ID_SHIFT              16
#define LW_GPIODF_MASK                  ((1 << ID_SHIFT) - 1)
#define LW_GPIODF_TRIGGER_MASK          (LW_GPIODF_TRIG_FALL | LW_GPIODF_TRIG_RISE)

    CPCHAR                  GD_pcLabel;
} LW_GPIO_DESC;
typedef LW_GPIO_DESC       *PLW_GPIO_DESC;

/*********************************************************************************************************
  GPIO �û��ܽ�����
*********************************************************************************************************/

typedef struct lw_gpio {
    UINT                    G_ulGpio;
    ULONG                   G_ulFlags;
#define LW_GPIOF_DIR_OUT                (0 << 0)
#define LW_GPIOF_DIR_IN                 (1 << 0)

#define LW_GPIOF_INIT_LOW               (0 << 1)
#define LW_GPIOF_INIT_HIGH              (1 << 1)

#define LW_GPIOF_IN                     (LW_GPIOF_DIR_IN)
#define LW_GPIOF_OUT_INIT_LOW           (LW_GPIOF_DIR_OUT | LW_GPIOF_INIT_LOW)
#define LW_GPIOF_OUT_INIT_HIGH          (LW_GPIOF_DIR_OUT | LW_GPIOF_INIT_HIGH)

#define LW_GPIOF_OPEN_DRAIN             (1 << 2)
#define LW_GPIOF_OPEN_SOURCE            (1 << 3)
    
    CPCHAR                  G_pcLabel;
} LW_GPIO;
typedef LW_GPIO            *PLW_GPIO;

/*********************************************************************************************************
  GPIO API (���� API �������������ڲ�ʹ��, Ӧ�ó��򲻿�ʹ��)
*********************************************************************************************************/

LW_API VOID             API_GpioInit(VOID);
LW_API INT              API_GpioChipAdd(PLW_GPIO_CHIP pgchip);
LW_API INT              API_GpioChipDelete(PLW_GPIO_CHIP pgchip);
LW_API PLW_GPIO_CHIP    API_GpioChipFind(PVOID pvData, 
                                         BOOL (*pfuncMatch)(PLW_GPIO_CHIP pgchip, 
                                                            PVOID  pvData));
LW_API INT              API_GpioIsValid(UINT uiGpio);
LW_API INT              API_GpioHasDrv(UINT uiGpio);
LW_API INT              API_GpioRequest(UINT uiGpio, CPCHAR pcLabel);
LW_API INT              API_GpioRequestOne(UINT uiGpio, ULONG ulFlags, CPCHAR pcLabel);
LW_API VOID             API_GpioFree(UINT uiGpio);
LW_API INT              API_GpioRequestArray(PLW_GPIO pgArray, size_t stNum);
LW_API VOID             API_GpioFreeArray(PLW_GPIO pgArray, size_t stNum);
LW_API INT              API_GpioGetFlags(UINT uiGpio, ULONG *pulFlags);
LW_API VOID             API_GpioOpenDrain(UINT uiGpio, BOOL bOpenDrain);
LW_API VOID             API_GpioOpenSource(UINT uiGpio, BOOL bOpenSource);
LW_API INT              API_GpioSetDebounce(UINT uiGpio, UINT uiDebounce);
LW_API INT              API_GpioSetPull(UINT uiGpio, UINT uiType);
LW_API INT              API_GpioDirectionInput(UINT uiGpio);
LW_API INT              API_GpioDirectionOutput(UINT uiGpio, INT iValue);
LW_API INT              API_GpioGetValue(UINT uiGpio);
LW_API VOID             API_GpioSetValue(UINT uiGpio, INT iValue);
LW_API ULONG            API_GpioGetIrq(UINT uiGpio, BOOL bIsLevel, UINT uiType);
LW_API ULONG            API_GpioSetupIrq(UINT uiGpio, BOOL bIsLevel, UINT uiType);
LW_API VOID             API_GpioClearIrq(UINT uiGpio);
LW_API irqreturn_t      API_GpioSvrIrq(UINT uiGpio);

#define gpioInit                API_GpioInit
#define gpioChipAdd             API_GpioChipAdd
#define gpioChipDelete          API_GpioChipDelete
#define gpioChipFind            API_GpioChipFind
#define gpioIsValid             API_GpioIsValid
#define gpioHasDrv              API_GpioHasDrv
#define gpioRequest             API_GpioRequest
#define gpioRequestOne          API_GpioRequestOne
#define gpioFree                API_GpioFree
#define gpioRequestArray        API_GpioRequestArray
#define gpioFreeArray           API_GpioFreeArray
#define gpioGetFlags            API_GpioGetFlags
#define gpioOpenDrain           API_GpioOpenDrain
#define gpioOpenSource          API_GpioOpenSource
#define gpioSetDebounce         API_GpioSetDebounce
#define gpioSetPull             API_GpioSetPull
#define gpioDirectionInput      API_GpioDirectionInput
#define gpioDirectionOutput     API_GpioDirectionOutput
#define gpioGetValue            API_GpioGetValue
#define gpioSetValue            API_GpioSetValue
#define gpioGetIrq              API_GpioGetIrq
#define gpioSetupIrq            API_GpioSetupIrq
#define gpioClearIrq            API_GpioClearIrq
#define gpioSvrIrq              API_GpioSvrIrq

#endif                                                                  /*  LW_CFG_GPIO_EN > 0          */
#endif                                                                  /*  __GPIOLIB_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
