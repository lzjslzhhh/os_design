#include<stdio.h>
#include<stdlib.h>
#include"memory.h"
#include"process.h"

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
    printf("\nstart allocating physical memory.\n");
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

    printf("\nfinish allocating physical memory, page frame id %d.\n", allocated_page_frame_id);
    return page;
}

void free_physical_memory(int physical_page_id)
{
    //根据物理页号来释放对应的物理页
    printf("\nstart to free physical memory, page id = %d\n", physical_page_id);
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
    printf("\nstart to create 2-level page table of process %d.\n", pid);
    PageTable *pt = (PageTable*)malloc(sizeof(PageTable));
    for(int i=0; i < FIRST_LEVEL_SIZE; i++)
    {
        pt->entries[i]=NULL;              
    }
    printf("finish creating page table of process %d.\n", pid);
    return pt;
}

PageTableEntry* get_page_table_entry(PageTable *pt, int logical_page, bool allocate_if_missing)
{
    int level1_index = (logical_page >> 9) & 0x1FF;
    int level2_index = logical_page & 0x1FF;

    if(pt->entries[level1_index] == NULL)
    {
        if(!allocate_if_missing) return NULL;
        pt->entries[level1_index] = (SecondLevelPageTable*)malloc(sizeof(SecondLevelPageTable));
        for(int i=0; i < SECOND_LEVEL_SIZE; i++)
        {
            pt->entries[level1_index]->entries[i].valid_bit = INVALID;
            pt->entries[level1_index]->entries[i].physical_page = NO_PHYSICAL_PAGE;
            pt->entries[level1_index]->entries[i].modified = NOT_MODIFIED;
        }
    }
    return &(pt->entries[level1_index]->entries[level2_index]);
}

void free_page_table(int pid, PageTable * pt)
{
    if(pt == NULL)
    {
        printf("\nfreeing process %d 's page table failed.\n", pid);
        return;
    }

    for(int i = 0; i < FIRST_LEVEL_SIZE; i++) {
        SecondLevelPageTable *second = pt->entries[i];
        if(second == NULL) continue;
        for(int j = 0; j < SECOND_LEVEL_SIZE; j++) {
            PageTableEntry *entry = &second->entries[j];
            if(entry->valid_bit){
                free_physical_memory(entry->physical_page);
            }
        }
        free(second);
    }

    free(pt);
    printf("\nfinish freeing process %d 's page table.\n", pid);
}

int handle_page_fault(int logical_page)
{
    printf("\ntrigger a page fault when visiting logical page %d.\n", logical_page);
    PhysicalPage * pp = allocate_physical_memory();
    if(pp != NULL)
        return pp -> id;
    else
        return NO_PHYSICAL_PAGE;
}

void visit_logical_memory_page(int logical_page, PCB *process ,int write_flag)
{
    printf("\nstart to translate logical page %d into physical page.\n", logical_page);
    if (logical_page >= MAX_LOGICAL_PAGES)
    {
        //越界中断
        printf("\ntrigger a segmentation fault when visiting logical page %d.\n", logical_page);
        return;
    }

    //这里没有TLB的设计，因此直接访问页表
    // PageTable *pt = process -> pt;
    PageTableEntry *entry = get_page_table_entry(process -> pt, logical_page, true);
    int physical_page_id = entry->physical_page;
    int valid_bit = entry->valid_bit;
    if(entry == NULL)
    {
        printf("trigger a segmentation fault when visiting logical page %d.\n", logical_page);
        return;
    }

    //先检查该页是否在驻留集中
    bool check_ppi_in_rss = false;
    for(int i=0; i<RESIDENT_SET_SIZE; i++)
    {
        if(process -> resident_set[i] == physical_page_id)
        {
            //该物理页在驻留集中
            check_ppi_in_rss = true;
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

        //更新该进程的驻留集情况
        //简单的使用FIFO对驻留集进行管理
        int victim_physical_page = process -> resident_set[process -> rss_ptr];
        if (victim_physical_page != -1)
        {
            //找到要被替换出去的物理页所映射的逻辑页
            printf("\nthe victim physical page %d need to be swapped out.\n", victim_physical_page);
            for (int i = 0; i < FIRST_LEVEL_SIZE; i++) {
                SecondLevelPageTable *second = process->pt->entries[i];
                if(second==NULL) continue;
                for(int j = 0; j < SECOND_LEVEL_SIZE; j++) {
                    PageTableEntry *victim_entry = &second->entries[j];
                    if  (victim_entry->physical_page == victim_physical_page) {
                        victim_entry->valid_bit = INVALID;
                        victim_entry->physical_page = NO_PHYSICAL_PAGE;
                        if (victim_entry->modified!=NOT_MODIFIED) {
                            printf("the victim physical page %d has been modified.\n", victim_physical_page);
                            printf("write back into disk.\n");
                            // 这里要补充写回逻辑
                        }
                        victim_entry->modified = NOT_MODIFIED;
                        free_physical_memory(victim_physical_page);
                    }
                }
            }
        }

        entry->physical_page = physical_page_id;

        //（虽然前面检查是否命中是是三个条件（check_ppi_in_rss && valid_bit && physical_page_id != NO_PHISYCAL_PAGE）一起检查）
        entry->valid_bit = VALID;
        entry->modified = NOT_MODIFIED;
        //更新驻留集
        process -> resident_set[process -> rss_ptr] = physical_page_id;
        process -> rss_ptr = (process -> rss_ptr + 1) % RESIDENT_SET_SIZE;
    }

    if (write_flag) {
        entry->modified = MODIFIED;
    }

    printf("\nfinish translating logical page %d into physical page %d.\n", logical_page, physical_page_id);
}

void print_page_table(PCB *process) {
    printf("Page Table for Process %d:\n", process->pid);
    printf("Logical Page | Physical Page | Valid | Modified\n");
    printf("------------------------------------------------\n");
    
    for(int i = 0; i < FIRST_LEVEL_SIZE; i++) {
        SecondLevelPageTable *second = process->pt->entries[i];
        if(second==NULL) continue;
        for(int j = 0; j < SECOND_LEVEL_SIZE; j++) {
            PageTableEntry *entry = &second->entries[j];
            if (entry->valid_bit) {
                int logical_page = (i << 9) | j;
                printf("%12d | %13d | %5d | %8d\n", 
                    logical_page, entry->physical_page, entry->valid_bit, entry->modified);
            }
        }
    }
    printf("------------------------------------------------\n");
}

int select_victim_page(PageTable *pt){


}

//  访问合法逻辑页，不触发缺页
void memory_testing_task_1(void)
{
    init_memory();
    PCB *process = create_process(1, 100);
    visit_logical_memory_page(0, process,0);
    visit_logical_memory_page(0, process,1);
}

//  测试驻留集大小限制、FIFO 替换策略、页写回、页表更新。
void memory_testing_task_2(void)
{
    init_memory();
    PCB* process = create_process(1, 500);
    int testing_page_sequence[8] = {1,2,3,4,1,2,7,8};
    int testing_length = 8;
    visit_logical_memory_page(testing_page_sequence[0], process, 1);
    for (int i=1;i<testing_length; i++)
    {
        print_process_information();
        visit_logical_memory_page(testing_page_sequence[i], process,0);
        print_process_information();
    }
    delete_process(process);
}

//  测试逻辑页边界是否合法，避免数组越界或非法内存访问
void memory_testing_task_3(void)
{
    init_memory();
    PCB* process = create_process(1, 500);
    visit_logical_memory_page(MAX_LOGICAL_PAGES, process,1);
    visit_logical_memory_page(MAX_LOGICAL_PAGES-1, process,1);
}

//  验证多进程各自维护驻留集和页表，不会相互影响
void memory_testing_task_4(void)
{
    init_memory();
    PCB* process = create_process(1, 500);
    PCB* process2 = create_process(1, 500);
    visit_logical_memory_page(0, process,1);
    visit_logical_memory_page(0, process2,1);
}

void memory_testing_task_5(void)
{

}
int main(void)
{
    // memory_testing_task_1();     // 预期输出:第二次访问应命中页表和驻留集，不触发缺页
    // memory_testing_task_2();     // 预期输出:触发页替换，且页0被写入磁盘后释放     
    // memory_testing_task_3();     // 预期输出:访问非法页，程序崩溃
    // memory_testing_task_4();     // 两个进程应有不同的物理页，互不干扰
    // memory_testing_task_5();
}