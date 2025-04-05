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
        else if (strcmp(input, "run1") == 0)
        {
            // 运行测试任务
            memory_testing_task_1();
        }
        // 独立测试
        // TLB 测试用例
        // 1.基础功能测试
        else if (strcmp(input, "tlb1") == 0)
        {
            test_TLB_1_1(); // TLB 未满时插入页号
            test_TLB_1_2(); // TLB 满时随机置换
            test_TLB_1_3(); // 查询命中
            test_TLB_1_4(); // 查询未命中
        }
        // 2.性能与随机性测试
        else if (strcmp(input, "tlb2")==0)
        {
            test_TLB_2_1(); // 重复页号插入
            test_TLB_2_2(); // 随机置换均匀性验证
        }
        // 3.边界与异常测试
        else if (strcmp(input, "tlb3")==0)
        {
            test_TLB_3_1(); // 非法页号处理
            test_TLB_3_2(); // 满表重复页号
        }
        // // 联合测试
        // // 1.进程上下文切换与 TLB 测试
        // else if (strcmp(input, "test1")==0)
        // {
        //     test_1_1(); // 进程切换时TLB刷新验证
        //     test_1_2(); // ASID支持验证
        // }
        else if (strcmp(input, "exit") == 0)
        {
            printf("exit the shell.\n");
            break;
        }
        else
        {
            printf("no such command:%s\n", input);
        }
    }
}