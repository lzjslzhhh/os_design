#ifndef MEMORY_H
#define MEMORY_H
// 虚拟地址'la'的三段结构如下:
//
// +--------10------+-------10-------+---------12----------+
// | Page Directory |   Page Table   | Offset within Page  |
// |      Index     |      Index     |                     |
// +----------------+----------------+---------------------+
//  \--- PDX(va) --/ \--- PTX(va) --/

// 页目录索引
#define PDX(va)         (((uint)(va) >> PDXSHIFT) & 0x3FF)

// 页表索引
#define PTX(va)         (((uint)(va) >> PTXSHIFT) & 0x3FF)

// 由索引和页内偏移组合成虚拟地址
#define PGADDR(d, t, o) ((uint)((d) << PDXSHIFT | (t) << PTXSHIFT | (o)))

// 页目录和页表的常量
#define NPDENTRIES      1024    // 页目录项数量
#define NPTENTRIES      1024    // 页表项数量
#define PGSIZE          4096    // 页大小,单位为字节

#define PTXSHIFT        12      // offset of PTX in a linear address
#define PDXSHIFT        22      // offset of PDX in a linear address

#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))

// Page table/directory entry flags.
#define PTE_P           0x001   // 页存在
#define PTE_W           0x002   // 页可写
#define PTE_U           0x004   // 用户可访问
#define PTE_PS          0x080   // 大页面标志

// Address in page table or page directory entry
#define PTE_ADDR(pte)   ((uint)(pte) & ~0xFFF)
#define PTE_FLAGS(pte)  ((uint)(pte) &  0xFFF)

// void init_memory(void);
// void* kalloc(void); // 分配一页
// void kfree(void* ptr); // 释放一页

#endif 