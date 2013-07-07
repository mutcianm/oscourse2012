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

std::vector<int> fds;

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
    printf("starting push\n");
        int cnt = write(fd, buf->data, buf->pos);
        if(cnt <= 0){
            perror("write() failed");
            return -1;
        }
        if(cnt > 0){
            memmove(buf->data, buf->data + cnt, buf->pos - cnt);
        }
        buf->pos -= cnt;
    }
    return 0;
}


void init_net(){
    struct addrinfo hints;
    struct addrinfo *servinfo;
    int status;
    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_UNSPEC; //don’tcareIPv4orIPv6
    hints.ai_socktype=SOCK_STREAM;//TCPstreamsockets
    hints.ai_flags=AI_PASSIVE; //fillinmyIPforme
    if ((status=getaddrinfo(NULL, PORT,&hints,&servinfo)) !=0){
        fprintf(stderr,"getaddrinfo error:%s\n",gai_strerror(
                    status));
        exit(1);
    }
    while (servinfo) {
        fds.push_back(socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol));
        if (fds.back() < 0)
        {
            perror("sockfd < 0");
            exit(1);
        }
        bind(fds.back(), servinfo->ai_addr, servinfo->ai_addrlen);
        int yes = 1;
        if (setsockopt(fds.back(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
        {
            perror("setsockopt < 0");
            exit(1);
        }
        if (listen(fds.back(), 5) < 0)
        {
            perror("listen() failed");
            exit(1);
        }
        servinfo = servinfo->ai_next;
    }
    freeaddrinfo(servinfo);
    if (fds.empty()) {
        perror("no interfaces to bind on");
        exit(1);
    }
}

void handle_client(int cfd){
    char name[1024];
    bzero(name, 1024);
    read (cfd, &name, 1024);
    printf("%s\n", name);
    int fret = open(name, O_RDONLY);
    if (fret < 0){
        //perror("file error");
        write(cfd, "ER", 2);
        char* err = strerror(errno);
        write(cfd, err, strlen(err));
        printf("%s", err);

        close(cfd);
        return;
    }
    printf("asdfasdfsaf\n");
    write(cfd, "OK", 2);
    struct stat stat_src;
    fstat(fret, &stat_src);
    size_t num = htons(stat_src.st_size);
    printf("sending file size: %d\n", stat_src.st_size);
    write(cfd, &num, sizeof(num));
    s_buffer* buf = alloc_buffer(256, fret, cfd);
    int bufret = 1;
    while(bufret > 0){
        bufret = read_to_buf(buf, fret);
        bufret = read_from_buffer(buf, cfd);
    }
    dealloc_buffer(buf);
    close(cfd);
}
 int dpid;

void handler(int){
    kill(dpid, SIGTERM);
}
int main(int argc, char** argv){
    dpid = fork();
    if (dpid != 0) {
        waitpid(dpid, NULL, 0);
        //int status;
        //wait3(&status, WNOHANG, NULL);
        exit(0);
    }
    int sid = setsid();
    init_net();
    if (sid < 0) {
        perror("session creation error");
        exit(1);
    }
    std::vector<pollfd> pollfds(fds.size());    
    for (int i = 0; i < pollfds.size(); i++) {
        pollfds[i].fd = fds[i];
        pollfds[i].events = POLLIN | POLLERR | POLLHUP | POLLRDHUP | POLLNVAL;
        pollfds[i].revents = 0;
    }
    for(;;){
        int ret = poll(pollfds.data(), pollfds.size(), -1);
        int status;
        wait3(&status, WNOHANG, NULL);
        if(ret == 0) continue;
        for (int i = 0; i < pollfds.size(); i++) {
            if (pollfds[i].revents & (POLLERR | POLLHUP | POLLRDHUP | POLLNVAL)) {
                pollfds[i].events = 0;
                close(fds[i]);
                perror("socket has been disconnected"); //this shoud not be reachable
                _exit(1);
            }
            if (pollfds[i].revents & POLLIN) {
                int cfd = accept(fds[i], NULL, NULL);
                if (cfd < 0) {
                    perror("fd < 0");
                    continue;
                }
                printf("Accepting %d\n", cfd);
                int pid = fork();
                if (pid == 0) {
                    for (int i = 0; i < fds.size(); i++) {
                        close(fds[i]);
                    }
                    handle_client(cfd);
                    return 0;
                }
                close(cfd);
            }
        }
        
    }
}
