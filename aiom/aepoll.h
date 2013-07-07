#ifndef AEPOLL_H
#define AEPOLL_H
#include<functional>
#include<vector>
#include<algorithm>
#include<memory>
#include<map>
#include<exception>
#include<sys/epoll.h>
#include<cstring>
#include<errno.h>
#include"autofd.h"

typedef std::function<void(int)> callback;

//struct epl_data{
    //std::shared_ptr<std::map<std::pair<int>, callback> > eventmap;
//};

class aepoll_exception: public std::exception{
    public:
        std::string msg;
        aepoll_exception(std::string _msg) noexcept : msg(_msg) {}
        const char* what() const noexcept { return msg.c_str(); }
};


class aepollfd{
    public:
        aepollfd(): done(false), timeout(-1) {
            efd = epoll_create1(0);
            if ( efd < 0 ){
                throw aepoll_exception("epoll_create() < 0");
            }
        }
        aepollfd(int _timeout): done(false), timeout(_timeout) {
            efd = epoll_create1(0);
            if ( efd < 0 ){
                throw aepoll_exception("epoll_create() < 0");
            }
        }
        void subscribe(autofd afd, uint32_t new_events, callback cb){
            tmp.data.fd = *afd;
            if(events.find(*afd) != events.end()){
                tmp.events = new_events | events[*afd];
                uint32_t old_events = events[*afd];
                int ret = epoll_ctl(efd, EPOLL_CTL_MOD, *afd, &tmp);
                if(ret < 0)
                    throw aepoll_exception("epoll_ctl() MOD failed!");
                events[*afd] = new_events | events[*afd];
                try{
                    listeners[std::make_pair(*afd, new_events)] = cb;
                } catch (std::bad_alloc& e){
                    std::cerr << "excetion caught while modifying callback: " << e.what() << std::endl;
                    tmp.events = old_events;
                    if(epoll_ctl(efd, EPOLL_CTL_MOD, *afd, &tmp) < 0)
                        throw aepoll_exception("epoll_ctl() MOD failed!");
                    events[*afd] = old_events;
                }
            } else {
                tmp.events = new_events;
                if(epoll_ctl(efd, EPOLL_CTL_ADD, *afd, &tmp) < 0)
                    throw aepoll_exception(std::string("epoll_ctl() ADD failed: ")+strerror(errno));
                try {
                    events[*afd] = new_events;
                    listeners[std::make_pair(*afd, new_events)] = cb;
                } catch (std::bad_alloc& e){
                    std::cerr << "excetion caught while adding callback: " << e.what() << std::endl;
                    if(epoll_ctl(efd, EPOLL_CTL_MOD, *afd, &tmp) < 0)
                        throw aepoll_exception("epoll_ctl() MOD failed!");
                }
            }
        }
        void unsubscribe(autofd afd, uint32_t flags){
            if(listeners.find(std::make_pair(*afd,flags)) != listeners.end()){
                if(events[*afd] == flags){
                    epoll_ctl(efd, EPOLL_CTL_DEL, *afd, NULL);
                    events.erase(events.find(*afd));
                    listeners.erase(listeners.find(std::make_pair(*afd, flags)));
                } else {
                    epoll_event tmp;
                    tmp.data.fd = *afd;
                    tmp.events = events[*afd] ^ flags;
                    epoll_ctl(efd, EPOLL_CTL_MOD, *afd, &tmp);
                    listeners.erase(listeners.find(std::make_pair(*afd, flags)));
                }
            } else {
                throw aepoll_exception("unsubscribe failed: (fd, flags) pair not found!");
            }
        }

        void run(){
            while(!done){
                int n = epoll_wait(efd, fevents, MAX_EVENTS, -1);
                for (int i = 0; i < n; ++i) {
                    int rfd = fevents[i].data.fd;
                    if (fevents[i].events & EPOLLIN) {
                        listeners.at(std::make_pair(rfd, EPOLLIN))(rfd);
                    }
                    if (fevents[i].events & EPOLLOUT) {
                        listeners[std::make_pair(rfd, EPOLLOUT)](rfd);
                    }
                    if (fevents[i].events & EPOLLRDHUP) {
                        listeners[std::make_pair(rfd, EPOLLRDHUP)](rfd);
                    }
                    if (fevents[i].events & EPOLLPRI) {
                        listeners[std::make_pair(rfd, EPOLLPRI)](rfd);
                    }
                    if (fevents[i].events & EPOLLERR) {
                        listeners[std::make_pair(rfd, EPOLLERR)](rfd);
                    }
                    if (fevents[i].events & EPOLLHUP) {
                        listeners[std::make_pair(rfd, EPOLLHUP)](rfd);
                    }
                    if (fevents[i].events & EPOLLET) {
                        listeners[std::make_pair(rfd, EPOLLET)](rfd);
                    }
                }
            }
        
        }
        
        void poll(){
            int n = epoll_wait(efd, fevents, MAX_EVENTS, -1);
            for (int i = 0; i < n; ++i) {
                int rfd = fevents[i].data.fd;
                if (fevents[i].events & EPOLLIN) {
                    listeners.at(std::make_pair(rfd, EPOLLIN))(rfd);
                }
                if (fevents[i].events & EPOLLOUT) {
                    listeners[std::make_pair(rfd, EPOLLOUT)](rfd);
                }
                if (fevents[i].events & EPOLLRDHUP) {
                    listeners[std::make_pair(rfd, EPOLLRDHUP)](rfd);
                }
                if (fevents[i].events & EPOLLPRI) {
                    listeners[std::make_pair(rfd, EPOLLPRI)](rfd);
                }
                if (fevents[i].events & EPOLLERR) {
                    listeners[std::make_pair(rfd, EPOLLERR)](rfd);
                }
                if (fevents[i].events & EPOLLHUP) {
                    listeners[std::make_pair(rfd, EPOLLHUP)](rfd);
                }
                if (fevents[i].events & EPOLLET) {
                    listeners[std::make_pair(rfd, EPOLLET)](rfd);
                }
            }
        }

        void set_done(){
            done = true;
        }
        bool is_done() const {
            return done;
        }
    private:
        int efd;
        int timeout;
        bool done;
        static const unsigned int MAX_EVENTS = 10;
        epoll_event fevents[MAX_EVENTS];
        std::map<int, __uint32_t> events;
        std::map<std::pair<int, __uint32_t>, callback> listeners;
        epoll_event tmp;
};

#endif
