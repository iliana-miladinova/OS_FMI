#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pwd.h>

int wrapped_open(const char* filename, int mode, int* access) {
    int fd;
    if (access) {
        fd = open(filename, mode, access);
    }
    else {
        fd = open(filename, mode);
    }

    if (fd < 0) {
        err(1, "Failed to open file %s", filename);
    }

    return fd;
}

int wrapped_write(int fd, const void* buff, size_t size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(2, "Failed to write to %d", fd);
    }

    return w;
}

int wrapped_read(int fd, void* buff, int size) {
    int r = read(fd, buff, size);
    if (r < 0) {
        err(13, "Could not read from file %d", fd);
    }
    return r;
}

pid_t wrapped_fork(void) {
	pid_t child = fork();
	if (child < 0) {
		err(7, "Failed to fork");
	}

	return child;
}

int wrapped_wait(void) {

    int w_status;
    
    if (wait(&w_status) < 0) {
        err(1, "Failed to wait for child to finish");
    }

    if (!WIFEXITED(w_status)) {
        errx(1, "Child exited abnormally!");
    }
    
    return w_status;
}

int wrapped_dup2(int oldfd, int newfd) {
    int d = dup2(oldfd, newfd);

    if (d < 0) {
        err(9, "Failed to duplicate %d to %d", oldfd, newfd);
    }

    return d;
}

int wrapped_pipe(int pipefd[2]) {
    int p = pipe(pipefd);

    if (p < 0) {
        err(10, "Failed to pipe");
    }

    return p;
}

int exec_ps(const char* username) {
	int pipefd[2];
	wrapped_pipe(pipefd);

	pid_t child = wrapped_fork();

	if (child == 0) {
		close(pipefd[0]);

		wrapped_dup2(pipefd[1], 1);
		close(pipefd[1]);

		if (execlp("ps", "ps", "-u", username, "-o", "pid=", (void*) NULL) < 0) {
			err(11, "Failed to exec"); 
		}
		
	}
	close(pipefd[1]);
	return pipefd[0];
}

int readNextProcess(int pipeInp, char buff[1024]) {
	int bytes = 0;
	char ch;
	int ind = 0;

	while ((bytes = wrapped_read(pipeInp, &ch, sizeof(ch))) > 0) {
		if (ch == ' ') {
			continue;
		}
		if (ch == '\n') {
			break;
		}
		buff[ind++] = ch;
	}

	if (bytes == 0) {
		return 0;
	}

	buff[ind++] = '\0';
	return bytes;
}

void killProcess(int pid) {
	pid_t child = wrapped_fork();

	if (child == 0) {
		char pid_str[16];
		snprintf(pid_str, sizeof(pid_str), "%s", pid);
		if (execlp("kill", "kill", pid_str, (void*)NULL) < 0) {
			err(12, "Failed to execute kill");
		}
	}
}

int main(int argc, char* argv[]) {
	struct timeval tv;
	if (gettimeofday(&tv, NULL) < 0) {
		err(3, "Failed to run gettimeval");
	}

	struct tm* tm = localtime(&tv.tv_sec);
	if (tm == NULL) {
		err(4, "Failed to get local time");
	}

	char data[30];
	if (strftime(data, 30, "%F %T", tm) <= 0) {
		err(5, "Failed to format date");
	}

	uid_t uid = getuid();
	struct passwd* pwd = getpwuid(uid);

	if (pwd == NULL) {
		err(6, "Failed to extract passwd struct");
	}

	int access = S_IWUSR | S_IRUSR;

	int fd = wrapped_open("foo.log", O_CREAT | O_APPEND | O_RDWR | &access);
	wrapped_write(fd, date, strlen(date));
	wrapped_write(fd, " ", 1);
	wrapped_write(fd, pwd->pw_name, strlen(pwd->pw_name));
	wrapped_write(fd, " ", 1);
	for (int i = 1; i < argc; i++) {
		wrapped_write(fd, argv[i], strlen(argv[i]));
		wrapped_write(fd, " ", 1);
	}

	wrapped_write(fd, "\n", 1);

	//lock the user - we dont run it
	/*pid_t child = wrapped_fork();
	if (child == 0) {
		if (execlp("passwd", "passwd", "-l", pwd->pw_name, (void*)NULL) < 0) {
		err(8, "Failed to exec");
		}
	} */
	//wrapped_wait();
	
	int pipeInp = exec_ps(pwd->pw_name);

	char buff[1024];
	while (readNextProcess(pipeInp, buff) > 0) {
		//killProcess(buff); dont exec
		printf("Kill %s\n", buff);
	}

	close(pipeInp);
	close(fd);
}
