#include <stdint.h>
#include <err.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

typedef struct {
    uint32_t res1;
    uint16_t count;
    uint64_t res2;
    uint16_t res3;
} Header;

typedef struct {
    uint16_t postStart;
    uint16_t postCount;

    uint16_t preStart;
    uint16_t preCount;

    uint16_t inStart;
    uint16_t inCount;

    uint16_t sufStart;
    uint16_t sufCount;
} Complect;

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

int wrapped_write(int fd, const void* buff, int size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(3, "Failed to write to %d", fd);
    }

    return w;
}

int wrapped_read(int fd, void* buff, int size) {
    int r = read(fd, buff, size);

    if (r < 0) {
        err(4, "Failed to read from %d", fd);
    }

    return r;
}

void checkFileSize(int fd, int elSize) {
    Header h;

    struct stat st;

    wrapped_read(fd, &h, sizeof(h));

    if (fstat(fd, &st) < 0) {
        err(5, "Failed to fstat");
    }

    off_t fileSize = st.st_size;

    if ((fileSize - 16) % elSize != 0) {
        err(6, "Invalid file contend");
    }

    if ((fileSize - 16) / elSize != h.count) {
        err(7, "Invalid el count in %d", fd);
    }
}

off_t wrapped_lseek(int fd, off_t offset, int whence) {
    off_t l = lseek(fd, offset, whence);

    if (l < 0) {
        err(8, "Failed to lseek");
    }

    return l;
}

int main(int argc, char* argv[]) {
    if (argc != 7) {
        errx(1, "Invalid num of args");
    }

    int fdA = wrapped_open(argv[1], O_RDONLY, NULL);
    int fdPost = wrapped_open(argv[2], O_RDONLY, NULL);
    int fdPre = wrapped_open(argv[3], O_RDONLY, NULL);
    int fdInf = wrapped_open(argv[4], O_RDONLY, NULL);
    int fdSuf = wrapped_open(argv[5], O_RDONLY, NULL);

    int access = S_IRUSR | S_IWUSR;

    int fdCr = wrapped_open(argv[6], O_CREAT | O_TRUNC | O_RDWR, &access);

    checkFileSize(fdA, 2);
    wrapped_lseek(fdA, 16, SEEK_SET);
    checkFileSize(fdPost, 4);
    //wrapped_lseek(fdPost, 16, SEEK_SET);
    checkFileSize(fdPre, 1);
    //wrapped_lseek(fdPre, 16, SEEK_SET);
    checkFileSize(fdInf, 2);
    //wrapped_lseek(fdInf, 16, SEEK_SET);
    checkFileSize(fdSuf, 8);
    //wrapped_lseek(fdSuf, 16, SEEK_SET);

    Complect c;
    while(wrapped_read(fdA, &c, sizeof(c)) == sizeof(c)) {

        wrapped_lseek(fdPost, 16 + c.postStart * sizeof(uint32_t), SEEK_SET); //16 + to skip the header 
        for (int i = 0; i < c.postCount; i++) {
            uint32_t el;
            wrapped_read(fdPost, &el, sizeof(el));
            wrapped_write(fdCr, &el, sizeof(el));
        }

        wrapped_lseek(fdPre, 16 + c.preStart * sizeof(uint8_t), SEEK_SET);
        for (int i = 0; i < c.preCount; i++) {
            uint8_t el;
            wrapped_read(fdPre, &el, sizeof(el));
            wrapped_write(fdCr, &el, sizeof(el));
        }

        wrapped_lseek(fdInf, 16 + c.inStart * sizeof(uint16_t), SEEK_SET);
        for (int i = 0; i < c.inCount; i++) {
            uint16_t el;
            wrapped_read(fdInf, &el, sizeof(el));
            wrapped_write(fdCr, &el, sizeof(el));
        }

        wrapped_lseek(fdSuf, 16 + c.sufStart * sizeof(uint64_t), SEEK_SET);
        for (int i = 0; i < c.sufCount; i++) {
            uint64_t el;
            wrapped_read(fdSuf, &el, sizeof(el));
            wrapped_write(fdCr, &el, sizeof(el));
        }
    }

    close(fdA);
    close(fdPost);
    close(fdPre);
    close(fdInf);
    close(fdSuf);
    close(fdCr);

    return 0;
}
