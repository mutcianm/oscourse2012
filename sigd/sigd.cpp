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
#include <string>
#include <map>
#include <list>
#include <queue>
#include <algorithm>
#define PORT "3111"

int write_str(int fd, std::string str){
    return write(fd, str.c_str(), str.length()+1);
}

class client{
    public:
        client(int _fd): fd(_fd){
            std::cout << "new client: " << _fd << std::endl;
        }
        ~client(){
        }
        int commit_message(){
          if(!msgqueue.empty()){
                int ret;
                std::cout << "writing msg:" << msgqueue.front() << std::endl;
                ret = write_str(fd, msgqueue.front());
                msgqueue.pop();
                return ret;
          }
          return 0;
        }
        void add_msg(std::string msg){
            std::cout << "adding " << msg << " to " << fd << std::endl;
            msgqueue.push(msg);
        }
        std::queue<std::string> msgqueue;
        int fd;
    
};


class Signal{
    public:
    Signal(std::string name, client* cli){
        data = name;
        listeners.push_back(cli);
    }
    ~Signal(){
        for(std::list<client*>::iterator it = listeners.begin(); it != listeners.end(); ++it){
            delete *it;
        }
    }
    std::string data;
    std::list<client*> listeners;
    void subscribe(client* cli){
        listeners.push_back(cli);
    }
    bool unsubscribe(client* cli){
        listeners.remove(cli);
        if(listeners.empty())
            return false;
        return true;
    }
    void emit_all(){
        for(std::list<client*>::iterator it = listeners.begin(); it != listeners.end(); ++it){
            (*it)->add_msg(data);
        }
    }
};




class server{
    public:
        server();
        ~server(){}
        void run();
    private:
        const int COMLEN = 3;
        const int SIGLEN = 5;
        const int MAX_LEN = 8;

        std::string read_str(client* cli, int len);
        std::string read_command(client*);
        std::string read_signal(client*);



        void init_net();
        int main_loop();
        void handle_client(pollfd fd);
        std::map<int, client*> clients;
        std::vector<pollfd> pollfds;
        std::vector<int> fds;
        std::list<Signal*> Signals;
        int dpid;

};

server::server(){
    init_net();

}

void server::run(){
    main_loop();
}

void server::init_net(){
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

int server::main_loop(){
    //dpid = fork();
    //if (dpid != 0) {
        //waitpid(dpid, NULL, 0);
        //exit(0);
    //}
    //int sid = setsid();
    //if (sid < 0) {
        //perror("session creation error");
        //exit(1);
    //}
    std::vector<pollfd> pollfds(fds.size());    
    for (int i = 0; i < pollfds.size(); i++) {
        pollfds[i].fd = fds[i];
        pollfds[i].events = POLLIN | POLLOUT | POLLERR | POLLHUP | POLLRDHUP | POLLNVAL;
        pollfds[i].revents = 0;
    }
    for(;;){
        int ret = poll(pollfds.data(), pollfds.size(), -1);
        if(ret == 0) continue;
        for (int i = 0; i < pollfds.size(); i++) {
            if (pollfds[i].revents & (POLLERR | POLLHUP | POLLRDHUP | POLLNVAL)) {
                pollfds[i].events = 0;
                close(pollfds[i].fd);
                pollfds.erase(pollfds.begin()+i);
                perror("socket has been disconnected"); //this shoud not be reachable
                //_xit(1);
                continue;
            }
            if ((pollfds[i].revents & POLLIN) || (pollfds[i].revents & POLLOUT)) {
                client* cli = clients[pollfds[i].fd];
                if(cli){
                    //printf("handling client: %d\n", cli->fd);
                    handle_client(pollfds[i]);
                } else {
                    printf("new connection: %d\n", pollfds[i].fd);
                    int cfd = accept(pollfds[i].fd, NULL, NULL);
                    pollfd tmp;
                    tmp.events = POLLIN | POLLOUT | POLLERR | POLLHUP | POLLRDHUP | POLLNVAL;
                    tmp.fd = cfd;
                    pollfds.push_back(tmp);
                    clients[cfd] = new client(cfd);

                }
            }
        }
    }
}

void server::handle_client(pollfd fd){
    client* cli = clients[fd.fd];
    if(fd.revents & POLLIN){
        std::string comm = read_str(cli, MAX_LEN);
        std::cout << comm << std::endl;
        if(comm.substr(0, COMLEN) == "sub"){
            std::string sig = comm.substr(3, SIGLEN);
            bool found = false;
            for(std::list<Signal*>::iterator it = Signals.begin(); it != Signals.end(); ++it){
                if((*it)->data == sig){
                    std::cout << "subscribing client " << cli->fd << " on signal " << sig << std::endl;
                    (*it)->subscribe(cli);
                    found = true;
                    break;
                }
            }
            if(!found){
                Signal* signl = new Signal(sig, cli);
                Signals.push_back(signl);
                std::cout << "new signal " << sig << std::endl;
            }
        } else if(comm.substr(0, COMLEN) == "emt"){
            std::string sig = comm.substr(3, SIGLEN);
            bool found = false;
            for(std::list<Signal*>::iterator it = Signals.begin(); it != Signals.end(); ++it){
                if((*it)->data == sig){
                    std::cout << "emiting client " << cli->fd << " on signal " << sig << std::endl;
                    (*it)->emit_all();
                    found = true;
                    break;
                }
            }
            if(!found){
                //std::cout << "signal " << sig << "not found!" << std::endl;
                cli->add_msg(sig+" not found!");
            }

        } else if(comm.substr(0, COMLEN) == "uns"){
            std::string sig = comm.substr(3, SIGLEN);
            bool found = false;
            for(std::list<Signal*>::iterator it = Signals.begin(); it != Signals.end(); ++it){
                if((*it)->data == sig){
                    std::cout << "unsubscribing client " << cli->fd << " on signal " << sig << std::endl;
                    if(!(*it)->unsubscribe(cli)){
                        Signals.erase(it);
                    }
                    found = true;
                    break;
                }
            }
            if(!found){
                 cli->add_msg(sig+" not found!");

            }
  
        } else if(comm.substr(0, COMLEN) == "lst"){
            for(std::list<Signal*>::iterator it = Signals.begin(); it != Signals.end(); ++it){
                std::cout << "list request\n";
                cli->add_msg((*it)->data);
            }
        }
    }
    if(fd.revents & POLLOUT){
        cli->commit_message();
    }
}
std::string server::read_str(client* cli, int len){
    char* tmp = new char[len];
    bzero(tmp, len);
    int cnt = read(cli->fd, tmp,len);
    if(cnt < 0){
        perror("read error");
    }
    return tmp;

}
std::string server::read_command(client* cli){
    char* tmp = new char[COMLEN];
    int cnt = read(cli->fd, tmp, COMLEN);
    if(cnt < 0){
        perror("read error");
    }
    return tmp;
}

std::string server::read_signal(client* cli){
    char* tmp = new char[COMLEN];
    int cnt = read(cli->fd, tmp, SIGLEN);
    if(cnt < 0){
        perror("read error");
    }
    return tmp;
}




int main(int argc, char** argv){
    server* srv = new server();
    srv->run();
    delete srv;

    return 0;
}

