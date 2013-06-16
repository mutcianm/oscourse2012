#!/bin/bash

exec 9<> asd
exec 10<> 123
#read -u 9 a
#echo $a

exec ./cp-epoll 9 1 10 2
