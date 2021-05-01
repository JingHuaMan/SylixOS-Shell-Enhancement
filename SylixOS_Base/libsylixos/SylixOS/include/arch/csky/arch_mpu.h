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
** 文   件   名: arch_mpu.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 11 月 12 日
**
** 描        述: C-SKY MPU 管理相关.
*********************************************************************************************************/

#ifndef __CSKY_ARCH_MPU_H
#define __CSKY_ARCH_MPU_H

#if (LW_CFG_VMM_EN == 0) && (LW_CFG_CSKY_MPU > 0)
/*********************************************************************************************************
  映射描述符
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

typedef struct {
    BOOL        MPUD_bEnable;                                   /*  Enable                              */
    BOOL        MPUD_bDmaPool;                                  /*  DMA buffer pool                     */

    UINT32      MPUD_uiAddr;                                    /*  Start address                       */
    UINT8       MPUD_ucSize;                                    /*  Region size                         */
#define CSKY_MPU_REGION_SIZE_END         ((UINT8)0x00)          /*  The last one                        */
#define CSKY_MPU_REGION_SIZE_4KB         ((UINT8)0x0b)
#define CSKY_MPU_REGION_SIZE_8KB         ((UINT8)0x0c)
#define CSKY_MPU_REGION_SIZE_16KB        ((UINT8)0x0d)
#define CSKY_MPU_REGION_SIZE_32KB        ((UINT8)0x0e)
#define CSKY_MPU_REGION_SIZE_64KB        ((UINT8)0x0f)
#define CSKY_MPU_REGION_SIZE_128KB       ((UINT8)0x10)
#define CSKY_MPU_REGION_SIZE_256KB       ((UINT8)0x11)
#define CSKY_MPU_REGION_SIZE_512KB       ((UINT8)0x12)
#define CSKY_MPU_REGION_SIZE_1MB         ((UINT8)0x13)
#define CSKY_MPU_REGION_SIZE_2MB         ((UINT8)0x14)
#define CSKY_MPU_REGION_SIZE_4MB         ((UINT8)0x15)
#define CSKY_MPU_REGION_SIZE_8MB         ((UINT8)0x16)
#define CSKY_MPU_REGION_SIZE_16MB        ((UINT8)0x17)
#define CSKY_MPU_REGION_SIZE_32MB        ((UINT8)0x18)
#define CSKY_MPU_REGION_SIZE_64MB        ((UINT8)0x19)
#define CSKY_MPU_REGION_SIZE_128MB       ((UINT8)0x1a)
#define CSKY_MPU_REGION_SIZE_256MB       ((UINT8)0x1b)
#define CSKY_MPU_REGION_SIZE_512MB       ((UINT8)0x1c)
#define CSKY_MPU_REGION_SIZE_1GB         ((UINT8)0x1d)
#define CSKY_MPU_REGION_SIZE_2GB         ((UINT8)0x1e)
#define CSKY_MPU_REGION_SIZE_4GB         ((UINT8)0x1f)

    UINT8       MPUD_ucMpuNumber;                               /*  Mpu Region Number                   */

    UINT32      MPUD_uiAttr;                                    /*  Access permission                   */
#define CSKY_MPU_ATTR_NX_POS              0
#define CSKY_MPU_ATTR_NX                  (1 << CSKY_MPU_ATTR_NX_POS)
                                                                /*  Instruction Fetched Execution       */

#define CSKY_MPU_ATTR_AP_POS              1
#define CSKY_MPU_ATTR_AP_BOTH_INACCESS    (0 << CSKY_MPU_ATTR_AP_POS)
                                                                /*  Both Inaccessible                   */
#define CSKY_MPU_ATTR_AP_S_RW_U_INACCESS  (1 << CSKY_MPU_ATTR_AP_POS)
                                                                /*  Super: Read Write/User: Inaccessible*/
#define CSKY_MPU_ATTR_AP_S_RW_U_RO        (2 << CSKY_MPU_ATTR_AP_POS)
                                                                /*  Super: Read Write/User: Read Only   */
#define CSKY_MPU_ATTR_AP_BOTH_RW          (3 << CSKY_MPU_ATTR_AP_POS)
                                                                /*  Both Read Write                     */
#define CSKY_MPU_ATTR_AP                  (3 << CSKY_MPU_ATTR_AP_POS)

#define CSKY_MPU_ATTR_S_POS               3
#define CSKY_MPU_ATTR_S                   (1 << CSKY_MPU_ATTR_S_POS)
                                                                /*  Security                            */
} CSKY_MPU_REGION;
typedef CSKY_MPU_REGION    *PCSKY_MPU_REGION;

#endif                                                          /*  __SYLIXOS_KERNEL                    */
#endif                                                          /*  LW_CFG_CSKY_MPU > 0                 */
#endif                                                          /*  __CSKY_ARCH_MPU_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
