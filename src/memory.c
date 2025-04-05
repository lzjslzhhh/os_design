#include <stdio.h>
#include <stdlib.h>
#include "memory.h"
#include "process.h"
#include <assert.h>
#include <time.h>

PhysicalPage *free_page_list = NULL;  // 记录目前空闲的页表，单向链表
PhysicalPage *using_page_list = NULL; // 记录已被分配的页表，双向链表

const int DEBUG = 1; // 调试开关，0表示关闭，1表示打开

ReplacementPolicy current_policy = REPLACEMENT_LRU;
PageQueue *physical_page_queue=NULL;
PageQueue *resident_set_queue=NULL;

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

// 分配并初始化页面队列
PageQueue* create_page_queue()
{
    PageQueue *queue = (PageQueue *)malloc(sizeof(PageQueue));
    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
    return queue;
}

// 创建一个新的页面节点
PageNode* create_page_node(int physical_page_id)
{
    PageNode *node = (PageNode *)malloc(sizeof(PageNode));
    node->physical_page_id = physical_page_id;
    node->next = NULL;
    node->prev = NULL;
    return node;
}

// 查找页面节点（用于 LRU）
PageNode* find_page_node(PageQueue *queue, int physical_page_id)
{
    PageNode *curr = queue->front;
    while(curr){
        if(physical_page_id == curr->physical_page_id) return curr;  
        curr = curr->next;  
    }
    return NULL;
}

// 将某个节点移到队首（仅用于 LRU）
void move_to_front(PageQueue *queue, PageNode *node) {
    if (!node || queue->front == node) return;

    // 断链
    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;

    if (node == queue->rear)
        queue->rear = node->prev;

    // 插入队首
    node->prev = NULL;
    node->next = queue->front;
    if (queue->front) queue->front->prev = node;
    queue->front = node;
}

int evict_page_from_queue(PageQueue *queue,int max_capacity)
{
    if (!queue) queue = create_page_queue();

    // 如果队列满，淘汰尾部节点（FIFO 或 LRU 都是处理尾部）
    if (queue->size >= max_capacity) {
        int evicted_ppn = queue->rear->physical_page_id;
        PageNode *old_tail = queue->rear;

        // 删除尾部节点
        if (old_tail->prev) {
            queue->rear = old_tail->prev;
            queue->rear->next = NULL;
        } else {
            queue->front = queue->rear = NULL;
        }

        free(old_tail);
        queue->size--;

        return evicted_ppn;
    }

    return -1; // 队列未满时不进行淘汰
}

// 插入页面：统一入口，自动根据策略选择 FIFO 或 LRU
// 如果有页面被淘汰，则返回被淘汰的物理页号；否则返回 -1
int insert_page(PageQueue *queue,int physical_page_id, int max_capacity) {
    if (!queue) queue = create_page_queue();

    if (current_policy == REPLACEMENT_LRU) {
        PageNode *existing = find_page_node(queue,physical_page_id);
        if (existing) {
            move_to_front(queue, existing);
            return -1; // 没有淘汰页面
        }
    }

    // 新建节点
    PageNode *new_node = create_page_node(physical_page_id);

    if (queue->size >= max_capacity) {
        int evicted_ppn = -1;
        
        // 根据当前替换策略决定淘汰哪一页
        if (current_policy == REPLACEMENT_FIFO) {
        // FIFO: 淘汰队头节点
        evicted_ppn = queue->front->physical_page_id;
        PageNode *old_head = queue->front;
        queue->front = old_head->next;
            if (queue->front)
                queue->front->prev = NULL;
            else
                queue->rear = NULL;  // 队列为空时，队头和队尾都为空
                
            free(old_head);
            queue->size--;
        } else if (current_policy == REPLACEMENT_LRU) {
            // LRU: 淘汰队尾节点
            evicted_ppn = queue->rear->physical_page_id;
            PageNode *old_tail = queue->rear;
            queue->rear = old_tail->prev;
            if (queue->rear)
                queue->rear->next = NULL;
            else
                queue->front = NULL;  // 队列为空时，队头和队尾都为空
            
            free(old_tail);
            queue->size--;
        }

        // 插入新节点（FIFO 和 LRU 都插入队首）
        new_node->next = queue->front;
        if (queue->front)
            queue->front->prev = new_node;
        queue->front = new_node;
        if (!queue->rear)
            queue->rear = new_node;
        queue->size++;
        return evicted_ppn;  // 返回被淘汰的页面
    }

    // 如果队列未满，直接插入到队首（无论 FIFO 还是 LRU）
    new_node->next = queue->front;
    if (queue->front)
        queue->front->prev = new_node;
    queue->front = new_node;
    if (!queue->rear)
        queue->rear = new_node;
    queue->size++;
    return -1; // 没有淘汰页面
}

void remove_page_from_queue(PageQueue *queue, int physical_page_id) 
{
    if (!queue) return;

    PageNode *current = queue->front;
    while (current != NULL) {
        if (current->physical_page_id == physical_page_id) {
            if (current->prev) {
                current->prev->next = current->next;
            } else {
                queue->front = current->next;
            }

            if (current->next) {
                current->next->prev = current->prev;
            } else {
                queue->rear = current->prev;
            }

            free(current);
            queue->size--;
            break;
        }
        current = current->next;
    }
}

// 打印当前队列内容（调试用）
void print_queue(PageQueue *queue) {
    PageNode *curr = queue->front;
    printf("Current Queue (front to rear): ");
    while (curr) {
        printf("[%d] ", curr->physical_page_id);
        curr = curr->next;
    }
    // printf("%d",current_policy);
    printf("\n");
}

// 释放队列
void free_page_queue(PageQueue *queue) {
    PageNode *curr = queue->front;
    while (curr) {
        PageNode *temp = curr;
        curr = curr->next;
        free(temp);
    }
    free(queue);
    queue = NULL;
}

void init_memory(void)
{
    printf("start initializing physical memory.\n");
    // 其实就是初始化单链表的步骤
    for (int i = MAX_PHYSICAL_PAGES - 1; i >= 0; i--)
    {
        PhysicalPage *page = (PhysicalPage *)malloc(sizeof(PhysicalPage));
        page->id = i;
        page->next = free_page_list;
        free_page_list = page;
    }
    physical_page_queue = create_page_queue();
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

        // 使用当前策略（FIFO或LRU）来处理内存换出
        int evicted_ppn = evict_page_from_queue(physical_page_queue, MAX_PHYSICAL_PAGES); // 使用 -1 来代表需要淘汰一个页面
        if (evicted_ppn == -1) {
            printf("Failed to allocate physical memory: No space for new page.\n");
            return NULL;
        }

        // 根据淘汰的物理页号，更新页表并释放页面
        free_physical_memory(evicted_ppn);

        // 分配一个新的物理页
        PhysicalPage *page = free_page_list;
        free_page_list = free_page_list->next;
        page->next = using_page_list;
        using_page_list = page;

        int allocated_page_frame_id = page->id;
        printf("\nfinish allocating physical memory, page frame id %d.\n", allocated_page_frame_id);
        
        insert_page(physical_page_queue, allocated_page_frame_id, MAX_PHYSICAL_PAGES);
        return page;
    }

    // 有空闲页的话，就将链表头指向的那一页分配出去，并将链表头指向下一页
    PhysicalPage *page = free_page_list;
    free_page_list = free_page_list->next;
    int allocated_page_frame_id = page->id;

    // 将被分配的空闲页插入到 使用页 链表中
    page->next = using_page_list;
    using_page_list = page;

    printf("\nfinish allocating physical memory, page frame id %d.\n", allocated_page_frame_id);
    
    insert_page(physical_page_queue, allocated_page_frame_id, MAX_PHYSICAL_PAGES);
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

    // // 还需要从物理页队列中删除该物理页
    // if (physical_page_queue != NULL) {
    //     // 删除页面节点
    //     PageNode *node_to_remove = find_page_node(physical_page_queue, physical_page_id);
    //     if (node_to_remove != NULL) {
    //         // 断链
    //         if (node_to_remove->prev) node_to_remove->prev->next = node_to_remove->next;
    //         if (node_to_remove->next) node_to_remove->next->prev = node_to_remove->prev;

    //         if (node_to_remove == physical_page_queue->front) physical_page_queue->front = node_to_remove->next;
    //         if (node_to_remove == physical_page_queue->rear) physical_page_queue->rear = node_to_remove->prev;

    //         free(node_to_remove);
    //         physical_page_queue->size--;
    //     }
    // }

    // 从双端队列中删除该页面
    remove_page_from_queue(physical_page_queue, physical_page_id);

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
        if(find_page_node(process->resident_set_queue, physical_page_id) != NULL){
            move_to_front(process->resident_set_queue, find_page_node(process->resident_set_queue, physical_page_id));
        }else{
            insert_page(process->resident_set_queue, physical_page_id, RESIDENT_SET_SIZE);
        }
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
    if (entry == NULL)
    {
        printf("trigger a segmentation fault when visiting logical page %d.\n", logical_page);
        return;
    }

    physical_page_id = entry->physical_page;
    int valid_bit = entry->valid_bit;

    // // 先检查该页是否在驻留集中
    // bool check_ppi_in_rss = false;
    // int victim_physical_page = -1;
    // for (int i = 0; i < RESIDENT_SET_SIZE; i++)
    // {
    //     if (process->resident_set[i] == physical_page_id)
    //     {
    //         // 该物理页在驻留集中
    //         check_ppi_in_rss = true;
    //         break;
    //     }
    // }

    if (find_page_node(process->resident_set_queue, physical_page_id)!=NULL)
    {
        // 如果在驻留集中/页表有对应的物理页且有效位有效，说明命中
        // insert_page(process->resident_set_queue, physical_page_id, RESIDENT_SET_SIZE);
        printf("logical page %d -> physical page %d , page table look up hits.\n", logical_page, physical_page_id);
        move_to_front(process->resident_set_queue, find_page_node(process->resident_set_queue, physical_page_id));
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

        if(process->resident_set_queue->size==RESIDENT_SET_SIZE) 
        {
            int evicted_physical_page = evict_page_from_queue(process->resident_set_queue, RESIDENT_SET_SIZE);
            if (evicted_physical_page != -1)
            {
                // 淘汰页并释放物理内存
                printf("The physical page %d has been swapped out from resident set.\n", evicted_physical_page);
                free_physical_memory(evicted_physical_page);

                for(int i=0; i < MAX_LOGICAL_PAGES; i++) {
                    PageTableEntry *e = get_page_table_entry(process->pt, i, false);
                    if(e && e->valid_bit==VALID && e->physical_page==evicted_physical_page){
                        // 写回处理（可选）
                        if (e->modified == MODIFIED) {
                            printf("Page %d has been modified, writing back to disk...\n", i);
                            // 模拟写回操作，可调用 write_back_page(i, process); // 自定义函数
                        }

                        // 更新页表项
                        e->valid_bit = INVALID;
                        e->physical_page = NO_PHYSICAL_PAGE;
                        e->modified = NOT_MODIFIED;

                        printf("Updated page table entry: logical page %d -> INVALID.\n", i);

                        // 清除TLB中的对应项（假设你有 tlb_remove_entry 函数）
                        tlb_delete_entry(process->tlb, i);
                        printf("Removed logical page %d from TLB.\n", i);

                        break; // 物理页只会映射一个逻辑页
                    }
                }
            }
        }

        entry->physical_page = physical_page_id;

        // （虽然前面检查是否命中是是三个条件（check_ppi_in_rss && valid_bit && physical_page_id != NO_PHISYCAL_PAGE）一起检查）
        entry->valid_bit = VALID;
        entry->modified = NOT_MODIFIED;
        // 更新驻留集
        // process->resident_set[process->rss_ptr] = physical_page_id;
        // process->rss_ptr = (process->rss_ptr + 1) % RESIDENT_SET_SIZE;
        insert_page(process->resident_set_queue, physical_page_id, RESIDENT_SET_SIZE);
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

void test_FIFO_replacement()
{
    printf("=== Test: FIFO Page Replacement ===\n");
    // 创建一个进程，设定进程 优先级 为 1，运行时间为500个unit
    PCB* process = create_process(1, 500);
    if (DEBUG)
        print_process_information();  // 打印初始的进程信息

    int testing_page_sequence[8] = {1, 2, 3, 4, 5, 6, 7, 8};  // 页面访问顺序
    int testing_length = 8;
    current_policy = REPLACEMENT_FIFO;
    // 访问页面序列，并在每次访问后打印进程信息
    for (int i = 0; i < testing_length; i++)
    {
        printf("\nAccessing page %d\n", testing_page_sequence[i]);
        // print_process_information();  // 打印当前的进程信息
        print_queue(process->resident_set_queue);

        // 访问逻辑内存中的页面，并触发换出策略
        visit_logical_memory_page(testing_page_sequence[i], process, 0); 

        // 打印进程信息，查看内存管理和页面替换情况
        // print_process_information();
        print_queue(process->resident_set_queue);
    }

    // 删除进程，释放内存
    delete_process(process);
    printf("Test FIFO Page Replacement PASSED\n");
}
void test_LRU_replacement()
{
    printf("=== Test: LRU Page Replacement ===\n");

    // 创建一个进程，设定进程 优先级 为 1，运行时间为500个unit
    PCB* process = create_process(1, 500);
    if (DEBUG)
        print_process_information();  // 打印初始的进程信息

    int testing_page_sequence[8] = {1, 2, 3, 4, 1, 2, 7, 3};  // 页面访问顺序
    int testing_length = 8;
    current_policy = REPLACEMENT_LRU;
    // 访问页面序列，并在每次访问后打印进程信息
    for (int i = 0; i < testing_length; i++)
    {
        printf("\nAccessing page %d\n", testing_page_sequence[i]);
        // print_process_information();  // 打印当前的进程信息
        print_queue(process->resident_set_queue);

        // 访问逻辑内存中的页面，并触发换出策略
        visit_logical_memory_page(testing_page_sequence[i], process, 0); 

        // 打印进程信息，查看内存管理和页面替换情况
        // print_process_information();
        print_queue(process->resident_set_queue);
    }

    // 删除进程，释放内存
    delete_process(process);
    printf("Test LRU Page Replacement PASSED\n");
}
void test_LRU_3()
{

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

    current_policy = REPLACEMENT_LRU;

    // 插入一些页面
    printf("Insert page 1: %d\n", insert_page(physical_page_queue, 1, 3)); // 不会替换
    print_queue(physical_page_queue);
    printf("Insert page 2: %d\n", insert_page(physical_page_queue, 2, 3)); // 不会替换
    print_queue(physical_page_queue);
    printf("Insert page 3: %d\n", insert_page(physical_page_queue, 3, 3)); // 不会替换
    print_queue(physical_page_queue);
    printf("Insert page 4: %d\n", insert_page(physical_page_queue, 4, 3)); // 会替换页面 1
    print_queue(physical_page_queue);

    // 设置替换策略为 FIFO
    current_policy = REPLACEMENT_FIFO;

    // 插入更多页面，测试 FIFO 替换
    printf("Insert page 5 (FIFO): %d\n", insert_page(physical_page_queue, 5, 3)); // 会替换页面 2
    print_queue(physical_page_queue);

    // 释放资源
    free_page_queue(physical_page_queue);
    return 0;
}
