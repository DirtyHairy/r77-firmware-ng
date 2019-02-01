/*
*************************************************************************************
*                         			eBsp
*					   Operation System Adapter Layer
*
*				(c) Copyright 2006-2010, All winners Co,Ld.
*							All	Rights Reserved
*
* File Name 	: OSAL_Semi.h
*
* Author 		: javen
*
* Description 	: �ź�������
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	   2010-09-07          1.0         create this word
*
*************************************************************************************
*/
#ifndef  __OSAL_SEMI_H__
#define  __OSAL_SEMI_H__


typedef void*  OSAL_SemHdle;

/*
*******************************************************************************
*                     eBase_CreateSemaphore
*
* Description:
*    �����ź���
*
* Parameters:
*    Count  :  input.  �ź����ĳ�ʼֵ��
* 
* Return value:
*    �ɹ��������ź��������ʧ�ܣ�����NULL��
*
* note:
*    void
*
*******************************************************************************
*/
OSAL_SemHdle OSAL_CreateSemaphore(__u32 Count);

/*
*******************************************************************************
*                     OSAL_DeleteSemaphore
*
* Description:
*    ɾ���ź���
*
* Parameters:
*    SemHdle  :  input.  OSAL_CreateSemaphore ����� �ź������
* 
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void OSAL_DeleteSemaphore(OSAL_SemHdle SemHdle);

/*
*******************************************************************************
*                     OSAL_SemPend
*
* Description:
*    ���ź���
*
* Parameters:
*    SemHdle  :  input.  OSAL_CreateSemaphore ����� �ź������
* 
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void OSAL_SemPend(OSAL_SemHdle SemHdle, __u16 TimeOut);

/*
*******************************************************************************
*                     OSAL_SemPost
*
* Description:
*    �ź�������
*
* Parameters:
*    SemHdle  :  input.  OSAL_CreateSemaphore ����� �ź������
* 
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void OSAL_SemPost(OSAL_SemHdle SemHdle);


#endif   //__OSAL_SEMI_H__

