#include <unistd.h>
#include <err.h>
#include <sys/wait.h>

int wrapped_pipe(int pipefd[2]) {
    int p = pipe(pipefd);

    if (p < 0) {
        err(1, "Could not pipe");
    }

    return p;
}

pid_t wrapped_fork(void) {
    pid_t child = fork();

    if (child < 0) {
        err(2, "Could not fork");
    }

    return child;
}

int wrapped_dup2(int oldfd, int newfd) {
    int d = dup2(oldfd, newfd);

    if (d < 0) {
        err(3, "Could not duplicate %d in %d", newfd, oldfd);
    }

    return d;
}

int wrapped_wait(void) {
    int w_status;

    if (wait(&w_status) < 0) {
        err(5, "Could not wait for child to finish");
    }

    if (!WIFEXITED(w_status)) {
        errx(6, "Child exited abnormally");
    }
    if (WEXITSTATUS(w_status) != 0) {
        err(7, "Child exited with status !=0");
    }

    return w_status;
}

int main(void) {
    int cutToSort[2];
    wrapped_pipe(cutToSort);

    pid_t proc1 = wrapped_fork();

    if (proc1 == 0) {
        close(cutToSort[0]);
        wrapped_dup2(cutToSort[1], 1);
        close(cutToSort[1]);

        if (execlp("cut", "cut", "-d", ":", "-f", "7", "/etc/passwd", (char*)NULL) < 0) {
            err(4, "Could not exec");
        }
    }

    close(cutToSort[1]); // the parent only reads from this pipe

    int sortToUniq[2];
    wrapped_pipe(sortToUniq);
    pid_t proc2 = wrapped_fork();

    if (proc2 == 0) {
        close(sortToUniq[0]);

        wrapped_dup2(sortToUniq[1], 1);
        close(sortToUniq[1]);
        wrapped_dup2(cutToSort[0], 0); // sort reads from the pipe, not from stdin

        if (execlp("sort", "sort", (char*)NULL) < 0) {
            err(4, "Could not exec");
        }
    }

    close(cutToSort[0]);
    close(sortToUniq[1]);

    int uniqToSort[2];
    wrapped_pipe(uniqToSort);
    pid_t proc3 = wrapped_fork();

    if (proc3 == 0) {
        close(uniqToSort[0]);

        wrapped_dup2(uniqToSort[1], 1);
        close(uniqToSort[1]);
        wrapped_dup2(sortToUniq[0], 0);
        close(sortToUniq[0]);

        if (execlp("uniq", "uniq", "-c", (char*)NULL) < 0) {
            err(4, "Could not exec");
        }
    }

    close(sortToUniq[0]);
    close(uniqToSort[1]);

    pid_t proc4 = wrapped_fork();

    if (proc4 == 0) {
        wrapped_dup2(uniqToSort[0], 0);
        close(uniqToSort[0]);

        if (execlp("sort", "sort", "-n", (char*)NULL) < 0) {
            err(4, "Could not exec");
        }
    }

    close(uniqToSort[0]);

    for (int i = 0; i < 4; i++) {
        wrapped_wait();
    }
}
