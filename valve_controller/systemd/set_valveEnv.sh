#!/bin/bash
ROOT_PATH=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

source ${ROOT_PATH}/lib/logger.sh

# Carry out specific functions when asked to by the system
logi "Starting Valve-pinctrl"
${ROOT_PATH}/lib/start_valve.sh D 11
${ROOT_PATH}/lib/start_valve.sh D 14
${ROOT_PATH}/lib/start_valve.sh A 13
${ROOT_PATH}/lib/start_valve.sh A 14
${ROOT_PATH}/lib/start_valve.sh A 15
${ROOT_PATH}/lib/start_valve.sh A 16
${ROOT_PATH}/lib/start_valve.sh A 18
${ROOT_PATH}/lib/start_valve.sh A 19

sync
sleep 1

