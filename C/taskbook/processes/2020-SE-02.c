#include <err.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdint.h>

int wrapped_pipe(int pipefd[2]) {
    int p = pipe(pipefd);

    if (p < 0) {
        err(1, "Failed to pipe");
    }

    return p;
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
        err(3, "Failed to duplicate %d in %d", oldfd, newfd);
    }

    return d;
}

int wrapped_open(const char* filename, int mode, int* access) {
    int fd;

    if (access) {
        fd = open(filename, mode, access);
    }
    else {
        fd = open(filename, mode);
    }

    if (fd < 0) {
        err(9, "Failed to open file %s", filename);
    }

    return fd;
}

int wrapped_read(int fd, void* buff, int size) {
    int r = read(fd, buff, size);

    if (r < 0) {
        err(10, "Failed to read from %d", fd);
    }

    return r;
}

int wrapped_write(int fd, const void* buff, int size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(11, "Failed to write to %d", fd);
    }

    return w;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        errx(4, "Invalid num of args");
    }

    int catToProg[2];
    wrapped_pipe(catToProg);

    pid_t pr1 = wrapped_fork();

    if (pr1 == 0) {
        close(catToProg[0]);

        wrapped_dup2(catToProg[1], 1);
        close(catToProg[1]);

        if (execlp("cat", "cat", argv[1], (char*)NULL) < 0) {
            err(5, "Failed to exec");
        }
    }

    close(catToProg[1]);

    int status;
    if (wait(&status) < 0) {
        err(7, "Failed to wait for child to finish");
    }
    if (!WIFEXITED(status)) {
        err(8, "Child exited abnormally");
    }
    if (WEXITSTATUS(status) != 0) {
        err(9, "Child exited with code != 0");
    }

    int access = S_IRUSR | S_IWUSR;
    int fd = wrapped_open(argv[2], O_CREAT | O_TRUNC | O_WRONLY, &access);
    uint8_t byte;
    while (wrapped_read(catToProg[0], &byte, sizeof(byte)) == sizeof(byte)) {
        if (byte == 0x55) {
            continue;
        }
        else if (byte == 0x7D) {
            uint8_t next;
            wrapped_read(catToProg[0], &next, sizeof(next));

            uint8_t decoded = next ^ 0x20;

            if (decoded == 0x00 || decoded == 0xFF || decoded == 0x55 || decoded == 0x7D) {
                wrapped_write(fd, &decoded, sizeof(decoded));
            }
            else {
                err(11, "Invalid ndecoded byte");
            }
        }
        else {
            wrapped_write(fd, &byte, sizeof(byte));
        }
    }

    close(fd);
    close(catToProg[0]);
}
