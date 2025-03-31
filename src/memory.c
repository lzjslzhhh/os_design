#include<stdio.h>
#include<stdlib.h>
#include"memory.h"

#define PAGE_SIZE 1024 //页面大小为1K
#define PHYSICAL_MEMORY_SIZE (1<<22) //物理内存大小为4MB
#define LOGICAL_MEMORY_SIZE (1<<30)  // 逻辑内存大小为1GB 

//物理地址管理
// 使用链表这一数据结构来管理物理内存，并且直接以页为单位来进行管理
typedef struct PhysicalPage {
    int id;                     // 物理页框ID
    struct PhysicalPage *next;  // 链表指针
} PhysicalPage;

PhysicalPage *free_page_list = NULL;  // 物理页链表最开始为空


//逻辑地址管理，采用虚拟页式，后续可以拓展为虚拟段页式
//页表项
typedef struct {
    int page_number;       // 页号
    int physical_page;     // 物理页框ID（若为-1表示未映射）
    int valid_bit;         // 是否有效
} PageTableEntry;

//页表
// 进程的页表
typedef struct {
    PageTableEntry entries[LOGICAL_MEMORY_SIZE / PAGE_SIZE]; 
    //目前只采用单级页表，后续可以拓展为多级页表
} PageTable;

void init_memory(void)
{
    printf("start initializing physical memory.\n");
    //其实就是初始化单链表的步骤
    for (int i = 0; i < PHYSICAL_MEMORY_SIZE / PAGE_SIZE; i++) 
    {
        PhysicalPage *page = (PhysicalPage *)malloc(sizeof(PhysicalPage));
        page->id = i;
        page->next = free_page_list;
        free_page_list = page;
    }
    printf("finish initializing physical memory.\n");
}

int allocate_physical_memory()
{
    printf("start allocating physical memory.\n");
    //从free_page_list中分配一个物理页给请求者，返回值为物理页框号
    if (free_page_list == NULL)
    {
        //当前没有空闲页
        printf("allocating physical memory failed.\n");
        return -1
    }

    //有空闲页的话，就将链表头指向的那一页分配出去，并将链表头指向下一页
    PhysicalPage * page = free_page_list;
    free_page_list = free_page_list -> next;
    int allocated_page_frame_id = page -> id;

    //将分频给这一链表项的内存释放
    free(page);
    page = NULL;

    printf("finish allocating physical memory, page frame id %d.\n", allocated_page_frame_id);
    return allocated_page_id;
}

void free_physical_memory(int page_id)
{
    //同样的，释放物理内存时直接插入在链表头即可
    PhysicalPage *page = (PhysicalPage *)malloc(sizeof(PhysicalPage));
    page->id = page_id;
    page->next = free_page_list;
    free_page_list = page;
}


