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
** ��   ��   ��: ioPath.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 02 �� 16 ��
**
** ��        ��: ϵͳ IO �ڲ����ܺ����⣬(PATH)

** BUG
2007.06.04  Line214 �� ppcArray++; Ӧ����ÿ�� while(...) ����һ�Ρ�
2007.09.12  ����ü�֧�֡�
2008.03.27  �޸��˴���ĸ�ʽ.
2008.10.07  ����� _PathCondense() ������һ���� BUG.
2008.10.09  �� _PathCondense() ����Ϊ��ʵ������ķָ��ʱ, �ͺ���.
2008.11.21  ���� _PathGetDef() ֧���߳�˽�� io ��ǰ·��.
2009.12.14  ��ϵͳʹ�÷ּ�Ŀ¼����ʱ, ʹ�� _PathCondense() �����а����豸����ȫ·��ѹ��.
2009.12.17  ���� _PathCondense() ����, ������Ҫ�޴�Ķ�ջ֧��.
2010.10.23  ȥ����Ŀǰ�Ѿ��ܶ಻��Ҫ�Ĺ���, ���������.
2011.02.17  ����ע��.
2011.08.07  ���� _PathBuildLink() ����, ʹ֮������.
2012.03.20  ���ٶ� _K_ptcbTCBCur ������, �������þֲ�����, ���ٶԵ�ǰ CPU ID ��ȡ�Ĵ���.
2012.10.25  _PathGetDef() ������·�������ȼ���: ����(��)->��ǰ����(��)->ȫ��(��)
2012.12.17  ���� _PathGetFull() ����.
2013.01.09  _PathGetDef() ʹ�� _IosEnvGetDef() ��ȡ��ǰ IO ����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
  PATH
*********************************************************************************************************/
static PCHAR _PathSlashRindex(CPCHAR  pcString);
static PCHAR _Strcatlim(PCHAR   pcStr1,
                        CPCHAR  pcStr2,
                        size_t  stLimit);
/*********************************************************************************************************
** ��������: _PathCondense
** ��������: ѹ��һ���ļ�����Ŀ¼��
** �䡡��  : 
**           pcPathName                       ����
** �䡡��  : VOID
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#define __LW_PATH_IS_DIVIDER(c) \
        (c == '/' || c == '\\')

#define __LW_PATH_PUTBACK(pcPtr, pcRoot)    \
        while (pcPtr != pcRoot) {   \
            pcPtr--;    \
            if (__LW_PATH_IS_DIVIDER(*pcPtr)) { \
                break;  \
            }   \
        }
        
VOID  _PathCondense (PCHAR  pcPathName)
{
    PCHAR    pcTail;                                                    /*  ѹ����ʼ��                  */
    PCHAR    pcNode;                                                    /*  ��ǰ�ڵ�                    */
    BOOL     bCheakDot = LW_TRUE;                                       /*  �Ƿ��� . �� ..            */
    BOOL     bNotInc   = LW_FALSE;                                      /*  �Ƿ���ǰ�����˱���ָ��      */
    
    PCHAR    pcTemp;                                                    /*  ������                      */
    INT      iDotNum = 0;                                               /*  ������ĸ���                */
    
    if ((pcPathName == LW_NULL) || (*pcPathName == PX_EOS)) {           /*  ��ЧĿ¼                    */
        return;
    }
    
    if (pcPathName[0] == PX_ROOT) {                                     /*  �Ӹ�Ŀ¼��ʼ                */
        /*
         *  ������ʼΪ��Ŀ¼.
         */
        pcTail  = pcPathName;
    } else {
        iosDevFind(pcPathName, &pcTail);                                /*  ���豸β����ʼѹ��          */
    }
    pcNode  = pcTail;
    pcTemp  = pcTail;
    
    /*
     *  ��ʼ����, ͬʱ�����е� '\' ת���� '/'! (��Щ�ļ�ϵͳ��������ʹ�� / ��Ϊ�ָ�)
     */
    while (*pcTemp != PX_EOS) {                                         /*  ����·��                    */
        if (__LW_PATH_IS_DIVIDER(*pcTemp)) {                            /*  �ָ���                      */
            *pcTemp = PX_DIVIDER;                                       /*  ת���ָ���                  */

            if (iDotNum > 1) {                                          /*  ˫��, ��������һ��Ŀ¼      */
                /*
                 *  XXX ��ʼ�ַ�����Ϊ / ��������ĺ��˻���һ���ַ�!
                 */
                __LW_PATH_PUTBACK(pcNode, pcTail);                      /*  �˻ص���һ��Ŀ¼            */
            } else if (iDotNum == 1) {                                  /*  ����                        */
                /*
                 *  ʲôҲ������, ֱ�Ӻ��Դ�Ŀ¼.
                 */
            } else {
                /*
                 *  ����ֱ�ӿ����ָ��������ı䱣��ָ��.
                 */
                *pcNode = *pcTemp;                                      /*  ֱ�ӿ���                    */
                bNotInc = LW_TRUE;
            }

            /*
             *  ���ַָ���ʱӦ�ÿ�ʼ��� . �� ..
             */
            iDotNum   = 0;                                              /*  ���¿�ʼ��� . �� ..        */
            bCheakDot = LW_TRUE;
        
        } else if ((*pcTemp == '.') && bCheakDot) {                     /*  ��                          */
            iDotNum++;

        } else {                                                        /*  ��ͨ�ַ�                    */
            /*
             *  ������ڷָ������µ�ǰ�ı���ָ��û����ǰ�ƽ�, ������Ҫ����.
             */
            if (bNotInc) {
                bNotInc = LW_FALSE;
                pcNode++;                                               /*  ��Ҫ��ǰ����                */
            }
            
            /*
             *  ����ַ�ǰ�����˴����жϵ� . ������Ҫ����.
             */
            if (bCheakDot) {
                INT         i;
                for (i = 0; i < iDotNum; i++) {
                     *pcNode++ = '.';                                   /*  ��֮ǰ�� . ������           */
                }
                
                bCheakDot = LW_FALSE;                                   /*  �����. �� ..               */
                iDotNum   = 0;
            }

            *pcNode++ = *pcTemp;                                        /*  ��������ַ�                */
        }

        pcTemp++;                                                       /*  ��һ���ַ�                  */
    }

    /*
     *  ����� .. ����, ��Ҫ������, (�������ַ������ָ����Ĵ������)
     */
    if (bCheakDot && (iDotNum > 1)) {                                   /*  �����˫��, ��������һ��Ŀ¼*/
        __LW_PATH_PUTBACK(pcNode, pcTail);                              /*  �˻ص���һ��Ŀ¼            */
    }

    if (bNotInc) {                                                      /*  ����ʱ���ָ����� /        */
        pcNode++;                                                       /*  ��Ҫ����һ���ֽ�            */
    }
    *pcNode = PX_EOS;                                                   /*  ����                        */

    /*
     *  ������һ���ֽ�Ϊ����ķָ���, ����ȥ��
     */
    if (((pcNode - pcTail) > 1) && 
        __LW_PATH_IS_DIVIDER(*(pcNode - 1))) {
        *(pcNode - 1) = PX_EOS;
    }
}
/*********************************************************************************************************
** ��������: _PathCat
** ��������: ����Ŀ¼���ļ���
** �䡡��  : 
**           pcDirName                     Ŀ¼��
**           pcFileName                    �ļ���
**           pcResult                      ���
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  _PathCat (CPCHAR  pcDirName,
                 CPCHAR  pcFileName,
                 PCHAR   pcResult)
{
    PCHAR    pcTail;
    
    if ((pcFileName == LW_NULL) || (pcFileName[0] == PX_EOS)) {
	    lib_strcpy(pcResult, pcDirName);
	    return  (ERROR_NONE);
	}
	
	if ((pcDirName == LW_NULL) || (pcDirName[0] == PX_EOS)) {
	    lib_strcpy(pcResult, pcFileName);
	    return  (ERROR_NONE);
	}
	
	if (lib_strnlen(pcFileName, MAX_FILENAME_LENGTH) >=
        MAX_FILENAME_LENGTH) {                                          /*  ����̫��                    */
	    return  (ENAMETOOLONG);
	}
	
	iosDevFind(pcFileName, &pcTail);
	if (pcTail != pcFileName) {                                         /*  pcFileName Ϊ�豸��         */
	    lib_strcpy(pcResult, pcFileName);                               /*  ֱ�ӷ����豸��              */
	    return  (ERROR_NONE);
	}
	
	*pcResult = PX_EOS;
	
#if LW_CFG_PATH_VXWORKS > 0                                             /*  �����зּ�Ŀ¼����          */
	/* 
	 *  �� '/' ��ʼ���豸��.
     */
    if (pcDirName[0] != PX_ROOT) {
        iosDevFind(pcDirName, &pcTail);
        if (pcTail != pcDirName) {
            lib_strncat(pcResult, pcDirName, pcTail - pcDirName);
	        pcDirName = pcTail;
	    }
	}
#endif                                                                  /*  LW_CFG_PATH_VXWORKS > 0     */
	
	/* 
	 *  if filename is relative path, prepend directory if any 
	 */
	if (lib_index("/\\~$", pcFileName[0]) == LW_NULL) {
	    if (pcDirName[0] != PX_EOS) {
	        _Strcatlim(pcResult, pcDirName, PATH_MAX);
	        if (pcDirName[lib_strlen(pcDirName) - 1] != PX_DIVIDER) {
	            _Strcatlim(pcResult, PX_STR_DIVIDER, PATH_MAX);
	        }
	    }
	}
	
	/* 
	 *  concatenate filename 
	 */
	_Strcatlim(pcResult, pcFileName, PATH_MAX);
	
	return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _Strcatlim
** ��������: �����ƵĽ�һ���ַ����������������һ���ַ���.
** �䡡��  : pcStr1         ԭʼ�ַ���
**           pcStr2         ����ַ���
**           stLimit        ��������
** �䡡��  : ���ַ���ͷ.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PCHAR  _Strcatlim (PCHAR   pcStr1,
                          CPCHAR  pcStr2,
                          size_t  stLimit)
{
    REGISTER size_t stN1;
    REGISTER size_t stN2;
    
    stN1 = lib_strlen(pcStr1);
    if (stN1 < stLimit) {
        stN2 = lib_strlen(pcStr2);
        stN2 = __MIN(stN2, (stLimit - stN1));
        
        /*  bcopy(pcStr2, &pcStr1 [iN1], iN2);
         *  void  bcopy(const void *src, void *dest, int n);
         *  void *memcpy(void *dest, void *src, unsigned int count);
         */
        lib_memcpy(&pcStr1[stN1], pcStr2, stN2);
        pcStr1[stN1 + stN2] = PX_EOS;
    }
    
    return  (pcStr1);
}
/*********************************************************************************************************
** ��������: _PathLastNamePtr
** ��������: ����·���е����һ������ָ��
** �䡡��  : 
**           pcPathName                    ·����
** �䡡��  : ָ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PCHAR  _PathLastNamePtr (CPCHAR  pcPathName)
{
    REGISTER PCHAR    pcP;
	
    pcP = _PathSlashRindex(pcPathName);
    
    if (pcP == LW_NULL) {
        return  ((PCHAR)pcPathName);
    } else {
        return  ((PCHAR)(pcP + 1));
    }
}
/*********************************************************************************************************
** ��������: _PathLastName
** ��������: ����·���е����һ������
** �䡡��  : 
**           pcPathName                    ·����
** �䡡��  : VOID
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _PathLastName (CPCHAR  pcPathName, PCHAR  *ppcLastName)
{
    *ppcLastName = _PathLastNamePtr(pcPathName);
}
/*********************************************************************************************************
** ��������: _PathSlashRindex
** ��������: �����ַ����е���� '/' �� '\' ָ��
** �䡡��  : 
**           pcString                      �ַ���
** �䡡��  : VOID
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PCHAR  _PathSlashRindex (CPCHAR  pcString)
{
    REGISTER PCHAR    pcForward;
    REGISTER PCHAR    pcBack;
    
    REGISTER PCHAR    pcLast;
    
    pcForward = lib_rindex(pcString, '/');
    pcBack    = lib_rindex(pcString, '\\');
    
    pcLast    = __MAX(pcForward, pcBack);
    
    return  (pcLast);
}
/*********************************************************************************************************
** ��������: _PathGetDef
** ��������: ��õ�ǰ·��ָ��
** �䡡��  : NONE
** �䡡��  : ��ǰ·��ָ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PCHAR  _PathGetDef (VOID)
{
    PLW_IO_ENV      pioe = _IosEnvGetDef();
    
    return  (pioe->IOE_cDefPath);
}
/*********************************************************************************************************
** ��������: _PathGetFull
** ��������: ����ļ�����·����
** �䡡��  : pcFullPath            �����ļ������
**           stMaxSize             ��������С
**           pcFileName            �ļ���
** �䡡��  : ����ɹ��򷵻������ļ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PCHAR  _PathGetFull (PCHAR  pcFullPath, size_t  stMaxSize, CPCHAR  pcFileName)
{
    if (stMaxSize < 2) {
        return  (LW_NULL);
    }

    if (pcFileName[0] != PX_ROOT) {
        size_t  stCpLen = lib_strlcpy(pcFullPath, _PathGetDef(), stMaxSize - 1);
        if ((stCpLen > 1) && (pcFullPath[stCpLen - 1] != PX_DIVIDER)) {
            pcFullPath[stCpLen] = PX_DIVIDER;
            stCpLen++;
        }
        lib_strlcpy(&pcFullPath[stCpLen], pcFileName, stMaxSize - stCpLen);
    
    } else {
        lib_strlcpy(pcFullPath, pcFileName, stMaxSize);
    }
    
    _PathCondense(pcFullPath);
    
    return  (pcFullPath);
}
/*********************************************************************************************************
** ��������: _PathBuildLink
** ��������: ������Ŀ¼����Ϊһ��Ŀ¼ (���ڷ��������ļ�)
** �䡡��  : pcBuildName   �������
**           stMaxSize     ��󳤶�
**           pcDevName     �豸��          ����: /yaffs2
**           pcPrefix      Ŀ¼ǰ׺              /n1/dir1
**           pcLinkDst     ����Ŀ��              linkdst or /linkdst
**           pcTail        ����β��              /subdir
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _PathBuildLink (PCHAR      pcBuildName, 
                     size_t     stMaxSize, 
                     CPCHAR     pcDevName,
                     CPCHAR     pcPrefix,
                     CPCHAR     pcLinkDst,
                     CPCHAR     pcTail)
{
    size_t  stLenDevName = 0;
    size_t  stLenPrefix = 0;
    size_t  stLenTail = 0;
    size_t  stLenLink;
    size_t  stLenTotal;
    
    PCHAR   pcBuffer;
    
    /*
     *  ��Ϊ pcTail ������ pcBuffer ��, ���Բ���ֱ��ʹ�� snprintf(..., "%s%s", ...) ����Ŀ¼.
     */
    if ((pcBuildName == LW_NULL) ||
        (stMaxSize   == 0) || 
        (pcLinkDst   == LW_NULL)) {                                     /*  �������                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pcDevName) {
        stLenDevName = lib_strlen(pcDevName);                           /*  ȷ�� dev name ����          */
    }
    if (pcPrefix) {
        stLenPrefix = lib_strlen(pcPrefix);                             /*  ȷ�� ǰ׺ ����              */
    }
    if (pcTail) {
        stLenTail = lib_strlen(pcTail);                                 /*  ȷ�� tail ����              */
    }
    stLenLink = lib_strlen(pcLinkDst);
    
    if ((stLenDevName + stLenLink + stLenPrefix) == 0) {                /*  ֱ����� pcLinkDst          */
        lib_strlcpy(pcBuildName, pcLinkDst, stMaxSize);
        return  (ERROR_NONE);
    }
    
    stLenTotal = stLenDevName + stLenPrefix + stLenTail + stLenLink + 4;
    pcBuffer   = (PCHAR)__SHEAP_ALLOC(stLenTotal);
    if (pcBuffer == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    if (pcDevName == LW_NULL) {
        pcDevName =  "";
    }
    if (pcPrefix == LW_NULL) {
        pcPrefix =  "";
    }
    if (pcTail == LW_NULL) {
        pcTail =  "";
    }
    
    if (*pcLinkDst == PX_ROOT) {                                        /*  ���Ե�ַ��д�豸����ǰ׺    */
        if ((*pcTail != PX_EOS) && (*pcTail != PX_DIVIDER)) {
            snprintf(pcBuffer, stLenTotal, 
                     "%s"PX_STR_DIVIDER"%s", pcLinkDst, pcTail);
        } else {
            snprintf(pcBuffer, stLenTotal, 
                     "%s%s", pcLinkDst, pcTail);
        }
        
    } else {
        if ((*pcTail != PX_EOS) && (*pcTail != PX_DIVIDER)) {
            snprintf(pcBuffer, stLenTotal, 
                     "%s%s"PX_STR_DIVIDER"%s"PX_STR_DIVIDER"%s", 
                     pcDevName, pcPrefix, pcLinkDst, pcTail);
        } else {
            snprintf(pcBuffer, stLenTotal, 
                     "%s%s"PX_STR_DIVIDER"%s%s", 
                     pcDevName, pcPrefix, pcLinkDst, pcTail);
        }
    }
    
    lib_strlcpy(pcBuildName, pcBuffer, stMaxSize);                      /*  �������                    */
    
    __SHEAP_FREE(pcBuffer);
    
    _PathCondense(pcBuildName);                                         /*  ѹ��Ŀ¼                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _PathMoveCheck
** ��������: ��һ���ڵ�(Ŀ¼���ļ�)�ƶ�(��������)����һ��Ŀ¼, ����Ƿ�Ϸ�.
** �䡡��  : pcSrc         Դ�ļ���Ŀ¼
**           pcDest        Ŀ���ļ���Ŀ¼
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  _PathMoveCheck (CPCHAR  pcSrc, CPCHAR  pcDest)
{
    CPCHAR  pcPos;

    if (*pcSrc == PX_DIVIDER) {
        pcSrc++;
    }

    if (*pcDest == PX_DIVIDER) {
        pcDest++;
    }

    if (lib_strstr(pcDest, pcSrc) == pcDest) {
        pcPos = pcDest + lib_strlen(pcSrc);
        if (*pcPos == PX_DIVIDER) {
            return  (PX_ERROR);                                         /*  ��������Ŀ¼������Ŀ¼    */
        }
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
