#include<stdio.h>
void start_shell(void)
{
    char input[256];
    while(1)
    {
        scanf("%s", input);
        printf("%s\n", input);
    }
}