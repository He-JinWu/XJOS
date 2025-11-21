#include <fs/fs.h>
#include <fs/buffer.h>
#include <drivers/device.h>
#include <libc/assert.h>
#include <libc/string.h>
#include <xjos/debug.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

void super_init() {
    // 2048 sectors before
    device_t *device = device_find(DEV_IDE_PART, 0);
    assert(device);

    // Read the boot block and superblock from the device
    buffer_t *boot = bread(device->dev, 0);
    buffer_t *super = bread(device->dev, 1);

    super_desc_t *sb = (super_desc_t *)(super->data);
    assert(sb->magic == MINIX1_MAGIC);

    // inode map
    buffer_t *imap = bread(device->dev, 2);

    // zone map
    buffer_t *zmap = bread(device->dev, 2 + sb->imap_blocks);

    // first data  block
    buffer_t *buf1 = bread(device->dev, 2 + sb->imap_blocks + sb->zmap_blocks);
    
    // root inode
    inode_desc_t *inode = (inode_desc_t *)(buf1->data); // skip root inode

    // root logical block
    buffer_t *buf2 = bread(device->dev, inode->zones[0]);

    dentry_t *dir = (dentry_t *)(buf2->data);

    inode_desc_t *helloi = NULL;

    // traverse
    while (dir->nr) {
        LOGK("inode: %04d, name: %s\n", dir->nr, dir->name);
        
        if (!strcmp(dir->name, "hello.txt")) {
            helloi = &((inode_desc_t *)buf1->data)[dir->nr - 1];

            strcpy(dir->name, "world.txt");
            buf2->dirty = true;
            bwrite(buf2);
        }

        dir++;
    }

    buffer_t *buf3 = bread(device->dev, helloi->zones[0]);

    // buf3->data = txt file
    LOGK("content %s\n", buf3->data);

    strcpy(buf3->data, "this is a test file\n");

    buf3->dirty = true;
    bwrite(buf3);

    helloi->size = strlen(buf3->data);
    buf1->dirty = true;
    bwrite(buf1);
}