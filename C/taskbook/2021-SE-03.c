#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <sys/stat.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

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
        err(3, "Failed to read from %d", fd);
    }

    return r;
}

int wrapped_write(int fd, const void* buff, size_t size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(4, "Failed to write to %d", fd);
    }

    return w;
}

int get_file_size(int fd) {
    struct stat st;
    if (fstat(fd, &st) < 0) {
        err(5, "Failed to fstat");
    }

    return st.st_size;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        errx(1, "Invalid num of args");
    }

    /*uint16_t buff;

    const uint32_t MAX_SIZE = 524288 * sizeof(uint16_t);
    uint32_t arrN = 0;
    uint16_t arr[524288];*/

    int fd1 = wrapped_open(argv[1], O_RDONLY, NULL);

    int inputSize = get_file_size(fd1);

    if (inputSize % sizeof(uint16_t) != 0) {
        errx(6, "Invalid file size");
    }

    int elementsCount = inputSize / sizeof(uint16_t);

    if (elementsCount > 524288) {
        errx(7, "File contains more than 524288 elements");
    }

    int access = S_IWUSR | S_IRUSR;
    int fd2 = wrapped_open(argv[2], O_CREAT | O_TRUNC | O_RDWR, &access);

    const char* include = "#include <stdint.h>\n";
    wrapped_write(fd2, include, strlen(include));
    const char* buff = "const uint16_t arr[] = { ";
    wrapped_write(fd2, buff, strlen(buff));

    uint16_t byte;
    int isFirst = 1;
    while (wrapped_read(fd1, &byte, sizeof(byte)) > 0) {
        if(isFirst == 0) {
            wrapped_write(fd2, ", ", 2);
        }
        else {
            isFirst = 0;
        }
        char byteToSave[12];

        //snprintf(byteToSave, 12, "%d", byte);  in decimal num

        snprintf(byteToSave, 12, "0x%x", byte); //in hex
        wrapped_write(fd2, byteToSave, strlen(byteToSave));
    }

    wrapped_write(fd2, "}; \n", 4);
    const char* arrNTxt = "uint32_t arrN = ";
    wrapped_write(fd2, arrNTxt, strlen(arrNTxt));
    char N[15];
    snprintf(N, 15, "%d; \n", elementsCount);
    wrapped_write(fd2, N, strlen(N));

    close(fd1);
    close(fd2);
}
