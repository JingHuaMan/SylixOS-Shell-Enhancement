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
** ��   ��   ��: rngLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 03 �� 05 ��
**
** ��        ��: VxWorks ���� ring buffer ����������.

** BUG:
2009.08.18  �����ȡ��������С�ĺ���.
2009.12.14  _rngCreate() ʹ�õ�һ���ڴ�, �����ڴ���Ƭ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����
*********************************************************************************************************/
VOID  _rngFlush(VX_RING_ID  vxringid);
/*********************************************************************************************************
** ��������: _rngCreate
** ��������: ����һ�� VxWorks ���� ring buffer ������
** �䡡��  : ��������С
** �䡡��  : ���������ƿ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VX_RING_ID  _rngCreate (INT  iNBytes)
{
    REGISTER PCHAR        pcBuffer;
    REGISTER VX_RING_ID   vxringid;
             size_t       stAllocSize;
    
    /* 
     *  bump number of bytes requested because ring buffer algorithm
     *  always leaves at least one empty byte in buffer 
     */
    iNBytes++;
    stAllocSize = (size_t)(sizeof(VX_RING) + iNBytes);
    
    /*
     *  ͳһ�����ڴ�����ڴ���Ƭ
     */
    vxringid = (VX_RING_ID)__SHEAP_ALLOC(stAllocSize);
    if (vxringid == LW_NULL) {
        return  (LW_NULL);
    }
    pcBuffer = (PCHAR)vxringid + sizeof(VX_RING);
    
    vxringid->VXRING_iBufByteSize = iNBytes;
    vxringid->VXRING_pcBuf        = pcBuffer;
    
    _rngFlush(vxringid);
    
    return  (vxringid);
}
/*********************************************************************************************************
** ��������: _rngDelete
** ��������: ɾ��һ�� VxWorks ���� ring buffer ������
** �䡡��  : ���������ƿ��ַ
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _rngDelete (VX_RING_ID  vxringid)
{
    __SHEAP_FREE(vxringid);
}
/*********************************************************************************************************
** ��������: _rngSizeGet
** ��������: ���һ�� VxWorks ���� ring buffer ��������С
** �䡡��  : ��������С
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _rngSizeGet (VX_RING_ID  vxringid)
{
    if (vxringid) {
        return  (vxringid->VXRING_iBufByteSize - 1);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: _rngFlush
** ��������: ��һ�� VxWorks ���� ring buffer ���������
** �䡡��  : ���������ƿ��ַ
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _rngFlush (VX_RING_ID  vxringid)
{
    vxringid->VXRING_iToBuf   = 0;
    vxringid->VXRING_iFromBuf = 0;
}
/*********************************************************************************************************
** ��������: _rngBufGet
** ��������: ��һ�� VxWorks ���� ring buffer ������������� iMaxBytes ���ֽ�
** �䡡��  : 
**           vxringid           ���������ƿ��ַ
**           pcBuffer           �������ݴ��λ��
**           iMaxBytes          ���������ֽ���
** �䡡��  : ʵ�ʶ������ֽ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _rngBufGet (VX_RING_ID  vxringid,
                 PCHAR       pcBuffer,
                 INT         iMaxBytes)
{
    REGISTER INT        iBytesGot = 0;
    REGISTER INT        iToBuf = vxringid->VXRING_iToBuf;
    REGISTER INT        iBytes2;
    REGISTER INT        iRngTemp = 0;
    
    if (iToBuf >= vxringid->VXRING_iFromBuf) {
    
        /* pToBuf has not wrapped around */
    
        iBytesGot = __MIN(iMaxBytes, iToBuf - vxringid->VXRING_iFromBuf);
        lib_memcpy(pcBuffer, &vxringid->VXRING_pcBuf[vxringid->VXRING_iFromBuf], iBytesGot);
        vxringid->VXRING_iFromBuf += iBytesGot;
    
    } else {
        
        /* pToBuf has wrapped around.  Grab chars up to the end of the
         * buffer, then wrap around if we need to. */

        iBytesGot = __MIN(iMaxBytes, vxringid->VXRING_iBufByteSize - vxringid->VXRING_iFromBuf);
        lib_memcpy(pcBuffer, &vxringid->VXRING_pcBuf[vxringid->VXRING_iFromBuf], iBytesGot);
        iRngTemp = vxringid->VXRING_iFromBuf + iBytesGot;
        
        /* If pFromBuf is equal to bufSize, we've read the entire buffer,
         * and need to wrap now.  If bytesgot < maxbytes, copy some more chars
         * in now. */
        
        if (iRngTemp == vxringid->VXRING_iBufByteSize) {
        
            iBytes2 = __MIN(iMaxBytes - iBytesGot, iToBuf);
            lib_memcpy(pcBuffer + iBytesGot, vxringid->VXRING_pcBuf, iBytes2);
            vxringid->VXRING_iFromBuf = iBytes2;
            iBytesGot += iBytes2;
        
        } else {
        
            vxringid->VXRING_iFromBuf = iRngTemp;
        }
    }
    
    return  (iBytesGot);
}
/*********************************************************************************************************
** ��������: _rngBufPut
** ��������: ��һ�� VxWorks ���� ring buffer ������д����� iMaxBytes ���ֽ�
** �䡡��  : 
**           vxringid           ���������ƿ��ַ
**           pcBuffer           д�����ݴ��λ��
**           iNBytes            д���ֽ���
** �䡡��  : ʵ�ʶ������ֽ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _rngBufPut (VX_RING_ID  vxringid,
                 PCHAR       pcBuffer,
                 INT         iNBytes)
{
    REGISTER INT        iBytesPut = 0;
    REGISTER INT        iFromBuf = vxringid->VXRING_iFromBuf;
    REGISTER INT        iBytes2;
    REGISTER INT        iRngTemp = 0;
    
    if (iFromBuf > vxringid->VXRING_iToBuf) {
        
        /* pFromBuf is ahead of pToBuf.  We can fill up to two bytes
         * before it */
         
        iBytesPut = __MIN(iNBytes, iFromBuf - vxringid->VXRING_iToBuf - 1);
        lib_memcpy(&vxringid->VXRING_pcBuf[vxringid->VXRING_iToBuf], pcBuffer, iBytesPut);
        vxringid->VXRING_iToBuf += iBytesPut;
        
    } else if (iFromBuf == 0) {
        
        /* pFromBuf is at the beginning of the buffer.  We can fill till
         * the next-to-last element */
        
        iBytesPut = __MIN(iNBytes, vxringid->VXRING_iBufByteSize - vxringid->VXRING_iToBuf - 1);
        lib_memcpy(&vxringid->VXRING_pcBuf[vxringid->VXRING_iToBuf], pcBuffer, iBytesPut);
        vxringid->VXRING_iToBuf += iBytesPut;
        
    } else {
        
        /* pFromBuf has wrapped around, and its not 0, so we can fill
         * at least to the end of the ring buffer.  Do so, then see if
         * we need to wrap and put more at the beginning of the buffer. */
         
        iBytesPut = __MIN(iNBytes, vxringid->VXRING_iBufByteSize - vxringid->VXRING_iToBuf);
        lib_memcpy(&vxringid->VXRING_pcBuf[vxringid->VXRING_iToBuf], pcBuffer, iBytesPut);
        iRngTemp = vxringid->VXRING_iToBuf + iBytesPut;
        
        if (iRngTemp == vxringid->VXRING_iBufByteSize) {
            
            /* We need to wrap, and perhaps put some more chars */
            
            iBytes2 = __MIN(iNBytes - iBytesPut, iFromBuf - 1);
            lib_memcpy(vxringid->VXRING_pcBuf, pcBuffer + iBytesPut, iBytes2);
            vxringid->VXRING_iToBuf = iBytes2;
            iBytesPut += iBytes2;
        
        } else {
            
            vxringid->VXRING_iToBuf = iRngTemp;
        }
    }
    
    return  (iBytesPut);
}
/*********************************************************************************************************
** ��������: _rngIsEmpty
** ��������: ���һ�� VxWorks ���� ring buffer �������Ƿ�Ϊ��
** �䡡��  : ���������ƿ��ַ
** �䡡��  : �Ƿ�Ϊ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _rngIsEmpty (VX_RING_ID  vxringid)
{
    return  (vxringid->VXRING_iToBuf == vxringid->VXRING_iFromBuf);
}
/*********************************************************************************************************
** ��������: _rngIsFull
** ��������: ���һ�� VxWorks ���� ring buffer �������Ƿ�����
** �䡡��  : ���������ƿ��ַ
** �䡡��  : �Ƿ�����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _rngIsFull (VX_RING_ID  vxringid)
{
    REGISTER INT    iNum = vxringid->VXRING_iToBuf - vxringid->VXRING_iFromBuf + 1;
    
    return  ((iNum == 0) || (iNum == vxringid->VXRING_iBufByteSize));
}
/*********************************************************************************************************
** ��������: _rngFreeBytes
** ��������: ���һ�� VxWorks ���� ring buffer �������Ŀ����ַ���
** �䡡��  : ���������ƿ��ַ
** �䡡��  : �ַ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _rngFreeBytes (VX_RING_ID  vxringid)
{
    REGISTER INT    iNum = vxringid->VXRING_iFromBuf - vxringid->VXRING_iToBuf - 1;
    
    if (iNum < 0) {
        iNum += vxringid->VXRING_iBufByteSize;
    }
    
    return  (iNum);
}
/*********************************************************************************************************
** ��������: _rngNBytes
** ��������: ���һ�� VxWorks ���� ring buffer ����������Ч�ַ���
** �䡡��  : ���������ƿ��ַ
** �䡡��  : �ַ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _rngNBytes (VX_RING_ID  vxringid)
{
    REGISTER INT    iNum = vxringid->VXRING_iToBuf - vxringid->VXRING_iFromBuf;
    
    if (iNum < 0) {
        iNum += vxringid->VXRING_iBufByteSize;
    }
    
    return  (iNum);
}
/*********************************************************************************************************
** ��������: _rngPutAhead
** ��������: ��һ���ֽ���Ϣ����һ�� VxWorks ���� ring buffer ��������ͷ��
** �䡡��  : 
**           vxringid               ���������ƿ��ַ
**           cByte                  �ַ�
**           iOffset                ƫ����
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _rngPutAhead (VX_RING_ID  vxringid,
                    CHAR        cByte,
                    INT         iOffset)
{
    REGISTER INT    iNum = vxringid->VXRING_iToBuf + iOffset;
    
    if (iNum >= vxringid->VXRING_iBufByteSize) {
        iNum -= vxringid->VXRING_iBufByteSize;
    }
    
    *(vxringid->VXRING_pcBuf + iNum) = cByte;
}
/*********************************************************************************************************
** ��������: _rngMoveAhead
** ��������: ��һ�� VxWorks ���� ring buffer ��������ָ����ǰ����
** �䡡��  : 
**           vxringid               ���������ƿ��ַ
**           iNum                   ������
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _rngMoveAhead (VX_RING_ID  vxringid, INT  iNum)
{
    iNum += vxringid->VXRING_iToBuf;
    
    if (iNum >= vxringid->VXRING_iBufByteSize) {
        iNum -= vxringid->VXRING_iBufByteSize;
    }
    
    vxringid->VXRING_iToBuf = iNum;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
