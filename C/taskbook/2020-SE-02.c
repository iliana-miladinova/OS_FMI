#include <err.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>

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

int wrapped_read(int fd, void* buff, size_t size) {
    int r = read(fd, buff, size);

    if (r < 0) {
        err(5, "Failed to read from %d", fd);
    }

    return r;
}

int wrapped_write(int fd, const void* buff, size_t size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(6, "Failed to write to %d", fd);
    }

    return w;
}

off_t wrapped_lseek(int fd, off_t offset, int whence) {
    off_t l = lseek(fd, offset, whence);

    if (l < 0) {
        err(7, "Failed to lseek %d", fd);
    }

    return l;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        errx(1, "Invalid num of args");
    }

    int fdScl = wrapped_open(argv[1], O_RDONLY, NULL);
    int fdSdl = wrapped_open(argv[2], O_RDONLY, NULL);

    int access = S_IRUSR | S_IWUSR;
    int fdRes = wrapped_open("res.txt", O_CREAT | O_TRUNC | O_RDWR, &access);

    struct stat st1;
    if (fstat(fdScl, &st1) < 0) {
        err(3, "Failed to stat %s", argv[1]);
    }

    struct stat st2;
    if (fstat(fdSdl, &st2) < 0) {
        err(3, "Failed to stat %s", argv[2]);
    }

    if ((st1.st_size * 8 * 2) != st2.st_size) { //*2 because st2.st_size returns the count of
                                                //bytes, not the count of elements
        errx(4, "The count of bits and the count of elements mismatch");
    }

    uint8_t byte;
    // int currPos = 0;
    while (wrapped_read(fdScl, &byte, sizeof(byte)) > 0) {
        for (int pos = 7; pos >= 0; pos--) {
            /*uint16_t el;
            wrapped_read(fdSdl, &el, sizeof(el));*/
            if ((byte & (1<<pos))) {
                uint16_t el;
                wrapped_read(fdSdl, &el, sizeof(el));
                wrapped_write(fdRes, &el, sizeof(el));
            }
            else {
                wrapped_lseek(fdSdl, sizeof(uint16_t), SEEK_CUR); //we should move the pointer
                                                                 //with the size equal to the
                                                             //size of the elements in the file
            }
            //currPos++;
        }
    }

    close(fdScl);
    close(fdSdl);
    close(fdRes);
}
