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
#define MAX_LOGICAL_PAGES (LOGICAL_MEMORY_SIZE / PAGE_SIZE) // 逻辑内存页数（262144）
#define MAX_PHYSICAL_PAGES (PHYSICAL_MEMORY_SIZE / PAGE_SIZE) // 物理内存页数（4096）
// #define MAX_PHYSICAL_PAGES 5
#define NO_PHYSICAL_PAGE -2                                 // 当前页表项没有映射到物理页（关于为什么设置为-2，因为初始化驻留集时值为-1，这样设置可以避免冲突）
#define INVALID 0                       // 当前页表项无效
#define VALID 1                         // 当前页表项有效
#define MODIFIED 1                      // 当前页表项对应的内存被修改过
#define NOT_MODIFIED 0                  // 当前页表项对应的内存没有被修改（即没有进行写操作）
#define RESIDENT_SET_SIZE 4             // 驻留集大小（假设值，需与实际实现一致）
#define TLB_SIZE 10                     // TLB条目数量

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

// 页表
//  二级页表
typedef struct {
    PageTableEntry entries[SECOND_LEVEL_SIZE];
    //目前只采用单级页表，后续可以拓展为多级页表
} SecondLevelPageTable;

// 一级页表
typedef struct
{
    SecondLevelPageTable *entries[FIRST_LEVEL_SIZE];
} PageTable;

// 页面替换策略枚举类型
typedef enum {
    REPLACEMENT_FIFO,
    REPLACEMENT_LRU
} ReplacementPolicy;

extern ReplacementPolicy current_policy; // 当前置换策略（全局）

// 双端队列节点
typedef struct PageNode {
    int physical_page_id;
    struct PageNode *prev;
    struct PageNode *next;
} PageNode;

// 双端队列，用于LRU或FIFO
typedef struct {
    PageNode *front;
    PageNode *rear;
    int size;
} PageQueue;

extern PageQueue *physical_page_queue;
extern PageQueue *resident_set_queue;

// TLB
typedef struct
{
    int virtual_page;  // 虚拟页号
    int physical_page; // 物理页框ID
} TLBEntry;

typedef struct
{
    TLBEntry entries[TLB_SIZE]; // TLB条目数组
    int asid;                   // 地址空间ID(ASID)，这里简单等于PID
    int size;                   // 当前TLB中有效条目数
    int hit_count;              // TLB命中次数
    int miss_count;             // TLB未命中次数
} TLB;

// 函数声明
void print_memory_information(void);                                                             // 打印内存信息
void init_memory(void);                                                                          // 初始化内存
PhysicalPage *allocate_physical_memory(void);                                                    // 申请物理页框
void free_physical_memory(int physical_page_id);                                                 // 释放物理页框
PageTable *create_page_table(int pid);                                                           // 创建页表
PageTableEntry *get_page_table_entry(PageTable *pt, int logical_page, bool allocate_if_missing); // 获取页表项
void free_page_table(int pid, PageTable *pt);                                                    // 释放页表
int handle_page_fault(int logical_page);                                                         // 处理缺页
void visit_logical_memory_page(int logical_page, PCB *process, int write_flag);                  // 访问逻辑页
void print_page_table(PCB *process);                                                             // 打印页表
int select_victim_page(PageTable *pt);                                                           // 选择一个被修改过的页面

PageQueue* create_page_queue();                                                      // 分配并初始化双端队列
PageNode* create_page_node(int physical_page_id);                                    // 创建一个新的页面节点
PageNode* find_page_node(PageQueue *queue, int physical_page_id);                    // 查找页面节点(用于LRU)
void move_to_front(PageQueue *queue, PageNode *node);                                // 将某个节点移动到队首(仅用于LRU)
int evict_page_from_queue(PageQueue *queue, int max_capacity);                       // 返回淘汰页面的ID                           
int insert_page(PageQueue *queue, int physical_page_id, int max_capacity);           // 插入一个页面节点到双端队列中
void remove_page_from_queue(PageQueue *queue, int physical_page_id);                 // 从双端队列中删除一个页面节点
void access_page(int physical_page_id);                                              // 访问页面节点(用于LRU)      
void print_queue(PageQueue *queue);                                                  // 打印双端队列的调试函数
void free_page_queue(PageQueue *queue);                                              // 释放双端队列

void print_tlb(TLB *tlb);                                          // 打印TLB内容的调试函数                                                             // 打印TLB内容
TLB *create_tlb(int pid);                                          // 初始化TLB
int tlb_lookup(TLB *tlb, int virtual_page);                        // 查找TLB
int tlb_add_entry(TLB *tlb, int virtual_page, int physical_page); // 添加TLB条目
void tlb_delete_entry(TLB *tlb, int physical_page);                // 删除TLB条目
void free_tlb(TLB *tlb);                                           // 释放TLB

int sys_handle_page_fault(int logical_page, int pid);                                  // 系统调用处理缺页
int sys_get_physical_page_status(int physical_page_id, int *valid_bit, int *modified); // 系统调用获取物理页框状态
int sys_clear_page_table(int pid);                                                     // 系统调用清除页表

void test_TLB_1_1();
void test_TLB_1_2();
void test_TLB_1_3();
void test_TLB_1_4();
void test_TLB_2_1();
void test_TLB_2_2();
void test_TLB_3_1();
void test_TLB_3_2();
void test_FIFO_replacement();
void test_LRU_replacement();
void test_LRU_3();
// void test_FIFO_vs_LRU();
// void test_policy_switch();
// void test_eviction_order();
void memory_testing_task_1(void);
void memory_testing_task_2(void);                               // 测试用例2
void memory_testing_task_3(void);                               // 测试用例3
void memory_testing_task_4(void);                               // 测试用例4   
void memory_testing_task_5(void);
// 全局变量声明（应在.c文件中定义）
extern PhysicalPage *free_page_list;
extern PhysicalPage *using_page_list;

#endif // MEMORY_H