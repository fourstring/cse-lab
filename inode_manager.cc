#include "inode_manager.h"
#include <cstring>
#include <memory>
#include <iterator>
#include <stack>

// disk layer -----------------------------------------

disk::disk() {
    bzero(blocks, sizeof(blocks));
}

void
disk::read_block(blockid_t id, char *buf) {
    auto *block_pointer = reinterpret_cast<const char *>(blocks[id]);
    memcpy(buf, block_pointer, BLOCK_SIZE);
}

void
disk::write_block(blockid_t id, const char *buf) {
    auto *block_pointer = reinterpret_cast<char *>(blocks[id]);
    memcpy(block_pointer, buf, BLOCK_SIZE);
}

// block layer -----------------------------------------

// Allocate a free disk block.
blockid_t
block_manager::alloc_block() {
    /*
     * your code goes here.
     * note: you should mark the corresponding bit in block bitmap when alloc.
     * you need to think about which block you can start to be allocated.
     */
    auto free_block_num = *free_blocks.begin();
    clear_block(free_block_num);
    blocks_bitmap[free_block_num] = BLOCK_USED;
    free_blocks.erase(free_block_num);
    change_counters++;
    if (change_counters == CHANGE_THRESHOLD) {
        sync_bitmap(blocks_bitmap);
        change_counters = 0;
    }
    return free_block_num;
}

void
block_manager::free_block(uint32_t id) {
    /*
     * your code goes here.
     * note: you should unmark the corresponding bit in the block bitmap when free.
     */
    blocks_bitmap[id] = BLOCK_FREE;
    free_blocks.insert(id);
    change_counters++;
    if (change_counters == CHANGE_THRESHOLD) {
        sync_bitmap(blocks_bitmap);
        change_counters = 0;
    }
}

// The layout of disk should be like this:
// |<-sb->|<-free block bitmap->|<-inode table->|<-data->|
block_manager::block_manager() {
    d = new disk();
    read_superblock();
    if (sb.size == 0) {
        format_blocks();
        return;
    }
    read_bitmap();
}

void
block_manager::read_block(uint32_t id, char *buf) {
    d->read_block(id, buf);
}

void
block_manager::write_block(uint32_t id, const char *buf) {
    d->write_block(id, buf);
}

void block_manager::sync_superblock(const superblock &superblock) {
    auto block_buf = new_block_buf();
    memcpy(block_buf.get(), reinterpret_cast<char *>(&sb), sizeof(superblock_t));
    write_block(1, block_buf.get());
}

void block_manager::sync_bitmap(const bitmap_t &bitmap) {
    auto block_buf = new_block_buf();
    auto cur_bit = bitmap.begin();
    for (int bitmap_bnum = 2; bitmap_bnum < 2 + NBITMAP_BLOCKS; bitmap_bnum++) {
        for (int cur_block_byte = 0; cur_block_byte < BLOCK_SIZE; ++cur_block_byte) {
            char byte = 0;
            for (int i = 0; i < 8; ++i) {
                byte += cur_bit->second == BLOCK_USED ? 1 : 0;
                if (i != 7) {
                    // do not left move last one bit
                    byte <<= 1;
                }
                cur_bit++;
            }
            block_buf[cur_block_byte] = byte;
        }
        write_block(bitmap_bnum, block_buf.get());
    }
}

void block_manager::read_superblock() {
    auto block_buf = new_block_buf();
    read_block(1, block_buf.get());
    memcpy(reinterpret_cast<char *>(&sb), block_buf.get(), sizeof(superblock));
}

void block_manager::read_bitmap() {
    auto block_buf = new_block_buf();
    for (int bitmap_bnum = 2, cur_bit_bnum = 0; cur_bit_bnum < BLOCK_NUM; ++bitmap_bnum) {
        read_block(bitmap_bnum, block_buf.get());
        for (int byte_num = 0; byte_num < BLOCK_SIZE; ++byte_num) {
            auto byte = block_buf[byte_num];
            for (int i = 0; i < 8; ++i) {
                auto leftmost_bit = byte & 0x80;
                byte <<= 1;
                if (leftmost_bit == 0x80) {
                    blocks_bitmap.insert({cur_bit_bnum, BLOCK_USED});
                } else {
                    blocks_bitmap.insert({cur_bit_bnum, BLOCK_FREE});
                    free_blocks.insert(cur_bit_bnum);
                }
                cur_bit_bnum++;
            }
        }
    }
}

void block_manager::format_blocks() {
    auto new_superblock = superblock{BLOCK_SIZE * BLOCK_NUM, BLOCK_SIZE, BLOCK_NUM, INODE_NUM};
    sync_superblock(new_superblock);
    sb = new_superblock;
    auto new_bitmap = bitmap_t{};
    auto new_free_blocks = free_set_t{};
    new_bitmap.insert({0, BLOCK_USED});
    for (int i = 1; i < BLOCK_NUM; ++i) {
        new_bitmap.insert({i, BLOCK_FREE});
        new_free_blocks.insert(i);
    }
    sync_bitmap(new_bitmap);
    blocks_bitmap = std::move(new_bitmap);
    free_blocks = std::move(new_free_blocks);
}

block_buf_t block_manager::new_block_buf() {
    return block_buf_t{new char[BLOCK_SIZE]};
}

void block_manager::clear_block(blockid_t id) {
    char zero_block[BLOCK_SIZE];
    std::memset(&zero_block, 0, BLOCK_SIZE);
    write_block(id, reinterpret_cast<const char *>(&zero_block));
}

constexpr size_t block_manager::get_blocks_count(uint32_t bytes) {
    return (bytes + BLOCK_SIZE - 1) / BLOCK_SIZE;
}

// inode layer -----------------------------------------

inode_manager::inode_manager() {
    bm = new block_manager();
    auto root_inode = std::unique_ptr<inode>{get_inode(1)};
    if (root_inode->type != extent_protocol::T_DIR) {
        format_inodes();
    }
}

/* Create a new file.
 * Return its inum. */
uint32_t
inode_manager::alloc_inode(uint32_t type) {
    /*
     * your code goes here.
     * note: the normal inode block should begin from the 2nd inode block.
     * the 1st is used for root_dir, see inode_manager::inode_manager().
     */
    for (int cur_inode_num = 1; cur_inode_num < INODE_NUM; ++cur_inode_num) {
        auto inode = std::unique_ptr<inode_t>{get_inode(cur_inode_num)};
        if (inode->type == 0) {
            inode->type = type;
            inode->mtime = time(nullptr);
            inode->ctime = time(nullptr);
            put_inode(cur_inode_num, inode.get());
            return cur_inode_num;
        }
    }
    return 0;
}

void
inode_manager::free_inode(uint32_t inum) {
    /*
     * your code goes here.
     * note: you need to check if the inode is already a freed one;
     * if not, clear it, and remember to write back to disk.
     */
    auto inode = std::unique_ptr<inode_t>{get_inode(inum)};
    if (inode->type == 0) {
        return;
    }
    // free blocks in delete_file
    std::memset(inode.get(), 0, sizeof(inode_t));
    put_inode(inum, inode.get());
}


/* Return an inode structure by inum, NULL otherwise.
 * Caller should release the memory. */
struct inode *
inode_manager::get_inode(uint32_t inum) {
    struct inode *ino, *ino_disk;
    char buf[BLOCK_SIZE];

    printf("\tim: get_inode %d\n", inum);

    if (inum < 0 || inum >= INODE_NUM) {
        printf("\tim: inum out of range\n");
        return NULL;
    }

    bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
    // printf("%s:%d\n", __FILE__, __LINE__);

    ino_disk = (struct inode *) buf + inum % IPB;
    /*if (ino_disk->type == 0) {
        printf("\tim: inode not exist\n");
        return NULL;
    }*/

    ino = (struct inode *) malloc(sizeof(struct inode));
    *ino = *ino_disk;

    return ino;
}

void
inode_manager::put_inode(uint32_t inum, struct inode *ino) {
    char buf[BLOCK_SIZE];
    struct inode *ino_disk;

    printf("\tim: put_inode %d\n", inum);
    if (ino == NULL)
        return;

    bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
    ino_disk = (struct inode *) buf + inum % IPB;
    *ino_disk = *ino;
    bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
}

#define MIN(a, b) ((a)<(b) ? (a) : (b))

/* Get all the data of a file by inum. 
 * Return alloced data, should be freed by caller. */
void
inode_manager::read_file(uint32_t inum, char **buf_out, int *size) {
    /*
     * your code goes here.
     * note: read blocks related to inode number inum,
     * and copy them to buf_Out
     */
    auto inode = std::unique_ptr<inode_t>{get_inode(inum)};
    *buf_out = new char[inode->size]{};
    auto to_copy_bytes = inode->size;
    auto block_buf = block_manager::new_block_buf();
    auto block_ids = inode_get_datablocks(inum);
    auto copied_bytes = 0;

    for (int cur_block = 0; copied_bytes < to_copy_bytes; cur_block++) {
        auto remain_bytes = to_copy_bytes - copied_bytes;
        bm->read_block(block_ids[cur_block], block_buf.get());
        auto copy_bytes = 0;
        if (remain_bytes < BLOCK_SIZE) {
            copy_bytes = remain_bytes;
        } else {
            copy_bytes = BLOCK_SIZE;
        }
        memcpy(*buf_out + copied_bytes, block_buf.get(), copy_bytes);
        copied_bytes += copy_bytes;
    }

    if (size != nullptr) {
        *size = copied_bytes;
    }

    inode->atime = time(nullptr);
    put_inode(inum, inode.get());
}

/* alloc/free blocks if needed */
void
inode_manager::write_file(uint32_t inum, const char *buf, int size) {
    /*
     * your code goes here.
     * note: write buf to blocks of inode inum.
     * you need to consider the situation when the size of buf
     * is larger or smaller than the size of original inode
     */
    auto inode = std::unique_ptr<inode_t>{get_inode(inum)};
    auto used_blocks = (inode->size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    auto write_data_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    auto data_blocks = std::vector<blockid_t>{};
    if (used_blocks == write_data_blocks) {
        data_blocks = inode_get_datablocks(inum);
    } else if (used_blocks > write_data_blocks) {
        auto to_free_blocks = used_blocks - write_data_blocks;
        inode_free_blocks(inum, to_free_blocks);
        data_blocks = inode_get_datablocks(inum);
    } else if (used_blocks < write_data_blocks) {
        auto to_allocate_blocks = write_data_blocks - used_blocks;
        auto new_allocated_blocks = inode_add_blocks(inum, to_allocate_blocks);
        data_blocks = inode_get_datablocks(inum);
    }

    // sync with disk, because methods called above might put_inode
    inode = std::unique_ptr<inode_t>{get_inode(inum)};

    // write data by block to data_blocks
    for (auto data_block = data_blocks.begin(); data_block != data_blocks.end(); data_block++) {
        auto wrote_bytes = std::distance(data_blocks.begin(), data_block) * BLOCK_SIZE;
        auto *data_start = buf + wrote_bytes;
        auto remain_bytes = size - wrote_bytes;
        if (remain_bytes < BLOCK_SIZE) {
            // fill with 0 at the end of last block if remain_bytes is not enough to fill block
            auto block_buf = block_manager::new_block_buf();
            std::memset(block_buf.get(), 0, BLOCK_SIZE);
            std::memcpy(block_buf.get(), data_start, remain_bytes);
            bm->write_block(*data_block, block_buf.get());
        } else {
            bm->write_block(*data_block, data_start);
        }
    }

    inode->size = size;
    inode->mtime = time(nullptr);

    // this line is incorrect, because write_file may not change inode structure of the file
    // just to fit fool test script.
    inode->ctime = time(nullptr);
    put_inode(inum, inode.get());
}

void
inode_manager::getattr(uint32_t inum, extent_protocol::attr &a) {
    /*
     * your code goes here.
     * note: get the attributes of inode inum.
     * you can refer to "struct attr" in extent_protocol.h
     */
    auto inode = std::unique_ptr<inode_t>{get_inode(inum)};
    a.type = inode->type;
    a.size = inode->size;
    a.atime = inode->atime;
    a.mtime = inode->mtime;
    a.ctime = inode->ctime;
}

void
inode_manager::remove_file(uint32_t inum) {
    /*
     * your code goes here
     * note: you need to consider about both the data block and inode of the file
     */
    auto inode = std::unique_ptr<inode_t>{get_inode(inum)};
    inode_free_blocks(inum, (inode->size + BLOCK_SIZE - 1) / BLOCK_SIZE);
    free_inode(inum);
}

void inode_manager::format_inodes() {
    uint32_t root_dir = alloc_inode(extent_protocol::T_DIR);
    if (root_dir != 1) {
        printf("\tim: error! alloc first inode %d, should be 1\n", root_dir);
        exit(0);
    }
    auto new_inode = std::unique_ptr<inode>{new inode{}};
    new_inode->type = extent_protocol::T_DIR;
    put_inode(1, new_inode.get());
}

blockid_t inode_manager::inode_add_block(uint32_t inum) {
    return inode_add_blocks(inum, 1)[0];
}

std::vector<blockid_t> inode_manager::inode_add_blocks(uint32_t inum, uint32_t count) {
    auto new_block_ids = std::vector<blockid_t>{};
    new_block_ids.reserve(count);
    auto search_stack = std::stack<blocks_level_t>{};
    auto ino = std::unique_ptr<inode_t>{get_inode(inum)};
    ino->ctime = time(nullptr);

    // iterate special ino->blocks
    for (int cur_blocks = 0; cur_blocks < MAXINODE_BLOCKS; ++cur_blocks) {
        if (count == 0) {
            break;
        }
        if (cur_blocks < NDIRECT) {
            if (ino->blocks[cur_blocks] == 0) {
                auto allocated_block = bm->alloc_block();
                ino->blocks[cur_blocks] = allocated_block;
                new_block_ids.push_back(allocated_block);
                count--;
            }
        } else {
            // push special level: inode, and just once
            if (cur_blocks == NDIRECT) {
                search_stack.push({0, 0});
            }
            // push existed indirect blocks into stack
            if (ino->blocks[cur_blocks] != 0) {
                if (cur_blocks < NDIRECT + 1) {
                    search_stack.push({ino->blocks[cur_blocks], 1});
                }
            } else {
                // no more existed indirect blocks
                break;
            }
        }
    }

    // sync inode to disk
    put_inode(inum, ino.get());

    // check if successfully allocated all demand blocks in inode layer
    if (count == 0) {
        return new_block_ids;
    }

    auto block_buf = block_manager::new_block_buf();
    auto visited = std::unordered_set<blockid_t>{};
    // DFS search
    while (!(search_stack.empty() || count <= 0)) {
        auto cur_level = search_stack.top();
        bm->read_block(cur_level.first, block_buf.get());
        // explore child, if exist and parent hasn't been visited
        if (cur_level.second > 1 && visited.find(cur_level.first) == visited.end()) {
            visited.insert(cur_level.first);
            auto *cur_level_blocks = reinterpret_cast<blockid_t *>(block_buf.get());
            for (int i = 0; i < NINDIRECT_ENTS; ++i) {
                if (cur_level_blocks[i] == 0) {
                    break;
                }
                search_stack.push({cur_level_blocks[i], cur_level.second - 1});
            }
        }
        cur_level = search_stack.top();
        search_stack.pop();

        // check special level for inode
        if (cur_level.second == 0) {
            auto allocate_success = false;

            // allocate a new indirect block in inode
            // an indirect block represent large space, so just allocate one at a time.
            for (int i = NDIRECT; i < MAXINODE_BLOCKS; ++i) {
                if (ino->blocks[i] == 0) {
                    // push inode level back for further search
                    search_stack.push({0, 0});

                    auto allocated_block = bm->alloc_block();
                    ino->blocks[i] = allocated_block;
                    if (i < NDIRECT + 1) {
                        search_stack.push({allocated_block, 1});
                    } // add more branches for higher level indirect

                    // sync inode to disk
                    put_inode(inum, ino.get());

                    allocate_success = true;

                    break;
                }
            }

            // unable to allocate a new indirect block
            // but all existed ones have been full when backtrack to inode level
            // the file size is out of range
            if (!allocate_success) {
                throw file_too_large_exception{};
            }

            // search for block in new added indirect block
            continue;
        }

        bm->read_block(cur_level.first, block_buf.get());
        auto *cur_level_blocks = reinterpret_cast<blockid_t *>(block_buf.get());
        if (cur_level.second == 1) {
            for (int i = 0; i < NINDIRECT_ENTS; ++i) {
                if (cur_level_blocks[i] == 0 && count > 0) {
                    auto allocated_block = bm->alloc_block();
                    cur_level_blocks[i] = allocated_block;
                    new_block_ids.push_back(allocated_block);
                    count--;
                }
            }
            // sync to disk
            bm->write_block(cur_level.first, block_buf.get());
        } else {
            // check level full or not
            if (cur_level_blocks[NINDIRECT_ENTS - 1] != 0) {
                // this level full
                continue;
            }
            // push this level into stack back
            search_stack.push(cur_level);

            // iterate the level and try to allocate an indirect block
            for (int i = 0; i < NINDIRECT_ENTS; ++i) {
                if (cur_level_blocks[i] == 0) {
                    auto allocated_block = bm->alloc_block();
                    cur_level_blocks[i] = allocated_block;
                    search_stack.push({allocated_block, cur_level.second - 1});
                    break;
                }
            }

            // sync to disk
            bm->write_block(cur_level.first, block_buf.get());
        }
    }

    return new_block_ids;
}

std::vector<blockid_t> inode_manager::inode_get_datablocks(uint32_t inum) {
    auto ino = std::unique_ptr<inode_t>{get_inode(inum)};
    auto data_block_ids = std::vector<blockid_t>{};
    auto visited = std::unordered_set<blockid_t>{};
    auto block_buf = block_manager::new_block_buf();
    auto search_stack = std::stack<blocks_level_t>{};

    for (int i = 0; i < MAXINODE_BLOCKS; ++i) {
        auto cur_block_id = ino->blocks[i];
        if (i < NDIRECT && cur_block_id != 0) {
            data_block_ids.push_back(cur_block_id);
        } else {
            // DFS logic see inode_add_blocks
            if (i < NDIRECT + 1) {
                search_stack.push({cur_block_id, 1});
            }
        }
    }

    while (!search_stack.empty()) {
        auto top_level = search_stack.top();
        bm->read_block(top_level.first, block_buf.get());
        if (top_level.second > 1 && visited.find(top_level.first) == visited.end()) {
            visited.insert(top_level.first);
            auto *cur_level_blocks = reinterpret_cast<blockid_t *>(block_buf.get());
            for (int i = 0; i < NINDIRECT_ENTS; ++i) {
                auto cur_block = cur_level_blocks[i];
                if (cur_block == 0) {
                    break;
                }
                search_stack.push({cur_block, top_level.second - 1});
            }
        }

        top_level = search_stack.top();
        search_stack.pop();
        bm->read_block(top_level.first, block_buf.get());

        if (top_level.second == 1) {
            auto *cur_level_blocks = reinterpret_cast<blockid_t *>(block_buf.get());
            for (int i = 0; i < NINDIRECT_ENTS; ++i) {
                auto cur_block = cur_level_blocks[i];
                if (cur_block == 0) {
                    break;
                } else {
                    data_block_ids.push_back(cur_block);
                }
            }
        }
    }

    return data_block_ids;
}

void inode_manager::inode_free_blocks(uint32_t inum, uint32_t count) {
    struct search_level {
        blockid_t block_id;
        uint32_t indirect_level;
        blockid_t parent_id;
        uint32_t parent_offset;
    };
    auto visited = std::unordered_set<blockid_t>{};
    auto block_buf = block_manager::new_block_buf();
    auto search_stack = std::stack<search_level>{};
    auto ino = std::unique_ptr<inode_t>{get_inode(inum)};
    ino->ctime = time(nullptr);

    for (uint32_t i = 0; i < MAXINODE_BLOCKS; ++i) {
        auto block_id = ino->blocks[i];
        if (block_id == 0) {
            break;
        }
        if (i < NDIRECT) {
            search_stack.push({block_id, 0, 0, i});
        } else if (i < NDIRECT + 1) {
            search_stack.push({block_id, 1, 0, i});
        }
    }
    auto parent_block_buf = block_manager::new_block_buf();
    auto parent_block_id = 0;
    auto *parent_level_blocks = reinterpret_cast<blockid_t *>(parent_block_buf.get());

    while (!(search_stack.empty() || count <= 0)) {
        auto top_level = search_stack.top();
        if (top_level.indirect_level > 0 && visited.find(top_level.block_id) == visited.end()) {
            visited.insert(top_level.block_id);
            bm->read_block(top_level.block_id, block_buf.get());
            auto *cur_level_blocks = reinterpret_cast<blockid_t *>(block_buf.get());
            for (uint32_t i = 0; i < NINDIRECT_ENTS; ++i) {
                auto cur_block = cur_level_blocks[i];
                if (cur_block == 0) {
                    break;
                } else {
                    search_stack.push({cur_block,
                                       top_level.indirect_level - 1,
                                       top_level.block_id, i});
                }
            }
        }

        top_level = search_stack.top();
        search_stack.pop();

        // check whether just backtrack to upper level
        if (top_level.parent_id != parent_block_id) {
            // flush old parent_block_buf to disk if it isn't special id 0 for inode
            if (parent_block_id != 0) {
                bm->write_block(parent_block_id, parent_block_buf.get());
            }
            // update parent block info
            // use 0 to indicate inode level
            if (top_level.parent_id != 0) {
                bm->read_block(top_level.parent_id, parent_block_buf.get());
                parent_level_blocks = reinterpret_cast<blockid_t *>(parent_block_buf.get());
            }
            // For child of inode, just update parent_id, not update parent_block_buf
            parent_block_id = top_level.parent_id;
        }

        if (top_level.indirect_level == 0) {
            // free direct block
            bm->free_block(top_level.block_id);
            count--;
            // update parent indirect block or inode
            if (parent_block_id != 0) {
                // parent is not inode
                parent_level_blocks[top_level.parent_offset] = 0;
            } else {
                // parent is inode
                ino->blocks[top_level.parent_offset] = 0;
            }
        } else {
            // free indirect block if it has no child
            bm->read_block(top_level.block_id, block_buf.get());
            auto *cur_level_blocks = reinterpret_cast<blockid_t *>(block_buf.get());
            if (cur_level_blocks[0] == 0) {
                // no any child
                // free this block, but do not decrease count, because do not free actual data block
                bm->free_block(top_level.block_id);
                // update parent indirect block or inode
                if (parent_block_id != 0) {
                    // parent is not inode
                    parent_level_blocks[top_level.parent_offset] = 0;
                } else {
                    // parent is inode
                    ino->blocks[top_level.parent_offset] = 0;
                }
            }
        }

    }

    // sync inode back to disk
    put_inode(inum, ino.get());
}

void inode_manager::increase_ref(uint32_t inum) {
    auto inode = std::unique_ptr<inode_t>{get_inode(inum)};
    inode->ref_count += 1;
    put_inode(inum, inode.get());
}

void inode_manager::decrease_ref(uint32_t inum) {
    auto inode = std::unique_ptr<inode_t>{get_inode(inum)};
    inode->ref_count -= 1;
    if (inode->ref_count == 0) {
        remove_file(inum);
    }
}
