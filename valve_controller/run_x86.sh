#!/bin/bash
PROG_NAME=app_valve_controller
__PROG_ROOT_PATH__=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

function runner_set_env() {
    local BUILD_MODE=release
    if [ ! -z ${1} ]; then
        BUILD_MODE=${1}
    fi

    export MACHINE_DEVICE_NAME="Machine-Valve-0x123457"
    export VALVE_GPIO_ROOT=${__PROG_ROOT_PATH__}/../${BUILD_MODE}/valve_controller/test/gpio
    export LD_LIBRARY_PATH=${__PROG_ROOT_PATH__}/../${BUILD_MODE}/common/lib/communicator/lib/:${LD_LIBRARY_PATH}

    # Make environment for testing GPIO set/get
    echo "Make GPIO-environment for Testing"
    sudo rm -rf ${VALVE_GPIO_ROOT}
    mkdir -p ${VALVE_GPIO_ROOT}/gpio12
    mkdir -p ${VALVE_GPIO_ROOT}/gpio13
    mkdir -p ${VALVE_GPIO_ROOT}/gpio14
    mkdir -p ${VALVE_GPIO_ROOT}/gpio18
    mkdir -p ${VALVE_GPIO_ROOT}/gpio19
    mkdir -p ${VALVE_GPIO_ROOT}/gpio107
    echo "1" > ${VALVE_GPIO_ROOT}/gpio12/value
    echo "1" > ${VALVE_GPIO_ROOT}/gpio13/value
    echo "1" > ${VALVE_GPIO_ROOT}/gpio14/value
    echo "1" > ${VALVE_GPIO_ROOT}/gpio18/value
    echo "1" > ${VALVE_GPIO_ROOT}/gpio19/value
    echo "1" > ${VALVE_GPIO_ROOT}/gpio107/value
}

function runner_start_program() {
    local TODAY_DATE=${1}
    local BUILD_MODE=release
    if [ ! -z ${2} ]; then
        BUILD_MODE=${2}
    fi
    local LOGGING="true"
    if [ ! -z ${3} ]; then
        LOGGING=${3}
    fi

    local LOG_FILE_NAME=log_${PROG_NAME}_${TODAY_DATE}.txt
    local PROG_FULL_PATH=${__PROG_ROOT_PATH__}/../${BUILD_MODE}/valve_controller/bin
    local ALIAS_FILE_PATH=${__PROG_ROOT_PATH__}/../${BUILD_MODE}/common/lib/communicator/etc/desp_alias.json
    local PROTO_FILE_PATH=${__PROG_ROOT_PATH__}/../${BUILD_MODE}/common/lib/communicator/etc/desp_UniversalCMD_protocol.json

    local CUR_PATH=${PWD}
    cd ${PROG_FULL_PATH}

    if [ ${LOGGING} == "true" ]; then
        ## PID를 얻기 위해서, runner_start_program내부에서 Program 실행시 후렵부에 "& echo $!" 를 붙혀야 한다.
        echo "Start Logging to ${LOG_FILE_NAME}"
        ./${PROG_NAME} ${ALIAS_FILE_PATH} ${PROTO_FILE_PATH} >& ./${LOG_FILE_NAME} & echo $!
    else
        ./${PROG_NAME} ${ALIAS_FILE_PATH} ${PROTO_FILE_PATH}
    fi

    cd ${CUR_PATH}
}

function __run_sample__() {
    local TODAY=$(date +"%F_%T")
    
    runner_set_env ${1}
    sleep 1
    runner_start_program ${TODAY} ${1} "false"
}
