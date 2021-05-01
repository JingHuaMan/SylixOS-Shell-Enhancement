/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: armExcV7M.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 11 月 14 日
**
** 描        述: ARMv7M 体系构架异常处理.
*********************************************************************************************************/

#ifndef __ARMEXCV7M_H
#define __ARMEXCV7M_H

/*********************************************************************************************************
  Structure type to access the System Control Block (SCB)
*********************************************************************************************************/
typedef struct {
    UINT32      CPUID;          /*  0x000 (R/ )  CPUID Base Register                                    */
    UINT32      ICSR;           /*  0x004 (R/W)  Interrupt Control and State Register                   */
    UINT32      VTOR;           /*  0x008 (R/W)  Vector Table Offset Register                           */
    UINT32      AIRCR;          /*  0x00C (R/W)  Application Interrupt and Reset Control Register       */
    UINT32      SCR;            /*  0x010 (R/W)  System Control Register                                */
    UINT32      CCR;            /*  0x014 (R/W)  Configuration Control Register                         */
    UINT8       SHP[12];        /*  0x018 (R/W)  System Handlers Priority Registers (4-7, 8-11, 12-15)  */
    UINT32      SHCSR;          /*  0x024 (R/W)  System Handler Control and State Register              */
    UINT32      CFSR;           /*  0x028 (R/W)  Configurable Fault Status Register                     */
    UINT32      HFSR;           /*  0x02C (R/W)  HardFault Status Register                              */
    UINT32      DFSR;           /*  0x030 (R/W)  Debug Fault Status Register                            */
    UINT32      MMFAR;          /*  0x034 (R/W)  MemManage Fault Address Register                       */
    UINT32      BFAR;           /*  0x038 (R/W)  BusFault Address Register                              */
    UINT32      AFSR;           /*  0x03C (R/W)  Auxiliary Fault Status Register                        */
    UINT32      PFR[2];         /*  0x040 (R/ )  Processor Feature Register                             */
    UINT32      DFR;            /*  0x048 (R/ )  Debug Feature Register                                 */
    UINT32      ADR;            /*  0x04C (R/ )  Auxiliary Feature Register                             */
    UINT32      MMFR[4];        /*  0x050 (R/ )  Memory Model Feature Register                          */
    UINT32      ISAR[5];        /*  0x060 (R/ )  Instruction Set Attributes Register                    */
    UINT32      RESERVED0[5];
    UINT32      CPACR;          /*  0x088 (R/W)  Coprocessor Access Control Register                    */
} SCB_Type;
/*********************************************************************************************************
  Memory mapping of Cortex-M Hardware
*********************************************************************************************************/
#define SCS_BASE            (0xe000e000)                                /*  System Control Space Address*/
#define SCB_BASE            (SCS_BASE + 0x0d00)                         /*  System Control Block Address*/
#define SCB                 ((SCB_Type *)SCB_BASE)
/*********************************************************************************************************
  数据类型定义
*********************************************************************************************************/
typedef enum {
    UNALIGN_TRP = 0x00000008,
    DIV_0_TRP   = 0x00000010,
} CONFIG_CTRL_BITS;

typedef enum {
    BUSFAULTENA = 0x00020000,
    USGFAULTENA = 0x00040000,
} SYS_HANDLER_CSR_BITS;

typedef enum {
    IBUSERR     = 0x00000100,
    PRECISERR   = 0x00000200,
    IMPRECISERR = 0x00000400,
    UNSTKERR    = 0x00000800,
    STKERR      = 0x00001000,
    BFARVALID   = 0x00008000,
    UNDEFINSTR  = 0x00010000,
    INVSTATE    = 0x00020000,
    INVPC       = 0x00040000,
    NOCP        = 0x00080000,
    UNALIGNED   = 0x01000000,
    DIVBYZERO   = 0x02000000,
} LOCAL_FAULT_STATUS_BITS;

typedef enum {
    VECTTBL     = 0x00000002,
    FORCED      = 0x40000000,
} HARD_FAULT_STATUS_BITS;

typedef enum {
    HARDFAULT   = 3,
    BUSFAULT    = 5,
    USAGEFAULT  = 6,
} INTERRUPTS;

typedef struct {
    CHAR   *pcName;
    INT     iTestBit;
    INT     iHandler;
} TRAPS;
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
VOID  armv7mTrapsInit(VOID);

VOID  armv7mFaultPrintInfo(INTERRUPTS  in,
                           addr_t      ulAbortAddr,
                           UINT32      uiHStatus,
                           UINT32      uiLStatus);

#endif                                                                  /*  __ARMEXCV7M_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
