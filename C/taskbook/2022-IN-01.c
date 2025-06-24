#include <fcntl.h>
#include <unistd.h>
#include <err.h>
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

int wrapped_read(int fd, void* buff, int size) {
    int r = read(fd, buff, size);

    if (r < 0) {
        err(3, "Failed to read from %d", fd);
    }

    return r;
}

int wrapped_write(int fd, const void* buff, int size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(4, "Failed to write to %d", fd);
    }

    return w;
}

off_t wrapped_lseek(int fd, off_t offset, int whence) {
    off_t l = lseek(fd, offset, whence);

    if (l < 0) {
        err(5, "Failed to lseek %d", fd);
    }

    return l;
}

typedef struct {
    uint16_t magic;
    uint16_t filetype;
    uint32_t count;
} Header;

int get_file_size(int fd) {
    struct stat st;
    if (fstat(fd, &st) < 0) {
        err(6, "Failed to fstat %d", fd);
    }

    return st.st_size;
}

int get_zero_count_in_data(int fd) {
    int count = 0;
    uint32_t byte;
    while (wrapped_read(fd, &byte, sizeof(byte)) > 0) {
        if (byte == 0) {
            count+=1;
        }
    }

    return count;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        errx(1, "Invalid num of args");
    }

    int fdL = wrapped_open(argv[1], O_RDONLY, NULL);
    int fdD = wrapped_open(argv[2], O_RDONLY, NULL);

    int access = S_IWUSR | S_IRUSR;

    int fdO = wrapped_open(argv[3], O_CREAT | O_TRUNC | O_RDWR, &access);

    Header headL;

    wrapped_read(fdL, &headL, sizeof(headL));

    if ((get_file_size(fdL) - sizeof(Header)) % sizeof(uint16_t) != 0 ||
                    (get_file_size(fdL) - sizeof(Header)) / sizeof(uint16_t) != headL.count) {
        err(7, "Invalid size of %s", argv[1]);
    }

    if (headL.magic != 0x5A4D) {
        err(8, "Invalid magic in %s", argv[1]);
    }

    if (headL.filetype != 1) {
        err(9, "Invalid filetype in %s", argv[1]);
    }

    Header headD;
    wrapped_read(fdD, &headD, sizeof(headD));
    if ((get_file_size(fdD) - sizeof(Header)) % sizeof(uint32_t) != 0 ||
            (get_file_size(fdD) - sizeof(Header)) / sizeof(uint32_t) != headD.count)  {
        err(7, "Invalid size of %s", argv[2]);
    }

    if (headD.magic != 0x5A4D) {
        err(8, "Invalid magic in %s", argv[2]);
    }

    if (headD.filetype != 2) {
        err(9, "Invalid filetype in %s", argv[2]);
    }

    Header headO;
    headO.magic = 0x5A4D;
    headO.filetype = 3;

    wrapped_lseek(fdL, sizeof(Header), SEEK_SET);

    uint16_t outOffset;
    uint32_t maxOffset = 0;
    //find the biggest pos in which we will write
    for (uint32_t i = 0; i < headL.count; i++) {
        wrapped_read(fdL, &outOffset, sizeof(outOffset));

        if (outOffset > maxOffset) {
            maxOffset = outOffset;
        }
    }

    headO.count = maxOffset + 1; //because the pos starts from 0
    wrapped_write(fdO, &headO, sizeof(headO));

    //set all vals on all pos to 0
    uint64_t zero = 0;
    for (uint32_t i = 0; i < headO.count; i++) {
        wrapped_write(fdO, &zero, sizeof(zero));
    }

    wrapped_lseek(fdL, sizeof(Header), SEEK_SET);
    for (uint32_t i = 0; i < headL.count; i++) {
        //find the pos on which we will need to write in out.bin
        wrapped_read(fdL, &outOffset, sizeof(outOffset));
        //go on the pos we want in data
        wrapped_lseek(fdD, sizeof(Header) + i * sizeof(uint32_t), SEEK_SET);

        //find the val we will need to write in out.bin
        uint32_t dataVal;
        wrapped_read(fdD, &dataVal, sizeof(dataVal));
        if (dataVal == 0) {
            continue;
        }

        // go on the pos we need to write in on out.bin
        wrapped_lseek(fdO, sizeof(Header) + outOffset * sizeof(uint64_t), SEEK_SET);
        uint64_t outData = dataVal;
        wrapped_write(fdO, &outData, sizeof(outData));
    }

    close(fdL);
    close(fdD);
    close(fdO);
}
