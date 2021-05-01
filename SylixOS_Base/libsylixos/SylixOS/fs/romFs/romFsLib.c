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
** ��   ��   ��: romFsLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 06 �� 27 ��
**
** ��        ��: rom �ļ�ϵͳ sylixos �ڲ�����. 
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "sys/endian.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0 && LW_CFG_ROMFS_EN > 0
#include "romFsLib.h"
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define ROMFS_MAGIC_STR             "-rom1fs-"
#define ROMFS_MAGIC_0               ('-' << 24 | 'r' << 16 | 'o' << 8 | 'm')
#define ROMFS_MAGIC_1               ('1' << 24 | 'f' << 16 | 's' << 8 | '-')
#define ROMFS_BUFFER_SIZE           512

#define ROMFH_ALIGN                 16                                  /*  16 �ֽڶ���                 */
#define ROMFH_ALIGN_SHIFT           4                                   /*  2 ^ 4 == 16                 */
/*********************************************************************************************************
  romfs ���ļ�����
*********************************************************************************************************/
#define ROMFH_HRD                   0
#define ROMFH_DIR                   1
#define ROMFH_REG                   2
#define ROMFH_LNK                   3
#define ROMFH_BLK                   4
#define ROMFH_CHR                   5
#define ROMFH_SCK                   6
#define ROMFH_FIF                   7
#define ROMFH_EXEC                  8
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
#define __ROMFS_SECTOR_READ(promfs, sector) \
        (promfs)->ROMFS_pblkd->BLKD_pfuncBlkRd((promfs)->ROMFS_pblkd, \
                                               (promfs)->ROMFS_pcSector, \
                                               (sector), 1)
#define __ROMFS_SECTOR_BUFFER(promfs) \
        (promfs)->ROMFS_pcSector
        
#define __ROMFS_SECTOR_SIZE(promfs) \
        (UINT32)(promfs)->ROMFS_ulSectorSize
        
#define __ROMFS_SECTOR_SIZE_MASK(promfs) \
        (UINT32)((promfs)->ROMFS_ulSectorSize - 1)
/*********************************************************************************************************
  ROMFS �ṹ����
*********************************************************************************************************/
typedef struct {
    UINT32      ROMFSSB_uiMagic0;
    UINT32      ROMFSSB_uiMagic1;
    UINT32      ROMFSSB_uiSize;
    UINT32      ROMFSSB_uiChecksum;
    CHAR        ROMFSSB_cName[1];
} ROMFS_SUPER_BLOCK;
typedef ROMFS_SUPER_BLOCK   *PROMFS_SUPER_BLOCK;

typedef struct {
    UINT32      ROMFSN_uiNext;
    UINT32      ROMFSN_uiSpec;
    UINT32      ROMFSN_uiSize;
    UINT32      ROMFSN_uiChecksum;
    CHAR        ROMFSN_cName[1];
} ROMFS_NODE;
typedef ROMFS_NODE          *PROMFS_NODE;
/*********************************************************************************************************
** ��������: __rfs_ntohl
** ��������: ��ͬ�� ntohl
** �䡡��  : uiData        ת����ֵ
** �䡡��  : ת�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT32 __rfs_ntohl (UINT32 uiData)
{
#if BYTE_ORDER == LITTLE_ENDIAN
    return  ((uiData & 0xff) << 24) |
            ((uiData & 0xff00) << 8) |
            ((uiData & 0xff0000UL) >> 8) |
            ((uiData & 0xff000000UL) >> 24);
#else
    return  (uiData);
#endif
}
/*********************************************************************************************************
** ��������: __rfs_checksum
** ��������: romfs ���� checksum 
** �䡡��  : pvData           ���ݵ�ַ
**           iSize            У���С
** �䡡��  : ������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT32 __rfs_checksum (PVOID pvData, INT iSize)
{
    UINT32  uiSum;
    UINT32 *puiData;

    uiSum   = 0; 
    puiData = (UINT32 *)pvData;
    
    iSize >>= 2;
    while (iSize > 0) {
        uiSum += __rfs_ntohl(*puiData);
        puiData++;
        iSize--;
    }
    
    return  (uiSum);
}
/*********************************************************************************************************
** ��������: __rfs_getsector
** ��������: romfs ��ָ����������װ�뻺��
** �䡡��  : promfs           �ļ�ϵͳ
**           ulSectorNo       ������
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __rfs_getsector (PROM_VOLUME  promfs, ULONG  ulSectorNo)
{
    if (promfs->ROMFS_ulCurSector == ulSectorNo) {
        return  (ERROR_NONE);
    }
    
    if (ulSectorNo >= promfs->ROMFS_ulSectorNum) {
        return  (PX_ERROR);
    }
    
    if (__ROMFS_SECTOR_READ(promfs, ulSectorNo) < ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    promfs->ROMFS_ulCurSector = ulSectorNo;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __rfs_getaddr
** ��������: romfs �� addr ָ����ַ������װ�뻺��
** �䡡��  : promfs           �ļ�ϵͳ
**           uiAddr           ��ַ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __rfs_getaddr (PROM_VOLUME  promfs, UINT32  uiAddr)
{
    ULONG  ulSectorNo = uiAddr / promfs->ROMFS_ulSectorSize;
    
    if (promfs->ROMFS_ulCurSector == ulSectorNo) {                      /*  �����ж�                    */
        return  (ERROR_NONE);
    }

    return  (__rfs_getsector(promfs, ulSectorNo));
}
/*********************************************************************************************************
** ��������: __rfs_bufaddr
** ��������: romfs ���ָ�� romfs ��ַ�����ݻ����ַ
** �䡡��  : promfs           �ļ�ϵͳ
**           uiAddr           ��ַ
** �䡡��  : ���ݻ����ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PCHAR __rfs_bufaddr (PROM_VOLUME  promfs, UINT32  uiAddr)
{
    if (__rfs_getaddr(promfs, uiAddr)) {                                /*  ���ָ����ַ����            */
        return  (LW_NULL);
    }

    return  (&__ROMFS_SECTOR_BUFFER(promfs)[uiAddr % promfs->ROMFS_ulSectorSize]);
}
/*********************************************************************************************************
** ��������: __rfs_memcpy
** ��������: romfs ���ļ�ϵͳ�豸ָ����ַ��ʼ���� uiSize ������
** �䡡��  : promfs           �ļ�ϵͳ
**           pcDest           ����Ŀ��
**           uiAddr           romfs �ڲ���ַ
**           uiSize           ��Ҫ�����Ĵ�С
** �䡡��  : ʵ�ʿ�������������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t __rfs_memcpy (PROM_VOLUME  promfs, PCHAR  pcDest, UINT32  uiAddr, UINT32  uiSize)
{
    UINT        uiTimes;
    UINT        uiLefts;
    PCHAR       pcSrc;
    INT         i;
    ssize_t     sstNum = 0;
    
    if (uiAddr & __ROMFS_SECTOR_SIZE_MASK(promfs)) {
        UINT32  uiAddrSector = ROUND_UP(uiAddr, __ROMFS_SECTOR_SIZE(promfs));
        UINT32  uiLen        = uiAddrSector - uiAddr;
        
        uiLen = __MIN(uiLen, uiSize);
        pcSrc = __rfs_bufaddr(promfs, uiAddr);
        if (!pcSrc) {
            return  (sstNum);
        }
        lib_memcpy(pcDest, pcSrc, uiLen);
        
        pcDest += uiLen;
        sstNum += uiLen;
        uiAddr += uiLen;
        uiSize -= uiLen;
    }
    
    if (uiSize == 0) {
        return  (sstNum);                                               /*  ��������                    */
    }
    
    uiTimes = uiSize / __ROMFS_SECTOR_SIZE(promfs);
    uiLefts = uiSize & __ROMFS_SECTOR_SIZE_MASK(promfs);
    
    for (i = 0; i < uiTimes; i++) {
        pcSrc = __rfs_bufaddr(promfs, uiAddr);
        if (!pcSrc) {
            return  (sstNum);
        }
        lib_memcpy(pcDest, pcSrc, (size_t)__ROMFS_SECTOR_SIZE(promfs));
        
        uiAddr += __ROMFS_SECTOR_SIZE(promfs);
        pcDest += __ROMFS_SECTOR_SIZE(promfs);
        sstNum += __ROMFS_SECTOR_SIZE(promfs);
    }
    
    if (uiLefts) {
        pcSrc = __rfs_bufaddr(promfs, uiAddr);
        if (!pcSrc) {
            return  (sstNum);
        }
        lib_memcpy(pcDest, pcSrc, uiLefts);
        
        sstNum += uiLefts;
    }
    
    return  (sstNum);
}
/*********************************************************************************************************
** ��������: __rfs_strncpy
** ��������: romfs ���ļ�ϵͳ�豸ָ����ַ��ʼ�����ַ���
** �䡡��  : promfs           �ļ�ϵͳ
**           pcDest           ����Ŀ��
**           uiAddr           romfs �ڲ���ַ
**           stMax            ��������󳤶�
** �䡡��  : �ַ�������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t __rfs_strncpy (PROM_VOLUME  promfs, PCHAR  pcDest, UINT32  uiAddr, size_t  stMax)
{
    UINT        uiTimes;
    UINT        uiLefts;
    PCHAR       pcSrc;
    INT         i;
    ssize_t     sstNum = 0;

    if (uiAddr & __ROMFS_SECTOR_SIZE_MASK(promfs)) {
        UINT32  uiAddrSector = ROUND_UP(uiAddr, __ROMFS_SECTOR_SIZE(promfs));
        UINT32  uiLen        = uiAddrSector - uiAddr;
        
        uiLen = __MIN(uiLen, stMax);
        
        pcSrc = __rfs_bufaddr(promfs, uiAddr);
        if (!pcSrc) {
            return  (sstNum);
        }
        
        for (i = 0; i < uiLen; i++) {
            if (*pcSrc) {
                *pcDest++ = *pcSrc++;
                sstNum++;
            } else {
                *pcDest = *pcSrc;
                return  (sstNum);
            }
        }
        
        stMax  -= uiLen;
        uiAddr += uiLen;
    }
    
    if (stMax == 0) {
        return  (sstNum);
    }
    
    uiTimes = stMax / __ROMFS_SECTOR_SIZE(promfs);
    uiLefts = stMax & __ROMFS_SECTOR_SIZE_MASK(promfs);
    
    for (i = 0; i < uiTimes; i++) {
        pcSrc = __rfs_bufaddr(promfs, uiAddr);
        if (!pcSrc) {
            return  (sstNum);
        }
        
        for (i = 0; i < __ROMFS_SECTOR_SIZE(promfs); i++) {
            if (*pcSrc) {
                *pcDest++ = *pcSrc++;
                sstNum++;
            } else {
                *pcDest = *pcSrc;
                return  (sstNum);
            }
        }
        
        uiAddr += __ROMFS_SECTOR_SIZE(promfs);
    }
    
    if (uiLefts) {
        pcSrc = __rfs_bufaddr(promfs, uiAddr);
        if (!pcSrc) {
            return  (sstNum);
        }
        
        for (i = 0; i < uiLefts; i++) {
            if (*pcSrc) {
                *pcDest++ = *pcSrc++;
                sstNum++;
            } else {
                *pcDest = *pcSrc;
                return  (sstNum);
            }
        }
    }
    
    return  (sstNum);
}
/*********************************************************************************************************
** ��������: __rfs_mount
** ��������: romfs mount ����
** �䡡��  : promfs           �ļ�ϵͳ
** �䡡��  : �Ƿ�ɹ� mount
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __rfs_mount (PROM_VOLUME  promfs)
{
    PROMFS_SUPER_BLOCK   prsb;
    size_t               stVolNameLen;
    UINT                 uiSize;
    
    if (__rfs_getsector(promfs, 0)) {                                   /*  ��� 0 ����                 */
        return  (PX_ERROR);
    }
    
    prsb = (PROMFS_SUPER_BLOCK)__ROMFS_SECTOR_BUFFER(promfs);           /*  promfs �������ֶ���         */
    
    if ((prsb->ROMFSSB_uiMagic0 != __rfs_ntohl(ROMFS_MAGIC_0)) || 
        (prsb->ROMFSSB_uiMagic1 != __rfs_ntohl(ROMFS_MAGIC_1))) {       /*  �鿴ħ��                    */
        return  (PX_ERROR);
    }
    
    uiSize = __rfs_ntohl(prsb->ROMFSSB_uiSize);
    if (__rfs_checksum(prsb, __MIN(uiSize, 512))) {                     /*  У�����                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "checksum error.\r\n");
        return  (PX_ERROR);
    }
    
    stVolNameLen = lib_strnlen(prsb->ROMFSSB_cName, (size_t)(promfs->ROMFS_ulSectorSize - 16));
    if (stVolNameLen >= promfs->ROMFS_ulSectorSize) {                   /*  ���������                  */
        return  (PX_ERROR);
    }
    
    if (ALIGNED(stVolNameLen, ROMFH_ALIGN)) {
        promfs->ROMFS_uiRootAddr = (UINT32)(stVolNameLen + 16) + 16;
    
    } else {
        promfs->ROMFS_uiRootAddr = (UINT32)ROUND_UP(stVolNameLen, ROMFH_ALIGN) + 16;
    }
    
    promfs->ROMFS_uiTotalSize = __rfs_ntohl(prsb->ROMFSSB_uiSize);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __rfs_getfile
** ��������: romfs ���ָ����ַ��Ӧ���ļ�
** �䡡��  : promfs           �ļ�ϵͳ
**           uiAddr           �豸��ַ
**           promdnt          �ļ���Ϣ
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __rfs_getfile (PROM_VOLUME  promfs, UINT32  uiAddr, PROMFS_DIRENT promdnt)
{
    PROMFS_NODE     prnode;
    UINT32          uiAddrName;
    UINT32          uiAddrData;
    
    UINT32          uiNext;
    UINT32          uiSpec;
    UINT32          uiSize;
    UINT            uiTemp;
    ssize_t         sstNameLen;
    
    prnode = (PROMFS_NODE)__rfs_bufaddr(promfs, uiAddr);                /*  promfs �������ֶ���         */
    if (prnode == LW_NULL) {
        return  (PX_ERROR);
    }
    
    promdnt->ROMFSDNT_uiMe = uiAddr;                                    /*  ��¼�Լ���λ��              */
    
    uiNext = __rfs_ntohl(prnode->ROMFSN_uiNext);
    uiSpec = __rfs_ntohl(prnode->ROMFSN_uiSpec);
    uiSize = __rfs_ntohl(prnode->ROMFSN_uiSize);
    
    uiAddrName = uiAddr + 16;                                           /*  �ļ�����ʼ��ַ              */
    sstNameLen = __rfs_strncpy(promfs, promdnt->ROMFSDNT_cName, uiAddrName, MAX_FILENAME_LENGTH);
    if (sstNameLen <= 0) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    if (sstNameLen >= MAX_FILENAME_LENGTH) {
        _ErrorHandle(ENAMETOOLONG);
        return  (PX_ERROR);
    }
    
    if (ALIGNED(sstNameLen, ROMFH_ALIGN)) {
        uiAddrData = (UINT32)((uiAddrName + sstNameLen) + 16);
        
    } else {
        uiAddrData = (UINT32)ROUND_UP((uiAddrName + sstNameLen), ROMFH_ALIGN);
    }
    
    uiTemp = uiNext & 0xF;
    promdnt->ROMFSDNT_stat.st_mode = 0;
    if (uiTemp & ROMFH_EXEC) {
        uiTemp &= (~ROMFH_EXEC);
        promdnt->ROMFSDNT_stat.st_mode = S_IXUSR | S_IXGRP | S_IXOTH;
    }
    
    switch (uiTemp) {
    
    case ROMFH_HRD:
        promdnt->ROMFSDNT_stat.st_mode |= S_IFLNK;
        break;
        
    case ROMFH_DIR:
        promdnt->ROMFSDNT_stat.st_mode |= S_IFDIR;
        break;
        
    case ROMFH_REG:
        promdnt->ROMFSDNT_stat.st_mode |= S_IFREG;
        break;
        
    case ROMFH_LNK:
        promdnt->ROMFSDNT_stat.st_mode |= S_IFLNK;
        break;
        
    case ROMFH_BLK:
        promdnt->ROMFSDNT_stat.st_mode |= S_IFBLK;
        break;
        
    case ROMFH_CHR:
        promdnt->ROMFSDNT_stat.st_mode |= S_IFCHR;
        break;
        
    case ROMFH_SCK:
        promdnt->ROMFSDNT_stat.st_mode |= S_IFSOCK;
        break;
        
    case ROMFH_FIF:
        promdnt->ROMFSDNT_stat.st_mode |= S_IFIFO;
        break;
        
    default:
        promdnt->ROMFSDNT_stat.st_mode |= S_IFCHR;
        break;
    }
    
    promdnt->ROMFSDNT_stat.st_dev     = LW_DEV_MAKE_STDEV(&promfs->ROMFS_devhdrHdr);
    promdnt->ROMFSDNT_stat.st_ino     = (ino_t)promdnt->ROMFSDNT_uiMe;
    promdnt->ROMFSDNT_stat.st_mode   |= S_IRUSR | S_IRGRP | S_IROTH;
    promdnt->ROMFSDNT_stat.st_nlink   = 1;
    promdnt->ROMFSDNT_stat.st_uid     = promfs->ROMFS_uid;
    promdnt->ROMFSDNT_stat.st_gid     = promfs->ROMFS_gid;
    promdnt->ROMFSDNT_stat.st_rdev    = 1;
    promdnt->ROMFSDNT_stat.st_size    = (off_t)uiSize;
    promdnt->ROMFSDNT_stat.st_blksize = uiSize;
    promdnt->ROMFSDNT_stat.st_blocks  = 1;
    promdnt->ROMFSDNT_stat.st_atime   = promfs->ROMFS_time;
    promdnt->ROMFSDNT_stat.st_mtime   = promfs->ROMFS_time;
    promdnt->ROMFSDNT_stat.st_ctime   = promfs->ROMFS_time;
    
    promdnt->ROMFSDNT_uiNext = uiNext & (~0xF);
    promdnt->ROMFSDNT_uiSpec = uiSpec;
    promdnt->ROMFSDNT_uiData = uiAddrData;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __rfs_lookup
** ��������: romfs ��ָ�� uiAddr �ļ�����, ��ȡָ���ļ�����Ӧ���ļ�
** �䡡��  : promfs           �ļ�ϵͳ
**           uiAddr           �豸��ַ
**           pcName           �ļ���
**           promdnt          �ļ���Ϣ
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __rfs_lookup (PROM_VOLUME  promfs, UINT32  uiAddr, PCHAR  pcName, PROMFS_DIRENT promdnt)
{
    do {
        if (uiAddr == 0) {
            return  (PX_ERROR);
        }
        
        if (__rfs_getfile(promfs, uiAddr, promdnt)) {
            return  (PX_ERROR);
        }
        
        if (lib_strcmp(pcName, promdnt->ROMFSDNT_cName) == 0) {
            break;
        }
        
        uiAddr = promdnt->ROMFSDNT_uiNext;
    } while (1);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __rfs_pread
** ��������: romfs ��ȡһ���ļ�
** �䡡��  : promfs           �ļ�ϵͳ
**           promdnt          �ļ�
**           pcDest           Ŀ�껺��
**           stSize           ��ȡ�ֽ���
**           uiOffset         ��ʼƫ����
** �䡡��  : ��ȡ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ssize_t  __rfs_pread (PROM_VOLUME  promfs, PROMFS_DIRENT promdnt, 
                      PCHAR  pcDest, size_t stSize, UINT32  uiOffset)
{
    UINT32      uiReadSize = (UINT32)__MIN((promdnt->ROMFSDNT_stat.st_size - uiOffset), stSize); 
    
    if (uiOffset >= promdnt->ROMFSDNT_stat.st_size) {
        return  (0);
    }
    
    return  (__rfs_memcpy(promfs, pcDest, 
                          promdnt->ROMFSDNT_uiData + uiOffset, 
                          uiReadSize));
}
/*********************************************************************************************************
** ��������: __rfs_readlink
** ��������: romfs ��ȡһ�������ļ�
** �䡡��  : promfs           �ļ�ϵͳ
**           promdnt          �ļ�
**           pcDest           Ŀ�껺��
**           stSize           ��ȡ�ֽ���
**           offt             ��ʼƫ����
** �䡡��  : ��ȡ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ssize_t  __rfs_readlink (PROM_VOLUME  promfs, PROMFS_DIRENT promdnt, PCHAR  pcDest, size_t stSize)
{
    UINT32      uiReadSize = stSize;
    
    return  (__rfs_strncpy(promfs, pcDest, promdnt->ROMFSDNT_uiData, uiReadSize));
}
/*********************************************************************************************************
** ��������: __rfs_open
** ��������: romfs ��һ���ļ�
** �䡡��  : promfs           �ļ�ϵͳ
**           pcName           �ļ���
**           ppcTail          ������������ļ�, ָ�������ļ����·��
**           ppcSymfile       �����ļ�
**           promdnt          �ļ��ṹ(��ȡ)
** �䡡��  : �򿪽��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __rfs_open (PROM_VOLUME  promfs, CPCHAR  pcName, 
                 PCHAR  *ppcTail, PCHAR  *ppcSymfile, PROMFS_DIRENT  promdnt)
{
    CHAR        cNameBuffer[PATH_MAX + 1];
    PCHAR       pcNameStart;
    PCHAR       pcPtr = cNameBuffer;
    
    UINT32      uiAddr = promfs->ROMFS_uiRootAddr;
    
    if (__STR_IS_ROOT(pcName)) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    lib_strlcpy(cNameBuffer, pcName, PATH_MAX + 1);
    if (*pcPtr) {
        pcPtr++;
    }
    
    while (*pcPtr) {
        pcNameStart = pcPtr;
        
        while (*pcPtr && *pcPtr != PX_DIVIDER) {
            pcPtr++;
        }
        if (*pcPtr != 0) {
            *pcPtr++ = 0;
        }
        
        if (__rfs_lookup(promfs, uiAddr, pcNameStart, promdnt)) {
            _ErrorHandle(ENOENT);
            return  (PX_ERROR);
        }
        
        if (S_ISLNK(promdnt->ROMFSDNT_stat.st_mode)) {
            *ppcTail    = (char *)(pcName + (pcPtr - cNameBuffer));
            *ppcSymfile = (char *)(pcName + (pcNameStart - cNameBuffer));/* point to symlink file name  */
            return  (ERROR_NONE);
        
        } else if (!S_ISDIR(promdnt->ROMFSDNT_stat.st_mode)) {
            break;
        }
        
        if ((promdnt->ROMFSDNT_uiSpec == uiAddr) || 
            (promdnt->ROMFSDNT_uiSpec == 0)) {                          /*  �¼�Ŀ¼û���ļ�������      */
            break;
        }
        uiAddr = promdnt->ROMFSDNT_uiSpec;
    }
    
    if (*pcPtr) {                                                       /*  û�в��ҵ�Ŀ��              */
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: __rfs_path_build_link
** ��������: ���������ļ�������������Ŀ��
** �䡡��  : promfs           �ļ�ϵͳ
**           promdnt          �ļ��ṹ
**           pcDest           �������
**           stSize           ��������С
**           pcPrefix         ǰ׺
**           pcTail           ��׺
** �䡡��  : < 0 ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __rfs_path_build_link (PROM_VOLUME  promfs, PROMFS_DIRENT  promdnt, 
                            PCHAR        pcDest, size_t         stSize,
                            PCHAR        pcPrefix, PCHAR        pcTail)
{
    CHAR        cLink[PATH_MAX + 1];
    
    if (__rfs_readlink(promfs, promdnt, cLink, PATH_MAX + 1) > 0) {
        return  (_PathBuildLink(pcDest, stSize, promfs->ROMFS_devhdrHdr.DEVHDR_pcName, pcPrefix, 
                                cLink, pcTail));
    } else {
        return  (PX_ERROR);
    }
}

#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
                                                                        /*  LW_CFG_ROMFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
