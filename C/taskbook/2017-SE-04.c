#include <err.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char* argv[]) {
    if (argc == 1) {
        char ch;
        while (read(0, &ch, sizeof(ch)) == sizeof(ch)) {
            if (write(1, &ch, sizeof(ch)) < 0) {
                err(1, "Could not write to stdout");
            }

            if (ch == '\n') {
                break;
            }
        }
    } else {
        for (int i = 1; i < argc; i++) {
            int fd;
            if (argv[i][0] == '-') {
                fd = 0; 
            } else {
                fd = open(argv[i], O_RDONLY);
                if (fd < 0) {
                    err(2, "Could not open file %s for reading", argv[i]);
                }
            }

            if (fd == 0) {
                char ch;
                while (read(0, &ch, sizeof(ch)) == sizeof(ch)) {
                    if (write(1, &ch, sizeof(ch)) < 0) {
                        err(1, "Could not write to stdout");
                    }

                    if (ch == '\n') {
                        break;
                    }
                }
            } else {
                char buff[4096];
                int bytes_count;

                while ((bytes_count = read(fd, buff, sizeof(buff))) > 0) {
                    if (write(1, buff, bytes_count) != bytes_count) {
                        err(1, "Could not write to stdout");
                    }
                }

                if (bytes_count < 0) {
                    err(3, "Could not read from file");
                }

                close(fd);
            }
        }
    }

    return 0;
}
