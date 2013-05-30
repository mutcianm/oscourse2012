#!/bin/bash

exec 9<> wtf.txt
exec 10<> out.txt

./a.out 0 1
