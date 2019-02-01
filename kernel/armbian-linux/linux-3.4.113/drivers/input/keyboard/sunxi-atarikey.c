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
static struct work_struct atari_gpio_work;
static struct timer_list atari_gpio_timer;
static struct i2c_client *g_client = NULL;
static struct work_struct atari_i2c_work;
static struct timer_list atari_i2c_timer;

static struct gpio_config p1_diffcult_key_io;//PD06
static struct gpio_config pd07_key_io;//PD07
static struct gpio_config fly_key_io;//PD08
static struct gpio_config reset_key_io;//PD09
static struct gpio_config scale_key_io;//PD10
static struct gpio_config save_key_io;//PD11
static struct gpio_config color_key_io;//PD12
static struct gpio_config load_key_io;//PD13
static struct gpio_config mode_key_io;//PD14
static struct gpio_config p2_diffcult_key_io;//PD15 
static int p1_diffcult_key_status = 1;
static int pd07_key_status = 1;
static int fly_key_status = 1;
static int reset_key_status = 1;
static int scale_key_status = 1;
static int save_key_status = 1;
static int color_key_status = 1;
static int load_key_status = 1;
static int mode_key_status = 1;
static int p2_diffcult_key_status = 1;

static void atari_gpio_key_timer(unsigned long _data){
	schedule_work(&atari_gpio_work);
}

static void atari_gpio_keys_report_event(struct work_struct *work){
	int val = 0;

	val = __gpio_get_value(p1_diffcult_key_io.gpio);
	if(val != p1_diffcult_key_status){
		static int p1_switch = 1;
		p1_diffcult_key_status = val;
		if(p1_switch == 1){
			input_report_key(atari_dev, KEY_F6, (val == 0) ? 1 : 0);
			input_sync(atari_dev);
			if(val == 1)
				p1_switch = 0;
		}else{
			input_report_key(atari_dev, KEY_F5, (val == 0) ? 1 : 0);
			input_sync(atari_dev);
			if(val == 1)
				p1_switch = 1;
		}
		//printk("stanley PD06[p1 diff] key[%d] = %d\n", p1_diffcult_key_io.gpio, val);
	}
	val = __gpio_get_value(pd07_key_io.gpio);
	if(val != pd07_key_status){
		pd07_key_status = val;
		//printk("stanley PD07[??] key[%d] = %d\n", pd07_key_io.gpio, val);
	}
	val = __gpio_get_value(fly_key_io.gpio);
	if(val != fly_key_status){
		fly_key_status = val;
		input_report_key(atari_dev, KEY_BACKSPACE, (val == 0) ? 1 : 0);
		input_sync(atari_dev);
		//printk("stanley PD08[fly] key[%d] = %d\n", fly_key_io.gpio, val);
	}
	val = __gpio_get_value(reset_key_io.gpio);
	if(val != reset_key_status){
		reset_key_status = val;
		input_report_key(atari_dev, KEY_F2, (val == 0) ? 1 : 0);
		input_sync(atari_dev);
		//printk("stanley PD09[reset] key[%d] = %d\n", reset_key_io.gpio, val);
	}
	val = __gpio_get_value(scale_key_io.gpio);
	if(val != scale_key_status){
		scale_key_status = val;
		input_report_key(atari_dev, KEY_RIGHTALT, (val == 0) ? 1 : 0);
		input_sync(atari_dev);
		input_report_key(atari_dev, KEY_ENTER, (val == 0) ? 1 : 0);
		input_sync(atari_dev);
		//printk("stanley PD10[scale] key[%d] = %d\n", scale_key_io.gpio, val);
	}
	val = __gpio_get_value(save_key_io.gpio);
	if(val != save_key_status){
		save_key_status = val;
		input_report_key(atari_dev, KEY_F9, (val == 0) ? 1 : 0);
		input_sync(atari_dev);
		//printk("stanley PD11[save] key[%d] = %d\n", save_key_io.gpio, val);
	}
	val = __gpio_get_value(color_key_io.gpio);
	if(val != color_key_status){
		static int color = 1;
		color_key_status = val;
		if(color == 1){
			input_report_key(atari_dev, KEY_F4, (val == 0) ? 1 : 0);
			input_sync(atari_dev);
			if(val == 1)
				color = 0;
		}else{
			input_report_key(atari_dev, KEY_F3, (val == 0) ? 1 : 0);
			input_sync(atari_dev);
			if(val == 1)
				color = 1;
		}
		//printk("stanley PD12[color] key[%d] = %d\n", color_key_io.gpio, val);
	}
	val = __gpio_get_value(load_key_io.gpio);
	if(val != load_key_status){
		load_key_status = val;
		input_report_key(atari_dev, KEY_F11, (val == 0) ? 1 : 0);
		input_sync(atari_dev);
		//printk("stanley PD13[load] key[%d] = %d\n", load_key_io.gpio, val);
	}
	val = __gpio_get_value(mode_key_io.gpio);
	if(val != mode_key_status){
		input_report_key(atari_dev, KEY_F1, (val == 0) ? 1 : 0);
		input_sync(atari_dev);
		mode_key_status = val;
		//printk("stanley PD14[mode] key[%d] = %d\n", mode_key_io.gpio, val);
	}
	val = __gpio_get_value(p2_diffcult_key_io.gpio);
	if(val != p2_diffcult_key_status){
		static int p2_switch = 1;
		p2_diffcult_key_status = val;
		if(p2_switch == 1){
			input_report_key(atari_dev, KEY_F8, (val == 0) ? 1 : 0);
			input_sync(atari_dev);
			if(val == 1)
				p2_switch = 0;
		}else{
			input_report_key(atari_dev, KEY_F7, (val == 0) ? 1 : 0);
			input_sync(atari_dev);
			if(val == 1)
				p2_switch = 1;
		}
		//printk("stanley PD15[p2 diff] key[%d] = %d\n", p2_diffcult_key_io.gpio, val);
	}


	mod_timer( &atari_gpio_timer, (jiffies) + 1);
}

static int atari_gpio_script_init(void){
	script_item_u	val;
	script_item_value_type_e  type;

	type = script_get_item("gpio_para", "gpio_pin_1", &val);
	p1_diffcult_key_io = val.gpio;
	type = script_get_item("gpio_para", "gpio_pin_2", &val);
	pd07_key_io = val.gpio;
	type = script_get_item("gpio_para", "gpio_pin_3", &val);
	fly_key_io = val.gpio;
	type = script_get_item("gpio_para", "gpio_pin_4", &val);
	reset_key_io = val.gpio;
	type = script_get_item("gpio_para", "gpio_pin_5", &val);
	color_key_io = val.gpio;
	type = script_get_item("gpio_para", "gpio_pin_6", &val);
	save_key_io = val.gpio;
	type = script_get_item("gpio_para", "gpio_pin_7", &val);
	scale_key_io = val.gpio;
	type = script_get_item("gpio_para", "gpio_pin_8", &val);
	load_key_io = val.gpio;
	type = script_get_item("gpio_para", "gpio_pin_9", &val);
	mode_key_io = val.gpio;
	type = script_get_item("gpio_para", "gpio_pin_10", &val);
	p2_diffcult_key_io = val.gpio;

	return 0;
}


#define ATARI_CONTROLLER (0x52 << 1) //i2c slave address
static const struct i2c_device_id bte_i2c_ids[] = {
	{ "bte_i2c", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, bte_i2c_ids);

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
static int old_i2c_key_paddle0 = 0;//Right PaddleA
static int old_i2c_key_paddle1 = 0;//Right PaddleB
static int old_i2c_key_paddle2 = 0;//Left  PaddleA
static int old_i2c_key_paddle3 = 0;//Left  PaddleB

static void atari_i2c_keys_report_event(struct work_struct *work){
	static struct i2c_key old_key = { 0, .left.key = 0x2f, .right.key = 0x2f};
	int scale_value = 190;
	int noise_value = 3;
	int paddle_mid_value = 0x70;
	int cur_paddle0 = 0;
	int cur_paddle1 = 0;
	int cur_paddle2 = 0;
	int cur_paddle3 = 0;

        g_i2c_key.left.key = 0;
        g_i2c_key.right.key = 0;

	if(g_client){
		char data[4];
		int ret = 0;

		data[0] = 0x0;
		i2c_master_send(g_client, data, 1);
		ret = i2c_master_recv(g_client, (char*)&g_i2c_key, 7); //return 7 bytes data from MCU
//		printk("stanley ret=%d len=%d, right=0x%x, R_PA=0x%x, R_PB=0x%x, left=0x%x, L_PA=0x%x, L_PB=0x%x\n", 
//			ret, g_i2c_key.len, g_i2c_key.right.key, g_i2c_key.paddle0, g_i2c_key.paddle1, g_i2c_key.left.key, g_i2c_key.paddle2, g_i2c_key.paddle3);
		if(ret == 7){
			if(g_i2c_key.paddle2 != 0 || g_i2c_key.paddle3 != 0){//Left PaddleA/PaddleB
				if((g_i2c_key.paddle2 != 0)){//Left PaddleA
					cur_paddle2 = g_i2c_key.paddle2 - paddle_mid_value;
					if(abs(cur_paddle2 - old_i2c_key_paddle2) > noise_value){
						input_event(input_left, EV_ABS, ABS_X, cur_paddle2 * scale_value);
						input_sync(input_left);
						old_i2c_key_paddle2 = cur_paddle2;
						//printk("Left PaddleA %d\n", cur_paddle2 * scale_value);
					}
				}
				if((g_i2c_key.paddle3 != 0)){//Left PaddleB
					cur_paddle3 = g_i2c_key.paddle3 - paddle_mid_value;
					if(abs(cur_paddle3 - old_i2c_key_paddle3) > noise_value){
						input_event(input_left, EV_ABS, ABS_Y, cur_paddle3 * scale_value);
						input_sync(input_left);
						old_i2c_key_paddle3 = cur_paddle3;
						//printk("Left PaddleB %d\n", cur_paddle3 * scale_value);
					}
				}
				if(g_i2c_key.left.u.k5 != old_key.left.u.k5){ //Hyperkin Left Paddle FireA
					if(g_i2c_key.left.u.k5 == 1){
						input_event(input_left, EV_KEY, BTN_0, 0);
					}else{
						input_event(input_left, EV_KEY, BTN_0, 1);
					}
					input_sync(input_left);
					//printk("Left Paddle FireA Up\n");
				}
				if(g_i2c_key.left.u.k3 != old_key.left.u.k3){ //Left Paddle FireA
					if(g_i2c_key.left.u.k3 == 1){
						input_event(input_left, EV_KEY, BTN_0, 0);
					}else{
						input_event(input_left, EV_KEY, BTN_0, 1);
					}
					input_sync(input_left);
					//printk("Left Paddle FireA Up\n");
				}
				if(g_i2c_key.left.u.k1 != old_key.left.u.k1){ //Left Paddle FireB
					if(g_i2c_key.left.u.k1 == 1){
						input_event(input_left, EV_KEY, BTN_1, 0);
					}else{
						input_event(input_left, EV_KEY, BTN_1, 1);
					}
					input_sync(input_left);
					//printk("Left Paddle FireB\n");
				}
			}else{
				if( g_i2c_key.left.key != old_key.left.key ){
					if(g_i2c_key.left.u.k0 != old_key.left.u.k0){ //Up
						if(g_i2c_key.left.u.k0 == 1){
							input_event(input_left, EV_KEY, KEY_UP, 0);
						}else{
							input_event(input_left, EV_KEY, KEY_UP, 1);
						}
						input_sync(input_left);
						//printk("Left KEY_UP\n");
					}
					if(g_i2c_key.left.u.k1 != old_key.left.u.k1){ //Left
						if(g_i2c_key.left.u.k1 == 1){
							input_event(input_left, EV_KEY, KEY_LEFT, 0);
						}else{
							input_event(input_left, EV_KEY, KEY_LEFT, 1);
						}
						input_sync(input_left);
						//printk("Left KEY_LEFT\n");
					}
					if(g_i2c_key.left.u.k2 != old_key.left.u.k2){ //Down
						if(g_i2c_key.left.u.k2 == 1){
							input_event(input_left, EV_KEY, KEY_DOWN, 0);
						}else{
							input_event(input_left, EV_KEY, KEY_DOWN, 1);
						}
						input_sync(input_left);
						//printk("Left KEY_DOWN\n");
					}
					if(g_i2c_key.left.u.k3 != old_key.left.u.k3){ //Right
						if(g_i2c_key.left.u.k3 == 1){
							input_event(input_left, EV_KEY, KEY_RIGHT, 0);
						}else{
							input_event(input_left, EV_KEY, KEY_RIGHT, 1);
						}
						input_sync(input_left);
						//printk("Left KEY_RIGHT\n");
					}
					if(g_i2c_key.left.u.k5 != old_key.left.u.k5){ //Fire
						if(g_i2c_key.left.u.k5 == 1){
							input_event(input_left, EV_KEY, KEY_ENTER, 0);
						}else{
							input_event(input_left, EV_KEY, KEY_ENTER, 1);
						}
						input_sync(input_left);
						//printk("Left KEY_ENTER\n");
					}
				}
			}


			if(g_i2c_key.paddle0 != 0 || g_i2c_key.paddle1 != 0){//Right PaddleA/PaddleB
				if((g_i2c_key.paddle0 != 0)){//Right PaddleA
					cur_paddle0 = g_i2c_key.paddle0 - paddle_mid_value;
					if(abs(cur_paddle0 - old_i2c_key_paddle0) > noise_value){
						input_event(input_right, EV_ABS, ABS_X, cur_paddle0 * scale_value);
						input_sync(input_right);
						old_i2c_key_paddle0 = cur_paddle0;
						//printk("Right PaddleA %d\n", cur_paddle0 * scale_value);
					}
				}
				if((g_i2c_key.paddle1 != 0)){//Right PaddleB
					cur_paddle1 = g_i2c_key.paddle1 - paddle_mid_value;
					if(abs(cur_paddle1 - old_i2c_key_paddle1) > noise_value){
						input_event(input_right, EV_ABS, ABS_Y, cur_paddle1 * scale_value);
						input_sync(input_right);
						old_i2c_key_paddle1 = cur_paddle1;
						//printk("Right PaddleB %d\n", cur_paddle1 * scale_value);
					}
				}
				if(g_i2c_key.right.u.k5 != old_key.right.u.k5){ //Hyperkin Right Paddle FireA
					if(g_i2c_key.right.u.k5 == 1){
						input_event(input_right, EV_KEY, BTN_0, 0);
					}else{
						input_event(input_right, EV_KEY, BTN_0, 1);
					}
					input_sync(input_right);
					//printk("Right Paddle FireA\n");
				}
				if(g_i2c_key.right.u.k3 != old_key.right.u.k3){ //Right Paddle FireA
					if(g_i2c_key.right.u.k3 == 1){
						input_event(input_right, EV_KEY, BTN_0, 0);
					}else{
						input_event(input_right, EV_KEY, BTN_0, 1);
					}
					input_sync(input_right);
					//printk("Right Paddle FireA\n");
				}
				if(g_i2c_key.right.u.k1 != old_key.right.u.k1){ //Right Paddle FireB
					if(g_i2c_key.right.u.k1 == 1){
						input_event(input_right, EV_KEY, BTN_1, 0);
					}else{
						input_event(input_right, EV_KEY, BTN_1, 1);
					}
					input_sync(input_right);
					//printk("Right Paddle FireB\n");
				}
			}else{
				if( g_i2c_key.right.key != old_key.right.key ){
					if(g_i2c_key.right.u.k0 != old_key.right.u.k0){ //Up
						if(g_i2c_key.right.u.k0 == 1){
							input_event(input_right, EV_KEY, KEY_Y, 0);
						}else{
							input_event(input_right, EV_KEY, KEY_Y, 1);
						}
						input_sync(input_right);
						//printk("Right UP\n");
					}
					if(g_i2c_key.right.u.k1 != old_key.right.u.k1){ //Left
						if(g_i2c_key.right.u.k1 == 1){
							input_event(input_right, EV_KEY, KEY_G, 0);
						}else{
							input_event(input_right, EV_KEY, KEY_G, 1);
						}
						input_sync(input_right);
						//printk("Right LEFT\n");
					}
					if(g_i2c_key.right.u.k2 != old_key.right.u.k2){ //Down
						if(g_i2c_key.right.u.k2 == 1){
							input_event(input_right, EV_KEY, KEY_H, 0);
						}else{
							input_event(input_right, EV_KEY, KEY_H, 1);
						}
						input_sync(input_right);
						//printk("Right DOWN\n");
					}
					if(g_i2c_key.right.u.k3 != old_key.right.u.k3){ //Right
						if(g_i2c_key.right.u.k3 == 1){
							input_event(input_right, EV_KEY, KEY_J, 0);
						}else{
							input_event(input_right, EV_KEY, KEY_J, 1);
						}
						input_sync(input_right);
						//printk("Right Right\n");
					}
					if(g_i2c_key.right.u.k5 != old_key.right.u.k5){ //Fire
						if(g_i2c_key.right.u.k5 == 1){
							input_event(input_right, EV_KEY, KEY_F, 0);
						}else{
							input_event(input_right, EV_KEY, KEY_F, 1);
						}
						input_sync(input_right);
						//printk("Right Fire\n");
					}
				}
			}
		}
		old_key.left.key = g_i2c_key.left.key;
		old_key.right.key = g_i2c_key.right.key;
	}
	mod_timer( &atari_i2c_timer, (jiffies) + 1);
}

static int bte_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id){
	//printk("stanley bte_i2c_probe\n");
	g_client = client;

	input_left = input_allocate_device();
	input_right = input_allocate_device();

	input_left->name = "i2c_controller";
	input_left->phys = "i2c_controller/input0";
	input_left->id.bustype = BUS_HOST;
	input_left->id.vendor = 0x0001;
	input_left->id.product = 0x0001;
	input_left->id.version = 0x0100;

	input_set_capability(input_left, EV_KEY, BTN_0);    //PaddleA Fire Button
	input_set_capability(input_left, EV_KEY, BTN_1);    //PaddleB Fire Button
	input_set_capability(input_left, EV_KEY, BTN_2);
	input_set_capability(input_left, EV_KEY, BTN_3);
	input_set_capability(input_left, EV_KEY, BTN_4);
	input_set_capability(input_left, EV_KEY, BTN_5);
	input_set_capability(input_left, EV_KEY, BTN_6);
	input_set_capability(input_left, EV_KEY, BTN_7);
	input_set_capability(input_left, EV_KEY, KEY_UP);   //Josytick0 HAT UP
	input_set_capability(input_left, EV_KEY, KEY_DOWN); //Joystick0 HAT DOWN
	input_set_capability(input_left, EV_KEY, KEY_LEFT); //Joystick0 HAT LEFT
	input_set_capability(input_left, EV_KEY, KEY_RIGHT);//Joystick0 HAT RIGHT
	input_set_capability(input_left, EV_KEY, KEY_ENTER);//Joystick0 HAT FIRE

	input_set_capability(input_left, EV_ABS, ABS_X);
	input_set_abs_params(input_left, ABS_X, -32768, 32767, 0, 0);
	input_set_capability(input_left, EV_ABS, ABS_Y);
	input_set_abs_params(input_left, ABS_Y, -32768, 32767, 0, 0);
	input_register_device(input_left);

	input_right->name = "i2c_controller";
	input_right->phys = "i2c_controller/input1";
	input_right->id.bustype = BUS_HOST;
	input_right->id.vendor = 0x0001;
	input_right->id.product = 0x0001;
	input_right->id.version = 0x0100;

	input_set_capability(input_right, EV_KEY, BTN_0); //PaddleA Fire Button
	input_set_capability(input_right, EV_KEY, BTN_1); //PaddleB Fire Button
	input_set_capability(input_right, EV_KEY, BTN_2);
	input_set_capability(input_right, EV_KEY, BTN_3);
	input_set_capability(input_right, EV_KEY, BTN_4);
	input_set_capability(input_right, EV_KEY, BTN_5);
	input_set_capability(input_right, EV_KEY, BTN_6);
	input_set_capability(input_right, EV_KEY, BTN_7);
	input_set_capability(input_right, EV_KEY, KEY_Y); //Joystick1 HAT UP
	input_set_capability(input_right, EV_KEY, KEY_H); //Joystick1 HAT DOWN
	input_set_capability(input_right, EV_KEY, KEY_G); //Joystick1 HAT LEFT
	input_set_capability(input_right, EV_KEY, KEY_J); //Joystick1 HAT RIGHT
	input_set_capability(input_right, EV_KEY, KEY_F); //Joystick1 HAT FIRE

	input_set_capability(input_right, EV_ABS, ABS_X);
	input_set_abs_params(input_right, ABS_X, -32768, 32767, 0, 0);
	input_set_capability(input_right, EV_ABS, ABS_Y);
	input_set_abs_params(input_right, ABS_Y, -32768, 32767, 0, 0);
	input_register_device(input_right);


	setup_timer(&atari_i2c_timer, atari_i2c_key_timer, 0);
	INIT_WORK(&atari_i2c_work, atari_i2c_keys_report_event);
	mod_timer(&atari_i2c_timer, (jiffies) + 1);
	return 0;
}

static int bte_i2c_remove(struct i2c_client *client){
	return 0;
}

static int bte_i2c_detect(struct i2c_client *client, struct i2c_board_info* info){
	//printk("stanley bte_i2c_detect\n");
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

static int __init atari_key_init(void){
	int err =0;
	
	//printk("atari_key_init \n");

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
	input_set_capability(atari_dev, EV_KEY, KEY_RIGHTALT);
	input_set_capability(atari_dev, EV_KEY, KEY_ENTER);
	input_set_capability(atari_dev, EV_KEY, KEY_F1);
	input_set_capability(atari_dev, EV_KEY, KEY_F2);
	input_set_capability(atari_dev, EV_KEY, KEY_F3);
	input_set_capability(atari_dev, EV_KEY, KEY_F4);
	input_set_capability(atari_dev, EV_KEY, KEY_F5);
	input_set_capability(atari_dev, EV_KEY, KEY_F6);
	input_set_capability(atari_dev, EV_KEY, KEY_F7);
	input_set_capability(atari_dev, EV_KEY, KEY_F8);
	input_set_capability(atari_dev, EV_KEY, KEY_F9);
	input_set_capability(atari_dev, EV_KEY, KEY_F11);
	input_set_capability(atari_dev, EV_KEY, KEY_BACKSPACE);

	err = input_register_device(atari_dev);
	if (err){
		goto fail2;
	}

	setup_timer(&atari_gpio_timer, atari_gpio_key_timer, 0);
	INIT_WORK(&atari_gpio_work, atari_gpio_keys_report_event);
	mod_timer(&atari_gpio_timer, (jiffies) + 1);

	err = i2c_add_driver(&bte_i2c_driver);
	if(err < 0) printk("bte_i2c driver failed.\n");
	else printk("bte_i2c driver successed.\n");

	printk("atari_init end.\n");
	return 0;

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
MODULE_DESCRIPTION("atari gpio key and joystick driver");
MODULE_LICENSE("GPL");
