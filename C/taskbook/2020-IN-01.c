#include <stdint.h>
#include <fcntl.h>
#include <err.h>
#include <unistd.h>
#include <sys/stat.h>

typedef struct {
    uint32_t magic;
    uint8_t header_version;
    uint8_t data_version;
    uint16_t count;
    uint32_t rev1;
    uint32_t rev2;
} Header;

typedef struct {
    uint16_t offset;
    uint8_t org_byte;
    uint8_t new_byte;
} Data0;

typedef struct {
    uint32_t offset;
    uint16_t org_word;
    uint16_t new_word;
} Data1;

int wrapped_open(const char* filename, int mode, int* access) {
    int fd;
    if (access) {
        fd = open(filename, mode, *access);
    } else {
        fd = open(filename, mode);
    }
    if (fd < 0) {
        err(2, "Could not open file %s", filename);
    }
    return fd;
}

int wrapped_read(int fd, void* buff, int size) {
    int r = read(fd, buff, size);
    if (r < 0) {
        err(3, "Could not read from file %d", fd);
    }
    return r;
}

int wrapped_write(int fd, const void* buff, size_t size) {
    int w = write(fd, buff, size);
    if (w < 0) {
        err(7, "Could not write to file %d", fd);
    }
    return w;
}

int wrapped_lseek(int fd, off_t offset, int whence) {
    int ls = lseek(fd, offset, whence);
    if (ls < 0) {
        err(8, "Could not lseek");
    }
    return ls;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        errx(1, "Invalid num of args");
    }

    int fdP = wrapped_open(argv[1], O_RDONLY, NULL);
    int fd1 = wrapped_open(argv[2], O_RDONLY, NULL);
    int access = S_IRUSR | S_IWUSR;
    int fd2 = wrapped_open(argv[3], O_CREAT | O_TRUNC | O_RDWR, &access);

    Header header;
    wrapped_read(fdP, &header, sizeof(header));

    if (header.magic != 0xEFBEADDE) {
        errx(4, "Invalid magic");
    }

    if (header.header_version != 0x01) {
        errx(4, "Invalid header version");
    }

    struct stat st;
    if (fstat(fdP, &st) < 0) {
        err(5, "Could not fstat");
    }

    long expected_size = sizeof(Header);
    if (header.data_version == 0x00) {
        expected_size += header.count * sizeof(Data0);
    } else if (header.data_version == 0x01) {
        expected_size += header.count * sizeof(Data1);
    }

    if (st.st_size != expected_size) {
        errx(6, "Incorrect patch file size");
    }

    if (header.data_version == 0x00) {
        Data0 data;
        uint8_t currByte;

        while (wrapped_read(fd1, &currByte, sizeof(currByte)) == sizeof(currByte)) {
            wrapped_write(fd2, &currByte, sizeof(currByte));
        }

        while (wrapped_read(fdP, &data, sizeof(data)) == sizeof(data)) {
            wrapped_lseek(fd2, data.offset, SEEK_SET);
            wrapped_read(fd2, &currByte, sizeof(currByte));
            if (currByte == data.org_byte) {
                wrapped_lseek(fd2, data.offset, SEEK_SET);
                wrapped_write(fd2, &data.new_byte, sizeof(data.new_byte));
            } else {
                errx(8, "Bytes mismatch");
            }
        }
    } else if (header.data_version == 0x01) {
        Data1 data;
        uint16_t currWord;

        while (wrapped_read(fd1, &currWord, sizeof(currWord)) == sizeof(currWord)) {
            wrapped_write(fd2, &currWord, sizeof(currWord));
        }

        while (wrapped_read(fdP, &data, sizeof(data)) == sizeof(data)) {
            wrapped_lseek(fd2, data.offset * 2, SEEK_SET);
            wrapped_read(fd2, &currWord, sizeof(currWord));
            if (currWord == data.org_word) {
                wrapped_lseek(fd2, data.offset * 2, SEEK_SET);
                wrapped_write(fd2, &data.new_word, sizeof(data.new_word));
            } else {
                errx(9, "Words mismatch");
            }
        }
    } else {
        errx(10, "Invalid version");
    }

    close(fdP);
    close(fd1);
    close(fd2);
}
