#!/bin/bash

function cecho() {
    echo -e "\033[38;5;214m$@\033[0m"
}

# limit max number of open files to force pqact to close open file descriptors.
ulimit -n 1024

cecho "PATH = ${PATH}"

# never exit
while true; do sleep 10000; done
