#!/bin/bash
ROOT_PATH=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

WORKSPACE=${ROOT_PATH}/..
LAUNCHER=${WORKSPACE}/launcher.sh
BUILD_MODE=${1}

sudo ${LAUNCHER} -s-path ${WORKSPACE}/cmd_scheduler -s-name run_x86.sh -build ${BUILD_MODE}
