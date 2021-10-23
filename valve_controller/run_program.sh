#!/bin/bash

if [ -z ${1} ]; then
    echo -e "\e[1;31m You have to insert TARGET_SCRIPT file. \e[0m"
fi
TARGET_SCRIPT=${1}

BUILD_MODE=release
if [ ! -z ${2} ]; then
    BUILD_MODE=${2}
fi


source ${TARGET_SCRIPT}
__run_sample__ ${BUILD_MODE}

