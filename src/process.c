#include<stdio.h>
#include"process.h"
#include"memory.h"

#define NEW 0
#define READY 1
#define RUNNING 2
#define WAITING 3
#define TERMINATED 4

typedef struct pcb{
    int pid;            //进程的id
    int priority;       //进程优先级
    int process_state;  // 进程状态
    int time_need;      //进程目前所需的运行时间
    int time_used;      //进程目前已经运行的时间
    PCB * next;         //仍然是用链表的形式对PCB进行管理
}PCB;

int max_pid = 1; //简单的实现pid赋值，实际上需要管理

//第一种管理方式，使用时间片轮转进行管理
typedef struct RRQueue{
    PCB *head;
    PCB *tail;
}rrqueue;

//第二种管理方式，使用抢占式先来先服务进行管理，这里需要用到priority
typdef FCFSQueue{
    PCB *head;
    PCB *tail;
}fcfsqueue;

//后续可以有第三种管理方式，即融合RR和preemptive FCFS

void init_thread_manager(void)
{
    printf("start initializing thread manager.\n");

    printf("finish initializing thread manager.\n");
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
}