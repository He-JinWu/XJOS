#include <fs/fs.h>
#include <fs/stat.h>
#include <xjos/syscall.h>
#include <libc/assert.h>
#include <xjos/debug.h>
#include <fs/buffer.h>
#include <libc/string.h>
#include <xjos/stdlib.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)


#define INODE_NR 64

static inode_t inode_table[INODE_NR];

// apply inode
static inode_t *get_free_inode() {
    for (size_t i = 0; i < INODE_NR; i++) {
        inode_t *inode = &inode_table[i];
        if (inode->dev == EOF)
            return inode;
    }

    panic("no free inode");
}


// release inode
static void put_free_inode(inode_t *inode) {
    assert(inode != inode_table);
    assert(inode->count == 0);
    inode->dev = EOF;
}


// get root inode
inode_t *get_root_inode() {
    return inode_table;
}


// inode nr -> block
static inline idx_t inode_block(super_block_t *sb, idx_t nr) {
    /*
        one block(1024 byte) / inode_desc_t(32 byte) = 32 inode_desc_t
        inode 0~31 -> block 1 (inode table)
        inode 32~63 -> block 2
    */
    return 2 + sb->desc->imap_blocks + sb->desc->zmap_blocks + (nr - 1) / BLOCK_INODES;
}


// find inode by nr
static inode_t *find_inode(dev_t dev, idx_t nr) {
    super_block_t *sb = get_super(dev);
    assert(sb);
    // inode list
    list_t *list = &sb->inode_list;

    inode_t *inode;

    list_for_each_entry(inode, list, node) {
        if (inode->nr == nr) {
            return inode;
        }
    }

    return NULL;
}


// get dev - nr inode
inode_t *iget(dev_t dev, idx_t nr) {
    // find cached inode
    inode_t *inode = find_inode(dev, nr);
    if (inode) {
        inode->count++;
        inode->atime = time();

        return inode;
    }

    // miss
    super_block_t *sb = get_super(dev);
    assert(sb);

    assert(nr <= sb->desc->inodes);

    inode = get_free_inode();
    inode->dev = dev;
    inode->nr = nr;
    inode->count = 1;

    // add super block inode list
    list_push(&sb->inode_list, &inode->node);

    idx_t block = inode_block(sb, nr);

    buffer_t *buf = bread(dev, block);
    inode->buf = buf;   // Record buffer pointer
    
    // point to inode descriptor in buffer
    inode->desc = &((inode_desc_t *)buf->data)[(inode->nr - 1) % BLOCK_INODES];

    inode->ctime = inode->desc->mtime;
    inode->atime = time();

    return inode;
}


// free inode
void iput(inode_t *inode) {
    if (!inode)
        return;

    if (inode->buf->dirty)
        bwrite(inode->buf);

    inode->count--;
    if (inode->count)
        return;
    
    // count == 0

    brelse(inode->buf);
    
    list_remove(&inode->node);

    put_free_inode(inode);
}


void inode_init() {
    for (size_t i = 0; i < INODE_NR; i++) {
        inode_t *inode = &inode_table[i];
        inode->dev = EOF;
    }
}


int inode_read(inode_t *inode, char *buf, u32 len, off_t offset) {
    assert(ISFILE(inode->desc->mode) || ISDIR(inode->desc->mode));

    if (offset >= inode->desc->size)
        return EOF;
    
    u32 begin = offset;     // 记录初始偏移
    
    // file size limit
    u32 left = MIN(len, inode->desc->size - offset);

    while (left) {
        // offset / block size 算出逻辑块号
        // nr 获取物理块号
        idx_t nr = bmap(inode, offset / BLOCK_SIZE, false);
        assert(nr);

        //[IO]
        buffer_t *bf = bread(inode->dev, nr);

        // exp. offset = 1500, BLOCK_SIZE = 1024 start = 1500 % 1024 = 476
        u32 start = offset % BLOCK_SIZE; // 块内偏移

        u32 chars = MIN(BLOCK_SIZE - start, left);

        // update
        offset += chars;    // file offset++
        left -= chars;      // left--

        char *ptr = bf->data + start;
        memcpy(buf, ptr, chars);

        buf += chars;

        brelse(bf);
    }

    inode->atime = time();  // update access time
    return offset - begin;  // 实际读取字节数
}


int inode_write(inode_t *inode, char *buf, u32 len, off_t offset) {
    assert(ISFILE(inode->desc->mode));

    u32 begin = offset;     // 记录初始偏移
    u32 left = len;

    while (left) {
        idx_t nr = bmap(inode, offset / BLOCK_SIZE, true);
        assert(nr);

        // [RMW] 先读后写
        buffer_t *bf = bread(inode->dev, nr);
        bf->dirty = true;

        u32 start = offset % BLOCK_SIZE; // 块内偏移
        char *ptr = bf->data + start;
        u32 chars = MIN(BLOCK_SIZE - start, left);

        offset += chars;    // file offset++
        left -= chars;      // left--

        // [Expansion]
        if (offset > inode->desc->size) {
            inode->desc->size = offset;
            inode->buf->dirty = true;
        }

        memcpy(ptr, buf, chars);

        buf += chars;

        brelse(bf);
    }

    inode->desc->mtime = inode->atime = time(); // update modify & access time

    bwrite(inode->buf);

    return offset - begin;  // 实际写入字节数
}