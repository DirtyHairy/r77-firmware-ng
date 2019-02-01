#include "de/bsp_display.h"
#include "disp_sys_intf.h"
#include "asm/cacheflush.h"
#include <asm/system.h>
#include <linux/clk/sunxi.h>
#include <linux/clk-private.h>

/* cache flush flags */
#define  CACHE_FLUSH_I_CACHE_REGION       0
#define  CACHE_FLUSH_D_CACHE_REGION       1
#define  CACHE_FLUSH_CACHE_REGION         2
#define  CACHE_CLEAN_D_CACHE_REGION       3
#define  CACHE_CLEAN_FLUSH_D_CACHE_REGION 4
#define  CACHE_CLEAN_FLUSH_CACHE_REGION   5

/*
*******************************************************************************
*                     OSAL_CacheRangeFlush
*
* Description:
*    Cache flush
*
* Parameters:
*    address    :  start address to be flush
*    length     :  size
*    flags      :  flush flags
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void disp_sys_cache_flush(void*address, u32 length, u32 flags)
{
	if(address == NULL || length == 0) {
		return;
	}

	switch(flags) {
	case CACHE_FLUSH_I_CACHE_REGION:
	break;

	case CACHE_FLUSH_D_CACHE_REGION:
	break;

	case CACHE_FLUSH_CACHE_REGION:
	break;

	case CACHE_CLEAN_D_CACHE_REGION:
	break;

	case CACHE_CLEAN_FLUSH_D_CACHE_REGION:
	break;

	case CACHE_CLEAN_FLUSH_CACHE_REGION:
	break;

	default:
	break;
	}
	return;
}

/*
*******************************************************************************
*                     disp_sys_register_irq
*
* Description:
*    irq register
*
* Parameters:
*    irqno    	    ��input.  irq no
*    flags    	    ��input.
*    Handler  	    ��input.  isr handler
*    pArg 	        ��input.  para
*    DataSize 	    ��input.  len of para
*    prio	        ��input.    priority

*
* Return value:
*
*
* note:
*    typedef s32 (*ISRCallback)( void *pArg)��
*
*******************************************************************************
*/
int disp_sys_register_irq(u32 IrqNo, u32 Flags, void* Handler,void *pArg,u32 DataSize,u32 Prio)
{
	__inf("%s, irqNo=%d, Handler=0x%x, pArg=0x%x\n", __func__, IrqNo, (int)Handler, (int)pArg);
	return request_irq(IrqNo, (irq_handler_t)Handler, IRQF_DISABLED, "dispaly", pArg);
}

/*
*******************************************************************************
*                     disp_sys_unregister_irq
*
* Description:
*    irq unregister
*
* Parameters:
*    irqno    	��input.  irq no
*    handler  	��input.  isr handler
*    Argment 	��input.    para
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void disp_sys_unregister_irq(u32 IrqNo, void* Handler, void *pArg)
{
	free_irq(IrqNo, pArg);
}

/*
*******************************************************************************
*                     disp_sys_enable_irq
*
* Description:
*    enable irq
*
* Parameters:
*    irqno ��input.  irq no
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void disp_sys_enable_irq(u32 IrqNo)
{
	//enable_irq(IrqNo);
}

/*
*******************************************************************************
*                     disp_sys_disable_irq
*
* Description:
*    disable irq
*
* Parameters:
*     irqno ��input.  irq no
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void disp_sys_disable_irq(u32 IrqNo)
{
	//disable_irq(IrqNo);
}

void disp_sys_irqlock(void* lock, unsigned long *cpu_sr)
{
	spin_lock_irqsave((spinlock_t*)lock, *cpu_sr);
}

void disp_sys_irqunlock(void* lock, unsigned long *cpu_sr)
{
	spin_unlock_irqrestore((spinlock_t*)lock, *cpu_sr);
}

void disp_sys_lock(void* lock)
{
	mutex_lock((struct mutex *)lock);
}

void disp_sys_unlock(void* lock)
{
	mutex_unlock((struct mutex *)lock);
}

void * disp_sys_malloc(u32 Size)
{
	return kmalloc(Size, GFP_KERNEL | __GFP_ZERO);
}

void disp_sys_free(void *Addr)
{
	kfree(Addr);
}

/* returns: 0:invalid, 1: int; 2:str, 3: gpio */
int disp_sys_script_get_item(char *main_name, char *sub_name, int value[], int count)
{
	script_item_u   val;
	script_item_value_type_e  type;
	int ret = -1;

	type = script_get_item(main_name, sub_name, &val);
	if(SCIRPT_ITEM_VALUE_TYPE_INT == type) {
		ret = 0;
		*value = val.val;
		__inf("%s.%s=%d\n", main_name, sub_name, *value);
	}	else if(SCIRPT_ITEM_VALUE_TYPE_PIO == type) {
		disp_gpio_set_t *gpio_info = (disp_gpio_set_t *)value;

		ret = 0;
		gpio_info->gpio = val.gpio.gpio;
		gpio_info->mul_sel = val.gpio.mul_sel;
		gpio_info->pull = val.gpio.pull;
		gpio_info->drv_level = val.gpio.drv_level;
		gpio_info->data = val.gpio.data;
		memcpy(gpio_info->gpio_name, sub_name, strlen(sub_name)+1);
		__inf("%s.%s gpio=%d,mul_sel=%d,data:%d\n",main_name, sub_name, gpio_info->gpio, gpio_info->mul_sel, gpio_info->data);
	}	else if(SCIRPT_ITEM_VALUE_TYPE_STR == type) {
		memcpy((void*)value, (void*)val.str, strlen(val.str)+1);
		__inf("%s.%s=%s\n",main_name, sub_name, val.str);
	} else {
		ret = -1;
		__inf("fetch script data %s.%s fail\n", main_name, sub_name);
	}

	return type;
}
EXPORT_SYMBOL(disp_sys_script_get_item);

int disp_sys_get_ic_ver(void)
{
    return 0;
}

int disp_sys_gpio_request(disp_gpio_set_t *gpio_list, u32 group_count_max)
{
	int ret = 0;
	struct gpio_config pin_cfg;
	char   pin_name[32];
	u32 config;

	if(gpio_list == NULL)
		return 0;

	pin_cfg.gpio = gpio_list->gpio;
	pin_cfg.mul_sel = gpio_list->mul_sel;
	pin_cfg.pull = gpio_list->pull;
	pin_cfg.drv_level = gpio_list->drv_level;
	pin_cfg.data = gpio_list->data;
	ret = gpio_request(pin_cfg.gpio, NULL);
	if(0 != ret) {
		__wrn("%s failed, gpio_name=%s, gpio=%d, ret=%d\n", __func__, gpio_list->gpio_name, gpio_list->gpio, ret);
		return ret;
	} else {
		__inf("%s, gpio_name=%s, gpio=%d, ret=%d\n", __func__, gpio_list->gpio_name, gpio_list->gpio, ret);
	}
	ret = pin_cfg.gpio;

	if (!IS_AXP_PIN(pin_cfg.gpio)) {
		/* valid pin of sunxi-pinctrl,
		* config pin attributes individually.
		*/
		sunxi_gpio_to_name(pin_cfg.gpio, pin_name);
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, pin_cfg.mul_sel);
		pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (pin_cfg.pull != GPIO_PULL_DEFAULT) {
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, pin_cfg.pull);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
		if (pin_cfg.drv_level != GPIO_DRVLVL_DEFAULT) {
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, pin_cfg.drv_level);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
		if (pin_cfg.data != GPIO_DATA_DEFAULT) {
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, pin_cfg.data);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
	} else if (IS_AXP_PIN(pin_cfg.gpio)) {
		/* valid pin of axp-pinctrl,
		* config pin attributes individually.
		*/
		sunxi_gpio_to_name(pin_cfg.gpio, pin_name);
		if (pin_cfg.data != GPIO_DATA_DEFAULT) {
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, pin_cfg.data);
			pin_config_set(AXP_PINCTRL, pin_name, config);
		}
	} else {
		pr_warn("invalid pin [%d] from sys-config\n", pin_cfg.gpio);
	}

	return ret;
}
EXPORT_SYMBOL(disp_sys_gpio_request);

int disp_sys_gpio_request_simple(disp_gpio_set_t *gpio_list, u32 group_count_max)
{
	int ret = 0;
	struct gpio_config pin_cfg;

	if(gpio_list == NULL)
		return 0;

	pin_cfg.gpio = gpio_list->gpio;
	pin_cfg.mul_sel = gpio_list->mul_sel;
	pin_cfg.pull = gpio_list->pull;
	pin_cfg.drv_level = gpio_list->drv_level;
	pin_cfg.data = gpio_list->data;
	ret = gpio_request(pin_cfg.gpio, NULL);
	if(0 != ret) {
		__wrn("%s failed, gpio_name=%s, gpio=%d, ret=%d\n", __func__, gpio_list->gpio_name, gpio_list->gpio, ret);
		return ret;
	} else {
		__inf("%s, gpio_name=%s, gpio=%d, ret=%d\n", __func__, gpio_list->gpio_name, gpio_list->gpio, ret);
	}
	ret = pin_cfg.gpio;

	return ret;
}
int disp_sys_gpio_release(int p_handler, s32 if_release_to_default_status)
{
	if(p_handler) {
		gpio_free(p_handler);
	} else {
		__wrn("OSAL_GPIO_Release, hdl is NULL\n");
	}
	return 0;
}
EXPORT_SYMBOL(disp_sys_gpio_release);

/* direction: 0:input, 1:output */
int disp_sys_gpio_set_direction(u32 p_handler, u32 direction, const char *gpio_name)
{
	int ret = -1;

	if(p_handler) {
		if(direction) {
			s32 value;

			value = __gpio_get_value(p_handler);
			ret = gpio_direction_output(p_handler, value);
			if(ret != 0) {
				__wrn("gpio_direction_output fail!\n");
			}
		} else {
			ret = gpio_direction_input(p_handler);
			if(ret != 0) {
				__wrn("gpio_direction_input fail!\n");
			}
		}
	} else {
		__wrn("OSAL_GPIO_DevSetONEPIN_IO_STATUS, hdl is NULL\n");
		ret = -1;
	}
	return ret;
}

int disp_sys_gpio_get_value(u32 p_handler, const char *gpio_name)
{
	if(p_handler) {
		return __gpio_get_value(p_handler);
	}
	__wrn("OSAL_GPIO_DevREAD_ONEPIN_DATA, hdl is NULL\n");
	return -1;
}

int disp_sys_gpio_set_value(u32 p_handler, u32 value_to_gpio, const char *gpio_name)
{
	if(p_handler) {
		__gpio_set_value(p_handler, value_to_gpio);
	} else {
		__wrn("OSAL_GPIO_DevWRITE_ONEPIN_DATA, hdl is NULL\n");
	}

	return 0;
}

int disp_sys_power_enable(char *name)
{
	int ret = 0;
#ifdef CONFIG_AW_AXP
	struct regulator *regu= NULL;
	regu= regulator_get(NULL, name);
	if (IS_ERR(regu)) {
		__wrn("some error happen, fail to get regulator %s\n", name);
		goto exit;
	}

	//enalbe regulator
	ret = regulator_enable(regu);
	if (0 != ret) {
		__wrn("some error happen, fail to enable regulator %s!\n", name);
		goto exit1;
	} else {
		__inf("suceess to enable regulator %s!\n", name);
	}

exit1:
	//put regulater, when module exit
	regulator_put(regu);
exit:
#endif
	return ret;
}
EXPORT_SYMBOL(disp_sys_power_enable);

int disp_sys_power_disable(char *name)
{
	int ret = 0;
#ifdef CONFIG_AW_AXP
	struct regulator *regu= NULL;

	regu= regulator_get(NULL, name);
	if (IS_ERR(regu)) {
		__wrn("some error happen, fail to get regulator %s\n", name);
		goto exit;
	}

	//disalbe regulator
	ret = regulator_disable(regu);
	if (0 != ret) {
		__wrn("some error happen, fail to disable regulator %s!\n", name);
		goto exit1;
	} else {
		__inf("suceess to disable regulator %s!\n", name);
	}

exit1:
	//put regulater, when module exit
	regulator_put(regu);
exit:
#endif
	return ret;
}
EXPORT_SYMBOL(disp_sys_power_disable);

int disp_sys_pwm_request(u32 pwm_id)
{
	int ret = 0;
#ifdef CONFIG_PWM_SUNXI
	struct pwm_device *pwm_dev;

	pwm_dev = pwm_request(pwm_id, "lcd");

	if(NULL == pwm_dev || IS_ERR(pwm_dev)) {
		__wrn("disp_sys_pwm_request pwm %d fail!\n", pwm_id);
	} else {
		__inf("disp_sys_pwm_request pwm %d success!\n", pwm_id);
	}
	ret = (int)pwm_dev;
#endif
	return ret;
}

int disp_sys_pwm_free(int p_handler)
{
	int ret = 0;
#ifdef CONFIG_PWM_SUNXI
	struct pwm_device *pwm_dev;

	pwm_dev = (struct pwm_device *)p_handler;
	if(NULL == pwm_dev || IS_ERR(pwm_dev)) {
		__wrn("disp_sys_pwm_free, handle is NULL!\n");
		ret = -1;
	} else {
		pwm_free(pwm_dev);
		__inf("disp_sys_pwm_free pwm %d \n", pwm_dev->pwm);
	}
#endif
	return ret;
}

int disp_sys_pwm_enable(int p_handler)
{
	int ret = 0;
#ifdef CONFIG_PWM_SUNXI
	struct pwm_device *pwm_dev;

	pwm_dev = (struct pwm_device *)p_handler;
	if(NULL == pwm_dev || IS_ERR(pwm_dev)) {
		__wrn("disp_sys_pwm_Enable, handle is NULL!\n");
		ret = -1;
	} else {
		ret = pwm_enable(pwm_dev);
		__inf("disp_sys_pwm_Enable pwm %d \n", pwm_dev->pwm);
	}
#endif
	return ret;
}

int disp_sys_pwm_disable(int p_handler)
{
	int ret = 0;
#ifdef CONFIG_PWM_SUNXI
	struct pwm_device *pwm_dev;

	pwm_dev = (struct pwm_device *)p_handler;
	if(NULL == pwm_dev || IS_ERR(pwm_dev)) {
		__wrn("disp_sys_pwm_Disable, handle is NULL!\n");
		ret = -1;
	} else {
		pwm_disable(pwm_dev);
		__inf("disp_sys_pwm_Disable pwm %d \n", pwm_dev->pwm);
	}
#endif
	return ret;
}

int disp_sys_pwm_config(int p_handler, int duty_ns, int period_ns)
{
	int ret = 0;
#ifdef CONFIG_PWM_SUNXI
	struct pwm_device *pwm_dev;

	pwm_dev = (struct pwm_device *)p_handler;
	if(NULL == pwm_dev || IS_ERR(pwm_dev)) {
		__wrn("disp_sys_pwm_Config, handle is NULL!\n");
		ret = -1;
	} else {
		ret = pwm_config(pwm_dev, duty_ns, period_ns);
		__debug("disp_sys_pwm_Config pwm %d, <%d / %d> \n", pwm_dev->pwm, duty_ns, period_ns);
	}
#endif
	return ret;
}

int disp_sys_pwm_set_polarity(int p_handler, int polarity)
{
	int ret = 0;
#ifdef CONFIG_PWM_SUNXI
	struct pwm_device *pwm_dev;

	pwm_dev = (struct pwm_device *)p_handler;
	if(NULL == pwm_dev || IS_ERR(pwm_dev)) {
		__wrn("disp_sys_pwm_Set_Polarity, handle is NULL!\n");
		ret = -1;
	} else {
		ret = pwm_set_polarity(pwm_dev, polarity);
		__inf("disp_sys_pwm_Set_Polarity pwm %d, active %s\n", pwm_dev->pwm, (polarity==0)? "high":"low");
	}
#endif
	return ret;
}

int disp_sys_clk_set_rate(const char *id, unsigned long rate)
{
	struct clk* hclk = NULL;
	int ret = 0;

	hclk = clk_get(NULL, id);

	if(NULL == hclk || IS_ERR(hclk)) {
		__wrn("Fail to get handle for clock %s.\n", id);
		return -1;
	}

	if(rate == clk_get_rate(hclk)) {
		__inf("Sys clk %s rate is alreay %ld, not need to set.\n", id, rate);
		clk_put(hclk);
		return 0;
	}

	ret = clk_set_rate(hclk, rate);
	if(0 != ret)
		__wrn("Fail to set rate[%ld] for clock %s.\n", rate, id);
	else
		__inf("Success to set rate[%ld] for clock %s.\n", rate, id);

	clk_put(hclk);

	return ret;
}

unsigned long disp_sys_clk_get_rate(const char *id)
{
	struct clk* hclk = NULL;
	unsigned long rate = 0;

	hclk = clk_get(NULL, id);

	if(NULL == hclk || IS_ERR(hclk)) {
		__wrn("Fail to get handle for clock %s.\n", id);
		return -1;
	}

	rate = clk_get_rate(hclk);
	__inf("clock %s's rate is %ld\n", id, rate);
	clk_put(hclk);

	return rate;
}
EXPORT_SYMBOL(disp_sys_clk_set_rate);
int disp_sys_clk_set_parent(const char *id, const char *parent)
{
	struct clk* hclk = NULL;
	struct clk* hckl_parent = NULL;
	int ret = 0;

	hclk = clk_get(NULL, id);

	if(NULL == hclk || IS_ERR(hclk)) {
		__wrn("Fail to get handle for clock %s.\n", id);
		return -1;
	}

	hckl_parent = clk_get(NULL, parent);

	if(NULL == hckl_parent || IS_ERR(hckl_parent)) {
		__wrn("Fail to get handle for clock %s.\n", parent);
		clk_put(hclk);
		return -1;
	}

	ret = clk_set_parent(hclk, hckl_parent);
	if(0 != ret)
		__wrn("Fail to set parent %s for clk %s\n", parent, id);
	else
		__inf("Success to set parent %s for clk %s\n", parent, id);

	clk_put(hclk);
	clk_put(hckl_parent);

	return ret;
}

int disp_sys_clk_enable(const char *id)
{
	struct clk* hclk = NULL;
	int ret = 0;

	hclk = clk_get(NULL, id);

	if(NULL == hclk || IS_ERR(hclk)) {
		__wrn("Fail to get handle for clock %s.\n", id);
		return -1;
	}

	ret = clk_prepare_enable(hclk);
	if(0 == ret)
		__inf("Success enable clock %s.\n", id);
	else
		__wrn("Fail enable clock %s.\n", id);
	clk_put(hclk);

	return ret;
}

int disp_sys_clk_disable(const char *id)
{
	struct clk* hclk = NULL;
	int ret = 0;

	hclk = clk_get(NULL, id);

	if(NULL == hclk || IS_ERR(hclk)) {
		__wrn("Fail to get handle for clock %s.\n", id);
		return -1;
	}

	if(hclk->enable_count > 0)
		clk_disable(hclk);
	else
		__wrn("clock %s is already disabled!!!\n", id);
	clk_put(hclk);

	return ret;
}

EXPORT_SYMBOL(disp_sys_clk_get_rate);
EXPORT_SYMBOL(disp_sys_clk_set_parent);
EXPORT_SYMBOL(disp_sys_clk_enable);
EXPORT_SYMBOL(disp_sys_clk_disable);