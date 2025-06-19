#include <err.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

pid_t wrapped_fork(void) {
    pid_t child = fork();

    if (child < 0) {
        err(2, "Failed to fork");
    }

    return child;
}

int wrapped_open(const char* filename, int mode, int* access) {
    int fd;

    if (access) {
        fd = open(filename, mode, *access);
    }
    else {
        fd = open(filename, mode);
    }

    if (fd < 0) {
        err(7, "Failed to open file %s", filename);
    }

    return fd;
}

int wrapped_write(int fd, const void* buff, int size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(8, "Failed to write");
    }

    return w;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        errx(1, "At least 2 args expected");
    }

    char* endptr;
    int sec = strtol(argv[1], &endptr, 10);

    if (*endptr != '\0') {
        errx(4, "Invalid num format %s", argv[1]);
    }

    if (sec < 0 || sec > 9) {
        errx(5, "Arg1 must be a num between 0-9");
    }

    const char* progQ = argv[2];
    char** progArgs = argv + 2;

    int access = S_IRUSR | S_IWUSR;
    int fd = wrapped_open("run.log", O_CREAT | O_TRUNC, &access);
    close(fd);

    int failedCount = 0;
    while (true) {
        pid_t pr1 = wrapped_fork();

        if (pr1 == 0) {
            if (execvp(progQ, progArgs) < 0) {
                err(3, "Failed to exec");
            }
        }

        int status;

        time_t startTime = time(NULL);  //sled fork, no predi wait
        if (wait(&status) < 0) {
            err(6, "Failed to wait for child to finish");
        }

        time_t endTime = time(NULL); //sled wait

        int exitCode;
        if (WIFEXITED(status)) {
            exitCode = WEXITSTATUS(status);
        }
        else {
            exitCode = 129;
        }

        fd = wrapped_open("run.log", O_WRONLY | O_APPEND, NULL);

        char startTimeBuff[32];
        snprintf(startTimeBuff, sizeof(startTimeBuff), "%ld", startTime);
        wrapped_write(fd, startTimeBuff, strlen(startTimeBuff));
        wrapped_write(fd, " ", 1);

        char endTimeBuff[32];
        snprintf(endTimeBuff, sizeof(endTimeBuff), "%ld", endTime);
        wrapped_write(fd, endTimeBuff, strlen(endTimeBuff));
        wrapped_write(fd, " ", 1);

        char codeBuff[32];
        snprintf(codeBuff, sizeof(codeBuff), "%d", exitCode);
        wrapped_write(fd, codeBuff, strlen(codeBuff));
        wrapped_write(fd, "\n", 1);

        close(fd);

        time_t dur = endTime - startTime;

        if (exitCode != 0 && dur < sec) {
            failedCount++;
        }
        else {
            failedCount = 0;
        }

        if (failedCount == 2) {
            break;
        }
    }
}
