/* linux/drivers/video/sunxi/disp/dev_capture.c
 *
 * Copyright (c) 2013 Allwinnertech Co., Ltd.
 * Author: Tyle <tyle@allwinnertech.com>
 *
 * Display capture module for sunxi platform
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "dev_disp.h"

static struct cdev *my_cdev;
static dev_t devid ;
static struct class *capture_class;
typedef struct
{
	struct device        *dev;
	ktime_t              capture_timestamp[3];
	struct work_struct   capture_work[3];
}capture_info_t;

static capture_info_t capture_info;

static void capture_work_0(struct work_struct *work)
{
	char buf[64];
	char *envp[2];
	int ret;

	snprintf(buf, sizeof(buf), "CAPTURE0=BUFFER%d,TIME=%llu",bsp_disp_capture_screen_get_buffer_id(0), ktime_to_ns(capture_info.capture_timestamp[0]));
	envp[0] = buf;
	envp[1] = NULL;
	ret = kobject_uevent_env(&capture_info.dev->kobj, KOBJ_CHANGE, envp);
}

static void capture_work_1(struct work_struct *work)
{
	char buf[64];
	char *envp[2];

	snprintf(buf, sizeof(buf), "CAPTURE1=BUFFER%d,TIME=%llu",bsp_disp_capture_screen_get_buffer_id(1), ktime_to_ns(capture_info.capture_timestamp[1]));
	envp[0] = buf;
	envp[1] = NULL;
	kobject_uevent_env(&capture_info.dev->kobj, KOBJ_CHANGE, envp);
}

__s32 capture_event(__u32 sel)
{
	capture_info.capture_timestamp[sel] = ktime_get();

	schedule_work(&capture_info.capture_work[sel]);

	return 0;
}

static int __devinit capture_probe(struct platform_device *pdev)
{
	pr_info("[DISP]capture_probe\n");

	memset(&capture_info, 0, sizeof(capture_info_t));
	capture_info.dev = &pdev->dev;
	INIT_WORK(&capture_info.capture_work[0], capture_work_0);
	INIT_WORK(&capture_info.capture_work[1], capture_work_1);

	return 0;
}

static struct platform_driver capture_driver = {
	.probe    = capture_probe,
	.driver   =
	{
		.name   = "capture",
		.owner  = THIS_MODULE,
	},
};


static struct platform_device capture_device = {
	.name           = "capture",
	.id             = -1,
	.dev            = {}
};

int capture_module_init(void)
{
	int ret = 0, err;

	pr_info("[DISP]capture_module_init\n");

	alloc_chrdev_region(&devid, 0, 1, "capture");
	my_cdev = cdev_alloc();
	my_cdev->owner = THIS_MODULE;
	err = cdev_add(my_cdev, devid, 1);
	if (err) {
		__wrn("cdev_add fail\n");
		return -1;
	}

	capture_class = class_create(THIS_MODULE, "capture");
	if (IS_ERR(capture_class))	{
		__wrn("class_create fail\n");
		return -1;
	}

	device_create(capture_class, NULL, devid, NULL, "capture");

	ret = platform_device_register(&capture_device);

	if (ret == 0) {
		ret = platform_driver_register(&capture_driver);
	}

	pr_info("[DISP]==capture finish==\n");

	return ret;
}

void  capture_module_exit(void)
{
	__inf("capture_module_exit\n");

	platform_driver_unregister(&capture_driver);
	platform_device_unregister(&capture_device);

	device_destroy(capture_class,  devid);
	class_destroy(capture_class);

	cdev_del(my_cdev);
}



