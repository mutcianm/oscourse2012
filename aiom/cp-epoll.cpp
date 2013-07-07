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
#include<memory>
#include<vector>
#include"async_op.h"

const int BUF_SIZE = 256;
typedef std::shared_ptr<async_op> aptr;

class read_op: public async_op{
    public:
        read_op(aepollfd& aep, int _fd, uint32_t _flags, pbuffer  _buf): async_op(aep, _fd, _flags, _buf) {}
    private:
        virtual void my_callback(aepollfd* efd, int fd){
            std::cout << "read_op()" << std::endl;
            std::string str(buf->get_data());
            if(str.find("quit") != str.npos)
                efd->set_done();
            if(buf->buf_read(fd) < 0){
                std::cout << "read finished" << std::endl;
            }
        }
};

class write_op: public async_op{
    public:
        write_op(aepollfd& aep, int _fd, uint32_t _flags, pbuffer  _buf): async_op(aep, _fd, _flags, _buf) {}
    private:
        virtual void my_callback(aepollfd* efd, int fd){
            if(buf->buf_write(fd) < 0){
                std::cout << "write finished" << std::endl;
            }
        }
};

class dummy_op: public async_op{
    public:
        dummy_op(): async_op() {}
        dummy_op(aepollfd& aep, int _fd, uint32_t _flags, pbuffer  _buf) {}
        virtual bool is_dummy() const{
            return true;
        }
};

struct transfer_state{
    transfer_state(async_op&& _read, async_op&& _write, pbuffer _buf, int in, int out): fd_in(in), fd_out(out), read_op(std::move(_read)), write_op(std::move(_write)), buf(_buf){}
    async_op read_op;
    async_op write_op;
    pbuffer buf;
    int fd_in, fd_out;
};

int main(int argc, char** argv){
    if((argc-1) % 2)
        exit(-1);
    int n = (argc - 1) / 2;
    aepollfd aep;
    std::vector<transfer_state> states;
    std::vector<pbuffer> buffers;
    for(int i = 0; i < n; ++i){
        std::cout << "meh" << std::endl;
        buffers.push_back(pbuffer(new s_buffer(BUF_SIZE)));
        int fd_in = atoi(argv[2*i]);
        int fd_out = atoi(argv[2*i+1]);
        states.push_back(transfer_state(read_op(aep, fd_in, EPOLLIN, buffers.back()),
                         write_op(aep, fd_out, EPOLLOUT, buffers.back()),
                         buffers.back(), fd_in, fd_out)
                );
    }
    std::cout << "return" << std::endl;
    while (!aep.is_done()){
        aep.poll();
        for (int i = 0; i < states.size(); ++i){
            if(states[i].buf->is_empty() && !states[i].write_op.is_dummy()){
                states[i].write_op = std::move(dummy_op());
                states[i].read_op = std::move(read_op(aep, states[i].fd_in, EPOLLIN, states[i].buf));
            } else if (states[i].buf->is_full() && !states[i].read_op.is_dummy()){
                states[i].read_op = std::move(dummy_op());
                states[i].write_op = std::move(write_op(aep, states[i].fd_in, EPOLLIN, states[i].buf));
            }
        }
    }
    return 0;     
}
