#!/bin/bash
PROG_NAME=valve_control
__PROG_ROOT_PATH__=/home/eunseok/project/app/valve_controller
__LIB_ROOT_PATH__=/home/eunseok/project/fw/communicator/debug

function runner_set_env() {
    export VALVE_GPIO_ROOT=/sys/class/gpio
    export LD_LIBRARY_PATH=${__LIB_ROOT_PATH__}/lib:${LD_LIBRARY_PATH}
}

function runner_start_program() {
    local IP=192.168.1.7
    local PORT=23456
    local TODAY_DATE=${1}
    local LOG_FILE_NAME=log_${PROG_NAME}_${TODAY_DATE}.txt
    local PROG_FULL_PATH=${__PROG_ROOT_PATH__}
    local ALIAS_FILE_PATH=${__LIB_ROOT_PATH__}/config/desp_alias.json

    cd ${PROG_FULL_PATH}
    ## PID를 얻기 위해서, runner_start_program내부에서 Program 실행시 후렵부에 "& echo $!" 를 붙혀야 한다.
    ./${PROG_NAME} ${IP} ${PORT} ${ALIAS_FILE_PATH} >& ./${LOG_FILE_NAME} & echo $!
}



