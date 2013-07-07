#include <sys/types.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <pty.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <poll.h>
#include <vector>
#include <iostream>

#define PORT "3111"
const int portno = 3111;

const int BUF_SIZE = 2048;

struct s_buffer{
    char data[BUF_SIZE];
    size_t pos;
    int fd_in;
    int fd_out;
};


s_buffer* alloc_buffer(size_t size, int in, int out){
    s_buffer* buf = (s_buffer*)malloc(sizeof(s_buffer));
    bzero(buf->data, size);
    buf->pos = 0;
    buf->fd_in = in;
    buf->fd_out = out;
    printf("allocated buffer %p for (%d : %d) sized %lu\n", buf, in, out, size);
    return buf;
}

void dealloc_buffer(s_buffer* buf){
    printf("freeing buffer %p\n", buf);
    free(buf);
}

int read_to_buf(s_buffer* buf, int fd){
    if(buf->pos < BUF_SIZE ){
    printf("starting pull: %d %p\n", fd, buf);
        int cnt = read(fd, buf->data + buf->pos, BUF_SIZE - buf->pos);
        if(cnt <= 0){
            perror("read() failed");
            return -1;
        }
        else{
            buf->pos += cnt;
            printf("read %d bytes\n", cnt);
            return cnt;
        }
    }
    return 0;
}

int read_from_buffer(s_buffer* buf, int fd){
    if(buf->pos > 0){
    printf("starting push to %d\n", fd);
        int cnt = write(fd, buf->data, buf->pos);
        if(cnt < 0){
            perror("write() failed");
            return -1;
        }
        if(cnt > 0){
            memmove(buf->data, buf->data + cnt, buf->pos - cnt);
        }
        buf->pos -= cnt;
        return cnt;
    }
    return 0;
}

int init_network(char* hostname){
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        exit(1);
    }
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0)
        perror("connect error");
    return sockfd;

}


int main(int argc, char* argv[]){
    int FUCK = 0;
    printf("%s\n", argv[1]);
    int fd = init_network(argv[1]);
    size_t fsize = 0;
    write(fd, argv[2], strlen(argv[2]));
    char comm[2];
    read(fd, &comm, 2);
       if(strcmp(comm, "OK")){
        fsize = 256;
        printf("error\n");
        printf("%d\n", fsize);
        char msg[256];
        bzero(msg, fsize);
        read(fd, &msg, fsize);
        printf("%s\n", msg);
        return -1;
    }
    read(fd, &fsize, sizeof(fsize));
    fsize = ntohs(fsize);
    printf("file size = %d\n", fsize);

    int read_n = 0;
    s_buffer* buf = alloc_buffer(BUF_SIZE, fd, 1);
    while(read_n < fsize){
        read_n += read_to_buf(buf, fd);
        if(read_from_buffer(buf, 1) < 0)
            exit(-1);
    }
    close(fd);
    return 0;
}
