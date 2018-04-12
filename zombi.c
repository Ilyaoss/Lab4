#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid;
    int status;

    if ((pid = fork()) < 0) {
        perror("fork");
        exit(1);
    }

    /* Child */
    if (pid == 0)
        exit(0);

    //приостанавливаем родителький процесс
    sleep(100);

    //ждём ответ дитя
    pid = wait(&status);
    if (WIFEXITED(status))
        fprintf(stderr, "\n\t[%d]\tProcess %d exited with status %d.\n",
                (int) getpid(), pid, WEXITSTATUS(status));

    return 0;
}