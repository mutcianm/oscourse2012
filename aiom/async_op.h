#ifndef ASYNC_H
#define ASYNC_H

#include"aepoll.h"
#include"buffer.h"
#include<utility>


class async_op{
    public:
        async_op(): subscribed(false){
        }
        async_op(aepollfd& aep, int _fd, uint32_t _flags, pbuffer  _buf): fd(_fd), flags(_flags), buf(_buf) {
            std::cout << "async()" << std::endl;
            this->aep = &aep;
            //buf = new s_buffer(128);
            aep.subscribe(autofd(fd), flags, std::bind(&async_op::my_callback, this, &aep, std::placeholders::_1));
            subscribed = true;
            //aep.subscribe(autofd(fd), flags, std::bind(&async_op::my_callback, this, &aep, cb);
        }
        async_op(async_op&& other) : subscribed(false) {
            std::cout << "move async()" << std::endl;
        }
        
        
        ~async_op(){
            std::cout << "~async()" << std::endl;
            if(subscribed) aep->unsubscribe(autofd(fd), flags); 
            //delete buf;
        }

        async_op& operator=(async_op&& other){
            this->fd = std::move(other.fd);
            this->flags = std::move(other.flags);
            this->buf = std::move(other.buf);
            this->aep = std::move(other.aep);
            return *this;
        }


        virtual bool is_dummy() const {
            return false;
        }
    private:
        async_op(const async_op& other){}
    protected:
        virtual void my_callback(aepollfd* afd, int cfd){
            std::cout << "callback()" << std::endl;
            //int fd = accept(cfd, NULL, NULL);
            //buf->buf_read(fd);
            //std::string str(buf->get_data());
            //std::cout << "from NET: " << str << std::endl;
            //if(str.find("quit") != std::string::npos)
                //aep->set_done();
            //close(fd);
            
        }

        aepollfd* aep;
        pbuffer buf;
        int fd;
        uint32_t flags;
        bool subscribed;

};

#endif
