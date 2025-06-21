#include <unistd.h>
#include <sys/wait.h>
#include <err.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    char name[8];
    uint32_t offset;
    uint32_t length;
} Entry;

int wrapped_open(const char* filename, int mode, int* access) {
    int fd;
    if (access) {
        fd = open(filename, mode, access);
    }
    else {
        fd = open(filename, mode);
    }

    if (fd < 0) {
        err(2, "Failed to open file %s", filename);
    }

    return fd;
}

off_t get_file_size(int fd) {
    struct stat st;

    if (fstat(fd, &st) < 0) {
        err(3, "Failed to stat %d", fd);
    }

    return st.st_size;
}

int wrapped_pipe(int pipefd[2]) {
    int p = pipe(pipefd);

    if (p < 0) {
        err(5, "Failed to pipe");
    }

    return p;
}

int wrapped_read(int fd, void* buff, size_t size) {
    int r = read(fd, buff, size);

    if (r < 0) {
        err(6, "Failed to read from %d", fd);
    }

    return r;
}

int wrapped_write(int fd, const void* buff, size_t size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(7, "Failed to write to %d", fd);
    }

    return w;
}

pid_t wrapped_fork(void) {
    pid_t child = fork();

    if (child < 0) {
        err(8, "Failed to fork a child");
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

int wrapped_lseek(int fd, off_t offset, int whence) {
    int l = lseek(fd, offset, whence);

    if (l < 0) {
        err(9, "Failed to lseek %d", fd);
    }

    return l;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        errx(1, "Invalid num of args");
    }

    int fd1 = wrapped_open(argv[1], O_RDONLY, NULL);

    off_t fileSize = get_file_size(fd1);

    if (fileSize % sizeof(Entry) != 0 || fileSize / sizeof(Entry) > 8) {
        errx(4, "Invalid file size for %s", argv[1]);
    }

    int childrenCount = 0;
    Entry entry;

    int pipefd[2];
    wrapped_pipe(pipefd);

    while (wrapped_read(fd1, &entry, sizeof(entry)) > 0) {
        pid_t child = wrapped_fork();
        childrenCount += 1;

        if (child == 0) {
            close(pipefd[0]);
            int fd2 = wrapped_open(entry.name, O_RDONLY, NULL);

            off_t fd2_size = get_file_size(fd2);

            if ((long unsigned int)fd2_size < (entry.offset + entry.length) * sizeof(uint16_t)) {
                errx(4, "Invalid file size for %s", entry.name);
            }

            wrapped_lseek(fd2, entry.offset * sizeof(uint16_t), SEEK_SET);

            uint16_t num;
            uint16_t res = 0x0000;

            for (uint32_t i = 0; i < entry.length; i++) {
                wrapped_read(fd2, &num, sizeof(num));
                res ^= num;
            }

            wrapped_write(pipefd[1], &res, sizeof(res));

            close(pipefd[1]);
            close(fd2);

            exit(0); //deteto prikluchva
        }
    }

    close(pipefd[1]);

    uint16_t numFromPipe;
    uint16_t finRes = 0;

    while (wrapped_read(pipefd[0], &numFromPipe, sizeof(numFromPipe)) > 0) {
        finRes ^= numFromPipe;
    }

    close(pipefd[0]);

    char buff[8];
    int len = snprintf(buff, sizeof(buff), "%04X\n", finRes);

    wrapped_write(1, "result: ", strlen("result: "));
    wrapped_write(1, buff, len);

    for (int i = 0 ; i < childrenCount; i++) {
        int status;
        if (wait(&status) < 0) {
            err(10, "Failed to wait");
        }
        if (!WIFEXITED(status)) {
            err(11, "Child exited abnormally");
        }
        if(WEXITSTATUS(status) != 0) {
            err(12, "Child exited with code != 0");
        }
    }

    close(fd1);
    exit(0);
}
