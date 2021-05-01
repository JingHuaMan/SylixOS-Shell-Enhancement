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
** ��   ��   ��: tpsfs.h
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2015 �� 9 �� 21 ��
**
** ��        ��: tpsfs API ʵ��

** BUG:
*********************************************************************************************************/

#ifndef __TSPSFS_H
#define __TSPSFS_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0

/*********************************************************************************************************
  Ŀ¼�ṹ
*********************************************************************************************************/

typedef struct {
    PTPS_INODE          DIR_pinode;                                     /* ��ǰĿ¼                     */
    PTPS_ENTRY          DIR_pentry;                                     /* ��ǰentry                    */
    TPS_OFF_T           DIR_offPos;                                     /* ��ǰentry��Ŀ¼�е�ƫ��      */
} TPS_DIR;
typedef TPS_DIR        *PTPS_DIR;

/*********************************************************************************************************
  API
*********************************************************************************************************/
#ifdef __cplusplus 
extern "C" { 
#endif 
                                                                        /* ���ļ�                     */
errno_t     tpsFsOpen(PTPS_SUPER_BLOCK psb, CPCHAR pcPath, INT iFlags,
                      INT iMode, PCHAR *ppcRemain, PTPS_INODE *ppinode);
                                                                        /* �ر��ļ�                     */
errno_t     tpsFsClose(PTPS_INODE pinode);
                                                                        /* �ύ�ļ�ͷ�����޸�           */
errno_t     tpsFsFlushHead(PTPS_INODE pinode);
                                                                        /* ɾ��entry                    */
errno_t     tpsFsRemove(PTPS_SUPER_BLOCK psb, CPCHAR pcPath);
                                                                        /* �ƶ�entry                    */
errno_t     tpsFsMove(PTPS_SUPER_BLOCK psb, CPCHAR pcPathSrc, CPCHAR pcPathDst);
                                                                        /* ����Ӳ����                   */
errno_t     tpsFsLink(PTPS_SUPER_BLOCK psb, CPCHAR pcPathSrc, CPCHAR pcPathDst);
                                                                        /* ���ļ�                       */
errno_t     tpsFsRead(PTPS_INODE pinode, PUCHAR pucBuff, TPS_OFF_T off,
                      TPS_SIZE_T szLen, TPS_SIZE_T *pszRet);
                                                                        /* д�ļ�                       */
errno_t     tpsFsWrite(PTPS_INODE pinode, PUCHAR pucBuff, TPS_OFF_T off,
                       TPS_SIZE_T szLen, TPS_SIZE_T *pszRet);
                                                                        /* �ض��ļ�                     */
errno_t     tpsFsTrunc(PTPS_INODE pinode, TPS_SIZE_T szNewSize);
                                                                        /* ����Ŀ¼                     */
errno_t     tpsFsMkDir(PTPS_SUPER_BLOCK psb, CPCHAR pcPath, INT iFlags, INT iMode);
                                                                        /* ��Ŀ¼                     */
errno_t     tpsFsOpenDir(PTPS_SUPER_BLOCK psb, CPCHAR pcPath, PTPS_DIR *ppdir);
                                                                        /* �ر�Ŀ¼                     */
errno_t     tpsFsCloseDir(PTPS_DIR pdir);
                                                                        /* ��ȡĿ¼                     */
errno_t     tpsFsReadDir(PTPS_INODE pinodeDir, BOOL bInHash, TPS_OFF_T off, PTPS_ENTRY *ppentry);
                                                                        /* ͬ���ļ�                     */
errno_t     tpsFsSync(PTPS_INODE pinode);
                                                                        /* ͬ�������ļ�ϵͳ����         */
errno_t     tpsFsVolSync (PTPS_SUPER_BLOCK psb);
                                                                        /* tpsfs ����ļ� stat          */
VOID        tpsFsStat(PVOID  pvDevHdr, PTPS_SUPER_BLOCK  psb, PTPS_INODE  pinode, struct stat *pstat);
                                                                        /* tpsfs ����ļ�ϵͳ statfs    */
VOID        tpsFsStatfs(PTPS_SUPER_BLOCK  psb, struct statfs *pstatfs);
                                                                        /* ��ȡ�ļ���С                 */
TPS_SIZE_T  tpsFsGetSize(PTPS_INODE pinode);
                                                                        /* ��ȡ�ļ�ģʽ                 */
errno_t     tpsFsGetmod(PTPS_INODE pinode);
                                                                        /* �޸��ļ�ģʽ                 */
errno_t     tpsFsChmod(PTPS_INODE pinode, INT iMode);
                                                                        /* �޸��ļ�������               */
errno_t     tpsFsChown(PTPS_INODE pinode, uid_t uid, gid_t gid);
                                                                        /* �޸��ļ�ʱ��                 */
errno_t     tpsFsChtime(PTPS_INODE pinode, struct utimbuf  *utim);
                                                                        /* ��д��inode�ڵ�              */
VOID        tpsFsFlushInodes(PTPS_SUPER_BLOCK psb);

#ifdef __cplusplus 
}
#endif 

#endif                                                                  /* LW_CFG_TPSFS_EN > 0          */
#endif                                                                  /* __TSPSFS_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
