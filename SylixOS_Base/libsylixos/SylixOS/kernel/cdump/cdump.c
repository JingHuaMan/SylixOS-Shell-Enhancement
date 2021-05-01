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
** ��   ��   ��: cdump.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 12 �� 01 ��
**
** ��        ��: ϵͳ/Ӧ�ñ�����Ϣ��¼.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if (LW_CFG_CDUMP_EN > 0) && (LW_CFG_DEVICE_EN > 0)
/*********************************************************************************************************
** ��������: API_CrashDumpBuffer
** ��������: ���¶�λϵͳ/Ӧ�ñ�����Ϣ��¼λ��. (�������ں��ܷ��ʵĵ�ַ)
** �䡡��  : pvCdump           �����ַ
**           stSize            �����С
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_CrashDumpBuffer (PVOID  pvCdump, size_t  stSize)
{
    if (!pvCdump || (pvCdump == (PVOID)PX_ERROR) || (stSize < 512)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    _CrashDumpSet(pvCdump, stSize);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_CrashDumpSave
** ��������: ���һ��ϵͳ/Ӧ�ñ�����Ϣ�������ļ�.
** �䡡��  : pcLogFile         ��־�ļ���
**           iFlag             open �ڶ�������
**           iMode             open ����������
**           bClear            �ɹ�������Ƿ���ջ�����
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_CrashDumpSave (CPCHAR  pcLogFile, INT  iFlag, INT  iMode, BOOL  bClear)
{
    PCHAR   pcCdump, pcBuffer;
    size_t  stSize, stLen;
    INT     iFd;
    
    pcCdump = (PCHAR)_CrashDumpGet(&stSize);
    if (!pcCdump || (pcCdump == (PCHAR)PX_ERROR) || (stSize < 512)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (((UINT8)pcCdump[0] != LW_CDUMP_MAGIC_0) ||
        ((UINT8)pcCdump[1] != LW_CDUMP_MAGIC_1) ||
        ((UINT8)pcCdump[2] != LW_CDUMP_MAGIC_2) ||
        ((UINT8)pcCdump[3] != LW_CDUMP_MAGIC_3)) {
        _ErrorHandle(EMSGSIZE);
        return  (PX_ERROR);
    }
    
    pcCdump[stSize - 1] = PX_EOS;
    
    iFd = open(pcLogFile, iFlag, iMode);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    pcBuffer = &pcCdump[4];
    stLen    = lib_strlen(pcBuffer);
    
    if (write(iFd, pcBuffer, stLen) != stLen) {
        close(iFd);
        return  (PX_ERROR);
    }
    
    close(iFd);
    
    if (bClear) {
        pcCdump[0] = 0;
        pcCdump[1] = 0;
        pcCdump[2] = 0;
        pcCdump[3] = 0;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_CrashDumpShow
** ��������: ��ʾ���һ��ϵͳ/Ӧ�ñ�����Ϣ.
** �䡡��  : iFd               ��ӡ�ļ�������
**           bClear            �Ƿ���ջ�����
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_CrashDumpShow (INT  iFd, BOOL  bClear)
{
    PCHAR   pcCdump, pcBuffer;
    size_t  stSize;
    
    pcCdump = (PCHAR)_CrashDumpGet(&stSize);
    if (!pcCdump || (pcCdump == (PCHAR)PX_ERROR) || (stSize < 512)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (((UINT8)pcCdump[0] != LW_CDUMP_MAGIC_0) ||
        ((UINT8)pcCdump[1] != LW_CDUMP_MAGIC_1) ||
        ((UINT8)pcCdump[2] != LW_CDUMP_MAGIC_2) ||
        ((UINT8)pcCdump[3] != LW_CDUMP_MAGIC_3)) {
        _ErrorHandle(EMSGSIZE);
        return  (PX_ERROR);
    }
    
    pcCdump[stSize - 1] = PX_EOS;
    pcBuffer            = &pcCdump[4];
    
    fdprintf(iFd, "%s", pcBuffer);
    
    if (bClear) {
        pcCdump[0] = 0;
        pcCdump[1] = 0;
        pcCdump[2] = 0;
        pcCdump[3] = 0;
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_CDUMP_EN > 0         */
                                                                        /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
