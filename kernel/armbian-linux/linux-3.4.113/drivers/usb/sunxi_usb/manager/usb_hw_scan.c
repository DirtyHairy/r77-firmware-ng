/*
 * drivers/usb/sunxi_usb/manager/usb_hw_scan.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * javen, 2011-4-14, create this file
 *
 * usb detect module.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <mach/irqs.h>
#include <linux/gpio.h>
#include <mach/platform.h>

#include  "../include/sunxi_usb_config.h"
#include  "usb_manager.h"
#include  "usb_hw_scan.h"
#include  "usb_msg_center.h"

static struct usb_scan_info g_usb_scan_info;
#if defined(CONFIG_AW_AXP)
extern int axp_usb_det(void);
#endif
#ifdef CONFIG_USB_SUNXI_USB0_OTG
extern atomic_t thread_suspend_flag;
#endif
int device_insmod_delay = 0;
static void (*__usb_hw_scan) (struct usb_scan_info *);

#ifndef  SUNXI_USB_FPGA

static __u32 get_pin_data(struct usb_gpio *usb_gpio)
{
	return __gpio_get_value(usb_gpio->gpio_set.gpio.gpio);
}
static int get_usb_gadget_functions(void)
{
	struct file *filep;
	loff_t pos;
	char buf[32] = {0};

	filep=filp_open("/sys/class/android_usb/android0/functions", O_RDONLY, 0);
	if (IS_ERR(filep)) {
		return 0;
	}

	pos = 0;
	vfs_read(filep, (char __user *)buf, 32, &pos);
	filp_close(filep, NULL);

	if (strlen(buf) == 0){
		return 0;
	}else{
		return 1;
	}
}
/*
 * filter the PIO burr
 * @usb_gpio: .
 * @value:    store the value to be read
 *
 * try 10 times, if value is the same, then consider no shake. return any one.
 * if not the same, then the current read is invalid
 *
 * return: 0 - the valude is the same, valid, 1 - the value change, invalid.
 */
static __u32 PIODataIn_debounce(struct usb_gpio *usb_gpio, __u32 *value)
{
	__u32 retry  = 0;
	__u32 time   = 10;
	__u32 temp1  = 0;
	__u32 cnt    = 0;
	__u32 change = 0;	/* if have shake */

	/* try 10 times, if value is the same, then current read is valid; otherwise invalid */
	if (usb_gpio->valid) {
		retry = time;
		while(retry--) {
			temp1 = get_pin_data(usb_gpio);
			if (temp1) {
				cnt++;
			}
		}

		/* 10 times, the value is all 0 or 1 */
		if ((cnt == time)||(cnt == 0)) {
			change = 0;
		} else {
			change = 1;
		}
	} else {
		change = 1;
	}

	if (!change) {
		*value = temp1;
	}

	DMSG_DBG_MANAGER("usb_gpio->valid = %x, cnt = %x, change= %d, temp1 = %x\n",
		usb_gpio->valid, cnt, change, temp1);

	return change;
}

static u32 get_id_state(struct usb_scan_info *info)
{
	enum usb_id_state id_state = USB_DEVICE_MODE;
	__u32 pin_data = 0;

	if (info->cfg->port[0].id.valid) {
		if (!PIODataIn_debounce(&info->cfg->port[0].id, &pin_data)) {
			if (pin_data) {
				id_state = USB_DEVICE_MODE;
			} else {
				id_state = USB_HOST_MODE;
			}

			info->id_old_state = id_state;
		} else {
			id_state = info->id_old_state;
		}
	}

	return id_state;
}

static u32 get_detect_vbus_state(struct usb_scan_info *info)
{
	enum usb_det_vbus_state det_vbus_state = USB_DET_VBUS_INVALID;
	__u32 pin_data = 0;

	if (info->cfg->port[0].det_vbus_type == USB_DET_VBUS_TYPE_GIPO) {
		if (info->cfg->port[0].det_vbus.valid) {
			if (!PIODataIn_debounce(&info->cfg->port[0].det_vbus, &pin_data)) {
				if (pin_data) {
					det_vbus_state = USB_DET_VBUS_VALID;
				} else {
					det_vbus_state = USB_DET_VBUS_INVALID;
				}

				info->det_vbus_old_state = det_vbus_state;
			} else {
				det_vbus_state = info->det_vbus_old_state;
			}
		}
	} else if (info->cfg->port[0].det_vbus_type == USB_DET_VBUS_TYPE_AXP) {
#if defined(CONFIG_AW_AXP)
		if (axp_usb_det()) {
			det_vbus_state = USB_DET_VBUS_VALID;
		} else {
			det_vbus_state = USB_DET_VBUS_INVALID;
		}

#endif
	} else {
		det_vbus_state = info->det_vbus_old_state;
	}
	return det_vbus_state;
}
#ifdef CONFIG_ARCH_SUN9IW1
static u32 get_dp_dm_status(struct usb_scan_info *info)
{
       return 0;
}
#else
static u32 get_dp_dm_status_normal(struct usb_scan_info *info)
{
	__u32 reg_val = 0;
	__u32 dp = 0;
	__u32 dm = 0;

	/* USBC_EnableDpDmPullUp */
	reg_val = USBC_Readl(USBC_REG_ISCR(SUNXI_USB_OTG_VBASE));
	reg_val |= (1 << USBC_BP_ISCR_DPDM_PULLUP_EN);
	USBC_Writel(reg_val, USBC_REG_ISCR(SUNXI_USB_OTG_VBASE));

	/* USBC_EnableIdPullUp */
	reg_val = USBC_Readl(USBC_REG_ISCR(SUNXI_USB_OTG_VBASE));
	reg_val |= (1 << USBC_BP_ISCR_ID_PULLUP_EN);
	USBC_Writel(reg_val, USBC_REG_ISCR(SUNXI_USB_OTG_VBASE));

	msleep(10);

	reg_val = USBC_Readl(USBC_REG_ISCR(SUNXI_USB_OTG_VBASE));
	dp = (reg_val >> USBC_BP_ISCR_EXT_DP_STATUS) & 0x01;
	dm = (reg_val >> USBC_BP_ISCR_EXT_DM_STATUS) & 0x01;

	return ((dp << 1) | dm);
}

static u32 get_dp_dm_status(struct usb_scan_info *info)
{
	u32 ret  = 0;
	u32 ret0 = 0;
	u32 ret1 = 0;
	u32 ret2 = 0;

	ret0 = get_dp_dm_status_normal(info);
	ret1 = get_dp_dm_status_normal(info);
	ret2 = get_dp_dm_status_normal(info);

	//continous 3 times, to avoid the voltage sudden changes
	if ((ret0 == ret1) && (ret0 == ret2)) {
		ret = ret0;
	} else if (ret2 == 0x11) {
		if (get_usb_role() == USB_ROLE_DEVICE) {
			ret = 0x11;
			DMSG_PANIC("ERR: dp/dm status is continuous(0x11)\n");
		}
	} else {
		ret = ret2;
	}

	return ret;
}
#endif
#endif

static void do_vbus0_id0(struct usb_scan_info *info)
{
	enum usb_role role = USB_ROLE_NULL;

	role = get_usb_role();
	device_insmod_delay = 0;

	switch(role) {
	case USB_ROLE_NULL:
		/* delay for vbus is stably */
		if (atomic_read(&thread_suspend_flag)) {
			break;
		}
		if (info->host_insmod_delay < USB_SCAN_INSMOD_HOST_DRIVER_DELAY) {
			info->host_insmod_delay++;
			break;
		}
		info->host_insmod_delay = 0;

		/* insmod usb host */
		hw_insmod_usb_host();
		break;

	case USB_ROLE_HOST:
		/* nothing to do */
		break;

	case USB_ROLE_DEVICE:
		/* rmmod usb device */
		if (atomic_read(&thread_suspend_flag)) {
			break;
		}
		hw_rmmod_usb_device();
		break;

	default:
		DMSG_PANIC("ERR: unkown usb role(%d)\n", role);
	}

	return;
}

static void do_vbus0_id1(struct usb_scan_info *info)
{
	enum usb_role role = USB_ROLE_NULL;

	role = get_usb_role();
	device_insmod_delay = 0;
	info->host_insmod_delay   = 0;

	switch(role) {
	case USB_ROLE_NULL:
		/* nothing to do */
		break;
	case USB_ROLE_HOST:
		if (atomic_read(&thread_suspend_flag)) {
			break;
		}
		hw_rmmod_usb_host();
		break;
	case USB_ROLE_DEVICE:
		if (atomic_read(&thread_suspend_flag)) {
			break;
		}
		hw_rmmod_usb_device();
		break;
	default:
		DMSG_PANIC("ERR: unkown usb role(%d)\n", role);
	}

	return;
}

static void do_vbus1_id0(struct usb_scan_info *info)
{
	enum usb_role role = USB_ROLE_NULL;

	role = get_usb_role();
	device_insmod_delay = 0;

	switch(role) {
	case USB_ROLE_NULL:
		if (atomic_read(&thread_suspend_flag)) {
			break;
		}
		/* delay for vbus is stably */
		if (info->host_insmod_delay < USB_SCAN_INSMOD_HOST_DRIVER_DELAY) {
			info->host_insmod_delay++;
			break;
		}
		info->host_insmod_delay = 0;

		hw_insmod_usb_host();
		break;
	case USB_ROLE_HOST:
		/* nothing to do */
		break;
	case USB_ROLE_DEVICE:
		if (atomic_read(&thread_suspend_flag)) {
			break;
		}
		hw_rmmod_usb_device();
		break;
	default:
		DMSG_PANIC("ERR: unkown usb role(%d)\n", role);
	}

	return;
}

static void do_vbus1_id1(struct usb_scan_info *info)
{
	enum usb_role role = USB_ROLE_NULL;

	role = get_usb_role();
	info->host_insmod_delay = 0;

	switch(role) {
	case USB_ROLE_NULL:
#ifndef  SUNXI_USB_FPGA
		if (get_dp_dm_status(info) == 0x00) {
			if (atomic_read(&thread_suspend_flag)) {
				break;
			}
			/* delay for vbus is stably */
			if (device_insmod_delay < USB_SCAN_INSMOD_DEVICE_DRIVER_DELAY) {
				device_insmod_delay++;
				break;
			}

			device_insmod_delay = 0;
#if defined(CONFIG_USB_G_ANDROID) || defined (CONFIG_USB_MASS_STORAGE)
			if(get_usb_gadget_functions())
#endif
				hw_insmod_usb_device();
		}
#else
		hw_insmod_usb_device();
#endif
		break;
	case USB_ROLE_HOST:
		if (atomic_read(&thread_suspend_flag)) {
			break;
		}
		hw_rmmod_usb_host();
		break;
	case USB_ROLE_DEVICE:
		/* nothing to do */
		break;
	default:
		DMSG_PANIC("ERR: unkown usb role(%d)\n", role);
	}

	return;
}

#ifdef  SUNXI_USB_FPGA
static u32 usb_vbus_id_state = 1;
__u32 set_vbus_id_state(u32 state)
{
	usb_vbus_id_state = state;
	return 0;
}
static __u32 get_vbus_id_state(struct usb_scan_info *info)
{
	return usb_vbus_id_state;
}
#else
static __u32 get_vbus_id_state(struct usb_scan_info *info)
{
	u32 state = 0;

	if (get_id_state(info) == USB_DEVICE_MODE) {
		x_set_bit(state, 0);
	}

	if (get_detect_vbus_state(info) == USB_DET_VBUS_VALID) {
		x_set_bit(state, 1);
	}

	return state;
}
#endif

static void vbus_id_hw_scan(struct usb_scan_info *info)
{
	__u32 vbus_id_state = 0;

	vbus_id_state = get_vbus_id_state(info);

	if (usb_hw_scan_debug)
		DMSG_INFO_MANAGER("vbus_id=%d,role=%d\n",vbus_id_state, get_usb_role());

	switch(vbus_id_state) {
	case  0x00:
		do_vbus0_id0(info);
		break;
	case  0x01:
		do_vbus0_id1(info);
		break;
	case  0x02:
		do_vbus1_id0(info);
		break;
	case  0x03:
		do_vbus1_id1(info);
		break;
	default:
		DMSG_PANIC("ERR: vbus_id_hw_scan: unkown vbus_id_state(0x%x)\n", vbus_id_state);
	}

	return;
}

static void null_hw_scan(struct usb_scan_info *info)
{
	DMSG_DBG_MANAGER("null_hw_scan\n");
	return;
}

void usb_hw_scan(struct usb_cfg *cfg)
{
	__usb_hw_scan(&g_usb_scan_info);
}

__s32 usb_hw_scan_init(struct usb_cfg *cfg)
{
	struct usb_scan_info *scan_info = &g_usb_scan_info;
	struct usb_port_info *port_info = NULL;
	__s32 ret = 0;

	memset(scan_info, 0, sizeof(struct usb_scan_info));
	device_insmod_delay = 0;
	scan_info->cfg 			= cfg;
	scan_info->id_old_state 	= USB_DEVICE_MODE;
	scan_info->det_vbus_old_state 	= USB_DET_VBUS_INVALID;

	port_info =&(cfg->port[0]);
	switch(port_info->port_type) {
	case USB_PORT_TYPE_DEVICE:
		__usb_hw_scan = null_hw_scan;
		break;
	case USB_PORT_TYPE_HOST:
		__usb_hw_scan = null_hw_scan;
		break;
	case USB_PORT_TYPE_OTG:
#ifdef  SUNXI_USB_FPGA
		{
			__usb_hw_scan = vbus_id_hw_scan;
		}
#else
		{
			switch(port_info->detect_type) {
			case USB_DETECT_TYPE_DP_DM:
				__usb_hw_scan = null_hw_scan;
				break;
			case USB_DETECT_TYPE_VBUS_ID:
				{
					__u32 need_det_vbus_req_gpio = 1;

					if (port_info->det_vbus_type == USB_DET_VBUS_TYPE_GIPO) {
						if ((port_info->id.valid == 0) || (port_info->det_vbus.valid == 0)) {
							DMSG_PANIC("ERR: usb detect tpye is vbus/id, but id(%d)/vbus(%d) is invalid\n",
								port_info->id.valid, port_info->det_vbus.valid);
							ret = -1;
							goto failed;
						}

						/* if id and vbus use the same pin, then need not to pull pio */
						if (port_info->id.gpio_set.gpio.gpio == port_info->det_vbus.gpio_set.gpio.gpio) {
							need_det_vbus_req_gpio = 0; /* when id and det_vbus reuse, the det bus need not request gpio again */
						}
					}

					/* request id gpio */
					if (port_info->id.valid) {
						ret = gpio_request(port_info->id.gpio_set.gpio.gpio, "otg_id");
						if (ret != 0) {
							DMSG_PANIC("ERR: id gpio_request failed\n");
							ret = -1;
							port_info->id.valid = 0;
							goto failed;
						}

						/* set config, input */
						//sunxi_gpio_setcfg(port_info->id.gpio_set.gpio.gpio, 0);
						gpio_direction_input(port_info->id.gpio_set.gpio.gpio);
						__gpio_set_value(port_info->id.gpio_set.gpio.gpio, 1);
					}

					/* request det_vbus gpio */
					if (port_info->det_vbus.valid && need_det_vbus_req_gpio) {
						ret = gpio_request(port_info->det_vbus.gpio_set.gpio.gpio, "otg_det");
						if (ret != 0) {
							DMSG_PANIC("ERR: det_vbus gpio_request failed\n");
							ret = -1;
							port_info->det_vbus.valid = 0;
							goto failed;
						}

						/* set config, input */
						//sunxi_gpio_setcfg(port_info->det_vbus.gpio_set.gpio.gpio, 0);
						gpio_direction_input(port_info->det_vbus.gpio_set.gpio.gpio);

						/* reserved is disable */
						//if (need_pull_pio) {
						//	sunxi_gpio_setpull(port_info->det_vbus.gpio_set.gpio.gpio, 0);
						//}
					}

					__usb_hw_scan = vbus_id_hw_scan;
				}
				break;
			default:
				DMSG_PANIC("ERR: unkown detect_type(%d)\n", port_info->detect_type);
				ret = -1;
				goto failed;
			}
		}
#endif
		break;
	default:
		DMSG_PANIC("ERR: unkown port_type(%d)\n", cfg->port[0].port_type);
		ret = -1;
		goto failed;
	}

	return 0;

failed:
#ifndef  SUNXI_USB_FPGA
	if (port_info->id.valid) {
		gpio_free(port_info->id.gpio_set.gpio.gpio);
	}

	if (port_info->det_vbus.valid) {
		gpio_free(port_info->det_vbus.gpio_set.gpio.gpio);
	}
#endif
	__usb_hw_scan = null_hw_scan;
	return ret;
}

#ifdef  SUNXI_USB_FPGA
__s32 usb_hw_scan_exit(struct usb_cfg *cfg)
{
	return 0;
}
#else
__s32 usb_hw_scan_exit(struct usb_cfg *cfg)
{
	if (cfg->port[0].id.valid) {
		gpio_free(cfg->port[0].id.gpio_set.gpio.gpio);
	}

	if (cfg->port[0].det_vbus.valid) {
		gpio_free(cfg->port[0].det_vbus.gpio_set.gpio.gpio);
	}
	return 0;
}
#endif

