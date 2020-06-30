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
int main(int argc, char const *argv[])
{
	int player_id = atoi(argv[1]), winner = 0;
	for (int i = 0; i < 10; ++i) {
		printf("%d %d\n", player_id, 100 * player_id);
		fflush(stdout); fsync(1);
		if (i != 9)
			scanf("%d", &winner);
	}
	exit(0);
}