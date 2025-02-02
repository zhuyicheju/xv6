// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13


// struct buf* hash[KEY_NUM];



struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;

struct {
  struct spinlock lock;
  struct buf* buf;

}table[NBUCKET];

int hash(uint dev, uint blockno){
  return blockno % NBUCKET;
}

void
binit(void)
{
  initlock(&bcache.lock, "bcache");
  for(int i = 0; i < NBUCKET; i ++){
    initlock(&table[i].lock, "bcache");
  }
  
  struct buf* b = bcache.buf;
  for(int i = 0; i < NBUF; i ++){
    initsleeplock(&b->lock, "buffer");
    b->blockno = i;
    int idx = hash(0, i);
    b->next = table[idx].buf;
    table[idx].buf = b;

    b ++;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int idx = hash(dev, blockno);
  acquire(&table[idx].lock);
  b = table[idx].buf;

  for( ; b != 0; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt ++;
      release(&table[idx].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  int min_time = 0x8fffffff;
  struct buf* replace_buf = 0;
  for(b = table[idx].buf; b != 0; b = b -> next){
    if(b->refcnt == 0 && b->timestamp < min_time){
      replace_buf = b;
      min_time = b->timestamp;
    }
  }

  if(replace_buf){
    replace_buf->valid = 0;
    replace_buf->refcnt = 1;
    replace_buf->dev = dev;
    replace_buf->blockno = blockno;
    release(&table[idx].lock);
    acquiresleep(&replace_buf->lock);
    return replace_buf;
  }
  release(&table[idx].lock);
  
refind:
  int lock_num = 0x3f;
  for(int i = 0; i < NBUCKET; i ++){
    acquire(&table[i].lock);
    b = table[i].buf;
    for(; b != 0; b = b->next){
      if(b->refcnt == 0 && b->timestamp < min_time){
        // if(lock_num != 0x3f)
        //   release(&table[lock_num].lock);
        min_time = b->timestamp;
        replace_buf = b;
        lock_num = i;
      }
    }
    // if(lock_num == 0x3f)
    release(&table[i].lock);
  }

  if(replace_buf){
    acquire(&table[lock_num].lock);
    if(replace_buf->refcnt != 0) {
      release(&table[lock_num].lock);
      goto refind;
    }
    if(table[lock_num].buf == replace_buf) 
      table[lock_num].buf = replace_buf->next;
    else{
      for(b = table[lock_num].buf; b->next != replace_buf;b = b->next);
      b->next = replace_buf->next;
    }
    release(&table[lock_num].lock);
    //remove buf from origin space

    replace_buf->dev = dev;
    replace_buf->blockno = blockno;
    replace_buf->refcnt = 1;
    replace_buf->valid = 0;
    
    acquire(&table[idx].lock);

    replace_buf->next = table[idx].buf; 
    table[idx].buf = replace_buf;

    release(&table[idx].lock);

    acquiresleep(&replace_buf->lock);
    return replace_buf;
  }

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int idx = hash(b->dev, b->blockno);
  acquire(&table[idx].lock);

  b->refcnt -= 1;
  if(b->refcnt == 0){
    b->timestamp = ticks;
  }
  release(&table[idx].lock);
  // acquire(&bcache.lock);
  // b->refcnt--;
  // if (b->refcnt == 0) {
  //   // no one is waiting for it.
  //   b->next->prev = b->prev;
  //   b->prev->next = b->next;
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }
  
  // release(&bcache.lock);
}

void
bpin(struct buf *b) {
  int idx = hash(b->dev, b->blockno);
  acquire(&table[idx].lock);
  b->refcnt++;
  release(&table[idx].lock);
}

void
bunpin(struct buf *b) {
  int idx = hash(b->dev, b->blockno);
  acquire(&table[idx].lock);
  b->refcnt--;
  release(&table[idx].lock);
}


