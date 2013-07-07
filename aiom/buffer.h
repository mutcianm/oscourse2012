#ifndef BUFFER_H
#define BUFFER_h

#include<unistd.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<memory>

class s_buffer{
    public:
        s_buffer(size_t _size): pos(0), BUF_SIZE(_size){
            data = new char[BUF_SIZE];
        }
        ~s_buffer(){
            delete[] data;
            std::cout << "~buffer()" << std::endl;
        }

        int buf_read(int fd){
            if(pos < BUF_SIZE ){
            printf("starting pull: %d %p\n", fd, this);
                int cnt = read(fd, data + pos, BUF_SIZE - pos);
                if(cnt <= 0){
                    perror("read() failed");
                    return -1;
                }
                else{
                    pos += cnt;
                    printf("read %d bytes\n", cnt);
                    return cnt;
                }
            }
            return 0;
        }
        int buf_write(int fd){
            if(pos > 0){
            printf("starting push\n");
                int cnt = write(fd, data, pos);
                if(cnt <= 0){
                    perror("write() failed");
                    return -1;
                }
                if(cnt > 0){
                    memmove(data, data + cnt, pos - cnt);
                }
                pos -= cnt;
            }
            return 0;
        }
        char* get_data() const {
            return data;
        }
        bool is_empty() const {
            return (pos == 0);
        }
        bool is_full() const {
            return (pos == BUF_SIZE);
        }
    private:
        char* data;
        size_t pos;
        size_t BUF_SIZE;
        int fd_in;
        int fd_out;
};

typedef std::shared_ptr<s_buffer> pbuffer;

#endif
