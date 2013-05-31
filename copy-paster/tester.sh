#!/bin/bash

exec 9<> wtf.txt
exec 10<> out.txt
#read -u 9 a
#echo $a

exec ./cp-poll 9 10
