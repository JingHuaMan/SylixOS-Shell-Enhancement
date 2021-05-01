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
** ��   ��   ��: memdev.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 06 �� 04 ��
**
** ��        ��: VxWorks �ڴ��豸���ݽӿ�.
*********************************************************************************************************/

#ifndef __MEMDEV_H
#define __MEMDEV_H

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_MEMDEV_EN > 0)

typedef struct mem_drv_direntry {                                       /*  ���� VxWorks �ṹ           */
    char                    *name;                                      /*  ����                        */
    char                    *base;                                      /*  �ڴ����ַ                  */
    struct mem_drv_direntry *pDir;                                      /*  ��һ���ڵ�                  */
    size_t                   length;                                    /*  �ڴ泤��                    */
} MEM_DRV_DIRENTRY;

/*********************************************************************************************************
  �ں� API
*********************************************************************************************************/

LW_API INT  API_MemDrvInstall(void);
LW_API INT  API_MemDevCreate(char *name, char *base, size_t length);
LW_API INT  API_MemDevCreateDir(char *name, MEM_DRV_DIRENTRY *files, int numFiles);
LW_API INT  API_MemDevDelete(char *name);

#define memDrv              API_MemDrvInstall
#define memDevCreate        API_MemDevCreate
#define memDevCreateDir     API_MemDevCreateDir
#define memDevDelete        API_MemDevDelete

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_MEMDEV_EN > 0)      */
#endif                                                                  /*  __MEMDEV_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
