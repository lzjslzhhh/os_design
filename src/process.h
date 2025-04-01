#ifndef PROCESS_H
#define PROCESS_H

#include <stdbool.h>
#include "memory.h"  // 包含PageTable定义

// 进程状态常量
#define NEW 0
#define READY 1
#define RUNNING 2
#define WAITING 3
#define TERMINATED 4

// 系统配置常量
#define RESIDENT_SET_SIZE 4     // 每个进程的驻留集大小
#define TIME_SLICE 10           // 时间片大小为10
#define TIME_UNIT 1000          // 时间单位(微秒)

// 进程控制块(PCB)结构体
typedef struct pcb{
    int pid;                            // 进程的id
    int priority;                       // 进程优先级，值越小优先级越大
    int process_state;                  // 进程状态
    int time_need;                      // 进程目前所需的运行时间
    int time_used;                      // 进程目前已经运行的时间
    int rss_ptr;                        // 该进程的驻留集指针
    int resident_set[RESIDENT_SET_SIZE];// 记录驻留的物理页号，内部元素的数量等于页表中有效位为1的表项的数量
    PageTable *pt;                      // 该进程的页表
    PCB *next;                          // 仍然是用链表的形式对PCB进行管理
    PCB *qnext;                         // 记录在（RR/FCFS）队列中的下一个元素
}PCB;

//第一种管理方式，使用时间片轮转进行管理
typedef struct RRQueue{
    PCB *head;
    PCB *tail;
}RRQueue;

//第二种管理方式，使用抢占式先来先服务进行管理，这里需要用到priority
typedef struct FCFSQueue{
    PCB *head;
    PCB *tail;
}FCFSQueue;

// 全局变量声明
extern PCB *process_list;
extern int max_pid;
extern RRQueue *rrqueue;
extern FCFSQueue *fcfsqueue;

// 函数声明
void init_thread_manager(void);
void print_process_information(void);
PCB* create_process(int priority, int time);
void delete_process(PCB* process);
void enqueue_fcfsQ(PCB* process);
PCB* dequeue_fcfsQ(void);
void testing_task_1(void);
void testing_task_2(void);

#endif // PROCESS_H