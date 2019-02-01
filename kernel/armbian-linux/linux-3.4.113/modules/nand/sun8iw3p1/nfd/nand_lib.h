/*
************************************************************************************************************************
*                                                      eNand
*                                     Nand flash driver logic control module define
*
*                             Copyright(C), 2008-2009, SoftWinners Microelectronic Co., Ltd.
*											       All Rights Reserved
*
* File Name : bsp_nand.h
*
* Author : Kevin.z
*
* Version : v0.1
*
* Date : 2008.03.25
*
* Description : This file define the function interface and some data structure export
*               for the nand bsp.
*
* Others : None at present.
*
*
* History :
*
*  <Author>        <time>       <version>      <description>
*
* Kevin.z         2008.03.25      0.1          build the file
*
************************************************************************************************************************
*/
#ifndef __BSP_NAND_H__
#define __BSP_NAND_H__

#include "nand_type.h"

//for partition
#define MAX_PART_COUNT_PER_FTL		24
#define MAX_PARTITION        		4
#define ND_MAX_PARTITION_COUNT      (MAX_PART_COUNT_PER_FTL*MAX_PARTITION)


typedef struct
{
	__u32		ChannelCnt;
	__u32        ChipCnt;                            //the count of the total nand flash chips are currently connecting on the CE pin
    __u32       ChipConnectInfo;                    //chip connect information, bit == 1 means there is a chip connecting on the CE pin
	__u32		RbCnt;
	__u32		RbConnectInfo;						//the connect  information of the all rb  chips are connected
    __u32        RbConnectMode;						//the rb connect  mode
	__u32        BankCntPerChip;                     //the count of the banks in one nand chip, multiple banks can support Inter-Leave
    __u32        DieCntPerChip;                      //the count of the dies in one nand chip, block management is based on Die
    __u32        PlaneCntPerDie;                     //the count of planes in one die, multiple planes can support multi-plane operation
    __u32        SectorCntPerPage;                   //the count of sectors in one single physic page, one sector is 0.5k
    __u32       PageCntPerPhyBlk;                   //the count of physic pages in one physic block
    __u32       BlkCntPerDie;                       //the count of the physic blocks in one die, include valid block and invalid block
    __u32       OperationOpt;                       //the mask of the operation types which current nand flash can support support
    __u32        FrequencePar;                       //the parameter of the hardware access clock, based on 'MHz'
    __u32        EccMode;                            //the Ecc Mode for the nand flash chip, 0: bch-16, 1:bch-28, 2:bch_32
    __u8        NandChipId[8];                      //the nand chip id of current connecting nand chip
    __u32       ValidBlkRatio;                      //the ratio of the valid physical blocks, based on 1024
	__u32 		good_block_ratio;					//good block ratio get from hwscan
	__u32		ReadRetryType;						//the read retry type
	__u32       DDRType;
	__u32		Reserved[22];
}boot_nand_para_t;

typedef struct boot_flash_info{
	__u32 chip_cnt;
	__u32 blk_cnt_per_chip;
	__u32 blocksize;
	__u32 pagesize;
	__u32 pagewithbadflag; /*bad block flag was written at the first byte of spare area of this page*/
}boot_flash_info_t;


struct boot_physical_param{
	__u32   chip; //chip no
	__u32  block; // block no within chip
	__u32  page; // apge no within block
	__u32  sectorbitmap; //done't care
	void   *mainbuf; //data buf
	void   *oobbuf; //oob buf
};

#define PARTITION_NAME_SIZE  16

struct _nand_disk{
	unsigned int		size;
	//unsigned int offset;
	unsigned int		type;
	unsigned  char      name[PARTITION_NAME_SIZE];
};

struct _nand_phy_partition{
    void * dat;
};


//extern struct _nand_phy_partition;

struct _nftl_blk{
	unsigned int                            nftl_logic_size;
	struct _nand_partition*                 nand;
	struct _nftl_blk*                       nftl_blk_next;
	void*                                   nftl_zone;
	void*                                   cfg;
	unsigned int		                    time;
	unsigned int		                    time_flush;
	void*                                   nftl_thread;
    void*                                   blk_lock;
	int (*read_data)               (struct _nftl_blk *nftl_blk, unsigned int sector, unsigned int len, unsigned char *buf);
	int (*write_data)              (struct _nftl_blk *nftl_blk, unsigned int sector, unsigned int len, unsigned char *buf);
	int (*flush_write_cache)       (struct _nftl_blk *nftl_blk,unsigned int num);
	int (*shutdown_op)             (struct _nftl_blk *nftl_blk);
	int (*read_sector_data)        (struct _nftl_blk *nftl_blk, unsigned int sector, unsigned int len, unsigned char *buf);
	int (*write_sector_data)       (struct _nftl_blk *nftl_blk, unsigned int sector, unsigned int len, unsigned char *buf);
	int (*flush_sector_write_cache)(struct _nftl_blk *nftl_blk,unsigned int num);
};

extern __s32 PHY_SimpleErase(struct boot_physical_param * eraseop);
extern __s32 PHY_SimpleErase_2CH(struct boot_physical_param * eraseop);
extern __s32 PHY_SimpleErase_CurCH(struct boot_physical_param * eraseop);
extern __s32 PHY_SimpleRead(struct boot_physical_param * readop);
extern __s32 PHY_SimpleRead_CurCH(struct boot_physical_param * readop);
extern __s32 PHY_SimpleRead_2CH (struct boot_physical_param *readop);
extern __s32 PHY_SimpleWrite(struct boot_physical_param * writeop);
extern __s32 PHY_SimpleWrite_CurCH(struct boot_physical_param * writeop);
extern __s32 PHY_SimpleWrite_1K(struct boot_physical_param * writeop);
extern __s32 PHY_SimpleWrite_1KCurCH(struct boot_physical_param * writeop);
extern __s32 PHY_SimpleWrite_Seq(struct boot_physical_param * writeop);
extern __s32 PHY_SimpleRead_Seq(struct boot_physical_param * readop);
extern __s32 PHY_SimpleRead_1K(struct boot_physical_param * readop);
extern __s32 PHY_SimpleRead_1KCurCH(struct boot_physical_param * readop);
extern __s32 PHY_WaitAllRbReady(void);
extern __s32 PHY_SimpleWrite_0xFF(struct boot_physical_param *writeop);
extern __s32 PHY_SimpleWrite_Seq_16K(struct boot_physical_param *writeop);
extern __s32 PHY_SimpleRead_Seq_16K (struct boot_physical_param *readop);
extern __u8 _cal_real_chip(__u32 global_bank);
extern __s32 PHY_SimpleWrite_1K_F16(struct boot_physical_param *writeop);



extern __s32 NFC_LSBEnable(__u32 chip, __u32 read_retry_type);
extern __s32 NFC_LSBDisable(__u32 chip, __u32 read_retry_type);
extern __s32 NFC_LSBInit(__u32 read_retry_type);
extern __s32 NFC_LSBExit(__u32 read_retry_type);
extern __u32 NAND_GetChannelCnt(void);

extern void ClearNandStruct( void );

//for param get&set
extern __u32 NAND_GetFrequencePar(void);
extern __s32 NAND_SetFrequencePar(__u32 FrequencePar);
extern __u32 NAND_GetNandVersion(void);
extern __s32 NAND_GetParam(boot_nand_para_t * nand_param);
extern __s32 NAND_GetFlashInfo(boot_flash_info_t *info);

struct _nand_info* NandHwInit(void);
__s32 NandHwExit(void);
__s32 NandHwSuperStandby(void);
__s32 NandHwSuperResume(void);
__s32 NandHwNormalStandby(void);
__s32 NandHwNormalResume(void);
__s32 NandHwShutDown(void);

int test_mbr(uchar* data);

//for NFTL
int nftl_initialize(struct _nftl_blk *nftl_blk,int no);
extern int nand_info_init(struct _nand_info* nand_info,unsigned char chip,uint16 start_block,unsigned char* mbr_data);


#endif  //ifndef __BSP_NAND_H__
