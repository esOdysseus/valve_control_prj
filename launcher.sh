#!/bin/bash
ROOT_PATH=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

# Custom Variables for Configuration
SLEEP_SEC=5
RESTART_PROG_MINTIME=02:00:00
RESTART_PROG_MAXTIME=02:00:06
LIMIT_RUN_TIME=$[3600 * 10]

# System Variables
NULL_VALUE="none"
SCRIPT_PATH=${ROOT_PATH}
SCRIPT_NAME=${NULL_VALUE}
PROG_NAME=${NULL_VALUE}
RUNNING_TIME=0
PROG_PID=0
BUILD_MODE=release


######################################
# Base Functions
###
function echoerr() {
    echo -e "\e[1;31m [ERROR]: $* \e[0m"
    exit 1
}

function clear_sys_vars() {
    RUNNING_TIME=0
    PROG_PID=0    
}

function get_today() {
    date +"%F_%T"
}

function get_cur_time() {
    date +"%T"
}

function append_running_time() {
    APPEND_TIME=${1}
    RUNNING_TIME=$((${RUNNING_TIME} + ${APPEND_TIME}))
}


######################################
# Functions for Life-cycle managing
###
function get_input_parameter()
{
    echo "Get input parameters = $*"
    local IN_HEAD=${NULL_VALUE}

    for input in $*
    do
        case ${input} in
            "-s-name" )      IN_HEAD=${input};;
            "-s-path" )      IN_HEAD=${input};;
            "-build" )      IN_HEAD=${input};;
            * )
                case ${IN_HEAD} in 
                    "-s-name" )  # script-name: for SCRIPT_NAME
                        SCRIPT_NAME=${input}
                        IN_HEAD=${NULL_VALUE};;
                    "-s-path" )  # script-path: for SCRIPT_PATH
                        SCRIPT_PATH=${input}
                        IN_HEAD=${NULL_VALUE};;
                    "-build" )  # build-mode: valid value [ release  debug ]
                        BUILD_MODE=${input}
                        IN_HEAD=${NULL_VALUE};;
                    ${NULL_VALUE} )
                        ;;
                    * )
                        echoerr "Invalid input-head. (${IN_HEAD})"
                esac
                ;;
        esac
    done

    if [ ${SCRIPT_NAME} == ${NULL_VALUE} ]; then
        echoerr "We need mandatory parameters.(-s-name)"
    fi
}

function stop_program() {
    echo "Stop ${PROG_NAME} program."
    kill -SIGINT ${PROG_PID}
    clear_sys_vars
    sync
    sleep 1
}

function run_program() {
    local TODAY=$(get_today)
    ## PID를 얻기 위해서, runner_start_program내부에서 Program 실행시 후렵부에 "& echo $!" 를 붙혀야 한다.
    PROG_PID=$(runner_start_program ${TODAY} ${BUILD_MODE})
    cd ${ROOT_PATH}
}

function run_program_if_need() {
    if [ ${PROG_NAME} == "none" ]; then
        echoerr "Need PROG_NAME=${PROG_NAME}"
    fi

    ps auxw | grep ${PROG_NAME} | grep -v grep > /dev/null

    if [ $? != 0 ]
    then
        echo "Running of ${PROG_NAME} is start."
        clear_sys_vars
        run_program
        echo "PROG_PID=${PROG_PID}"
    fi
}


##############
# Main routin
##############
# Get & Parse input-parameter.
get_input_parameter $*

## Include script to run program. #
# Assumption : Following is mandatory.
#  - Variables : PROG_NAME
#  - Method    : runner_set_env , runner_start_program
source ${SCRIPT_PATH}/${SCRIPT_NAME}

# Print Major Variables.
echo ""
echo "###################################"
echo "# Program Name : ${PROG_NAME}"
echo "# RESTART_PROG_MINTIME : ${RESTART_PROG_MINTIME}"
echo "# RESTART_PROG_MAXTIME : ${RESTART_PROG_MAXTIME}"
echo "# LIMIT_RUN_TIME : ${LIMIT_RUN_TIME} seconds"
echo "###################################"
echo ""

# Call function of custom-script to export for Global variables with regard to running-program.
runner_set_env ${BUILD_MODE}
cd ${ROOT_PATH}

# Run launcher for monitoring the program.
while [ true ]; do
    run_program_if_need

    # Sleep & count of running-time.
    sleep ${SLEEP_SEC}
    append_running_time ${SLEEP_SEC}

    # Get current time.
    TIME_NOW=$(get_cur_time)
    echo "TIME_NOW=${TIME_NOW}, RUNNING_TIME=${RUNNING_TIME}, PROG_PID=${PROG_PID}"

    # Check program stoping.
    if [ ${RUNNING_TIME} -gt ${LIMIT_RUN_TIME} ]; then
        if [[ ${RESTART_PROG_MINTIME} < ${TIME_NOW} ]] && [[ ${TIME_NOW} < ${RESTART_PROG_MAXTIME} ]]; then
            stop_program
        fi
    fi
done

# exit program
stop_program
exit 0
