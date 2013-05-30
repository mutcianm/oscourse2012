#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <netdb.h>
#include <pty.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <vector>
#include <iostream>
#include <pty.h>
 
const int BUF_SIZE = 4096;
 
struct s_buffer {
    char buff[BUF_SIZE];
    size_t pos;
    char in_dead;
    char out_died;
    bool is_EOF;
};
 
using namespace std;
 
int pull_data(pollfd *src, s_buffer *buff, pollfd *dest) {
    if ((buff->pos < BUF_SIZE) && (src->revents & POLLIN)) {
            int cnt =  read(src->fd, buff->buff + buff->pos, BUF_SIZE - buff->pos);
            if (cnt <= 0) {
                buff->is_EOF = true;
                src->events ^= POLLIN;
            } else {
                buff->pos+=cnt;
            }
    }
 
    if ((buff->pos > 0) && (dest->revents & POLLOUT)) {
            int cnt = write(dest->fd, buff->buff, buff->pos);
            if (cnt < 0) {
                perror("write error");
                exit(1);
            }
            if (cnt > 0) {
                memmove(buff->buff, buff->buff+cnt, buff->pos - cnt);
                buff->pos -= cnt;
            }      
            buff->pos-=cnt;
    }
 
    if (buff->pos < BUF_SIZE && !buff->in_dead) {
        src->events |= POLLIN;
    } else {
        src->events &= ~POLLIN;
    }
    if(buff->out_died){
        dest->events = 0;
    }
 
    return 0;
}
 
void handle_client(int cfd) {
    int amaster = 0;
    int aslave = 0;
    char name[4096];
    if (openpty(&amaster, &aslave, name, NULL, NULL) == -1) {
        perror("openpty failed");
        exit(1);    
    }
    int shellpid = fork();
    if (shellpid != 0) {
        close(aslave);
        pollfd pfds[2];
        pfds[0].fd = cfd;
        pfds[0].events = POLLIN | POLLERR | POLLOUT;
        pfds[0].revents = 0;
        pfds[1].fd = amaster;
        pfds[1].events = POLLIN | POLLERR | POLLOUT;
        pfds[1].revents = 0;
        int ret;
        struct s_buffer from;
        //from.buff = (char*)malloc(BUF_SIZE);
        memset(from.buff, BUF_SIZE, 0);
        struct s_buffer to;
        //jbufffrm.buff = (char*)malloc(BUF_SIZE);
        memset(to.buff, BUF_SIZE, 0);
        while (!from.in_dead && !to.in_dead) {
            ret = poll(pfds, 2, 0);
            if (!ret) {
                continue;
            }
            if (ret < 0) {
                //error
                perror("something bad with poll");
                exit(1);
            }
            pull_data(&pfds[0],&from, &pfds[1]);
            pull_data(&pfds[1], &to, &pfds[0]);
            sleep(1);
        }
        exit(0)
    }
    setsid();
    close(amaster);
    close(cfd);
    dup2(aslave, 0);
    dup2(aslave, 1);
    dup2(aslave, 2);
    close(aslave);
   int fd = open(name, O_RDWR);
    close(fd);
    execlp("sh", "sh",  NULL);
 

    //Oup2(cfd, 0);
    //dup2(cfd, 1);
    //dup2(cfd, 2);
    // close(cfd);
    //execvp("cat", "cat", "-", NULL);
    //execlp("echo", "echo", "hello", NULL);
    exit(0);
}
 
int main()
{
    int dpid = fork();
    if (dpid != 0) {
        waitpid(dpid, NULL, 0);
        exit(0);
    }
    int sid = setsid();
    if (sid < 0) {
        perror("session creation error");
        exit(1);
    }
    struct addrinfo hints;
    struct addrinfo *servinfo;
    int status;
    vector<int> fds;
    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_UNSPEC; //don’tcareIPv4orIPv6
    hints.ai_socktype=SOCK_STREAM;//TCPstreamsockets
    hints.ai_flags=AI_PASSIVE; //fillinmyIPforme
    if ((status=getaddrinfo(NULL,"3491",&hints,&servinfo)) !=0){
        fprintf(stderr,"getaddrinfo error:%s\n",gai_strerror(
                    status));
        exit(1);
    }
    while (servinfo) {
        if (servinfo == NULL)
        {
            printf("servinfo == NULL\n");
            exit(1);
        }
        fds.push_back(socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol));
        if (fds.back() < 0)
        {
            perror("sockfd < 0");
            exit(1);
        }
        if (bind(fds.back(), servinfo->ai_addr, servinfo->ai_addrlen) < 0)
        {
           // perror("bind < 0");
           // exit(1);
        }
        int yes = 1;
        if (setsockopt(fds.back(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
        {
            perror("setsockopt < 0");
            exit(1);
        }
        if (listen(fds.back(), 5) < 0)
        {
            perror("listen < 0");
            exit(1);
        }
        servinfo = servinfo->ai_next;
    }
    freeaddrinfo(servinfo);
    if (fds.empty()) {
        perror("no binds");
        exit(1);
    }
    int fd = -1;
    vector<pollfd> pollfds(fds.size());
    for (int i = 0; i < pollfds.size(); i++) {
        pollfds[i].fd = fds[i];
        pollfds[i].events = POLLIN | POLLERR | POLLHUP | POLLRDHUP | POLLNVAL;
        pollfds[i].revents = 0;
    }
 
    while (true) {
        int ret = poll(pollfds.data(), pollfds.size(), 0);
        if (ret < 0) {
            //error
            perror("something bad with poll");
            exit(1);
        }
        if (ret == 0) {
            continue;
        }
        for (int i = 0; i < pollfds.size(); i++) {
            if (pollfds[i].revents & (POLLERR | POLLHUP | POLLRDHUP | POLLNVAL)) {
                //pollfds[i].events = 0;
                //close(fds[i]);
                cerr << "listen error" << endl;
                exit(1);
            }
            if (pollfds[i].revents & POLLIN) {
                int cfd = accept(fds[i], NULL, NULL);
                if (cfd < 0) {
                    perror("fd < 0");
                    continue;
                }
                int pid = fork();
                if (pid == 0) {
                    for (int i = 0; i < fds.size(); i++) {
                        close(fds[i]);
                    }
                    handle_client(cfd);
                }
                close(cfd);
            }
        }
    }
}


