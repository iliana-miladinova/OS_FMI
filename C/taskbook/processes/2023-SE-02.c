#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <err.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

pid_t wrapped_fork(void) {
    pid_t child = fork();

    if (child < 0) {
        err(26, "Failed to fork");
    }

    return child;
}

int wrapped_pipe(int pipefd[2]) {
    int p = pipe(pipefd);

    if (p < 0) {
        err(26, "Failed to pipe");
    }

    return p;
}

int wrapped_dup2(int oldfd, int newfd) {
    int d = dup2(oldfd, newfd);

    if (d < 0) {
        err(26, "Failed to duplicate %d into %d", oldfd, newfd);
    }

    return d;
}

int wrapped_read(int fd, void* buff, int size) {
    int r = read(fd, buff, size);

    if (r < 0) {
        err(26, "Failed to read from %d", fd);
    }

    return r;
}

int wrapped_kill(pid_t pid, int signal) {
    int k = kill(pid, signal);

    if (k < 0) {
        err(26, "Failed to kill %d", pid);
    }

    return k;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        errx(26, "Invalid num of args");
    }

    pid_t pids[4096];

    int pipefd[2];
    wrapped_pipe(pipefd);

    for (int i = 1; i < argc; i++) {
        pid_t child = wrapped_fork();

        if (child == 0) {
            close(pipefd[0]);

            wrapped_dup2(pipefd[1], 1);
            close(pipefd[1]);

            if (execlp(argv[i], argv[i], (const char*) NULL) < 0) {
                err(26, "Failed to exec %s", argv[i]);
            }
        }
        pids[i-1] = child;
    }

    close(pipefd[1]);

    const char* patt = "found it";

    char ch;
    size_t ind = 0;

    while(wrapped_read(pipefd[0], &ch, sizeof(ch)) > 0) {
        if (patt[ind] == ch) {
            ind++;
        }
        else {
            ind = 0;
        }

        if (ind == strlen(patt)) {
            for (int i = 0; i < argc - 1; i++) {
                wrapped_kill(pids[i], SIGTERM);
            }
            exit(0);
        }
    }

    for (int i = 1; i < argc; i++) {
        int status;
        if(wait(&status) < 0) {
            err(26, "Failed to wait");
        }

        if (!WIFEXITED(status)) {
            err(26, "Child exited abnormally");
        }

        if (WEXITSTATUS(status) != 0) {
            err(26, "Exit code != 0");
        }

    }

    exit(1);
}
