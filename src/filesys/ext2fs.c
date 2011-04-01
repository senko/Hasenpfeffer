#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "ext2fs.h"

#include "linux_ext2_fs.h"

static FILE *fp;
static struct ext2_super_block sb;

/* linux/stat.h */
#define S_IFMT  00170000
#define S_IFLNK  0120000
#define S_IFREG  0100000
#define S_IFDIR  0040000
#define S_ISLNK(m)      (((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR)

bool ext2_mount(FILE *dev_fp)
{
    fp = dev_fp;
    fseek(fp, 1024, SEEK_SET);
    if (1 != fread(&sb, sizeof(sb), 1, fp)) return false;
    if (sb.s_magic != EXT2_SUPER_MAGIC) return false;
    return true;
}

static bool ext2_read_block(uint32_t blknum, int count, void *buffer)
{
    fseek(fp, blknum * EXT2_BLOCK_SIZE(&sb), SEEK_SET);
    return (count == fread(buffer, EXT2_BLOCK_SIZE(&sb), count, fp));
}

bool ext2_read_file_block(struct ext2_inode *inode, uint32_t blknum, uint8_t *buffer)
{
    uint32_t array[EXT2_MAX_BLOCK_SIZE / sizeof(uint32_t)];
    uint32_t blocksize = EXT2_ADDR_PER_BLOCK(&sb);
    uint32_t blocksize2 = blocksize * blocksize;

    if (blknum < EXT2_NDIR_BLOCKS) {
        return ext2_read_block(inode->i_block[blknum], 1, buffer);
    }

    blknum -= EXT2_NDIR_BLOCKS;
    if (blknum < blocksize) {
        if (false == ext2_read_block(inode->i_block[EXT2_IND_BLOCK], 1, array)) return false;
        return ext2_read_block(array[blknum], 1, buffer);
    }

    blknum -= blocksize;
    if (blknum < blocksize2) {
        if (false == ext2_read_block(inode->i_block[EXT2_IND_BLOCK], 1, array)) return false;
        if (false == ext2_read_block(array[blknum / blocksize], 1, array)) return false;
        if (false == ext2_read_block(array[blknum % blocksize], 1, buffer)) return false;
    }

    blknum -= blocksize2;
    if (blknum > (blocksize * blocksize2)) return false;

    if (false == ext2_read_block(inode->i_block[EXT2_IND_BLOCK], 1, array)) return false;
    if (false == ext2_read_block(array[blknum / blocksize2], 1, array)) return false;
    if (false == ext2_read_block(array[(blknum / blocksize) % blocksize], 1, array)) return false;
    if (false == ext2_read_block(array[blknum % blocksize], 1, buffer)) return false;
    return true;
}

bool ext2_read_file_block_multiple(struct ext2_inode *inode, uint32_t blknum, int count, uint8_t *buffer)
{
    while (count--) {
        if (false == ext2_read_file_block(inode, blknum, buffer)) return false;
        blknum++;
        buffer += EXT2_BLOCK_SIZE(&sb);
    }

    return true;
}

bool ext2_lookup_inode(uint32_t inum, struct ext2_inode *buf)
{
    uint32_t group_id = (inum - 1) / sb.s_inodes_per_group;
    uint32_t group_desc = group_id / EXT2_DESC_PER_BLOCK(&sb);
    uint32_t desc = group_id % EXT2_DESC_PER_BLOCK(&sb);

    uint8_t tmp[EXT2_MAX_BLOCK_SIZE];
    if (false == ext2_read_block(sb.s_first_data_block + 1 + group_desc, 1, tmp)) return false;
    struct ext2_group_desc *gdp = (struct ext2_group_desc *) tmp;

    uint32_t ino_blk = gdp[desc].bg_inode_table +
        ((inum - 1) % sb.s_inodes_per_group) / (EXT2_BLOCK_SIZE(&sb) / sizeof(struct ext2_inode));
    
    if (false == ext2_read_block(ino_blk, 1, tmp)) return false;

    uint32_t ioff = (inum - 1) % (EXT2_BLOCK_SIZE(&sb) / sizeof(struct ext2_inode));
    struct ext2_inode *inode = (struct ext2_inode *) tmp;
    inode = inode + ioff;
    memcpy(buf, inode, sizeof(struct ext2_inode));
    return true;
}

bool ext2_lookup_path(char *path, uint32_t *inum)
{
    uint32_t cur_ino = EXT2_ROOT_INO;
    uint32_t updir_ino = cur_ino;

    while (*path == '/') path++;

    while (true) {
        if (!*path) {
            *inum = cur_ino;
            return true;
        }

        struct ext2_inode inodebuf;
        if (!ext2_lookup_inode(cur_ino, &inodebuf)) return false;
        struct ext2_inode *inode = &inodebuf;

        updir_ino = cur_ino;

        if ((inode->i_size == 0) || !S_ISDIR(inode->i_mode)) return false;

        char *subdir = path;
        while (*subdir && *subdir != '/') subdir++;
        if (*subdir == '/') *subdir++ = 0;

        uint8_t dlen = strlen(path);

        uint32_t loc = 0;
        uint32_t curr_blk = (~0UL);
        uint8_t dtmp[EXT2_MAX_BLOCK_SIZE];
        struct ext2_dir_entry_2 *dent = NULL;

        while (true) {
            if (loc >= inode->i_size) return false;

            uint32_t blk = loc >> EXT2_BLOCK_SIZE_BITS(&sb);

            if (blk != curr_blk) {
                if (false == ext2_read_file_block(inode, blk, dtmp)) return false;
                curr_blk = blk;
            }

            dent = (struct ext2_dir_entry_2 *) (dtmp + loc);
            loc += dent->rec_len;

            if (!dent->inode) {
                if (dent->rec_len == 0) return false;
                continue;
            }

            if (dent->name_len != dlen) continue;
            if (memcmp(path, dent->name, dlen)) continue;

            cur_ino = dent->inode; 
            path = subdir;
            break;
        }
    }

    return false;
}

