#include <stdio.h>
#include <stdlib.h>
#include "memory.h"
#include "process.h"
#include <assert.h>
#include <time.h>

PhysicalPage *free_page_list = NULL;  // 记录目前空闲的页表，单向链表
PhysicalPage *using_page_list = NULL; // 记录已被分配的页表，双向链表

const int DEBUG = 1; // 调试开关，0表示关闭，1表示打开

void print_memory_information()
{
    // 打印内存使用情况
    int cnt_free_page = 0;
    int cnt_using_page = 0;
    PhysicalPage *ptr = free_page_list;
    while (ptr != NULL)
    {
        cnt_free_page++;
        ptr = ptr->next;
    }
    ptr = using_page_list;
    while (ptr != NULL)
    {
        cnt_using_page++;
        ptr = ptr->next;
    }
    printf("%d free pages, %d pages is being used.\n", cnt_free_page, cnt_using_page);
}

void init_memory(void)
{
    printf("start initializing physical memory.\n");
    // 其实就是初始化单链表的步骤
    for (int i = PHYSICAL_MEMORY_SIZE / PAGE_SIZE - 1; i >= 0; i--)
    {
        PhysicalPage *page = (PhysicalPage *)malloc(sizeof(PhysicalPage));
        page->id = i;
        page->next = free_page_list;
        free_page_list = page;
    }
    printf("finish initializing physical memory.\n");
}

PhysicalPage *allocate_physical_memory()
{
    // 这里的分配物理内存只是一个很简单的实现，并没有采用FIFO
    printf("\nstart allocating physical memory.\n");
    // 从free_page_list中分配一个物理页给请求者，返回值为物理页框
    if (free_page_list == NULL)
    {
        // 当前没有空闲页，后续要在这里添加逻辑，来处理页面换出
        // 可以采用LRU的方法将页面换出，代码上是对using_page_list进行操作
        printf("no free page in memory.\n");
        printf("start to swap a page into the memory.\n");

        // 待补充代码

        return NULL;
    }

    // 有空闲页的话，就将链表头指向的那一页分配出去，并将链表头指向下一页
    PhysicalPage *page = free_page_list;
    free_page_list = free_page_list->next;
    int allocated_page_frame_id = page->id;

    // 将被分配的空闲页插入到 使用页 链表中
    page->next = using_page_list;
    using_page_list = page;

    printf("\nfinish allocating physical memory, page frame id %d.\n", allocated_page_frame_id);
    return page;
}

void free_physical_memory(int physical_page_id)
{
    // 根据物理页号来释放对应的物理页
    printf("\nstart to free physical memory, page id = %d\n", physical_page_id);
    // 同样的，释放物理内存时要删除使用页链表里的表项，并插入在空闲页的链表头
    if (using_page_list == NULL)
    {
        printf("no page is used. Freeing memory failed.\n");
        return;
    }

    PhysicalPage *current = using_page_list;
    PhysicalPage *prev = NULL;
    // 从前往后遍历找到要删除的表项
    while (current != NULL && current->id != physical_page_id)
    {
        prev = current;
        current = current->next;
    }

    if (current == NULL)
    {
        printf("Page not found. Freeing memory failed.\n");
        return;
    }

    if (prev == NULL)
    {
        // 要释放的页刚好是头结点
        using_page_list = current->next;
    }
    else
    {
        prev->next = current->next;
    }

    // 将释放的页插入到空闲页的表头
    current->next = free_page_list;
    free_page_list = current;

    printf("finish freeing physical memory, page id = %d\n", physical_page_id);
}

PageTable *create_page_table(int pid)
{
    printf("\nstart to create 2-level page table of process %d.\n", pid);
    PageTable *pt = (PageTable *)malloc(sizeof(PageTable));
    for (int i = 0; i < FIRST_LEVEL_SIZE; i++)
    {
        pt->entries[i] = NULL;
    }
    printf("finish creating page table of process %d.\n", pid);
    return pt;
}

PageTableEntry *get_page_table_entry(PageTable *pt, int logical_page, bool allocate_if_missing)
{
    int level1_index = (logical_page >> 9) & 0x1FF;
    int level2_index = logical_page & 0x1FF;

    if (pt->entries[level1_index] == NULL)
    {
        if (!allocate_if_missing)
            return NULL;
        pt->entries[level1_index] = (SecondLevelPageTable *)malloc(sizeof(SecondLevelPageTable));
        for (int i = 0; i < SECOND_LEVEL_SIZE; i++)
        {
            pt->entries[level1_index]->entries[i].valid_bit = INVALID;
            pt->entries[level1_index]->entries[i].physical_page = NO_PHYSICAL_PAGE;
            pt->entries[level1_index]->entries[i].modified = NOT_MODIFIED;
        }
    }
    return &(pt->entries[level1_index]->entries[level2_index]);
}

void free_page_table(int pid, PageTable *pt)
{
    if (pt == NULL)
    {
        printf("\nfreeing process %d 's page table failed.\n", pid);
        return;
    }

    for (int i = 0; i < FIRST_LEVEL_SIZE; i++)
    {
        SecondLevelPageTable *second = pt->entries[i];
        if (second == NULL)
            continue;
        for (int j = 0; j < SECOND_LEVEL_SIZE; j++)
        {
            PageTableEntry *entry = &second->entries[j];
            if (entry->valid_bit)
            {
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
    PhysicalPage *pp = allocate_physical_memory();
    if (pp != NULL)
        return pp->id;
    else
        return NO_PHYSICAL_PAGE;
}

void visit_logical_memory_page(int logical_page, PCB *process, int write_flag)
{
    printf("\nstart to translate logical page %d into physical page.\n", logical_page);

    // 1. 首先检查TLB
    int physical_page_id = tlb_lookup(process->tlb, logical_page);
    if (physical_page_id != NO_PHYSICAL_PAGE&& physical_page_id != -1)
    {
        printf("\nTLB hit: logical page %d -> physical page %d\n", logical_page, physical_page_id);
        // 处理写操作
        if (write_flag)
        {
            // 更新页表
            PageTableEntry *entry = get_page_table_entry(process->pt, logical_page, true);
            if (entry)
            {
                entry->modified = MODIFIED;
            }
        }
        return;
    }
    else
    {
        printf("\nTLB miss: logical page %d not found in TLB.\n", logical_page);
    }

    // 2. TLB未命中，继续原来的页表查找流程
    if (logical_page >= MAX_LOGICAL_PAGES)
    {
        // 越界中断
        printf("\ntrigger a segmentation fault when visiting logical page %d.\n", logical_page);
        return;
    }

    //  PageTable *pt = process -> pt;
    PageTableEntry *entry = get_page_table_entry(process->pt, logical_page, true);
    physical_page_id = entry->physical_page;
    int valid_bit = entry->valid_bit;
    if (entry == NULL)
    {
        printf("trigger a segmentation fault when visiting logical page %d.\n", logical_page);
        return;
    }

    // 先检查该页是否在驻留集中
    bool check_ppi_in_rss = false;
    for (int i = 0; i < RESIDENT_SET_SIZE; i++)
    {
        if (process->resident_set[i] == physical_page_id)
        {
            // 该物理页在驻留集中
            check_ppi_in_rss = true;
            break;
        }
    }

    if (check_ppi_in_rss && valid_bit && physical_page_id != NO_PHYSICAL_PAGE)
    {
        // 如果在驻留集中/页表有对应的物理页且有效位有效，说明命中
        printf("logical page %d -> physical page %d , page table look up hits.\n", logical_page, physical_page_id);
    }
    else
    {
        // 没有命中，就再进行映射与驻留集处理
        // 未在页表命中，可能是没有分配物理页，也有可能是分配的物理页被换出了，导致有效位为0
        // 处理缺页中断，分配一个物理页
        physical_page_id = handle_page_fault(logical_page);
        if (physical_page_id == NO_PHYSICAL_PAGE)
        {
            // 这里后续要补充新逻辑，有可能物理页满，因此返回的物理页号为NO_PHYSICAL_PAGE
            printf("translate logical page %d failed because of no free space of physical page.\n", logical_page);
            return;
        }

        // 更新该进程的驻留集情况
        // 简单的使用FIFO对驻留集进行管理
        int victim_physical_page = process->resident_set[process->rss_ptr];
        if (victim_physical_page != -1)
        {
            // 找到要被替换出去的物理页所映射的逻辑页
            printf("\nthe victim physical page %d need to be swapped out.\n", victim_physical_page);
            for (int i = 0; i < FIRST_LEVEL_SIZE; i++)
            {
                SecondLevelPageTable *second = process->pt->entries[i];
                if (second == NULL)
                    continue;
                for (int j = 0; j < SECOND_LEVEL_SIZE; j++)
                {
                    PageTableEntry *victim_entry = &second->entries[j];
                    if (victim_entry->physical_page == victim_physical_page)
                    {
                        victim_entry->valid_bit = INVALID;
                        victim_entry->physical_page = NO_PHYSICAL_PAGE;
                        if (victim_entry->modified != NOT_MODIFIED)
                        {
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

        // （虽然前面检查是否命中是是三个条件（check_ppi_in_rss && valid_bit && physical_page_id != NO_PHISYCAL_PAGE）一起检查）
        entry->valid_bit = VALID;
        entry->modified = NOT_MODIFIED;
        // 更新驻留集
        process->resident_set[process->rss_ptr] = physical_page_id;
        process->rss_ptr = (process->rss_ptr + 1) % RESIDENT_SET_SIZE;
    }

    if (write_flag)
    {
        entry->modified = MODIFIED;
    }

    printf("\nfinish translating logical page %d into physical page %d.\n", logical_page, physical_page_id);

    // 3. 在成功完成地址转换后，将映射关系添加到TLB
    if (physical_page_id != NO_PHYSICAL_PAGE)
    {
        tlb_add_entry(process->tlb, logical_page, physical_page_id);
    }
}

void print_page_table(PCB *process)
{
    printf("Page Table for Process %d:\n", process->pid);
    printf("Logical Page | Physical Page | Valid | Modified\n");
    printf("------------------------------------------------\n");

    for (int i = 0; i < FIRST_LEVEL_SIZE; i++)
    {
        SecondLevelPageTable *second = process->pt->entries[i];
        if (second == NULL)
            continue;
        for (int j = 0; j < SECOND_LEVEL_SIZE; j++)
        {
            PageTableEntry *entry = &second->entries[j];
            if (entry->valid_bit)
            {
                int logical_page = (i << 9) | j;
                printf("%12d | %13d | %5d | %8d\n",
                       logical_page, entry->physical_page, entry->valid_bit, entry->modified);
            }
        }
    }
    printf("------------------------------------------------\n");
}

// int select_victim_page(PageTable *pt){

// }

// 打印TLB内容的调试函数
void print_tlb(TLB *tlb)
{
    printf("\n=== TLB for Process %d Debug Info ===\n", tlb->asid);
    printf("Total Entries: %d/%d (Used/Total)\n", tlb->size, TLB_SIZE);
    printf("Hit Count: %d, Miss Count: %d\n", tlb->hit_count, tlb->miss_count);
    printf("------------------------------------\n");
    printf("Index | Virtual Page | Physical Page\n");
    printf("------------------------------------\n");

    for (int i = 0; i < TLB_SIZE; i++)
    {
        TLBEntry *entry = &tlb->entries[i];
        printf("%5d | %12d | %13d\n",
               i,
               entry->virtual_page,
               entry->physical_page);
    }
    printf("------------------------------------\n");
}

// 初始化TLB
TLB *create_tlb(int pid)
{
    printf("\nstart to create TLB of process %d.\n", pid);
    TLB *tlb = (TLB *)malloc(sizeof(TLB));
    if (tlb == NULL)
    {
        printf("Failed to allocate memory for TLB.\n");
        return NULL;
    }
    for (int i = 0; i < TLB_SIZE; i++)
    {
        tlb->entries[i].virtual_page = -1;
        tlb->entries[i].physical_page = NO_PHYSICAL_PAGE;
    }
    tlb->size = 0;
    tlb->hit_count = 0;
    tlb->miss_count = 0;
    tlb->asid = pid; // 地址空间ID(ASID)，这里简单等于PID
    printf("finish creating TLB of process %d.\n", pid);
    return tlb;
}

// TLB查找
int tlb_lookup(TLB *tlb, int virtual_page)
{
    // 检查参数合法性
    if (virtual_page == NULL || virtual_page < 0 || virtual_page >= MAX_LOGICAL_PAGES)
    {
        printf("Invalid page number.\n");
        return -1; // 返回-1表示参数错误
    }
    for (int i = 0; i < TLB_SIZE; i++)
    {
        if (tlb->entries[i].virtual_page == virtual_page)
        {
            tlb->hit_count++;                     // 命中计数
            return tlb->entries[i].physical_page; // 返回物理页框ID
        }
    }
    tlb->miss_count++;       // 未命中计数
    return NO_PHYSICAL_PAGE; // 未命中
}

// 添加TLB条目（使用随机置换策略）
int tlb_add_entry(TLB *tlb, int virtual_page, int physical_page)
{
    // 检查参数合法性
    if (virtual_page == NULL || physical_page == NULL || virtual_page < 0 || physical_page < 0 || virtual_page >= MAX_LOGICAL_PAGES || physical_page >= PHYSICAL_MEMORY_SIZE)
    {
        printf("Invalid page number.\n");
        return -1; // 返回-1表示参数错误
    }
    // 检查是否重复插入
    for (int i = 0; i < TLB_SIZE; i++)
    {
        if (tlb->entries[i].virtual_page == virtual_page)
        {
            return -2; // 返回-2表示重复插入
        }
    }

    // 查找是否有空闲位置
    for (int i = 0; i < TLB_SIZE; i++)
    {
        if (tlb->entries[i].virtual_page == -1)
        {
            tlb->entries[i].virtual_page = virtual_page;
            tlb->entries[i].physical_page = physical_page;
            tlb->size++;
            return i; // 返回插入位置
        }
    }

    // 没有空闲位置，随机替换一个条目
    int random_index = rand() % TLB_SIZE;
    tlb->entries[random_index].virtual_page = virtual_page;
    tlb->entries[random_index].physical_page = physical_page;
    return random_index; // 返回替换位置
}

// 物理页换出时删除TLB条目
void tlb_delete_entry(TLB *tlb, int physical_page)
{
    // 检查参数合法性
    if (physical_page == NULL || physical_page < 0 || physical_page >= PHYSICAL_MEMORY_SIZE)
    {
        printf("Invalid page number.\n");
        return;
    }
    // 删除TLB中对应的条目
    for (int i = 0; i < TLB_SIZE; i++)
    {
        if (tlb->entries[i].physical_page == physical_page)
        {
            tlb->entries[i].virtual_page = -1;
            tlb->entries[i].physical_page = NO_PHYSICAL_PAGE;
            tlb->size--;
            return;
        }
    }
}

void free_tlb(TLB *tlb)
{
    if(tlb == NULL)
    {
        printf("freeing TLB failed.\n");
        return;
    }
    // 释放TLB
    free(tlb);
    printf("\nfinish freeing process %d 's TLB.\n", tlb->asid);
}

void memory_testing_task_1()
{
    init_memory();
    PCB *process = create_process(1, 100);
    visit_logical_memory_page(0, process, 0);
    visit_logical_memory_page(0, process, 1);
}

// TLB测试用例 1.1：TLB未满时插入页号
void test_TLB_1_1()
{
    printf("=== Test 1.1: TLB insert when not full ===\n");
    TLB *tlb = create_tlb(1);
    if (DEBUG)
        print_tlb(tlb); // 打印TLB内容

    // 插入5个页号
    for (int i = 1; i <= 5; i++)
    {
        tlb_add_entry(tlb, i, i * 100);
    }
    if (DEBUG)
        print_tlb(tlb); // 打印TLB内容
    // 验证TLB中包含全部5个页号
    assert(tlb->size == 5);
    for (int i = 1; i <= 5; i++)
    {
        int phys_page = tlb_lookup(tlb, i);
        assert(phys_page == i * 100); // 验证物理页号
    }
    printf("Test 1.1 PASSED\n");
    // 释放TLB
    free_tlb(tlb);
}

// TLB测试用例 1.2：TLB满时随机置换
void test_TLB_1_2()
{
    printf("=== Test 1.2: TLB random replacement when full ===\n");
    TLB *tlb = create_tlb(1);
    if (DEBUG)
        print_tlb(tlb); // 打印TLB内容
    // 填满TLB（1-10）
    for (int i = 1; i <= TLB_SIZE; i++)
    {
        tlb_add_entry(tlb, i, i * 100);
    }
    if (DEBUG)
        print_tlb(tlb); // 打印TLB内容
    assert(tlb->size == TLB_SIZE);

    // 插入第11个页号
    tlb_add_entry(tlb, 11, 1100);
    if (DEBUG)
        print_tlb(tlb); // 打印TLB内容
    // 验证TLB中有一个旧页号被替换
    int found_count = 0;
    for (int i = 1; i <= 11; i++)
    {
        int phys_page = tlb_lookup(tlb, i);
        if (phys_page != NO_PHYSICAL_PAGE)
        {
            found_count++;
        }
    }
    assert(found_count == TLB_SIZE); // 最多只有10个条目能被找到
    printf("Test 1.2 PASSED\n");
    // 释放TLB
    free_tlb(tlb);
}

// TLB测试用例 1.3：查询命中
void test_TLB_1_3()
{
    printf("=== Test 1.3: TLB lookup hit ===\n");
    TLB *tlb = create_tlb(1);
    if (DEBUG)
        print_tlb(tlb); // 打印TLB内容
    // 插入测试数据
    tlb_add_entry(tlb, 3, 300);
    if (DEBUG)
        print_tlb(tlb); // 打印TLB内容

    int initial_hits = tlb->hit_count; // 记录初始命中次数

    // 查询存在的页号
    int phys_page = tlb_lookup(tlb, 3);
    if (DEBUG)
        print_tlb(tlb); // 打印TLB内容
    // 验证结果
    assert(phys_page != NO_PHYSICAL_PAGE);
    assert(phys_page == 300);
    assert(tlb->hit_count == initial_hits + 1);
    printf("Test 1.3 PASSED\n");
    // 释放TLB
    free_tlb(tlb);
}

// TLB测试用例 1.4：查询未命中
void test_TLB_1_4()
{
    printf("=== Test 1.4: TLB lookup miss ===\n");
    TLB *tlb = create_tlb(1);
    if (DEBUG)
        print_tlb(tlb); // 打印TLB内容
    // 不插入页号99
    int initial_misses = tlb->miss_count;

    // 查询不存在的页号
    int phys_page = tlb_lookup(tlb, 3);
    if (DEBUG)
        print_tlb(tlb); // 打印TLB内容
    // 验证结果
    assert(phys_page == NO_PHYSICAL_PAGE);
    assert(tlb->miss_count == initial_misses + 1);
    printf("Test 1.4 PASSED\n");
    // 释放TLB
    free_tlb(tlb);
}

// TLB测试用例 2.1：重复页号插入
void test_TLB_2_1()
{
    printf("\n=== Test 2.1: Repeated Page Insertion ===\n");
    TLB *tlb = create_tlb(1);
    if (DEBUG)
        print_tlb(tlb); // 打印TLB内容

    // 重复插入页号7共11次
    for (int i = 0; i < 11; i++)
    {
        tlb_add_entry(tlb, 7, 700);
    }
    if (DEBUG)
        print_tlb(tlb); // 打印TLB内容

    // 验证TLB中页号7存在且无置换
    int found_count = 0;
    for (int i = 0; i < TLB_SIZE; i++)
    {
        if (tlb->entries[i].virtual_page == 7)
        {
            found_count++;
        }
    }
    if (DEBUG)
        print_tlb(tlb);       // 打印TLB内容
    assert(found_count == 1); // 应只存在一个条目
    printf("Test 2.1 PASSED\n");
    // 释放TLB
    free_tlb(tlb);
}

// TLB测试用例 2.2:随机置换均匀性验证
void test_TLB_2_2()
{
    printf("\n=== Test 2.2: Random Replacement Uniformity ===\n");
    TLB *tlb = create_tlb(1);
    if (DEBUG)
        print_tlb(tlb);                     // 打印TLB内容
    int replacement_counts[TLB_SIZE] = {0}; // 记录每个条目被替换的次数

    // 1. 首先填满TLB
    for (int i = 0; i < TLB_SIZE; i++)
    {
        tlb_add_entry(tlb, i, i * 100);
    }
    if (DEBUG)
        print_tlb(tlb); // 打印TLB内容

    // 2. 插入1000个随机页号（范围1-1000）
    const int TEST_RUNS = 1000;
    srand(time(NULL)); // 设置随机种子
    for (int i = 0; i < TEST_RUNS; i++)
    {
        int new_page = rand() % 1000 + 1;
        int old_page = tlb_add_entry(tlb, new_page, new_page * 100);
        if (old_page != -1)
        {
            // 记录被替换的页号
            replacement_counts[old_page]++;
        }
    }
    if (DEBUG)
        print_tlb(tlb); // 打印TLB内容
    // 3. 简单验证：所有条目都应被替换过
    for (int i = 0; i < TLB_SIZE; i++)
    {
        assert(replacement_counts[i] > 0);
    }
    printf("Test 2.2 PASSED\n");
    // 释放TLB
    free_tlb(tlb);
}

// TLB测试用例 3.1：非法页号处理
void test_TLB_3_1()
{
    printf("\n=== Test 3.1: Invalid Page Number Handling ===\n");
    TLB *tlb = create_tlb(1);
    if (DEBUG)
        print_tlb(tlb); // 打印TLB内容
    //  插入非法页号
    // 1. 页号为NULL
    // 2. 页号越界
    // 4. 页号为负数
    // 3. 物理页号越界
    // 5. 物理页号为负数
    int invalid_page = tlb_add_entry(tlb, NULL, 100);
    assert(invalid_page == -1); // 返回-1表示参数错误
    invalid_page = tlb_add_entry(tlb, -1, 100);
    assert(invalid_page == -1); // 返回-1表示参数错误
    invalid_page = tlb_add_entry(tlb, MAX_LOGICAL_PAGES, 100);
    assert(invalid_page == -1); // 返回-1表示参数错误
    invalid_page = tlb_add_entry(tlb, 1, PHYSICAL_MEMORY_SIZE);
    assert(invalid_page == -1); // 返回-1表示参数错误
    invalid_page = tlb_add_entry(tlb, 1, -100);
    assert(invalid_page == -1); // 返回-1表示参数错误
    // 验证TLB中没有插入非法页号
    assert(tlb->size == 0);
    for (int i = 0; i < TLB_SIZE; i++)
    {
        assert(tlb->entries[i].virtual_page == -1);
        assert(tlb->entries[i].physical_page == NO_PHYSICAL_PAGE);
    }
    if (DEBUG)
        print_tlb(tlb); // 打印TLB内容
    // 查找非法页号
    // 1. 页号为NULL
    // 2. 页号越界
    // 3. 页号为负数
    int phys_page = tlb_lookup(tlb, NULL);
    assert(phys_page == -1); // 返回-1表示参数错误
    phys_page = tlb_lookup(tlb, MAX_LOGICAL_PAGES);
    assert(phys_page == -1); // 返回-1表示参数错误
    phys_page = tlb_lookup(tlb, -1);
    assert(phys_page == -1); // 返回-1表示参数错误
    // 验证TLB中没有插入非法页号
    assert(tlb->size == 0);
    for (int i = 0; i < TLB_SIZE; i++)
    {
        assert(tlb->entries[i].virtual_page == -1);
        assert(tlb->entries[i].physical_page == NO_PHYSICAL_PAGE);
    }
    if (DEBUG)
        print_tlb(tlb); // 打印TLB内容
    printf("Test 3.1 PASSED\n");
    // 释放TLB
    free_tlb(tlb);
}

// TLB测试用例 3.2：满表重复页号
void test_TLB_3_2()
{
    printf("\n=== Test 3.2: Repeated Page in Full TLB ===\n");
    TLB* tlb = create_tlb(1);
    // 填满TLB（1-10号页）
    for (int i = 1; i <= TLB_SIZE; i++)
    {
        tlb_add_entry(tlb, i, i * 100);
    }
    int initial_hits = tlb->hit_count;

    // 重复插入页号1
    tlb_add_entry(&tlb, 1, 100);

    // 检查TLB内容是否变化
    bool page1_exists = false;
    int page1_count = 0;
    for (int i = 0; i < TLB_SIZE; i++)
    {
        if (tlb->entries[i].virtual_page == 1)
        {
            page1_exists = true;
            page1_count++;
        }
    }

    if (page1_exists && page1_count == 1)
    {
        printf("Test PASSED: Page 1 remains in TLB without duplication\n");
    }
    else
    {
        printf("Test FAILED: Page 1 should exist exactly once in TLB\n");
    }
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
    visit_logical_memory_page(0, process, 1);
    visit_logical_memory_page(0, process2, 1);
}

void memory_testing_task_5(void)
{

}
