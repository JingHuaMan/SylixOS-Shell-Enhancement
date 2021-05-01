/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: symLibc.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 03 �� 04 ��
**
** ��        ��: gcc ���������ܻ��Զ����ɵ��� memcmp, memset, memcpy and memmove �ĳ���, ����, ��Щ����
                 ��������� sylixos ���ű���, ����װ��������װ���������, 
                 
                 ����: char  a[] = "abcd"; gcc ���붯̬���, ���Զ����� memcpy �����г�ʼ��.
                 ����ķ��ű�֤���ڶ�̬����ʱ, �������һЩ���ŵ�δ�������.
                 
                 �ɲο�: http://gcc.gnu.org/onlinedocs/gcc/Link-Options.html
                 
** BUG:
2011.05.16  ���� setjmp / longjmp �ȳ��ú���.
2012.02.03  �����������.
2012.10.23  sylixos ��ʼʹ�������ṩ�� setjmp �� longjmp ����, ���ﲻ��Ҫ��������
2012.12.18  rc36 ֮��, Ϊ�˼�����, lwip_* ��ͷ�ķ���ʹ�� sylixos socket �����ķ��� .
*********************************************************************************************************/
#define  __SYLIXOS_INTTYPES
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "setjmp.h"
#include "process.h"
#if LW_CFG_NET_EN > 0
#include "socket.h"
#include "netdb.h"
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_SYMBOL_EN > 0
static LW_SYMBOL    _G_symLibc[] = {
/*********************************************************************************************************
  string
*********************************************************************************************************/
    {   {LW_NULL, LW_NULL}, "__lib_strerror", (caddr_t)lib_strerror,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
/*********************************************************************************************************
  net (Ϊ�˼�������ֱ��ʹ�� lwip socket api ��Ӧ�ó���)
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
    {   {LW_NULL, LW_NULL}, "lwip_socket", (caddr_t)socket,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_accept", (caddr_t)accept,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_bind", (caddr_t)bind,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_shutdown", (caddr_t)shutdown,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_connect", (caddr_t)connect,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_getsockname", (caddr_t)getsockname,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_getpeername", (caddr_t)getpeername,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_setsockopt", (caddr_t)setsockopt,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_getsockopt", (caddr_t)getsockopt,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_listen", (caddr_t)listen,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_recv", (caddr_t)recv,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_recvfrom", (caddr_t)recvfrom,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_send", (caddr_t)send,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_sendto", (caddr_t)sendto,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_ioctl", (caddr_t)ioctl,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_gethostbyname", (caddr_t)gethostbyname,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_gethostbyname_r", (caddr_t)gethostbyname_r,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_freeaddrinfo", (caddr_t)freeaddrinfo,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "lwip_getaddrinfo", (caddr_t)getaddrinfo,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
/*********************************************************************************************************
  net ��ַת������
*********************************************************************************************************/
    {   {LW_NULL, LW_NULL}, "ipaddr_aton", (caddr_t)ip4addr_aton,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "ipaddr_ntoa", (caddr_t)ip4addr_ntoa,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "ipaddr_ntoa_r", (caddr_t)ip4addr_ntoa_r,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
/*********************************************************************************************************
  process
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
    {   {LW_NULL, LW_NULL}, "_execl", (caddr_t)execl,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_execle", (caddr_t)execle,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_execlp", (caddr_t)execlp,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_execv", (caddr_t)execv,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_execve", (caddr_t)execve,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_execvp", (caddr_t)execvp,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_execvpe", (caddr_t)execvpe,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    
    {   {LW_NULL, LW_NULL}, "_spawnl", (caddr_t)spawnl,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_spawnle", (caddr_t)spawnle,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_spawnlp", (caddr_t)spawnlp,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_spawnv", (caddr_t)spawnv,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_spawnve", (caddr_t)spawnve,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_spawnvp", (caddr_t)spawnvp,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_spawnvpe", (caddr_t)spawnvpe,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  longjmp setjmp
*********************************************************************************************************/
    {   {LW_NULL, LW_NULL}, "_longjmp", (caddr_t)longjmp,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
    {   {LW_NULL, LW_NULL}, "_setjmp", (caddr_t)setjmp,
    	 LW_SYMBOL_FLAG_STATIC | LW_SYMBOL_FLAG_REN | LW_SYMBOL_FLAG_XEN
    },
};
/*********************************************************************************************************
** ��������: __symbolAddLibc
** ��������: ����ű������ libc ���� 
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __symbolAddLibc (VOID)
{
    return  (API_SymbolAddStatic(_G_symLibc, (sizeof(_G_symLibc) / sizeof(LW_SYMBOL))));
}

#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
