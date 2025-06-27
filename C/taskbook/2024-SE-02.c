#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <err.h>

typedef struct {
    uint32_t magic;
    uint32_t packet_count;
    uint64_t org_size;
} Header;

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
        err(3, "Failed to read from file");
    }

    return r;
}

off_t wrapped_lseek(int fd, off_t offset, int whence) {
    int l = lseek(fd, offset, whence);

    if (l < 0) {
        err(5, "Failed to lseek %d", fd);
    }

    return l;
}

int wrapped_write(int fd, const void* buff, int size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(6, "Failed to write from file");
    }

    return w;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        errx(1, "2 args expected");
    }

    int fdComp = wrapped_open(argv[1], O_RDONLY, NULL);

    int access = S_IRUSR | S_IWUSR;
    int fdRes = wrapped_open(argv[2], O_CREAT | O_TRUNC | O_RDWR, &access);

    Header h;
    wrapped_read(fdComp, &h, sizeof(h));

    if (h.magic != 0x21494D46) {
        errx(4, "Invalid file magic(description)");
    }

    wrapped_lseek(fdComp, sizeof(Header), SEEK_SET); //skip the header

    for (uint32_t i = 0; i < h.packet_count; i++) {
        uint8_t firstByte;

        wrapped_read(fdComp, &firstByte, sizeof(firstByte));

        uint8_t mask = 1;
        if (firstByte & (mask << 7)) {
            uint8_t N = firstByte & ~(mask << 7);

            uint8_t nextByte;
            wrapped_read(fdComp, &nextByte, sizeof(nextByte));

            for (uint8_t j = 0; j < N + 1; j++) {
                wrapped_write(fdRes, &nextByte, sizeof(nextByte));
            }
        }
        else {
            uint8_t N = firstByte;

            for (uint8_t j = 0; j < N + 1; j++) {
                uint8_t nextByte;
                wrapped_read(fdComp, &nextByte, sizeof(nextByte));
                wrapped_write(fdRes, &nextByte, sizeof(nextByte));
            }
        }
    }

    close(fdComp);
    close(fdRes);
}
