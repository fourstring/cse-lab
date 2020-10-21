// inode layer interface.

#ifndef inode_h
#define inode_h

#include <stdint.h>
#include <unordered_set>
#include "extent_protocol.h" // TODO: delete it
#include <memory>
#include <vector>
#include <mutex>
#include "concurrent_queue.h"

#define DISK_SIZE  1024*1024*16
#define BLOCK_SIZE 512
#define BLOCK_NUM  (DISK_SIZE/BLOCK_SIZE)

typedef uint32_t blockid_t;

// disk layer -----------------------------------------

class disk {
private:
    unsigned char blocks[BLOCK_NUM][BLOCK_SIZE];

public:
    disk();

    void read_block(uint32_t id, char *buf);

    void write_block(uint32_t id, const char *buf);
};

// block layer -----------------------------------------

typedef struct superblock {
    uint32_t size;
    uint32_t block_size;
    uint32_t nblocks;
    uint32_t ninodes;
} superblock_t;

// Bitmap bits per block
#define BPB           (BLOCK_SIZE*8)
const auto BLOCK_FREE = false, BLOCK_USED = true;
const auto CHANGE_THRESHOLD = 100;
const auto NBITMAP_BLOCKS = (BLOCK_NUM + BPB - 1) / BPB;
const auto NINDIRECT_ENTS = BLOCK_SIZE / sizeof(blockid_t);
using bitmap_t = std::map<blockid_t, bool>;
using free_set_t = std::unordered_set<blockid_t>;
using block_buf_t = std::unique_ptr<char[]>;

class block_manager {
private:
    disk *d;
    bitmap_t blocks_bitmap;
    free_set_t free_blocks;
    uint32_t change_counters = 0;
    std::mutex block_mutex{};

    void format_blocks();

    void read_superblock();

    void read_bitmap();

    void sync_superblock(const superblock &superblock);

    void sync_bitmap(const bitmap_t &bitmap);

    void clear_block(blockid_t id);

public:
    block_manager();

    struct superblock sb{};

    uint32_t alloc_block();

    void free_block(uint32_t id);

    void read_block(uint32_t id, char *buf);

    void write_block(uint32_t id, const char *buf);

    static block_buf_t new_block_buf();

    static constexpr size_t get_blocks_count(uint32_t bytes);
};

// inode layer -----------------------------------------

#define INODE_NUM  1024

// Inodes per block.
#define IPB           1
//(BLOCK_SIZE / sizeof(struct inode))

// Block containing inode i
#define IBLOCK(i, nblocks)     ((nblocks)/BPB + (i)/IPB + 3)

// Block containing bit for block b
#define BBLOCK(b) ((b)/BPB + 2)

#define NDIRECT 100
#define NINDIRECT (BLOCK_SIZE / sizeof(blockid_t))
#define MAXINODE_BLOCKS (NDIRECT + 1)
#define MAXFILE_BLOCKS (NDIRECT + NINDIRECT)
#define MAXFILE_SIZE (MAXFILE_BLOCKS * BLOCK_SIZE)

typedef struct inode {
    short type; // 0 means not used
    unsigned int size;
    unsigned int ref_count;
    unsigned int atime;
    unsigned int mtime;
    unsigned int ctime;
    blockid_t blocks[NDIRECT + 1];   // Data block addresses
} inode_t;

class inode_manager {
private:
    using blocks_level_t = std::pair<blockid_t, uint32_t>;
    std::mutex inode_mutex{};

    block_manager *bm;

    void format_inodes();

    blockid_t inode_add_block(uint32_t inum);

    std::vector<blockid_t> inode_add_blocks(uint32_t inum, uint32_t count);

    std::vector<blockid_t> inode_get_datablocks(uint32_t inum);

    void inode_free_blocks(uint32_t inum, uint32_t count);

    struct inode *get_inode(uint32_t inum);

    void put_inode(uint32_t inum, struct inode *ino);

public:
    inode_manager();

    uint32_t alloc_inode(uint32_t type);

    void free_inode(uint32_t inum);

    void read_file(uint32_t inum, char **buf, int *size);

    void write_file(uint32_t inum, const char *buf, int size);

    void remove_file(uint32_t inum);

    void getattr(uint32_t inum, extent_protocol::attr &a);

    void increase_ref(uint32_t inum);

    void decrease_ref(uint32_t inum);
};

class file_too_large_exception : public std::exception {
};

#endif

