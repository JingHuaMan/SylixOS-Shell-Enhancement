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
** ��   ��   ��: iso9660_port.c
**
** ��   ��   ��: Tiger.Jiang (��̫��)
**
** �ļ���������: 2018 �� 09 �� 15 ��
**
** ��        ��: ISO9660 �ļ�ϵͳ cdio ����ֲ��.
*********************************************************************************************************/
#include "iso9660_cfg.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_ISO9660FS_EN > 0)
#include "include/memory.h"
#include "include/types.h"
#include "include/iso9660.h"
/*********************************************************************************************************
  block ��������
*********************************************************************************************************/
extern INT  __blockIoDevRead(INT iIndex, VOID *pvBuffer, ULONG ulStartSector, ULONG ulSectorCount);
/*********************************************************************************************************
** ��������: cdio_stdio_new
** ��������: �½����ݲ����
** �䡡��  : drv_num    ���豸��
**           p_iso      cdio ���ڲ�����
** �䡡��  : ���ݲ����
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: cdio_stdio_new
** ��������: �������ݲ����
** �䡡��  : p_cds      ���ݲ����
** �䡡��  : ���ݲ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
void cdio_stdio_destroy (CdioDataSource_t *p_cds)
{
    cdio_free(p_cds);
}
/*********************************************************************************************************
** ��������: cdio_stdio_new
** ��������: ������
** �䡡��  : p_cds      ���ݲ����
**           ptr        ���ݻ�����
**           i_size     ��������λ��С
**           nmemb      ��������λ��������ȡ���� = i_size * nmemb
** �䡡��  : �ɹ����ض�ȡ���ȣ�ʧ�ܷ��� PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
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

    if (oft & ISO_BLOCK_MASK) {                                         /*  ��ʼλ�ò�����              */
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
     * ������ʼ������������
     */
    start_sec = (ULONG)(oft >> ISO_BLOCK_SHIFT);
    sec_cnt   = (ULONG)(total_bytes >> ISO_BLOCK_SHIFT);

    /*
     *  ��ȡ���豸����
     */
    if (__blockIoDevRead(p_cds->drv_num, ptr,
                         start_sec, sec_cnt) != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (total_bytes & ISO_BLOCK_MASK) {                                 /*  ��ȡ���Ȳ�����              */
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
** ��������: cdio_stream_seek
** ��������: ��������ƫ��
** �䡡��  : p_cds      ���ݲ����
**           i_offset   ƫ��ֵ
**           whence     ƫ�Ƽ��㷽ʽ
** �䡡��  : �ɹ����ض�ȡ���ȣ�ʧ�ܷ��� PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: cdio_calloc
** ��������: �����ڴ沢��0
** �䡡��  : sz         �ڴ浥λ��С
**           num        �ڴ浥λ����
** �䡡��  : �ɹ��ڴ�ָ�룬ʧ�ܷ��� LW_NULL
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: cdio_free
** ��������: �ͷ��ڴ�
** �䡡��  : p_memory   �ڴ��ַ
** �䡡��  : �ɹ��ڴ�ָ�룬ʧ�ܷ��� LW_NULL
** ȫ�ֱ���:
** ����ģ��:
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
