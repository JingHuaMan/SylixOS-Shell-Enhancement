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
** ��   ��   ��: distribute.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2016 �� 06 �� 10 ��
**
** ��        ��: SylixOS ��������Ϣ.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  Ĭ�� Logo ��Ϣ
*********************************************************************************************************/
#if LW_CFG_KERNEL_LOGO > 0
extern  const   CHAR  _K_cKernelLogo[];
#endif                                                                  /*  LW_CFG_KERNEL_LOGO > 0      */
/*********************************************************************************************************
** ��������: API_SystemLogoPrint
** ��������: ��ӡϵͳ LOGO
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_SystemLogoPrint (VOID)
{
    INT     iLogoFd;
    CHAR    cBuf[128];
    ssize_t sstNum;
    
    iLogoFd = open("/etc/oemlogo", O_RDONLY);
    if (iLogoFd < 0) {
#if LW_CFG_KERNEL_LOGO > 0
        write(STD_OUT, (CPVOID)_K_cKernelLogo, lib_strlen((PCHAR)_K_cKernelLogo));
#else
        write(STD_OUT, "\nKERNEL: Long Wing(C) 2006 - 2016\n",  \
              lib_strlen("\nKERNEL: Long Wing(C) 2006 - 2016\n"));
#endif                                                                  /*  LW_CFG_KERNEL_LOGO > 0      */
        return  (ERROR_NONE);
    }
    
    write(STD_OUT, "\n", 1);
    
    do {
        sstNum = read(iLogoFd, cBuf, sizeof(cBuf));
        if (sstNum > 0) {
            write(STD_OUT, cBuf, (size_t)sstNum);
        }
    } while (sstNum > 0);
    
    write(STD_OUT, "\n", 1);
    
    close(iLogoFd);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SystemHwInfoPrint
** ��������: ��ӡϵͳӲ����Ϣ
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_SystemHwInfoPrint (VOID)
{
    printf("%s\n", __SYLIXOS_LICENSE);
    printf("%s\n", __SYLIXOS_VERINFO);
    printf("\n");
    printf("CPU     : %s\n", bspInfoCpu());
    printf("CACHE   : %s\n", bspInfoCache());
    printf("PACKET  : %s\n", bspInfoPacket());
    printf("ROM SIZE: 0x%08zx Bytes (0x%08lx - 0x%08lx)\n",
           bspInfoRomSize(),
           bspInfoRomBase(),
           (bspInfoRomBase() + bspInfoRomSize() - 1));
    printf("RAM SIZE: 0x%08zx Bytes (0x%08lx - 0x%08lx)\n",
           bspInfoRamSize(),
           bspInfoRamBase(),
           (bspInfoRamBase() + bspInfoRamSize() - 1));
    printf("BSP     : %s\n", bspInfoVersion());
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
