#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <string.h>
#include <stdlib.h>

int wrapped_open(const char* filename, int mode, int* access) {
    int fd;
    if (access) {
        fd = open(filename, mode, access);
    } else {
        fd = open(filename, mode);
    }
    if (fd < 0) {
        err(2, "Failed to open file %s", filename);
    }
    return fd;
}

off_t wrapped_lseek(int fd, off_t offset, int whence) {
    off_t l = lseek(fd, offset, whence);
    if (l < 0) {
        err(4, "Failed to lseek");
    }
    return l;
}

int wrapped_read(int fd, void* buff, int size) {
    int r = read(fd, buff, size);
    if (r < 0) {
        err(5, "Failed to read from %d", fd);
    }
    return r;
}

int wrapped_write(int fd, const void* buff, int size) {
    int w = write(fd, buff, size);
    if (w < 0) {
        err(6, "Failed to write to %d", fd);
    }
    return w;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        errx(1, "Invalid num of args");
    }

    if (strlen(argv[1]) < 1 || strlen(argv[1]) > 63) {
        errx(3, "Invalid word len");
    }

    int fd1 = wrapped_open(argv[2], O_RDONLY, NULL);

    off_t beg = 0;
    off_t end = wrapped_lseek(fd1, 0, SEEK_END);

    while (beg < end) {
        off_t mid = beg + (end - beg) / 2;

        wrapped_lseek(fd1, mid, SEEK_SET); //we cant be sure that a new word starts here

        char buff;
        int r;
        //serch for the beginig symbol of a word
        while ((r = wrapped_read(fd1, &buff, sizeof(buff))) > 0) {
            if (buff == 0x00) {
                break;
            }
        }

        char buffer[64];
        int ind = 0;

        //read the word
        while ((r = wrapped_read(fd1, &buff, sizeof(buff))) > 0) {
            buffer[ind] = buff;
            ind++;

            if (buff == '\n') {
                break; //end of word
            }
        }

        buffer[ind - 1] = '\0';

        if (strcmp(argv[1], buffer) > 0) {
            beg = mid + 1;
        } else if (strcmp(argv[1], buffer) < 0) {
            end = mid -1;
        } else {
            while ((r = wrapped_read(fd1, &buff, sizeof(buff))) > 0) {
                if (buff == 0x00) { //beginig of the other word
                    break;
                }
                if (buff != '\n') {
                    wrapped_write(1, &buff, sizeof(buff));
                }
            }

            wrapped_write(1, "\n", 1);
            close(fd1);
            exit(0); //otherwise the loop will continue and will try to lseek a closed descriptor
        }
    }

    close(fd1);
    errx(7, "No such word - %s - in the dic", argv[1]);
}
