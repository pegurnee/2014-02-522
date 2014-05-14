#include <stdio.h>
#include <sys/mman.h>

void main ()
{
 int x;
 int * ptr;

 ptr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);

 if (fork() == 0)
{       for (x=1; x < 5; x++)
      {
                printf("child: %d\n", (*ptr)++);
      sleep(1);
      }
        exit(0);
} 
 for (x=1; x < 5; x++)
{
        printf("parent: %d\n", (*ptr)++);
sleep(1);
}
}
