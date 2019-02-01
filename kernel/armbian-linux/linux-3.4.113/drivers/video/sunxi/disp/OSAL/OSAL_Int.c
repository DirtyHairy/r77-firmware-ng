/*
*************************************************************************************
*                         			eBsp
*					   Operation System Adapter Layer
*
*				(c) Copyright 2006-2010, All winners Co,Ld.
*							All	Rights Reserved
*
* File Name 	: OSAL_Int.h
*
* Author 		: javen
*
* Description 	: �жϲ���
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	   2010-09-07          1.0         create this word
*
*************************************************************************************
*/


#include "OSAL.h"

/*
*******************************************************************************
*                     OSAL_RegISR
*
* Description:
*    ע���жϷ������
*
* Parameters:
*    irqno    	    ��input.  �жϺ�
*    flags    	    ��input.  �ж����ͣ�Ĭ��ֵΪ0��
*    Handler  	    ��input.  �жϴ��������ڣ������ж��¼����
*    pArg 	        ��input.  ����
*    DataSize 	    ��input.  �����ĳ���
*    prio	        ��input.  �ж����ȼ�

*
* Return value:
*     ���سɹ�����ʧ�ܡ�
*
* note:
*    �жϴ�����ԭ�ͣ�typedef s32 (*ISRCallback)( void *pArg)��
*
*******************************************************************************
*/
int OSAL_RegISR(u32 IrqNo, u32 Flags,ISRCallback Handler,void *pArg,u32 DataSize,u32 Prio)
{
	__inf("OSAL_RegISR, irqNo=%d, Handler=0x%x, pArg=0x%x\n", IrqNo, (int)Handler, (int)pArg);
	return request_irq(IrqNo, (irq_handler_t)Handler, IRQF_DISABLED, "dispaly", pArg);
}

/*
*******************************************************************************
*                     OSAL_UnRegISR
*
* Description:
*    ע���жϷ������
*
* Parameters:
*    irqno    	��input.  �жϺ�
*    handler  	��input.  �жϴ��������ڣ������ж��¼����
*    Argment 	��input.  ����
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void OSAL_UnRegISR(u32 IrqNo, ISRCallback Handler, void *pArg)
{
	free_irq(IrqNo, pArg);
}

/*
*******************************************************************************
*                     OSAL_InterruptEnable
*
* Description:
*    �ж�ʹ��
*
* Parameters:
*    irqno ��input.  �жϺ�
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void OSAL_InterruptEnable(u32 IrqNo)
{
	enable_irq(IrqNo);
}

/*
*******************************************************************************
*                     OSAL_InterruptDisable
*
* Description:
*    �жϽ�ֹ
*
* Parameters:
*     irqno ��input.  �жϺ�
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void OSAL_InterruptDisable(u32 IrqNo)
{
	disable_irq(IrqNo);
}

