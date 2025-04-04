#ifndef MEMORY_H
#define MEMORY_H

#include <stdbool.h>

// 前向声明
typedef struct pcb PCB;


// 常量定义
#define PAGE_SIZE 1024                  // 页面大小
#define PHYSICAL_MEMORY_SIZE (1<<22)    // 物理内存大小
#define LOGICAL_MEMORY_SIZE (1<<28)     // 逻辑内存大小
#define FIRST_LEVEL_SIZE 512            // 一级页表大小
#define SECOND_LEVEL_SIZE 512           // 二级页表大小
#define MAX_LOGICAL_PAGES    (LOGICAL_MEMORY_SIZE / PAGE_SIZE) // 逻辑内存页数（262144）
#define NO_PHYSICAL_PAGE -2             // 当前页表项没有映射到物理页（关于为什么设置为-2，因为初始化驻留集时值为-1，这样设置可以避免冲突）
#define INVALID 0                       // 当前页表项无效
#define VALID 1                         // 当前页表项有效
#define MODIFIED 1                      // 当前页表项对应的内存被修改过 
#define NOT_MODIFIED 0                  // 当前页表项对应的内存没有被修改（即没有进行写操作）
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
    // int page_number;        // 页号
    int physical_page;      // 物理页框ID（若为-1表示未映射）
    int valid_bit;          // 是否有效，即是否调入内存
    int modified;           // 该页是否被修改过，在发生页面交换时需要用到
} PageTableEntry;

//页表
// 二级页表
typedef struct {
    PageTableEntry entries[SECOND_LEVEL_SIZE]; 
    //目前只采用单级页表，后续可以拓展为多级页表
} SecondLevelPageTable;

// 一级页表
typedef struct {
    SecondLevelPageTable *entries[FIRST_LEVEL_SIZE];
} PageTable;

// 函数声明
void print_memory_information(void);                            // 打印内存信息
void init_memory(void);                                         // 初始化内存                              
PhysicalPage* allocate_physical_memory(void);                   // 申请物理页框
void free_physical_memory(int physical_page_id);                // 释放物理页框
PageTable* create_page_table(int pid);                          // 创建页表 
PageTableEntry* get_page_table_entry(PageTable *pt, int logical_page, bool allocate_if_missing); // 获取页表项
void free_page_table(int pid, PageTable *pt);                   // 释放页表
int handle_page_fault(int logical_page);                        // 处理缺页
void visit_logical_memory_page(int logical_page, PCB *process,int write_flag); // 访问逻辑页
void print_page_table(PCB *process);                            // 打印页表
int select_victim_page(PageTable *pt);                          // 选择一个被修改过的页面
void memory_testing_task_1(void);                               // 测试用例1    
void memory_testing_task_2(void);                               // 测试用例2
void memory_testing_task_3(void);                               // 测试用例3
void memory_testing_task_4(void);                               // 测试用例4   
void memory_testing_task_5(void);                               // 测试用例5

// 全局变量声明（应在.c文件中定义）
extern PhysicalPage *free_page_list;
extern PhysicalPage *using_page_list;

#endif // MEMORY_H