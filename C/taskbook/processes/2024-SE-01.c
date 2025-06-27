#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <sys/wait.h>
#include <stdint.h>
#include <stdlib.h>

int wrapped_open(const char* filename, int mode, int* access) {
    int fd;
    if (access) {
        fd = open(filename, mode, access);
    } else {
        fd = open(filename, mode);
    }

    if (fd < 0) {
        err(3, "Failed to open file %s", filename);
    }

    return fd;
}

int wrapped_read(int fd, void* buff, int size) {
    int r = read(fd, buff, size);

    if (r < 0) {
        err(4, "Failed to read from %d", fd);
    }

    return r;
}

int wrapped_write(int fd, const void* buff, int size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(5, "Failed to write to %d", fd);
    }

    return w;
}

int wrapped_pipe(int pipefd[2]) {
    int p = pipe(pipefd);

    if (p < 0) {
        err(6, "Failed to pipe");
    }

    return p;
}

pid_t wrapped_fork(void) {
    int child = fork();

    if (child < 0) {
        err(7, "Failed to fork");
    }

    return child;
}

int wrapped_dup2(int oldfd, int newfd) {
    int d = dup2(oldfd, newfd);

    if (d < 0) {
        err(8, "Failed to duplicate %d to %d", oldfd, newfd);
    }

    return d;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        errx(1, "3 args expected");
    }

    const char* prog = argv[1];
    char* endPtr;
    int N = strtol(argv[2], &endPtr, 10);
    if (*endPtr != '\0') {
        errx(2, "The second arg must be a num");
    }

    const char* resFile = argv[3];

    int access = S_IRUSR | S_IWUSR;
    int fdRes = wrapped_open(resFile, O_CREAT | O_TRUNC | O_RDWR, &access);
    int fdRan = wrapped_open("/dev/urandom", O_RDONLY, NULL);
    int fdNull = wrapped_open("/dev/null", O_WRONLY, NULL);

    for (int i = 0; i < N; i++) {
        int pipefd[2];

        wrapped_pipe(pipefd);

        uint16_t S;
        wrapped_read(fdRan, &S, sizeof(S));

        char buff[65536];

        wrapped_read(fdRan, buff, S);
        wrapped_write(pipefd[1], buff, S);

        pid_t child = wrapped_fork();

        if (child == 0) {
            close(pipefd[1]);

            wrapped_dup2(pipefd[0], 0); // we read the input for prog from the pipe instead from stdin
            close(pipefd[0]);
            wrapped_dup2(fdNull, 1);
            wrapped_dup2(fdNull, 2);

            close(fdNull);

            if (execlp(prog, prog, (const char*)NULL) < 0) {
                err(9, "Failed to exec %s", prog);
            }
        }

        close(pipefd[0]);
        close(pipefd[1]);

        int status;
        if (wait(&status) < 0) {
            err(10, "Failed to wait");
        }

        if (WIFSIGNALED(status)) {
            wrapped_write(fdRes, buff, S);
            exit(42);
        }

        if (WEXITSTATUS(status) == 9 && WIFEXITED(status)) {
            errx(10, "Child exited abnormally");
        }
    }

    close(fdNull);
    close(fdRes);
    exit(0);
}
