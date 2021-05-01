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
** ��   ��   ��: ttinyShellExtCmd.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 11 �� 17 ��
**
** ��        ��: ϵͳ extension ����.
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
  �ü�����
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
/*********************************************************************************************************
  Extension ֤���ײ�
*********************************************************************************************************/
static  UINT8  _G_ucSylixOSExtCertHeader[16] = {
    0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55,
};
/*********************************************************************************************************
  BSP ֤��
*********************************************************************************************************/
LW_WEAK UINT8  _G_ucSylixOSBspCert[32] = {
    0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55,
    0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55,
};
/*********************************************************************************************************
** ��������: __tshellExtCmdTc
** ��������: ϵͳ���� "tc"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
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
    
    if (sscanf(ppcArgV[1], "%lx", &addrT) != 1) {                       /*  ���ص��ĵ�ַ                */
        fprintf(stderr, "address error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (!ALIGNED(addrT, sizeof(FUNCPTR))) {                             /*  ��ַ�Ϸ��Լ��              */
        fprintf(stderr, "address not aligned!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    if ((iArgC > 2) && (*ppcArgV[2] != '-')) {                          /*  ���ָ���� Extension �ļ�   */
#if LW_CFG_DEVICE_EN > 0
#define BUFFER_LEN  512
        INT         iFd;
        ssize_t     stLen;
        size_t      stTotalLen = 0;
        CHAR        cBuf[BUFFER_LEN];
        PCHAR       pcDest = (PCHAR)addrT;
        struct stat stat;

        iFd = open(ppcArgV[2], O_RDONLY, 0666);                         /*  �� Extension �ļ�         */
        if (iFd < 0) {
            fprintf(stderr, "failed to open file %s\n", ppcArgV[2]);
            return  (-ERROR_TSHELL_EPARAM);
        }

        if (fstat(iFd, &stat) < 0) {
            close(iFd);                                                 /*  �ر� Extension �ļ�         */
            fprintf(stderr, "failed to get file %s status\n", ppcArgV[2]);
            return  (-ERROR_TSHELL_EPARAM);
        }

        while (1) {                                                     /*  �� Extension �ļ����ص���ַ */
            stLen = read(iFd, cBuf, BUFFER_LEN);
            if (stLen > 0) {
                lib_memcpy(pcDest, cBuf, stLen);
                pcDest     += stLen;
                stTotalLen += stLen;

            } else {
                break;
            }
        }

        close(iFd);                                                     /*  �ر� Extension �ļ�         */

        if (stat.st_size != stTotalLen) {
            fprintf(stderr, "failed to read file %s\n", ppcArgV[2]);
            return  (-ERROR_TSHELL_EPARAM);
        }

#if LW_CFG_CACHE_EN > 0
        API_CacheTextUpdate((PVOID)addrT, stTotalLen);                  /*  Extension ����θ���        */
#endif
#else
        fprintf(stderr, "failed to open file %s\n", ppcArgV[2]);        /*  û���ļ�ϵͳ֧��            */
        return  (-ERROR_TSHELL_EPARAM);
#endif
    }

    while ((iC = getopt(iArgC, ppcArgV, "n:p:s:a:")) != EOF) {          /*  ����ѡ�����                */
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
    
    if ((iPrio < LW_PRIO_HIGHEST) || (iPrio >= LW_PRIO_LOWEST)) {       /*  ���ȼ��Ϸ��Լ��            */
        fprintf(stderr, "priority error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (iStack < (ARCH_STK_MIN_WORD_SIZE * sizeof(PVOID))) {            /*  ��ջ��С�Ϸ��Լ��          */
        fprintf(stderr, "stack size error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (pcName) {                                                       /*  �߳����ֺϷ��Լ��          */
        if (lib_strnlen(pcName, LW_CFG_OBJECT_NAME_SIZE) >= LW_CFG_OBJECT_NAME_SIZE) {
            fprintf(stderr, "name too long!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
    
    } else {
        snprintf(cName, LW_CFG_OBJECT_NAME_SIZE, "tc_%lx", addrT);      /*  ʹ�õ�ַ���߳�����          */
        pcName = cName;
    }

    if (lib_memcmp(_G_ucSylixOSExtCertHeader, (PVOID)addrT,
                   sizeof(_G_ucSylixOSExtCertHeader)) == 0) {           /*  ���Extensionʹ������֤֤�� */

        if (lib_memcmp(_G_ucSylixOSBspCert, (PVOID)addrT,
                       sizeof(_G_ucSylixOSBspCert)) == 0) {             /*  Extension��֤�������BSPƥ��*/
            addrT += sizeof(_G_ucSylixOSBspCert);                       /*  ����֤��                    */

        } else {                                                        /*  ֤����ʧ��                */
            fprintf(stderr, "extension certificate check failure! "\
                            "please rebuild your extension program!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }

    } else {
        /*
         * Ϊ�����ϵ�ʹ�÷�ʽ, ��ʱ֧�ֲ�ʹ����֤֤�������� Extension
         */
    }

#if defined(__SYLIXOS_ARM_ARCH_M__)
    addrT |= 1;                                                         /*  ���� Thumb ģʽ             */
#endif

    /*
     * �����߳����� Extension
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
** ��������: __tshellExtCmdInit
** ��������: ��ʼ�� Extension ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
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
