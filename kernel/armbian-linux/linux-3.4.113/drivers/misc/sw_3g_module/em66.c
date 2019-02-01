/*
 * SoftWinners Oviphone 2G module
 *
 * Copyright (C) 2012 SoftWinners Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

/*
*******************************************************************************
*
*                                   ŷ�� em66
*
*
* ע: 1��������Ϊģ����ڲ����ű仯.
*     2��������Ҫע���ģ������ʱ�Ƿ����������.������������ܣ������еĲ���
*        ˳��Ӧ�ú�ģ������Ų���˳���෴.
*
*
* ģ���ڲ�Ĭ��״̬:
* vbat  : ��
* power : ��
* reset : ��
* sleep : ��
*
* ����:
* (1)��sleep����
* (2)��vbat����
* (3)����ʱ1.2s
* (4)��power����
* (5)����ʱ2s
* (6)��power����
*
* �ػ�:
* (1)����λ�ػ���ִ�и�λ����
*
* ��λ:
* (1)��reset����
* (2)����ʱ60ms
* (3)��reset����
* (4)����ʱ10ms
*
* ����:
* (1)��sleep����
*
* ����:
* (1)��sleep����
*
*******************************************************************************
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/kmemcheck.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/idr.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/signal.h>

#include <linux/time.h>
#include <linux/timer.h>

#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>

#include "sw_module.h"

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#define DRIVER_DESC             SW_DRIVER_NAME
#define DRIVER_VERSION          "1.0"
#define DRIVER_AUTHOR			"Javen Xu"

#define MODEM_NAME              "em66"

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static struct sw_modem g_em66;
static char g_em66_name[] = MODEM_NAME;

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void em66_reset(struct sw_modem *modem)
{
    modem_dbg("reset %s modem\n", modem->name);

	modem_reset(modem, 1);
    sw_module_mdelay(100);
	modem_reset(modem, 0);
	sw_module_mdelay(10);

    return;
}

/*
*******************************************************************************
*
* (1)��wankeup : ���߳���100����, ���׳���100����, �����߳���100����, ������
* (2)��sleep   : ���߳���100����, ���׳���100����, �����߳���100����, ������
*
*******************************************************************************
*/
static void em66_sleep(struct sw_modem *modem, u32 sleep)
{
    modem_dbg("%s modem %s\n", modem->name, (sleep ? "sleep" : "wakeup"));

    if(sleep){
        modem_sleep(modem, 1);
    }else{
        modem_sleep(modem, 0);
    }

    return;
}

static void em66_rf_disable(struct sw_modem *modem, u32 disable)
{
    modem_dbg("set %s modem rf %s\n", modem->name, (disable ? "disable" : "enable"));

    modem_rf_disable(modem, disable);

    return;
}

/*
*******************************************************************************
*
* ��������:
* (1)��ִ�йػ�����
* (2)����GPIOĬ��״̬
* (3)��ִ�п������� (ע��)
*   (3-1)��power����
*   (3-2)����ʱ2s
*   (3-3)��power����
*
* �ػ�����:
* (1)��vbat����
*
*******************************************************************************
*/
void em66_power(struct sw_modem *modem, u32 on)
{
    modem_dbg("set %s modem power %s\n", modem->name, (on ? "on" : "off"));

    if(on){
		modem_dldo_on_off(modem, 1);

        /* power off, Prevent abnormalities restart of the PAD. */
        //�����غ�ģ����ֱ����Ҫִ��һ�ιػ�������Ȼ����ִ�п�������
		em66_reset(modem);
		modem_vbat(modem, 1);
        /* default */
        modem_sleep(modem, 1);

        /* power on */
        modem_power_on_off(modem, 1);
        sw_module_mdelay(2000);
        modem_power_on_off(modem, 0);
    }else{

		modem_vbat(modem, 0);
		//em66_reset(modem);

		modem_dldo_on_off(modem, 0);
    }
    return;
}

static int em66_start(struct sw_modem *mdev)
{
    int ret = 0;

    if(!mdev->start){
        ret = modem_irq_init(mdev, IRQF_TRIGGER_RISING);
        if(ret != 0){
           modem_err("err: sw_module_irq_init failed\n");
           return -1;
        }

        em66_power(mdev, 1);
        mdev->start = 1;
    }

    return 0;
}

static int em66_stop(struct sw_modem *mdev)
{
    if(mdev->start){
        em66_power(mdev, 0);
        modem_irq_exit(mdev);
        mdev->start = 0;
    }

    return 0;
}

static int em66_suspend(struct sw_modem *mdev)
{
    em66_sleep(mdev, 1);

    return 0;
}

static int em66_resume(struct sw_modem *mdev)
{
    em66_sleep(mdev, 0);

    return 0;
}

static struct sw_modem_ops em66_ops = {
	.power          = em66_power,
	.reset          = em66_reset,
	.sleep          = em66_sleep,
	.rf_disable     = em66_rf_disable,

	.start          = em66_start,
	.stop           = em66_stop,

	.early_suspend  = modem_early_suspend,
	.early_resume   = modem_early_resume,

	.suspend        = em66_suspend,
	.resume         = em66_resume,
};

static struct platform_device em66_device = {
	.name				= SW_DRIVER_NAME,
	.id					= -1,

	.dev = {
		.platform_data  = &g_em66,
	},
};

static int __init em66_init(void)
{
    int ret = 0;

    memset(&g_em66, 0, sizeof(struct sw_modem));

    /* gpio */
    ret = modem_get_config(&g_em66);
    if(ret != 0){
        modem_err("err: em66_get_config failed\n");
        goto get_config_failed;
    }

    if(g_em66.used == 0){
        modem_err("em66 is not used\n");
        goto get_config_failed;
    }

    ret = modem_pin_init(&g_em66);
    if(ret != 0){
       modem_err("err: em66_pin_init failed\n");
       goto pin_init_failed;
    }

    /* ��ֹ�ű���ģ������bb_name���������Ʋ�һ�£����ֻʹ���������� */
//    if(g_em66.name[0] == 0){
        strcpy(g_em66.name, g_em66_name);
//    }
    g_em66.ops = &em66_ops;

    modem_dbg("%s modem init\n", g_em66.name);

    platform_device_register(&em66_device);

	return 0;
pin_init_failed:

get_config_failed:

    modem_dbg("%s modem init failed\n", g_em66.name);

	return -1;
}

static void __exit em66_exit(void)
{
    platform_device_unregister(&em66_device);
}

//late_initcall(em66_init);
module_init(em66_init);
module_exit(em66_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(MODEM_NAME);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");


