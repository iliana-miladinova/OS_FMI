#include <unistd.h>
#include <err.h>
#include <sys/wait.h>

int wrapped_pipe(int pipefd[2]) {
    int p = pipe(pipefd);

    if (p < 0) {
        err(1, "Failed to pipe");
    }

    return p;
}

pid_t wrapped_fork(void) {
    pid_t child = fork();

    if (child < 0) {
        err(2, "Failed to fork");
    }

    return child;
}

int wrapped_dup2(int oldfd, int newfd) {
    int d = dup2(oldfd, newfd);

    if (d < 0) {
        err(3, "Failed to duplicate %d in %d", newfd, oldfd);
    }

    return d;
}

int main(int argc, char* argv[]) {
    // comm: find taskbook -type f -printf "%T@ %p\n" | sort -n | tail -n 1 | cut -d ' ' -f 2
    if (argc != 2) {
        errx(4, "Invalid num of args");
    }

    int findToSort[2];
    wrapped_pipe(findToSort);

    pid_t pr1 = wrapped_fork();

    if (pr1 == 0) {
        close(findToSort[0]);
        wrapped_dup2(findToSort[1], 1);
        close(findToSort[1]);

        if (execlp("find", "find", argv[1], "-type", "f", "-printf", "%T@ %p\n", (char*)NULL) < 0) {
            err(6, "Failed to exec");
        }
    }

    close(findToSort[1]);

    int sortToTail[2];
    wrapped_pipe(sortToTail);

    pid_t pr2 = wrapped_fork();

    if (pr2 == 0) {
        close(sortToTail[0]);
        wrapped_dup2(sortToTail[1], 1); //write in the pipe not on stdout
        close(sortToTail[1]);
        wrapped_dup2(findToSort[0], 0); //read from the pipe not from stdin

        close(findToSort[0]);

        if (execlp("sort", "sort", "-n", (char*)NULL) < 0) {
            err(6, "Failed to exec");
        }
    }

    close(sortToTail[1]);
    close(findToSort[0]);

    int tailToCut[2];
    wrapped_pipe(tailToCut);

    pid_t pr3 = wrapped_fork();

    if (pr3 == 0) {
        close(tailToCut[0]);
        wrapped_dup2(tailToCut[1], 1);
        close(tailToCut[1]);
        wrapped_dup2(sortToTail[0], 0);
        close(sortToTail[0]);

        if (execlp("tail", "tail", "-n", "1", (char*)NULL) < 0) {
            err(6, "Failed to exec");
        }
    }

    close(tailToCut[1]);
    close(sortToTail[0]);

    pid_t pr4 = wrapped_fork();

    if (pr4 == 0) {
        wrapped_dup2(tailToCut[0], 0);
        close(tailToCut[0]);

        if (execlp("cut", "cut", "-d" , " ", "-f", "2", (char*)NULL) < 0) {
            err(6, "Failed to exec");
        }
    }

    close(tailToCut[0]);

    for (int i = 0; i < 4; i++) {
        int w_status;

        if (wait(&w_status) < 0) {
            err(7, "Failed to wait for child to finish");
        }

        if (!WIFEXITED(w_status)) {
            err(7, "Child exited abnormally");
        }

        if (WEXITSTATUS(w_status) != 0) {
            err(8, "Child exited with status != 0");
        }
    }

    return 0;
}
