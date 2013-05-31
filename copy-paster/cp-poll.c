#include<stdlib.h>
#include<poll.h>
#include<fcntl.h>
#include<stdio.h>
#include<sys/stat.h>
#include<sys/sendfile.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>

int main(int argc, char** argv){
    if((argc-1) % 2)
        exit(-1);
    int n = (argc - 1) / 2;
    struct pollfd* fds = (struct pollfd*)malloc(2*n*sizeof(struct pollfd));
    for(int i = 0; i < n; ++i){
        fds[2*i].fd = atoi(argv[2*i+1]);
        fds[2*i].events = POLLIN;
        fds[2*i + 1].fd = atoi(argv[2*i + 2]);
        fds[2*i + 1].events = POLLOUT;
    }
    printf("descr: %d %d\n", fds[0].fd, fds[1].fd);
    int num_left = n;
    while(num_left > 0){
        int res = poll(fds, 2*n, -1); //wait until interrupted
        if(res < 0){
            printf("poll exited with error: %s\n", strerror(errno));
            exit(-1);
        }
        /*printf("polling %d %d\n", fds[0].revents, fds[1].revents);*/
        for(int i = 0; i < 2*n; ++i){
            if((fds[2*i].revents & POLLIN) && (fds[2*i + 1].revents & POLLOUT)){
                // linux-way
                struct stat stat_src;
                fstat(fds[2*i].fd, &stat_src);
                if(sendfile(fds[2*i + 1].fd, fds[2*i].fd, 0 , stat_src.st_size) < 0){
                    printf("sendfile exited with error: %s\n", strerror(errno));
                }
                fds[2*i].events = fds[2*i + 1].events = 0;
                num_left--;
            }
        }
    }
    return 0;     
}
