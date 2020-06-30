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
#define que_size 50000
int per[20], queue[que_size], front, end, fd[20], rand_key[80000], 
Score[10] = {0, 7, 6, 5, 4, 3, 2, 1, 0};
FILE* fds[20];
typedef struct Player {
	int score, id, rank;
}Player;
Player player[20];
void clear(int n) {
	fclose(fds[n]);
	char buf[20];
	if (n != 0)
		sprintf(buf, "Host%d.FIFO", n);
	else
		sprintf(buf, "Host.FIFO");
	if (unlink(buf) == -1)
		perror("unlink");
}
int cmp (const void * a, const void * b) {
	Player* ta = (Player*)a, *tb = (Player*)b;
	return tb->score - ta->score;
}
int cmp1 (const void * a, const void * b) {
	Player* ta = (Player*)a, *tb = (Player*)b;
	return ta->id - tb->id;
}
void push(int num) {
	queue[end] = num;
	++end;
}
int pop() {
	int res = queue[front];
	++front;
	return res;
}
bool empty() {
	return front == end;
}
int size() {
	return end - front; 
}
void get_result() {
	int key;
	fscanf(fds[0], "%d", &key);
	int a, b;
	for (int i = 0; i < 8; ++i) {
		fscanf(fds[0], "%d%d", &a, &b);
		player[a].score += Score[b];
	}
	push(rand_key[key]);
}
void make_round() {
	while (empty())
		get_result();
	int cur = pop();
	fprintf(fds[cur], "%d %d %d %d %d %d %d %d\n", per[1], per[2], per[3], per[4], per[5], per[6], per[7], per[8]);
	fflush(fds[cur]);
	fsync(fd[cur]);
}
void generate_round(int size, int player_num) {
	if (size == 9) {
		make_round();
		return;
	}
	for (int i = per[size - 1] + 1; i <= player_num; ++i) {
		per[size] = i;
		generate_round(size + 1, player_num);
	}
}
int main(int argc, char const *argv[])
{
	srand(time(0));
	int host_num = atoi(argv[1]), player_num = atoi(argv[2]);
	for (int i = 1; i <= player_num; ++i) {
		player[i].score = 0;
		player[i].id = i;
	}
	if (mkfifo("Host.FIFO", S_IRUSR | S_IWUSR | S_IWGRP) == -1)
		perror("mkfifo");
	fd[0] = open("Host.FIFO", O_RDONLY | O_NONBLOCK);
	int flags = fcntl(fd[0], F_GETFL);
	flags &= ~O_NONBLOCK;
	fcntl(fd[0], F_SETFL, flags);
	fds[0] = fdopen(fd[0], "rb");
	for (int i = 1; i <= host_num; ++i) {
		char fifo_name[20];
		sprintf(fifo_name, "Host%d.FIFO", i);
		if (mkfifo(fifo_name, S_IRUSR | S_IWUSR | S_IWGRP) == -1)
			perror("mkfifo");
		int key = 0;
		do {
			key = rand() % 65536;
		}
		while (rand_key[key]);
		rand_key[key] = i;
		pid_t pid = fork();
		if (pid == -1)
			perror("fork");
		else if (pid == 0) {
			char t1[5], t2[10];
			sprintf(t1, "%d", i); sprintf(t2, "%d", key);
			if (execlp("./host", "host", t1, t2, "0", (char *)NULL) == -1)
				perror("execlp");
			exit(0);
		}
		else {
			fd[i] = open(fifo_name, O_WRONLY);
			fds[i] = fdopen(fd[i], "wb");
			push(i);
		}
	}
	generate_round(1, player_num);
	while (size() != host_num)
		get_result();
	for (int i = 0; i <= host_num; ++i) {
		if (i)
			fprintf(fds[i], "-1 -1 -1 -1 -1 -1 -1 -1\n");
		clear(i);
	}
	qsort(player + 1, player_num, sizeof(Player), cmp);
	for (int i = 1; i <= player_num; ++i) {
		if (player[i].score == player[i - 1].score)
			player[i].rank = player[i - 1].rank;
		else
			player[i].rank = i;
	}
	qsort(player + 1, player_num, sizeof(Player), cmp1);
	for (int i = 1; i <= player_num; ++i)
		printf("%d %d\n", player[i].id, player[i].rank);
	int status = 0;
	while (wait(&status) != -1);
	return 0;
}