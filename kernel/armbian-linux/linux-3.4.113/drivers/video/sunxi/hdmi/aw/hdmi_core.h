#ifndef __HDMI_CORE_H__
#define __HDMI_CORE_H__

#include "../hdmi_hal.h"
#include "hdmi_interface.h"
#include "hdmi_hw_interface.h"

#define HDMI_WUINT32(offset,value)  writel(value, (void __iomem *)(HDMI_BASE + offset))
#define HDMI_RUINT32(offset)        readl((void __iomem *)(HDMI_BASE + offset))
#define HDMI_WUINT16(offset,value)  writew(value, (void __iomem *)(HDMI_BASE + offset))
#define HDMI_RUINT16(offset)        readw((void __iomem *)(HDMI_BASE + offset))
#define HDMI_WUINT8(offset,value)   writeb(value, (void __iomem *)(HDMI_BASE + offset))
#define HDMI_RUINT8(offset)         readb((void __iomem *)(HDMI_BASE + offset))

#define Abort_Current_Operation             0
#define Special_Offset_Address_Read         1
#define Explicit_Offset_Address_Write       2
#define Implicit_Offset_Address_Write       3
#define Explicit_Offset_Address_Read        4
#define Implicit_Offset_Address_Read        5
#define Explicit_Offset_Address_E_DDC_Read  6
#define Implicit_Offset_Address_E_DDC_Read  7

/*
typedef struct video_timing
{
    __s32 VIC;
    __s32 PCLK;
    __s32 AVI_PR;
    __s32 INPUTX;
    __s32 INPUTY;
    __s32 HT;
    __s32 HBP;
    __s32 HFP;
    __s32 HPSW;
    __s32 VT;
    __s32 VBP;
    __s32 VFP;
    __s32 VPSW;
}HDMI_VIDE_INFO;
*/
typedef struct audio_timing
{
    __s32   audio_en;
    __s32   sample_rate;
    __s32   channel_num;

    __s32   CTS;
    __s32   ACR_N;
    __s32   CH_STATUS0;
    __s32   CH_STATUS1;
    __u8    data_raw;		/*0:pcm;1:raw*/
    __u8    sample_bit;/* 8/16/20/24/32 bit */
    __u8    ca; /* channel allocation */
}HDMI_AUDIO_INFO;

__u32 hdmi_ishdcp(void);
__s32 hdmi_core_initial(void);
__s32 hdmi_core_open(void);
__s32 hdmi_core_close(void);
__s32 hdmi_main_task_loop(void);
__s32 Hpd_Check(void);
__s32 ParseEDID(void);
__s32 get_video_info(__s32 vic);
__s32 get_audio_info(__s32 sample_rate);
__s32 video_config(__u32 vic);
__s32 audio_config(void);
__s32 video_enter_lp(void);

extern __u32    hdmi_pll;//0:video pll 0; 1:video pll 1
extern __u32    hdmi_clk;
extern __s32    cts_enable;
extern __u8     isHDMI;
extern __u8  	  YCbCr444_Support;
extern __s32    hdcp_enable;
void DDC_Init(void);
void send_ini_sequence(void);
__s32 DDC_Read(char cmd,char pointer,char offset,int nbyte,char * pbuf);
extern 	__u8	EDID_Buf[1024];
extern 	__u8	Device_Support_VIC[512];

extern volatile __u32 HDMI_BASE;
extern __u32 hdmi_print;
extern __s32		hdmi_state;
extern bool		video_enable;
extern __u32		video_mode;
extern HDMI_AUDIO_INFO audio_info;
extern __s32		HPD;
extern disp_video_timing video_timing[];

#endif

