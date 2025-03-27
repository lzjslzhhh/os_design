#include "types.h"
// #include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "memory.h"
#include <stdio.h>
// #include "spinlock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>

void freerange(void *vstart, void *vend);
// extern char end[];

// 物理内存链表节点
struct run {
  struct run *next;
};

// 物理内存管理结构
struct {
    // struct spinlock lock;
    // int use_lock;
    struct run *freelist;
} kmem;

// 模拟 `end`，假设内核占用了前 1MB
char *end;

// **初始化内核物理内存**
void 
kinit(void *vstart, void *vend)
{
    // initlock(&kmem.lock, "kmem");
    // kmem.use_lock = 1;
    freerange(vstart, vend);
}

// **释放一段物理内存**
void
freerange(void *vstart, void *vend)
{
  char *p;
  p = (char*)PGROUNDUP((uint)vstart);
  printf("freerange: freeing memory from %p to %p\n", p, vend);
  for(; p + PGSIZE <= (char*)PGROUNDDOWN((uint)vend); p += PGSIZE){
    printf("kfree: freeing %p\n", p);
    kfree(p);
  }
    
}

// **释放单个页面**
void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP){
    printf("kfree: error at %p\n", v);
    // panic("kfree");
  }

  printf("kfree: freeing %p\n", v);
  // Fill with junk to catch dangling refs.
  memset(v, 0xCC, PGSIZE);

//   if(kmem.use_lock)
//     acquire(&kmem.lock);
  r = (struct run*)v;
  r->next = kmem.freelist;
  kmem.freelist = r;
//   if(kmem.use_lock)
//     release(&kmem.lock);
}

// **分配单个页面**
char*
kalloc(void)
{
  struct run *r;

//   if(kmem.use_lock)
//     acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
//   if(kmem.use_lock)
//     release(&kmem.lock);
  return (char*)r;
}

int main(){
    size_t memsize = 4*PGSIZE;
    char *fake_mem = (char *)mmap((void*)0x10000000, memsize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);


    if (!fake_mem) {
        printf("分配内存失败!\n");
        return 1;
    }

    end = fake_mem;  // `end` 作为可用内存的开始

    kinit(end, end + memsize);

    char *p1= kalloc();
    char *p2= kalloc();
    char *p3= kalloc();

    // **释放 1 页**
    kfree(p2);

    // **再次分配**
    char *p4 = kalloc();

    // 释放映射
    munmap(fake_mem, memsize);
    return 0;

}