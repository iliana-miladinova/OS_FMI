#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <err.h>
#include <sys/stat.h>
#include <stdlib.h>

typedef struct {
    uint64_t next;
    char data[504];
} Node;

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

off_t get_file_size(int fd) {
    struct stat st;

    if (fstat(fd, &st) < 0) {
        err(3, "Failed to fstat %d", fd);
    }

    return st.st_size;
}

void wrapped_unlink(const char* filename) {
    if (unlink(filename) < 0) {
        err(6, "Failed to unlink %s", filename);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        errx(1, "Invalid num of args");
    }

    int fd1 = wrapped_open(argv[1], O_RDONLY, NULL);

    off_t inpSize = get_file_size(fd1);

    if (inpSize % sizeof(Node) != 0) {
        errx(4, "Invalid file size %s", argv[1]);
    }

    char temp_path[] = "/tmp/my_tempXXXXXX";

    int fdTemp = mkstemp(temp_path);
    if (fdTemp < 0) {
        err(5, "Failed to open temp file");
    }

    char empty[sizeof(Node)];

    for (long unsigned int i = 0; i < sizeof(Node); i++) {
        empty[i] = 0;
    }

    int nodes_count = inpSize / sizeof(Node);

    for (int i = 0; i < nodes_count; i++) {
        if (write(fdTemp, &empty, sizeof(empty)) < 0) {
            wrapped_unlink(temp_path);
            err(7, "Failed to write to temp file");
        }
    }

    if (lseek(fdTemp, 0, SEEK_SET) < 0) {
        wrapped_unlink(temp_path);
        err(8, "Failed to lseek temp file");
    }

    Node n;
    n.next = 0;

    do {
        int r = read(fd1, &n, sizeof(n));

        if (r != sizeof(Node)) {
            wrapped_unlink(temp_path);
            errx(9, "Error reading from %s, invalid node", argv[1]);
        }

        if (write(fdTemp, &n, sizeof(n)) < 0) {
            wrapped_unlink(temp_path);
            err(10, "Failed to write to temp file");
        }

        if (lseek(fd1, n.next * sizeof(Node), SEEK_SET) < 0) {
            wrapped_unlink(temp_path);
            err(11, "Failed to lseek original file");
        }

        if (lseek(fdTemp, n.next * sizeof(Node), SEEK_SET) < 0) { // so next time it writes in the pos
                                                                  // in which is the next node in the org
            wrapped_unlink(temp_path);
            err(11, "Failed to lseek temp file");
        }
    } while(n.next); // ako ima samo edin vazel cikalat shte se izpalni vednaj

    close(fd1);

    if (lseek(fdTemp, 0, SEEK_SET) < 0) { // so it is set in the begining when we start rewriting org
        wrapped_unlink(temp_path);
        err(11, "Failed to lseek temp file");
    }

    int fdReWrite = open(argv[1], O_TRUNC | O_WRONLY);

    if (fdReWrite < 0) {
        wrapped_unlink(temp_path);
        err(12, "Failed to open file %s", argv[1]);
    }

    int r;

    while ((r = read(fdTemp, &n, sizeof(n))) > 0) {
        if (write(fdReWrite, &n, sizeof(n)) < 0) {
            wrapped_unlink(temp_path);
            err(13, "Failed to write to %s", argv[1]);
        }
    }

    if (r < 0) {
        wrapped_unlink(temp_path);
        err(14, "Failed to read from temp file");
    }

    wrapped_unlink(temp_path);
    close(fdReWrite);
}
