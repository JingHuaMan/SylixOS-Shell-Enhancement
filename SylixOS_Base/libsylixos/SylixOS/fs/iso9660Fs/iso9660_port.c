/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: iso9660_port.c
**
** 创   建   人: Tiger.Jiang (蒋太金)
**
** 文件创建日期: 2018 年 09 月 15 日
**
** 描        述: ISO9660 文件系统 cdio 库移植层.
*********************************************************************************************************/
#include "iso9660_cfg.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_ISO9660FS_EN > 0)
#include "include/memory.h"
#include "include/types.h"
#include "include/iso9660.h"
/*********************************************************************************************************
  block 函数声明
*********************************************************************************************************/
extern INT  __blockIoDevRead(INT iIndex, VOID *pvBuffer, ULONG ulStartSector, ULONG ulSectorCount);
/*********************************************************************************************************
** 函数名称: cdio_stdio_new
** 功能描述: 新建数据层对象
** 输　入  : drv_num    块设备号
**           p_iso      cdio 库内部对象
** 输　出  : 数据层对象
** 全局变量:
** 调用模块:
*********************************************************************************************************/
CdioDataSource_t *cdio_stdio_new (int drv_num, void *p_iso)
{
    CdioDataSource_t *p_cds = (CdioDataSource_t *)__SHEAP_ALLOC(sizeof(CdioDataSource_t));

    if (!p_cds) {
        return  (LW_NULL);
    }

    p_cds->drv_num = drv_num;
    p_cds->p_iso   = p_iso;
    p_cds->oft     = 0;

    return  (p_cds);
}
/*********************************************************************************************************
** 函数名称: cdio_stdio_new
** 功能描述: 销毁数据层对象
** 输　入  : p_cds      数据层对象
** 输　出  : 数据层对象
** 全局变量:
** 调用模块:
*********************************************************************************************************/
void cdio_stdio_destroy (CdioDataSource_t *p_cds)
{
    cdio_free(p_cds);
}
/*********************************************************************************************************
** 函数名称: cdio_stdio_new
** 功能描述: 读数据
** 输　入  : p_cds      数据层对象
**           ptr        数据缓冲区
**           i_size     缓冲区单位大小
**           nmemb      缓冲区单位个数，读取长度 = i_size * nmemb
** 输　出  : 成功返回读取长度，失败返回 PX_ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ssize_t cdio_stream_read (CdioDataSource_t *p_cds,
                          void             *ptr,
                          size_t            i_size,
                          size_t            nmemb)
{
    ULONG      start_sec;
    ULONG      sec_cnt;
    PUCHAR     sec_buff;
    size_t     total_bytes = i_size * nmemb;
    ULONG      oft         = p_cds->oft;

    if (oft & ISO_BLOCK_MASK) {                                         /*  起始位置不对齐              */
        sec_buff = cdio_calloc(ISO_BLOCKSIZE, 1);
        if (!sec_buff) {
            return  (PX_ERROR);
        }

        if (__blockIoDevRead(p_cds->drv_num, sec_buff,
                             (oft >> ISO_BLOCK_SHIFT), 1) != ERROR_NONE) {
            cdio_free(sec_buff);
            return  (PX_ERROR);
        }

        lib_memcpy(ptr, sec_buff + (oft & ISO_BLOCK_MASK),
                   ISO_BLOCKSIZE - (oft & ISO_BLOCK_MASK));

        cdio_free(sec_buff);

        oft         += ISO_BLOCKSIZE - (oft & ISO_BLOCK_MASK);
        ptr         += ISO_BLOCKSIZE - (oft & ISO_BLOCK_MASK);
        total_bytes -= ISO_BLOCKSIZE - (oft & ISO_BLOCK_MASK);
    }

    /*
     * 计算起始扇区和扇区数
     */
    start_sec = (ULONG)(oft >> ISO_BLOCK_SHIFT);
    sec_cnt   = (ULONG)(total_bytes >> ISO_BLOCK_SHIFT);

    /*
     *  读取块设备数据
     */
    if (__blockIoDevRead(p_cds->drv_num, ptr,
                         start_sec, sec_cnt) != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (total_bytes & ISO_BLOCK_MASK) {                                 /*  读取长度不对齐              */
        sec_buff = cdio_calloc(ISO_BLOCKSIZE, 1);
        if (!sec_buff) {
            return  (PX_ERROR);
        }

        if (__blockIoDevRead(p_cds->drv_num, sec_buff,
                             start_sec + sec_cnt, 1) != ERROR_NONE) {
            cdio_free(sec_buff);
            return  (ssize_t)(PX_ERROR);
        }

        lib_memcpy(ptr + start_sec * sec_cnt, sec_buff, total_bytes & ISO_BLOCK_MASK);

        cdio_free(sec_buff);
    }

    return  (i_size * nmemb);
}
/*********************************************************************************************************
** 函数名称: cdio_stream_seek
** 功能描述: 设置数据偏移
** 输　入  : p_cds      数据层对象
**           i_offset   偏移值
**           whence     偏移计算方式
** 输　出  : 成功返回读取长度，失败返回 PX_ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
int cdio_stream_seek (CdioDataSource_t *p_cds,
                      off_t             i_offset,
                      int               whence)
{
    if (whence != SEEK_SET) {
        return  (PX_ERROR);
    }

    p_cds->oft = i_offset;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: cdio_calloc
** 功能描述: 分配内存并清0
** 输　入  : sz         内存单位大小
**           num        内存单位个数
** 输　出  : 成功内存指针，失败返回 LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
void *cdio_calloc (size_t sz, int num)
{
    void *prtn = LW_NULL;

    prtn = __SHEAP_ALLOC(((size_t)((sz) * (num))));
    if (prtn) {
        lib_bzero(prtn, ((sz) * (num)));
    }

    return  (prtn);
}
/*********************************************************************************************************
** 函数名称: cdio_free
** 功能描述: 释放内存
** 输　入  : p_memory   内存地址
** 输　出  : 成功内存指针，失败返回 LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
void cdio_free (void *p_memory)
{
    if (p_memory != LW_NULL) {
        __SHEAP_FREE(p_memory);
    }
}

#endif                                                                  /*  (LW_CFG_ISO9660FS_EN > 0)   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
