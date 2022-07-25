#!/bin/bash
ROOT_PATH=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

############################################################
#  Run cmd-scheduler in x86 machine with root permission.
#    - Prerequisites
#       It require 'sudo' permission.
#
# example) $ sudo ./run_cmd_scheduler_as_root.sh debug real
#          $ sudo ./run_cmd_scheduler_as_root.sh release launch
############################################################

WORKSPACE=${ROOT_PATH}/..
LAUNCHER=${WORKSPACE}/launcher.sh
BUILD_MODE=release
if [ ! -z ${1} ]; then
    BUILD_MODE=${1}
fi

RUN_TYPE="real"
if [ ! -z ${2} ]; then
    RUN_TYPE=${2}
fi

if [ "${RUN_TYPE}" == "real" ]; then
    cd ${WORKSPACE}/cmd_scheduler
    source ./run_x86.sh
    __run_sample__ ${BUILD_MODE}
elif [ "${RUN_TYPE}" == "launch" ]; then
    ${LAUNCHER} -s-path ${WORKSPACE}/cmd_scheduler -s-name run_x86.sh -build ${BUILD_MODE}
else
    echo -e "\e[1;31m [ERROR] Invalid input RUN_TYPE. (${RUN_TYPE}) \e[0m"
fi


