/*
 * lfs_bsp.c
 *
 *  Created on: Jul 9, 2022
 *      Author: Gabriel
 */

#include "common.h"

lfs_t lfs;

#define CACHE_SIZE 1024


static uint8_t lfs_lookahead[4];
static uint8_t lfs_read_cache[CACHE_SIZE];
static uint8_t lfs_prog_cache[CACHE_SIZE];

static int lfs_flash_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size){
	return 0;
}

static int lfs_flash_write(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size){
	return 0;
}

static int lfs_flash_erase(const struct lfs_config *c, lfs_block_t block){
	return 0;
}

static int lfs_flash_sync(const struct lfs_config *c){
	return 0;
}

// configuration of the filesystem is provided by this struct
static struct lfs_config cfg = {

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

void lfs_init(){
	//cfg.block_size = SECTOR_SIZE;
	//cfg.block_count = *_LFS_SIZE/SECTOR_SIZE;
}
