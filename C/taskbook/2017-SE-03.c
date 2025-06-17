#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>

typedef struct {
    uint16_t offset;
    uint8_t orgByte;
    uint8_t newByte;
} patch;

int main(int argc, char* argv[]) {
    if (argc != 4) {
        errx(1, "Invalid number of args");
    }

    int fdP = open(argv[1], O_RDONLY);
    if (fdP < 0) {
        err(2, "Could not open file %s for reading", argv[1]);
    }

    int fd1 = open(argv[2], O_RDONLY);
    if (fd1 < 0) {
        err(2, "Could not open file %s for reading", argv[2]);
    }

    int fd2 = open(argv[3], O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd2 < 0) {
        err(2, "Could not open file %s for writing", argv[3]);
    }

    patch p;
    int bytes_count;
    uint16_t lastInd = 0;

    while ((bytes_count = read(fdP, &p, sizeof(p))) == sizeof(p)) {
        if (lseek(fd1, lastInd, SEEK_SET) < 0) {
            err(3, "Could not lseek file");
        }

        for (uint16_t i = lastInd; i < p.offset; i++) {
            uint8_t currByte;

            if (read(fd1, &currByte, sizeof(currByte)) != sizeof(currByte)) {
                err(4, "Could not read from file %s", argv[2]);
            }

            if (write(fd2, &currByte, sizeof(currByte)) != sizeof(currByte)) {
                err(5, "Could not write to file %s", argv[3]);
            }
        }

        if (lseek(fd1, p.offset, SEEK_SET) == -1) {
            err(3, "Could not lseek file %s", argv[2]);
        }

        uint8_t fd1Byte;

        if (read(fd1, &fd1Byte, sizeof(fd1Byte)) != sizeof(fd1Byte)) {
            err(4, "Could not read from file %s", argv[2]);
        }

        if (fd1Byte != p.orgByte) {
            errx(6, "The bytes are different");
        }

        if (write(fd2, &p.newByte, sizeof(p.newByte)) != sizeof(p.newByte)) {
            err(5, "Could not write to file %s", argv[3]);
        }

        lastInd = p.offset + 1;
    }

    if (bytes_count < 0) {
        err(7, "Could not read from file %s", argv[1]);
    }

    uint8_t buff[4096];
    int bytes_read;

    if (lseek(fd1, lastInd, SEEK_SET) < 0) {
        err(3, "Could not lseek file %s", argv[2]);
    }

    while ((bytes_read = read(fd1, &buff, sizeof(buff))) > 0) {
        if (write(fd2, &buff, bytes_read) != bytes_read) {
            err(5, "Could not write to file %s", argv[3]);
        }
    }

    if (bytes_read < 0) {
        err(4, "Could not read from file %s", argv[2]);
    }

    close(fdP);
    close(fd1);
    close(fd2);
}
