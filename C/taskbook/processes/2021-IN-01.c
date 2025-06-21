#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>

int convertStringToInt(const char* str) {
    char* endPtr;
    int num = strtol(str, &endPtr, 10);

    if (str[0] == '\0' || *endPtr != '\0') {
        errx(1, "Arg should be a number");
    }

    return num;
}

int wrapped_pipe(int pipefd[2]) {
    int p = pipe(pipefd);

    if (p < 0) {
        err(3, "Failed to pipe");
    }

    return p;
}

pid_t wrapped_fork(void) {
    pid_t child = fork();

    if (child < 0) {
        err(4, "Failed to fork");
    }

    return child;
}

int wrapped_read(int fd, void* buff, size_t size) {
    int r = read(fd, buff, size);

    if (r < 0) {
        err(5, "Failed to read from %d", fd);
    }

    return r;
}

int wrapped_write(int fd, const void* buff, size_t size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(6, "Failed to write to %d", fd);
    }

    return w;
}

const char* ding = "DING\n";
const char* dong = "DONG\n";

int main(int argc, char* argv[]) {
    if (argc != 3) {
        errx(2, "Invalid num of args");
    }

    int n = convertStringToInt(argv[1]);
    int d = convertStringToInt(argv[2]);

    int parent_to_child[2];
    int child_to_parent[2];

    wrapped_pipe(parent_to_child);
    wrapped_pipe(child_to_parent);

    pid_t child = wrapped_fork();

    if (child == 0) {
        close(parent_to_child[1]);
        close(child_to_parent[0]);

        char buff;

        while (read(parent_to_child[0], &buff, 1) > 0) { //if there is nothing in parent_to_child to
                                                         //read, then we block the child and
                                                         //we wait until the parent writes
                                                         //smth in it and it unblocks the child
            wrapped_write(1, dong, strlen(dong));
            wrapped_write(child_to_parent[1], "r", 1); //signal the parent that the child has done
                                                       //its job
        }

        close(parent_to_child[0]);
        close(child_to_parent[1]);
        exit(0); //finish the child process
    }

    close(child_to_parent[1]);
    close(parent_to_child[0]);

    char buff;

    for (int i = 0; i < n; i++) {
        wrapped_write(1, ding, strlen(ding));
        wrapped_write(parent_to_child[1], "r", 1);
        wrapped_read(child_to_parent[0], &buff, 1);

        sleep(d);
    }

    close(parent_to_child[1]);
    close(child_to_parent[0]);
}
