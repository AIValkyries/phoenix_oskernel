#include <kernel/unistd.h>

static inline syscall0(int, exit);

void main()
{
    int b = 10;
    int c = 20;

    printf("hello world! user task2.............,b+c=%d \\n",(b+c));
}