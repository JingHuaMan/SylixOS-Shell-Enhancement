#!/usr/bin/env bash

srcfile=libsylixos.a
symbolc=symbol.c
symbolh=symbol.h
symbolld=symbol.ld
NM=nm
funcfile=func.lst
objsfile=objs.lst

echo "create $symbolc from: $srcfile"

rm -f $funcfile $objfile 1>/dev/null 2>&1

for i in $srcfile; do 
    # function
    $NM $i | sed -n 's/.*\ [TW]\ \(.*\)/\1/gp' >>$funcfile;
    # obj, remove __sylixos_version
    $NM $i | sed -n 's/.*\ [BDRSCVG]\ \(.*\)/\1/gp' | sed '/__sylixos_version/d' >>$objsfile;
done

cat << EOF >$symbolld
-Wl,--no-undefined
-Wl,--no-allow-shlib-undefined
-Wl,--error-unresolved-symbols
-Wl,--unresolved-symbols=report-all
-Wl,--ignore-unresolved-symbol,bspDebugMsg
-Wl,--ignore-unresolved-symbol,udelay
-Wl,--ignore-unresolved-symbol,ndelay
-Wl,--ignore-unresolved-symbol,bspDelayUs
-Wl,--ignore-unresolved-symbol,bspDelayNs
-Wl,--ignore-unresolved-symbol,bspInfoCpu
-Wl,--ignore-unresolved-symbol,bspInfoCache
-Wl,--ignore-unresolved-symbol,bspInfoPacket
-Wl,--ignore-unresolved-symbol,bspInfoVersion
-Wl,--ignore-unresolved-symbol,bspInfoHwcap
-Wl,--ignore-unresolved-symbol,bspInfoRomBase
-Wl,--ignore-unresolved-symbol,bspInfoRomSize
-Wl,--ignore-unresolved-symbol,bspInfoRamBase
-Wl,--ignore-unresolved-symbol,bspInfoRamSize
-Wl,--ignore-unresolved-symbol,bspTickHighResolution
-Wl,--ignore-unresolved-symbol,null
-Wl,--ignore-unresolved-symbol,__lib_strerror
-Wl,--ignore-unresolved-symbol,_execl
-Wl,--ignore-unresolved-symbol,_execle
-Wl,--ignore-unresolved-symbol,_execlp
-Wl,--ignore-unresolved-symbol,_execv
-Wl,--ignore-unresolved-symbol,_execve
-Wl,--ignore-unresolved-symbol,_execvp
-Wl,--ignore-unresolved-symbol,_execvpe
-Wl,--ignore-unresolved-symbol,_spawnl
-Wl,--ignore-unresolved-symbol,_spawnle
-Wl,--ignore-unresolved-symbol,_spawnlp
-Wl,--ignore-unresolved-symbol,_spawnv
-Wl,--ignore-unresolved-symbol,_spawnve
-Wl,--ignore-unresolved-symbol,_spawnvp
-Wl,--ignore-unresolved-symbol,_spawnvpe
-Wl,--ignore-unresolved-symbol,_longjmp
-Wl,--ignore-unresolved-symbol,_setjmp
-Wl,--ignore-unresolved-symbol,__aeabi_read_tp
-Wl,--ignore-unresolved-symbol,msgget
-Wl,--ignore-unresolved-symbol,msgctl
-Wl,--ignore-unresolved-symbol,msgrcv
-Wl,--ignore-unresolved-symbol,msgsnd
-Wl,--ignore-unresolved-symbol,semget
-Wl,--ignore-unresolved-symbol,semctl
-Wl,--ignore-unresolved-symbol,semop
-Wl,--ignore-unresolved-symbol,semtimedop
-Wl,--ignore-unresolved-symbol,shmget
-Wl,--ignore-unresolved-symbol,shmctl
-Wl,--ignore-unresolved-symbol,shmat
-Wl,--ignore-unresolved-symbol,shmdt
-Wl,--ignore-unresolved-symbol,ftok
-Wl,--ignore-unresolved-symbol,_mpool_close
-Wl,--ignore-unresolved-symbol,_mpool_filter
-Wl,--ignore-unresolved-symbol,_strvisx
-Wl,--ignore-unresolved-symbol,_getservbyport
-Wl,--ignore-unresolved-symbol,_regfree
-Wl,--ignore-unresolved-symbol,_freeaddrinfo
-Wl,--ignore-unresolved-symbol,_getprotoent_r
-Wl,--ignore-unresolved-symbol,_wcstof
-Wl,--ignore-unresolved-symbol,_uuid_create_nil
-Wl,--ignore-unresolved-symbol,_dirname
-Wl,--ignore-unresolved-symbol,_res_mkquery
-Wl,--ignore-unresolved-symbol,_vis
-Wl,--ignore-unresolved-symbol,_regcomp
-Wl,--ignore-unresolved-symbol,_endservent
-Wl,--ignore-unresolved-symbol,_res_search
-Wl,--ignore-unresolved-symbol,_strsvisx
-Wl,--ignore-unresolved-symbol,_basename
-Wl,--ignore-unresolved-symbol,_uuid_is_nil
-Wl,--ignore-unresolved-symbol,_wcstod
-Wl,--ignore-unresolved-symbol,_getservbyname_r
-Wl,--ignore-unresolved-symbol,_getprotobynumber
-Wl,--ignore-unresolved-symbol,_endprotoent_r
-Wl,--ignore-unresolved-symbol,_regexec
-Wl,--ignore-unresolved-symbol,_wcstold
-Wl,--ignore-unresolved-symbol,_dn_expand
-Wl,--ignore-unresolved-symbol,_getservbyname
-Wl,--ignore-unresolved-symbol,_getnameinfo
-Wl,--ignore-unresolved-symbol,_mpool_get
-Wl,--ignore-unresolved-symbol,_gai_strerror
-Wl,--ignore-unresolved-symbol,_setprotoent
-Wl,--ignore-unresolved-symbol,_mpool_new
-Wl,--ignore-unresolved-symbol,_mpool_put
-Wl,--ignore-unresolved-symbol,_herror
-Wl,--ignore-unresolved-symbol,_res_query
-Wl,--ignore-unresolved-symbol,_strsvis
-Wl,--ignore-unresolved-symbol,_getprotoent
-Wl,--ignore-unresolved-symbol,_strvis
-Wl,--ignore-unresolved-symbol,_getprotobyname
-Wl,--ignore-unresolved-symbol,_iconv
-Wl,--ignore-unresolved-symbol,_setprotoent_r
-Wl,--ignore-unresolved-symbol,_getprotobyname_r
-Wl,--ignore-unresolved-symbol,_setservent
-Wl,--ignore-unresolved-symbol,_getservent
-Wl,--ignore-unresolved-symbol,_res_init
-Wl,--ignore-unresolved-symbol,_mpool_sync
-Wl,--ignore-unresolved-symbol,_getaddrinfo
-Wl,--ignore-unresolved-symbol,_getprotobynumber_r
-Wl,--ignore-unresolved-symbol,_nsdispatch
-Wl,--ignore-unresolved-symbol,_svis
-Wl,--ignore-unresolved-symbol,_iconv_open
-Wl,--ignore-unresolved-symbol,_endprotoent
-Wl,--ignore-unresolved-symbol,_iconv_close
-Wl,--ignore-unresolved-symbol,_wcwidth
-Wl,--ignore-unresolved-symbol,_getservbyport_r
-Wl,--ignore-unresolved-symbol,_mpool_open
-Wl,--ignore-unresolved-symbol,_regerror
-Wl,--ignore-unresolved-symbol,_dbopen
EOF

sed 's/\(.*\)/-Wl,--ignore-unresolved-symbol,\1\ /g' $funcfile >>$symbolld
sed 's/\(.*\)/-Wl,--ignore-unresolved-symbol,\1\ /g' $objsfile >>$symbolld

cat << EOF >$symbolc
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
 ** 文   件   名: `basename $symbolc`                     
 **                                                        
 ** 创   建   人: makesymbol.sh 工具                                        
 **                                                        
 ** 文件创建日期: `date`
 **                                                        
 ** 描        述: 系统 sylixos 符号表. (此文件由 makesymbol.sh 工具自动生成, 请勿修改)                
********************************************************************************************************/
#ifdef __GNUC__
#if __GNUC__ <= 4
#pragma GCC diagnostic warning "-w"
#elif __GNUC__ >= 7
#pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"
#endif
#endif

#include "symboltools.h"

#define SYMBOL_TABLE_BEGIN LW_STATIC_SYMBOL   _G_symLibSylixOS[] = {
                                                         
#define SYMBOL_TABLE_END };                                        
                                                       
#define SYMBOL_ITEM_FUNC(pcName)                \                                
    {   {(void *)0, (void *)0},                 \                                
        #pcName, (char *)pcName,                \                                
        LW_SYMBOL_TEXT                          \                                
    },                                                    
                                                        
#define SYMBOL_ITEM_OBJ(pcName)                 \                                
    {   {(void *)0, (void *)0},                 \                                
        #pcName, (char *)&pcName,               \                                
        LW_SYMBOL_DATA                          \                                
    },                                                    
                                                        
/*********************************************************************************************************
  全局对象声明                                                
*********************************************************************************************************/
#ifdef SYLIXOS_EXPORT_KSYMBOL
EOF

# 声明
# extern int xxxx();
sed 's/\(.*\)/extern int \1\(\)\;/g' $funcfile >>$symbolc

cat << EOF >>$symbolc

/* objs */
EOF

# obj
# extern int xxxx;
sed 's/\(.*\)/extern int \1\;/g' $objsfile >>$symbolc


cat << EOF >>$symbolc

/*********************************************************************************************************
  系统静态符号表                                                
*********************************************************************************************************/
SYMBOL_TABLE_BEGIN                                                
EOF

sed 's/\(.*\)/    SYMBOL_ITEM_FUNC\(\1\)/g' $funcfile   >>$symbolc;
sed 's/\(.*\)/    SYMBOL_ITEM_OBJ\(\1\)/g' $objsfile   >>$symbolc;
cat $funcfile >tempfilesymbol.txt
cat $objsfile >>tempfilesymbol.txt

cat << EOF >>$symbolc
SYMBOL_TABLE_END                                                
#endif
/*********************************************************************************************************
  END                                                    
*********************************************************************************************************/
EOF

cat << EOF > $symbolh
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
 ** 文   件   名: `basename $symbolh`
 **                                                        
 ** 创   建   人: makesymbol.sh 工具                                        
 **                                                        
 ** 文件创建日期: `date`
 **                                                        
 ** 描        述: 系统 sylixos 符号表. (此文件由 makesymbol.sh 工具自动生成, 请勿修改)                
*********************************************************************************************************/
                                                    
#ifndef __SYMBOL_H                                                
#define __SYMBOL_H                                                
                                                        
#include "SylixOS.h"                                            
#include "symboltools.h"
                                                        
#ifdef SYLIXOS_EXPORT_KSYMBOL
#define SYM_TABLE_SIZE      `cat tempfilesymbol.txt|wc -l`
extern  LW_STATIC_SYMBOL     _G_symLibSylixOS[SYM_TABLE_SIZE];
                                                        
static LW_INLINE  INT symbolAddAll (VOID)
{                                                        
    return  (symbolAddStatic((LW_SYMBOL *)_G_symLibSylixOS, SYM_TABLE_SIZE));                
}                                                        
#else
static LW_INLINE  INT symbolAddAll (VOID)
{
    return  (ERROR_NONE);                
}
#endif
                                         
#endif                                                                  /*  __SYMBOL_H                  */
/********************************************************************************************************* 
  END 
*********************************************************************************************************/
EOF

rm -f $funcfile $objsfile tempfilesymbol.txt

exit 0
