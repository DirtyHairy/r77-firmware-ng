/*
*************************************************************************************
*                         			eBsp
*					   Operation System Adapter Layer
*
*				(c) Copyright 2006-2010, All winners Co,Ld.
*							All	Rights Reserved
*
* File Name 	: OSAL_Clock.h
*
* Author 		: javen
*
* Description 	: ����ϵͳ�����
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	   	2010-09-07          1.0         create this word
*		holi			2010-12-03			1.1			ʵ���˾���Ľӿ�
*************************************************************************************
*/

#ifndef  __OSAL_CLOCK_H__
#define  __OSAL_CLOCK_H__

//#define __OSAL_CLOCK_MASK__

#if defined (CONFIG_ARCH_SUN8IW5P1)
#include "../de/lowlevel_sun8iw5/de_clock.h"
#elif defined (CONFIG_ARCH_SUN9IW1P1)
#include "../de/lowlevel_sun9iw1/de_clock.h"
#endif

#define CLK_ON 1
#define CLK_OFF 0

#if 0
typedef enum
{
	CLK_NONE = 0,

	SYS_CLK_PLL3 = 1,
	SYS_CLK_PLL7 = 2,
	SYS_CLK_PLL9 = 3,
	SYS_CLK_PLL10 = 4,
	SYS_CLK_PLL3X2 = 5,
	SYS_CLK_PLL6 = 6,
	SYS_CLK_PLL6x2 = 7,
	SYS_CLK_PLL7X2 = 8,
	SYS_CLK_MIPIPLL = 9,

	MOD_CLK_DEBE0 = 16,
	MOD_CLK_DEBE1 = 17,
	MOD_CLK_DEFE0 = 18,
	MOD_CLK_DEFE1 = 19,
	MOD_CLK_LCD0CH0 = 20,
	MOD_CLK_LCD0CH1 = 21,
	MOD_CLK_LCD1CH0 = 22,
	MOD_CLK_LCD1CH1 = 23,
	MOD_CLK_HDMI = 24,
	MOD_CLK_HDMI_DDC = 25,
	MOD_CLK_MIPIDSIS = 26,
	MOD_CLK_MIPIDSIP = 27,
	MOD_CLK_IEPDRC0 = 28,
	MOD_CLK_IEPDRC1 = 29,
	MOD_CLK_IEPDEU0 = 30,
	MOD_CLK_IEPDEU1 = 31,
	MOD_CLK_LVDS = 32,
	MOD_CLK_EDP  = 33,
	MOD_CLK_DEBE2 = 34,
	MOD_CLK_DEFE2 = 35,
	MOD_CLK_SAT0 = 36,
	MOD_CLK_SAT1 = 37,
	MOD_CLK_SAT2 = 38,
#if defined(CONFIG_ARCH_SUN6I)
	AHB_CLK_MIPIDSI,
    AHB_CLK_LCD0,
    AHB_CLK_LCD1,
    AHB_CLK_CSI0,
    AHB_CLK_CSI1,
    AHB_CLK_HDMI,
    AHB_CLK_DEBE0,
    AHB_CLK_DEBE1,
    AHB_CLK_DEFE0,
    AHB_CLK_DEFE1,
    AHB_CLK_DEU0,
    AHB_CLK_DEU1,
    AHB_CLK_DRC0,
    AHB_CLK_DRC1,
    AHB_CLK_TVE0, //not exist in a31
    AHB_CLK_TVE1, //not exist in a31

    DRAM_CLK_DRC0,
    DRAM_CLK_DRC1,
    DRAM_CLK_DEU0,
    DRAM_CLK_DEU1,
    DRAM_CLK_DEFE0,
    DRAM_CLK_DEFE1,
    DRAM_CLK_DEBE0,
    DRAM_CLK_DEBE1,
#endif
}__disp_clk_id_t;

#define CLK_BE_SRC SYS_CLK_PLL10
#if defined(CONFIG_ARCH_SUN8IW1P1) || defined(CONFIG_ARCH_SUN6I)
#define CLK_LCD_CH0_SRC SYS_CLK_MIPIPLL
#define CLK_LCD_CH1_SRC SYS_CLK_PLL7
#define CLK_FE_SRC SYS_CLK_PLL7
#define CLK_DSI_SRC SYS_CLK_PLL7
/* 0:pll3, 1:pll7 */
#define CLK_MIPI_PLL_SRC 1
#elif defined CONFIG_ARCH_SUN8IW3P1
#define CLK_LCD_CH0_SRC SYS_CLK_MIPIPLL
#define CLK_FE_SRC SYS_CLK_PLL10
#define CLK_LCD_CH1_SRC SYS_CLK_PLL3
#define CLK_DSI_SRC SYS_CLK_PLL3
/* 0:pll3, 1:pll7 */
#define CLK_MIPI_PLL_SRC 0
#elif defined(CONFIG_ARCH_SUN8IW3P1)
#define CLK_LCD_CH0_SRC SYS_CLK_MIPIPLL
#define CLK_LCD_CH1_SRC SYS_CLK_PLL7
#define CLK_FE_SRC SYS_CLK_PLL7
#define CLK_DSI_SRC SYS_CLK_PLL7
/* 0:pll3, 1:pll7 */
#define CLK_MIPI_PLL_SRC 1
#elif defined CONFIG_ARCH_SUN9IW1P1
#define CLK_LCD_CH0_SRC SYS_CLK_PLL7
#define CLK_LCD_CH1_SRC SYS_CLK_PLL7
#define CLK_FE_SRC SYS_CLK_PLL10
#define CLK_DSI_SRC SYS_CLK_PLL10
/* 0:pll3, 1:pll7 */
#define CLK_MIPI_PLL_SRC 1
#elif defined(CONFIG_ARCH_SUN8IW5P1)
#define CLK_LCD_CH0_SRC SYS_CLK_MIPIPLL
#define CLK_FE_SRC SYS_CLK_PLL10
#define CLK_DSI_SRC SYS_CLK_PLL3
#else

#endif
#define CLK_HDMI_SRC SYS_CLK_PLL7
#define CLK_HDMI_DDC_SRC SYS_CLK_PLL7
#endif

#ifndef __OSAL_CLOCK_MASK__
#define RESET_OSAL
#define RST_INVAILD 1
#define RST_VAILD   0
#endif

/*
*********************************************************************************************************
*/
s32 osal_init_clk_pll(void);

/*
*********************************************************************************************************
*                                   SET SOURCE CLOCK FREQUENCY
*
* Description:
*		set source clock frequency;
*
* Arguments  :
*		nSclkNo  	:	source clock number;
*       nFreq   	:	frequency, the source clock will change to;
*
* Returns    : result;
*
* Note       :
*********************************************************************************************************
*/
s32 OSAL_CCMU_SetSrcFreq(u32 nSclkNo, u32 nFreq);



/*
*********************************************************************************************************
*                                   GET SOURCE CLOCK FREQUENCY
*
* Description:
*		get source clock frequency;
*
* Arguments  :
*		nSclkNo  	:	source clock number need get frequency;
*
* Returns    :
*		frequency of the source clock;
*
* Note       :
*********************************************************************************************************
*/
u32 OSAL_CCMU_GetSrcFreq(u32 nSclkNo);



/*
*********************************************************************************************************
*                                   OPEN MODULE CLK
* Description:
*		open module clk;
*
* Arguments  :
*		nMclkNo	:	number of module clock which need be open;
*
* Returns    :
*		EBSP_TRUE/EBSP_FALSE
*
* Note       :
*********************************************************************************************************
*/
__hdle OSAL_CCMU_OpenMclk(s32 nMclkNo);


/*
*********************************************************************************************************
*                                    CLOSE MODULE CLK
* Description:
*		close module clk;
*
* Arguments  :
*		hMclk	:	handle
*
* Returns    :
*		EBSP_TRUE/EBSP_FALSE
*
* Note       :
*********************************************************************************************************
*/
s32  OSAL_CCMU_CloseMclk(__hdle hMclk);

/*
*********************************************************************************************************
*                                   GET MODULE SRC
* Description:
*		set module src;
*
* Arguments  :
*		nMclkNo	:	number of module clock which need be open;
*       nSclkNo	:	call-back function for process clock change;
*
* Returns    :
*		EBSP_TRUE/EBSP_FALSE
*
* Note       :
*********************************************************************************************************
*/
s32 OSAL_CCMU_SetMclkSrc(__hdle hMclk);





/*
*********************************************************************************************************
*                                  GET MODULE SRC
*
* Description:
*		get module src;
*
* Arguments  :
*		nMclkNo	:	handle of the module clock;
*
* Returns    :
*		src no
*
* Note       :
*********************************************************************************************************
*/
u32 OSAL_CCMU_GetMclkSrc(__hdle hMclk);




/*
*********************************************************************************************************
*                                   SET MODUEL CLOCK FREQUENCY
*
* Description:
*		set module clock frequency;
*
* Arguments  :
*		nSclkNo  :	number of source clock which the module clock will use;
*		nDiv     :	division for the module clock;
*
* Returns    :
*		EBSP_TRUE/EBSP_FALSE
*
* Note       :
*********************************************************************************************************
*/
s32 OSAL_CCMU_SetMclkFreq(__hdle hMclk, u32 nFreq);



/*
*********************************************************************************************************
*                                   GET MODUEL CLOCK FREQUENCY
*
* Description:
*		get module clock requency;
*
* Arguments  :
*		hMclk    	:	module clock handle;
*
* Returns    :
*		frequency of the module clock;
*
* Note       :
*********************************************************************************************************
*/
u32 OSAL_CCMU_GetMclkFreq(__hdle hMclk);



/*
*********************************************************************************************************
*                                   MODUEL CLOCK ON/OFF
*
* Description:
*		module clock on/off;
*
* Arguments  :
*		nMclkNo		:	module clock handle;
*       bOnOff   	:	on or off;
*
* Returns    :
*		EBSP_TRUE/EBSP_FALSE
*
* Note       :
*********************************************************************************************************
*/
s32 OSAL_CCMU_MclkOnOff(__hdle hMclk, s32 bOnOff);

s32 OSAL_CCMU_MclkReset(__hdle hMclk, s32 bReset);


/*
//��һ��
s32  esCLK_SetSrcFreq(s32 nSclkNo, u32 nFreq);
u32  esCLK_GetSrcFreq(s32 nSclkNo);

__hdle esCLK_OpenMclk(s32 nMclkNo, __pCB_ClkCtl_t pCb);
s32  esCLK_CloseMclk(__hdle hMclk);

s32  esCLK_SetMclkSrc(s32 nMclkNo, s32 nSclkNo);
s32  esCLK_GetMclkSrc(s32 nMclkNo);

s32  esCLK_SetMclkDiv(s32 nMclkNo, s32 nDiv);
u32  esCLK_GetMclkDiv(s32 nMclkNo);

s32  esCLK_MclkOnOff(s32 nMclkNo, s32 bOnOff);

//======================================================================================

//�ڶ���
s32 esCLK_reg_cb(s32 nMclkNo, __pCB_ClkCtl_t pCb);	//__hdle esCLK_OpenMclk(s32 nMclkNo, __pCB_ClkCtl_t pCb);
s32  esCLK_unreg_cb(s32 nMclkNo);					//s32  esCLK_CloseMclk(__hdle hMclk);

//------------------------------------------------------

					s32  esCLK_SetSrcFreq(s32 nSclkNo, u32 nFreq);
					u32  esCLK_GetSrcFreq(s32 nSclkNo);


__hdle esCLK_OpenMclk(s32 nMclkNo);
s32  esCLK_CloseMclk(__hdle hMclk);



s32  esCLK_SetMclkSrc(__hdle hMclk, s32 nSclkNo);	//s32  esCLK_SetMclkSrc(s32 nMclkNo, s32 nSclkNo);
s32  esCLK_GetMclkSrc(__hdle hMclk);					//s32  esCLK_GetMclkSrc(s32 nMclkNo);

s32  esCLK_SetMclkDiv(__hdle hMclk, s32 nDiv);
u32  esCLK_GetMclkDiv(__hdle hMclk);

s32  esCLK_MclkOnOff(__hdle hMclk, s32 bOnOff);


*/

#endif   //__OSAL_CLOCK_H__

