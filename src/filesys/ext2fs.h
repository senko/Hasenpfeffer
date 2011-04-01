#ifndef _EXT2FS_H_
#define _EXT2FS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "linux_ext2_fs.h"

extern bool ext2_mount(FILE *fp);
extern bool ext2_read_file_block(struct ext2_inode *inode, uint32_t blknum, uint8_t *buffer);
extern bool ext2_read_file_block_multiple(struct ext2_inode *inode, uint32_t blknum, int count, uint8_t *buffer);
extern bool ext2_lookup_inode(uint32_t inum, struct ext2_inode *buf);
extern bool ext2_lookup_path(char *path, uint32_t *inum);

#endif

