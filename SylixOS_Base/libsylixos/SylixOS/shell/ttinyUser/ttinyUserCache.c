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
** ��   ��   ��: ttinyUserAuthen.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 05 �� 21 ��
**
** ��        ��: shell �û��������Ļ���
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "unistd.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
#include "shadow.h"
#include "pwd.h"
#include "grp.h"
#include "../ttinyShell/ttinyShellLib.h"
/*********************************************************************************************************
  �û�/�黺��
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE            UC_lineManage;
    uid_t                   UC_uid;
    PCHAR                   UC_pcHome;
    CHAR                    UC_cName[1];
} __TSHELL_UCACHE;
typedef __TSHELL_UCACHE    *__PTSHELL_UCACHE;

typedef struct {
    LW_LIST_LINE            GC_lineManage;
    gid_t                   GC_gid;
    CHAR                    GC_cName[1];
} __TSHELL_GCACHE;
typedef __TSHELL_GCACHE    *__PTSHELL_GCACHE;
/*********************************************************************************************************
  ����
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  _G_plineUsrCache;
static LW_LIST_LINE_HEADER  _G_plineGrpCache;
/*********************************************************************************************************
** ��������: __tshellGetUserName
** ��������: ���һ���û���
** �䡡��  : uid           �û� id
**           pcName        �û���
**           stNSize       �û�����������С
**           pcHome        �û���Ŀ¼
**           stHSize       �û���Ŀ¼��������С
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __tshellGetUserName (uid_t  uid, PCHAR  pcName, size_t  stNSize, PCHAR  pcHome, size_t  stHSize)
{
    PLW_LIST_LINE       plineTmp;
    __PTSHELL_UCACHE    puc;
    struct passwd       passwd;
    struct passwd      *ppasswd = LW_NULL;
    CHAR                cBuffer[MAX_FILENAME_LENGTH];
    
    __TTINY_SHELL_LOCK();                                               /*  �������                    */
    for (plineTmp  = _G_plineUsrCache;
         plineTmp != LW_NULL;
         plineTmp  = _list_line_get_next(plineTmp)) {
        
        puc = _LIST_ENTRY(plineTmp, __TSHELL_UCACHE, UC_lineManage);
        if (puc->UC_uid == uid) {
            if (pcName && stNSize) {
                lib_strlcpy(pcName, puc->UC_cName, stNSize);
            }
            if (pcHome && stHSize) {
                lib_strlcpy(pcHome, puc->UC_pcHome, stHSize);
            }
            if (plineTmp != _G_plineUsrCache) {
                _List_Line_Del(&puc->UC_lineManage, 
                               &_G_plineUsrCache);
                _List_Line_Add_Ahead(&puc->UC_lineManage, 
                                     &_G_plineUsrCache);                /*  ���ʹ�õķ��ڱ�ͷ          */
            }
            __TTINY_SHELL_UNLOCK();                                     /*  �ͷ���Դ                    */
            return  (ERROR_NONE);
        }
    }
    __TTINY_SHELL_UNLOCK();                                             /*  �ͷ���Դ                    */
    
    getpwuid_r(uid, &passwd, cBuffer, sizeof(cBuffer), &ppasswd);
    if (!ppasswd) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    puc = (__PTSHELL_UCACHE)__SHEAP_ALLOC(sizeof(__TSHELL_UCACHE) + 
                                          lib_strlen(ppasswd->pw_name));
    if (!puc) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    puc->UC_pcHome = (PCHAR)__SHEAP_ALLOC(lib_strlen(ppasswd->pw_dir) + 1);
    if (!puc->UC_pcHome) {
        __SHEAP_FREE(puc);
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
                                          
    puc->UC_uid = uid;
    lib_strcpy(puc->UC_cName,  ppasswd->pw_name);                       /*  ������ܻ����ظ����, û����*/
    lib_strcpy(puc->UC_pcHome, ppasswd->pw_dir);
    
    if (pcName && stNSize) {
        lib_strlcpy(pcName, puc->UC_cName,  stNSize);
    }
    
    if (pcHome && stHSize) {
        lib_strlcpy(pcHome, puc->UC_pcHome, stHSize);
    }
    
    __TTINY_SHELL_LOCK();                                               /*  �������                    */
    _List_Line_Add_Ahead(&puc->UC_lineManage, 
                         &_G_plineUsrCache);                            /*  ���ʹ�õķ��ڱ�ͷ          */
    __TTINY_SHELL_UNLOCK();                                             /*  �ͷ���Դ                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellGetGrpName
** ��������: ���һ������
** �䡡��  : gid           �� id
**           pcName        ����
**           stSize        ��������С
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __tshellGetGrpName (gid_t  gid, PCHAR  pcName, size_t  stSize)
{
    PLW_LIST_LINE       plineTmp;
    __PTSHELL_GCACHE    pgc;
    struct group        group;
    struct group       *pgroup = LW_NULL;
    CHAR                cBuffer[MAX_FILENAME_LENGTH];

    __TTINY_SHELL_LOCK();                                               /*  �������                    */
    for (plineTmp  = _G_plineGrpCache;
         plineTmp != LW_NULL;
         plineTmp  = _list_line_get_next(plineTmp)) {
        
        pgc = _LIST_ENTRY(plineTmp, __TSHELL_GCACHE, GC_lineManage);
        if (pgc->GC_gid == gid) {
            if (pcName && stSize) {
                lib_strlcpy(pcName, pgc->GC_cName, stSize);
            }
            if (plineTmp != _G_plineGrpCache) {
                _List_Line_Del(&pgc->GC_lineManage, 
                               &_G_plineGrpCache);
                _List_Line_Add_Ahead(&pgc->GC_lineManage, 
                                     &_G_plineGrpCache);                /*  ���ʹ�õķ��ڱ�ͷ          */
            }
            __TTINY_SHELL_UNLOCK();                                     /*  �ͷ���Դ                    */
            return  (ERROR_NONE);
        }
    }
    __TTINY_SHELL_UNLOCK();                                             /*  �ͷ���Դ                    */
    
    getgrgid_r(gid, &group, cBuffer, sizeof(cBuffer), &pgroup);
    if (!pgroup) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    pgc = (__PTSHELL_GCACHE)__SHEAP_ALLOC(sizeof(__TSHELL_GCACHE) + 
                                          lib_strlen(pgroup->gr_name));
    if (!pgc) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
                                          
    pgc->GC_gid = gid;
    lib_strcpy(pgc->GC_cName, pgroup->gr_name);                         /*  ������ܻ����ظ����, û����*/
    
    if (pcName && stSize) {
        lib_strlcpy(pcName, pgc->GC_cName, stSize);
    }
    
    __TTINY_SHELL_LOCK();                                               /*  �������                    */
    _List_Line_Add_Ahead(&pgc->GC_lineManage, 
                         &_G_plineGrpCache);                            /*  ���ʹ�õķ��ڱ�ͷ          */
    __TTINY_SHELL_UNLOCK();                                             /*  �ͷ���Դ                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellFlushCache
** ��������: ɾ������ shell ������û�������Ϣ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __tshellFlushCache (VOID)
{
    __PTSHELL_UCACHE    puc;
    __PTSHELL_GCACHE    pgc;

    __TTINY_SHELL_LOCK();                                               /*  �������                    */
    while (_G_plineUsrCache) {
        puc = _LIST_ENTRY(_G_plineUsrCache, __TSHELL_UCACHE, UC_lineManage);
        _List_Line_Del(&puc->UC_lineManage, &_G_plineUsrCache);
        __SHEAP_FREE(puc->UC_pcHome);
        __SHEAP_FREE(puc);
    }
    while (_G_plineGrpCache) {
        pgc = _LIST_ENTRY(_G_plineGrpCache, __TSHELL_GCACHE, GC_lineManage);
        _List_Line_Del(&pgc->GC_lineManage, &_G_plineGrpCache);
        __SHEAP_FREE(pgc);
    }
    __TTINY_SHELL_UNLOCK();                                             /*  �ͷ���Դ                    */
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
