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
** ��   ��   ��: module.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 03 �� 02 ��
**
** ��        ��: �ں�ģ��װ������ز����� api.
*********************************************************************************************************/

#ifndef __MODULE_H
#define __MODULE_H

#ifdef __GNUC__
#if __GNUC__ < 2  || (__GNUC__ == 2 && __GNUC_MINOR__ < 5)
#ifndef __attribute__
#define __attribute__(args)
#endif                                                                  /*  __attribute__               */
#endif                                                                  /*  __GNUC__ < 2  || ...        */
#endif                                                                  /*  __GNUC__                    */

/*********************************************************************************************************
  �ں�ģ����ŵ���
*********************************************************************************************************/

#define LW_SYMBOL_KERNEL_SECTION        ".__sylixos_kernel"

/*********************************************************************************************************
  �ں�ģ��������������
  
  ����: 
        LW_SYMBOL_EXPORT int foo ()  -> �˺���װ�ص��ں�ģ��, ���������ں˿ɼ�
        {
            ...;
        }
        
        int foo ()                   -> �˺��������ں��ڲ��ɼ�. �統����̬��ʹ��, ���ſɼ�.
        {
            ...;
        }
        
        LW_SYMBOL_LOCAL int foo ()   -> �˺��������ں��ڲ��ɼ�, �統����̬��ʹ��, ����Ҳ���ɼ�.
        {
            ...;
        }
*********************************************************************************************************/

#define LW_SYMBOL_EXPORT                __attribute__((section(LW_SYMBOL_KERNEL_SECTION)))
#define LW_SYMBOL_LOCAL                 __attribute__((visibility("hidden")))

/*********************************************************************************************************
  �ں�ģ���ʼ��������ж�غ��� (�����ں�ģ�����ںͳ��ں���, ÿ��ģ�鶼Ӧ�þ���)
  ע��, ������������ģ��Դ�ļ��в��ܼ� LW_SYMBOL_EXPORT �� LW_SYMBOL_LOCAL
*********************************************************************************************************/

#define LW_MODULE_INIT                  "module_init"
#define LW_MODULE_EXIT                  "module_exit"

/*********************************************************************************************************
  �ں�ģ�� module_init() ��������ֵ
*********************************************************************************************************/

#define LW_INIT_RET_UNLOAD_DISALLOW     1
#define LW_INIT_RET_OK                  0
#define LW_INIT_RET_ERROR               -1

/*********************************************************************************************************
  �ں�ģ����븲���ʷ�����������, �ں�ģ����Ҫ����ʵ�����º������Ҽ��� LW_SYMBOL_EXPORT �����������.
  
  LW_SYMBOL_EXPORT  void  module_gcov (void)
  {
      __gcov_flush();
  }
*********************************************************************************************************/

#define LW_MODULE_GCOV                  "module_gcov"

/*********************************************************************************************************
  DSP C6x ���ⶨ��

  ע��: DSP CCS �������ĵ�ָ��: ��ģ�麯������ʹ�� static ����, ��Ϊ static ���Ͳ������� B14 �Ĵ���, ����
        ���ô���. ����һЩ��ģ��ʹ�õĺ���, ���������ʱ�����ڵĸ��ֻص�, �߳����к�����, �Բ����� static.

        DSP ��������������������������Ϊ static,
        ���Կ�Ĺ�����������������ʹ�����¶���:

        LW_CONSTRUCTOR_BEGIN
        LW_LIB_HOOK_STATIC void con_func (void)
        {
            ...;
        }
        LW_CONSTRUCTOR_END(con_func)

        LW_DESTRUCTOR_BEGIN
        LW_LIB_HOOK_STATIC void des_func (void)
        {
            ...;
        }
        LW_DESTRUCTOR_END(des_func)
*********************************************************************************************************/

#ifdef LW_CFG_CPU_ARCH_C6X
#define LW_LIB_HOOK_STATIC

#define constructor     "Warning: you must use LW_CONSTRUCTOR_BEGIN & LW_CONSTRUCTOR_END"
#define destructor      "Warning: you must use LW_DESTRUCTOR_BEGIN & LW_DESTRUCTOR_END"

#define LW_CONSTRUCTOR_BEGIN
#define LW_CONSTRUCTOR_END(f)   \
        __attribute__((section(".init_array"))) void *__$$_c6x_dsp_lib_##f##_ctor = (void *)f;

#define LW_DESTRUCTOR_BEGIN
#define LW_DESTRUCTOR_END(f)    \
        __attribute__((section(".fini_array"))) void *__$$_c6x_dsp_lib_##f##_dtor = (void *)f;

#else
#define LW_LIB_HOOK_STATIC  static

#define LW_CONSTRUCTOR_BEGIN    \
        __attribute__((constructor))
#define LW_CONSTRUCTOR_END(f)

#define LW_DESTRUCTOR_BEGIN     \
        __attribute__((destructor))
#define LW_DESTRUCTOR_END(f)
#endif

/*********************************************************************************************************
  ע��ͽ��ע���ں�ģ��
*********************************************************************************************************/

#define API_ModuleRegister(pcFile)                      \
        API_ModuleLoadEx(pcFile,                        \
                         LW_OPTION_LOADER_SYM_GLOBAL,   \
                         LW_MODULE_INIT,                \
                         LW_MODULE_EXIT,                \
                         LW_NULL,                       \
                         LW_SYMBOL_KERNEL_SECTION,      \
                         LW_NULL)
                         
#define API_ModuleUnregister(pvModule)                  \
        API_ModuleUnload(pvModule)
        
#define moduleRegister                  API_ModuleRegister
#define moduleUnregister                API_ModuleUnregister

#endif                                                                  /*  __MODULE_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
