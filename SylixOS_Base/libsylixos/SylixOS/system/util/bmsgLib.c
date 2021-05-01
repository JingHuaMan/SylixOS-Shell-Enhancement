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
** ��   ��   ��: bmsgLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 10 �� 02 ��
**
** ��        ��: �ɿ���Ϣ������.
**
** BUG:
2020.04.03  ���������ֶδ��ڱ߽�ʱ���жϴ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ��ز���
*********************************************************************************************************/
#define LW_BMSG_PREFIX_LEN      2
/*********************************************************************************************************
** ��������: _bmsgCreate
** ��������: ����һ�� block msg ������
** �䡡��  : stSize       ��������С 
** �䡡��  : �½��Ļ�����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_BMSG  _bmsgCreate (size_t  stSize)
{
    PLW_BMSG    pbmsg;
    
    if (stSize < LW_BMSG_MIN_SIZE) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    pbmsg = (PLW_BMSG)__SHEAP_ALLOC(sizeof(LW_BMSG) + stSize);
    if (!pbmsg) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }
    
    pbmsg->BM_pucBuffer = (PUCHAR)pbmsg + sizeof(LW_BMSG);
    pbmsg->BM_stSize    = stSize;
    pbmsg->BM_stLeft    = stSize;
    
    pbmsg->BM_pucPut    = pbmsg->BM_pucBuffer;
    pbmsg->BM_pucGet    = pbmsg->BM_pucBuffer;
    
    return  (pbmsg);
}
/*********************************************************************************************************
** ��������: _bmsgDelete
** ��������: ɾ��һ�� block msg ������
** �䡡��  : pbmsg        ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _bmsgDelete (PLW_BMSG  pbmsg)
{
    __SHEAP_FREE(pbmsg);
}
/*********************************************************************************************************
** ��������: _bmsgPut
** ��������: ��һ�����ݴ��� block msg ������
** �䡡��  : pbmsg        ������
**           pvMsg        ��Ϣ
**           stSize       ��Ϣ���� (���ܴ��� 65535)
** �䡡��  : ������ֽ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _bmsgPut (PLW_BMSG  pbmsg, CPVOID  pvMsg, size_t  stSize)
{
#define LW_BMSG_ADJUST_PUT() \
        if (pbmsg->BM_pucPut >= (pbmsg->BM_pucBuffer + pbmsg->BM_stSize)) { \
            pbmsg->BM_pucPut  = pbmsg->BM_pucBuffer; \
        }

    size_t  stLen;
    size_t  stSave = stSize + LW_BMSG_PREFIX_LEN;
    
    if (stSave >= (UINT16)(~0)) {
        return  (0);
    }
    
    if (pbmsg->BM_stLeft < stSave) {
        return  (0);
    }
    
    *pbmsg->BM_pucPut = (UCHAR)(stSize >> 8);                           /*  ���������ֽڳ�����Ϣ        */
    pbmsg->BM_pucPut++;
    LW_BMSG_ADJUST_PUT();
    *pbmsg->BM_pucPut = (UCHAR)(stSize & 0xff);
    pbmsg->BM_pucPut++;
    LW_BMSG_ADJUST_PUT();
    
    stLen = (pbmsg->BM_pucBuffer + pbmsg->BM_stSize) - pbmsg->BM_pucPut;
    if (stLen >= stSize) {
        lib_memcpy(pbmsg->BM_pucPut, pvMsg, stSize);
        if (stLen > stSize) {
            pbmsg->BM_pucPut += stSize;
        
        } else {
            pbmsg->BM_pucPut = pbmsg->BM_pucBuffer;
        }
        
    } else {
        lib_memcpy(pbmsg->BM_pucPut, pvMsg, stLen);
        pbmsg->BM_pucPut  = pbmsg->BM_pucBuffer;
        
        lib_memcpy(pbmsg->BM_pucPut, (PUCHAR)pvMsg + stLen, stSize - stLen);
        pbmsg->BM_pucPut += (stSize - stLen);
    }
    
    pbmsg->BM_stLeft -= stSave;
    
    return  ((INT)stSize);
}
/*********************************************************************************************************
** ��������: _bmsgGet
** ��������: �� block msg ������ȡ��һ����Ϣ
** �䡡��  : pbmsg        ������
**           pvMsg        ���ջ���
**           stBufferSize ���ջ��峤��
** �䡡��  : ��ȡ���ֽ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _bmsgGet (PLW_BMSG  pbmsg, PVOID  pvMsg, size_t  stBufferSize)
{
#define LW_BMSG_ADJUST_GET() \
        if (pbmsg->BM_pucGet >= (pbmsg->BM_pucBuffer + pbmsg->BM_stSize)) { \
            pbmsg->BM_pucGet  = pbmsg->BM_pucBuffer; \
        }

    size_t  stLen;
    size_t  stSize;
    UCHAR   ucHigh;
    UCHAR   ucLow;
    
    stLen = pbmsg->BM_stSize - pbmsg->BM_stLeft;
    if (stLen == 0) {
        return  (0);
    }
    
    ucHigh = pbmsg->BM_pucGet[0];
    if ((pbmsg->BM_pucGet - pbmsg->BM_pucBuffer + 1) >= pbmsg->BM_stSize) {
        ucLow = pbmsg->BM_pucBuffer[0];
    } else {
        ucLow = pbmsg->BM_pucGet[1];
    }
    
    stSize = (size_t)((ucHigh << 8) + ucLow);
    if (stSize > stBufferSize) {
        return  (PX_ERROR);                                             /*  ������̫С                  */
    }
    
    pbmsg->BM_pucGet++;
    LW_BMSG_ADJUST_GET();
    pbmsg->BM_pucGet++;
    LW_BMSG_ADJUST_GET();
    
    stLen = (pbmsg->BM_pucBuffer + pbmsg->BM_stSize) - pbmsg->BM_pucGet;
    if (stLen >= stSize) {
        lib_memcpy(pvMsg, pbmsg->BM_pucGet, stSize);
        if (stLen > stSize) {
            pbmsg->BM_pucGet += stSize;
        
        } else {
            pbmsg->BM_pucGet = pbmsg->BM_pucBuffer;
        }
    
    } else {
        lib_memcpy(pvMsg, pbmsg->BM_pucGet, stLen);
        pbmsg->BM_pucGet  = pbmsg->BM_pucBuffer;
        
        lib_memcpy((PUCHAR)pvMsg + stLen, pbmsg->BM_pucGet, stSize - stLen);
        pbmsg->BM_pucGet += (stSize - stLen);
    }
    
    pbmsg->BM_stLeft += (stSize + LW_BMSG_PREFIX_LEN);
    
    return  ((INT)stSize);
}
/*********************************************************************************************************
** ��������: _bmsgFlush
** ��������: ��� block msg ������
** �䡡��  : pbmsg        ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _bmsgFlush (PLW_BMSG  pbmsg)
{
    pbmsg->BM_stLeft = pbmsg->BM_stSize;
    pbmsg->BM_pucPut = pbmsg->BM_pucBuffer;
    pbmsg->BM_pucGet = pbmsg->BM_pucBuffer;
}
/*********************************************************************************************************
** ��������: _bmsgIsEmpty
** ��������: block msg �������Ƿ�Ϊ��
** �䡡��  : pbmsg        ������
** �䡡��  : �Ƿ�Ϊ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _bmsgIsEmpty (PLW_BMSG  pbmsg)
{
    return  (pbmsg->BM_stLeft == pbmsg->BM_stSize);
}
/*********************************************************************************************************
** ��������: _bmsgIsFull
** ��������: block msg �������Ƿ�Ϊ��
** �䡡��  : pbmsg        ������
** �䡡��  : �Ƿ�����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _bmsgIsFull (PLW_BMSG  pbmsg)
{
    return  (pbmsg->BM_stLeft <= LW_BMSG_PREFIX_LEN);
}
/*********************************************************************************************************
** ��������: _bmsgSizeGet
** ��������: block msg ��������С
** �䡡��  : pbmsg        ������
** �䡡��  : �ܴ�С
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
size_t  _bmsgSizeGet (PLW_BMSG  pbmsg)
{
    return  (pbmsg->BM_stSize);
}
/*********************************************************************************************************
** ��������: _bmsgFreeByte
** ��������: block msg ������ʣ��ռ��С
** �䡡��  : pbmsg        ������
** �䡡��  : ʣ��ռ��С
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _bmsgFreeByte (PLW_BMSG  pbmsg)
{
    if (pbmsg->BM_stLeft <= LW_BMSG_PREFIX_LEN) {
        return  (0);
    } else {
        return  ((INT)(pbmsg->BM_stLeft - LW_BMSG_PREFIX_LEN));
    }
}
/*********************************************************************************************************
** ��������: _bmsgNBytes
** ��������: block msg ����������Ϣ��
** �䡡��  : pbmsg        ������
** �䡡��  : ����Ϣ��С
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _bmsgNBytes (PLW_BMSG  pbmsg)
{
    return  ((INT)(pbmsg->BM_stSize - pbmsg->BM_stLeft));
}
/*********************************************************************************************************
** ��������: _bmsgNBytesNext
** ��������: block msg ��������һ����Ϣ����
** �䡡��  : pbmsg        ������
** �䡡��  : ��һ����Ϣ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _bmsgNBytesNext (PLW_BMSG  pbmsg)
{
    size_t  stSize;
    UCHAR   ucHigh;
    UCHAR   ucLow;
    
    if (pbmsg->BM_stSize == pbmsg->BM_stLeft) {
        return  (0);
    }
    
    ucHigh = pbmsg->BM_pucGet[0];
    if ((pbmsg->BM_pucGet - pbmsg->BM_pucBuffer + 1) >= pbmsg->BM_stSize) {
        ucLow = pbmsg->BM_pucBuffer[0];
    } else {
        ucLow = pbmsg->BM_pucGet[1];
    }
    
    stSize = (size_t)((ucHigh << 8) + ucLow);
    
    return  ((INT)stSize);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
