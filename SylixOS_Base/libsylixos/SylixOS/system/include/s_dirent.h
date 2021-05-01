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
** ��   ��   ��: s_dirent.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 09 �� 19 ��
**
** ��        ��: POSIX dirent.h ͷ�ļ�.

** BUG:
2010.09.10  struct dirent �м��� d_type �ֶ�.
*********************************************************************************************************/

#ifndef __S_DIRENT_H
#define __S_DIRENT_H

/*********************************************************************************************************
  ��ؽṹ���� (���ļ����������ļ�ϵͳ����Ч)
*********************************************************************************************************/

struct dirent {
    char	            d_name[NAME_MAX + 1];                           /*  �ļ���                      */
    unsigned char       d_type;                                         /*  �ļ����� (����Ϊ DT_UNKNOWN)*/
    char                d_shortname[13];                                /*  fat ���ļ��� (���ܲ�����)   */
    PVOID              *d_resv;                                         /*  ����                        */
};

struct dirent64 {                                                       /*  same as `struct dirent'     */
    char	            d_name[NAME_MAX + 1];                           /*  �ļ���                      */
    unsigned char       d_type;                                         /*  �ļ����� (����Ϊ DT_UNKNOWN)*/
    char                d_shortname[13];                                /*  fat ���ļ��� (���ܲ�����)   */
    PVOID              *d_resv;                                         /*  ����                        */
};

typedef struct {
    int                 dir_fd;                                         /*  �ļ�������                  */
    LONG                dir_pos;                                        /*  λ��                        */
    struct dirent       dir_dirent;                                     /*  ��õ�ѡ��                  */
    LW_OBJECT_HANDLE    dir_lock;                                       /*  readdir_r ��Ҫ����          */
    LW_RESOURCE_RAW     dir_resraw;                                     /*  ����ԭʼ��Դ��¼            */
    
#ifdef __SYLIXOS_KERNEL
#define DIR_RESV_DATA_PV0(pdir)   ((pdir)->dir_resraw.RESRAW_pvArg[5])  /*  �ļ�ϵͳ����������          */
#define DIR_RESV_DATA_PV1(pdir)   ((pdir)->dir_resraw.RESRAW_pvArg[4])
#endif                                                                  /*  __SYLIXOS_KERNEL            */
} DIR;

/*********************************************************************************************************
  d_type 
  
  ע��: ������ÿһ���ļ�ϵͳ(���豸)��֧�� d_type �ֶ�, ���Ե� d_type == DT_UNKNOWN ʱ, ������� stat() 
        ϵ�к�������ȡ�ļ���ʵ������.
*********************************************************************************************************/

#define DT_UNKNOWN      0
#define DT_FIFO         1
#define DT_CHR          2
#define DT_DIR          4
#define DT_BLK          6
#define DT_REG          8
#define DT_LNK          10
#define DT_SOCK         12
#define DT_WHT          14

#define IFTODT(mode)    (unsigned char)(((mode) & 0xF000) >> 12)        /*  st_mode to d_type           */
#define DTTOIF(dtype)   (mode_t)((dtype) << 12)                         /*  d_type to st_mode           */

/*********************************************************************************************************
  POSIX API
*********************************************************************************************************/

LW_API INT              mkdir(CPCHAR  dirname, mode_t  mode);
LW_API INT              rmdir(CPCHAR  pathname); 

LW_API INT              dirfd(DIR  *pdir);
LW_API DIR             *opendir(CPCHAR   pathname);  
LW_API INT              closedir(DIR    *dir);
LW_API struct dirent   *readdir(DIR     *dir);
LW_API struct dirent64 *readdir64(DIR   *dir);
LW_API INT              readdir_r(DIR             *pdir, 
                                  struct dirent   *pdirentEntry,
                                  struct dirent  **ppdirentResult);
LW_API INT              readdir64_r(DIR             *pdir, 
                                    struct dirent64   *pdirent64Entry,
                                    struct dirent64  **ppdirent64Result);
LW_API INT              rewinddir(DIR   *dir);  

#endif                                                                  /*  __S_DIRENT_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
