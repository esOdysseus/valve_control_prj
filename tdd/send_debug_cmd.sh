#!/bin/bash
ROOT_PATH=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
echo "Current PWD=${ROOT_PATH}"

# Get argument
CMD_FILE_PATH=${ROOT_PATH}/Principle6_test.txt
if [ ! -z ${1} ]; then
    CMD_FILE_PATH=${1}
fi
echo "Command-File = ${CMD_FILE_PATH}"

BOARD_TYPE="x86"
if [ ! -z ${2} ]; then
    BOARD_TYPE=${2}
fi
echo "Board-Type = ${BOARD_TYPE}"

if [ "${BOARD_TYPE}" == "x86" ]; then
    # Set Debug-Interface to communicate with CMD-Scheduler.
    IP_CMD_SCHEDULER=192.168.1.2
    PORT_CMD_SCHEDULER=20000        # for def_debugger Provider.    
else
    # Set Debug-Interface to communicate with CMD-Scheduler.
    IP_CMD_SCHEDULER=192.168.1.6
    PORT_CMD_SCHEDULER=20000        # for def_debugger Provider.
fi

# Try to send UDP-command according to FILE_PATH to CMD_SCHEDULER.
# cat ${CMD_FILE_PATH} | nc -u ${IP_CMD_SCHEDULER} ${PORT_CMD_SCHEDULER}
python3 ./debug_commander.py  -file ${CMD_FILE_PATH}  -ipr ${IP_CMD_SCHEDULER}  -portr ${PORT_CMD_SCHEDULER}

