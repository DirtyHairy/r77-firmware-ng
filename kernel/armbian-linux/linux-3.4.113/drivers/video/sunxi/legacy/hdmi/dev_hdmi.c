/* linux/drivers/video/sunxi/hdmi/dev_hdmi.c
 *
 * Copyright (c) 2013 Allwinnertech Co., Ltd.
 * Author: Tyle <tyle@allwinnertech.com>
 *
 * Hdmi driver for sunxi platform
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "dev_hdmi.h"
#include "drv_hdmi_i.h"


static struct cdev *my_cdev;
static dev_t devid ;
static struct class *hdmi_class;

hdmi_info_t ghdmi;


static struct resource hdmi_resource[1] =
{
	[0] = {
		.start = 0x01c16000,
		.end   = 0x01c165ff,
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device hdmi_device =
{
	.name           = "hdmi",
	.id		        = -1,
	.num_resources  = ARRAY_SIZE(hdmi_resource),
	.resource	    = hdmi_resource,
	.dev            = {}
};

static ssize_t hdmi_debug_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "debug=%s\n", hdmi_print?"on" : "off");
}

static ssize_t hdmi_debug_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	if (count < 1)
		return -EINVAL;

	if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0) {
		hdmi_print = 1;
	} else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0) {
		hdmi_print = 0;
	} else {
		return -EINVAL;
	}

	return count;
}

static DEVICE_ATTR(debug, S_IRUGO|S_IWUSR|S_IWGRP,
		hdmi_debug_show, hdmi_debug_store);

static ssize_t hdmi_rgb_only_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "rgb_only=%s\n", rgb_only?"on" : "off");
}

static ssize_t hdmi_rgb_only_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	if (count < 1)
		return -EINVAL;

	if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0) {
		rgb_only = 1;
	} else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0) {
		rgb_only = 0;
	} else {
		return -EINVAL;
	}

	return count;
}

static DEVICE_ATTR(rgb_only, S_IRUGO|S_IWUSR|S_IWGRP,
		hdmi_rgb_only_show, hdmi_rgb_only_store);


__s32 hdmi_hpd_state(__u32 state);
static ssize_t hdmi_state_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "nothing\n");
}

static ssize_t hdmi_state_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	if (count < 1)
		return -EINVAL;

	if (strnicmp(buf, "1", 1) == 0) {
		hdmi_hpd_state(1);
	} else {
		hdmi_hpd_state(0);
	}

	return count;
}

static DEVICE_ATTR(state, S_IRUGO|S_IWUSR|S_IWGRP,
		hdmi_state_show, hdmi_state_store);


static int __devinit hdmi_probe(struct platform_device *pdev)
{
	pr_info("[HDMI]==hdmi_probe call==\n");

	memset(&ghdmi, 0, sizeof(hdmi_info_t));

	ghdmi.base_hdmi = 0xf1c16000;
	ghdmi.dev = &pdev->dev;

	Hdmi_init();

	pr_info("[HDMI]==hdmi_probe finish==\n");

	return 0;
}


static int hdmi_remove(struct platform_device *pdev)
{
	__inf("hdmi_remove call\n");

	Hdmi_exit();

	return 0;
}

static int hdmi_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int hdmi_resume(struct platform_device *pdev)
{
	return 0;
}
static struct platform_driver hdmi_driver =
{
	.probe		= hdmi_probe,
	.remove		= hdmi_remove,
	.suspend    = hdmi_suspend,
	.resume    = hdmi_resume,
	.driver		=
	{
		.name	= "hdmi",
		.owner	= THIS_MODULE,
	},
};

int hdmi_open(struct inode *inode, struct file *file)
{
	return 0;
}

int hdmi_release(struct inode *inode, struct file *file)
{
	return 0;
}


ssize_t hdmi_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	return -EINVAL;
}

ssize_t hdmi_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	return -EINVAL;
}

int hdmi_mmap(struct file *file, struct vm_area_struct * vma)
{
	return 0;
}

long hdmi_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return 0;
}


static const struct file_operations hdmi_fops = {
	.owner		= THIS_MODULE,
	.open		= hdmi_open,
	.release    = hdmi_release,
	.write      = hdmi_write,
	.read		= hdmi_read,
	.unlocked_ioctl	= hdmi_ioctl,
	.mmap       = hdmi_mmap,
};

static struct attribute *hdmi_attributes[] = {
	&dev_attr_debug.attr,
	&dev_attr_rgb_only.attr,
	&dev_attr_state.attr,
	NULL
};

static struct attribute_group hdmi_attribute_group = {
	.name = "attr",
	.attrs = hdmi_attributes
};

static int __init hdmi_module_init(void)
{
	int ret = 0, err;

	pr_info("[HDMI]==hdmi_module_init==\n");

	alloc_chrdev_region(&devid, 0, 1, "hdmi");
	my_cdev = cdev_alloc();
	cdev_init(my_cdev, &hdmi_fops);
	my_cdev->owner = THIS_MODULE;
	err = cdev_add(my_cdev, devid, 1);
	if (err) {
		__wrn("cdev_add fail.\n");
		return -1;
	}

	hdmi_class = class_create(THIS_MODULE, "hdmi");
	if (IS_ERR(hdmi_class)) {
		__wrn("class_create fail\n");
		return -1;
	}

	ghdmi.dev = device_create(hdmi_class, NULL, devid, NULL, "hdmi");

	ret = sysfs_create_group(&ghdmi.dev->kobj,
	               &hdmi_attribute_group);

	ret |= hdmi_i2c_add_driver();

	ret = platform_device_register(&hdmi_device);

	if (ret == 0) {
		ret = platform_driver_register(&hdmi_driver);
	}

	pr_info("[HDMI]==hdmi_module_init finish==\n");

	return ret;
}

static void __exit hdmi_module_exit(void)
{
	__inf("hdmi_module_exit\n");

	platform_driver_unregister(&hdmi_driver);
	platform_device_unregister(&hdmi_device);

	hdmi_i2c_del_driver();

	class_destroy(hdmi_class);

	cdev_del(my_cdev);
}


late_initcall(hdmi_module_init);
module_exit(hdmi_module_exit);

MODULE_AUTHOR("tyle");
MODULE_DESCRIPTION("hdmi driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:hdmi");

