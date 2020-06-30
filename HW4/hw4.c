#pragma GCC optimize("Ofast")
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define MAX_ROW 60000
#define MAX_COL 784
#define MAX_CAT 10
#define MAX_TEST 10000
#define min(a, b) (a < b? a: b)
#define max(a, b) (a > b? a: b)
unsigned char matrix[MAX_ROW][MAX_COL], matrix_T[MAX_COL][MAX_ROW], y[MAX_ROW + 5], ty[MAX_TEST], tx[MAX_TEST][MAX_COL];
double W_grad[MAX_COL + 5][MAX_ROW + 5], W[MAX_COL + 5][MAX_CAT + 5],
	y_hat[MAX_ROW + 5][MAX_CAT + 5], ry[MAX_ROW + 5][MAX_CAT + 5], lr = 0.01;
pthread_t IDs[MAX_ROW];
int int_id[MAX_ROW], thread_num, row_num, col_num, TR_NUM = 36;
inline int get_int(double num) {return (num == (int)num)? (int)num: (int)num + 1;}
void multiplication(int id, bool test, unsigned char MX[][MAX_COL], double MY[][MAX_CAT + 5]) {
	int bound = min(row_num * id, (test? MAX_TEST: MAX_ROW));
	for (int i = row_num * (id - 1); i < bound; ++i)
		for (int j = 0; j < MAX_CAT; ++j)
			MY[i][j] = 0;
	for (int i = row_num * (id - 1); i < bound; ++i)
		for (int k = 0; k < MAX_COL; ++k) {
			for (int j = 0; j < MAX_CAT; ++j)
				MY[i][j] += (double)MX[i][k] * W[k][j];
		}
}
void soft_max(int id) {
	int bound = min(row_num * id, MAX_ROW);
	for (int i = row_num * (id - 1); i < bound; ++i) {
		double sum = 0, maxi = 0;
		for (int j = 0; j < MAX_CAT; ++j)
			maxi = max(maxi, y_hat[i][j]);
		for (int j = 0; j < MAX_CAT; ++j)
			sum += exp(y_hat[i][j] - maxi);
		for (int j = 0; j < MAX_CAT; ++j)
			y_hat[i][j] = exp(y_hat[i][j] - maxi) / sum;
		y_hat[i][y[i]] -= 1;
	}
}
void calc_w(int id) {
	int bound = min(col_num * id, MAX_COL);
	for (int i = col_num * (id - 1); i < bound; ++i)
		for (int j = 0; j < MAX_CAT; ++j)
			W[i][j] -= lr * W_grad[i][j];
}
void trans_mul(int id) {
	int bound = min(col_num * id, MAX_COL);
	for (int i = col_num * (id - 1); i < bound; ++i)
		for (int j = 0; j < MAX_CAT; ++j)
			W_grad[i][j] = 0;
	for (int i = col_num * (id - 1); i < bound; ++i) {
		for (int k = 0; k < MAX_ROW; ++k)
			for (int j = 0; j < MAX_CAT; ++j)
				W_grad[i][j] += (double)matrix_T[i][k] * y_hat[k][j];
		}
}
static void* thread_funct(void *arg) {
	int id = *(int *)arg;
	if (id > MAX_ROW) {
		multiplication(id - MAX_ROW, 1, tx, ry);
		pthread_exit(0);
	} else if (id > 0) {
		multiplication(id, 0, matrix, y_hat);
		soft_max(id);
		pthread_exit(0);
	} else {
		trans_mul(-id);
		calc_w(-id);
		pthread_exit(0);
	}
}
void output() {
	for (int i = 0; i < thread_num; ++i) {
		if (row_num * i >= MAX_TEST)
			break;
		int_id[i] = MAX_ROW + i + 1;
		pthread_create(&IDs[i], NULL, thread_funct, &int_id[i]);
	}
	for (int i = 0; i < thread_num; ++i) {
		if (row_num * i >= MAX_TEST)
			break;
		pthread_join(IDs[i], NULL);
	}
	FILE* fd = fopen("result.csv", "w");
	fprintf(fd, "id,label\n");
	for (int i = 0; i < MAX_TEST; ++i) {
		double maxi = -1; int res;
		for (int j = 0; j < MAX_CAT; ++j)
			if (ry[i][j] > maxi) {
				maxi = ry[i][j];
				res = j;
			}
		fprintf(fd, "%d,%d\n", i, res);
	}
}
int get_acc() {
	int correct = 0;
	for (int i = 0; i < thread_num; ++i) {
		if (row_num * i >= MAX_TEST)
			break;
		int_id[i] = MAX_ROW + i + 1;
		pthread_create(&IDs[i], NULL, thread_funct, &int_id[i]);
	}
	for (int i = 0; i < thread_num; ++i) {
		if (row_num * i >= MAX_TEST)
			break;
		pthread_join(IDs[i], NULL);
	}
	for (int i = 0; i < MAX_TEST; ++i) {
		double maxi = -1; int res;
		for (int j = 0; j < MAX_CAT; ++j)
			if (ry[i][j] > maxi) {
				maxi = ry[i][j];
				res = j;
			}
		if (res == ty[i])
			++correct;
	}
	return correct;
}
int main(int argc, char const *argv[]) {
	int train_x = open(argv[1], O_RDONLY), train_y = open(argv[2], O_RDONLY);
	int test_x = open("X_test", O_RDONLY), test_y = open("y_test", O_RDONLY);
	thread_num = atoi(argv[4]);
	if (thread_num == 100)
		TR_NUM = 199;
	row_num = get_int((double)MAX_ROW / thread_num), col_num = get_int((double)MAX_COL / thread_num);
	read(train_x, matrix, sizeof(matrix));
	read(train_y, y, sizeof(y));
	read(test_x, tx, sizeof(tx));
	read(test_y, ty, sizeof(ty));
	close(train_x); close(train_y); close(test_x); close(test_y);
	for (int i = 0; i < MAX_ROW; ++i)
		for (int j = 0; j < MAX_COL; ++j)
			matrix_T[j][i] = matrix[i][j];
	for (int i = 0; i < TR_NUM; ++i) {
		for (int j = 0; j < thread_num; ++j) {
			if (row_num * j >= MAX_ROW)
				break;
			int_id[j] = j + 1;
			pthread_create(&IDs[j], NULL, thread_funct, &int_id[j]);
		}
		for (int j = 0; j < thread_num; ++j) {
			if (row_num * j >= MAX_ROW)
				break;
			pthread_join(IDs[j], NULL);
		}
		for (int j = 0; j < thread_num; ++j) {
			if (row_num * j >= MAX_ROW)
				break;
			int_id[j] = -(j + 1);
			pthread_create(&IDs[j], NULL, thread_funct, &int_id[j]);
		}
		for (int j = 0; j < thread_num; ++j) {
			if (row_num * j >= MAX_ROW)
				break;
			pthread_join(IDs[j], NULL);
		}
		if (i == 0) lr = 0.001;
		if (i > 65)
			lr *= (1.0 / (1.0 + 0.0000005 * (i + 1)));
	}
	output();
	exit(0);
}