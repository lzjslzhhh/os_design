#include<stdio.h>
#include<string.h>
#include"filesys.h"
#include"memory.h"
#include"shell.h"
#include"thread.h"
int main(void)
{
    printf("start BUPTscsOS\n");
    kinit();//初始化内核内存
    // init_memory();

    init_file_system();
    init_thread_manager();
    start_shell();
    return 0;
}