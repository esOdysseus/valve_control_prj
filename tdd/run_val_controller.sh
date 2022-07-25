#!/bin/bash
ROOT_PATH=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

############################################################
#  Run valve-controller in x86 machine with root permission.
#    - Prerequisites
#       It require 'sudo' permission.
#
# example) $ sudo ./run_val_controller_as_root.sh debug real
#          $ sudo ./run_val_controller_as_root.sh release launch
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
    cd ${WORKSPACE}/valve_controller
    source ./run_x86.sh
    __run_sample__ ${BUILD_MODE}
elif [ "${RUN_TYPE}" == "launch" ]; then
    ${LAUNCHER} -s-path ${WORKSPACE}/valve_controller -s-name run_x86.sh -build ${BUILD_MODE}
else
    echo -e "\e[1;31m [ERROR] Invalid input RUN_TYPE. (${RUN_TYPE}) \e[0m"
fi

