#include <fcntl.h>
#include <err.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

bool isInSet(const char* set, char ch) {
    size_t len = strlen(set);

    for (size_t i = 0; i < len; i++) {
        if (set[i] == ch) {
            return true;
        }
    }
    return false;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        errx(1, "Invalid num of args");
    }

    if (strcmp(argv[1], "-d") == 0) {
        if (argc != 3) {
            errx(1, "3 args expected");
        }

        if (strlen(argv[2]) == 0) {
            errx(2, "Empty SET1");
        }

        char ch;
        int bytes_count;

        while ((bytes_count = read(0, &ch, sizeof(ch))) == sizeof(ch)) {
            if (!isInSet(argv[2], ch)) {
                if (write(1, &ch, sizeof(ch)) != sizeof(ch)) {
                    err(3, "Could not write to stdout");
                }
            }
        }

        if (bytes_count < 0) {
            err(4, "Could not read from stdin");
        }
    }
    else if (strcmp(argv[1], "-s") == 0) {
        if (argc != 3) {
            errx(1, "3 args expected");
        }

        if (strlen(argv[2]) != 1) {
            errx(2, "1 char expected");
        }

        char ch;
        int bytes_count;
        //bool seen = true;
        char lastChar = '\0';
        bool first = true;

        while ((bytes_count = read(0, &ch, sizeof(ch))) == sizeof(ch)) {
            // if (argv[2][0] != ch) {
            //    seen = false;
            //    if (write(1, &ch, sizeof(ch)) != sizeof(ch)) {
            //        err(5, "Could not write to stdout");
            //    }
            // }
            // if (seen) {
            //     continue;
            // }
            // if (ch == argv[2][0]) {
            //     seen = true;
            //     if (write(1, &ch, sizeof(ch)) != sizeof(ch)) {
            //         err(5, "Could not write to stdout");
            //     }
            // }

            if (!isInSet(argv[2], ch)) {
                if (write(1, &ch, sizeof(ch)) != sizeof(ch)) {
                    err(5, "Could not write to stdout");
                }
            }
            else {
                if (first || ch != lastChar) {
                    if (write(1 , &ch, sizeof(ch)) != sizeof(ch)) {
                        err(5, "Could not write to stdout");
                    }
                }
            }

            lastChar = ch;
            first = false;
        }

        if (bytes_count < 0) {
            err(4, "Could not read from stdin");
        }
    }
    else {
        if (strlen(argv[1]) != strlen(argv[2])) {
            errx(6, "SET1 and SET2 must have the same length");
        }

        size_t len = strlen(argv[1]);

        char ch;
        int bytes_count;

        while ((bytes_count = read(0, &ch, sizeof(ch))) == sizeof(ch)) {
            bool replaced = false;
            for (size_t i = 0; i < len; i++) {
                if (ch == argv[1][i]) {
                    if (write(1, &argv[2][i], sizeof(argv[2][i])) != sizeof(argv[2][i])) {
                        err(5, "Could not write to stdout");
                    }
                    replaced = true;
                    break;
                }
            }
            if (!replaced) {
                if (write(1, &ch, sizeof(ch)) != sizeof(ch)) {
                    err(5, "Could not write to stdout");
                }
            }
        }

        if (bytes_count < 0) {
            err(4, "Could not read from stdin");
        }
    }
}
