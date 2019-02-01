
#include "de_be.h"

s32 DE_BE_Format_To_Bpp(u32 sel, u8 format)
{
    u8 bpp = 0;

	switch(format)
	{
		case  DE_MONO_1BPP:
			bpp = 1;
			break;

		case DE_MONO_2BPP:
			bpp = 2;
			break;

		case DE_MONO_4BPP:
			bpp = 4;
			break;

		case DE_MONO_8BPP:
			bpp = 8;
			break;

		case DE_COLOR_RGB655:
		case DE_COLOR_RGB565:
		case DE_COLOR_RGB556:
		case DE_COLOR_ARGB1555:
		case DE_COLOR_RGBA5551:
		case DE_COLOR_ARGB4444:
			bpp=16;
			break;

		case DE_COLOR_RGB0888:
			bpp = 32;
			break;

		case DE_COLOR_ARGB8888:
			bpp = 32;
			break;

		case DE_COLOR_RGB888:
			bpp = 24;
			break;

		default:
		    bpp = 0;
			break;
	}

    return bpp;
}

u32 DE_BE_Offset_To_Addr(u32 src_addr,u32 width,u32 x,u32 y,u32 bpp)
{
    u32 addr;

    addr = src_addr + ((y*(width*bpp))>>3) + ((x*bpp)>>3);

    return addr;
}

u32  DE_BE_Addr_To_Offset(u32 src_addr,u32 off_addr,u32 width,u32 bpp,disp_position *pos)
{
    u32    dist;
    disp_position  offset;

    dist        = off_addr-src_addr;
    offset.y    = (dist<<3)/(width*bpp);
    offset.x    = ((dist<<3)%(width*bpp))/bpp;
    pos->x      = offset.x;
    pos->y      = offset.y;

    return 0;

}

s32 DE_BE_Layer_Set_Work_Mode(u32 sel, u8 layidx,u8 mode)
{
    u32 tmp;

    tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx);
    DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx,(tmp&0xff3fffff)|mode<<22);

    return 0;
}

static s32 DE_BE_Layer_Set_Addr(u32 sel, u8 layidx,u32 addr)   //bit
{
	u32 tmp_l,tmp_h,tmp;
	tmp_l = addr<<3;
	tmp_h = (addr&0xe0000000)>>29;
    DE_BE_WUINT32IDX(sel, DE_BE_FRMBUF_LOW32ADDR_OFF,layidx,tmp_l);

    tmp = DE_BE_RUINT32(sel,DE_BE_FRMBUF_HIGH4ADDR_OFF) & (~(0xff<<(layidx*8)));
    DE_BE_WUINT32(sel, DE_BE_FRMBUF_HIGH4ADDR_OFF, tmp | (tmp_h << (layidx*8)));

    return 0;
}

static s32 DE_BE_Layer_Set_Line_Width(u32 sel, u8 layidx,u32 width)    //byte
{
    DE_BE_WUINT32IDX(sel, DE_BE_FRMBUF_WLINE_OFF,layidx,width);
    return 0;
}


s32 DE_BE_Layer_Set_Format(u32 sel, u8 layidx,u8 format,bool br_swap,u8 order, bool pre_multiply)
{
    u32 tmp;

    tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF1,layidx);
    DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF1,layidx,(tmp&0xfffff000)|format<<8|br_swap<<2|order);

    tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx) & (~(3<<20));
    DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0, layidx, tmp | (pre_multiply<<20));

    return 0;
}

s32 DE_BE_Layer_Set_Framebuffer(u32 sel, u8 layidx, layer_src_t *layer_fb)
{
	s32 bpp;
	u32 addr;

	bpp = DE_BE_Format_To_Bpp(sel, layer_fb->format);
	if(bpp <= 0)
	{
		return -1;
	}
	addr = DE_BE_Offset_To_Addr(layer_fb->fb_addr, layer_fb->fb_width, layer_fb->offset_x, layer_fb->offset_y,bpp);
    DE_BE_Layer_Set_Format(sel, layidx,layer_fb->format,layer_fb->br_swap,layer_fb->pixseq, layer_fb->pre_multiply);

    DE_BE_Layer_Set_Addr(sel, layidx,addr);
    DE_BE_Layer_Set_Line_Width(sel, layidx,layer_fb->fb_width*bpp);

	return 0;
}

s32 DE_BE_Layer_Set_Screen_Win(u32 sel, u8 layidx, disp_window * win)
{
    u32 tmp;
    u32 width, height;

    width = win->width;
    height = win->height;

    width = (width==0)? 0:(width-1);
    height = (height==0)? 0:(height-1);

    tmp = ((((u32)(win->y))>>31)<<31)|((((u32)(win->y))&0x7fff)<<16)|((((u32)(win->x))>>31)<<15)|(((u32)(win->x))&0x7fff);
    DE_BE_WUINT32IDX(sel, DE_BE_LAYER_CRD_CTL_OFF,layidx,tmp);
    DE_BE_WUINT32IDX(sel, DE_BE_LAYER_SIZE_OFF,layidx,(height)<<16|(width));

    return 0;
}

s32 DE_BE_Layer_Video_Enable(u32 sel, u8 layidx,bool video_en)
{

    u32 tmp;

    tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx);
    DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx,(tmp&0xfffffffd)|video_en<<1);


    return 0;
}

s32 DE_BE_Layer_Video_Ch_Sel(u32 sel, u8 layidx,u8 scaler_index)
{

    u32 tmp;

    tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx);
    DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx,(tmp&0xffffffcf)|scaler_index<<4);


    return 0;
}

s32 DE_BE_Layer_Yuv_Ch_Enable(u32 sel, u8 layidx,bool yuv_en)
{

    u32 tmp;

    tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx);
    DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx,(tmp&0xfffffffb)|yuv_en<<2);


    return 0;
}

s32 DE_BE_Layer_Set_Prio(u32 sel, u8 layidx,u8 prio)
{
    u32 tmp;

    tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx);
    DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx,(tmp&0xfffff3ff)|prio<<10);

    return 0;
 }

s32 DE_BE_Layer_Set_Pipe(u32 sel, u8 layidx,u8 pipe)
{
    u32 tmp;

    tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx);
    DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx,(tmp&0xffff7fff)|pipe<<15);

    return 0;
}


s32 DE_BE_Layer_ColorKey_Enable(u32 sel, u8 layidx, bool enable)
{

    u32 tmp;

    if(enable)
    {
        tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx);
        DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx,(tmp&0xfff3ffff)|1<<18);
    }
    else
    {
        tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx);
        DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx,(tmp&0xfff3ffff));
    }

    return 0;
}

s32 DE_BE_Layer_Alpha_Enable(u32 sel, u8 layidx, bool enable)
{
    u32 tmp;

    if(enable)
    {
        tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx);
        DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx,(tmp&0xfffffffe)|0x01);
    }
    else
    {
        tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx);
        DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx,(tmp&0xfffffffe));
    }

    return 0;
}

//FIXME, global pixel alpha support
s32 DE_BE_Layer_Set_Alpha_Value(u32 sel, u8 layidx,u8 alpha_val)
{

    u32 tmp;

    tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx);
    DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx,(tmp&0x0ffffff)|alpha_val<<24);

    return 0;
}

s32 DE_BE_Layer_Global_Pixel_Alpha_Enable(u32 sel, u8 layidx, bool enable)
{
    u32 tmp;

    if(enable)
    {
        tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx);
        DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx,(tmp&(~(0x3<<16)))|(0x01<<16));
    }
    else
    {
        tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx);
        DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,layidx,(tmp&(~(0x3<<16))));
    }

    return 0;
}

s32 DE_BE_Layer_Enable(u32 sel, u8 layidx, bool enable)
{
	if(enable)
	{
	    DE_BE_WUINT32(sel, DE_BE_MODE_CTL_OFF,DE_BE_RUINT32(sel, DE_BE_MODE_CTL_OFF)|(1<<layidx)<<8);
	}
	else
	{
	    DE_BE_WUINT32(sel, DE_BE_MODE_CTL_OFF,DE_BE_RUINT32(sel, DE_BE_MODE_CTL_OFF)&(~((1<<layidx)<<8)));
	}

    return 0;
}


static s32 DE_BE_YUV_CH_Cfg_Csc_Coeff(u32 sel, u8 cs_mode)
{
	u32 csc_coef_off;
	u32 *pdest_end;
    u32 *psrc_cur;
    u32 *pdest_cur;
    u32 temp;

	csc_coef_off = (((cs_mode&0x3)<<7) + ((cs_mode&0x3)<<6)) + 0/*yuv in*/ + 0/*rgb out*/;

	pdest_cur = (u32*)(DE_Get_Reg_Base(sel)+DE_BE_YG_COEFF_OFF);
	psrc_cur = (u32*)(&csc_tab[csc_coef_off>>2]);
	pdest_end = pdest_cur + 12;

    while(pdest_cur < pdest_end)
    {
    	temp = *(volatile u32 *)pdest_cur;
		temp &= 0xffff0000;
		*(volatile u32 *)pdest_cur++ = ((*psrc_cur++)&0xffff) | temp;
    }

	return 0;
}

//==================================================================
//function name:    DE_BE_YUV_CH_Set_Format
//author:
//date:             2009-9-28
//description:      de be input YUV channel format setting
//parameters:	----format(0-4)
//					0:	planar YUV 411
//					1:	planar YUV 422
//					2:	planar YUV 444
//					3:	interleaved YUV 422
//					4:	interleaved YUV 444
//				----pixel_seq(0-3)
//					in planar data format mode
//						0:Y3Y2Y1Y0
//						1:Y0Y1Y2Y3
//					in interleaved YUV 422 data format mode
//						0:DE_SCAL_UYVY
//						1:DE_SCAL_YUYV
//						2:DE_SCAL_VYUY
//						3:DE_SCAL_YVYU
//					in interleaved YUV 444 format mode
//						0:DE_SCAL_AYUV
//						1:DE_SCAL_VUYA
//return:           if success return DIS_SUCCESS
//                  if fail return the number of fail
//modify history:
//==================================================================
static s32 DE_BE_YUV_CH_Set_Format(u32 sel, u8 format,u8 pixel_seq)
{
    u32 tmp;

    tmp = DE_BE_RUINT32(sel, DE_BE_YUV_CTRL_OFF);
    tmp &= 0xffff8cff;//clear bit14:12, bit9:8
	DE_BE_WUINT32(sel, DE_BE_YUV_CTRL_OFF, tmp | (format<<12) | (pixel_seq<<8));

	return 0;
}

static s32 DE_BE_YUV_CH_Set_Addr(u32 sel, u8 ch_no,u32 addr)
{
	DE_BE_WUINT32IDX(sel, DE_BE_YUV_ADDR_OFF,ch_no,addr);//addr in BYTE
	return 0;
}

static s32 DE_BE_YUV_CH_Set_Line_Width(u32 sel, u8 ch_no,u32 width)
{
	DE_BE_WUINT32IDX(sel, DE_BE_YUV_LINE_WIDTH_OFF,ch_no,width);
	return 0;
}

s32 DE_BE_YUV_CH_Set_Src(u32 sel, de_yuv_ch_src_t * in_src)
{
	u32 ch0_base, ch1_base, ch2_base;
	u32 image_w;
	u32 offset_x, offset_y;
    u8 in_fmt,in_mode,pixseq;
    u32 ch0_addr, ch1_addr, ch2_addr;
    u32 ch0_line_stride, ch1_line_stride, ch2_line_stride;
    u8 w_shift, h_shift;
	u32 de_scal_ch0_offset;
	u32 de_scal_ch1_offset;
	u32 de_scal_ch2_offset;

    ch0_base = in_src->ch0_base;
    ch1_base = in_src->ch1_base;
    ch2_base = in_src->ch2_base;
    image_w = in_src->line_width;
    offset_x = in_src->offset_x;
    offset_y = in_src->offset_y;
    in_fmt = in_src->format;
    in_mode = in_src->mode;
    pixseq = in_src->pixseq;

    w_shift = (in_fmt==0x1 || in_fmt==0x3) ? 1 : ((in_fmt==0x0)? 2: 0);
    h_shift = 0;
    //modify offset and input size
    offset_x = (offset_x>>w_shift)<<w_shift;
    offset_y = (offset_y>>h_shift)<<h_shift;
    image_w =((image_w+((1<<w_shift)-1))>>w_shift)<<w_shift;
    //compute buffer address
    //--the size ratio of Y/G to UV/RB must be fit with input format and mode &&&&
    if(in_mode == 0x00)    //non macro block plannar
    {
        //line stride
        ch0_line_stride = image_w;
        ch1_line_stride = image_w>>(w_shift);
        ch2_line_stride = image_w>>(w_shift);
        //buffer address
        de_scal_ch0_offset = image_w * offset_y + offset_x;
        de_scal_ch1_offset = (image_w>>w_shift) * (offset_y>>h_shift) + (offset_x>>w_shift); //image_w'
        de_scal_ch2_offset = (image_w>>w_shift) * (offset_y>>h_shift) + (offset_x>>w_shift); //image_w'

        ch0_addr = ch0_base + de_scal_ch0_offset;
        ch1_addr = ch1_base + de_scal_ch1_offset;
        ch2_addr = ch2_base + de_scal_ch2_offset;
    }
    else if(in_mode == 0x01) //interleaved data
    {
        //line stride
        ch0_line_stride = image_w<<(0x02 - w_shift);
        ch1_line_stride = 0x00;
        ch2_line_stride = 0x00;
        //buffer address
        de_scal_ch0_offset = ((image_w * offset_y + offset_x)<<(0x02 - w_shift));
        de_scal_ch1_offset = 0x0;
        de_scal_ch2_offset = 0x0;

        ch0_addr = ch0_base + de_scal_ch0_offset;
        ch1_addr = 0x00;
        ch2_addr = 0x00;
    }
    else
    {
    	return 0;
    }

    DE_BE_YUV_CH_Set_Format(sel, in_fmt,pixseq);
    //set line stride
    DE_BE_YUV_CH_Set_Line_Width(sel, 0x00, ch0_line_stride<<3);
    DE_BE_YUV_CH_Set_Line_Width(sel, 0x01, ch1_line_stride<<3);
    DE_BE_YUV_CH_Set_Line_Width(sel, 0x02, ch2_line_stride<<3);
    //set buffer address
    DE_BE_YUV_CH_Set_Addr(sel, 0x00, ch0_addr);
    DE_BE_YUV_CH_Set_Addr(sel, 0x01, ch1_addr);
    DE_BE_YUV_CH_Set_Addr(sel, 0x02, ch2_addr);

    DE_BE_YUV_CH_Cfg_Csc_Coeff(sel, in_src->cs_mode);
    return 0;
}

s32 DE_BE_YUV_CH_Enable(u32 sel, bool enable)
{
    if(enable)
    {
	    DE_BE_WUINT32(sel, DE_BE_YUV_CTRL_OFF,DE_BE_RUINT32(sel, DE_BE_YUV_CTRL_OFF)|0x00000001);
	}
	else
	{
	    DE_BE_WUINT32(sel, DE_BE_YUV_CTRL_OFF,DE_BE_RUINT32(sel, DE_BE_YUV_CTRL_OFF)&0xfffffffe);
	}
	return 0;
}


