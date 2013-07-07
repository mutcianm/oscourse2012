#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<sys/types.h>
#include<stdlib.h>
#include<cstring>
#include<vector>

std::vector<int> init_net(const char* PORT){
    std::vector<int> fds;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    int status;
    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_UNSPEC; //don’tcareIPv4orIPv6
    hints.ai_socktype=SOCK_STREAM;//TCPstreamsockets
    hints.ai_flags=AI_PASSIVE; //fillinmyIPforme
    if ((status=getaddrinfo(NULL, PORT,&hints,&servinfo)) !=0){
        fprintf(stderr,"getaddrinfo error:%s\n",strerror(
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
    return fds;

}

