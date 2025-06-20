#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <err.h>
#include <sys/stat.h>

int wrapped_open(const char* filename, int mode, int* access) {
    int fd;

    if (access) {
        fd = open(filename, mode, access);
    }
    else {
        fd = open(filename, mode);
    }

    if (fd < 0) {
        err(1, "Failed to open file %s", filename);
    }

    return fd;
}

int wrapped_read(int fd, void* buff, size_t size) {
    int r = read(fd, buff, size);

    if (r < 0) {
        err(2, "Failed to read from %d", fd);
    }

    return r;
}

int wrapped_write(int fd, const void* buff, size_t size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(3, "Failed to write to %d", fd);
    }

    return w;
}

int get_file_size(int fd) {
    struct stat st;

    if (fstat(fd, &st) < 0) {
        err(5, "Failed to fstat %d", fd);
    }

    return st.st_size;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        errx(4, "Invalid num of args");
    }

    int fd1 = wrapped_open(argv[1], O_RDONLY, NULL);

    int access = S_IRUSR | S_IWUSR;
    int fdRes = wrapped_open(argv[2], O_CREAT | O_TRUNC | O_RDWR, &access);

    if (get_file_size(fd1) % 2 != 0) {
        errx(6, "Invalid input file size"); //because 2 bytes from input make 1 byte in output;
    }

    uint16_t buff;
    while (wrapped_read(fd1, &buff, sizeof(buff)) > 0) {
        buff = (buff >> 8) | (buff << 8); //because of endianess we need to swap the 1st
                                          //and 2nd byte
        uint16_t mask = 1;
        uint8_t decoded = 0x00;

        for (int i = 0; i < 8; i++) {
            if (buff & (mask << (2 * i))) { //0
                //decoded |= 0 << i; this can be use but we dont need it(it does nothing)
                continue;
            }
            else {
                decoded |= 1 << i;
            }
        }

        wrapped_write(fdRes, &decoded, sizeof(decoded));
    }

    close(fd1);
    close(fdRes);
}
