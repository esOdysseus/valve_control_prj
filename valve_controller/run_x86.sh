#!/bin/bash
PROG_NAME=valve_control
__PROG_ROOT_PATH__=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

function runner_set_env() {
    export VALVE_GPIO_ROOT=${__PROG_ROOT_PATH__}/test/gpio
    export LD_LIBRARY_PATH=${__PROG_ROOT_PATH__}/lib/external_lib/lib/x86:${LD_LIBRARY_PATH}
}

function runner_start_program() {
    local TODAY_DATE=${1}
    local LOG_FILE_NAME=log_${PROG_NAME}_${TODAY_DATE}.txt
    local PROG_FULL_PATH=${__PROG_ROOT_PATH__}/debug/bin
    local ALIAS_FILE_PATH=${__PROG_ROOT_PATH__}/lib/external_lib/config/desp_alias.json
    local PROTO_FILE_PATH=${__PROG_ROOT_PATH__}/lib/external_lib/config/desp_UniversalCMD_protocol.json

    cd ${PROG_FULL_PATH}
    ## PID를 얻기 위해서, runner_start_program내부에서 Program 실행시 후렵부에 "& echo $!" 를 붙혀야 한다.
    ./${PROG_NAME} ${ALIAS_FILE_PATH} ${PROTO_FILE_PATH} >& ./${LOG_FILE_NAME} & echo $!
}

