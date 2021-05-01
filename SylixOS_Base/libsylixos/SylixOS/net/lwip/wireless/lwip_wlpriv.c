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
** ��   ��   ��: lwip_wlpriv.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 06 �� 26 ��
**
** ��        ��: lwip ��������˽�нṹ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_WIRELESS_EN > 0)
#include "netdev.h"
#include "net/if.h"
#include "net/if_arp.h"
#include "net/if_wireless.h"
#include "net/if_whandler.h"
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
extern int call_commit_handler(struct netdev *dev);
/*********************************************************************************************************
  Size (in bytes) of the various private data types
*********************************************************************************************************/
static const char iw_priv_type_size[] = {
    0,                                                                  /* IW_PRIV_TYPE_NONE            */
    1,                                                                  /* IW_PRIV_TYPE_BYTE            */
    1,                                                                  /* IW_PRIV_TYPE_CHAR            */
    0,                                                                  /* Not defined                  */
    sizeof(u32),                                                        /* IW_PRIV_TYPE_INT             */
    sizeof(struct iw_freq),                                             /* IW_PRIV_TYPE_FLOAT           */
    sizeof(struct sockaddr),                                            /* IW_PRIV_TYPE_ADDR            */
    0,                                                                  /* Not defined                  */
};
/*********************************************************************************************************
** ��������: iw_handler_get_private
** ��������: ִ��˽�п��ƾ��
** �䡡��  : dev       ����ӿ�
** �䡡��  : OK or ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
int iw_handler_get_private (struct netdev           *dev,
                            struct iw_request_info  *info,
                            union iwreq_data        *wrqu,
                            char                    *extra)
{
    struct iw_handler_def *handlers = (struct iw_handler_def *)dev->wireless_handlers;
    
    if (!handlers) {
        return  (-EOPNOTSUPP);
    }
    
    /* Check if the driver has something to export */
    if ((handlers->num_private_args == 0) ||
        (handlers->private_args == NULL)) {
        return  (-EOPNOTSUPP);
    }

    /* Check if there is enough buffer up there */
    if (wrqu->data.length < handlers->num_private_args) {
        /* User space can't know in advance how large the buffer
         * needs to be. Give it a hint, so that we can support
         * any size buffer we want somewhat efficiently... */
        wrqu->data.length = handlers->num_private_args;
        return  (-E2BIG);
    }

    /* Set the number of available ioctls. */
    wrqu->data.length = handlers->num_private_args;

    /* Copy structure to the user buffer. */
    lib_memcpy(extra, handlers->private_args,
               sizeof(struct iw_priv_args) * wrqu->data.length);

    return  (0);
}
/*********************************************************************************************************
** ��������: get_priv_size
** ��������: ���˽�в�����С
** �䡡��  : 
** �䡡��  : OK or ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static int get_priv_size (u16 args)
{
    int    num  = args & IW_PRIV_SIZE_MASK;
    int    type = (args & IW_PRIV_TYPE_MASK) >> 12;

    return  (num * iw_priv_type_size[type]);
}
/*********************************************************************************************************
** ��������: adjust_priv_size
** ��������: ��׼��˽�в�����С
** �䡡��  : dev       ����ӿ�
** �䡡��  : OK or ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static int adjust_priv_size (u16 args, struct iw_point *iwp)
{
    int    num  = iwp->length;
    int    max  = args & IW_PRIV_SIZE_MASK;
    int    type = (args & IW_PRIV_TYPE_MASK) >> 12;

    /* Make sure the driver doesn't goof up */
    if (max < num) {
        num = max;
    }

    return  (num * iw_priv_type_size[type]);
}
/*********************************************************************************************************
** ��������: get_priv_descr_and_size
** ��������: Wrapper to call a private Wireless Extension handler.
** �䡡��  : 
** �䡡��  : OK or ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static int get_priv_descr_and_size (struct netdev *dev, unsigned int cmd,
                                    const struct iw_priv_args **descrp)
{
    const struct iw_priv_args *descr;
    int i, extra_size;
    struct iw_handler_def *handlers = (struct iw_handler_def *)dev->wireless_handlers;
    
    if (!handlers) {
        return  (-EOPNOTSUPP);
    }

    descr = NULL;
    for (i = 0; i < handlers->num_private_args; i++) {
        if (cmd == handlers->private_args[i].cmd) {
            descr = &handlers->private_args[i];
            break;
        }
    }

    extra_size = 0;
    if (descr) {
        if (IW_IS_SET(cmd)) {
            int    offset = 0;    /* For sub-ioctls */
            /* Check for sub-ioctl handler */
            if (descr->name[0] == '\0') {
                /* Reserve one int for sub-ioctl index */
                offset = sizeof(u32);
            }

            /* Size of set arguments */
            extra_size = get_priv_size(descr->set_args);

            /* Does it fits in iwr ? */
            if ((descr->set_args & IW_PRIV_SIZE_FIXED) &&
               ((extra_size + offset) <= IFNAMSIZ)) {
                extra_size = 0;
            }
        } else {
            /* Size of get arguments */
            extra_size = get_priv_size(descr->get_args);

            /* Does it fits in iwr ? */
            if ((descr->get_args & IW_PRIV_SIZE_FIXED) &&
               (extra_size <= IFNAMSIZ)) {
                extra_size = 0;
            }
        }
    }
    
    *descrp = descr;
    return  (extra_size);
}
/*********************************************************************************************************
** ��������: ioctl_private_iw_point
** ��������: ioctl priv
** �䡡��  : 
** �䡡��  : OK or ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static int ioctl_private_iw_point (struct iw_point *iwp, unsigned int cmd,
                                   const struct iw_priv_args *descr,
                                   iw_handler handler, struct netdev *dev,
                                   struct iw_request_info *info, int extra_size)
{
    char *extra;
    int err;

    /* Check what user space is giving us */
    if (IW_IS_SET(cmd)) {
        if (!iwp->pointer && iwp->length != 0) {
            return  (-EFAULT);
        }

        if (iwp->length > (descr->set_args & IW_PRIV_SIZE_MASK)) {
            return  (-E2BIG);
        }
    } else if (!iwp->pointer) {
        return  (-EFAULT);
    }

    extra = kzalloc(extra_size, GFP_KERNEL);
    if (!extra) {
        return -ENOMEM;
    }

    /* If it is a SET, get all the extra data in here */
    if (IW_IS_SET(cmd) && (iwp->length != 0)) {
        if (copy_from_user(extra, iwp->pointer, extra_size)) {
            err = -EFAULT;
            goto    out;
        }
    }

    /* Call the handler */
    err = handler(dev, info, (union iwreq_data *) iwp, extra);

    /* If we have something to return to the user */
    if (!err && IW_IS_GET(cmd)) {
        /* Adjust for the actual length if it's variable,
         * avoid leaking kernel bits outside.
         */
        if (!(descr->get_args & IW_PRIV_SIZE_FIXED)) {
            extra_size = adjust_priv_size(descr->get_args, iwp);
        }

        if (copy_to_user(iwp->pointer, extra, extra_size)) {
            err = -EFAULT;
        }
    }

out:
    kfree(extra);
    return  (err);
}
/*********************************************************************************************************
** ��������: ioctl_private_iw_point
** ��������: ioctl priv call
** �䡡��  : 
** �䡡��  : OK or ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
int ioctl_private_call (struct netdev *dev, struct iwreq *iwr,
                        unsigned int cmd, struct iw_request_info *info,
                        iw_handler handler)
{
    int extra_size = 0, ret = -EINVAL;
    const struct iw_priv_args *descr = NULL;

    extra_size = get_priv_descr_and_size(dev, cmd, &descr);

    /* Check if we have a pointer to user space data or not. */
    if (extra_size == 0) {
        /* No extra arguments. Trivial to handle */
        ret = handler(dev, info, &(iwr->u), (char *) &(iwr->u));
    } else {
        ret = ioctl_private_iw_point(&iwr->u.data, cmd, descr,
                         handler, dev, info, extra_size);
    }

    /* Call commit handler if needed and defined */
    if (ret == -EIWCOMMIT) {
        ret = call_commit_handler(dev);
    }

    return  (ret);
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_WIRELESS_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
