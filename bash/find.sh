#!/bin/bash

#Bash version 4.2.37
#Find version 4.4.2

#lolol

shopt -s dotglob
shopt -s nocasematch
V_TYPE="e"
V_NAME="*"
V_DIR="."
#fucking mag^W^W^Wbash, how do you work?
SYMHACK="-e "
#fug
matchfunc(){
    if [ "$V_TYPE" "$1" -a  $SYMHACK "$1" ]; then
        if [[ "$1" = $V_NAME ]]; then
            echo "$1"
        fi
    fi
}

walk(){
for i in "$1"/*
do
    matchfunc "$i"
    if [ -d "$i" ]; then
        walk "$i"
    fi
done      
}

parsecmdline(){
until [ -z "$1" ]
do

    case "$1" in
        "-iname")
            shift
            V_NAME="$1"
            ;;
        "-type")
            shift
            V_TYPE="$1"
            ;;
        *)
            if [ "$V_DIR" = "." ]; then
                V_DIR="$1"
            else
                echo "Too many or incorrect parameters"
                exit -1
            fi
            ;;
    esac
    shift
done  
}

ensurevalidargs(){
#fucking bash, WHY?    
    case "$V_TYPE" in
        b|c|d|p|e) ;;
        l) V_TYPE="h" ;;
        s) V_TYPE="S" ;;
        f)
            SYMHACK="! -h "
            ;;
        *) 
            echo "Wrong type specifier"
            exit -1
            ;;
    esac
    V_TYPE="-$V_TYPE"
}


main(){
    parsecmdline "$@"
    ensurevalidargs
    matchfunc "$V_DIR" #WHY, bash, WHY?
    walk "$V_DIR"
}

main "$@"
