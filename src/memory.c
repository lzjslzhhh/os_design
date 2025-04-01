#include<stdio.h>
#include<stdlib.h>
#include"memory.h"
#include"process.h"

#define PAGE_SIZE 1024                  // 页面大小
#define PHYSICAL_MEMORY_SIZE (1<<22)    // 物理内存大小
#define LOGICAL_MEMORY_SIZE (1<<28)     // 逻辑内存大小
#define NO_PHYSICAL_PAGE -2             // 当前页表项没有映射到物理页（关于为什么设置为-2，因为初始化驻留集时值为-1，这样设置可以避免冲突）
#define INVALID 0                       // 当前页表项无效
#define NOT_MODIFIED 0                  // 当前页表项对应的内存没有被修改（即没有进行写操作）


PhysicalPage *free_page_list = NULL;    // 记录目前空闲的页表，单向链表
PhysicalPage *using_page_list = NULL;   // 记录已被分配的页表，双向链表

void print_memory_information()
{
    //打印内存使用情况
    int cnt_free_page = 0;
    int cnt_using_page = 0;
    PhysicalPage *ptr = free_page_list;
    while(ptr != NULL)
    {
        cnt_free_page ++;
        ptr = ptr -> next;
    }
    ptr = using_page_list;
    while(ptr != NULL)
    {
        cnt_using_page ++;
        ptr = ptr -> next;
    }
    printf("%d free pages, %d pages is being used.\n", cnt_free_page, cnt_using_page);
}

void init_memory(void)
{
    printf("start initializing physical memory.\n");
    //其实就是初始化单链表的步骤
    for (int i = PHYSICAL_MEMORY_SIZE / PAGE_SIZE - 1; i >= 0; i--) 
    {
        PhysicalPage *page = (PhysicalPage *)malloc(sizeof(PhysicalPage));
        page->id = i;
        page->next = free_page_list;
        free_page_list = page;
    }
    printf("finish initializing physical memory.\n");
}

PhysicalPage* allocate_physical_memory()
{
    //这里的分配物理内存只是一个很简单的实现，并没有采用FIFO
    printf("start allocating physical memory.\n");
    //从free_page_list中分配一个物理页给请求者，返回值为物理页框
    if (free_page_list == NULL)
    {
        //当前没有空闲页，后续要在这里添加逻辑，来处理页面换出
        //可以采用LRU的方法将页面换出，代码上是对using_page_list进行操作
        printf("no free page in memory.\n");
        printf("start to swap a page into the memory.\n");

        //待补充代码
        
        return NULL;
    }

    //有空闲页的话，就将链表头指向的那一页分配出去，并将链表头指向下一页
    PhysicalPage * page = free_page_list;
    free_page_list = free_page_list -> next;
    int allocated_page_frame_id = page -> id;

    //将被分配的空闲页插入到 使用页 链表中
    page -> next = using_page_list;
    using_page_list = page;

    printf("finish allocating physical memory, page frame id %d.\n", allocated_page_frame_id);
    return page;
}

void free_physical_memory(int physical_page_id)
{
    //根据物理页号来释放对应的物理页
    printf("start to free physical memory, page id = %d\n", physical_page_id);
    //同样的，释放物理内存时要删除使用页链表里的表项，并插入在空闲页的链表头
    if (using_page_list == NULL)
    {
        printf("no page is used. Freeing memory failed.\n");
        return;
    }

    PhysicalPage *current = using_page_list;
    PhysicalPage *prev = NULL;
    //从前往后遍历找到要删除的表项
    while(current != NULL && current->id != physical_page_id)
    {
        prev = current;
        current = current -> next;
    }

    if (current == NULL)
    {
        printf("Page not found. Freeing memory failed.\n");
        return;
    }

    if (prev == NULL)
    {
        //要释放的页刚好是头结点
        using_page_list = current -> next;
    }
    else
    {
        prev -> next = current -> next;
    }

    //将释放的页插入到空闲页的表头
    current -> next = free_page_list;
    free_page_list = current;

    printf("finish freeing physical memory, page id = %d\n", physical_page_id);
}

PageTable* create_page_table(int pid)
{
    printf("start to create page table of process %d.\n", pid);
    PageTable *pt = (PageTable*)malloc(sizeof(PageTable));
    for(int i=0; i < (LOGICAL_MEMORY_SIZE / PAGE_SIZE); i++)
    {
        pt->entries[i].page_number = i;                     // 页号=数组下标
        pt->entries[i].physical_page = NO_PHYSICAL_PAGE;    // 目前还未映射物理页
        pt->entries[i].valid_bit = INVALID;                 // 无效
        pt->entries[i].modified = NOT_MODIFIED;             // 未修改
    }
    printf("finish creating page table of process %d.\n", pid);
    return pt;
}

void free_page_table(int pid, PageTable * pt)
{
    if(pt == NULL)
    {
        printf("freeing process %d 's page table failed.\n", pid);
        return;
    }

    for(int i=0; i < (LOGICAL_MEMORY_SIZE / PAGE_SIZE); i++)
    {
        if (pt->entries[i].valid_bit == 1)
        {
            free_physical_memory(pt->entries[i].physical_page);
        }
    }

    free(pt);
    printf("finish freeing process %d 's page table.\n", pid);
}

int handle_page_fault(int logical_page)
{
    printf("trigger a page fault when visiting logical page %d.\n", logical_page);
    PhysicalPage * pp = allocate_physical_memory();
    if(pp != NULL)
        return pp -> id;
    else
        return NO_PHYSICAL_PAGE;
}

void visit_logical_memory_page(int logical_page, PCB *process)
{
    printf("start to translate logical page %d into physical page.\n", logical_page);
    if (logical_page >= (LOGICAL_MEMORY_SIZE / PAGE_SIZE))
    {
        //越界中断
        printf("trigger a segmentation fault when visiting logical page %d.\n", logical_page);
        return;
    }

    //这里没有TLB的设计，因此直接访问页表
    PageTable *pt = process -> pt;

    int physical_page_id = pt->entries[logical_page].physical_page;
    int valid_bit = pt->entries[logical_page].valid_bit;

    //先检查该页是否在驻留集中
    bool check_ppi_in_rss = 0;
    for(int i=0; i<RESIDENT_SET_SIZE; i++)
    {
        if(process -> resident_set[i] == physical_page_id)
        {
            //该物理页在驻留集中
            check_ppi_in_rss = 1;
            break;
        }
    }

    if(check_ppi_in_rss && valid_bit && physical_page_id != NO_PHYSICAL_PAGE)
    {
        //如果在驻留集中/页表有对应的物理页且有效位有效，说明命中
        printf("logical page %d -> physical page %d , page table look up hits.\n", logical_page, physical_page_id);
    }
    else
    {
        //没有命中，就再进行映射与驻留集处理
        //未在页表命中，可能是没有分配物理页，也有可能是分配的物理页被换出了，导致有效位为0
        //处理缺页中断，分配一个物理页
        physical_page_id = handle_page_fault(logical_page);
        if (physical_page_id == NO_PHYSICAL_PAGE)
        {
            //这里后续要补充新逻辑，有可能物理页满，因此返回的物理页号为NO_PHYSICAL_PAGE
            printf("translate logical page %d failed because of no free space of physical page.\n", logical_page);
            return;
        }

        pt->entries[logical_page].physical_page = physical_page_id;

        //（虽然前面检查是否命中是是三个条件（check_ppi_in_rss && valid_bit && physical_page_id != NO_PHISYCAL_PAGE）一起检查）
        pt->entries[logical_page].valid_bit = 1;

        pt->entries[logical_page].modified = NOT_MODIFIED;

        //更新该进程的驻留集情况
        //简单的使用FIFO对驻留集进行管理
        int victim_physical_page = process -> resident_set[process -> rss_ptr];
        if (victim_physical_page != -1)
        {
            //找到要被替换出去的物理页所映射的逻辑页
            printf("the victim physical page %d need to be swapped out.\n", victim_physical_page);
            for(int i=0; i < (LOGICAL_MEMORY_SIZE / PAGE_SIZE); i++)
            {
                if(victim_physical_page == pt->entries[i].physical_page)
                {
                    //该物理页被换出，对应逻辑页的有效位设置为无效，映射置为无映射
                    pt->entries[i].valid_bit = 0;
                    pt->entries[i].physical_page = NO_PHYSICAL_PAGE;

                    //如果这个页被修改过，那么需要额外写内存
                    if(pt->entries[i].modified != NOT_MODIFIED)
                    {
                        //这里只是简单输出个语句表示写回外存（磁盘）
                        printf("the victim physical page %d has been modified.\n", victim_physical_page);
                        printf("write back into disk.\n");
                    }
                    
                    pt->entries[i].modified = NOT_MODIFIED;
                    //被换出的页可以释放，不考虑共享页
                    free_physical_memory(victim_physical_page);
                }
            }
        }
        //更新驻留集
        process -> resident_set[process -> rss_ptr] = pt->entries[logical_page].physical_page;
        process -> rss_ptr = (process -> rss_ptr + 1) % RESIDENT_SET_SIZE;
    }

    printf("finish translating logical page %d into physical page %d.\n", logical_page, physical_page_id);
}