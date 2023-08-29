#include <kernel/unistd.h>

// 5383ec0c 8d442418 50ff7424 18680030

static inline syscall0(int, pause);

void main()
{   
    printf("hello world! my_user_task.............");
}