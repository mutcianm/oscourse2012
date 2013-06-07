#include<sys/types.h>
#include<sys/time.h>
#include<sys/resource.h>
#include<sys/wait.h>
#include<sys/socket.h>
#include<netdb.h>
#include<stdlib.h>
#include<poll.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<map>
#include<vector>

const int BUF_SIZE = 32*1024*1024;
const int MAX_TAG_LEN = 4;
int ID = 0;

#define PORT "3111"

struct s_buffer {
    char* data;
    size_t pos;
    char in_died;
    char out_died;
    int tag;
};

using namespace std;

vector<int> fds;
vector<pollfd> polls;
map<int, s_buffer*> buffers;
vector<bool> senders;
vector<int> ids;


int pull_data(pollfd* in, s_buffer* buf){
    printf("starting pull: %d %d\n", in, buf);
    if(buf->in_died) return 0;
    if(buf->pos < BUF_SIZE && in->revents & POLLIN){
        int cnt = read(in->fd, buf->data + buf->pos, BUF_SIZE - buf->pos);
        if(cnt <= 0){
            perror("read() failed");
            buf->in_died = true;
            return 0;
        }
        else{
            buf->pos += cnt;
            printf("read %d bytes\n", cnt);
        }
        if(in->revents & POLLERR){
            printf("POLLERR on in\n");
            buf->in_died = true;
        }
    }
    return 1;
}

int push_data(pollfd* out, s_buffer* buf){
    printf("starting push\n");
    if(buf->out_died) return 0;
    if(buf->pos > 0){
        int cnt = write(out->fd, buf->data, buf->pos);
        if(cnt <= 0){
            buf->out_died = true;
            perror("write() failed");
            return 0;
        }
        if(cnt > 0){
            memmove(buf->data, buf->data + cnt, buf->pos - cnt);
        }
        buf->pos -= cnt;
    }
    if(buf->in_died && buf->pos == 0){
        printf("POLLERR on out\n");
        buf->out_died = true;
    }
    printf("buff pos = %d\n", buf->pos);
    return 1;
}

s_buffer* allocate_buffer(int tag){
    s_buffer* buf = (s_buffer*)malloc(sizeof(s_buffer));
    buf->tag = tag;
    buf->data = (char*)malloc(BUF_SIZE);
    buf->pos = 0;
    buf->out_died = false;
    buf->in_died = false;
    return buf;
}

void dealloc_buffer(s_buffer* buf){
    printf("deallocating buffer %s\n", buf->data);
    free(buf->data);
    free(buf);
}

void init_network(){
    struct addrinfo hints;
    struct addrinfo* srvinfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(NULL, PORT, &hints, &srvinfo) != 0){
        perror("getaddrinfo() failed");
    }
    while(srvinfo){
        fds.push_back(socket(srvinfo->ai_family, srvinfo->ai_socktype, srvinfo->ai_protocol));
        bind(fds.back(), srvinfo->ai_addr, srvinfo->ai_addrlen);
        int yes = 1;
        setsockopt(fds.back(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        listen(fds.back(), 5);
        srvinfo = srvinfo->ai_next;
        printf("initializes socket %d\n", fds.back());
    }
    freeaddrinfo(srvinfo);


}


void handle_client(int fd){
    printf("handling client: %d\n", fd);
    char cmd[5];
    read(fd, cmd, 5);
    pollfd client;
    if(!strcmp(cmd, "send")){
        printf("sender connectedi\n");
        client.fd = fd;
        client.events = POLLIN | POLLERR;
        polls.push_back(client);
        ids.push_back(ID);
        senders.push_back(true);
        buffers[ID] = allocate_buffer(ID);
        memset(cmd, 0, 5);
        sprintf(cmd, "%d", ID);
        ID++;
        write(fd, cmd, 5);
    }
    else if(!strcmp(cmd, "recv")){
        printf("recvr connected\n");
        client.fd = fd;
        client.events = POLLOUT | POLLERR;
        polls.push_back(client);
        read(fd, cmd, 5);
        int id = atoi(cmd);
        ids.push_back(id);
        senders.push_back(false);
    }
    else {
        close(fd);
        printf("unknown command: %s\n", cmd);
        return;
    }
}

void main_srv(){
    init_network();
    vector<pollfd> pollfds(fds.size());
    for(int i = 0; i < pollfds.size(); ++i){
        pollfds[i].fd = fds[i];
        pollfds[i].events = POLLIN | POLLERR | POLLHUP | POLLRDHUP | POLLNVAL;
    }
    while (true){
        int ret = poll(pollfds.data(), pollfds.size(), 1);
        if(ret < 0){
            perror("poll() failed");
            exit(1);
        }
        int status;
        wait3(&status, WNOHANG, NULL);
        if(ret == 0) continue;
        for(int i = 0; i < pollfds.size(); ++i){
            if(pollfds[i].revents & (POLLERR | POLLHUP | POLLRDHUP | POLLNVAL)){
                pollfds[i].events = 0;
                close(pollfds[i].fd);
                perror("socket has been disconnected");
            }
            if(pollfds[i].revents & POLLIN){
                int cfd = accept(fds[i], NULL, NULL);
                if (cfd < 0){
                    perror("accept() failed");
                    continue;
                }
                printf("accepting %d\n", cfd);
                handle_client(cfd);
            }
        }
        int sret = poll(polls.data(), polls.size(), 1);
        if(sret < 0){
            perror("polls() error");
            exit(1);
        }
        if(sret == 0) continue;
        for(int i = 0; i < polls.size(); ++i){
            if(polls[i].revents & POLLERR){
                printf("socket disconnected: %d\n", polls[i].fd);
                //close(polls[i].fd);
                //continue;
            }
            if(senders[i]){
                if(polls[i].revents){
                    printf("pulling data from sender id: %d\n", ids[i]);
                    pull_data(&polls[i], buffers[ids[i]]);
                }
            } else {
                if(polls[i].revents){
                    printf("pushing data\n");
                    push_data(&polls[i], buffers[ids[i]]);
                }
            }
        }
        for(int i = 0; i < polls.size(); ++i){
            if(senders[i]){
                if(buffers[ids[i]]->in_died && buffers[ids[i]]->out_died){
                    printf("sending finished6 %d\n", ids[i]);
                    polls[i].events = 0;
                    close(polls[i].fd);
                }
            } else {
                if(buffers[ids[i]]->out_died){
                    printf("recving finished: %d\n", ids[i]);
                    polls[i].events = 0;
                   // if(buffers[ids[i]] != 0)
                       // dealloc_buffer(buffers[ids[i]]);
                    close(polls[i].fd);
                }
            }
        }
    }
}

int main(int argc, char** argv){
    main_srv();
return 0;
}
