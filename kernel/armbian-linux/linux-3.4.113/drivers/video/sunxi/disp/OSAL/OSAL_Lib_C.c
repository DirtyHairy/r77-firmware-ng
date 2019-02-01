/*
*************************************************************************************
*                         			eBsp
*					   Operation System Adapter Layer
*
*				(c) Copyright 2006-2010, All winners Co,Ld.
*							All	Rights Reserved
*
* File Name 	: OSAL_Lib_C.h
*
* Author 		: javen
*
* Description 	: C�⺯��
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	   2010-09-07          1.0         	create this word
*		holi		   2010-12-03		   1.1			������OSAL_io_remap
*************************************************************************************
*/

#include "OSAL.h"

extern int kdb_trap_printk;

/* ��ͨ�ڴ���� */
void * OSAL_malloc(u32 Size)
{
	return kmalloc(Size, GFP_KERNEL | __GFP_ZERO);
}

void OSAL_free(void *pAddr)
{
	kfree(pAddr);
}

/* �����������ڴ���� */
void * OSAL_PhyAlloc(u32 Size)
{
	return kmalloc(Size, GFP_KERNEL | __GFP_ZERO);
}

void OSAL_PhyFree(void *pAddr, u32 Size)
{
	kfree(pAddr);
}


/* �����ڴ�������ڴ�֮���ת�� */
unsigned int OSAL_VAtoPA(void *va)
{
#if defined(CONFIG_ARCH_SUN8IW1P1) || defined(CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN6I)
    if((unsigned int)(va) > 0x40000000)
		return (unsigned int)(va) - 0x40000000;
#endif
	return (unsigned int)(va);
}

void *OSAL_PAtoVA(unsigned int pa)
{
	return (void*)(pa);
}


/*
*******************************************************************************
*                     IO��ַת��
*
* Description:
*    	��һ�������ַת��Ϊ�����ַ
*
* Parameters:
*		phy_addr	��	�����ַ
*		size		:	��ַ�ĳ���
*
* Return value:
*		==0			:	ʧ��
*		!=0			:	�����ַ
*
* note:
*    	size������4KΪ������������4k��������
*
*******************************************************************************
*/
void *	 OSAL_io_remap(u32 phy_addr , u32 size)
{
	return (void* __force)ioremap(phy_addr,  size);
}

int OSAL_printf(const char *fmt, ...)
{
#if 0
	va_list args;
	int r;

#ifdef CONFIG_KGDB_KDB
	if (unlikely(kdb_trap_printk)) {
		va_start(args, fmt);
		r = vkdb_printf(fmt, args);
		va_end(args);
		return r;
	}
#endif
	va_start(args, fmt);
	r = vprintk(fmt, args);
	va_end(args);
#endif
	return 0;
}

int OSAL_putchar(int value)
{
	return 0;
	}
int OSAL_puts(const char * value)
{
	return 0;
}
int OSAL_getchar(void)
{
	return 0;
}
char * OSAL_gets(char *value)
{
	return NULL;
}

//----------------------------------------------------------------
//  ʵ�ú���
//----------------------------------------------------------------
/* �ַ���ת������ */
long OSAL_strtol (const char *str, const char **err, int base)
{
	return 0;
}

/* �з���ʮ��������ת�ַ���*/
void OSAL_int2str_dec(int input, char * str)
{
}

/* ʮ����������ת�ַ���*/
void OSAL_int2str_hex(int input, char * str, int hex_flag)
{
}

/* �޷���ʮ��������ת�ַ���*/
void OSAL_uint2str_dec(unsigned int input, char * str)
{
}



