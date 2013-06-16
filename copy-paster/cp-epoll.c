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

const int BUF_SIZE = 2048;

struct s_buffer{
    char* data;
    size_t pos;
};


s_buffer* alloc_buffer(size_t size){
    s_buffer* buf = (s_buffer*)malloc(sizeof(s_buffer));
    buf->data = (char*)malloc(size);
    bzero(buf->data, size);
    data->pos = 0;
    return buf;
}

void dealloc_buffer(s_buffer* buf){
    free(buf->data);
    free(buf);
}

int read_to_buf(s_buffer* buf, int fd){
    printf("starting pull: %d %d\n", fd, buf);
    if(buf->pos < BUF_SIZE ){
        int cnt = read(fd, buf->data + buf->pos, BUF_SIZE - buf->pos);
        if(cnt <= 0){
            perror("read() failed");
            return 0;
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
    printf("starting push\n");
    if(buf->pos > 0){
        int cnt = write(fd, buf->data, buf->pos);
        if(cnt <= 0){
            perror("write() failed");
            return 0;
        }
        if(cnt > 0){
            memmove(buf->data, buf->data + cnt, buf->pos - cnt);
        }
        buf->pos -= cnt;
    }
    printf("buff pos = %d\n", buf->pos);
    return 1;
}



int main(int argc, char** argv){
    if((argc-1) % 2)
        exit(-1);
    int n = (argc - 1) / 2;
    int efd = epoll_create(2*n);
    struct epoll_event* fds = (struct epoll_event*)malloc(2*n*sizeof(struct epoll_event));
    struct epoll_event* events = (struct epoll_event*)malloc(2*n*sizeof(struct epoll_event));
    for(int i = 0; i < n; ++i){
        s_buffer* buf = alloc_buffer(BUF_SIZE);
        fds[2*i].data = buf;
        fds[2*i].events = POLLIN | EPOLLET;
        if(epoll_ctl(efd, EPOLL_CTL_ADD, atoi(argv[2*i]+1), &fds[2*i]) < 0){
            printf("epoll_ctl failed: %s\n", strerror(errno));
            exit(-1);
        }
        fds[2*i + 1].data = buf;
        fds[2*i + 1].events = POLLOUT;
        if(epoll_ctl(efd, EPOLL_CTL_ADD, atoi(argv[2*i]+2), &fds[2*i+1]) < 0){
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
