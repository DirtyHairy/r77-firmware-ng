#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/spinlock.h>
#include <linux/hdreg.h>
#include <linux/init.h>
#include <linux/semaphore.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/mutex.h>
//#include <mach/clock.h>
//#include <mach/platform.h>
//#include <mach/hardware.h>
//#include <mach/sys_config.h>
#include <linux/dma-mapping.h>
//#include <mach/dma.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/cacheflush.h>
//#include <mach/gpio.h>
#include <linux/gpio.h>

#include "nand_lib.h"

extern int NAND_Print(const char *fmt, ...);

#define OOB_BUF_SIZE                    32
#define NAND_BOOT0_BLK_START            0
#define NAND_BOOT0_BLK_CNT              2
#define NAND_UBOOT_BLK_START            (NAND_BOOT0_BLK_START+NAND_BOOT0_BLK_CNT)
#define NAND_UBOOT_BLK_CNT              5
#define NAND_BOOT0_PAGE_CNT_PER_COPY    64


#define debug NAND_Print

extern int get_nand_para(void *boot_buf);
extern int gen_uboot_check_sum( void *boot_buf );
extern int gen_check_sum( void *boot_buf );
extern int get_dram_para(void *boot_buf);
extern int get_nand_para_for_boot1(void *boot_buf);

extern int NAND_PhysicLockInit(void);
extern int NAND_PhysicLock(void);
extern int NAND_PhysicUnLock(void);
extern int NAND_PhysicLockExit(void);

extern __u32 NAND_GetPageSize(void);
extern __u32 NAND_GetPageCntPerBlk(void);
extern __u32 NAND_GetReadRetryType(void);
extern __u32 NAND_GetVersion(__u8* nand_version);
extern __s32 PHY_Readretry_reset(void);
extern __u32 NAND_GetBlkCntPerChip(void);
extern __u32 NAND_GetChipCnt(void);
extern __s32 NAND_GetBlkCntOfDie(void);
extern __s32 NAND_GetDieSkipFlag(void);
extern __u32 NAND_GetChipConnect(void);
extern __u32 NAND_GetBadBlockFlagPos(void);



__s32 burn_boot0_1k_mode(__u32 read_retry_type, __u32 Boot0_buf)
{
	__u32 i, j, k;
	__u32 pages_per_block;
	__u32 copies_per_block;
	__u8  oob_buf[32]; 
	struct boot_physical_param  para;

	debug("burn boot0 normal mode!\n");

	for (i=0; i<32; i++)
		oob_buf[i] = 0xff;

	NAND_GetVersion(oob_buf);
	if ((oob_buf[0]!=0xff) || (oob_buf[1]!= 0x00))
	{
		debug("get flash driver version error!");
		goto error;
	}

	pages_per_block = NAND_GetPageCntPerBlk();
	if (pages_per_block%64)
	{
		debug("get page cnt per block error %x!\n", pages_per_block);
		goto error;
	}

	/* cal copy cnt per bock */
	copies_per_block = pages_per_block / NAND_BOOT0_PAGE_CNT_PER_COPY;

	/* burn boot0 copys */
	for (i=NAND_BOOT0_BLK_START; i<(NAND_BOOT0_BLK_START + NAND_BOOT0_BLK_CNT); i++)
	{
		debug("boot0 %x \n", i);

		para.chip  = 0;
		para.block = i;
		if (PHY_SimpleErase( &para ) < 0)
		{
			debug("Fail in erasing block %d.\n", i);
			//continue;
		}
		debug("after erase.\n" );

		for (j=0; j<copies_per_block; j++)
		{
			for (k=0; k<NAND_BOOT0_PAGE_CNT_PER_COPY; k++)
			{
				para.chip  = 0;
				para.block = i;
				para.page = j * NAND_BOOT0_PAGE_CNT_PER_COPY + k;
				para.mainbuf = (void *)(Boot0_buf + k * 1024);
				para.oobbuf = oob_buf;
				if(PHY_SimpleWrite_1K( &para ) < 0)
				{
					debug("Warning. Fail in writing page %d in block %d.\n", j * NAND_BOOT0_PAGE_CNT_PER_COPY + k, i );
				}
			}
		}
	}
	return 0;

error:
	return -1;
}

__s32 burn_boot0_1k_mode_F16(__u32 read_retry_type, __u32 Boot0_buf)
{
	__u32 i, j, k;
	__u32 pages_per_block;
	__u32 copies_per_block;
	__u8  oob_buf[32]; 
	struct boot_physical_param  para;

	debug("burn boot0 normal mode!\n");

	for (i=0; i<32; i++)
		oob_buf[i] = 0xff;

	NAND_GetVersion(oob_buf);
	if ((oob_buf[0]!=0xff) || (oob_buf[1]!= 0x00))
	{
		debug("get flash driver version error!");
		goto error;
	}

	pages_per_block = NAND_GetPageCntPerBlk();
	if (pages_per_block%64)
	{
		debug("get page cnt per block error %x!\n", pages_per_block);
		goto error;
	}

	/* cal copy cnt per bock */
	copies_per_block = pages_per_block / NAND_BOOT0_PAGE_CNT_PER_COPY;

	/* burn boot0 copys */
	for (i=NAND_BOOT0_BLK_START; i<(NAND_BOOT0_BLK_START + NAND_BOOT0_BLK_CNT); i++)
	{
		debug("boot0 %x \n", i);

		para.chip  = 0;
		para.block = i;
		if (PHY_SimpleErase( &para ) < 0)
		{
			debug("Fail in erasing block %d.\n", i);
			//continue;
		}
		debug("after erase.\n" );

		for (j=0; j<copies_per_block; j++)
		{
			for (k=0; k<NAND_BOOT0_PAGE_CNT_PER_COPY; k++)
			{
				para.chip  = 0;
				para.block = i;
				para.page = j * NAND_BOOT0_PAGE_CNT_PER_COPY + k;
				para.mainbuf = (void *)(Boot0_buf + k * 1024);
				para.oobbuf = oob_buf;
				if(PHY_SimpleWrite_1K_F16( &para ) < 0)
				{
					debug("Warning. Fail in writing page %d in block %d.\n", j * NAND_BOOT0_PAGE_CNT_PER_COPY + k, i );
				}
			}
		}
	}
	return 0;

error:
	return -1;
}


__s32 burn_boot0_lsb_mode(__u32 read_retry_type, __u32 Boot0_buf)
{
	__u32 i, k; 
	__u8  oob_buf[32]; 
	__u32 page_size;
	struct boot_physical_param para;
	void *buf;

	debug("burn boot0 lsb mode!\n");

	for(i=0; i<32; i++)
		oob_buf[i] = 0xff;

	/* get nand driver version */
	NAND_GetVersion(oob_buf);
	if ((oob_buf[0]!=0xff)||(oob_buf[1]!= 0x00))
	{
		debug("get flash driver version error!");
		goto error;
	}

	/* lsb enable */
	debug("lsb enalbe\n");
	debug("read retry mode: 0x%x\n", read_retry_type);
	if ( NFC_LSBInit(read_retry_type) )
	{
		debug("lsb init failed.\n");
		goto error;
	}
	NFC_LSBEnable(0, read_retry_type);
	
	page_size = NAND_GetPageSize();
	{
		if (page_size % 1024)
		{
			debug("get flash page size error!\n");
			goto error;
		}
	}
	page_size = 8192;

	/* burn boot0 */
	for (i=NAND_BOOT0_BLK_START; i<(NAND_BOOT0_BLK_START + NAND_BOOT0_BLK_CNT); i++)
	{
		debug("burn boot0 %x \n", i);

		para.chip  = 0;
		para.block = i;
		if (PHY_SimpleErase( &para ) < 0)
		{
			debug("Fail in erasing block %d\n", i);
			//continue;
		}
		debug("after erase.\n" );

		for (k=0; k<4; k++)
		{
			para.chip  = 0;
			para.block = i;
			para.page  = k;
			para.mainbuf = (void *)(Boot0_buf + k * page_size);
			para.oobbuf = oob_buf;
			if (PHY_SimpleWrite_Seq( &para ) < 0)
			{
				debug("Warning. Fail in writing page %d in block %d\n", k, i);
			}
		}
	}

	for(i=0; i<32; i++)
		oob_buf[i]=0x55;

	//check boot0
	buf = (void *)kmalloc(page_size, GFP_KERNEL);
	if(!buf){
		debug("malloc fail\n");
		goto exit;
	}	
	for (i=NAND_BOOT0_BLK_START; i<(NAND_BOOT0_BLK_START + NAND_BOOT0_BLK_CNT); i++)
	{
		debug("check boot0 %x \n", i);
		
		for (k=0; k<4; k++)
		{
			para.chip  = 0;
			para.block = i;
			para.page  = k;
			para.mainbuf = (void *)(buf);
			para.oobbuf = oob_buf;
			if( PHY_SimpleRead_Seq( &para ) <0 )
			{
				debug("Warning. Fail in reading page %d in block %d.\n",  k, i );
			}
		}
	}
	
exit:

	/* lsb disable */
	NFC_LSBDisable(0, read_retry_type);
    NFC_LSBExit(read_retry_type);
    debug("lsb disalbe 0\n");
    NFC_LSBExit(read_retry_type);
    debug("lsb disalbe 1\n");
    NFC_LSBExit(read_retry_type);
    debug("lsb disalbe 2\n");
    NFC_LSBExit(read_retry_type);
	debug("lsb disalbe 3\n");
	debug("lsb disalbe \n");
	return 0;

error:
	return -1;
}


__s32  burn_boot0_lsb_FF_mode(__u32 read_retry_type, __u32 Boot0_buf )
{
    __u32 i,k;
    __u8  oob_buf[64];
    __u32 page_size;
	__u8 * data_FF_buf;
	__u32 data_debug[2];
    struct boot_physical_param  para;
   void* buf;

    debug("burn boot0 lsb + FF mode!\n");

	data_FF_buf = kmalloc(18048,GFP_KERNEL);
	if(data_FF_buf == NULL)
	{
		debug("data_FF_buf malloc error!");
		return -1;
	}

	for(i=0;i<(16384+1664);i++)
		*((__u8 *)data_FF_buf + i) = 0xFF;

    for(i=0;i<64;i++)
        oob_buf[i] = 0xff;

	/* get nand driver version */
    NAND_GetVersion(oob_buf);
	if((oob_buf[0]!=0xff)||(oob_buf[1]!= 0x00))
	{
		debug("get flash driver version error!");
		goto error;
	}

	/* ��� page count */
	page_size = NAND_GetPageSize();
	{
		if(page_size %1024)
		{
			debug("get flash page size error!");
			goto error;
		}
	}

	data_debug[0] = *((__u32 *)Boot0_buf);
	data_debug[1] = *((__u32 *)(Boot0_buf + page_size));

	/* burn boot0 */
    for( i = NAND_BOOT0_BLK_START;  i < (NAND_BOOT0_BLK_START + NAND_BOOT0_BLK_CNT);  i++ )
    {
        debug("down boot0 %x \n", i);

		/* ������ */
		para.chip  = 0;
		para.block = i;
		if( PHY_SimpleErase( &para ) <0 )
		{
		    debug("Fail in erasing block %d.\n", i );
    		//continue;
    	}

        /* �ڿ�����дboot0����, lsb mode�£�ÿ����ֻ��дǰ2��page */
		for( k = 0;  k < 5;  k++ )
		{
			if(k<2)
			{
				para.chip  = 0;
				para.block = i;
				para.page  = k;
				para.mainbuf = (void *) (Boot0_buf + k * page_size);
				para.oobbuf = oob_buf;
				if( PHY_SimpleWrite_Seq_16K( &para ) <0 )
				{
					debug("Warning. Fail in writing page %d in block %d.\n", k, i );
	   			}
			}
			if(k == 3)
			{
				para.chip  = 0;
				para.block = i;
				para.page  = k;
				para.mainbuf = (void *) (Boot0_buf);
				para.oobbuf = oob_buf;
				if( PHY_SimpleWrite_Seq_16K( &para ) <0 )
				{
					debug("Warning. Fail in writing page %d in block %d.\n", k, i );
	   			}
			}
			else
			{
				para.chip  = 0;
				para.block = i;
				para.page  = k;
				para.mainbuf = (void *) data_FF_buf ;
				para.oobbuf = oob_buf;
				if( PHY_SimpleWrite_0xFF( &para ) <0 )
				{
					debug("Warning. Fail in writing page %d in block %d.\n", k, i );
	   			}
			}
			
   		}

    }

    //check boot0
	buf = (void *)kmalloc(page_size, GFP_KERNEL);
	if(!buf){
		debug("malloc fail\n");
		goto exit;
	}	    
    
    for( i = NAND_BOOT0_BLK_START;  i < (NAND_BOOT0_BLK_START + NAND_BOOT0_BLK_CNT);  i++ )
    {
		struct boot_physical_param  para;
		__u32  k;

        debug("verify boot0 %x \n", i);

        /* �ڿ�����дboot0����, lsb mode�£�ÿ����ֻ��дǰ2��page */
		for( k = 0;  k < 2;  k++ )
		{
			para.chip  = 0;
			para.block = i;
			para.page  = k;
			para.mainbuf = (void *) (buf);
			para.oobbuf = oob_buf;
			if( PHY_SimpleRead_Seq_16K( &para ) <0 )
			{
				debug("Warning. Fail in reading page %d in block %d.\n",  k, i );
   			}
			if(data_debug[k] != *((__u32 *)(buf)))
				debug("Warning. data in reading page %d in block %d error.\n",  k, i );
   		}

    }
exit:

	kfree(data_FF_buf);
	return 0;

error:
    return -1;
}


int NAND_BurnBoot0(unsigned int length, void *buf)
{
	__u32 read_retry_type = 0, read_retry_mode;
	void *buffer = NULL;
	
	NAND_PhysicLock();
	PHY_WaitAllRbReady();
	
	debug("buf_from %x \n",buf);
	buffer = (void *)kmalloc(length, GFP_KERNEL);
	if(buffer == NULL)
	{
		debug("no memory!\n");
		NAND_PhysicUnLock();
		return -1;
	}
	debug("buf_kmalloc %x \n", buffer);
	
	if ( copy_from_user(buffer, (const void*)buf, length) ) 
	{
		debug("copy_from_user error!\n");	
		goto error;
	}

	if ( get_dram_para(buffer) ) {
		debug("get dram para error\n");
		goto error;
	} else {
		debug("get dram para ok\n");
	}
	get_nand_para(buffer);
	debug("get nand para ok\n");
	gen_check_sum(buffer);
	debug("get check sum ok\n");

	PHY_Readretry_reset();
	read_retry_type = NAND_GetReadRetryType();
	read_retry_mode = (read_retry_type>>16)&0xff;
	if ((read_retry_type>0) && (read_retry_mode<0x10))
	{
		if(read_retry_mode == 0x4)
		{
			if(NAND_GetPageSize()==8192)
			{
				if( burn_boot0_1k_mode_F16(read_retry_type, (__u32)buffer) )
			  	goto error;
			}
			else
			{
				debug("NAND_BurnBoot0:burn_boot0_lsb_FF_mode start\n");
				if( burn_boot0_lsb_FF_mode(read_retry_type, (__u32)buffer) )
			        goto error;
			}
		}
		else
		{
			if( burn_boot0_lsb_mode(read_retry_type, (__u32)buffer) )
	        	goto error;
		}

	}
	else
	{
		if( burn_boot0_1k_mode_F16(read_retry_type, (__u32)buffer) )
			goto error;
	}
	debug("burn boot 0 success\n");
	kfree(buffer);
	PHY_WaitAllRbReady();
	NAND_PhysicUnLock();
	return 0;

error:
	debug("burn boot 0 failed\n");
	kfree(buffer);
	PHY_WaitAllRbReady();
	NAND_PhysicUnLock();
	return -1;
}

__s32 read_boot0_1k_mode(__u32 read_retry_type, __u32 Boot0_buf)
{
	__u32 i, j, k,m,err_flag;
	__u32 pages_per_block;
	__u32 copies_per_block;
	__u8  oob_buf[32];
	struct boot_physical_param para;

	debug("read boot0 normal mode!\n");

	pages_per_block = NAND_GetPageCntPerBlk();
	if (pages_per_block%64)
	{
		debug("get page cnt per block error %x!", pages_per_block);
		goto error;
	}

	/* cal copy cnt per bock */
	copies_per_block = pages_per_block / NAND_BOOT0_PAGE_CNT_PER_COPY;

	/* read boot0 copys, until get a right copy or read all copys */
	for (i=NAND_BOOT0_BLK_START; i<(NAND_BOOT0_BLK_START + NAND_BOOT0_BLK_CNT); i++)
	{
		debug("boot0 blk %x \n", i);

		para.chip  = 0;
		para.block = i;

		for (j=0; j<copies_per_block; j++)
		{
			err_flag = 0;
			for (k =0; k<NAND_BOOT0_PAGE_CNT_PER_COPY; k++)
			{
				para.chip  = 0;
				para.block = i;
				para.page = j * NAND_BOOT0_PAGE_CNT_PER_COPY + k;
				para.mainbuf = (void *)(Boot0_buf + k * 1024);
				para.oobbuf = oob_buf;
				for (m=0; m<32; m++)
					oob_buf[m] = 0x55;
				if (PHY_SimpleRead_1K( &para ) < 0)
				{
					debug("Warning. Fail in read page %d in block %d.\n", j * NAND_BOOT0_PAGE_CNT_PER_COPY + k, i );
					err_flag = 1;
					break;
				}
				if ((oob_buf[0]!=0xff) || (oob_buf[1]!= 0x00))
				{
					debug("get flash driver version error!\n");
					err_flag = 1;
					break;
				}
			}
			if (err_flag == 0)
				break;
		}
		if (err_flag == 0)
			break;
	}
	return 0;

error:
	return -1;
}

__s32 read_boot0_lsb_mode(__u32 read_retry_type, __u32 Boot0_buf)
{
	__u32 i, j, k, err_flag;
	__u8  oob_buf[32];
	__u32 page_size;
	struct boot_physical_param para;

	page_size = NAND_GetPageSize();
	{
		if (page_size % 1024) 
		{
			debug("get flash page size error!\n");
			goto error;
		}
	}
	page_size = 8192;

	//check boot0
	for (i=NAND_BOOT0_BLK_START; i<(NAND_BOOT0_BLK_START + NAND_BOOT0_BLK_CNT); i++)
	{
		debug("boot0 blk %x \n", i);
		err_flag = 0;

		for (k=0; k<4; k++)
		{
			para.chip  = 0;
			para.block = i;
			para.page  = k;
			para.mainbuf = (void *) (Boot0_buf + k * page_size);
			para.oobbuf = oob_buf;

			for(j=0; j<32; j++)
				oob_buf[j] = 0x55;

			if (PHY_SimpleRead_Seq( &para ) < 0)
			{
				debug("Warning. Fail in reading page %d in block %d.\n",  k, i );
				err_flag = 1;
				break;
			}
			if ((oob_buf[0]!=0xff) || (oob_buf[1]!= 0x00))
			{
				debug("get flash driver version error!");
				err_flag = 1;
				break;
			}
		}
		if(err_flag == 0)
			break;
	}
	return 0;

error:
	return -1;
}

__s32  read_boot0_lsb_FF_mode(__u32 read_retry_type, __u32 Boot0_buf )
{
    __u32 i,k,n;
    __u8  oob_buf[32];
    __u32 page_size;
	__u32 err_flag;
    struct boot_physical_param  para;

    debug("read boot0 lsb + FF mode!\n");

	page_size = NAND_GetPageSize();
	{
		if(page_size %1024)
		{
			debug("get flash page size error!\n");
			goto error;
		}
	}

	/* read boot0 */
    for( i = NAND_BOOT0_BLK_START;  i < (NAND_BOOT0_BLK_START + NAND_BOOT0_BLK_CNT);  i++ )
    {
        debug("blk %x \n", i);

		err_flag = 0;
		for( k = 0;  k < 2;  k++ )
		{
				
			para.chip  = 0;
			para.block = i;
			para.page  = k;
			para.mainbuf = (void *) (Boot0_buf + k * page_size);
			para.oobbuf = oob_buf;
			
			for(n=0;n<32;n++)
    			oob_buf[n] = 0x55;
				
			if( PHY_SimpleRead_Seq_16K( &para ) <0 )
			{
				debug("Warning. Fail in reading page %d in block %d.\n",  k, i );
				err_flag = 1;
				break;
			}
			if ((oob_buf[0]!=0xff) || (oob_buf[1]!= 0x00))
			{
				debug("get flash driver version error!\n");
				err_flag = 1;
				break;
			}
		}
		if(err_flag == 0)
			break;
        
    }
	if(err_flag == 1)
		return -1;
	else
		return 0;

error:
    return -1;
}


int NAND_ReadBoot0(unsigned int length, void *buf)
{
	__u32 read_retry_type = 0, read_retry_mode;
	void *buffer = NULL;

	PHY_WaitAllRbReady();

//	NAND_PhysicLock();
	buffer =(void *)kmalloc(1024*512,GFP_KERNEL);
	if (buffer == NULL)
	{
		debug("no memory!\n");
		return -1;
	}

	PHY_Readretry_reset();
	read_retry_type = NAND_GetReadRetryType();
	read_retry_mode = (read_retry_type>>16)&0xff;
	if ((read_retry_type>0) && (read_retry_mode < 0x10))
	{
		if((read_retry_mode == 0x4)&&(NAND_GetPageSize()==8192))
		{
			if ( read_boot0_1k_mode(read_retry_type, (__u32)buffer) )
				goto error;
		}
		else if((read_retry_mode == 0x4)&&(NAND_GetPageSize()==16384))
		{
			if ( read_boot0_lsb_FF_mode(read_retry_type, (__u32)buffer) )
				goto error;
		}
		else
		{
			if( read_boot0_lsb_mode(read_retry_type, (__u32)buffer) )
				goto error;
		}
	}
	else
	{
		if ( read_boot0_1k_mode(read_retry_type, (__u32)buffer) )
			goto error;
	}

	memcpy(buf, buffer, length);
	kfree(buffer);
	buffer = NULL;
	debug("nand read boot0 ok\n");
//	NAND_PhysicUnLock();
	return 0;

error:
	kfree(buffer);
	buffer = NULL;
	debug("nand read boot0 fail\n");
	return -1;
}

__s32 burn_boot1_in_one_blk(__u32 BOOT1_buf, __u32 length)
{
	__u32 i, k;
	__u8  oob_buf[32];
	__u32 page_size, pages_per_block, pages_per_copy;
	struct boot_physical_param para;

	debug("burn boot1 normal mode!\n");

	for (i=0; i<32; i++)
		oob_buf[i] = 0xff;

	/* get nand driver version */
	NAND_GetVersion(oob_buf);
	if ((oob_buf[0]!=0xff) || (oob_buf[1]!=0x00))
	{
		debug("get flash driver version error!\n");
		goto error;
	}

	page_size = NAND_GetPageSize();
	{
		if (page_size % 1024)
		{
			debug("get flash page size error!\n");
			goto error;
		}
	}

	pages_per_block = NAND_GetPageCntPerBlk();
	if (pages_per_block % 64)
	{
		debug("get page cnt per block error %x!\n", pages_per_block);
		goto error;
	}
	debug("pages_per_block: 0x%x\n", pages_per_block);

	if (length % page_size)
	{
		debug("uboot length check error!\n");
		goto error;
	}
	pages_per_copy = length / page_size;
	if (pages_per_copy > pages_per_block)
	{
		debug("pages_per_copy check error!\n");
		goto error;
	}
	debug("pages_per_copy: 0x%x\n", pages_per_copy);

	/* burn uboot */
	for (i=NAND_UBOOT_BLK_START; i<(NAND_UBOOT_BLK_START + NAND_UBOOT_BLK_CNT); i++)
	{
		debug("boot1 %x \n", i);

		para.chip  = 0;
		para.block = i;
		if( PHY_SimpleErase( &para ) < 0 )
		{
			debug("Fail in erasing block %d.\n", i );
			//continue;
		}
	
		for (k=0; k<pages_per_copy; k++)
		{
			para.chip  = 0;
			para.block = i;
			para.page  = k;
			para.mainbuf = (void *)(BOOT1_buf + k * page_size);
			para.oobbuf = oob_buf;
			//debug("burn uboot: block: 0x%x, page: 0x%x, mainbuf: 0x%x, maindata: 0x%x \n", para.block, para.page, (__u32)para.mainbuf, *((__u32 *)para.mainbuf));
			if ( PHY_SimpleWrite( &para ) <0 )
			{
				debug("Warning. Fail in writing page %d in block %d.\n", k, i);
			}
		}
		debug("fill uboot block with dummy data\n");
		for (k=pages_per_copy; k<pages_per_block; k++)
		{
			para.chip  = 0;
			para.block = i;
			para.page  = k;
			para.mainbuf = (void *)(BOOT1_buf + k * page_size);
			para.oobbuf = oob_buf;
			//debug("burn uboot: block: 0x%x, page: 0x%x, mainbuf: 0x%x, maindata: 0x%x \n", para.block, para.page, (__u32)para.mainbuf, *((__u32 *)para.mainbuf));
			if ( PHY_SimpleWrite( &para ) <0 )
			{
				debug("Warning. Fail in writing page %d in block %d.\n", k, i);
			}
		}		
	}
	return 0;

error:
	return -1;
}

__s32 burn_boot1_in_many_blks(__u32 BOOT1_buf, __u32 length)
{
	__u32 i,  k;
	__u8  oob_buf[32];
	__u32 page_size, pages_per_block, pages_per_copy, page_index;
	struct boot_physical_param  para;

	debug("burn uboot normal mode!\n");

	for (i=0; i<32; i++)
		oob_buf[i] = 0xff;

	/* get nand driver version */
	NAND_GetVersion(oob_buf);
	if ((oob_buf[0]!=0xff) || (oob_buf[1]!= 0x00))
	{
		debug("get flash driver version error!\n");
		goto error;
	}

	page_size = NAND_GetPageSize();
	{
		if (page_size % 1024)
		{
			debug("get flash page size error!\n");
			goto error;
		}
	}

	pages_per_block = NAND_GetPageCntPerBlk();
	if (pages_per_block % 64)
	{
		debug("get page cnt per block error %x!\n", pages_per_block);
		goto error;
	}

	if (length % page_size)
	{
		debug("uboot length check error!\n");
		goto error;
	}
	
	pages_per_copy = length / page_size;
	if (pages_per_copy <= pages_per_block)
	{
		debug("pages_per_copy check error!\n");
		goto error;
	}

	/* burn uboot, only one copy!!!*/
	page_index = 0;
	for (i=NAND_UBOOT_BLK_START; i<(NAND_UBOOT_BLK_START + NAND_UBOOT_BLK_CNT); i++)
	{
		debug("uboot %x \n", i);
	
		para.chip  = 0;
		para.block = i;
		if (PHY_SimpleErase( &para ) < 0)
		{
			debug("Fail in erasing block %d.\n", i );
			continue;
		}
	
		for (k =0; k<pages_per_block; k++)
		{
			para.chip  = 0;
			para.block = i;
			para.page  = k;
			para.mainbuf = (void *)(BOOT1_buf + page_index* page_size);
			para.oobbuf = oob_buf;
			if (PHY_SimpleWrite( &para ) < 0)
			{
				debug("Warning. Fail in writing page %d in block %d.\n", k, i);
			}
			page_index++;
	
			if (page_index >= pages_per_copy)
				break;
		}
	
		if (page_index >= pages_per_copy)
			break;
	}

	if(page_index >= pages_per_copy)
		return 0;
	else
		goto error;

error:
	return -1;
}

int NAND_BurnBoot1(unsigned int length, void *buf)
{
	int ret = 0;
	__u32 page_size, pages_per_block, block_size;
	void *buffer = NULL;

	NAND_PhysicLock();
	PHY_WaitAllRbReady();

	buffer = (void *)kmalloc(length, GFP_KERNEL);
	if (buffer == NULL)
	{
		debug("no memory!\n");
		NAND_PhysicUnLock();
		return -1;
	}

	if ( copy_from_user(buffer, (const void*)buf, length) )
	{
		debug("NAND_BurnBoot1, copy_from_user error!\n");
		goto error;
	}

	get_nand_para_for_boot1(buffer);
	gen_uboot_check_sum(buffer);

	PHY_Readretry_reset();

	page_size = NAND_GetPageSize();
	{
		if (page_size % 1024)
		{
			debug("get flash page size error!\n");
			goto error;
		}
	}

	pages_per_block = NAND_GetPageCntPerBlk();
	if (pages_per_block % 64)
	{
		debug("get page cnt per block error %x!\n", pages_per_block);
		goto error;
	}

	block_size = page_size*pages_per_block;
	if (length % page_size)
	{
		debug("uboot length check error!\n");
		goto error;
	}

	if (length <= block_size)
	{
		ret = burn_boot1_in_one_blk((__u32)buffer, length);
		debug("NAND_BurnBoot1, in one blk, %d\n", ret);
	}
	else
	{
		ret = burn_boot1_in_many_blks((__u32)buffer, length);
		debug("NAND_BurnBoot1, in many blks, %d\n", ret);
	}
		
	kfree(buffer);
	buffer = NULL;
	PHY_WaitAllRbReady();
	NAND_PhysicUnLock();
	debug("burn boot 1 success\n");
	return ret;

error:
	kfree(buffer);
	buffer = NULL;
	PHY_WaitAllRbReady();
	NAND_PhysicUnLock();
	debug("burn boot 1 failed\n");
	return -1;
}

void test_dram_para(void *buffer)
{
	int *data;
	int  i;

	data = (int *)buffer;
	for (i=0; i<40; i+=4)
	{
		debug("%x %x %x %x\n", data[i+0], data[i+1], data[i+2], data[i+3]);
	}
	debug("\n");

	return ;
}

__u32  PHY_erase_chip(void)
{
	struct boot_physical_param  para_read;
	int  i,j,k;
	int  ret;
	uint  bad_block_flag;
	uint  chip_cnt, page_size, page_per_block, blk_cnt_per_chip;
	uint block_cnt_of_die, die_skip_flag, start_blk;
	int  page_index[4];
	uint  chip;
	unsigned char   oob_buf_read[OOB_BUF_SIZE];
	unsigned char*  page_buf_read;
	int  error_flag = 0;

    page_buf_read = (unsigned char*)kmalloc(64 * 1024,GFP_KERNEL);
    if(!page_buf_read)
    {
        debug("malloc memory for page read fail\n");
        return -1;
    }
	debug("Ready to erase chip.\n");

	page_size = NAND_GetPageSize();
	page_per_block = NAND_GetPageCntPerBlk();
	blk_cnt_per_chip = NAND_GetBlkCntPerChip();
	debug("page_size=%d, page_per_block=%d, blk_cnt_per_chip=%d\n", page_size, page_per_block, blk_cnt_per_chip);
	chip_cnt = NAND_GetChipCnt();
    debug("chip_cnt = %x\n",chip_cnt);
    block_cnt_of_die = NAND_GetBlkCntOfDie();
    die_skip_flag = NAND_GetDieSkipFlag();

    page_index[0] = 0;
	page_index[1] = page_per_block - 1;;
	page_index[2] = 0xEE;
	page_index[3] = 0xEE;


	for( i = 0;  i < chip_cnt;  i++ )
	{
	    //select chip
		chip = _cal_real_chip( i);
        debug("erase chip %u \n", chip);

		start_blk = 0;

        //scan for bad blocks, only erase good block, all 0x00 blocks is defined bad blocks
		for( j = start_blk;  j < blk_cnt_per_chip;  j++ )
		{


			if(!die_skip_flag)
			    para_read.block = j;
			else
			    para_read.block = j%block_cnt_of_die + 2*block_cnt_of_die*(j/block_cnt_of_die);

			if((j&0xff) == 0)
				debug("erase chip %u, block %u\n",chip, para_read.block);

			para_read.chip = chip;
			para_read.mainbuf = page_buf_read;
			para_read.oobbuf = oob_buf_read;

			bad_block_flag = 0;

			for(k = 0; k<4; k++)
			{
				para_read.page = page_index[k];
				if( para_read.page== 0xEE)
				    break;

				ret = PHY_SimpleRead_2CH(& para_read );

				//check the current block is a all 0x00 block
				if(oob_buf_read[0] == 0x0)
				{
					bad_block_flag = 1;
					debug("find a bad block %u\n", para_read.block);
					break;
				}

			}

			if(bad_block_flag)
				continue;


			ret = PHY_SimpleErase_2CH( &para_read );
			if( ret != 0 )
	    	{
	    		debug("erasing block %u failed.\n", para_read.block );
    		}
		}
	}
	debug("has cleared the chip.\n");
	if(error_flag)
		debug("the nand is Bad.\n");
	else
		debug("the nand is OK.\n");

    kfree(page_buf_read);

	return 0;

}
