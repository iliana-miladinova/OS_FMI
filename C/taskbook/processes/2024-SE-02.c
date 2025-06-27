#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <err.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>

int wrapped_open(const char* filename, int mode, int* access) {
    int fd;

    if (access) {
        fd = open(filename, mode, access);
    } else {
        fd = open(filename, mode);
    }

    if (fd < 0) {
        err(2, "Failed to open file %s", filename);
    }

    return fd;
}

pid_t wrapped_fork(void) {
    pid_t child = fork();

    if (child < 0) {
        err(3, "Failed to fork a child");
    }

    return child;
}

int wrapped_dup2(int oldfd, int newfd) {
    int d = dup2(oldfd, newfd);

    if (d < 0) {
        err(4, "Failed to duplicate %d to %d", oldfd, newfd);
    }

    return d;
}

int null_fd;
pid_t runner_pids[10] = {0};

pid_t start_child(char* pathname) {
    pid_t child = wrapped_fork();

    if (child == 0) {
        wrapped_dup2(null_fd, 1);
        wrapped_dup2(null_fd, 2);
        if (execlp(pathname, pathname, (const char*) NULL) < 0) {
            err(5, "Failed to exec %s", pathname);
        }
    }

    return child;
}

int get_pid_idx(int pid) {
    for (int i = 0; i < 10; i++) {
        if (pid == runner_pids[i]) {
            return i;
        }
    }

    return -1;
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 11) {
        errx(1, "Invalid num of args");
    }

    int exit_status = 0;
    null_fd = wrapped_open("/dev/null", O_WRONLY, NULL);

    for (int i = 1; i < argc; i++) {
        pid_t child = start_child(argv[i]);
        runner_pids[i - 1] = child;
    }

    int w;
    int status;

    while ((w = wait(&status)) > 0) {
        int pid_ind = get_pid_idx(w);

        if (pid_ind < 0) {
            errx(6, "Process with id %d not found in table", w);
        }

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            runner_pids[pid_ind] = 0;
        } else if (WIFSIGNALED(status)) {
            runner_pids[pid_ind] = 0;
            exit_status = pid_ind + 1;
            break;
        } else {
            runner_pids[pid_ind] = start_child(argv[pid_ind + 1]);
        }
    }

    if (w < 0) {
        err(7, "Failed to wait for child");
    }

    close(null_fd);

    for (int i = 0; i < argc - 1; i++) {
        if (runner_pids[i] > 0) {
            if (kill(runner_pids[i], SIGTERM) < 0) {
                if (errno != ESRCH) {
                    err(8, "Failed to kill pid %d", runner_pids[i]);
                }
            }
        }
    }

    for (int i = 0; i < argc - 1; i++) {
        if (runner_pids[i] > 0) {
            if (wait(&status) < 0) {
                err(9, "Failed to wait child");
            }
        }
    }

    exit(exit_status);
}
