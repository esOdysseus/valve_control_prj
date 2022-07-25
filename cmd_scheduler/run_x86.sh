#!/bin/bash
###########################################################
# Limitation
#   - 상황: Source로 다른 script file에서 호출되어질때,
#   - 한계점: global 변수를 const로 초기값 설정은 가능하다.
#             그러나, 함수내에서 수정한 값이 반영되지 않는다.
###########################################################

PROG_NAME=app_cmd_scheduler
__PROG_ROOT_PATH__=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )


function runner_set_env() {
    local BUILD_MODE=release
    if [ ! -z ${1} ]; then
        BUILD_MODE=${1}
    fi

    export EXPORT_ENV_GPS_PATH="/dev/ttyUSB0"
    export MACHINE_DEVICE_NAME="Machine-0x123456"
    export LD_LIBRARY_PATH=${__PROG_ROOT_PATH__}/../${BUILD_MODE}/common/lib/communicator/lib:${LD_LIBRARY_PATH}

    sudo rm -rf ${PROG_FULL_PATH}/db_*
}

function runner_start_program() {
    local LOG_FILE_NAME=${1}
    local BUILD_MODE=release
    if [ ! -z ${2} ]; then
        BUILD_MODE=${2}
    fi
    local LOGGING="true"
    if [ ! -z ${3} ]; then
        LOGGING=${3}
    fi

    local PROG_FULL_PATH=${__PROG_ROOT_PATH__}/../${BUILD_MODE}/cmd_scheduler/bin
    local ALIAS_FILE_PATH=${__PROG_ROOT_PATH__}/../${BUILD_MODE}/common/lib/communicator/etc/desp_alias.json
    local PROTO_FILE_PATH=${__PROG_ROOT_PATH__}/../${BUILD_MODE}/common/lib/communicator/etc/desp_UniversalCMD_protocol.json

    local CUR_PATH=${PWD}
    cd ${PROG_FULL_PATH}
    
    if [ ${LOGGING} == "true" ]; then
        ## PID를 얻기 위해서, runner_start_program내부에서 Program 실행시 후렵부에 "& echo $!" 를 붙혀야 한다.
        # echo "Start Logging to ${LOG_FILE_NAME}"
        ./${PROG_NAME} ${ALIAS_FILE_PATH} ${PROTO_FILE_PATH} >& ${LOG_FILE_NAME} & echo $!
    else
        ./${PROG_NAME} ${ALIAS_FILE_PATH} ${PROTO_FILE_PATH} & echo $!
    fi
    
    cd ${CUR_PATH}
}

function __run_sample__() {
    local TODAY=$(date +"%F_%T")
    local LOG_PATH=$(get_log_path ${TODAY} ${1})

    runner_set_env ${1}
    sleep 1
    runner_start_program ${LOG_PATH} ${1} "false"
}

function get_log_path() {
    local TODAY_DATE=${1}
    local BUILD_MODE=release
    if [ ! -z ${2} ]; then
        BUILD_MODE=${2}
    fi

    local PROG_FULL_PATH=${__PROG_ROOT_PATH__}/../${BUILD_MODE}/cmd_scheduler/bin
    echo "${PROG_FULL_PATH}/log_${PROG_NAME}_${TODAY_DATE}.txt"
}
