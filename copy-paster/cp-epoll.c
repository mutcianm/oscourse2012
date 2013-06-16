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
    int fd_in;
    int fd_out;
};


s_buffer* alloc_buffer(size_t size, int in, int out){
    s_buffer* buf = (s_buffer*)malloc(sizeof(s_buffer));
    buf->data = (char*)malloc(size);
    bzero(buf->data, size);
    buf->pos = 0;
    buf->fd_in = in;
    buf->fd_out = out;
    printf("allocated buffer %p for (%d : %d) sized %lu\n", buf, in, out, size);
    return buf;
}

void dealloc_buffer(s_buffer* buf){
    printf("freeing buffer %p\n", buf);
    free(buf->data);
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

void remove_pair(int efd, s_buffer* buf){
    printf("IO finished for buffer %p; (%d : %d)\n", buf, buf->fd_in, buf->fd_out);
    epoll_ctl(efd, EPOLL_CTL_DEL, buf->fd_out, NULL);
    epoll_ctl(efd, EPOLL_CTL_DEL, buf->fd_in, NULL);
    dealloc_buffer(buf);
}


int main(int argc, char** argv){
    if((argc-1) % 2)
        exit(-1);
    int n = (argc - 1) / 2;
    int efd = epoll_create(2*n);
    struct epoll_event* fds = (struct epoll_event*)malloc(2*n*sizeof(struct epoll_event));
    struct epoll_event* events = (struct epoll_event*)malloc(2*n*sizeof(struct epoll_event));
    for(int i = 0; i < n; ++i){
        s_buffer* buf = alloc_buffer(BUF_SIZE, atoi(argv[2*i+1]), atoi(argv[2*i+2]));
        fds[2*i].data.ptr = buf;
        fds[2*i].events = EPOLLIN  | EPOLLHUP | EPOLLERR;
        fcntl(buf->fd_out, F_SETFL, O_NONBLOCK);
        fcntl(buf->fd_in, F_SETFL, O_NONBLOCK);
        if(epoll_ctl(efd, EPOLL_CTL_ADD, buf->fd_in, &fds[2*i]) < 0){
            printf("epoll_ctl failed on %d: %s\n", buf->fd_in, strerror(errno));
            exit(-1);
        }
        fds[2*i + 1].data.ptr = buf;
        fds[2*i + 1].events = EPOLLOUT | EPOLLHUP | EPOLLERR;
        if(epoll_ctl(efd, EPOLL_CTL_ADD, buf->fd_out, &fds[2*i+1]) < 0){
            printf("epoll_ctl failed on %d: %s\n", buf->fd_out, strerror(errno));
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
            if(events[i].events & EPOLLOUT){
                s_buffer* buf = (s_buffer*)events[i].data.ptr;
                int ret = read_from_buffer(buf, buf->fd_out);
                if(ret > 0){
                    continue;
                }
                if(ret < 0){
                    /*remove_pair(efd, buf);*/
                    /*num_left--;*/
                    break;
                }
            }
            if(events[i].events & EPOLLIN){
                s_buffer* buf = (s_buffer*)events[i].data.ptr;
                int ret = read_to_buf(buf, buf->fd_in);
                if(ret > 0){
                    continue;
                }
                if(ret < 0){
                    /*remove_pair(efd, buf);*/
                    /*num_left--;*/
                    break;
                }
            }
            if((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)){
                printf("descriptor error: %X\n", events[i].events & (EPOLLHUP | EPOLLERR));
                remove_pair(efd, (s_buffer*)events[i].data.ptr);
                num_left--;
            }
        }
    }
    return 0;     
}
