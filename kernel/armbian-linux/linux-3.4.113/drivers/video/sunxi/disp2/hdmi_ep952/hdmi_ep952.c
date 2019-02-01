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
#include <linux/kthread.h> //kthread_create()??ï¿½ï¿½|kthread_run()
#include <linux/err.h> //IS_ERR()??ï¿½ï¿½|PTR_ERR()
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/switch.h> // switch_dev
#include <mach/sys_config.h>
#include <mach/platform.h>
#include "../disp/disp_sys_intf.h"
#include <video/sunxi_display2.h>
#include "EP952api.h"

#define HDMI720P_50         19
#define HDMI720P_60         4
#define HDMI1080P_50        31
#define HDMI1080P_60        16
#define HDMI480P            2
#define HDMI576P            17

static char modules_name[32] = "ep952";
static char key_name[20] = "hdmi_ep952_para";
static u32 hdmi_used = 0;

static int g_ep952_enabled = 0;
static disp_tv_mode g_hdmi_mode = DISP_TV_MOD_720P_50HZ;
static unsigned char g_hdmi_vic = HDMI720P_50;
static unsigned char g_hpd_state = 0;
static unsigned char sunxi_edid[256] = {0};

static u32 hdmi_i2c_id = 1;
static u32 hdmi_i2c_used = 0;
static u32 hdmi_screen_id = 0;
static u32 hdmi_power_used = 0;
static char hdmi_power[16] = {0};

static bool hdmi_io_used[28];
static disp_gpio_set_t hdmi_io[28];
static bool hdmi_gpio_used[4];
static disp_gpio_set_t hdmi_gpio[4];
static struct i2c_adapter *hdmi_i2c_adapter = NULL;
static struct i2c_adapter *ep952_i2c_adapter = NULL;
static struct i2c_client *ep952_i2c_client;
static struct disp_device *ep952_device = NULL;
static disp_vdevice_source_ops hdmi_source_ops;

static struct task_struct *ep952_task = NULL;

#if defined(CONFIG_SWITCH) || defined(CONFIG_ANDROID_SWITCH)
static struct switch_dev hdmi_switch_dev = {
    .name = "hdmi",
};
#endif

s32 ep952_i2c_write(u32 client_addr, u8 * data,int size);
s32 ep952_i2c_read(u32 client_addr,u8 sub_addr,u8 * data,int size);
extern struct i2c_adapter *i2c_get_adapter(int nr);

extern struct disp_device* disp_vdevice_register(disp_vdevice_init_data *data);
extern s32 disp_vdevice_unregister(struct disp_device *vdevice);
extern s32 disp_vdevice_get_source_ops(disp_vdevice_source_ops *ops);
extern unsigned int disp_boot_para_parse(void);

extern void EP952Controller_Timer(void);
extern void EP952Controller_Task(void);
extern unsigned char HDMI_Tx_hpd_state(void);
extern int sunxi_edid_update(unsigned char * pedid);

static disp_video_timings video_timing[] =
{
    /*vic  hdmi_mode         PCLK    AVI  x   y    HT  HBP HFP HST  VT  VBP VFP VST  H_P V_P I  TRD */
    {HDMI720P_50,   DISP_TV_MOD_720P_50HZ,  74250000,  0,  1280,  720,   1980,  220,  440,  40,  750,   20,  5,  5,  1,   1,   0,   0,   0},
    {HDMI720P_60,   DISP_TV_MOD_720P_60HZ,  74250000,  0,  1280,  720,   1650,  220,  110,  40,  750,   20,  5,  5,  1,   1,   0,   0,   0},
    {HDMI1080P_50,  DISP_TV_MOD_1080P_50HZ, 148500000, 0,  1920,  1080, 2640,  148,  528,  44,  1125,  36,  4,  5,  1,   1,   0,   0,   0},
    {HDMI1080P_60,  DISP_TV_MOD_1080P_60HZ, 148500000, 0,  1920,  1080, 2200,  148,  88,   44,  1125,  36,  4,  5,  1,   1,   0,   0,   0},
    {HDMI480P,      DISP_TV_MOD_480P,       27000000,  0,  720,  480,   858,   60,   16,   62,  525,   30,  9,  6,  0,   0,   0,   0,   0},
    {HDMI576P,      DISP_TV_MOD_576P,       27000000,  0,  720,  576,   864,   68,   12,   64,  625,   39,  5,  5,  0,   0,   0,   0,   0},
};

static int hdmi_parse_config(void)
{
    disp_gpio_set_t  *gpio_info;
    int i, ret;
    char io_name[32];

    for(i=0; i<28; i++) {
        gpio_info = &(hdmi_io[i]);
        sprintf(io_name, "hdmi_d%d", i);
        ret = disp_sys_script_get_item(key_name, io_name, (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
        if(ret == 3) {
            hdmi_io_used[i]= 1;
        }
    }
    for(i=0; i<sizeof(hdmi_gpio); i++) {
        gpio_info = &(hdmi_gpio[i]);
        sprintf(io_name, "gpio%d", i);
        ret = disp_sys_script_get_item(key_name, io_name, (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
        if(ret == 3) {
            hdmi_gpio_used[i]= 1;
        }
    }

    return 0;
}

// bon: 0-disable the gpio, 1-out as same as configed.
int hdmi_gpio_config(int gpio_id, int bon)
{
    int hdl;

    if(gpio_id <= sizeof(hdmi_gpio_used) && hdmi_gpio_used[gpio_id]) {
        disp_gpio_set_t  gpio_info[1];

        memcpy(gpio_info, &(hdmi_gpio[gpio_id]), sizeof(disp_gpio_set_t));
        if(!bon) {
            gpio_info->mul_sel = 7;
        }
        hdl = disp_sys_gpio_request(gpio_info, 1);
        disp_sys_gpio_release(hdl, 2);
    }
    return 0;
}

static int hdmi_pin_config(u32 bon)
{
    int hdl,i;

    for(i=0; i<28; i++) {
        if(hdmi_io_used[i]) {
            disp_gpio_set_t  gpio_info[1];

            memcpy(gpio_info, &(hdmi_io[i]), sizeof(disp_gpio_set_t));
            if(!bon) {
                gpio_info->mul_sel = 7;
            }
            hdl = disp_sys_gpio_request(gpio_info, 1);
            disp_sys_gpio_release(hdl, 2);
        }
    }
    return 0;
}

static s32 ep952_hdmi_power_on(u32 on_off)
{
    if(hdmi_power_used == 0)
        return 0;

    if(on_off)
        disp_sys_power_enable(hdmi_power);
    else
        disp_sys_power_disable(hdmi_power);

    return 0;
}

static unsigned char mode2vic(disp_tv_mode hdmi_mode)
{
    switch(hdmi_mode) {
    case DISP_TV_MOD_480P:
        return HDMI480P;
    case DISP_TV_MOD_576P:
        return HDMI576P;
    case DISP_TV_MOD_720P_50HZ:
        return HDMI720P_50;
    case DISP_TV_MOD_720P_60HZ:
        return HDMI720P_60;
    case DISP_TV_MOD_1080P_50HZ:
        return HDMI1080P_50;
    case DISP_TV_MOD_1080P_60HZ:
        return HDMI1080P_60;
    default:
        printk("fixme:%s:%d!!!", __func__, __LINE__);
        return HDMI720P_50;
    }
}

static void ep952_update_hpd_status(void)
{
    static unsigned char s_hpd_change_count = 0;
    unsigned char current_state = HDMI_Tx_hpd_state() ? 1 : 0;

    if(g_hpd_state != current_state) {
        if(s_hpd_change_count++ >= 10) {
            s_hpd_change_count = 0;
            g_hpd_state = current_state;
            if(current_state) {
                sunxi_edid_update(sunxi_edid);
            }
            switch_set_state(&hdmi_switch_dev, current_state);
        }
    } else {
        s_hpd_change_count = 0;
    }
}

static s32 ep952_get_hpd_status(void)
{
    return g_hpd_state;
}

int ep952_thread(void *parg)
{
	unsigned int timeout = HZ / 50; // 20ms
    printk("%s:%d\n", __func__, __LINE__);
    while(1) {
        if(kthread_should_stop()) {
            break;
        }
        ep952_update_hpd_status();
        //printk("ep952 tx hpd state [%d]\n",ep952_get_hpd_status());
        if(g_ep952_enabled) {
           EP952Controller_Timer();
           EP952Controller_Task();
        }
        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout(timeout);
    }
    return 0;
}

int ep952_thread_enable(void)
{
	int err;

    if(ep952_task)
        return 0;

    ep952_task = kthread_create(ep952_thread, NULL, "ep952_task");
    if(IS_ERR(ep952_task)){
        printk("Unable to start kernel thread./n");
        err = PTR_ERR(ep952_task);
        ep952_task = NULL;
        return err;
    } else {
        wake_up_process(ep952_task);
    }
	return 0;
}

int ep952_thread_disable(void)
{
    if(ep952_task) {
        kthread_stop(ep952_task);
        ep952_task = NULL;
    }
	return 0;
}

static s32 ep952_get_mode_support(disp_tv_mode hdmi_mode)
{
    switch(hdmi_mode) {
    case DISP_TV_MOD_480P:
    case DISP_TV_MOD_576P:
    case DISP_TV_MOD_720P_50HZ:
    case DISP_TV_MOD_720P_60HZ:
    //case DISP_TV_MOD_1080P_50HZ:
    //case DISP_TV_MOD_1080P_60HZ:
        return 1;
    default:
        return 0;
    }
}

static s32 ep952_set_mode(disp_tv_mode hdmi_mode)
{
    if(ep952_get_mode_support(hdmi_mode)) {
        g_hdmi_mode = hdmi_mode;
        g_hdmi_vic = mode2vic(hdmi_mode);
        if(g_ep952_enabled) {
            EP_HDMI_Set_Video_Timing(g_hdmi_vic);
        }
    }
    return 0;
}

static s32 ep952_get_video_timing_info(disp_video_timings **video_info)
{
    disp_video_timings *info;
    int ret = -1;
    int i, list_num;
    info = video_timing;

    list_num = sizeof(video_timing)/sizeof(disp_video_timings);
    for(i=0; i<list_num; i++) {
        if(info->tv_mode == g_hdmi_mode){
            *video_info = info;
            ret = 0;
            break;
        }
        info ++;
    }
    return ret;
}

static s32 ep952_hdmi_get_interface_para(void* para)
{
    disp_vdevice_interface_para *intf_para = (disp_vdevice_interface_para *)para;
    if(NULL == para) {
        printk("%s: null pointer\n", __func__);
        return -1;
    }
    intf_para->intf = 0;
    intf_para->sub_intf = 0;
    intf_para->sequence = 0;
    intf_para->clk_phase = 1;
    intf_para->sync_polarity = 3;

    return 0;
}

//0:rgb;  1:yuv
static s32 ep952_get_input_csc(void)
{
    return 0;
}

static s32 ep952_open(void)
{
    printk("%s:%d\n", __func__, __LINE__);
    ep952_thread_disable();
    ep952_hdmi_power_on(1);
    hdmi_pin_config(1);
    msleep(100);
    if(hdmi_source_ops.tcon_enable)
        hdmi_source_ops.tcon_enable(ep952_device);
    msleep(100);
    //EP_HDMI_Init();
    EP_HDMI_Set_Video_Timing(g_hdmi_vic);               // 720p50hz
    EP_HDMI_Set_Audio_Fmt(AUD_I2S, AUD_SF_48000Hz);     // IIS input , 48KHz
	g_ep952_enabled = 1;

    ep952_thread_enable();

    return 0;
}

static s32 ep952_close(void)
{
    printk("%s:%d\n", __func__, __LINE__);
	g_ep952_enabled = 0;
    if(hdmi_source_ops.tcon_disable)
        hdmi_source_ops.tcon_disable(ep952_device);
    msleep(100);
    hdmi_pin_config(0);
    ep952_hdmi_power_on(0);
    msleep(100);
    return 0;
}

static int ep952_suspend(void)
{
	ep952_close();
	ep952_thread_disable();
	return 0;
}

static int ep952_resume(void)
{
	//ep952_thread_enable();
	ep952_open();
	return 0;
}

static int ep952_early_suspend(void)
{
    printk("%s:%d: fixme if neccessry.\n", __func__, __LINE__);
    return 0;
}

static int ep952_late_resume(void)
{
    printk("%s:%d: fixme if neccessry.\n", __func__, __LINE__);
    return 0;
}

static int hdmi_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    disp_vdevice_init_data init_data;

    pr_info("[DISP_I2C] hdmi_i2c_probe\n");
    memset(&init_data, 0, sizeof(disp_vdevice_init_data));
    ep952_i2c_client = client;

    init_data.disp = hdmi_screen_id;
    memcpy(init_data.name, modules_name, 32);
    init_data.type = DISP_OUTPUT_TYPE_HDMI;
    init_data.fix_timing = 0;

    init_data.func.enable = ep952_open;
    init_data.func.disable = ep952_close;
    init_data.func.set_mode = ep952_set_mode;
    init_data.func.mode_support = ep952_get_mode_support;
    init_data.func.get_HPD_status = ep952_get_hpd_status;
    init_data.func.get_input_csc = ep952_get_input_csc;
    init_data.func.get_video_timing_info = ep952_get_video_timing_info;
	init_data.func.suspend = ep952_suspend;
	init_data.func.resume = ep952_resume;
	init_data.func.early_suspend = ep952_early_suspend;
	init_data.func.late_resume = ep952_late_resume;
    init_data.func.get_interface_para = ep952_hdmi_get_interface_para;
    disp_vdevice_get_source_ops(&hdmi_source_ops);
    hdmi_parse_config();
	EP_HDMI_Init();

    #if defined(CONFIG_SWITCH) || defined(CONFIG_ANDROID_SWITCH)
    switch_dev_register(&hdmi_switch_dev);
    #endif
	ep952_thread_enable();

    ep952_device = disp_vdevice_register(&init_data);

    return 0;
}

static int __devexit hdmi_i2c_remove(struct i2c_client *client)
{
    if(ep952_device)
        disp_vdevice_unregister(ep952_device);
    ep952_device = NULL;
    return 0;
}

static const struct i2c_device_id hdmi_i2c_id_table[] = {
    { "hdmi_i2c", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, hdmi_i2c_id_table);

static int hdmi_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    if(hdmi_i2c_id == client->adapter->nr){
        const char *type_name = "hdmi_i2c";
        ep952_i2c_adapter = client->adapter;
        pr_info("[DISP_I2C] hdmi_i2c_detect, get right i2c adapter, id=%d\n", ep952_i2c_adapter->nr);
        strlcpy(info->type, type_name, I2C_NAME_SIZE);
        return 0;
    }
    return -ENODEV;
}

static  unsigned short normal_i2c[] = {0x46, I2C_CLIENT_END};

static struct i2c_driver hdmi_i2c_driver = {
    .class = I2C_CLASS_HWMON,
    .probe        = hdmi_i2c_probe,
    .remove        = __devexit_p(hdmi_i2c_remove),
    .id_table    = hdmi_i2c_id_table,
    .driver    = {
        .name    = "hdmi_i2c",
        .owner    = THIS_MODULE,
    },
    .detect        = hdmi_i2c_detect,
    .address_list    = normal_i2c,
};

static int  hdmi_i2c_init(void)
{
    int ret;
    int value;

    ret = disp_sys_script_get_item(key_name, "hdmi_twi_used", &value, 1);
    if(1 == ret) {
        hdmi_i2c_used = value;
        if(hdmi_i2c_used == 1) {
            ret = disp_sys_script_get_item(key_name, "hdmi_twi_id", &value, 1);
            hdmi_i2c_id = (ret == 1)? value : hdmi_i2c_id;
            return i2c_add_driver(&hdmi_i2c_driver);
        }
    }
    return 0;
}

static void hdmi_i2c_exit(void)
{
    i2c_del_driver(&hdmi_i2c_driver);
}

s32 ep952_i2c_write(u32 client_addr, u8 *data, int size)
{
    s32 ret = 0;
    struct i2c_msg msg;

    if(hdmi_i2c_used) {
        msg.addr = client_addr;
        msg.flags = 0;
        msg.len = size;
        msg.buf = data;
        ret = i2c_transfer(ep952_i2c_adapter, &msg, 1);
    }

    return (ret==1)?0:2;
}

s32 ep952_i2c_read(u32 client_addr, u8 sub_addr, u8 *data, int size)
{
    s32 ret = 0;
    struct i2c_msg msgs[] = {
        {
            .addr   = client_addr,
            .flags  = 0,
            .len    = 1,
            .buf    = &sub_addr,
        },
        {
            .addr   = client_addr,
            .flags  = I2C_M_RD,
            .len    = size,
            .buf    = data,
        },
    };

    if(hdmi_i2c_used) {
        ret = i2c_transfer(ep952_i2c_adapter, msgs, 2);
    }

    return (ret==2)?0:2;
}

s32 hdmi_i2c_write(u32 client_addr, u8 *data, int size)
{
    s32 ret = 0;
    struct i2c_msg msg;

    if(hdmi_i2c_adapter == NULL) {
        hdmi_i2c_adapter = i2c_get_adapter(1); // fixme
    }
    if(hdmi_i2c_used) {
        msg.addr = client_addr;
        msg.flags = 0;
        msg.len = size;
        msg.buf = data;
        ret = i2c_transfer(hdmi_i2c_adapter, &msg, 1);
    }

    return (ret==1)?0:2;
}

s32 hdmi_i2c_read(u32 client_addr, u8 sub_addr, u8 *data, int size)
{
    s32 ret = 0;

    struct i2c_msg msgs[] = {
        {
            .addr   = client_addr,
            .flags  = 0,
            .len    = 1,
            .buf    = &sub_addr,
        },
        {
            .addr   = client_addr,
            .flags  = I2C_M_RD,
            .len    = size,
            .buf    = data,
        },
    };

    if(hdmi_i2c_adapter == NULL) {
        hdmi_i2c_adapter = i2c_get_adapter(1); // fixme
    }

    if(hdmi_i2c_used) {
        ret = i2c_transfer(hdmi_i2c_adapter, msgs, 2);
    }

    return (ret==2)?0:2;
}

static int __init ep952_module_init(void)
{
    int ret;
    int value;

    pr_info("[HDMI]ep952_module_init begin\n");

    ret = disp_sys_script_get_item(key_name, "hdmi_used", &value, 1);
    if(1 == ret) {
       hdmi_used = value;
       if(hdmi_used) {
           ret = disp_sys_script_get_item(key_name, "hdmi_power", (int*)hdmi_power, 32/sizeof(int));
           if(2 == ret) {
               hdmi_power_used = 1;
               printk("[HDMI] hdmi_power: %s\n", hdmi_power);
           }
           hdmi_i2c_init();
#if 0
           unsigned int value, output_type0, output_mode0, output_type1, output_mode1;
           value = disp_boot_para_parse();
           output_type0 = (value >> 8) & 0xff;
           output_mode0 = (value) & 0xff;
           output_type1 = (value >> 24)& 0xff;
           output_mode1 = (value >> 16) & 0xff;
           if((output_type0 == DISP_OUTPUT_TYPE_HDMI) ||
           (output_type1 == DISP_OUTPUT_TYPE_HDMI)) {
               printk("[HDMI]%s:smooth boot", __func__);
               ep952_hdmi_power_on(1);
               if(DISP_OUTPUT_TYPE_HDMI == output_type0)
               g_hdmi_mode = output_mode0;
               else if(DISP_OUTPUT_TYPE_HDMI == output_type1)
               g_hdmi_mode = output_mode1;
           }
#endif
       }
    } else
        hdmi_used = 0;

    return 0;
}

static void __exit ep952_module_exit(void)
{
    pr_info("ep952_module_exit\n");
    hdmi_i2c_exit();
}

late_initcall(ep952_module_init);
module_exit(ep952_module_exit);

MODULE_AUTHOR("hezuyao");
MODULE_DESCRIPTION("ep952_driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ep952");
