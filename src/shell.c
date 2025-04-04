#include<stdio.h>
#include<string.h>
#include"process.h"
#include"memory.h"
#include"filesys.h"
void start_shell(void)
{
    char input[256];
    while(1)
    {
        //暂时先用scanf读取，后续再改进为fget
        scanf("%s", input);
        if(strcmp(input, "ls") == 0)
        {
            //list all the files in this content
        }
        else if(strcmp(input, "mem") == 0)
        {
            print_memory_information();
            //show memory status
        }
        else if(strcmp(input, "run1") == 0)
        {
            //运行测试任务1
            testing_task_1();
        }
        else if(strcmp(input, "run2") == 0)
        {
            testing_task_2();
        }
        else if(strcmp(input, "memtest1") == 0)
        {
            memory_testing_task_1();
        }
        else if(strcmp(input, "memtest2") == 0)
        {
            memory_testing_task_2();
        }
        else if(strcmp(input, "memtest3") == 0)
        {
            memory_testing_task_3();
        }
        else if(strcmp(input, "memtest4") == 0)
        {
            memory_testing_task_4();
        }
        else if(strcmp(input, "memtest5") == 0)
        {
            memory_testing_task_5();
        }
        else
        {
            printf("no such command:%s\n", input);
        }
    }
}