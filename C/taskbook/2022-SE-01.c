#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <err.h>

int wrapped_open(const char* filename, int mode, int* access) {
    int fd;
    if (access) {
        fd = open(filename, mode, *access);
    } else {
        fd = open(filename, mode);
    }

    if (fd < 0) {
        err(2, "Failed to open file %s", filename);
    }

    return fd;
}

int get_file_size(int fd) {
    struct stat st;
    if (fstat(fd, &st) < 0) {
        err(3, "Failed to fstat %d", fd);
    }
    return st.st_size;
}

typedef struct {
    uint32_t magic;
    uint32_t count;
} HeaderData;

typedef struct {
    uint32_t magic1;
    uint16_t magic2;
    uint16_t reserved;
    uint64_t count;
} HeaderComp;

typedef struct {
    uint16_t type;
    uint16_t res1;
    uint16_t res2;
    uint16_t res3;
    uint32_t offset1;
    uint32_t offset2;
} ComparatorData;

int wrapped_read(int fd, void* buff, int size) {
    int r = read(fd, buff, size);
    if (r < 0) {
        err(5, "Failed to read from %d", fd);
    }
    return r;
}

off_t wrapped_lseek(int fd, off_t offset, int whence) {
    off_t l = lseek(fd, offset, whence);
    if (l < 0) {
        err(8, "Failed to lseek %d", fd);
    }
    return l;
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
        errx(1, "Invalid num of args");
    }

    int fdD = wrapped_open(argv[1], O_RDWR, NULL);
    int fdC = wrapped_open(argv[2], O_RDONLY, NULL);

    int fileDataSize = get_file_size(fdD);
    int fileCompSize = get_file_size(fdC);

    HeaderData headD;
    if ((fileDataSize - sizeof(HeaderData)) % sizeof(uint64_t) != 0) {
        errx(4, "Invalid file size of %s", argv[1]);
    }

    HeaderComp headC;
    if ((fileCompSize - sizeof(HeaderComp)) % sizeof(ComparatorData) != 0) {
        errx(4, "Invalid file size of %s", argv[2]);
    }

    wrapped_read(fdD, &headD, sizeof(headD));

    if ((fileDataSize - sizeof(HeaderData)) / sizeof(uint64_t) != headD.count) {
        errx(6, "Invalid count of elements in %s", argv[1]);
    }

    if (headD.magic != 0x21796F4A) {
        errx(7, "Invalid magic in %s", argv[1]);
    }

    wrapped_read(fdC, &headC, sizeof(headC));
    if ((fileCompSize - sizeof(HeaderComp)) / sizeof(ComparatorData) != headC.count) {
        errx(6, "Invalid count of elements in %s", argv[2]);
    }

    if (headC.magic1 != 0xAFBC7A37 || headC.magic2 != 0x1C27) {
        errx(7, "Invalid magic in %s", argv[2]);
    }

    ComparatorData c;
    wrapped_lseek(fdC, sizeof(HeaderComp), SEEK_SET);

    while (wrapped_read(fdC, &c, sizeof(c)) == sizeof(c)) {
        if (c.type != 0 && c.type != 1) {
            errx(9, "Invalid comp type");
        }
        if (c.res1 != 0 || c.res2 != 0 || c.res3 != 0) {
            errx(10, "Invalid reserve in comp");
        }
        if (c.offset1 >= headD.count || c.offset2 >= headD.count) {
            errx(12, "Offset out of range");
        }

        uint64_t num1, num2;

        wrapped_lseek(fdD, sizeof(HeaderData) + c.offset1 * sizeof(uint64_t), SEEK_SET);
        wrapped_read(fdD, &num1, sizeof(num1));

        wrapped_lseek(fdD, sizeof(HeaderData) + c.offset2 * sizeof(uint64_t), SEEK_SET);
        wrapped_read(fdD, &num2, sizeof(num2));

        if ((c.type == 0 && num1 > num2) || (c.type == 1 && num1 < num2)) {
            wrapped_lseek(fdD, sizeof(HeaderData) + c.offset1 * sizeof(uint64_t), SEEK_SET);
            wrapped_write(fdD, &num2, sizeof(num2));

            wrapped_lseek(fdD, sizeof(HeaderData) + c.offset2 * sizeof(uint64_t), SEEK_SET);
            wrapped_write(fdD, &num1, sizeof(num1));
        }
    }

    close(fdD);
    close(fdC);
    return 0;
}
