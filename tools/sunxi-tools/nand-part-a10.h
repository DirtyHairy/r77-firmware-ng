/*
 * drivers/block/sun4i_nand/nfd/mbr.h
 *
 * (C) Copyright 2007-2012
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef    __MBR_H__
#define    __MBR_H__

#include "types.h"

#define MBR_MAGIC "softw311"
#define MBR_VERSION 0x100
#define nand_part nand_part_a10
#define checkmbrs checkmbrs_a10

#define MAX_PART_COUNT		15	 									//max part count
#define MBR_COPY_NUM        4    									//mbr backup count

#define MBR_START_ADDRESS	0x0										//mbr start address
#define MBR_SIZE			1024									//mbr size
#define MBR_RESERVED        (MBR_SIZE - 20 - (MAX_PART_COUNT * 64)) //mbr reserved space

// extern struct __NandDriverGlobal_t NandDriverInfo;

// extern struct __NandStorageInfo_t  NandStorageInfo;

#define DiskSize  (SECTOR_CNT_OF_SINGLE_PAGE * PAGE_CNT_OF_PHY_BLK * BLOCK_CNT_OF_DIE * \
            DIE_CNT_OF_CHIP * NandStorageInfo.ChipCnt  / 1024 * DATA_BLK_CNT_OF_ZONE)


struct nand_disk{
	unsigned long size;
	unsigned long offset;
	unsigned char type;
};

/* part info */
typedef struct tag_PARTITION{
	__u32 addrhi;				//start address high 32 bit
	__u32 addrlo;				//start address low 32 bit
	__u32 lenhi;				//size high 32 bit
	__u32 lenlo;				//size low 32 bit
	__u8  classname[12];		//major device name
	__u8  name[12];				//minor device name
	unsigned  int       user_type;          //标志当前盘符所属于的用户
	unsigned  int       ro;                 //标志当前盘符的读写属性
	__u8  res[16];				//reserved
}PARTITION;

/* mbr info */
typedef struct tag_MBR{
	__u32 crc32;					// crc, from byte 4 to mbr tail
	__u32 version;					// version
	__u8  magic[8];					// magic number
	__u8  copy;						// mbr backup count
	__u8  index;					// current part	no
	__u16   PartCount;				// part counter
	PARTITION array[MAX_PART_COUNT];// part info
	__u8 res[MBR_RESERVED];         // reserved space
}MBR;

int mbr2disks(struct nand_disk* disk_array);

#endif    //__MBR_H__
