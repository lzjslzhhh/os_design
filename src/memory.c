// #include "memory.h"
// #include "shell.h"
// #include <stdint.h>
// static uint8_t mem_bitmap[TOTAL_PAGES / 8]; // 位图管理页
// static uint8_t *mem_start; // 内存起始地址
// void init_memory(void)
// {
//     console_write("start initializing memory.\n");
//     mem_start = (uint8_t*) 0x80000000; // 假设内存起始地址
//     for (int i = 0; i < PAGE_COUNT / 8; i++) {
//         page_bitmap[i] = 0;  // 所有页标记为空闲
//     }
//     console_write("finish initializing memory.\n");
// }

// // 分配一页
// void* kalloc() {
//     for (int i = 0; i < TOTAL_PAGES; i++) {
//         int byte = i / 8, bit = i % 8;
//         if (mem_bitmap[byte] & (1 << bit)) {
//             mem_bitmap[byte] &= ~(1 << bit); // 标记为已分配
//             return mem_start + i * PAGE_SIZE;
//         }
//     }
//     return 0; // 没有可用内存
// }

// // 释放一页
// void kfree(void *page) {
//     uint32_t index = ((uint8_t*) page - mem_start) / PAGE_SIZE;
//     mem_bitmap[index / 8] |= (1 << (index % 8));
// }