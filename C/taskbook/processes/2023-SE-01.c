#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

int wrapped_pipe(int pipefd[2]) {
    int p = pipe(pipefd);
    if (p < 0) {
        err(2, "Failed to pipe");
    }
    return p;
}

pid_t wrapped_fork(void) {
    pid_t child = fork();
    if (child < 0) {
        err(3, "Failed to fork");
    }
    return child;
}

int wrapped_dup2(int oldfd, int newfd) {
    int d = dup2(oldfd, newfd);
    if (d < 0) {
        err(4, "Failed to duplicate %d in %d", oldfd, newfd);
    }
    return d;
}

int wrapped_write(int fd, const void* buff, size_t size) {
    int w = write(fd, buff, size);
    if (w < 0) {
        err(6, "Failed to write to %d", fd);
    }
    return w;
}

int wrapped_read(int fd, void* buff, size_t size) {
    int r = read(fd, buff, size);
    if (r < 0) {
        err(7, "Failed to read from %d", fd);
    }
    return r;
}

int wrapped_open(const char* filename, int mode, int* access) {
    int fd;
    if (access) {
        fd = open(filename, mode, *access);
    } else {
        fd = open(filename, mode);
    }
    if (fd < 0) {
        err(8, "Failed to open file %s", filename);
    }
    return fd;
}

void get_hash(char* filename) {
    int md5Pipe[2];
    wrapped_pipe(md5Pipe);

    pid_t child = fork();
    if (child == 0) {
        close(md5Pipe[0]);
        wrapped_dup2(md5Pipe[1], 1);
        close(md5Pipe[1]);
        if (execlp("md5sum", "md5sum", filename, (char*)NULL) < 0) {
            err(5, "Failed to exec md5sum");
        }
    }

    close(md5Pipe[1]);

    char hashName[4096];
    strcpy(hashName, filename);
    strncat(hashName, ".hash", 4096 - strlen(hashName) - 1);

    int access = S_IRUSR | S_IWUSR;
    int fd = wrapped_open(hashName, O_CREAT | O_TRUNC | O_RDWR, &access);

    int r;
    char ch;
    while ((r = wrapped_read(md5Pipe[0], &ch, sizeof(ch))) > 0) {
        if (ch == ' ') {
            wrapped_write(fd, "\n", 1);
            break;
        }
        wrapped_write(fd, &ch, sizeof(ch));
    }

    close(md5Pipe[0]);
    close(fd);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        errx(1, "Invalid num of args");
    }

    int pipeFind[2];
    wrapped_pipe(pipeFind);

    pid_t childFind = fork();
    if (childFind == 0) {
        close(pipeFind[0]);
        wrapped_dup2(pipeFind[1], 1);
        close(pipeFind[1]);
        if (execlp("find", "find", argv[1], "-type", "f", "!", "-name", "*.hash", (char*)NULL) < 0) {
            err(5, "Failed to exec find");
        }
    }

    close(pipeFind[1]);

    char byte;
    char fileName[4096];
    int r;
    int ind = 0;
    int children = 0;

    while ((r = wrapped_read(pipeFind[0], &byte, sizeof(byte))) > 0) {
        fileName[ind] = byte;
        ind++;

        if (byte == '\n') {
            fileName[ind - 1] = '\0';

            pid_t childMd5 = wrapped_fork();
            if (childMd5 == 0) {
                close(pipeFind[0]);
                get_hash(fileName);
                exit(0);
            }

            ind = 0;
            children++;
        }
    }

    for (int i = 0; i < children; i++) {
        int status;
        if (wait(&status) < 0) {
            err(9, "Failed to wait for child to finish");
        }
        if (!WIFEXITED(status)) {
            err(10, "Child exited abnormally");
        }
        if (WEXITSTATUS(status) != 0) {
            err(11, "Exited with code != 0");
        }
    }

    return 0;
}
