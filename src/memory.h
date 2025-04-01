#ifndef MEMORY_H
#define MEMORY_H

#include <stdbool.h>

// 前向声明
typedef struct pcb PCB;


// 常量定义
#define PAGE_SIZE 1024                  // 页面大小
#define PHYSICAL_MEMORY_SIZE (1<<22)    // 物理内存大小
#define LOGICAL_MEMORY_SIZE (1<<28)     // 逻辑内存大小
#define NO_PHYSICAL_PAGE -2             // 当前页表项没有映射到物理页
#define INVALID 0                       // 当前页表项无效
#define NOT_MODIFIED 0                  // 当前页表项对应的内存没有被修改
#define RESIDENT_SET_SIZE 4             // 驻留集大小（假设值，需与实际实现一致）

//物理地址管理
// 使用链表这一数据结构来管理物理内存，并且直接以页为单位来进行管理
typedef struct PhysicalPage {
    int id;                     // 物理页框ID
    struct PhysicalPage *next;  // 指向下一项
} PhysicalPage;

//逻辑地址管理，采用虚拟页式，后续可以拓展为虚拟段页式
//页表项
typedef struct {
    int page_number;        // 页号
    int physical_page;      // 物理页框ID（若为-1表示未映射）
    int valid_bit;          // 是否有效，即是否调入内存
    int modified;           // 该页是否被修改过，在发生页面交换时需要用到
} PageTableEntry;

//页表
// 进程的页表
typedef struct {
    PageTableEntry entries[LOGICAL_MEMORY_SIZE / PAGE_SIZE]; 
    //目前只采用单级页表，后续可以拓展为多级页表
} PageTable;

// 函数声明
void print_memory_information(void);
void init_memory(void);
PhysicalPage* allocate_physical_memory(void);
void free_physical_memory(int physical_page_id);
PageTable* create_page_table(int pid);
void free_page_table(int pid, PageTable *pt);
int handle_page_fault(int logical_page);
void visit_logical_memory_page(int logical_page, PCB *process);

// 全局变量声明（应在.c文件中定义）
extern PhysicalPage *free_page_list;
extern PhysicalPage *using_page_list;

#endif // MEMORY_H