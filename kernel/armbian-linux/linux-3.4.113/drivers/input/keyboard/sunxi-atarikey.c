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

/**
 * This structure contains all information required for reading from GPIO
 * pins and sending applicable events.
 * These are used for console keys (select, reset, etc) only.
 */
struct gpio_key_t
{
  struct gpio_config cfg;  // gpio handler
  unsigned int code;       // event to send when pin state changes
  int pin_state;           // current state of the pin: 0 (pressed) or 1 (released)
  int swap_signal;         // whether to swap the signal when sending the event
  char desc[15];           // description used in debugging
};
static struct gpio_key_t gpio_key[10];  // there are 10 pins; only 9 are used


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void atari_gpio_key_timer(unsigned long _data){
  schedule_work(&atari_gpio_work);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void atari_gpio_assign_key(unsigned int i, char* pin_id,
                                  unsigned int code, const char* desc)
{
  script_item_u val;
  script_item_value_type_e type;

  type = script_get_item("gpio_para", pin_id, &val);
  gpio_key[i].cfg = val.gpio;
  gpio_key[i].code = code;
  gpio_key[i].pin_state = 1;
  strncpy(gpio_key[i].desc, desc, 14);

  /**
   * Some keys seem to be wired backwards; we detect that here
   * Signal should start in state 1 (released); if not, it needs to be swapped
   * The function that actually reads the pins detects this and swaps it there
   */
  gpio_key[i].swap_signal = __gpio_get_value(gpio_key[i].cfg.gpio) == 1 ? 0 : 1;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void atari_gpio_script_init(void)
{
  atari_gpio_assign_key(0, "gpio_pin_1",  KEY_F6,        "PD06[p1 diff]");
  atari_gpio_assign_key(1, "gpio_pin_2",  0,             "PD07[??]"     );
  atari_gpio_assign_key(2, "gpio_pin_3",  KEY_BACKSPACE, "PD08[fry]"    );
  atari_gpio_assign_key(3, "gpio_pin_4",  KEY_F2,        "PD09[reset]"  );
  atari_gpio_assign_key(4, "gpio_pin_5",  KEY_F4,        "PD12[color]"  );
  atari_gpio_assign_key(5, "gpio_pin_6",  KEY_F9,        "PD11[save]"   );
  atari_gpio_assign_key(6, "gpio_pin_7",  KEY_F13,       "PD10[aspect]" );
  atari_gpio_assign_key(7, "gpio_pin_8",  KEY_F11,       "PD13[load]"   );
  atari_gpio_assign_key(8, "gpio_pin_9",  KEY_F1,        "PD14[select]" );
  atari_gpio_assign_key(9, "gpio_pin_10", KEY_F8,        "PD15[p2 diff]");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//#define DEBUG_KEYS
static void atari_gpio_keys_report_event(struct work_struct* work)
{
  for(unsigned int i = 0; i < 10; ++i)
  {
    int state = __gpio_get_value(gpio_key[i].cfg.gpio);
    if(gpio_key[i].swap_signal)
      state ^= 1;

    if(state != gpio_key[i].pin_state)
    {
      gpio_key[i].pin_state = state;
      input_report_key(atari_dev, gpio_key[i].code, state ^ 1);
      input_sync(atari_dev);
    #if defined(DEBUG_KEYS)
      printk("GPIO key: %-14s key[%d] = %d\n", gpio_key[i].desc, gpio_key[i].cfg.gpio, state);
    #endif
    }
  }
  mod_timer( &atari_gpio_timer, (jiffies) + 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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

#define AXIS_MAX  32767
#define AXIS_MIN -32768
#define AXIS_CENTER 0

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
//    printk("stanley ret=%d len=%d, right=0x%x, R_PA=0x%x, R_PB=0x%x, left=0x%x, L_PA=0x%x, L_PB=0x%x\n",
//      ret, g_i2c_key.len, g_i2c_key.right.key, g_i2c_key.paddle0, g_i2c_key.paddle1, g_i2c_key.left.key, g_i2c_key.paddle2, g_i2c_key.paddle3);
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
              input_event(input_left, EV_ABS, ABS_Y, AXIS_CENTER);
            }else{
              input_event(input_left, EV_ABS, ABS_Y, AXIS_MIN);
            }
            input_sync(input_left);
            //printk("Left KEY_UP\n");
          }
          if(g_i2c_key.left.u.k1 != old_key.left.u.k1){ //Left
            if(g_i2c_key.left.u.k1 == 1){
              input_event(input_left, EV_ABS, ABS_X, AXIS_CENTER);
            }else{
              input_event(input_left, EV_ABS, ABS_X, AXIS_MIN);
            }
            input_sync(input_left);
            //printk("Left KEY_LEFT\n");
          }
          if(g_i2c_key.left.u.k2 != old_key.left.u.k2){ //Down
            if(g_i2c_key.left.u.k2 == 1){
              input_event(input_left, EV_ABS, ABS_Y, AXIS_CENTER);
            }else{
              input_event(input_left, EV_ABS, ABS_Y, AXIS_MAX);
            }
            input_sync(input_left);
            //printk("Left KEY_DOWN\n");
          }
          if(g_i2c_key.left.u.k3 != old_key.left.u.k3){ //Right
            if(g_i2c_key.left.u.k3 == 1){
              input_event(input_left, EV_ABS, ABS_X, AXIS_CENTER);
            }else{
              input_event(input_left, EV_ABS, ABS_X, AXIS_MAX);
            }
            input_sync(input_left);
            //printk("Left KEY_RIGHT\n");
          }
          if(g_i2c_key.left.u.k5 != old_key.left.u.k5){ //Fire
            if(g_i2c_key.left.u.k5 == 1){
              input_event(input_left, EV_KEY, BTN_0, 0);
            }else{
              input_event(input_left, EV_KEY, BTN_0, 1);
            }
            input_sync(input_left);
            //printk("Left KEY_SPACE\n");
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
              input_event(input_right, EV_ABS, ABS_Y, AXIS_CENTER);
            }else{
              input_event(input_right, EV_ABS, ABS_Y, AXIS_MIN);
            }
            input_sync(input_right);
            //printk("Right UP\n");
          }
          if(g_i2c_key.right.u.k1 != old_key.right.u.k1){ //Left
            if(g_i2c_key.right.u.k1 == 1){
              input_event(input_right, EV_ABS, ABS_X, AXIS_CENTER);
            }else{
              input_event(input_right, EV_ABS, ABS_X, AXIS_MIN);
            }
            input_sync(input_right);
            //printk("Right LEFT\n");
          }
          if(g_i2c_key.right.u.k2 != old_key.right.u.k2){ //Down
            if(g_i2c_key.right.u.k2 == 1){
              input_event(input_right, EV_ABS, ABS_Y, AXIS_CENTER);
            }else{
              input_event(input_right, EV_ABS, ABS_Y, AXIS_MAX);
            }
            input_sync(input_right);
            //printk("Right DOWN\n");
          }
          if(g_i2c_key.right.u.k3 != old_key.right.u.k3){ //Right
            if(g_i2c_key.right.u.k3 == 1){
              input_event(input_right, EV_ABS, ABS_X, AXIS_CENTER);
            }else{
              input_event(input_right, EV_ABS, ABS_X, AXIS_MAX);
            }
            input_sync(input_right);
            //printk("Right Right\n");
          }
          if(g_i2c_key.right.u.k5 != old_key.right.u.k5){ //Fire
            if(g_i2c_key.right.u.k5 == 1){
              input_event(input_right, EV_KEY, BTN_0, 0);
            }else{
              input_event(input_right, EV_KEY, BTN_0, 1);
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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

  input_set_capability(input_left, EV_KEY, BTN_0); //PaddleA Fire Button, Joy0 Fire0
  input_set_capability(input_left, EV_KEY, BTN_1); //PaddleB Fire Button, Joy0 Fire1
  input_set_capability(input_left, EV_KEY, BTN_2);
  input_set_capability(input_left, EV_KEY, BTN_3);
  input_set_capability(input_left, EV_KEY, BTN_4);
  input_set_capability(input_left, EV_KEY, BTN_5);
  input_set_capability(input_left, EV_KEY, BTN_6);
  input_set_capability(input_left, EV_KEY, BTN_7);

  input_set_capability(input_left, EV_ABS, ABS_X);
  input_set_abs_params(input_left, ABS_X, AXIS_MIN, AXIS_MAX, 0, 0);
  input_set_capability(input_left, EV_ABS, ABS_Y);
  input_set_abs_params(input_left, ABS_Y, AXIS_MIN, AXIS_MAX, 0, 0);
  input_register_device(input_left);

  input_right->name = "i2c_controller";
  input_right->phys = "i2c_controller/input1";
  input_right->id.bustype = BUS_HOST;
  input_right->id.vendor = 0x0001;
  input_right->id.product = 0x0001;
  input_right->id.version = 0x0100;

  input_set_capability(input_right, EV_KEY, BTN_0); //PaddleA Fire Button, Joy1 Fire0
  input_set_capability(input_right, EV_KEY, BTN_1); //PaddleB Fire Button, Joy1 Fire1
  input_set_capability(input_right, EV_KEY, BTN_2);
  input_set_capability(input_right, EV_KEY, BTN_3);
  input_set_capability(input_right, EV_KEY, BTN_4);
  input_set_capability(input_right, EV_KEY, BTN_5);
  input_set_capability(input_right, EV_KEY, BTN_6);
  input_set_capability(input_right, EV_KEY, BTN_7);

  input_set_capability(input_right, EV_ABS, ABS_X);
  input_set_abs_params(input_right, ABS_X, AXIS_MIN, AXIS_MAX, 0, 0);
  input_set_capability(input_right, EV_ABS, ABS_Y);
  input_set_abs_params(input_right, ABS_Y, AXIS_MIN, AXIS_MAX, 0, 0);
  input_register_device(input_right);


  setup_timer(&atari_i2c_timer, atari_i2c_key_timer, 0);
  INIT_WORK(&atari_i2c_work, atari_i2c_keys_report_event);
  mod_timer(&atari_i2c_timer, (jiffies) + 1);
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static int bte_i2c_remove(struct i2c_client *client){
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static int bte_i2c_detect(struct i2c_client *client, struct i2c_board_info* info){
  //printk("stanley bte_i2c_detect\n");
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
  input_set_capability(atari_dev, EV_KEY, KEY_F1);  // Select
  input_set_capability(atari_dev, EV_KEY, KEY_F2);  // Reset
  input_set_capability(atari_dev, EV_KEY, KEY_F4);  // Color/BW
  input_set_capability(atari_dev, EV_KEY, KEY_F6);  // P1 difficulty
  input_set_capability(atari_dev, EV_KEY, KEY_F8);  // P2 difficulty
  input_set_capability(atari_dev, EV_KEY, KEY_F9);  // Save state
  input_set_capability(atari_dev, EV_KEY, KEY_F11); // Load state
  input_set_capability(atari_dev, EV_KEY, KEY_F13); // Aspect (4:3/16:9)
  input_set_capability(atari_dev, EV_KEY, KEY_BACKSPACE); // Fry

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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void __exit atari_key_exit(void){
  i2c_del_driver(&bte_i2c_driver);
  input_unregister_device(atari_dev);
}

module_init(atari_key_init);
module_exit(atari_key_exit);

MODULE_AUTHOR("Stella Team");
MODULE_DESCRIPTION("atari gpio key and joystick driver");
MODULE_LICENSE("GPL");
