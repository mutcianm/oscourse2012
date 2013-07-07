#ifndef AUTOFD_H
#define AUTOFD_H
#include<exception>
#include<iostream>
#include<utility>
#include<unistd.h>

class autofd_exception: public std::exception{
    public:
        std::string msg;
        autofd_exception(std::string _msg) noexcept : msg(_msg) {}
        const char* what() const noexcept { return msg.c_str(); }
};

class autofd{
    public:
        autofd(int fd){
            if(fd < 0)
                throw autofd_exception("fd < 0!");
            this->fd = fd;
        }
        autofd(autofd&& other): fd(std::move(other.fd)){
        }
        int operator*(){
            return fd;
        }
        ~autofd(){
            //if(fd > 0)
                //close(fd);
        }
    private:
        int fd;
        autofd(const autofd& other) = delete;
        autofd() = delete;
        autofd& operator=(const autofd&) = delete;
};

#endif
