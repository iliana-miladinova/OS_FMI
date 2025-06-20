#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <stdint.h>

int wrapped_open(const char* filename, int mode, int* access) {
    int fd;

    if(access) {
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
        err(3, "Failed to read from %d", fd);
    }

    return fd;
}

int wrapped_write(int fd, const void* buff, size_t size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(4, "Failed to write to %d", fd);
    }

    return w;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        err(1, "Invalid num of args");
    }

    int fd1 = wrapped_open(argv[1], O_RDONLY, NULL);

    int access = S_IRUSR | S_IWUSR;
    int fdRes = wrapped_open(argv[2], O_CREAT | O_TRUNC | O_RDWR, &access);

    uint8_t buff;
    while (wrapped_read(fd1, &buff, sizeof(buff)) > 0) {
        uint8_t mask = 1;
        uint16_t encoded = 0x0000; //0 in uint16_t

        for (int i = 0; i < 8; i++) {
            if (buff & (mask << i)) {
                encoded |= 2 << (2 * i);
            }
            else {
                encoded |= 1 << (2 * i);
            }
        }

        // !!!! Endianess
        uint8_t* encoded_ptr = (uint8_t*) &encoded; //so we can access the first and second byte
                                                    //separatly
        wrapped_write(fdRes, encoded_ptr + 1, sizeof(uint8_t)); //write the 2nd byte first;
        wrapped_write(fdRes, encoded_ptr, sizeof(uint8_t));
    }

    close(fd1);
    close(fdRes);
}
