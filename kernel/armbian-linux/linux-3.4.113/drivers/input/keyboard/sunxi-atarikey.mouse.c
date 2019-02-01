#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/keyboard.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/timer.h> 
#include <linux/clk.h>
#include <mach/sys_config.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>

static struct input_dev *atari_dev = NULL;
struct input_dev *input_left = NULL, *input_right = NULL;
struct input_dev *input_mouse = NULL;
static struct platform_device *virmouse_dev = NULL;
static struct work_struct atari_gpio_work;
static struct timer_list atari_gpio_timer;
static struct i2c_client *g_client = NULL;
static struct work_struct atari_i2c_work;
static struct timer_list atari_i2c_timer;

static struct gpio_config select_key_io;
static int select_key_status = 0;

static void atari_gpio_key_timer(unsigned long _data){
	schedule_work(&atari_gpio_work);
}

static void atari_gpio_keys_report_event(struct work_struct *work){
	int val = 0;
	val = __gpio_get_value(select_key_io.gpio);
	if(val != select_key_status){
		input_report_key(atari_dev, KEY_ENTER, val);
		input_sync(atari_dev);
		select_key_status = val;
		printk("stanley select key[%d] = %d\n", select_key_io.gpio, val);
	}
	mod_timer( &atari_gpio_timer, (jiffies+HZ/10));
}

static int atari_gpio_script_init(void){
	script_item_u	val;
	script_item_value_type_e  type;

	type = script_get_item("gpio_para", "gpio_pin_5", &val);
	select_key_io = val.gpio;
}


#define ATARI_CONTROLLER (0x52 << 1) //i2c slave address
static const struct i2c_device_id bte_i2c_ids[] = {
	{ "bte_i2c", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, bte_i2c_ids);

static int g_i2c_handle = -1;
typedef union{
	struct{
		unsigned char k0 : 1;
		unsigned char k1 : 1;
		unsigned char k2 : 1;
		unsigned char k3 : 1;
		unsigned char k4 : 1;
		unsigned char k5 : 1;
		unsigned char k6 : 1;
		unsigned char k7 : 1;
	} u;
	unsigned char key;
}key_status;

struct i2c_key{
	unsigned char len;
	key_status right; //bit => nc/nc/fire/nc/right/left/down/up
	unsigned char paddle0;
	unsigned char paddle1;
	key_status left;  //bit => nc/nc/fire/nc/right/left/down/up
	unsigned char paddle2;
	unsigned char paddle3;
} g_i2c_key;

static void atari_i2c_key_timer(unsigned long _data){
        schedule_work(&atari_i2c_work);
}
static unsigned char old_i2c_key_paddle0 = 0;
static unsigned char old_i2c_key_paddle1 = 0;
static unsigned char cur_paddle0 = 0;
static unsigned char cur_paddle1 = 0;


static void atari_i2c_keys_report_event(struct work_struct *work){
	static struct i2c_key old_key = { 0, .left.key = 0x2f, .right.key = 0x2f};

        g_i2c_key.left.key = 0;
        g_i2c_key.right.key = 0;

	if(g_client){
		char data[4];
		int ret = 0;

		data[0] = 0x0;
		i2c_master_send(g_client, data, 1);
		ret = i2c_master_recv(g_client, &g_i2c_key, 7);
		if(ret == 7){
#if 0
			printk("stanley right=0x%x, left=0x%x, paddle0=0x%x, paddle1=0x%x\n", g_i2c_key.right.key, g_i2c_key.left.key, g_i2c_key.paddle0, g_i2c_key.paddle1);
			if(g_i2c_key.paddle0 != 0){//Paddle0
				cur_paddle0 = g_i2c_key.paddle0;
				if(old_i2c_key_paddle0 == 0 || abs(cur_paddle0 - old_i2c_key_paddle0) == 1){
					old_i2c_key_paddle0 = cur_paddle0;
				}else{
					if((cur_paddle0 - old_i2c_key_paddle0) != 0){
						input_report_rel(input_left, REL_X, (cur_paddle0 - old_i2c_key_paddle0) * 10);
						input_report_rel(input_left, REL_Y, 0);
					}
					if(g_i2c_key.left.u.k1 != old_key.left.u.k1){ //Left
						if(g_i2c_key.left.u.k1 == 1){//release
							input_report_key(input_left, BTN_LEFT, 0);
						}else{//press
							input_report_key(input_left, BTN_LEFT, 1);
						}
					}
					input_sync(input_left);
				}
				old_i2c_key_paddle0 = cur_paddle0;
			}else{
				if( g_i2c_key.left.key != old_key.left.key ){
					if(g_i2c_key.left.u.k0 != old_key.left.u.k0){ //Up
						if(g_i2c_key.left.u.k0 == 1){
							input_event(input_left, EV_ABS, ABS_Y, 0);
						}else{
							input_event(input_left, EV_ABS, ABS_Y, -32768);
						}
					}
					if(g_i2c_key.left.u.k1 != old_key.left.u.k1){ //Left
						if(g_i2c_key.left.u.k1 == 1){
							input_event(input_left, EV_ABS, ABS_X, 0);
						}else{
							input_event(input_left, EV_ABS, ABS_X, -32768);
						}
					}
					if(g_i2c_key.left.u.k2 != old_key.left.u.k2){ //Down
						if(g_i2c_key.left.u.k2 == 1){
							input_event(input_left, EV_ABS, ABS_Y, 0);
						}else{
							input_event(input_left, EV_ABS, ABS_Y, 32767);
						}
					}
					if(g_i2c_key.left.u.k3 != old_key.left.u.k3){ //Right
						if(g_i2c_key.left.u.k3 == 1){
							input_event(input_left, EV_ABS, ABS_X, 0);
						}else{
							input_event(input_left, EV_ABS, ABS_X, 32767);
						}
					}
					if(g_i2c_key.left.u.k5 != old_key.left.u.k5){ //Fire
						if(g_i2c_key.left.u.k5 == 1){
							input_event(input_left, EV_KEY, BTN_0, 0);
						}else{
							input_event(input_left, EV_KEY, BTN_0, 1);
						}
					}

					old_key.left.key = g_i2c_key.left.key;
					input_sync(input_left);
				}
			}


			if(g_i2c_key.paddle1 != 0){
				cur_paddle1 = g_i2c_key.paddle1;

				if(old_i2c_key_paddle1 == 0 || abs(cur_paddle1 - old_i2c_key_paddle1) == 1){
					old_i2c_key_paddle1 = cur_paddle1;
				}else{
					if((cur_paddle1 - old_i2c_key_paddle1) != 0){
						input_report_rel(input_left, REL_X, 0);
						input_report_rel(input_left, REL_Y, (cur_paddle1 - old_i2c_key_paddle1) * 10);
					}
					if(g_i2c_key.left.u.k3 != old_key.left.u.k3){ //Right
						if(g_i2c_key.left.u.k3 == 1){//release
							input_report_key(input_left, BTN_RIGHT, 0);
						}else{//press
							input_report_key(input_left, BTN_RIGHT, 1);
						}
					}
					input_sync(input_left);
				}
				old_i2c_key_paddle1 = cur_paddle1;
			}else{
				if( g_i2c_key.right.key != old_key.right.key ){
					if(g_i2c_key.right.u.k0 != old_key.right.u.k0){ //Up
						if(g_i2c_key.right.u.k0 == 1){
							input_event(input_right, EV_ABS, ABS_Y, 0);
						}else{
							input_event(input_right, EV_ABS, ABS_Y, -32768);
						}
					}
					if(g_i2c_key.right.u.k1 != old_key.right.u.k1){ //Left
						if(g_i2c_key.right.u.k1 == 1){
							input_event(input_right, EV_ABS, ABS_X, 0);
						}else{
							input_event(input_right, EV_ABS, ABS_X, -32768);
						}
					}
					if(g_i2c_key.right.u.k2 != old_key.right.u.k2){ //Down
						if(g_i2c_key.right.u.k2 == 1){
							input_event(input_right, EV_ABS, ABS_Y, 0);
						}else{
							input_event(input_right, EV_ABS, ABS_Y, 32767);
						}
					}
					if(g_i2c_key.right.u.k3 != old_key.right.u.k3){ //Right
						if(g_i2c_key.right.u.k3 == 1){
							input_event(input_right, EV_ABS, ABS_X, 0);
						}else{
							input_event(input_right, EV_ABS, ABS_X, 32767);
						}
					}
					if(g_i2c_key.right.u.k5 != old_key.right.u.k5){ //Fire
						if(g_i2c_key.right.u.k5 == 1){
							input_event(input_right, EV_KEY, BTN_0, 0);
						}else{
							input_event(input_right, EV_KEY, BTN_0, 1);
						}
					}

					old_key.right.key = g_i2c_key.right.key;
					input_sync(input_right);
				}
			}
//			old_key.left.key = g_i2c_key.left.key;
//			old_key.right.key = g_i2c_key.right.key;
//			if(g_i2c_key.paddle2 != 0 && g_i2c_key.paddle3 != 0){
//				printk("stanley paddle2=%x, paddle3=%x\n", g_i2c_key.paddle2, g_i2c_key.paddle3);
//			}
		}
#endif
			if( g_i2c_key.left.key != old_key.left.key ){
				if(g_i2c_key.left.u.k0 != old_key.left.u.k0){ //Up
					if(g_i2c_key.left.u.k0 == 1){
						input_event(input_left, EV_ABS, ABS_Y, 0);
					}else{
						input_event(input_left, EV_ABS, ABS_Y, -32768);
					}
				}
				if(g_i2c_key.left.u.k1 != old_key.left.u.k1){ //Left
					if(g_i2c_key.left.u.k1 == 1){
						input_event(input_left, EV_ABS, ABS_X, 0);
					}else{
						input_event(input_left, EV_ABS, ABS_X, -32768);
					}
				}
				if(g_i2c_key.left.u.k2 != old_key.left.u.k2){ //Down
					if(g_i2c_key.left.u.k2 == 1){
						input_event(input_left, EV_ABS, ABS_Y, 0);
					}else{
						input_event(input_left, EV_ABS, ABS_Y, 32767);
					}
				}
				if(g_i2c_key.left.u.k3 != old_key.left.u.k3){ //Right
					if(g_i2c_key.left.u.k3 == 1){
						input_event(input_left, EV_ABS, ABS_X, 0);
					}else{
						input_event(input_left, EV_ABS, ABS_X, 32767);
					}
				}
				if(g_i2c_key.left.u.k5 != old_key.left.u.k5){ //Fire
					if(g_i2c_key.left.u.k5 == 1){
						input_event(input_left, EV_KEY, BTN_0, 0);
					}else{
						input_event(input_left, EV_KEY, BTN_0, 1);
					}
				}

				old_key.left.key = g_i2c_key.left.key;
				input_sync(input_left);
			}
			if( g_i2c_key.right.key != old_key.right.key ){
				if(g_i2c_key.right.u.k0 != old_key.right.u.k0){ //Up
					if(g_i2c_key.right.u.k0 == 1){
						input_event(input_right, EV_ABS, ABS_Y, 0);
					}else{
						input_event(input_right, EV_ABS, ABS_Y, -32768);
					}
				}
				if(g_i2c_key.right.u.k1 != old_key.right.u.k1){ //Left
					if(g_i2c_key.right.u.k1 == 1){
						input_event(input_right, EV_ABS, ABS_X, 0);
					}else{
						input_event(input_right, EV_ABS, ABS_X, -32768);
					}
				}
				if(g_i2c_key.right.u.k2 != old_key.right.u.k2){ //Down
					if(g_i2c_key.right.u.k2 == 1){
						input_event(input_right, EV_ABS, ABS_Y, 0);
					}else{
						input_event(input_right, EV_ABS, ABS_Y, 32767);
					}
				}
				if(g_i2c_key.right.u.k3 != old_key.right.u.k3){ //Right
					if(g_i2c_key.right.u.k3 == 1){
						input_event(input_right, EV_ABS, ABS_X, 0);
					}else{
						input_event(input_right, EV_ABS, ABS_X, 32767);
					}
				}
				if(g_i2c_key.right.u.k5 != old_key.right.u.k5){ //Fire
					if(g_i2c_key.right.u.k5 == 1){
						input_event(input_right, EV_KEY, BTN_0, 0);
					}else{
						input_event(input_right, EV_KEY, BTN_0, 1);
					}
				}

				old_key.right.key = g_i2c_key.right.key;
				input_sync(input_right);
			}
		}
	}
	mod_timer( &atari_i2c_timer, (jiffies+HZ/10));
}

static int bte_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id){
	printk("stanley bte_i2c_probe\n");
	g_client = client;

	input_left = input_allocate_device();
	input_right = input_allocate_device();

	input_left->name = "i2c_controller";
	input_left->phys = "i2c_controller/input0";
	input_left->id.bustype = BUS_HOST;
	input_left->id.vendor = 0x0001;
	input_left->id.product = 0x0001;
	input_left->id.version = 0x0100;

	input_set_capability(input_left, EV_KEY, BTN_0);
	input_set_capability(input_left, EV_KEY, BTN_1);
	input_set_capability(input_left, EV_KEY, BTN_2);
	input_set_capability(input_left, EV_KEY, BTN_3);
	input_set_capability(input_left, EV_KEY, BTN_4); //joystic 1 only use this for fire button
	input_set_capability(input_left, EV_KEY, BTN_5);
	input_set_capability(input_left, EV_KEY, BTN_6);
	input_set_capability(input_left, EV_KEY, BTN_7);

	input_set_capability(input_left, EV_ABS, ABS_X);
	input_set_abs_params(input_left, ABS_X, -32768, 32767, 0, 0);
	input_set_capability(input_left, EV_ABS, ABS_Y);
	input_set_abs_params(input_left, ABS_Y, -32768, 32767, 0, 0);
	input_set_capability(input_left, EV_ABS, ABS_Z);
	input_set_abs_params(input_left, ABS_Z, -32768, 32767, 0, 0);

//	set_bit(EV_REL, input_left->evbit);
//	set_bit(REL_X, input_left->relbit);
//	set_bit(REL_Y, input_left->relbit);
//	set_bit(EV_KEY, input_left->evbit);
//	set_bit(BTN_LEFT, input_left->keybit);
//	set_bit(BTN_MIDDLE, input_left->keybit);
//	set_bit(BTN_RIGHT, input_left->keybit);

	input_register_device(input_left);

	input_right->name = "i2c_controller";
	input_right->phys = "i2c_controller/input1";
	input_right->id.bustype = BUS_HOST;
	input_right->id.vendor = 0x0001;
	input_right->id.product = 0x0001;
	input_right->id.version = 0x0100;

	input_set_capability(input_right, EV_KEY, BTN_0);
	input_set_capability(input_right, EV_KEY, BTN_1);
	input_set_capability(input_right, EV_KEY, BTN_2);
	input_set_capability(input_right, EV_KEY, BTN_3);
	input_set_capability(input_right, EV_KEY, BTN_4); //joystic 1 only use this for fire button
	input_set_capability(input_right, EV_KEY, BTN_5);
	input_set_capability(input_right, EV_KEY, BTN_6);
	input_set_capability(input_right, EV_KEY, BTN_7);

	input_set_capability(input_right, EV_ABS, ABS_X);
	input_set_abs_params(input_right, ABS_X, -32768, 32767, 0, 0);
	input_set_capability(input_right, EV_ABS, ABS_Y);
	input_set_abs_params(input_right, ABS_Y, -32768, 32767, 0, 0);
	input_set_capability(input_right, EV_ABS, ABS_Z);
	input_set_abs_params(input_right, ABS_Z, -32768, 32767, 0, 0);

//	set_bit(EV_REL, input_right->evbit);
//	set_bit(REL_X, input_right->relbit);
//	set_bit(REL_Y, input_right->relbit);
//	set_bit(EV_KEY, input_right->evbit);
//	set_bit(BTN_LEFT, input_right->keybit);
//	set_bit(BTN_MIDDLE, input_right->keybit);
//	set_bit(BTN_RIGHT, input_right->keybit);

	input_register_device(input_right);


	setup_timer(&atari_i2c_timer, atari_i2c_key_timer, 0);
	INIT_WORK(&atari_i2c_work, atari_i2c_keys_report_event);
	mod_timer(&atari_i2c_timer, (jiffies+HZ/10));
	return 0;
}

static int bte_i2c_remove(struct i2c_client *client){
	return 0;
}

static int bte_i2c_detect(struct i2c_client *client, struct i2c_board_info* info){
	printk("stanley bte_i2c_detect\n");
	return 0;
}

static struct i2c_driver bte_i2c_driver = {
	.driver = {
		.name = "bte_i2c",
		.owner = THIS_MODULE,
	},
	.probe = bte_i2c_probe,
	.remove = bte_i2c_remove,
	.detect = bte_i2c_detect,
	.id_table = bte_i2c_ids,
};
#if 0
static ssize_t write_virmouse(struct device *dev,
                              struct device_attribute *attr,
                              const char *buffer, size_t count){
	return 0;
}

/* Attach the sysfs write method */
DEVICE_ATTR(vmevent, 0644, NULL, write_virmouse);

/* Attribute Descriptor */
static struct attribute *virmouse_attrs[] = {
	&dev_attr_vmevent.attr,
	NULL
};

/* Attribute group */
static struct attribute_group virmouse_attr_group = {
	.attrs = virmouse_attrs,
};
#endif
static int __init atari_key_init(void){
	int err =0, irq_number = 0;
	
	printk("atari_key_init \n");

	atari_gpio_script_init();
	atari_dev = input_allocate_device();
	if (!atari_dev) {
		printk( "atari: not enough memory for input device\n");
		err = -ENOMEM;
		goto fail_memory;
	}

	atari_dev->name = "atari-key";
	atari_dev->phys = "atari/input0";
	atari_dev->id.bustype = BUS_HOST;
	atari_dev->id.vendor = 0x0001;
	atari_dev->id.product = 0x0001;
	atari_dev->id.version = 0x0100;

	atari_dev->evbit[0] = BIT_MASK(EV_KEY);
	input_set_capability(atari_dev, EV_KEY, KEY_ENTER);

	err = input_register_device(atari_dev);
	if (err){
		goto fail2;
	}

	setup_timer(&atari_gpio_timer, atari_gpio_key_timer, 0);
	INIT_WORK(&atari_gpio_work, atari_gpio_keys_report_event);
	mod_timer(&atari_gpio_timer, (jiffies+HZ/10));

	err = i2c_add_driver(&bte_i2c_driver);
	if(err < 0) printk("stanley bte_i2c driver failed.\n");
	else printk("stanley bte_i2c driver successed.\n");
/*
	virmouse_dev = platform_device_register_simple("virmouse", -1, NULL, 0);
	sysfs_create_group(&virmouse_dev->dev.kobj, &virmouse_attr_group);
	input_mouse = input_allocate_device();
	set_bit(EV_REL, input_mouse->evbit);
	set_bit(REL_X, input_mouse->relbit);
	set_bit(REL_Y, input_mouse->relbit);
*/	

	printk("atari_init end.\n");
	return 0;

fail3:
	input_unregister_device(atari_dev);
fail2:
	input_free_device(atari_dev);
fail_memory:
	printk("atari_init failed.\n");

	return err;
}

static void __exit atari_key_exit(void){
	i2c_del_driver(&bte_i2c_driver);
	input_unregister_device(atari_dev);
}

module_init(atari_key_init);
module_exit(atari_key_exit);

MODULE_AUTHOR("<stanley.ho@elansat.com>");
MODULE_DESCRIPTION("atari gpio power key driver");
MODULE_LICENSE("GPL");


