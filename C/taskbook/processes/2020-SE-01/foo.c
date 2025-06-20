#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

int wrapped_open(const char* filename, int mode, int* access) {
    int fd;
    if (access) {
        fd = open(filename, mode, access);
    }
    else {
        fd = open(filename, mode);
    }

    if (fd < 0) {
        err(1, "Failed to open file %s", filename);
    }
    return fd;
}

pid_t wrapped_fork(void) {
    pid_t child = fork();

    if (child < 0) {
        err(2, "Failed to fork");
    }

    return child;
}

int wrapped_dup2(int oldfd, int newfd) {
    int d = dup2(oldfd, newfd);

    if (d < 0) {
        err(5, "Failed to duplicate %d to %d", oldfd, newfd);
    }

    return d;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        errx(3, "Invalid num of args");
    }

    if (mkfifo("fooBarPipe", 0666) < 0) {
        err(4, "Failed to make named pipe");
    }

    pid_t pr1 = wrapped_fork();

    if (pr1 == 0) {
        int fd = wrapped_open("fooBarPipe", O_WRONLY, NULL);

        wrapped_dup2(fd, 1); //izhoda ot cat shte se zapishe v trabata, ne na stdout

        if (execlp("cat", "cat", argv[1], (char*)NULL) < 0) {
            err(6, "Failed to exec");
        }
    }

    int status;
    if (wait(&status) < 0) {
        err(7, "Failed to wait for child to finish");
    }
    if (!WIFEXITED(status)) {
        err(8, "Child process exited abnormally");
    }
    if (WEXITSTATUS(status) != 0) {
        err(9, "Exited with status != 0");
    }
}
