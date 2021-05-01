/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: arm64_gdb.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 08 月 27 日
**
** 描        述: ARM64 体系构架 GDB 调试接口.
*********************************************************************************************************/

#ifndef __ARCH_ARM64_GDB_H
#define __ARCH_ARM64_GDB_H

/*********************************************************************************************************
  最大寄存器数
*********************************************************************************************************/
#define GDB_FPU_REG_CNT     (32)
#define GDB_MAX_REG_CNT     (34 + (GDB_FPU_REG_CNT << 1) + 2)

/*********************************************************************************************************
  寄存器集合结构
  每 2 个 GDBRA_ulValue 组成一个 128 位 FPU 寄存器
*********************************************************************************************************/
typedef struct {
    INT         GDBR_iRegCnt;                                           /*  寄存器数量                  */
    INT         GDBR_iPstateInx;                                        /*  PSTATE 的序号               */
    INT         GDBR_iFpsrInx;                                          /*  FPSR   的序号               */
    INT         GDBR_iFpcrInx;                                          /*  FPCR   的序号               */
    struct {
        ULONG   GDBRA_ulValue;                                          /*  寄存器值                    */
    } regArr[GDB_MAX_REG_CNT];                                          /*  寄存器数组                  */
} GDB_REG_SET;

/*********************************************************************************************************
  Xfer:features:read:target.xml 与 Xfer:features:read:arm64-core.xml 回应包
*********************************************************************************************************/
CPCHAR  archGdbTargetXml(VOID);

CPCHAR  archGdbCoreXml(VOID);

/*********************************************************************************************************
  gdb 需要的和体系结构相关的功能
*********************************************************************************************************/
INT     archGdbRegsGet(PVOID               pvDtrace, 
                       LW_OBJECT_HANDLE    ulThread, 
                       GDB_REG_SET        *pregset);                    /*  获取系统寄存器信息          */

INT     archGdbRegsSet(PVOID               pvDtrace, 
                       LW_OBJECT_HANDLE    ulThread, 
                       GDB_REG_SET        *pregset);                    /*  设置系统寄存器信息          */

INT     archGdbRegSetPc(PVOID              pvDtrace, 
                        LW_OBJECT_HANDLE   ulThread, 
                        ULONG              uiPc);                       /*  设置 pc 寄存器              */

ULONG   archGdbRegGetPc(GDB_REG_SET       *pRegs);                      /*  获取 pc 寄存器值            */

ULONG   archGdbGetNextPc(PVOID             pvDtrace,
                         LW_OBJECT_HANDLE  ulThread,
                         GDB_REG_SET      *pRegs);                      /*  获取下一个 pc 值，含分支预测*/

BOOL    archGdbGetStepSkip(PVOID            pvDtrace,
                           LW_OBJECT_HANDLE ulThread,
                           addr_t           ulAddr);                    /*  是否忽略本次单步点          */

#endif                                                                  /*  __ARCH_ARM64_GDB_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
