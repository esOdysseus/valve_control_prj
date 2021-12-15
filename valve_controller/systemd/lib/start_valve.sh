#!/bin/bash

CUR_DIR=${PWD}
PATH_SCRIPT_FILE=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

# Input-description about GPIO-pin that is wanted by User.
PORT_TYPE=${1}
PORT_NUM=${2}
PORT_DIRECTION=out

PORT_INIT_VALUE=1
if [ ! -z ${3} ] ; then
        PORT_INIT_VALUE=${3}
fi


# Definition of Function
source ${PATH_SCRIPT_FILE}/logger.sh
function calculate_sunxi_number() {
        ASC_TRG=$(get_asc ${1})
        ASC_A=$(get_asc A)
        NUMBER=$[ (${ASC_TRG}-${ASC_A})*32 + ${2} ]
        printf ${NUMBER}
}



# set system variables
SUNXI_NUM=$(calculate_sunxi_number ${PORT_TYPE} ${PORT_NUM})
SUNXI_ROOT=/sys/class
SUNXI_GPIO_PATH=${SUNXI_ROOT}/gpio
SUNXI_GPIO_PIN=${SUNXI_GPIO_PATH}/gpio${SUNXI_NUM}
SUNXI_GPIO_PIN_VALUE=${SUNXI_GPIO_PIN}/value
if [ -z ${SUNXI_NUM} ]; then
        loge "SUNXI_NUM calculation is failed.(It's empty)"
fi
echo ""
logi "SUNXI_NUM=${SUNXI_NUM}"
logd "SUNXI_GPIO_PATH=${SUNXI_GPIO_PATH}"
logd "SUNXI_GPIO_PIN=${SUNXI_GPIO_PIN}"
echo ""



# Setting PIN-control of GPIO port. (need root right for setting pin-ctrl.)
if [ ! -L "${SUNXI_GPIO_PIN}" ]; then   # check SUNXI_GPIO_PIN is exist as link-file, or not.
        echo ${SUNXI_NUM} > ${SUNXI_GPIO_PATH}/export
        ls -al ${SUNXI_GPIO_PATH}
fi

if [ ! -f "${SUNXI_GPIO_PIN}/direction" ]; then # check direction-file is exist, or not.
        loge "Exporting of Sunxi PIN(${SUNXI_NUM}) is failed."
fi

echo ${PORT_DIRECTION} > ${SUNXI_GPIO_PIN}/direction
if [ ! -f "${SUNXI_GPIO_PIN_VALUE}" ]; then     # check value-file is exist, or not.
        loge "Direction-setting of Sunxi PIN(${SUNXI_NUM}) is failed."
fi

chmod 666 ${SUNXI_GPIO_PIN_VALUE}


# Test result-pin of OUT-PIN setting.
echo ${PORT_INIT_VALUE} > ${SUNXI_GPIO_PIN_VALUE}
RESULT=$(cat ${SUNXI_GPIO_PIN_VALUE})

if [ ${RESULT} != ${PORT_INIT_VALUE} ]; then
        loge "Test-set pin-out is failed."
fi

# sleep 1
# echo 0 > ${SUNXI_GPIO_PIN_VALUE}


cd ${CUR_DIR}
