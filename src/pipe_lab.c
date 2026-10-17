#include "pipe_lab.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

/*
 * Task A – one-way pipe
 *
 * The parent writes the string "ping" into a pipe.
 * The child reads it and prints exactly:
 *   child received: ping
 * The parent then waits for the child and prints exactly:
 *   parent: sent ping
 *
 * Required steps in the child:
 *   1. close fd[1] (unused write end)
 *   2. read the message into a buffer
 *   3. null-terminate the buffer and print "child received: <message>"
 *   4. close fd[0]
 *
 * Required steps in the parent:
 *   1. close fd[0] (unused read end)
 *   2. write the string "ping" into fd[1]
 *   3. close fd[1]
 *   4. waitpid() for the child
 *   5. print "parent: sent ping"
 */
int task_one_way(void) {
    char buffer[BUFSIZ + 1];
    int n_bytes;
    char data[] = "ping";

    int fd[2];
    if (pipe(fd) == -1) { perror("pipe"); return 1; }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        /* TODO: implement child logic (steps 1-4 above) */
        close (fd[1]);
        n_bytes = read(fd[0], buffer, BUFSIZ); //อ่านจำนวนไบต์
        if(n_bytes > 0)
        {
            buffer[n_bytes] = '\0'; //ปิดท้ายสตริง
            printf("child received: %s\n", buffer); //แสดงผล
            fflush(stdout);
        }
        close (fd[0]);
        exit(0);
    }

    /* TODO: implement parent logic (steps 1-5 above) */
    int stat_val;
    close (fd[0]);
    write(fd[1], data, strlen(data));
    close (fd[1]);
    waitpid(pid, &stat_val, 0);
    printf("parent: sent %s\n", data);
    fflush(stdout);

    return 0;
}

/*
 * Task B – two-way pipe
 *
 * Pipes are unidirectional, so bidirectional communication needs TWO pipes:
 *   p2c[2]: parent writes → child reads
 *   c2p[2]: child writes  → parent reads
 *
 * The parent sends the integer 21 through p2c.
 * The child reads it, doubles it to 42, and prints exactly:
 *   child computed: 42
 * The child sends the result back through c2p.
 * The parent reads the result and prints exactly:
 *   parent received: 42
 *
 * Required: close every pipe end that is not used in each process.
 * Hint: write/read a raw int with write(fd, &val, sizeof(val)).
 */
int task_two_way(void) {
    int p2c[2], c2p[2];
    if (pipe(p2c) == -1 || pipe(c2p) == -1) { perror("pipe"); return 1; }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        /* TODO: implement child logic */
        //child ใช้ p2c[0] อ่านจากแม่, c2p[1] ในการส่งให้แม่ ต้องปิดที่เหลือ
        close (p2c[1]);
        close (c2p[0]);
        int child_val = 0;
        int n_bytes = read(p2c[0], &child_val, sizeof(child_val));
        if(n_bytes > 0) //ไม่ใช้ != 0 เพราะป้องกันการเกิด error (-1)
        {
            child_val *= 2;
            printf("child computed: %d\n", child_val);
            fflush(stdout);
            write(c2p[1], &child_val, sizeof(child_val));
        }
        close (p2c[0]);
        close (c2p[1]);

        exit(0);
    }

    /* TODO: implement parent logic */
    int stat_val;
    int parent_val = 21; 
    int val_from_c = 0;

    //parent ใช้ p2c[1] ส่งให้ลูก และ c2p[0] รับค่าจากลูก
    close (p2c[0]);
    close (c2p[1]);

    write(p2c[1], &parent_val, sizeof(parent_val));
    close (p2c[1]);

    int n_bytes = read(c2p[0], &val_from_c, sizeof(val_from_c));
    if(n_bytes > 0)
    {
        printf("parent received: %d\n", val_from_c);
        fflush(stdout);
    }
    close (c2p[0]);
    waitpid(pid, &stat_val, 0);

    return 0;
}

/*
 * Task C – EOF limitation
 *
 * When every write end of a pipe is closed, the next read() returns 0.
 * This is how a reader detects end-of-stream (EOF).
 *
 * Sequence:
 *   parent: write "hello" (5 bytes) into the pipe, then CLOSE the write end
 *   child:  read in a loop until read() returns 0; track total bytes
 *           print exactly: child: got EOF after 5 bytes
 *
 * The loop must check the return value of read() on every iteration.
 * The child should NOT assume a single read delivers all the bytes.
 */
int task_eof(void) {
    int fd[2];
    char buffer [BUFSIZ + 1];
    int n_bytes;
    char text[] = "hello";
    int stat_val;

    if (pipe(fd) == -1) { perror("pipe"); return 1; }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        /* TODO: close fd[1], read in a loop until read() == 0,
                 print "child: got EOF after <n> bytes", close fd[0] */
        close (fd[1]);
        int total_bytes = 0;

        //อ่านคำว่า hello จบ loop มาอ่านอีกแต่ใน pipe ไม่มีข้อมูลเหลือให้อ่านแล้ว เลยออกจาก loop
        while((n_bytes = read(fd[0], buffer, BUFSIZ)) > 0)
        {
            total_bytes += n_bytes;
        }
        printf("child: got EOF after %d bytes\n", total_bytes);
        fflush(stdout);
        close (fd[0]);

        exit(0);
    }

    /* TODO: close fd[0], write "hello", close fd[1], waitpid */
    close (fd[0]);
    write(fd[1], text, strlen(text));
    close (fd[1]);
    waitpid(pid, &stat_val, 0);

    return 0;
}

/*
 * Task D – SIGPIPE / EPIPE limitation
 *
 * Writing to a pipe with no readers delivers SIGPIPE to the writer,
 * killing it unless the signal is ignored.  With SIGPIPE ignored,
 * write() returns -1 and sets errno to EPIPE.
 *
 * A synchronisation pipe (sync_fd) is used so the parent only writes
 * after the child has already closed the read end of data_fd.
 *
 * Required sequence:
 *   child:  close data_fd[0] and data_fd[1]
 *           write one byte to sync_fd[1] (ack), close sync_fd[1], exit
 *   parent: close sync_fd[1]
 *           read one byte from sync_fd[0] (blocks until child sends ack)
 *           close sync_fd[0] and data_fd[0]
 *           call signal(SIGPIPE, SIG_IGN)
 *           write one byte to data_fd[1]; check if errno == EPIPE
 *           print exactly: parent: broken pipe detected
 *           close data_fd[1], waitpid
 */
int task_broken_pipe(void) {
    int data_fd[2];
    int sync_fd[2];
    char ack = '1';

    if (pipe(data_fd) == -1 || pipe(sync_fd) == -1) { perror("pipe"); return 1; }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        /* TODO: implement child logic (see description above) */
        close (data_fd[0]);
        close (data_fd[1]);
        close (sync_fd[0]);

        write(sync_fd[1], &ack, 1);
        close (sync_fd[1]);
        exit(0);
    }

    /* TODO: implement parent logic (see description above) */
    int stat_val;
    close (sync_fd[1]);
    read(sync_fd[0], &ack, 1);
    close (sync_fd[0]);
    close (data_fd[0]);

    signal(SIGPIPE, SIG_IGN); //เมินสัญญาณ
    int ret = write(data_fd[1], "X", 1);
    if(ret == -1 && errno == EPIPE)
    {
        printf("parent: broken pipe detected\n");
        fflush(stdout);
    }
    close (data_fd[1]);

    waitpid(pid, &stat_val, 0);
    return 0;
}

/* ------------------------------------------------------------------*/
int main(void) {
    if (task_one_way() != 0)     { fprintf(stderr, "task_one_way failed\n");     return 1; }
    if (task_two_way() != 0)     { fprintf(stderr, "task_two_way failed\n");     return 1; }
    if (task_eof() != 0)         { fprintf(stderr, "task_eof failed\n");         return 1; }
    if (task_broken_pipe() != 0) { fprintf(stderr, "task_broken_pipe failed\n"); return 1; }
    printf("all tasks done\n");
    return 0;
}
