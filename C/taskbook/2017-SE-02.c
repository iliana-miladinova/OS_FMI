#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <string.h>
#include <stdio.h>

int wrapped_write(int fd, const void* buff, size_t size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(1, "Could not write to %d", fd);
    }

    return w;
}

int wrapped_read(int fd, void* buff, size_t size) {
    int r = read(fd, buff, size);

    if (r < 0) {
        err(2, "Could not read from %d", fd);
    }

    return r;
}

int wrapped_open(const char* filename, int mode, int* access) {
    int fd;

    if (access) {
        fd = open(filename, mode, *access);
    }
    else {
        fd = open(filename, mode);
    }

    return fd;
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        char buff[1024];
        int bytesCount = 0;
        while ((bytesCount = wrapped_read(0, &buff, sizeof(buff))) > 0) {
            wrapped_write(1, buff, bytesCount);
        }
    }
    else {
        if (strcmp("-n", argv[1]) == 0){
            int lineNum = 1;
            for (int i = 2; i < argc; i++) {
                int currFd;
                if (strcmp("-", argv[i]) == 0) {
                    currFd = 0;
                }
                else {
                    currFd = wrapped_open(argv[i], O_RDONLY, NULL);
                }

                char buff[1024];
                int bytesCount = 0;
                int prevIsNewline = 1;

                while ((bytesCount = wrapped_read(currFd, &buff, sizeof(buff))) > 0) {
                    for (int j = 0; j < bytesCount; j++) {
                        if (prevIsNewline) {
                            char lineNumStr[32];
                            int len = snprintf(lineNumStr, sizeof(lineNumStr), "%d ", lineNum++);
                            if (wrapped_write(1, lineNumStr, len) != len) {
                                err(4, "Could not write num of line");
                            }
                        }

                        if (wrapped_write(1, &buff[j], 1) != 1) {
                            err(5, "Could not write char");
                        }

                        if (buff[j] == '\n') {
                            prevIsNewline = 1;
                        }
                        else {
                            prevIsNewline = 0;
                        }
                    }
                }
            }
        }
        else {
            for (int i = 1; i < argc; i++) {
                int currFd;
                if (strcmp("-", argv[i]) == 0) {
                    currFd = 0;
                }
                else {
                    currFd = wrapped_open(argv[i], O_RDONLY, NULL);
                }

                char buff[1024];
                int bytesCount = 0;
                while ((bytesCount = wrapped_read(currFd, &buff, sizeof(buff))) > 0) {
                    wrapped_write(1, buff, bytesCount);
                }
            }
        }
    }
}
