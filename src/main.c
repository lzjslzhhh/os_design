#include<stdio.h>
#include<string.h>
int main(void)
{
    printf("start BUPTscsOS\n");
    init_memory();
    init_file_system();
    init_thread_manager();
    start_shell();
    return 0;
}