#!/bin/bash
ROOT_PATH=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

if [ -z ${1} ] || [ -z ${2} ] || [ -z ${3} ] || [ -z ${4} ] ; then
    echo ""
    echo "==================================================================="
    echo "= Deploy Application for ARMv7 & Aarch64 target."
    echo "= "
    echo "= param-01 : app      --> valid-name: cmd_scheduler , valve_controller"
    echo "= param-02 : ip       --> IPv4 number: 192.168.1.6"
    echo "= param-03 : user     --> ID: root , eunseok"
    echo "= param-04 : passwd   --> password"
    echo "= param-05 : build    --> build mode : debug , release (default)"
    echo "= param-06 : mode     --> mode : only_app , only_common, all (default)"
    echo "==================================================================="
    echo ""
    echo -e "\e[1;31m [ERROR] Please insert parameters \e[0m"
    exit
fi

# Get arguments
APP_NAME=${1}
IP=${2}
USER=${3}
PASSWD=${4}
BUILD_MODE=release
if [ ! -z ${5} ] ; then
    BUILD_MODE=${5}
fi
MODE=all
if [ ! -z ${6} ] ; then
    MODE=${6}
fi


echo ""
echo "=========================="
echo "APP_NAME=${APP_NAME}"
echo "IP=${IP}"
echo "USER=${USER}"
echo "PASSWD=${PASSWD}"
echo "BUILD_MODE=${BUILD_MODE}"
echo "MODE=${MODE}"
echo "=========================="
echo ""

if [ "${MODE}" != "only_app" ] && [ "${MODE}" != "only_common" ] && [ "${MODE}" != "all" ] ; then
    echo -e "\e[1;31m [ERROR] \"${MODE}\" is not supported mode. \e[0m"
    echo -e "\e[1;31m         Please valid-value like(\"only_app\", \"only_common\", \"all\") \e[0m"
    exit
fi


# Set local-variables
APP_ROOT="${ROOT_PATH}/${BUILD_MODE}/${APP_NAME}"
COMMON_ROOT="${ROOT_PATH}/${BUILD_MODE}/common"
if [ ! -d "${APP_ROOT}" ] ; then
    echo -e "\e[1;31m [ERROR] Not exist \"${APP_ROOT}\" folder \e[0m"
    exit
fi
if [ ! -d "${COMMON_ROOT}" ] ; then
    echo -e "\e[1;31m [ERROR] Not exist \"${COMMON_ROOT}\" folder \e[0m"
    exit
fi

# Set remote-variables
RMT_APP_ROOT="/root/project/app"
RMT_COMMON_ROOT="/root/project/fw"

##################################
# Internal-Function definition.
##################################
function exec_command() {
    if [ -z "${1}" ] ; then
        echo -e "\e[1;31m [ERROR][exec_command] We need command. \e[0m"
        exit
    fi

    local COMMAND="${1}"
    echo "> Run COMMAND(${COMMAND}) in Remote-Target."
    sshpass -p ${PASSWD} ssh -t -p 22 -o StrictHostKeyChecking=no ${USER}@${IP} "${COMMAND}"
}

function exec_script() {
    if [ -z "${1}" ] ; then
        echo -e "\e[1;31m [ERROR][exec_command] We need local script-path. \e[0m"
        exit
    fi

    local SCRIPT="${1}"
    echo "> Run SCRIPT(${SCRIPT}) in Remote-Target."
    sshpass -p ${PASSWD} ssh -t -p 22 -o StrictHostKeyChecking=no ${USER}@${IP} "bash -s" < ${SCRIPT}
}

function send_dir_file() {
    if [ -z "${1}" ] || [ -z "${2}" ] ; then
        echo -e "\e[1;31m [ERROR][send_dir_file] We need arguments. \e[0m"
        exit
    fi

    local SOURCE="${1}"
    local TARGET="${2}"
    echo "> Send ${SOURCE} to ${TARGET} in Remote-Target."
    sshpass -p ${PASSWD} scp -r ${SOURCE} ${USER}@${IP}:${TARGET}
}

################################################################
# Transfer apps, libraries, configuration-files, service-files
################################################################

##### Unregister Application to systemd.
echo ""
echo "===== Unregister Application files. ====="
exec_command "systemctl stop ${APP_NAME}.service"
exec_command "systemctl stop dlt.service"

exec_command "systemctl disable ${APP_NAME}.service"
exec_command "systemctl disable dlt.service"
echo "===== Done unregistring Application files. ====="
echo ""


echo ""
echo "===== Remove Application/Common files. ====="
if [ "${MODE}" == "only_app" ] || [ "${MODE}" == "all" ] ; then
    exec_command "rm -rf ${RMT_APP_ROOT}"
fi
if [ "${MODE}" == "only_common" ] || [ "${MODE}" == "all" ] ; then
    exec_command "rm -rf ${RMT_COMMON_ROOT}"
fi


##### Send Common files.
if [ "${MODE}" == "only_common" ] || [ "${MODE}" == "all" ] ; then
    echo ""
    echo "===== Deloy Common files. ====="
    exec_command "mkdir -p ${RMT_COMMON_ROOT}/bin"
    exec_command "mkdir -p ${RMT_COMMON_ROOT}/lib"
    exec_command "mkdir -p ${RMT_COMMON_ROOT}/config"

    echo "-------------- DLT common files ------------------"
    DLT_ROOT="${COMMON_ROOT}/lib/dlt"           # DLT
    send_dir_file "${DLT_ROOT}/bin/dlt-daemon"  "${RMT_COMMON_ROOT}/bin/"
    send_dir_file "${DLT_ROOT}/bin/dlt-receive"  "${RMT_COMMON_ROOT}/bin/"
    send_dir_file "${DLT_ROOT}/etc/*"  "${RMT_COMMON_ROOT}/config/"
    send_dir_file "${DLT_ROOT}/lib"  "${RMT_COMMON_ROOT}/"
    echo "-------------- SQLite common files ------------------"
    SQLITE_ROOT="${COMMON_ROOT}/lib/sqlite"     # SQLITE
    send_dir_file "${SQLITE_ROOT}/bin"  "${RMT_COMMON_ROOT}/"
    send_dir_file "${SQLITE_ROOT}/lib"  "${RMT_COMMON_ROOT}/"
    echo "-------------- COMM common files ------------------"
    COMM_ROOT="${COMMON_ROOT}/lib/communicator"     # COMM
    send_dir_file "${COMM_ROOT}/etc/*"  "${RMT_COMMON_ROOT}/config/"
    send_dir_file "${COMM_ROOT}/lib"  "${RMT_COMMON_ROOT}/"

    echo "===== Done deloy Common files. ====="
    echo ""
fi

##### Send Application files.
if [ "${MODE}" == "only_app" ] || [ "${MODE}" == "all" ] ; then
    echo ""
    echo "===== Deloy Application files. ====="
    exec_command "mkdir -p ${RMT_APP_ROOT}"
    exec_command "mkdir -p ${RMT_APP_ROOT}/log"
    exec_command "cp -Rdp ${RMT_COMMON_ROOT}/config/dlt_logstorage.conf  ${RMT_APP_ROOT}/log/"

    send_dir_file "${APP_ROOT}/bin/app_${APP_NAME}"  "${RMT_APP_ROOT}/${APP_NAME}"
    send_dir_file "${ROOT_PATH}/${APP_NAME}/systemd" "${RMT_APP_ROOT}/"

    echo "===== Done deloy Application files. ====="
    echo ""
fi


##### Register Application to systemd.
echo ""
echo "===== Register Application files. ====="
exec_command "systemctl enable ${RMT_APP_ROOT}/systemd/${APP_NAME}.service"
exec_command "systemctl enable ${RMT_APP_ROOT}/systemd/dlt.service"
exec_command "sync"
exec_command "sync"
echo "===== Done registring Application files. ====="
echo ""


echo ""
echo "===== Reboot target-board. ====="
exec_command "reboot"


##### Tip ##########################
# >>> How to check service status? Overall.
#
#  $ systemctl list-units --all --state=failed,not-found,auto-restart
#  $ systemctl status dlt.service
#  $ systemctl status cmd_scheduler
#
# >>> How to debug a auto-restart service?
#
#  $ journalctl --no-pager -x -e -u cmd_scheduler
#  <주의사항>
#    /lib/aarch64-linux-gnu/libc.so.6: version `GLIBC_2.28' not found (required by libsqlite3.so)
####################################
