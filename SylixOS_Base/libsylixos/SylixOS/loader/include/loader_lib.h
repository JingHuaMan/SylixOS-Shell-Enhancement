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
** ��   ��   ��: loader_lib.h
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2010 �� 04 �� 17 ��
**
** ��        ��: ����ͷ�ļ�

** BUG:
2011.02.20  Ϊ���� posix ��̬���ӿ��׼, ����Է���ȫ������ֲ��ԵĴ���.
2012.09.21  ���� BSD ����, �����˶� issetugid �Ĺ���.
2012.10.25  ������� I/O ����.
2012.12.09  �������������.
2012.12.18  �������˽�� FD ��.
2014.05.13  �������߳�����.
2014.09.30  ������̶�ʱ��.
2015.09.01  ���� MIPS �ܹ�
*********************************************************************************************************/

#ifndef __LOADER_LIB_H
#define __LOADER_LIB_H

#include "../elf/elf_type.h"
#include "loader_vppatch.h"

/*********************************************************************************************************
  ģ������ʱռ�õ��ڴ�Σ�ж��ʱ���ͷ���Щ��
*********************************************************************************************************/

typedef struct {
    addr_t                  ESEG_ulAddr;                                /*  �ڴ�ε�ַ                  */
    size_t                  ESEG_stLen;                                 /*  �ڴ�γ���                  */
#if (LW_CFG_TRUSTED_COMPUTING_EN > 0) || defined(LW_CFG_CPU_ARCH_C6X)   /*  C6X ʹ��SEGMENTʵ��backtrace*/
    BOOL                    ESEG_bCanExec;                              /*  �Ƿ��ִ��                  */
#endif                                                                  /*  LW_CFG_TRUSTED_COMPUTING_EN */
} LW_LD_EXEC_SEGMENT;

/*********************************************************************************************************
  ģ����ű���
*********************************************************************************************************/

typedef struct {
    LW_LIST_LINE            ESYM_lineManage;                            /*  ��������                    */
    size_t                  ESYM_stSize;                                /*  �ֶλ����С                */
    size_t                  ESYM_stUsed;                                /*  ���ֶ��Ѿ�ʹ�õ��ڴ�����    */
} LW_LD_EXEC_SYMBOL;

#define LW_LD_EXEC_SYMBOL_HDR_SIZE  ROUND_UP(sizeof(LW_LD_EXEC_SYMBOL), sizeof(LW_STACK))

/*********************************************************************************************************
  ģ�����ͣ�KO\SO
*********************************************************************************************************/

#define LW_LD_MOD_TYPE_KO           0                                   /*  �ں�ģ��                    */
#define LW_LD_MOD_TYPE_SO           1                                   /*  Ӧ�ó����̬���ӿ�        */

/*********************************************************************************************************
  ģ������״̬
*********************************************************************************************************/

#define LW_LD_STATUS_UNLOAD         0                                   /*  δ����                      */
#define LW_LD_STATUS_LOADED         1                                   /*  �Ѽ���δ��ʼ��              */
#define LW_LD_STATUS_INITED         2                                   /*  �ѳ�ʼ��                    */
#define LW_LD_STATUS_FINIED         3                                   /*  ������                      */

/*********************************************************************************************************
  ģ��ṹ��������֯ģ�����Ϣ
*********************************************************************************************************/

#define __LW_LD_EXEC_MODULE_MAGIC   0x25ef68af

/*********************************************************************************************************
  MIPS_HI16_RELOC_INFO �ṩһ�ַ����� HI16 �ض�λ����Ϣ���ݸ� LO16 �ض�λ��
*********************************************************************************************************/
#ifdef LW_CFG_CPU_ARCH_MIPS

struct __MIPS_HI16_RELOC_INFO;
typedef struct __MIPS_HI16_RELOC_INFO  MIPS_HI16_RELOC_INFO, *PMIPS_HI16_RELOC_INFO;

struct __MIPS_HI16_RELOC_INFO {
    Elf_Addr               *HI16_pAddr;
    Elf_Addr                HI16_valAddr;
    PMIPS_HI16_RELOC_INFO   HI16_pNext;
};

#endif                                                                  /*  LW_CFG_CPU_ARCH_MIPS        */
/*********************************************************************************************************
  �ں�ģ�� atexit ����
*********************************************************************************************************/

typedef struct {
    LW_LIST_MONO            EMODAE_pmonoNext;
    VOIDFUNCPTR             EMODAE_pfunc;                               /*  atexit ���ú���             */
} LW_LD_EXEC_MODATEXIT;

/*********************************************************************************************************
  ģ��
*********************************************************************************************************/

typedef struct {
    ULONG                   EMOD_ulMagic;                               /*  ����ʶ�𱾽ṹ��            */
    ULONG                   EMOD_ulModType;                             /*  ģ�����ͣ�KO����SO�ļ�      */
    ULONG                   EMOD_ulStatus;                              /*  ģ�鵱ǰ����״̬            */
    ULONG                   EMOD_ulRefs;                                /*  ģ�����ü���                */
    PVOID                   EMOD_pvUsedArr;                             /*  ������ģ������              */
    ULONG                   EMOD_ulUsedCnt;                             /*  ����ģ�������С            */

    PCHAR                   EMOD_pcSymSection;                          /*  ������ָ�� section ���ű�   */

    VOIDFUNCPTR            *EMOD_ppfuncInitArray;                       /*  ��ʼ����������              */
    ULONG                   EMOD_ulInitArrCnt;                          /*  ��ʼ����������              */
    VOIDFUNCPTR            *EMOD_ppfuncFiniArray;                       /*  ������������                */
    ULONG                   EMOD_ulFiniArrCnt;                          /*  ������������                */

    PCHAR                   EMOD_pcInit;                                /*  ��ʼ����������              */
    FUNCPTR                 EMOD_pfuncInit;                             /*  ��ʼ������ָ��              */

    PCHAR                   EMOD_pcExit;                                /*  ������������                */
    FUNCPTR                 EMOD_pfuncExit;                             /*  ��������ָ��                */

    PCHAR                   EMOD_pcEntry;                               /*  ��ں�������                */
    FUNCPTR                 EMOD_pfuncEntry;                            /*  main����ָ��                */
    BOOL                    EMOD_bIsSymbolEntry;                        /*  �Ƿ�Ϊ�������              */

    size_t                  EMOD_stLen;                                 /*  Ϊģ�������ڴ泤��        */
    PVOID                   EMOD_pvBaseAddr;                            /*  Ϊģ�������ڴ��ַ        */

    BOOL                    EMOD_bIsGlobal;                             /*  �Ƿ�Ϊȫ�ַ���              */
    ULONG                   EMOD_ulSymCount;                            /*  ����������Ŀ                */
    ULONG                   EMOD_ulSymHashSize;                         /*  ���� hash ���С            */
    LW_LIST_LINE_HEADER    *EMOD_psymbolHash;                           /*  ���������� hash ��          */
    LW_LIST_LINE_HEADER     EMOD_plineSymbolBuffer;                     /*  �������ű���              */

    ULONG                   EMOD_ulSegCount;                            /*  �ڴ����Ŀ                  */
    LW_LD_EXEC_SEGMENT     *EMOD_psegmentArry;                          /*  �ڴ���б�                  */
    BOOL                    EMOD_bExportSym;                            /*  �Ƿ񵼳�����                */
    PCHAR                   EMOD_pcModulePath;                          /*  ģ���ļ�·��                */

    LW_LD_VPROC            *EMOD_pvproc;                                /*  ��������                    */
    LW_LIST_RING            EMOD_ringModules;                           /*  ���������еĿ�����          */
    PVOID                   EMOD_pvFormatInfo;                          /*  �ض�λ�����Ϣ              */

    BOOL                    EMOD_bKoUnloadDisallow;                     /*  ������ж���ں�ģ��          */
    LW_LIST_MONO_HEADER     EMOD_pmonoAtexit;                           /*  �ں�ģ�� atexit             */

    dev_t                   EMOD_dev;                                   /*  ģ���ļ��豸��ʶ            */
    ino_t                   EMOD_ino;                                   /*  ģ���ļ� inode ��ʶ         */

#if LW_CFG_MODULELOADER_TEXT_RO_EN > 0
    size_t                  EMOD_stCodeLen;                             /*  ģ�����γ���              */
    ULONG                   EMOD_ulCodeOft;                             /*  ģ������ƫ��              */
#endif                                                                  /*  LW_CFG_MODULELOADER_TEXT... */

#ifdef LW_CFG_CPU_ARCH_ARM
    size_t                  EMOD_stARMExidxCount;                       /*  ARM.exidx �γ���            */
    PVOID                   EMOD_pvARMExidx;                            /*  ARM.exidx ���ڴ��ַ        */
#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM         */

#ifdef LW_CFG_CPU_ARCH_MIPS
    MIPS_HI16_RELOC_INFO   *EMOD_pMIPSHi16List;
#endif                                                                  /*  LW_CFG_CPU_ARCH_MIPS        */

#ifdef LW_CFG_CPU_ARCH_C6X
    ULONG                  *EMOD_pulDsbtTable;                          /*  DSP DSBT ��λ��             */
    ULONG                   EMOD_ulDsbtSize;                            /*  DSP DSBT ���С             */
    ULONG                   EMOD_ulDsbtIndex;                           /*  DSP DSBT ������             */
#endif                                                                  /*  LW_CFG_CPU_ARCH_C6X         */

#ifdef LW_CFG_CPU_ARCH_RISCV
    addr_t                  EMOD_ulRiscvHi20Base;                       /*  HI20 �ض�λ��Ŀ��ַ         */
    ULONG                   EMOD_ulRiscvHi20Nr;                         /*  HI20 �ض�λ��Ŀ��Ŀ         */
    addr_t                  EMOD_ulRiscvGotBase;                        /*  �ں�ģ�� GOT ��ַ           */ 
    ULONG                   EMOD_ulRiscvGotNr;                          /*  �ں�ģ�� GOT ��Ŀ           */ 
#endif                                                                  /*  LW_CFG_CPU_ARCH_RISCV       */

#ifdef LW_CFG_CPU_ARCH_CSKY
    PVOID                   EMOD_pvCSkyDynamicAddr;                     /*  .dynamic �ε��ڴ��ַ       */
#endif                                                                  /*  LW_CFG_CPU_ARCH_CSKY        */
} LW_LD_EXEC_MODULE;

/*********************************************************************************************************
  �����������Ŀ
*********************************************************************************************************/
#define __LW_MAX_NEEDED_LIB     64
/*********************************************************************************************************
   �������dynamic�����ݽṹ
*********************************************************************************************************/

typedef struct {
    Elf_Sym     *psymTable;                                             /*  ���ű�ָ��                  */
    ULONG        ulSymCount;                                            /*  ������Ŀ                    */
    PCHAR        pcStrTable;                                            /*  �ַ�����ָ��                */
    Elf_Rel     *prelTable;                                             /*  rel�ض�λ��ָ��             */
    ULONG        ulRelSize;                                             /*  �ض�λ���С                */
    ULONG        ulRelCount;                                            /*  �ض�λ������Ŀ              */
    Elf_Rela    *prelaTable;                                            /*  rel�ض�λ��ָ��             */
    ULONG        ulRelaSize;                                            /*  �ض�λ���С                */
    ULONG        ulRelaCount;                                           /*  �ض�λ������Ŀ              */
    Elf_Hash    *phash;                                                 /*  hash��ָ��                  */
    PCHAR        pvJmpRTable;                                           /*  plt�ض�λ��ָ��             */
    ULONG        ulPltRel;                                              /*  plt�ض�λ��������           */
    ULONG        ulJmpRSize;                                            /*  plt�ض�λ���С             */
    Elf_Addr    *paddrInitArray;                                        /*  ��ʼ����������              */
    Elf_Addr    *paddrFiniArray;                                        /*  ������������                */
    ULONG        ulInitArrSize;                                         /*  ��ʼ�����������С          */
    ULONG        ulFiniArrSize;                                         /*  �������������С            */
    Elf_Addr     addrInit;                                              /*  ��ʼ������                  */
    Elf_Addr     addrFini;                                              /*  ��������                    */
    Elf_Addr     addrMin;                                               /*  ��С�����ַ                */
    Elf_Addr     addrMax;                                               /*  ��������ַ                */
    Elf_Word     wdNeededArr[__LW_MAX_NEEDED_LIB];
    ULONG        ulNeededCnt;
    Elf_Addr    *ulPltGotAddr;                                          /*  ȫ��GOT�ĵ�ַ               */

#ifdef LW_CFG_CPU_ARCH_MIPS
    ULONG        ulMIPSGotSymIdx;                                       /*  Dynsym ��һ��GOT���        */
    ULONG        ulMIPSLocalGotNumIdx;                                  /*  MIPS Local GOT �������     */
    ULONG        ulMIPSSymNumIdx;                                       /*  MIPS Dynsym �������        */
    ULONG        ulMIPSPltGotIdx;                                       /*  MIPS .got.plt ��ַ          */
#endif                                                                  /*  LW_CFG_CPU_ARCH_MIPS        */
} ELF_DYN_DIR;

/*********************************************************************************************************
  module ������
*********************************************************************************************************/

extern LW_OBJECT_HANDLE     _G_ulVProcMutex;

#define LW_LD_LOCK()        API_SemaphoreMPend(_G_ulVProcMutex, LW_OPTION_WAIT_INFINITE)
#define LW_LD_UNLOCK()      API_SemaphoreMPost(_G_ulVProcMutex)

/*********************************************************************************************************
  ������Ϣ��ӡ
*********************************************************************************************************/

#ifdef  __SYLIXOS_DEBUG
#define LD_DEBUG_MSG(msg)   printf msg
#else
#define LD_DEBUG_MSG(msg)
#endif                                                                  /*  __LOAD_DEBUG                */

/*********************************************************************************************************
  ����Ĭ�������Ӧ�����
*********************************************************************************************************/

#define LW_LD_DEFAULT_ENTRY         "_start"                            /*  ���̳�ʼ����ڷ���          */
#define LW_LD_PROCESS_ENTRY         "main"                              /*  ��������ڷ���              */

/*********************************************************************************************************
  С�����ڴ����
*********************************************************************************************************/

#define LW_LD_SAFEMALLOC(size)      __SHEAP_ALLOC((size_t)size)
#define LW_LD_SAFEFREE(a)           { if (a) { __SHEAP_FREE((PVOID)a); a = 0; } }
                                                                        /*  ��ȫ�ͷ�                    */

/*********************************************************************************************************
  �ں�ģ��������ڴ����
*********************************************************************************************************/

PVOID __ldMalloc(size_t  stLen);                                        /*  �����ڴ�                    */
PVOID __ldMallocAlign(size_t  stLen, size_t  stAlign);
VOID  __ldFree(PVOID  pvAddr);                                          /*  �ͷ��ڴ�                    */

#define LW_LD_VMSAFEMALLOC(size)    \
        __ldMalloc(size)
        
#define LW_LD_VMSAFEMALLOC_ALIGN(size, align)   \
        __ldMallocAlign(size, align)
        
#define LW_LD_VMSAFEFREE(a) \
        { if (a) { __ldFree((PVOID)a); a = 0; } }

/*********************************************************************************************************
  �����������ڴ����
*********************************************************************************************************/

extern LW_OBJECT_HANDLE             _G_ulExecShareLock;                 /*  ������ڴ����              */

PVOID  __ldMallocArea(size_t  stLen);
PVOID  __ldMallocAreaAlign(size_t  stLen, size_t  stAlign);
VOID   __ldFreeArea(PVOID  pvAddr);

#define LW_LD_VMSAFEMALLOC_AREA(size)   \
        __ldMallocArea(size)
        
#define LW_LD_VMSAFEMALLOC_AREA_ALIGN(size, align)  \
        __ldMallocAreaAlign(size, align)
        
#define LW_LD_VMSAFEFREE_AREA(a)    \
        { if (a) { __ldFreeArea((PVOID)a); a = 0; } }
        
INT     __ldMmap(PVOID  pvBase, size_t  stAddrOft, INT  iFd, struct stat64 *pstat64,
                 off_t  oftOffset, size_t  stLen,  BOOL  bCanShare, BOOL  bCanExec);
VOID    __ldShare(PVOID  pvBase, size_t  stLen, dev_t  dev, ino64_t ino64);
VOID    __ldShareAbort(dev_t  dev, ino64_t  ino64);
INT     __ldShareConfig(BOOL  bShareEn, BOOL  *pbPrev);
VOID    __ldProtect(PVOID  pvBase, size_t  stAddrOft, size_t  stLen);

#define LW_LD_VMSAFEMAP_AREA(base, addr_offset, fd, pstat64, file_offset, len, can_share, can_exec) \
        __ldMmap(base, addr_offset, fd, pstat64, file_offset, len, can_share, can_exec)
        
#define LW_LD_VMSAFE_PROTECT(base, addr_offset, len) \
        __ldProtect(base, addr_offset, len)

#define LW_LD_VMSAFE_SHARE(base, len, dev, ino64) \
        __ldShare(base, len, dev, ino64)
        
#define LW_LD_VMSAFE_SHARE_ABORT(dev, ino64)    \
        __ldShareAbort(dev, ino64)
        
#define LW_LD_VMSAFE_SHARE_CONFIG(can_share, prev)  \
        __ldShareConfig(can_share, prev)

/*********************************************************************************************************
  ��ַת��
*********************************************************************************************************/

#define LW_LD_V2PADDR(vBase, pBase, vAddr)      \
        ((size_t)pBase + (size_t)vAddr - (size_t)vBase)                 /*  ���������ַ                */ 

#define LW_LD_P2VADDR(vBase, pBase, pAddr)      \
        ((size_t)vBase + (size_t)pAddr - (size_t)pBase)                 /*  ���������ַ                */
        
/*********************************************************************************************************
  vp ��������
*********************************************************************************************************/

#define LW_LD_VMEM_MAX      64

PCHAR __moduleVpPatchVersion(LW_LD_EXEC_MODULE *pmodule);               /*  ��ò����汾                */

#if LW_CFG_VMM_EN == 0
PVOID __moduleVpPatchHeap(LW_LD_EXEC_MODULE *pmodule);                  /*  ��ò��������ڴ��          */
#endif                                                                  /*  LW_CFG_VMM_EN == 0          */

INT   __moduleVpPatchVmem(LW_LD_EXEC_MODULE *pmodule, PVOID  ppvArea[], INT  iSize);
                                                                        /*  ��ý��������ڴ�ռ�        */
VOID  __moduleVpPatchInit(LW_LD_EXEC_MODULE *pmodule);                  /*  ���̲�������������          */
VOID  __moduleVpPatchFini(LW_LD_EXEC_MODULE *pmodule);

/*********************************************************************************************************
  ��ϵ�ṹ���к���
*********************************************************************************************************/

#ifdef LW_CFG_CPU_ARCH_ARM
typedef long unsigned int   *_Unwind_Ptr;
_Unwind_Ptr dl_unwind_find_exidx(_Unwind_Ptr pc, int *pcount, PVOID *pvVProc);
#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM         */

#endif                                                                  /*  __LOADER_LIB_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
