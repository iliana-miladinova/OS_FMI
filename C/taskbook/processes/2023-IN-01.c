#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int wrapped_pipe(int pipefd[2]) {
    int p = pipe(pipefd);
    if (p < 0) {
        err(5, "Failed to pipe");
    }
    return p;
}

pid_t wrapped_fork(void) {
    pid_t child = fork();
    if (child < 0) {
        err(6, "Failed to fork");
    }
    return child;
}

int wrapped_write(int fd, const void* buff, size_t size) {
    int w = write(fd, buff, size);
    if (w < 0) {
        err(7, "Failed to write to %d", fd);
    }
    return w;
}

int wrapped_read(int fd, void* buff, size_t size) {
    int r = read(fd, buff, size);
    if (r < 0) {
        err(8, "Failed to read from %d", fd);
    }
    return r;
}

int pipes[8][2]; //max 8 pipes because the max num of childeren is 7 + 1 for the parent
const char* words[3] = {"tic ", "tac ", "toe\n"};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        errx(1, "Invalid num of params");
    }

    char* ncEnd;
    int nc = strtol(argv[1], &ncEnd, 10);
    if (*ncEnd != '\0') {
        errx(2, "Invalid nc");
    }

    char* wcEnd;
    int wc = strtol(argv[2], &wcEnd, 10);
    if (*wcEnd != '\0') {
        errx(3, "Invalid wc");
    }

    if (nc < 1 || nc > 7) {
        errx(4, "Invalid num of children");
    }

    if (wc < 1 || wc > 35) {
        errx(4, "Invalid num of words");
    }

    for (int i = 0; i <= nc; i++) {
        wrapped_pipe(pipes[i]);
    }

    int readingEnd = 0;
    int writingEnd = 0;

    for (int i = 1; i <= nc; i++) {
        pid_t child = wrapped_fork();

        if (child == 0) {
            for (int j = 0; j <= nc; j++) {
                // the process writes in pipes[i][1]; and reads from pipes[i-1][0]
                if (j == i) {
                    close(pipes[j][0]);
                    writingEnd = pipes[j][1];
                } else if (j == i - 1) {
                    close(pipes[j][1]);
                    readingEnd = pipes[j][0];
                } else { // we don't need the other pipes
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
            }

            int r;
            uint8_t ind;

            while ((r = wrapped_read(readingEnd, &ind, sizeof(ind))) > 0) {
                if (ind > wc) {
                    // we have received end signal => we forward it and then break
                    wrapped_write(writingEnd, &ind, sizeof(ind));
                    break;
                } else {
                    wrapped_write(1, words[ind % 3], strlen(words[ind % 3]));
                    ind++;
                    wrapped_write(writingEnd, &ind, sizeof(ind)); // send the ind to the next process
                }
            }

            close(readingEnd);
            close(writingEnd);
            exit(0);
        }
    }

    // parent
    // close the ends we won't use
    close(pipes[0][0]);
    close(pipes[nc][1]);

    for (int i = 1; i < nc; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // read from the last pipe and write in the first
    readingEnd = pipes[nc][0];
    writingEnd = pipes[0][1];

    // start the loop by printing the first word
    uint8_t ind = 1;
    wrapped_write(1, words[0], strlen(words[0]));
    wrapped_write(writingEnd, &ind, sizeof(ind));

    while (wrapped_read(readingEnd, &ind, sizeof(ind)) > 0) {
        if (ind > wc) {
            wrapped_write(writingEnd, &ind, sizeof(ind));
            break;
        } else {
            wrapped_write(1, words[ind % 3], strlen(words[ind % 3]));
            ind++;
            wrapped_write(writingEnd, &ind, sizeof(ind));
        }
    }

    close(readingEnd);
    close(writingEnd);

    // wait for all the children to finish
    for (int i = 1; i <= nc; i++) {
        int status;
        if (wait(&status) < 0) {
            err(9, "Failed to wait");
        }

        if (!WIFEXITED(status)) {
            errx(10, "Child exited abnormally");
        }

        if (WEXITSTATUS(status)) {
            errx(11, "Exit code != 0");
        }
    }

    exit(0);
}
