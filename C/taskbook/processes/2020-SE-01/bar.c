#include <unistd.h>
#include <fcntl.h>
#include <err.h>
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
        err(4, "Failed to duplicate %d to %d", oldfd, newfd);
    }

    return d;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        errx(3, "Invalid num of args");
    }

    pid_t pr1 = wrapped_fork();

    if (pr1 == 0) {
        int fd = wrapped_open("fooBarPipe", O_RDONLY, NULL);

        dup2(fd, 0);

        if (execl(argv[1], argv[1], (char*)NULL) < 0) {
            err(5, "Failed to exec");
        }
    }

    int status;
    if (wait(&status) < 0) {
        err(6, "Failed to wait for child to finish");
    }
    if (!WIFEXITED(status)) {
        err(7, "Child process exited abnormally");
    }
    if (WEXITSTATUS(status) != 0) {
        err(8, "Exited with code != 0");
    }

    if (unlink("fooBarPipe") < 0) { //"deletes the pipe"
        err(9, "Failed to unlink");
    }
}
