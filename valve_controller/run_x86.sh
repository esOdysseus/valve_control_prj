#!/bin/bash
PROG_NAME=app_valve_control
__PROG_ROOT_PATH__=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

function runner_set_env() {
    export MACHINE_DEVICE_NAME="Machine-Valve-0x123457"
    export VALVE_GPIO_ROOT=${__PROG_ROOT_PATH__}/test/gpio
    export LD_LIBRARY_PATH=${__PROG_ROOT_PATH__}/../common/lib/communicator/lib/x86:${LD_LIBRARY_PATH}
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
    local ALIAS_FILE_PATH=${__PROG_ROOT_PATH__}/../common/lib/communicator/config/desp_alias.json
    local PROTO_FILE_PATH=${__PROG_ROOT_PATH__}/../common/lib/communicator/config/desp_UniversalCMD_protocol.json

    local CUR_PATH=${PWD}
    cd ${PROG_FULL_PATH}

    if [ ${LOGGING} == "true" ]; then
        ## PID를 얻기 위해서, runner_start_program내부에서 Program 실행시 후렵부에 "& echo $!" 를 붙혀야 한다.
        ./${PROG_NAME} ${ALIAS_FILE_PATH} ${PROTO_FILE_PATH} >& ./${LOG_FILE_NAME} & echo $!
    else
        ./${PROG_NAME} ${ALIAS_FILE_PATH} ${PROTO_FILE_PATH}
    fi

    cd ${CUR_PATH}
}

function __run_sample__() {
    local TODAY=$(date +"%F_%T")
    
    runner_set_env
    runner_start_program ${TODAY} ${1} "false"
}
