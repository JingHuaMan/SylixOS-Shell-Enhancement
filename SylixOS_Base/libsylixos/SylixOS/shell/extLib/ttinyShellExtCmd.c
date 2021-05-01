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
** 文   件   名: ttinyShellExtCmd.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 11 月 17 日
**
** 描        述: 系统 extension 命令.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  Only for LITE version.
*********************************************************************************************************/
#if defined(__SYLIXOS_LITE)
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
/*********************************************************************************************************
  Extension 证书首部
*********************************************************************************************************/
static  UINT8  _G_ucSylixOSExtCertHeader[16] = {
    0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55,
};
/*********************************************************************************************************
  BSP 证书
*********************************************************************************************************/
LW_WEAK UINT8  _G_ucSylixOSBspCert[32] = {
    0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55,
    0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55,
};
/*********************************************************************************************************
** 函数名称: __tshellExtCmdTc
** 功能描述: 系统命令 "tc"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellExtCmdTc (INT  iArgC, PCHAR  ppcArgV[])
{
    INT                   iC;
    INT                   iPrio  = LW_PRIO_NORMAL;
    INT                   iStack = LW_CFG_THREAD_DEFAULT_STK_SIZE;
    ULONG                 ulArg  = 0ul;
    addr_t                addrT  = (addr_t)PX_ERROR;
    PCHAR                 pcName = LW_NULL;
    CHAR                  cName[LW_CFG_OBJECT_NAME_SIZE];
    
    LW_CLASS_THREADATTR   attr;
    LW_OBJECT_HANDLE      hThread;

    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (sscanf(ppcArgV[1], "%lx", &addrT) != 1) {                       /*  加载到的地址                */
        fprintf(stderr, "address error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (!ALIGNED(addrT, sizeof(FUNCPTR))) {                             /*  地址合法性检查              */
        fprintf(stderr, "address not aligned!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    if ((iArgC > 2) && (*ppcArgV[2] != '-')) {                          /*  如果指定了 Extension 文件   */
#if LW_CFG_DEVICE_EN > 0
#define BUFFER_LEN  512
        INT         iFd;
        ssize_t     stLen;
        size_t      stTotalLen = 0;
        CHAR        cBuf[BUFFER_LEN];
        PCHAR       pcDest = (PCHAR)addrT;
        struct stat stat;

        iFd = open(ppcArgV[2], O_RDONLY, 0666);                         /*  打开 Extension 文件         */
        if (iFd < 0) {
            fprintf(stderr, "failed to open file %s\n", ppcArgV[2]);
            return  (-ERROR_TSHELL_EPARAM);
        }

        if (fstat(iFd, &stat) < 0) {
            close(iFd);                                                 /*  关闭 Extension 文件         */
            fprintf(stderr, "failed to get file %s status\n", ppcArgV[2]);
            return  (-ERROR_TSHELL_EPARAM);
        }

        while (1) {                                                     /*  将 Extension 文件加载到地址 */
            stLen = read(iFd, cBuf, BUFFER_LEN);
            if (stLen > 0) {
                lib_memcpy(pcDest, cBuf, stLen);
                pcDest     += stLen;
                stTotalLen += stLen;

            } else {
                break;
            }
        }

        close(iFd);                                                     /*  关闭 Extension 文件         */

        if (stat.st_size != stTotalLen) {
            fprintf(stderr, "failed to read file %s\n", ppcArgV[2]);
            return  (-ERROR_TSHELL_EPARAM);
        }

#if LW_CFG_CACHE_EN > 0
        API_CacheTextUpdate((PVOID)addrT, stTotalLen);                  /*  Extension 代码段更新        */
#endif
#else
        fprintf(stderr, "failed to open file %s\n", ppcArgV[2]);        /*  没有文件系统支持            */
        return  (-ERROR_TSHELL_EPARAM);
#endif
    }

    while ((iC = getopt(iArgC, ppcArgV, "n:p:s:a:")) != EOF) {          /*  分析选项参数                */
        switch (iC) {
        
        case 'p':
            iPrio = lib_atoi(optarg);
            break;
            
        case 's':
            iStack = lib_atoi(optarg);
            break;
            
        case 'a':
            ulArg = lib_strtoul(optarg, LW_NULL, 16);
            break;
            
        case 'n':
            pcName = optarg;
            break;
        }
    }
    
    getopt_free();
    
    if ((iPrio < LW_PRIO_HIGHEST) || (iPrio >= LW_PRIO_LOWEST)) {       /*  优先级合法性检查            */
        fprintf(stderr, "priority error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (iStack < (ARCH_STK_MIN_WORD_SIZE * sizeof(PVOID))) {            /*  堆栈大小合法性检查          */
        fprintf(stderr, "stack size error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (pcName) {                                                       /*  线程名字合法性检查          */
        if (lib_strnlen(pcName, LW_CFG_OBJECT_NAME_SIZE) >= LW_CFG_OBJECT_NAME_SIZE) {
            fprintf(stderr, "name too long!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
    
    } else {
        snprintf(cName, LW_CFG_OBJECT_NAME_SIZE, "tc_%lx", addrT);      /*  使用地址作线程名字          */
        pcName = cName;
    }

    if (lib_memcmp(_G_ucSylixOSExtCertHeader, (PVOID)addrT,
                   sizeof(_G_ucSylixOSExtCertHeader)) == 0) {           /*  如果Extension使用了认证证书 */

        if (lib_memcmp(_G_ucSylixOSBspCert, (PVOID)addrT,
                       sizeof(_G_ucSylixOSBspCert)) == 0) {             /*  Extension的证书必须与BSP匹配*/
            addrT += sizeof(_G_ucSylixOSBspCert);                       /*  跳过证书                    */

        } else {                                                        /*  证书检查失败                */
            fprintf(stderr, "extension certificate check failure! "\
                            "please rebuild your extension program!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }

    } else {
        /*
         * 为兼容老的使用方式, 暂时支持不使用认证证书来运行 Extension
         */
    }

#if defined(__SYLIXOS_ARM_ARCH_M__)
    addrT |= 1;                                                         /*  进入 Thumb 模式             */
#endif

    /*
     * 创建线程运行 Extension
     */
    API_ThreadAttrBuild(&attr, (size_t)iStack, (UINT8)iPrio,
                        LW_OPTION_OBJECT_LOCAL | LW_OPTION_THREAD_STK_CHK,
                        (PVOID)ulArg);

    hThread = API_ThreadCreate(pcName, (PTHREAD_START_ROUTINE)addrT, &attr, LW_NULL);
    if (!hThread) {
        fprintf(stderr, "can not create thread: %s!\n", lib_strerror(errno));
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellExtCmdInit
** 功能描述: 初始化 Extension 命令集
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __tshellExtCmdInit (VOID)
{
    API_TShellKeywordAdd("tc", __tshellExtCmdTc);
    API_TShellFormatAdd("tc", " address(Hex) [-n name] [-p priority(0~254)] [-s stacksize(Dec)] [-a argument(Hex)]");
    API_TShellHelpAdd("tc", "create a thread and run begin at specific address.\n"
                            "eg. tc 0x???\n"
                            "    tc 0x??? -n test -p 200 -s 4096\n"
                            "    tc 0x??? -n test -p 200 -s 4096\n"
                            "    tc 0x??? -n test -p 200 -s 4096 -a 0x00\n");

    API_TShellKeywordAdd("extension", __tshellExtCmdTc);
    API_TShellFormatAdd("extension", " address(Hex) [filepath] [-n name] [-p priority(0~254)] [-s stacksize(Dec)] [-a argument(Hex)]");
    API_TShellHelpAdd("extension", "load extension from file system to a specific address and create a thread to run it.\n"
                            "eg. extension 0x???\n"
                            "    extension 0x??? -n test -p 200 -s 4096\n"
                            "    extension 0x??? /apps/ext/ext.bin -n test -p 200 -s 4096\n"
                            "    extension 0x??? /apps/ext/ext.bin -n test -p 200 -s 4096 -a 0x00\n");
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
#endif                                                                  /*  __SYLIXOS_LITE              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
