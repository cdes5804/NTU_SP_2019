#include <unistd.h>                                                                                                  
#include <stdio.h>                                                                                                   
#include <string.h>                                                                                                  
#include <fcntl.h>                                                                                                   
#include <sys/stat.h>                                                                                                
#include <sys/file.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
typedef struct Player{
	int score, id, rank;
}Player;
int cmp (const void * a, const void * b) {
	Player* ta = (Player*)a, *tb = (Player*)b;
	return tb->score - ta->score;
}
int cmp1 (const void * a, const void * b) {
	Player* ta = (Player*)a, *tb = (Player*)b;
	return ta->id - tb->id;
}
void make_pipe(int fd_left_pipe[2][2], int fd_right_pipe[2][2]) {
	for (int i = 0; i < 2; ++i) {
		if (pipe(fd_left_pipe[i]) == -1)
			perror("pipe");
		if (pipe(fd_right_pipe[i]) == -1)
			perror("pipe");
	}
}
void handle_child_pipe(int pipes[2][2], int pipes2[2][2]) {
	close(pipes[0][0]); close(pipes[1][1]);
	dup2(pipes[0][1], 1); dup2(pipes[1][0], 0);
	close(pipes[0][1]); close(pipes[1][0]);
	for (int i = 0; i < 2; ++i) {
		close(pipes2[i][0]); close(pipes2[i][1]);
	}
}
void handle_parent_pipe(int pipes[2][2], FILE* pipe_stream[2]) {
	close(pipes[0][1]); close(pipes[1][0]);
	pipe_stream[0] = fdopen(pipes[0][0], "rb");
	pipe_stream[1] = fdopen(pipes[1][1], "wb");
}
int main(int argc, char const *argv[])
{
	int host_id = atoi(argv[1]), key = atoi(argv[2]), depth = atoi(argv[3]);
	if (depth == 0) {
		int fd_write = open("Host.FIFO", O_WRONLY);
		char buf[150];
		sprintf(buf, "Host%d.FIFO", host_id);
		int fd_read = open(buf, O_RDONLY);
		FILE* fds_left_pipe[2], *fds_right_pipe[2];
		int fd_left_pipe[2][2], fd_right_pipe[2][2];
		make_pipe(fd_left_pipe, fd_right_pipe);
		for (int i = 0; i < 2; ++i) {
			pid_t pid = fork();
			if (pid == -1)
				perror("fork");
			else if (pid == 0) {
				if (i == 0)
					handle_child_pipe(fd_left_pipe, fd_right_pipe);
				else
					handle_child_pipe(fd_right_pipe, fd_left_pipe);
				if (execlp("./host", "host", argv[1], argv[2], "1", (char *)NULL) == -1)
					perror("execlp");
				exit(0);
			}
			else {
				if (i == 0)
					handle_parent_pipe(fd_left_pipe, fds_left_pipe);
				else
					handle_parent_pipe(fd_right_pipe, fds_right_pipe);
			}
		}
		int p[30], map[30], winner1, price1, winner2, price2; Player player[30];
		FILE* fds_write = fdopen(fd_write, "wb");
		FILE* fds_read = fdopen(fd_read, "rb");
		while (1) {
			for (int i = 1; i <= 8; ++i) {
				fscanf(fds_read, "%d", &p[i]);
				player[i].id = p[i]; player[i].score = player[i].rank = 0;
				map[p[i]] = i;
			}
			fprintf(fds_left_pipe[1], "%d %d %d %d\n", p[1], p[2], p[3], p[4]);
			fflush(fds_left_pipe[1]);
			fprintf(fds_right_pipe[1], "%d %d %d %d\n", p[5], p[6], p[7], p[8]);
			fflush(fds_right_pipe[1]);
			if (p[0] == -1) {
				fclose(fds_left_pipe[1]); fclose(fds_right_pipe[1]);
				fclose(fds_left_pipe[0]); fclose(fds_right_pipe[0]);
				int status = 0;
				while (wait(&status) != -1);
				exit(0);
			}
			for (int i = 0; i < 10; ++i) {
				fscanf(fds_left_pipe[0], "%d%d", &winner1, &price1);
				fscanf(fds_right_pipe[0], "%d%d", &winner2, &price2);
				int winner = 0;
				if (price1 > price2) {
					++player[map[winner1]].score;
					winner = winner1;
					
				}
				else {
					++player[map[winner2]].score;
					winner = winner2;
				}
				if (i != 9) {
					fprintf(fds_left_pipe[1], "%d\n", winner);
					fflush(fds_left_pipe[1]);
					fprintf(fds_right_pipe[1], "%d\n", winner);
					fflush(fds_right_pipe[1]);
				}
			}
			qsort(player + 1, 8, sizeof(Player), cmp);
			for (int i = 1; i <= 8; ++i)
				if (player[i].score == player[i - 1].score)
					player[i].rank = player[i - 1].rank;
				else
					player[i].rank = i;
			qsort(player + 1, 8, sizeof(Player), cmp1);
			char *tmp = buf;
			tmp += sprintf(tmp, "%d\n", key);
			for (int i = 1; i <= 8; ++i)
				tmp += sprintf(tmp, "%d %d\n", player[i].id, player[i].rank);
			fprintf(fds_write, "%s", buf);
			fflush(fds_write);
		}
	}
	else if (depth == 1) {
		int fd_left_pipe[2][2], fd_right_pipe[2][2];
		FILE* fds_left_pipe[2], *fds_right_pipe[2];
		make_pipe(fd_left_pipe, fd_right_pipe);
		for (int i = 0; i < 2; ++i) {
			pid_t pid = fork();
			if (pid == -1)
				perror("fork");
			else if (pid == 0) {
				if (i == 0)
					handle_child_pipe(fd_left_pipe, fd_right_pipe);
				else
					handle_child_pipe(fd_right_pipe, fd_left_pipe);
				if (execlp("./host", "host", argv[1], argv[2], "2", (char *)NULL) == -1)
					perror("execlp");
				exit(0);
			}
			else {
				if (i == 0)
					handle_parent_pipe(fd_left_pipe, fds_left_pipe);
				else
					handle_parent_pipe(fd_right_pipe, fds_right_pipe);
			}
		}
		while (1) {
			int p[10], winner1, price1, winner2, price2;
			for (int i = 0; i < 4; ++i)
				scanf("%d", &p[i]);
			fprintf(fds_left_pipe[1], "%d %d\n", p[0], p[1]);
			fflush(fds_left_pipe[1]);
			fprintf(fds_right_pipe[1], "%d %d\n", p[2], p[3]);
			fflush(fds_right_pipe[1]);
			if (p[0] == -1) {
				fclose(fds_left_pipe[1]); fclose(fds_right_pipe[1]);
				fclose(fds_left_pipe[0]); fclose(fds_right_pipe[0]);
				int status = 0;
				while (wait(&status) != -1);
				exit(0);
			}
			for (int i = 0; i < 10; ++i) {
				fscanf(fds_left_pipe[0], "%d%d", &winner1, &price1);
				fscanf(fds_right_pipe[0], "%d%d", &winner2, &price2);
				if (price1 > price2)
					printf("%d %d\n", winner1, price1);
				else
					printf("%d %d\n", winner2, price2);
				fflush(stdout);
				int winner;
				if (i != 9) {
					scanf("%d", &winner);
					fprintf(fds_left_pipe[1], "%d\n", winner);
					fflush(fds_left_pipe[1]);
					fprintf(fds_right_pipe[1], "%d\n", winner);
					fflush(fds_right_pipe[1]);
				}
			}
		}
	}
	else {
		while (1) {
			int player[2];
			scanf("%d%d", &player[0], &player[1]);
			if (player[0] == -1) {
				int status;
				while (wait(&status) != -1);
				exit(0);
			}
			int fd_left_pipe[2][2], fd_right_pipe[2][2];
			FILE* fds_left_pipe[2], *fds_right_pipe[2];
			make_pipe(fd_left_pipe, fd_right_pipe);
			for (int i = 0; i < 2; ++i) {
				pid_t pid = fork();
				if (pid == -1)
					perror("fork");
				else if (pid == 0) {
					if (i == 0)
						handle_child_pipe(fd_left_pipe, fd_right_pipe);
					else
						handle_child_pipe(fd_right_pipe, fd_left_pipe);
					char buf[20];
					sprintf(buf, "%d", player[i]);
					if (execlp("./player", "player", buf, (char *)NULL) == -1)
						perror("execlp");
					exit(0);
				}
				else {
					if (i == 0)
						handle_parent_pipe(fd_left_pipe, fds_left_pipe);
					else
						handle_parent_pipe(fd_right_pipe, fds_right_pipe);
				}
			}
			int winner1, price1, winner2, price2;
			for (int i = 0; i < 10; ++i) {
				fscanf(fds_left_pipe[0], "%d%d", &winner1, &price1);
				fscanf(fds_right_pipe[0], "%d%d", &winner2, &price2);
				if (price1 > price2)
					printf("%d %d\n", winner1, price1);
				else
					printf("%d %d\n", winner2, price2);
				fflush(stdout);
				if (i != 9) {
					int winner;
					scanf("%d", &winner);
					fprintf(fds_left_pipe[1], "%d\n", winner);
					fflush(fds_left_pipe[1]);
					fprintf(fds_right_pipe[1], "%d\n", winner);
					fflush(fds_right_pipe[1]);
				}
			}
			int status = 0;
			fclose(fds_left_pipe[1]); fclose(fds_right_pipe[1]);
			fclose(fds_left_pipe[0]); fclose(fds_right_pipe[0]);
			while (wait(&status) != -1);
		}
		exit(0);
	}
	exit(0);
}