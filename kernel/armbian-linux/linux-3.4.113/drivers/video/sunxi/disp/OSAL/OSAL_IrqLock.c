/*
*************************************************************************************
*                         			eBsp
*					   Operation System Adapter Layer
*
*				(c) Copyright 2006-2010, All winners Co,Ld.
*							All	Rights Reserved
*
* File Name 	: OSAL_IrqLock.h
*
* Author 		: javen
*
* Description 	: �ٽ�������
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	   2010-09-07          1.0         create this word
*
*************************************************************************************
*/
#include "OSAL.h"
void OSAL_IrqLock(u32 *cpu_sr)
{
    //local_irq_save(*cpu_sr);

	//unsigned long flags = *cpu_sr;
	//local_irq_save(flags);
}

void OSAL_IrqUnLock(u32 cpu_sr)
{
	//local_irq_restore(cpu_sr);

    //unsigned long flags = cpu_sr;
    //local_irq_restore(flags);
}



