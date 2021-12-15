#!/bin/bash
ROOT_PATH=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

source ${ROOT_PATH}/lib/logger.sh

logi "Starting Valve-pinctrl"
# Set GPIO port for Relay of valves.
${ROOT_PATH}/lib/start_valve.sh D 11
${ROOT_PATH}/lib/start_valve.sh D 14
${ROOT_PATH}/lib/start_valve.sh A 13
${ROOT_PATH}/lib/start_valve.sh A 14
${ROOT_PATH}/lib/start_valve.sh A 15
${ROOT_PATH}/lib/start_valve.sh A 16
${ROOT_PATH}/lib/start_valve.sh A 18
${ROOT_PATH}/lib/start_valve.sh A 19

# Set GPIO port for In-Active indicator.
${ROOT_PATH}/lib/start_valve.sh A 12

sync
sleep 1

