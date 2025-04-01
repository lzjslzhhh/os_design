#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include"process.h"
#include"memory.h"

#define NEW 0
#define READY 1
#define RUNNING 2
#define WAITING 3
#define TERMINATED 4
#define RESIDENT_SET_SIZE 4     // 每个进程的驻留集大小
#define TIME_SLICE 10           // 时间片大小为10
#define TIME_UNIT 1000            // 统一使用usleep（单位为微秒），设定时间单位为1ms


PCB * process_list = NULL;              // 记录当前所有的进程

int max_pid = 1; //简单的实现pid赋值，实际上需要对pid进行管理，包括分配和回收

//定义时间片轮转循环队列
RRQueue *rrqueue;

FCFSQueue *fcfsqueue;

//后续可以有第三种管理方式，即融合RR和preemptive FCFS

void init_thread_manager(void)
{
    printf("start initializing thread manager.\n");
    rrqueue = (RRQueue *) malloc(sizeof(RRQueue));
    rrqueue -> head = NULL;
    rrqueue -> tail = NULL;

    fcfsqueue = (FCFSQueue *) malloc(sizeof(FCFSQueue));
    fcfsqueue -> head = NULL;
    fcfsqueue -> tail = NULL;
    printf("finish initializing thread manager.\n");
}

void print_process_information()
{
    //打印当前所有进程的PCB信息
    PCB * current_process = process_list;
    if (current_process == NULL)
    {
        printf("There is no process in BUPTscsOS.\n");
    }
    while(current_process != NULL)
    {
        printf("Process pid = %d, priority = %d, process state = %d, time need = %d, time used = %d. ", 
            current_process -> pid, current_process -> priority, current_process -> process_state , current_process -> time_need, current_process -> time_used);
        printf("Process resident set: [ ");
        for(int i=0;i<RESIDENT_SET_SIZE;i++)
        {
            printf("%d, ", current_process -> resident_set[i]);
        }
        printf("]\n\n");
        //printf("next pid = %d\n", current_process -> next -> pid);
        current_process = current_process -> next;
    }
}

PCB* create_process(int priority, int time)
{
    //首先创建新进程的PID
    //我考虑在这里做简化，新进程本身就是用PID标识的PCB，不包含其他的上下文等实际实现中会有的内容；
    PCB *new_process = (PCB *)malloc(sizeof(PCB));
    new_process -> pid = max_pid++;
    new_process -> priority = priority;
    new_process -> process_state = NEW;
    new_process -> time_need = time;
    new_process -> time_used = 0;
    new_process -> rss_ptr = 0;
    memset(new_process -> resident_set, -1, sizeof(new_process -> resident_set));
    new_process -> pt = create_page_table(new_process -> pid);
    new_process -> qnext = NULL;

    //将该进程插入到目前已创建的进程链表中，便于统一打印其状态
    new_process -> next = process_list;
    process_list = new_process;

    return new_process;
}

void delete_process(PCB* process)
{
    printf("process %d 's state changed into TERMINATED.\n", process -> pid);
    process -> process_state = TERMINATED;
    //首先从进程列表中找到删除该项
    PCB * current_process = process_list;
    if (current_process == NULL)
    {
        printf("No process in BUPTscsOS.\n");
        return;
    }

    if (current_process == process)
    {
        process_list = current_process -> next;
    }
    else
    {
        PCB *prev = NULL;
        while(current_process != NULL)
        {
            if(current_process != process)
            {
                prev = current_process;
                current_process = current_process -> next;
            }
            else
            {
                prev -> next = current_process -> next;
                break;
            }
        }
    }

    process -> next = NULL;

    //删除该项后，释放创建该进程所占用的内存，注意物理页的释放包含在了释放页表的函数里，不需要单独写
    free_page_table(process -> pid, process -> pt);
    printf("finish freeing process %d.\n", process -> pid);
    free(process);
}

void enqueue_fcfsQ(PCB* process)
{
    //将进程添加进FCFS队列
    //队列进程状态都为就绪态
    printf("process %d 's state changed into READY.\n", process -> pid);
    process -> process_state = READY;

    if (fcfsqueue -> tail == NULL)
    {
        //如果队列为空，那么头指针尾指针都指向该元素
        fcfsqueue -> head = process;
        fcfsqueue -> tail = process;
    }
    else
    {
        //否则队尾元素更新为要插入的元素
        fcfsqueue -> tail -> qnext = process;
        fcfsqueue -> tail = process;
    }
}


PCB * dequeue_fcfsQ()
{
    //获取当前队头的元素
    if(fcfsqueue -> head == NULL)
    {
        //当前队列为空，获取队头元素失败
        printf("empty FCFSQueue.\n");
        return NULL;
    }
    else
    {
        PCB * process = fcfsqueue -> head;
        fcfsqueue -> head = fcfsqueue -> head -> qnext;
        if(fcfsqueue -> head == NULL)
        {
            fcfsqueue -> tail = NULL;
        }
        return process;
    }
}


void testing_task_1()
{
    //这里先实现一个非常简单的FCFS，而且进程同时开始，只涉及进程调度，不涉及内存
    printf("now begin the testing task 1.\n");
    PCB* process_1 = create_process(1, 1000);
    PCB* process_2 = create_process(2, 1000);
    enqueue_fcfsQ(process_1);
    enqueue_fcfsQ(process_2);

    PCB * current_process = dequeue_fcfsQ();
    while(current_process != NULL)
    {
        print_process_information();
        printf("FCFS: now begin to run process %d.\n", current_process -> pid);
        //通过睡眠来模拟处理
        current_process -> process_state = RUNNING;
        printf("process %d 's state changed into RUNNING.\n", current_process -> pid);

        usleep((current_process -> time_need)*TIME_UNIT);
        current_process -> time_used = current_process -> time_need;
        current_process -> time_need = 0;

        printf("FCFS: now finish running process %d.\n", current_process -> pid);

        //执行完后删除释放该进程
        printf("process %d 's state changed into READY.\n", current_process -> pid);
        print_process_information();

        delete_process(current_process);

        print_process_information();
        current_process = dequeue_fcfsQ();

    }
}

void testing_task_2()
{
    //这里简单测试内存管理的内容
    printf("now begin the testing task 2.\n");
    PCB* process = create_process(1, 500);
    int testing_page_sequence[8] = {1,2,3,4,1,2,7};
    int testing_length = 8;
    for (int i=0;i<testing_length; i++)
    {
        print_process_information();
        visit_logical_memory_page(testing_page_sequence[i], process);
        print_process_information();
    }
    delete_process(process);
}