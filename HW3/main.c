#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#define SIGUSR3 SIGWINCH
int sig[20], fds[2];
char buf[10000];
int send(int num) {
	if (num == 1)
		return SIGUSR1;
	else if (num == 2)
		return SIGUSR2;
	else
		return SIGUSR3;
}
int main(int argc, char const *argv[])
{
	char P[5], Q[5]; int n; scanf("%s%s%d", &P, &Q, &n);
	for (int i = 0; i < n; ++i)
		scanf("%d", &sig[i]);
	if (pipe(fds) != 0)
		perror("pipe");
	pid_t pid;
	if ((pid = fork()) == 0) {
		close(fds[0]);
		dup2(fds[1], 1);
		close(fds[1]);
		if (execlp("./hw3", "./hw3", P, Q, "3", "0", (char *)NULL) == -1)
			perror("execlp");
		exit(0);
	}
	else {
		close(fds[1]);
		for (int i = 0; i < n; ++i) {
			sleep(5);
			kill(pid, send(sig[i]));
			int n = read(fds[0], buf, sizeof(buf));
			if (sig[i] == 3)
				printf("%s", buf);
			memset(buf, 0, sizeof(buf));
		}
		read(fds[0], buf, sizeof(buf));
		close(fds[0]);
		int status; wait(&status);
		printf("%s", buf); fflush(stdout);
		exit(0);
	}
}