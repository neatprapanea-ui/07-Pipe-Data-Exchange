#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    int n_bytes, pipefd[2];
    char buffer[BUFSIZ + 1], data[] = "12345";
    memset(buffer, '\0', sizeof(buffer));
    if (pipe(pipefd) == 0)
    {
        n_bytes = write(pipefd[1], data, strlen(data));
        printf("% d bytes have been written !\n", n_bytes);
        n_bytes = read(pipefd[0], buffer, BUFSIZ);
        printf("% d bytes have been read !\n", n_bytes);
        exit(EXIT_SUCCESS);
    }
    exit(EXIT_FAILURE);
}