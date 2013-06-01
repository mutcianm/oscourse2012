#include<sys/socket.h>
#include<netdb.h>
#include<stdlib.h>
#include<poll.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<map>
#include<vector>

const int BUF_SIZE = 2048;

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

int prpagate_data(pollfd* src, s_buffer* buf, pollfd* dst){

}

s_buffer* allocate_buffer(int tag){

}

void dealloc_buffer(s_buffer* buf){

}

void init_network(){
    struct addrinfo hints;
    struct addrinfo* srvinfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (!getaddrinfo(NULL, PORT, &hints, &srvinfo)){
        perror("getaddrinfo() failed");
        exit(1);
    }
    while(srvinfo){
        fds.push_back(socket(srvinfo->ai_family, srvinfo->ai_socktype, srvinfo->ai_protocol));
        bind(fds.back(), srvinfo->ai_addr, srvinfo->ai_addrlen);
        int yes = 1;
        setsockopt(fds.back(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        listen(fds.back(), 5);
        srvinfo = srvinfo->ai_next;
    }
    freeaddrinfo(srvinfo);


}

void handle_sender(int fd){
    printf("handling sender: %d\n", fd);

}

void handle_recvr(int fd){
    printf("handling recvr: %d\n", fd);
}

void handle_client(int fd){
    char[5] cmd;
    read(fd, cmd, 5);
    if(!strcmp(cmd, "send"))
        handle_sender(fd);
    else if(!strcmp(cmd, "recv"))
        handle_recvr(fd);
    else {
        close(fd);
        pirntf("unknown command: %s\n", cmd);
    }
}

void main_srv(){
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
        for(int i = 0; i < pollfds.size(), ++i){
            if(pollfds[i].revents & (POLLERR | POLLHUP | POLLRDHUP | POLLNVAL)){
                pollfds[i].events = 0;
                close(fds[i]);
                perror("socket has been disconnected");
            }
            if(pollfds[i].revents & POLLIN){
                int cfd = accept(fds[i], NULL, NULL);
                if (cfd < 0){
                    perror("accept() failed");
                    continue;
                }
                printf("accepting %d\n", cfd);
                int pid = fork();
                if(pid = 0){
                    for(int i = 0; i < fds.size(); ++i){
                        close(fds[i]);
                    }
                    handle_client(cfd);
                    return;
                }
                close(cfd);
            }
        }
    }
}

int main(int argc, char** argv){

return 0;
}
