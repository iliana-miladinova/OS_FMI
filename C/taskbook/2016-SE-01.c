#include <fcntl.h>
#include <err.h>
#include <unistd.h>
#include <stdint.h>

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

int wrapped_read(int fd, void* buff, size_t size) {
    int r = read(fd, buff, size);

    if (r < 0) {
        err(3, "Could not read from file %d", fd);
    }

    return r;
}

int wrapped_write(int fd, const void* buff, size_t size) {
    int w = write(fd, buff, size);
    if (w < 0) {
        err(5, "Could not write to file %d", fd);
    }
    return w;
}

void bubble_sort(uint8_t* arr, int size) {
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                uint8_t temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        errx(1, "Invalid num of args");
    }

    int fd1 = open(argv[1], O_RDONLY, NULL);
    if (fd1 < 0) {
        err(2, "Could not open file for reading");
    }

    uint8_t sorted[4096];
    uint8_t byte;
    int count = 0;

    while (wrapped_read(fd1, &byte, sizeof(byte)) == sizeof(byte)) {
        sorted[count++] = byte;
    }

    close(fd1);

    bubble_sort(sorted, count);

    int fd2 = wrapped_open(argv[1], O_WRONLY | O_TRUNC, NULL);
    if (fd2 < 0) {
        err(4, "Could not open file for writing");
    }

    wrapped_write(fd2, sorted, count);

    close(fd2);
}
