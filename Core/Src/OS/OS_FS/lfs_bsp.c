/*
 * lfs_bsp.c
 *
 *  Created on: Jul 9, 2022
 *      Author: Gabriel
 */

#include "common.h"

/**********************************************************
 * DEFINES
 **********************************************************/

#define CACHE_SIZE 1024

/**********************************************************
 * PUBLIC VARIABLES
 **********************************************************/

lfs_t lfs;

/**********************************************************
 * PRIVATE VARIABLES
 **********************************************************/

static uint8_t __align(8) lfs_lookahead[16];
static uint8_t __align(8) lfs_read_cache[CACHE_SIZE];
static uint8_t __align(8) lfs_prog_cache[CACHE_SIZE];

/**********************************************************
 * PRIVATE FUNCTIONS
 **********************************************************/

/***********************************************************************
 * LFS Read Flash
 *
 * @brief This function reads a number of bytes starting at a position in flash
 *
 * @param const struct lfs_config *c	: [ in] Configuration file used
 * @param lfs_block_t block				: [ in] The index of the block
 * @param lfs_off_t off					: [ in] The offset inside the block
 * @param void *buffer					: [out] Output buffer
 * @param lfs_size_t size				: [ in] Size of the output buffer
 *
 * @return int : 0 if OK, != 0 if problem
 **********************************************************************/
static int lfs_flash_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size){

	/* Calculate first byte's address
	 ------------------------------------------------*/
	uint32_t addr = block * SECTOR_SIZE + off + LFS_BASE_ADDR;

	/* Check arguments
	 ------------------------------------------------*/
	if(size == 0) return -5;
	if(addr < LFS_BASE_ADDR) return -5;
	if(addr >= LFS_END_ADDR) return -5;

	/* Read from flash and returns 0 if OK
	 ------------------------------------------------*/
	int32_t ret = os_flash_read(addr, buffer, size);
	ASSERT(ret >= 0);
	return ret < 0 ? (int)ret : 0;
}


/***********************************************************************
 * LFS Write Flash
 *
 * @brief This function writes a number of bytes starting at a position in flash
 *
 * @param const struct lfs_config *c	: [ in] Configuration file used
 * @param lfs_block_t block				: [ in] The index of the block
 * @param lfs_off_t off					: [ in] The offset inside the block
 * @param const void *buffer			: [ in] Input buffer
 * @param lfs_size_t size				: [ in] Size of the output buffer
 *
 * @return int : 0 if OK, != 0 if problem
 **********************************************************************/
static int lfs_flash_write(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size){

	/* Calculate address of the first byte
	 ------------------------------------------------*/
	uint32_t addr = block * SECTOR_SIZE + off + LFS_BASE_ADDR;

	/* Write buffer in flash and return 0 if OK
	 ------------------------------------------------*/
	int32_t ret = os_flash_write(addr, (uint8_t*)buffer, size);
	ASSERT(ret >= 0);
	return ret < 0 ? (int)ret : 0;
}


/***********************************************************************
 * LFS Erase Flash
 *
 * @brief This function erases a number of sectors in the flash, forcing them to 0xFF
 *
 * @param const struct lfs_config *c	: [ in] Configuration file used
 * @param lfs_block_t block				: [ in] The index of the block
 *
 * @return int : 0 if OK, != 0 if problem
 **********************************************************************/
static int lfs_flash_erase(const struct lfs_config *c, lfs_block_t block){

	/* Calculate fist address
	 ------------------------------------------------*/
	uint32_t addr = block * SECTOR_SIZE + LFS_BASE_ADDR;

	/* Erase block and return 0 if ok
	 ------------------------------------------------*/
	int32_t ret = os_flash_erase(addr, addr + SECTOR_SIZE - 1);
	ASSERT(ret >= 0);
	return ret < 0 ? (int)ret : 0;
}


/***********************************************************************
 * LFS Sync Flash
 *
 * @brief This function synchronizes the state of the block. Unused in this application
 *
 * @param const struct lfs_config *c	: [ in] Configuration file used
 *
 * @return int : 0 if OK, != 0 if problem
 **********************************************************************/
static int lfs_flash_sync(const struct lfs_config *c){
	return 0;
}

/**********************************************************
 * PRIVATE VARIABLES
 **********************************************************/

// configuration of the filesystem is provided by this struct
static struct lfs_config lfs_cfg = {

	// Read a region in a block. Negative error codes are propagated
    // to the user.
    .read = lfs_flash_read,

    // Program a region in a block. The block must have previously
    // been erased. Negative error codes are propagated to the user.
    // May return LFS_ERR_CORRUPT if the block should be considered bad.
	.prog = lfs_flash_write,

    // Erase a block. A block must be erased before being programmed.
    // The state of an erased block is undefined. Negative error codes
    // are propagated to the user.
    // May return LFS_ERR_CORRUPT if the block should be considered bad.
    .erase = lfs_flash_erase,

    // Sync the state of the underlying block device. Negative error codes
    // are propagated to the user.
	.sync = lfs_flash_sync,

    // Minimum size of a block read in bytes. All read operations will be a
    // multiple of this value.
    .read_size = 2,

    // Minimum size of a block program in bytes. All program operations will be
    // a multiple of this value.
    .prog_size = 2,

    // Number of erase cycles before littlefs evicts metadata logs and moves
    // the metadata to another block. Suggested values are in the
    // range 100-1000, with large values having better performance at the cost
    // of less consistent wear distribution.
    //
    // Set to -1 to disable block-level wear-leveling.
    .block_cycles = 500,

    // Size of block caches in bytes. Each cache buffers a portion of a block in
    // RAM. The littlefs needs a read cache, a program cache, and one additional
    // cache per file. Larger caches can improve performance by storing more
    // data and reducing the number of disk accesses. Must be a multiple of the
    // read and program sizes, and a factor of the block size.
    .cache_size = (sizeof(lfs_read_cache) / sizeof(*lfs_read_cache)),

    // Size of the lookahead buffer in bytes. A larger lookahead buffer
    // increases the number of blocks found during an allocation pass. The
    // lookahead buffer is stored as a compact bitmap, so each byte of RAM
    // can track 8 blocks. Must be a multiple of 8.
    .lookahead_size = (sizeof(lfs_lookahead) / sizeof(*lfs_lookahead)),

    // Optional statically allocated read buffer. Must be cache_size.
    // By default lfs_malloc is used to allocate this buffer.
    .read_buffer = lfs_read_cache,

    // Optional statically allocated program buffer. Must be cache_size.
    // By default lfs_malloc is used to allocate this buffer.
    .prog_buffer = lfs_prog_cache,

    // Optional statically allocated lookahead buffer. Must be lookahead_size
    // and aligned to a 32-bit boundary. By default lfs_malloc is used to
    // allocate this buffer.
    .lookahead_buffer = lfs_lookahead,
};

/**********************************************************
 * PUBLIC FUNCTIONS
 **********************************************************/


/***********************************************************************
 * LFS Init
 *
 * @brief This function initializes the file system
 *
 **********************************************************************/
void os_lfs_init(){

	/* Calculate sector size and number of sector using the pieces of information given by the linker
	 ------------------------------------------------*/
	lfs_cfg.block_size = SECTOR_SIZE;
	lfs_cfg.block_count = (uint32_t)LFS_BASE_SIZE/SECTOR_SIZE;

	/* Try to mount the file system
	 ------------------------------------------------*/
    int err = lfs_mount(&lfs, &lfs_cfg);
    if(err < 0){

    	PRINTLN("LFS mount fail %d", err);

    	/* Reformat if we can't mount the filesystem
    	 * this should only happen on the first boot
    	 ------------------------------------------------*/
        err = lfs_format(&lfs, &lfs_cfg);
        if(err < 0){
        	PRINTLN("LFS format fail %d", err);
        }

    	/* Try to mount the file system again
    	 ------------------------------------------------*/
        err = lfs_mount(&lfs, &lfs_cfg);
        if(err < 0){
        	PRINTLN("LFS mount fail %d", err);
        }

        else{
        	PRINTLN("LFS Mount OK");
        }
    }

	/* Print everything is fine
	 ------------------------------------------------*/
    else{
    	PRINTLN("LFS Mount OK");
    }

}
