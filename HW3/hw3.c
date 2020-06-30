#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "scheduler.h"
#define SIGUSR3 SIGWINCH
int idx, P, Q, B, task, mutex, queue;
char arr[10000];
jmp_buf SCHEDULER, local;
FCB_ptr function[5];
FCB_ptr Current, Head;
sigset_t block, unblock, pending;
struct sigaction sa;
static void handler(int Sig) {
	if (Sig != SIGUSR3) {
		printf("ACK\n");
		fflush(stdout);
	}
	else {
		int cnt = 0;
		for (int i = 1; i <= 4; ++i)
			if (queue & (1 << i))
				++cnt;
		for (int i = 1; i <= 4; ++i)
			if (queue & (1 << i)) {
				printf("%d", i);
				--cnt;
				if (cnt)
					printf(" ");
				else
					printf("\n");
			}
		fflush(stdout);
	}
	sigprocmask(SIG_BLOCK, &block, NULL);
	longjmp(SCHEDULER, 1);
}
void funct_1(int name) {
	volatile int i = 1, j = 1;
	if (setjmp(function[0]->Environment) == 0)
		funct_5(name + 1);
	else {
		if (mutex && mutex != 1) {
			queue |= (1 << 1);
			longjmp(SCHEDULER, 1);
		}
		mutex = 1; queue &= ~(1 << 1);
		while (j <= P) {
			for (i = 1; i <= Q; ++i) {
				sleep(1);
				arr[idx++] = '1';
			}
			++j;
			if (task == 2 && (j - 1) % B == 0) {
				mutex = 0;
				longjmp(SCHEDULER, 1);
			}
			if (task == 3) {
				sigpending(&pending);
				if (sigismember(&pending, SIGUSR1)) {
					sigemptyset(&unblock); sigaddset(&unblock, SIGUSR1);
					sigprocmask(SIG_UNBLOCK, &unblock, NULL);
				}
				if (sigismember(&pending, SIGUSR2)) {
					mutex = 0;
					sigemptyset(&unblock); sigaddset(&unblock, SIGUSR2);
					sigprocmask(SIG_UNBLOCK, &unblock, NULL);
				}
				if (sigismember(&pending, SIGUSR3)) {
					Current = Current->Previous;
					sigemptyset(&unblock); sigaddset(&unblock, SIGUSR3);
					sigprocmask(SIG_UNBLOCK, &unblock, NULL);
				}
			}
		}
		mutex = 0;
		longjmp(SCHEDULER, -2);
	}
}
void funct_2(int name) {
	volatile int i = 1, j = 1;
	if (setjmp(function[1]->Environment) == 0)
		funct_5(name + 1);
	else {
		if (mutex && mutex != 2) {
			queue |= (1 << 2);
			longjmp(SCHEDULER, 1);
		}
		mutex = 2; queue &= ~(1 << 2);
		while (j <= P) {
			for (i = 1; i <= Q; ++i) {
				sleep(1);
				arr[idx++] = '2';
			}
			++j;
			if (task == 2 && (j - 1) % B == 0) {
				mutex = 0;
				longjmp(SCHEDULER, 1);
			}
			if (task == 3) {
				sigpending(&pending);
				if (sigismember(&pending, SIGUSR1)) {
					sigemptyset(&unblock); sigaddset(&unblock, SIGUSR1);
					sigprocmask(SIG_UNBLOCK, &unblock, NULL);
				}
				if (sigismember(&pending, SIGUSR2)) {
					mutex = 0;
					sigemptyset(&unblock); sigaddset(&unblock, SIGUSR2);
					sigprocmask(SIG_UNBLOCK, &unblock, NULL);
				}
				if (sigismember(&pending, SIGUSR3)) {
					Current = Current->Previous;
					sigemptyset(&unblock); sigaddset(&unblock, SIGUSR3);
					sigprocmask(SIG_UNBLOCK, &unblock, NULL);
				}
			}
		}
		mutex = 0;
		longjmp(SCHEDULER, -2);
	}
}
void funct_3(int name) {
	volatile int i = 1, j = 1;
	if (setjmp(function[2]->Environment) == 0)
		funct_5(name + 1);
	else {
		if (mutex && mutex != 3) {
			queue |= (1 << 3);
			longjmp(SCHEDULER, 1);
		}
		mutex = 3; queue &= ~(1 << 3);
		while (j <= P) {
			for (i = 1; i <= Q; ++i) {
				sleep(1);
				arr[idx++] = '3';
			}
			++j;
			if (task == 2 && (j - 1) % B == 0) {
				mutex = 0;
				longjmp(SCHEDULER, 1);
			}
			if (task == 3) {
				sigpending(&pending);
				if (sigismember(&pending, SIGUSR1)) {
					sigemptyset(&unblock); sigaddset(&unblock, SIGUSR1);
					sigprocmask(SIG_UNBLOCK, &unblock, NULL);
				}
				if (sigismember(&pending, SIGUSR2)) {
					mutex = 0;
					sigemptyset(&unblock); sigaddset(&unblock, SIGUSR2);
					sigprocmask(SIG_UNBLOCK, &unblock, NULL);
				}
				if (sigismember(&pending, SIGUSR3)) {
					Current = Current->Previous;
					sigemptyset(&unblock); sigaddset(&unblock, SIGUSR3);
					sigprocmask(SIG_UNBLOCK, &unblock, NULL);
				}
			}
		}
		mutex = 0;
		longjmp(SCHEDULER, -2);
	}
}
void funct_4(int name) {
	volatile int i = 1, j = 1;
	if (setjmp(function[3]->Environment) == 0)
		longjmp(local, 1);
	else {
		if (mutex && mutex != 4) {
			queue |= (1 << 4);
			longjmp(SCHEDULER, 1);
		}
		mutex = 4; queue &= ~(1 << 4);
		while (j <= P) {
			for (i = 1; i <= Q; ++i) {
				sleep(1);
				arr[idx++] = '4';
			}
			++j;
			if (task == 2 && (j - 1) % B == 0) {
				mutex = 0;
				longjmp(SCHEDULER, 1);
			}
			if (task == 3) {
				sigpending(&pending);
				if (sigismember(&pending, SIGUSR1)) {
					sigemptyset(&unblock); sigaddset(&unblock, SIGUSR1);
					sigprocmask(SIG_UNBLOCK, &unblock, NULL);
				}
				if (sigismember(&pending, SIGUSR2)) {
					mutex = 0;
					sigemptyset(&unblock); sigaddset(&unblock, SIGUSR2);
					sigprocmask(SIG_UNBLOCK, &unblock, NULL);
				}
				if (sigismember(&pending, SIGUSR3)) {
					Current = Current->Previous;
					sigemptyset(&unblock); sigaddset(&unblock, SIGUSR3);
					sigprocmask(SIG_UNBLOCK, &unblock, NULL);
				}
			}
		}
		mutex = 0;
		longjmp(SCHEDULER, -2);
	}
}
void funct_5(int name) {
	int a[10000];
	if (name == 1)
		funct_1(name);
	else if (name == 2)
		funct_2(name);
	else if (name == 3)
		funct_3(name);
	else
		funct_4(name);
}
int main(int argc, char const *argv[])
{
	sigemptyset(&block); sigaddset(&block, SIGUSR1); sigaddset(&block, SIGUSR2); sigaddset(&block, SIGUSR3);
	sigprocmask(SIG_BLOCK, &block, NULL);
	sigemptyset(&sa.sa_mask); sa.sa_flags = 0; sa.sa_handler = handler;
	sigaction(SIGUSR1, &sa, NULL); sigaction(SIGUSR2, &sa, NULL); sigaction(SIGUSR3, &sa, NULL);
	P = atoi(argv[1]), Q = atoi(argv[2]), task = atoi(argv[3]), B = atoi(argv[4]);
	for (int i = 0; i < 4; ++i)
		function[i] = (FCB*)malloc(sizeof(FCB));
	Head = (FCB*)malloc(sizeof(FCB)); Current = (FCB*)malloc(sizeof(FCB));
	if (setjmp(local) == 0)
		funct_5(1);
	else {
		Head->Next = Current->Next = function[0];
		for (int i = 0; i < 4; ++i) {
			function[i]->Next = function[(i + 1) % 4];
			function[i]->Previous = function[(i + 3) % 4];
			function[i]->Name = i + 1;
		}
		Scheduler();
	}
	exit(0);
}