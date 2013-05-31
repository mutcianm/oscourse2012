#include<stdlib.h>
#include<poll.h>
#include<fcntl.h>
#include<stdio.h>
#include<sys/stat.h>
#include<sys/sendfile.h>
#include<sys/epoll.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>

int main(int argc, char** argv){
    if((argc-1) % 2)
    /*if(argc < 3)*/
        exit(-1);
    int n = (argc - 1) / 2;
    int efd = epoll_create(2*n);
    struct epoll_event* fds = (struct epoll_event*)malloc(2*n*sizeof(struct epoll_event));
    struct epoll_event* events = (struct epoll_event*)malloc(2*n*sizeof(struct epoll_event));
    for(int i = 0; i < n; ++i){
        fds[2*i].data.fd = atoi(argv[2*i]+1);
        fds[2*i].events = POLLIN;
        fds[2*i + 1].data.fd = atoi(argv[2*i + 2]);
        fds[2*i + 1].events = POLLOUT;
    }
    printf("descr: %d %d\n", fds[0].data.fd, fds[1].data.fd);
    for(int i = 0; i < 2*n; ++i){
        if(epoll_ctl(efd, EPOLL_CTL_ADD, fds[i].data.fd, &fds[i]) < 0){
            printf("epoll_ctl failed: %s\n", strerror(errno));
            exit(-1);
        }
    }
    int num_left = n;
    while(num_left > 0){
        int res = epoll_wait(efd, events, 2*n, -1); //wait until interrupted
        if(res < 0){
            printf("poll exited with error: %s\n", strerror(errno));
            exit(-1);
        }
        for(int i = 0; i < 2*n; ++i){
            if((events[2*i].events & POLLIN) && (events[2*i + 1].events & POLLOUT)){
                // linux-way
                struct stat stat_src;
                fstat(events[2*i].data.fd, &stat_src);
                if(sendfile(events[2*i + 1].data.fd, events[2*i].data.fd, 0 , stat_src.st_size) < 0){
                    printf("sendfile exited with error: %s\n", strerror(errno));
                }
                epoll_ctl(efd, EPOLL_CTL_DEL, events[2*i].data.fd, &events[2*i]);
                epoll_ctl(efd, EPOLL_CTL_DEL, events[2*i+1].data.fd, &events[2*i+1]);
                /*fds[2*i].events = fds[2*i + 1].events = 0;*/
                num_left--;
            }
        }
    }
    return 0;     
}
