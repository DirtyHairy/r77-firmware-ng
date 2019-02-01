#ifndef __DEV_LCD_H__
#define __DEV_LCD_H__

#include <linux/module.h>
#include <asm/uaccess.h>
#include <asm/memory.h>
#include <asm/unistd.h>
#include "asm-generic/int-ll64.h"
#include "linux/kernel.h"
#include "linux/mm.h"
#include "linux/semaphore.h"
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>   //wake_up_process()
#include <linux/kthread.h> //kthread_create()?��kthread_run()
#include <linux/err.h> //IS_ERR()?��PTR_ERR()
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <mach/platform.h>

#include <mach/sys_config.h>
typedef unsigned int __hdle;
#include <video/drv_display.h>

extern int sunxi_disp_get_source_ops(struct sunxi_disp_source_ops *src_ops);
extern s32 DRV_DISP_Init(void);
extern s32 Fb_Init(u32 from);
extern void LCD_set_panel_funs(void);


#define OSAL_PRINTF(msg...) {printk(KERN_WARNING "[LCD] ");printk(msg);}
#define __inf(msg...)       {printk(KERN_WARNING "[LCD] ");printk(msg);}
#define __msg(msg...)       {printk(KERN_WARNING "[LCD] file:%s,line:%d:    ",__FILE__,__LINE__);printk(msg);}
#define __wrn(msg...)       {printk(KERN_WARNING "[LCD WRN] file:%s,line:%d:    ",__FILE__,__LINE__); printk(msg);}
#define __here__            {printk(KERN_WARNING "[LCD] file:%s,line:%d\n",__FILE__,__LINE__);}

struct sunxi_lcd_drv
{
  struct device                     *dev;
  struct cdev                       *lcd_cdev;
  dev_t                             devid;
  struct class                      *lcd_class;
  struct sunxi_disp_source_ops      src_ops;
};

#include "panels/panels.h"

#endif
