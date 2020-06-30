#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <sys/wait.h>

#define ERR_EXIT(a) { perror(a); exit(1); }

typedef struct {
    char hostname[512];  // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;  // fd to wait for a new connection
} server;

typedef struct {
    char host[512];  // client's host
    int conn_fd;  // fd to talk with client
    char buf[512];  // data sent by/to client
    size_t buf_len;  // bytes used by buf
    // you don't need to change this.
	int item;
    int wait_for_write;  // used by handle_read to know if the header is read or not.
} request;

typedef struct {
    int id;
    int balance;
} Account;

server svr;  // server
request* requestP = NULL;  // point to a list of requests
int maxfd;  // size of open file descriptor table, size of request list

const char* accept_read_header = "ACCEPT_FROM_READ";
const char* accept_write_header = "ACCEPT_FROM_WRITE";

// Forwards

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request* reqP);
// initailize a request instance

static void free_request(request* reqP);
// free resources used by a request instance

static int handle_read(request* reqP);
// return 0: socket ended, request done.
// return 1: success, message (without header) got this time is in reqP->buf with reqP->buf_len bytes. read more until got <= 0.
// It's guaranteed that the header would be correctly set after the first read.
// error code:
// -1: client connection error
int write_lock(int id, int file_fd, int *local_lock) {
    if (local_lock[id])
        return -1;
    struct flock fl;
    errno = 0;
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = (id - 1) * sizeof(Account);
    fl.l_len = sizeof(Account);
    if (fcntl(file_fd, F_SETLK, &fl) != -1) {
        local_lock[id] = -1;
        return 1;
    }
    else if (errno == EAGAIN || errno == EACCES)
        return -1;
    else {
        ERR_EXIT("fcntl");
    }
}
int read_lock(int id, int file_fd, int *local_lock) {
    if (local_lock[id] == -1)
        return -1;
    struct flock fl;
    errno = 0;
    fl.l_type = F_RDLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = (id - 1) * sizeof(Account);
    fl.l_len = sizeof(Account);
    if (fcntl(file_fd, F_SETLK, &fl) != -1) {
        ++local_lock[id];
        return 1;
    }
    else if (errno == EAGAIN || errno == EACCES)
        return -1;
    else
        ERR_EXIT("fcntl");
}
void unlock(int id, int file_fd, int *local_lock) {
    struct flock fl;
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = (id - 1) * sizeof(Account);
    fl.l_len = sizeof(Account);
    fcntl(file_fd, F_SETLK, &fl);
    if (local_lock[id] > 0)
        --local_lock[id];
    else if (local_lock[id] == -1)
        local_lock[id] = 0;
}
void close_fd(int fd, fd_set *readfds) {
    FD_CLR(requestP[fd].conn_fd, &(*readfds));
    close(requestP[fd].conn_fd);
    free_request(&requestP[fd]);
}
void failed(int fd, char *buf) {
    sprintf(buf, "Operation failed.\n");
    write(requestP[fd].conn_fd, buf, strlen(buf));
}

int main(int argc, char** argv) {
    int i, ret;

    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;

    int conn_fd;  // fd for a new connection with client
    int file_fd = open("account_list", O_RDWR);  // fd for file that we open for reading
    char buf[512];
    int buf_len;

    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    // Initialize server
    init_server((unsigned short) atoi(argv[1]));

    // Get file descripter table size and initize request table
    maxfd = getdtablesize();
    int local_lock[maxfd + 5];
    for (int i = 0; i <= maxfd; ++i)
        local_lock[i] = 0;
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);
    // Loop for handling connections
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);
    int nfds = 0;
    struct timeval tv;
    fd_set readfds, writefds, svr_readfds;
    FD_ZERO(&readfds); FD_ZERO(&writefds); FD_ZERO(&svr_readfds);
    Account buffer_account;
    FD_SET(svr.listen_fd, &svr_readfds);
    while (1) {
        clilen = sizeof(cliaddr);
        tv.tv_sec = tv.tv_usec = 0;
        FD_SET(svr.listen_fd, &svr_readfds);
        if (select(svr.listen_fd + 1, &svr_readfds, NULL, NULL, &tv) == -1) {
            ERR_EXIT("select");
        }
        if (FD_ISSET(svr.listen_fd, &svr_readfds)){
            conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
            if (conn_fd < 0) {
                if (errno == EINTR || errno == EAGAIN) continue;  // try again
                if (errno == ENFILE) {
                    (void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                    continue;
                }
                ERR_EXIT("accept")
            }
            requestP[conn_fd].conn_fd = conn_fd;
            strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
            fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);
            nfds = nfds > conn_fd + 1? nfds: conn_fd + 1;
        }
        for (int cur_fd = 0; cur_fd < nfds; ++cur_fd)
            if (requestP[cur_fd].conn_fd != -1)
                FD_SET(requestP[cur_fd].conn_fd, &readfds);
        tv.tv_sec = tv.tv_usec = 0;
        if (select(nfds, &readfds, NULL, NULL, &tv) == -1) {
            ERR_EXIT("select");
        }
        for (int cur_fd = 0; cur_fd < nfds; ++cur_fd) {
            if (cur_fd != svr.listen_fd && FD_ISSET(cur_fd, &readfds)) {
        		ret = handle_read(&requestP[cur_fd]); // parse data from client to requestP[conn_fd].buf
        		if (ret < 0) {
        			fprintf(stderr, "bad request from %s\n", requestP[cur_fd].host);
        			continue;
        		}
            }
        }
#ifdef READ_SERVER
        for (int i = 0; i < nfds; ++i)
            if (requestP[i].buf_len) {
                requestP[i].item = atoi(requestP[i].buf);
                if (read_lock(requestP[i].item, file_fd, local_lock) == -1) {
                    sprintf(buf, "This account is locked.\n");
                    write(requestP[i].conn_fd, buf, strlen(buf));
                }
                else {
                    pread(file_fd, &buffer_account, sizeof(Account), sizeof(Account) * (requestP[i].item - 1));                    sprintf(buf, "%d %d\n", buffer_account.id, buffer_account.balance);
                    write(requestP[i].conn_fd, buf, strlen(buf));
                    unlock(requestP[i].item, file_fd, local_lock);
                }
                close_fd(i, &readfds);
            }
#else
        for (int i = 0; i < nfds; ++i)
            if (requestP[i].buf_len) {
                if (requestP[i].wait_for_write == 0) {
                    requestP[i].item = atoi(requestP[i].buf);
                    if (write_lock(requestP[i].item, file_fd, local_lock) == -1) {
                        sprintf(buf, "This account is locked.\n");
                        write(requestP[i].conn_fd, buf, strlen(buf));
                        close_fd(i, &readfds);
                    }
                    else {
                        requestP[i].wait_for_write = 1;
                        sprintf(buf, "This account is modifiable.\n");
                        write(requestP[i].conn_fd, buf, strlen(buf));
                        requestP[i].buf_len = 0;
                    }
                }
                else {
                    pread(file_fd, &buffer_account, sizeof(Account), sizeof(Account) * (requestP[i].item - 1));
                    char operation, char_buf[10]; int id, amount;
                    operation = requestP[i].buf[0];
                    if (operation == 's') {
                        sscanf(requestP[i].buf, "%s%d", char_buf, &amount);
                        if (amount < 0) {
                            failed(i, buf);
                        }
                        else {
                            buffer_account.balance += amount;
                            pwrite(file_fd, &buffer_account, sizeof(Account), sizeof(Account) * (requestP[i].item - 1));
                        }
                    }
                    else if (operation == 'w') {
                        sscanf(requestP[i].buf, "%s%d", char_buf, &amount);
                        if (amount < 0 || amount > buffer_account.balance) {
                            failed(i, buf);
                        }
                        else {
                            buffer_account.balance -= amount;
                            pwrite(file_fd, &buffer_account, sizeof(Account), sizeof(Account) * (requestP[i].item - 1));                            
                        }
                    }
                    else if (operation == 'b') {
                        sscanf(requestP[i].buf, "%s%d", char_buf, &amount);
                        if (amount <= 0) {
                            failed(i, buf);
                        }
                        else {
                            buffer_account.balance = amount;
                            pwrite(file_fd, &buffer_account, sizeof(Account), sizeof(Account) * (requestP[i].item - 1));
                        }
                    }
                    else {
                        sscanf(requestP[i].buf, "%s%d%d", char_buf, &id, &amount);
                        if (amount < 0 || amount > buffer_account.balance || write_lock(id, file_fd, local_lock) == -1) {
                            failed(i, buf);
                        }
                        else {
                            Account buffer_account2;
                            pread(file_fd, &buffer_account2, sizeof(Account), sizeof(Account) * (id - 1));
                            buffer_account.balance -= amount;
                            buffer_account2.balance += amount;
                            pwrite(file_fd, &buffer_account2, sizeof(Account), sizeof(Account) * (id - 1));
                            unlock(id, file_fd, local_lock);
                            pwrite(file_fd, &buffer_account, sizeof(Account), sizeof(Account) * (requestP[i].item - 1));
                        }
                    }
                    unlock(requestP[i].item, file_fd, local_lock);
                    close_fd(i, &readfds);
                }
            }
#endif
    }
    free(requestP);
    close(file_fd);
    return 0;
}



// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>



static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->item = 0;
    reqP->wait_for_write = 0;
}

static void free_request(request* reqP) {
    /*if (reqP->filename != NULL) {
        free(reqP->filename);
        reqP->filename = NULL;
    }*/
    init_request(reqP);
}

// return 0: socket ended, request done.
// return 1: success, message (without header) got this time is in reqP->buf with reqP->buf_len bytes. read more until got <= 0.
// It's guaranteed that the header would be correctly set after the first read.
// error code:
// -1: client connection error
static int handle_read(request* reqP) {
    int r;
    char buf[512] = {0};

    // Read in request from client
    r = read(reqP->conn_fd, buf, sizeof(buf));
    if (r < 0) return -1;
    if (r == 0) return 0;
	char* p1 = strstr(buf, "\015\012");
	int newline_len = 2;
	// be careful that in Windows, line ends with \015\012
	if (p1 == NULL) {
		p1 = strstr(buf, "\012");
		newline_len = 1;
		if (p1 == NULL) {
			ERR_EXIT("this really should not happen...");
		}
	}
	size_t len = p1 - buf + 1;
    memset(reqP->buf, 0, sizeof(reqP->buf));
	memmove(reqP->buf, buf, len);
	reqP->buf[len - 1] = '\0';
	reqP->buf_len = len-1;
    return 1;
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }
}

static void* e_malloc(size_t size) {
    void* ptr;

    ptr = malloc(size);
    if (ptr == NULL) ERR_EXIT("out of memory");
    return ptr;
}

