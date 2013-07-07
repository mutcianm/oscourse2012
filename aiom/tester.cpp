#include"autofd.h"
#include<cstring>
#include<unistd.h>
#include<iostream>
#include"aepoll.h"
#include"net.h"
#include"async_op.h"

const int BUF_LEN = 16;

void handle_read(aepollfd* aep, int fd){
    char buf[BUF_LEN];
    bzero(buf, BUF_LEN);
    int cnt = read(fd, buf, BUF_LEN);
    std::string str(buf);
    std::cout << "from stdin: " << str;
    if(str.find("quit") != std::string::npos)
        aep->set_done();
}

void handle_net(aepollfd* aep, int cfd){
    int fd = accept(cfd, NULL, NULL);
    char buf[BUF_LEN];
    bzero(buf, BUF_LEN);
    int cnt = read(fd, buf, BUF_LEN);
    std::string str(buf);
    std::cout << "from NET: " << str << std::endl;
    if(str.find("quit") != std::string::npos)
        aep->set_done();
    close(fd);
}

int main(int argc, char** argv){
    autofd a(1);
    autofd b(autofd(3));
    //autofd c(a); //compilation error - nocopyable
    autofd c = std::move(a);
    aepollfd aep;
    std::vector<int> fds = init_net("3111");
    std::cout << fds.size() << std::endl;
    async_op op(aep, fds[0], EPOLLIN, NULL);
    async_op op1(aep, fds[1], EPOLLIN, NULL);
    aep.subscribe(autofd(0), EPOLLIN, std::bind(handle_read, &aep, std::placeholders::_1));
    //for(int i = 0; i < fds.size(); ++i)
        //aep.subscribe(autofd(fds[i]), EPOLLIN, std::bind(handle_net, &aep, std::placeholders::_1));
    aep.run();    
    return 0;
}
