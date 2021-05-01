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
** ��   ��   ��: lwip_natlib.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 03 �� 19 ��
**
** ��        ��: lwip NAT ֧�ְ��ڲ���.
*********************************************************************************************************/

#ifndef __LWIP_NATLIB_H
#define __LWIP_NATLIB_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_ROUTER > 0) && (LW_CFG_NET_NAT_EN > 0)
/*********************************************************************************************************
  NAT ���ƿ�
*********************************************************************************************************/
typedef struct {
    union {
        LW_LIST_MONO    NATL_monoFree;                                  /*  ������                      */
        LW_LIST_LINE    NATL_lineManage;                                /*  ���ƿ����� (Դ�˿� hash)    */
    } l;
#define NAT_monoFree    l.NATL_monoFree
#define NAT_lineManage  l.NATL_lineManage

    LW_LIST_LINE        NAT_lineAssHash;                                /*  AssPort hash                */
    u8_t                NAT_ucInAssHash;                                /*  �Ƿ��� AssPort hash         */
    u8_t                NAT_ucProto;                                    /*  Э��                        */
    u16_t               NAT_usMapHash;                                  /*  ���ؾ���Դ��ַ hash         */
    ip4_addr_t          NAT_ipaddrLocal;                                /*  ���� IP ��ַ                */
    u16_t               NAT_usLocalPort;                                /*  ���ض˿ں�                  */
    u16_t               NAT_usAssPort;                                  /*  ӳ��˿ں� (Ψһ��)         */
    u16_t               NAT_usAssPortSave;                              /*  MAP ӳ��Ķ˿���Ҫ����      */
    
    u32_t               NAT_uiLocalSequence;                            /*  TCP ���к�                  */
    u32_t               NAT_uiLocalRcvNext;                             /*  TCP ��������                */

    ULONG               NAT_ulIdleTimer;                                /*  ���ж�ʱ��                  */
    INT                 NAT_iStatus;                                    /*  ͨ��״̬                    */
#define __NAT_STATUS_OPEN           0
#define __NAT_STATUS_SYN            1
#define __NAT_STATUS_FIN            2
#define __NAT_STATUS_CLOSING        3
} __NAT_CB;
typedef __NAT_CB       *__PNAT_CB;
/*********************************************************************************************************
  NAT ������������ӳ���ϵ
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE        NATM_lineManage;
    
    u8_t                NATM_ucProto;                                   /*  Э��                        */
    ip4_addr_t          NATM_ipaddrLocalIp;                             /*  ���� IP ��ַ                */
    u16_t               NATM_usLocalCnt;                                /*  ���� IP �θ��� (���ؾ���)   */
    u16_t               NATM_usLocalPort;                               /*  ���ض˿ں�                  */
    u16_t               NATM_usAssPort;                                 /*  ӳ��˿ں� (Ψһ��)         */
} __NAT_MAP;
typedef __NAT_MAP      *__PNAT_MAP;
/*********************************************************************************************************
  NAT ������
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE        NATA_lineManage;
    
    ip4_addr_t          NATA_ipaddrAliasIp;                             /*  ������ַ                    */
    ip4_addr_t          NATA_ipaddrSLocalIp;                            /*  ������Ӧ���� IP ��Χ        */
    ip4_addr_t          NATA_ipaddrELocalIp;
} __NAT_ALIAS;
typedef __NAT_ALIAS    *__PNAT_ALIAS;

/*********************************************************************************************************
  �����¼��ص�
*********************************************************************************************************/
VOID  nat_netif_add_hook(struct netif *pnetif);
VOID  nat_netif_remove_hook(struct netif *pnetif);

/*********************************************************************************************************
  ��������
*********************************************************************************************************/
VOID        __natInit(VOID);
INT         __natStart(CPCHAR  pcLocal, CPCHAR  pcAp);
INT         __natStop(VOID);
INT         __natIpFragSet(UINT8  ucProto, BOOL  bOn);
INT         __natIpFragGet(UINT8  ucProto, BOOL  *pbOn);
INT         __natAddLocal(CPCHAR  pcLocal);
INT         __natDeleteLocal(CPCHAR  pcLocal);
INT         __natAddAp(CPCHAR  pcAp);
INT         __natDeleteAp(CPCHAR  pcAp);
INT         __natMapAdd(ip4_addr_t  *pipaddr, u16_t  usIpCnt, u16_t  usPort, u16_t  AssPort, u8_t  ucProto);
INT         __natMapDelete(ip4_addr_t  *pipaddr, u16_t  usPort, u16_t  AssPort, u8_t  ucProto);
INT         __natAliasAdd(const ip4_addr_t  *pipaddrAlias, 
                          const ip4_addr_t  *ipaddrSLocalIp,
                          const ip4_addr_t  *ipaddrELocalIp);
INT         __natAliasDelete(const ip4_addr_t  *pipaddrAlias);

#if LW_CFG_PROCFS_EN > 0
VOID        __procFsNatInit(VOID);
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_ROUTER > 0       */
                                                                        /*  LW_CFG_NET_NAT_EN > 0       */
#endif                                                                  /*  __LWIP_NATLIB_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
