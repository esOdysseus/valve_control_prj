#!/bin/bash

PRG_PATH=/root/project/app/cmd_scheduler
BUILD_MODE=debug
COMM_ROOT=/root/project/fw/communicator/${BUILD_MODE}


export MACHINE_DEVICE_NAME="Machine-0x123456"
export LD_LIBRARY_PATH=${COMM_ROOT}/lib:${LD_LIBRARY_PATH}


cd ${PRG_PATH}
#./valve_control 192.168.1.6 12346 ${COMM_ROOT}/config/desp_alias.json >& ${PRG_PATH}/log.txt &
./valve_control 192.168.1.6 12346 ${COMM_ROOT}/config/desp_alias.json

