#include<stdio.h>
#include<string.h>
void start_shell(void)
{
    char input[256];
    while(1)
    {
        scanf("%s", input);
        if(strcmp(input, "ls") == 0)
        {
            //list all the files in this content
        }
        else if(strcmp(input, "mem") == 0)
        {
            //show memory status
        }
        printf("%s\n", input);
    }
}