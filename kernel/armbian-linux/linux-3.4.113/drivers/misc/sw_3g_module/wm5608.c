/*
 * SoftWinners longcheer wm5608 3G module
 *
 * Copyright (C) 2012 SoftWinners Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
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

#include "sw_module.h"

/*
*******************************************************************************
*
*                                   ���� wm5608
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
* (1)��power����
* (2)����ʱ200ms
* (3)��power����
*
* �ػ�:
* (1)��power����
* (2)����ʱ3.5s
* (3)��power����
*
* ��λ:
* (1)��reset����
* (2)����ʱ150ms
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

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#define DRIVER_DESC             SW_DRIVER_NAME
#define DRIVER_VERSION          "1.0"
#define DRIVER_AUTHOR			"Javen Xu"

#define MODEM_NAME             "wm5608"

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static struct sw_modem g_wm5608;
static char g_wm5608_name[] = MODEM_NAME;

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void wm5608_reset(struct sw_modem *modem)
{
    modem_dbg("reset %s modem\n", modem->name);

    modem_reset(modem, 0);
    sw_module_mdelay(150);
    modem_reset(modem, 1);
    sw_module_mdelay(10);

    return;
}

static void wm5608_sleep(struct sw_modem *modem, u32 sleep)
{
    modem_dbg("%s modem %s\n", modem->name, (sleep ? "sleep" : "wakeup"));

    if(sleep){
        modem_sleep(modem, 1);
    }else{
        modem_sleep(modem, 0);
    }

    return;
}

static void wm5608_rf_disable(struct sw_modem *modem, u32 disable)
{
    modem_dbg("set %s modem rf %s\n", modem->name, (disable ? "disable" : "enable"));

    return;
}

void wm5608_power(struct sw_modem *modem, u32 on)
{
    modem_dbg("set %s modem power %s\n", modem->name, (on ? "on" : "off"));

    if(on){
        /* default */
    	modem_reset(modem, 1);
    	modem_power_on_off(modem, 1);
    	modem_sleep(modem, 0);
		msleep(10);

        /* power off, Prevent abnormalities restart of the PAD. */
        //�����غ�ģ����ֱ����Ҫִ��һ�ιػ�������Ȼ����ִ�п�������

    	/* power on */
        modem_power_on_off(modem, 0);
        sw_module_mdelay(200);
        modem_power_on_off(modem, 1);
    }else{
        modem_power_on_off(modem, 0);
        sw_module_mdelay(3500);
        modem_power_on_off(modem, 1);
    }

    return;
}

static int wm5608_start(struct sw_modem *mdev)
{
    int ret = 0;

    ret = modem_irq_init(mdev, IRQF_TRIGGER_FALLING);
    if(ret != 0){
       modem_err("err: sw_module_irq_init failed\n");
       return -1;
    }

    wm5608_power(mdev, 1);

    return 0;
}

static int wm5608_stop(struct sw_modem *mdev)
{
    wm5608_power(mdev, 0);
    modem_irq_exit(mdev);

    return 0;
}

static int wm5608_suspend(struct sw_modem *mdev)
{
    wm5608_sleep(mdev, 1);

    return 0;
}

static int wm5608_resume(struct sw_modem *mdev)
{
    wm5608_sleep(mdev, 0);

    return 0;
}

static struct sw_modem_ops wm5608_ops = {
	.power          = wm5608_power,
	.reset          = wm5608_reset,
	.sleep          = wm5608_sleep,
	.rf_disable     = wm5608_rf_disable,

	.start          = wm5608_start,
	.stop           = wm5608_stop,

	.early_suspend  = modem_early_suspend,
	.early_resume   = modem_early_resume,

	.suspend        = wm5608_suspend,
	.resume         = wm5608_resume,
};

static struct platform_device wm5608_device = {
	.name				= SW_DRIVER_NAME,
	.id					= -1,

	.dev = {
		.platform_data  = &g_wm5608,
	},
};

static int __init wm5608_init(void)
{
    int ret = 0;

    memset(&g_wm5608, 0, sizeof(struct sw_modem));

    /* gpio */
    ret = modem_get_config(&g_wm5608);
    if(ret != 0){
        modem_err("err: wm5608_get_config failed\n");
        goto get_config_failed;
    }

    if(g_wm5608.used == 0){
        modem_err("wm5608 is not used\n");
        goto get_config_failed;
    }

    ret = modem_pin_init(&g_wm5608);
    if(ret != 0){
       modem_err("err: wm5608_pin_init failed\n");
       goto pin_init_failed;
    }

    /* ��ֹ�ű���ģ������bb_name���������Ʋ�һ�£����ֻʹ���������� */
//    if(g_wm5608.name[0] == 0){
        strcpy(g_wm5608.name, g_wm5608_name);
//    }
    g_wm5608.ops = &wm5608_ops;

    modem_dbg("%s modem init\n", g_wm5608.name);

    platform_device_register(&wm5608_device);

	return 0;
pin_init_failed:

get_config_failed:

    modem_dbg("%s modem init failed\n", g_wm5608.name);

	return -1;
}

static void __exit wm5608_exit(void)
{
    platform_device_unregister(&wm5608_device);
}

late_initcall(wm5608_init);
module_exit(wm5608_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(MODEM_NAME);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");




