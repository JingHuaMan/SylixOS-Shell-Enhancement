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
** ��   ��   ��: gmemDev.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 03 �� 28 ��
**
** ��        ��: ͼ�λ��豸ģ��. (����ͼ�β�����Ҫ�㹻���ٶ�, ����ͼ���豸����������Խ IO ��)

** BUG:
2011.11.17  varinfo �м���ɫ��λ����Ϣ.
2014.05.20  �������� varinfo.
*********************************************************************************************************/

#ifndef __GMEMDEV_H
#define __GMEMDEV_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_GRAPH_EN > 0

/*********************************************************************************************************
  λ��Ϣ
*********************************************************************************************************/

typedef struct {
    UINT32             GMBF_uiOffset;                                   /* beginning of bitfield        */
    UINT32             GMBF_uiLength;                                   /* length of bitfield           */
    UINT32             GMBF_uiMsbRight;                                 /* !=0:Most significant bit is  */
                                                                        /* right                        */
} LW_GM_BITFIELD;

/*********************************************************************************************************
  ��ʾ��Ϣ
*********************************************************************************************************/

typedef struct {
    ULONG               GMVI_ulXRes;                                    /*  ��������                    */
    ULONG               GMVI_ulYRes;
    
    ULONG               GMVI_ulXResVirtual;                             /*  ��������                    */
    ULONG               GMVI_ulYResVirtual;
    
    ULONG               GMVI_ulXOffset;                                 /*  ��ʾ����ƫ��                */
    ULONG               GMVI_ulYOffset;

    ULONG               GMVI_ulBitsPerPixel;                            /*  ÿ�����ص�����λ��          */
    ULONG               GMVI_ulBytesPerPixel;                           /*  ÿ�����صĴ洢�ֽ���        */
                                                                        /*  ��Щͼ�δ����� DMA Ϊ�˶��� */
                                                                        /*  ʹ�������Ч�ֽ�          */
    ULONG               GMVI_ulGrayscale;                               /*  �Ҷȵȼ�                    */
    
    ULONG               GMVI_ulRedMask;                                 /*  ��ɫ����                    */
    ULONG               GMVI_ulGreenMask;                               /*  ��ɫ����                    */
    ULONG               GMVI_ulBlueMask;                                /*  ��ɫ����                    */
    ULONG               GMVI_ulTransMask;                               /*  ͸��������                  */
    
/*********************************************************************************************************
  0.9.9 227 ���ڼ���
*********************************************************************************************************/
    LW_GM_BITFIELD      GMVI_gmbfRed;                                   /* bitfield in gmem (true color)*/
    LW_GM_BITFIELD      GMVI_gmbfGreen;
    LW_GM_BITFIELD      GMVI_gmbfBlue;
    LW_GM_BITFIELD      GMVI_gmbfTrans;
    
    BOOL                GMVI_bHardwareAccelerate;                       /*  �Ƿ�ʹ��Ӳ������            */
    ULONG               GMVI_ulMode;                                    /*  ��ʾģʽ                    */
    ULONG               GMVI_ulStatus;                                  /*  ��ʾ��״̬                  */
} LW_GM_VARINFO;
typedef LW_GM_VARINFO  *PLW_GM_VARINFO;

typedef struct {
    PCHAR               GMSI_pcName;                                    /*  ��ʾ������                  */
    ULONG               GMSI_ulId;                                      /*  ID                          */
    size_t              GMSI_stMemSize;                                 /*  framebuffer �ڴ��С        */
    size_t              GMSI_stMemSizePerLine;                          /*  ÿһ�е��ڴ��С            */
    caddr_t             GMSI_pcMem;                                     /*  ��ʾ�ڴ� (�Դ������ַ)     */
} LW_GM_SCRINFO;
typedef LW_GM_SCRINFO  *PLW_GM_SCRINFO;

/*********************************************************************************************************
  ������Ϣ (�� GMPHY_uiXmm �� GMPHY_uiYmm ��Ϊ 0 ʱ, GMPHY_uiDpi ������Ч)
*********************************************************************************************************/

typedef struct {
    UINT                GMPHY_uiXmm;                                    /*  ���������                  */
    UINT                GMPHY_uiYmm;                                    /*  ���������                  */
    UINT                GMPHY_uiDpi;                                    /*  ÿӢ��������                */
    ULONG               GMPHY_ulReserve[16];                            /*  ����                        */
} LW_GM_PHYINFO;
typedef LW_GM_PHYINFO  *PLW_GM_PHYINFO;

/*********************************************************************************************************
  ��׼��Ļ���� ioctl
*********************************************************************************************************/
#define LW_GM_GET_VARINFO   LW_OSIOR('g', 200, LW_GM_VARINFO)           /*  �����ʾ���                */
#define LW_GM_SET_VARINFO   LW_OSIOW('g', 201, LW_GM_VARINFO)           /*  ������ʾ���                */
#define LW_GM_GET_SCRINFO   LW_OSIOR('g', 202, LW_GM_SCRINFO)           /*  �����ʾ����                */
#define LW_GM_GET_PHYINFO   LW_OSIOR('g', 203, LW_GM_PHYINFO)           /*  �����ʾ��������            */

/*********************************************************************************************************
  ��ʾģʽ
*********************************************************************************************************/
#define LW_GM_MODE_PALETTE  0x80000000                                  /*  ��ɫ��ģʽ����              */

#define LW_GM_GET_MODE      LW_OSIOR('g', 204, ULONG)                   /*  ��ȡ��ʾģʽ                */
#define LW_GM_SET_MODE      LW_OSIOD('g', 205, ULONG)                   /*  ������ʾģʽ                */

/*********************************************************************************************************
  ͼ���豸 file_operations
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

typedef struct gmem_file_operations {
    FUNCPTR             GMFO_pfuncOpen;                                 /*  ����ʾ                    */
    FUNCPTR             GMFO_pfuncClose;                                /*  �ر���ʾ                    */
    
    FUNCPTR             GMFO_pfuncIoctl;                                /*  �豸����                    */
    
    INT               (*GMFO_pfuncGetVarInfo)(LONG            lDev, 
                                              PLW_GM_VARINFO  pgmvi);   /*  ��� VARINFO                */
    INT               (*GMFO_pfuncSetVarInfo)(LONG                  lDev, 
                                              const PLW_GM_VARINFO  pgmvi);
                                                                        /*  ���� VARINFO                */
    INT               (*GMFO_pfuncGetScrInfo)(LONG            lDev, 
                                              PLW_GM_SCRINFO  pgmsi);   /*  ��� SCRINFO                */
    INT               (*GMFO_pfuncGetPhyInfo)(LONG            lDev, 
                                              PLW_GM_PHYINFO  pgmphy);  /*  ��� PHYINFO                */
    INT               (*GMFO_pfuncGetMode)(LONG     lDev, 
                                           ULONG   *pulMode);           /*  ��ȡ��ʾģʽ                */
    INT               (*GMFO_pfuncSetMode)(LONG     lDev, 
                                           ULONG    ulMode);            /*  ������ʾģʽ                */
                                           
    INT               (*GMFO_pfuncSetPalette)(LONG     lDev,
                                              UINT     uiStart,
                                              UINT     uiLen,
                                              ULONG   *pulRed,
                                              ULONG   *pulGreen,
                                              ULONG   *pulBlue);        /*  ���õ�ɫ��                  */
    INT               (*GMFO_pfuncGetPalette)(LONG     lDev,
                                              UINT     uiStart,
                                              UINT     uiLen,
                                              ULONG   *pulRed,
                                              ULONG   *pulGreen,
                                              ULONG   *pulBlue);        /*  ��ȡ��ɫ��                  */
    /*
     *  2D ���� (���� GMVI_bHardwareAccelerate Ϊ LW_TRUE ʱ��Ч)
     */
    INT               (*GMFO_pfuncSetPixel)(LONG     lDev, 
                                            INT      iX, 
                                            INT      iY, 
                                            ULONG    ulColor);          /*  ����һ������                */
    INT               (*GMFO_pfuncGetPixel)(LONG     lDev, 
                                            INT      iX, 
                                            INT      iY, 
                                            ULONG   *pulColor);         /*  ��ȡһ������                */
    INT               (*GMFO_pfuncSetColor)(LONG     lDev, 
                                            ULONG    ulColor);          /*  ���õ�ǰ��ͼǰ��ɫ          */
    INT               (*GMFO_pfuncSetAlpha)(LONG     lDev, 
                                            ULONG    ulAlpha);          /*  ���õ�ǰ��ͼ͸����          */
    INT               (*GMFO_pfuncDrawHLine)(LONG    lDev, 
                                             INT     iX0,
                                             INT     iY,
                                             INT     IX1);              /*  ����һ��ˮƽ��              */
    INT               (*GMFO_pfuncDrawVLine)(LONG    lDev, 
                                             INT     iX,
                                             INT     iY0,
                                             INT     IY1);              /*  ����һ����ֱ��              */
    INT               (*GMFO_pfuncFillRect)(LONG    lDev, 
                                            INT     iX0,
                                            INT     iY0,
                                            INT     iX1,
                                            INT     iY1);               /*  �������                    */
} LW_GM_FILEOPERATIONS;
typedef LW_GM_FILEOPERATIONS    *PLW_GM_FILEOPERATIONS;

/*********************************************************************************************************
  ͼ���豸���ƿ�
*********************************************************************************************************/

typedef struct {
    LW_DEV_HDR                  GMDEV_devhdrHdr;                        /*  IO �豸ͷ                   */
    PLW_GM_FILEOPERATIONS       GMDEV_gmfileop;                         /*  �豸����������              */
    ULONG                       GMDEV_ulMapFlags;                       /*  �ڴ�ӳ��ѡ��                */
    PVOID                       GMDEV_pvReserved[8];                    /*  ����������                  */
    /*
     * ... (�豸�����Ϣ, �˽ṹ����Ϊ���л�ͼ�����ĵ�һ������)
     */
} LW_GM_DEVICE;
typedef LW_GM_DEVICE           *PLW_GM_DEVICE;

/*********************************************************************************************************
  ��ʾģʽ
*********************************************************************************************************/


LW_API INT              API_GMemDevAdd(CPCHAR  cpcName, PLW_GM_DEVICE  pgmdev);
LW_API PLW_GM_DEVICE    API_GMemGet2D(INT  iFd);
LW_API INT              API_GMemSetPalette(INT      iFd,
                                           UINT     uiStart,
                                           UINT     uiLen,
                                           ULONG   *pulRed,
                                           ULONG   *pulGreen,
                                           ULONG   *pulBlue);
LW_API INT              API_GMemGetPalette(INT      iFd,
                                           UINT     uiStart,
                                           UINT     uiLen,
                                           ULONG   *pulRed,
                                           ULONG   *pulGreen,
                                           ULONG   *pulBlue);
LW_API INT              API_GMemSetPixel(PLW_GM_DEVICE    gmdev,
                                         INT              iX, 
                                         INT              iY, 
                                         ULONG            ulColor);
LW_API INT              API_GMemGetPixel(PLW_GM_DEVICE    gmdev,
                                         INT              iX, 
                                         INT              iY, 
                                         ULONG           *pulColor);
LW_API INT              API_GMemSetColor(PLW_GM_DEVICE    gmdev,
                                         ULONG            ulColor);
LW_API INT              API_GMemSetAlpha(PLW_GM_DEVICE    gmdev,
                                         ULONG            ulAlpha);
LW_API INT              API_GMemDrawHLine(PLW_GM_DEVICE   gmdev,
                                          INT             iX0,
                                          INT             iY,
                                          INT             iX1);
LW_API INT              API_GMemDrawVLine(PLW_GM_DEVICE   gmdev,
                                          INT             iX,
                                          INT             iY0,
                                          INT             iY1);
LW_API INT              API_GMemFillRect(PLW_GM_DEVICE   gmdev,
                                         INT             iX0,
                                         INT             iY0,
                                         INT             iX1,
                                         INT             iY1);
                         
#define gmemDevAdd                      API_GMemDevAdd
#define gmemGet2d                       API_GMemGet2D
#define gmemSetPalette                  API_GMemSetPalette
#define gmemGetPalette                  API_GMemGetPalette
#define gmemSetPixel                    API_GMemSetPixel
#define gmemGetPixel                    API_GMemGetPixel
#define gmemSetColor                    API_GMemSetColor
#define gmemSetAlpha                    API_GMemSetAlpha
#define gmemDrawHLine                   API_GMemDrawHLine
#define gmemDrawVLine                   API_GMemDrawVLine
#define gmemFillRect                    API_GMemFillRect

#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  LW_CFG_GRAPH_EN             */
#endif                                                                  /*  __GMEMDEV_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
