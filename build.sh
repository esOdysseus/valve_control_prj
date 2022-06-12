#!/bin/bash
ROOT_PATH=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
export ROOT_PATH=${ROOT_PATH}
DEF_CPU_ARCH="x86"

# Setting default-value.
BUILD_MODE=release
BUILD_TARGET="valve"
CPU_ARCH=${DEF_CPU_ARCH}    # Available Values : x86 , armv7 , aarch64
BOARD_TARGET="none"         # Available Values : none , orangepi-i96 , orangepi-pc+ , orangepi-zero+2H5


#######################################
# Entry Pointer of Script.
#######################################
function main() {
    # Get & Parse input-parameter.
    get_input_parameter $*

    # Make default-Environment variables for Build.
    BUILD_DIR=${ROOT_PATH}/build
    INSTALL_DIR=${ROOT_PATH}/${BUILD_MODE}
    echo
    echo "--------- Default Environment ---------"
    echo "BUILD_MODE=${BUILD_MODE}"
    echo "BUILD_DIR=${BUILD_DIR}"
    echo "INSTALL_DIR=${INSTALL_DIR}"
    echo "BUILD_TARGET=${BUILD_TARGET}"
    echo "CPU_ARCH=${CPU_ARCH}"
    echo "BOARD_TARGET=${BOARD_TARGET}"
    echo

    # Set enviroment for CROSS-COMPILE.
    if [ "${CPU_ARCH}" != "${DEF_CPU_ARCH}" ]; then
        echo "Set environment for CROSS-COMPILE of ${CPU_ARCH}."
        source env_for_cross_compile.sh  ${CPU_ARCH}  ${BOARD_TARGET}
    fi

    # RUN task.
    case ${BUILD_TARGET} in
        "clean")    # build clean
            if [ ! -d "${BUILD_DIR}" ]; then
                echo -e "\e[1;31m Can't clean. because Not exist BUILD_DIR folder. \e[0m"
                exit 1
            fi

            echo
            echo ">>>> Clear all-data of installation & objects. <<<<"
            rm -rf ${BUILD_DIR}
            rm -rf ${INSTALL_DIR}
            rm -rf ${ROOT_PATH}/common/lib/dlt/dlt-daemon
            ;;
        "all") # build all components
            run_build_common_lib all
            run_build_task  cmd_scheduler  ${INSTALL_DIR}/cmd_scheduler/bin
            run_build_task  valve_controller  ${INSTALL_DIR}/valve_controller/bin
            ;;
        "common-lib")   # build common-lib
            run_build_common_lib all
            ;;
        "server-rebuild")     # build cmd_scheduler
            echo
            echo ">>>> Clear all-data of CMD-Scheduler. <<<<"
            rm -rf ${BUILD_DIR}/cmd_scheduler
            rm -rf ${INSTALL_DIR}/cmd_scheduler
            echo
            echo ">>>> Re-build CMD-Scheduler. <<<<"
            run_build_task  cmd_scheduler  ${INSTALL_DIR}/cmd_scheduler/bin
            ;;
        "server")     # build cmd_scheduler
            run_build_task  cmd_scheduler  ${INSTALL_DIR}/cmd_scheduler/bin
            ;;
        "valve")     # build valve_control
            run_build_task  valve_controller  ${INSTALL_DIR}/valve_controller/bin
            ;;
        "none")
            echo -e "\e[1;31m [ERROR] We need BUILD_TARGET. Please, insert -t option. \e[0m"
            exit 1
            ;;
        *) 
            echo -e "\e[1;31m [ERROR] Not Supported BUILD_TARGET.(${BUILD_TARGET}) \e[0m"
            exit 1
            ;;
    esac

    # Exit build-situation.
    cd ${ROOT_PATH}
    exit 0      # Success & Exit script.
}


##################################
# Internal-Function definition.
##################################
function run_build_task() {
    BUILD_TARGET=${1}
    DESTDIR=${2}
    echo
    echo "------ Build Environment ------"
    echo "BUILD_DIR=${BUILD_DIR}"
    echo "BUILD_TARGET=${BUILD_TARGET}"
    echo "BUILD_MODE=${BUILD_MODE}"
    echo "DESTDIR=${DESTDIR}"
    echo "CPU_ARCH=${CPU_ARCH}"
    echo "BOARD_TARGET=${BOARD_TARGET}"
    echo

    if [ ! -d "${BUILD_DIR}" ]; then
        mkdir -p ${BUILD_DIR}
    fi
    cd ${BUILD_DIR}

    # Build Target
    echo ">>>> Build ${BUILD_TARGET} & install"
    echo "qmake ${ROOT_PATH} TARGET=${BUILD_TARGET} BUILD_MODE=${BUILD_MODE} DESTDIR=${DESTDIR} CPU_ARCH=${CPU_ARCH}"
    qmake ${ROOT_PATH} TARGET=${BUILD_TARGET} BUILD_MODE=${BUILD_MODE} DESTDIR=${DESTDIR} CPU_ARCH=${CPU_ARCH}
    make
    make install
}

function run_build_common_lib() {
    BUILD_TARGET=${1}
    DESTDIR=${INSTALL_DIR}/common/lib
    local BUILD_COMLIB_DIR=${BUILD_DIR}/common/lib

    echo 
    echo "----- Build Environment -----"
    echo "BUILD_COMLIB_DIR=${BUILD_COMLIB_DIR}"
    echo "BUILD_TARGET=${BUILD_TARGET}"
    echo "BUILD_MODE=${BUILD_MODE}"
    echo "DESTDIR=${DESTDIR}"
    echo "CPU_ARCH=${CPU_ARCH}"
    echo "BOARD_TARGET=${BOARD_TARGET}"
    echo 

    # Build Target
    case ${BUILD_TARGET} in
        "all") # build all components
            build_common_sqlite  ${BUILD_COMLIB_DIR}   ${DESTDIR}
            if [ "${CPU_ARCH}" != "${DEF_CPU_ARCH}" ]; then
                build_common_dlt     ${BUILD_COMLIB_DIR}   ${DESTDIR}
            fi
            build_common_communicator   "${ROOT_PATH}/common/lib"   ${DESTDIR}    ${CPU_ARCH}
            ;;
        "sqlite")    # build sqlite
            build_common_sqlite  ${BUILD_COMLIB_DIR}   ${DESTDIR}
            ;;
        "dlt")    # build dlt
            build_common_dlt  ${BUILD_COMLIB_DIR}   ${DESTDIR}
            ;;
        "comm")    # build communicator
            build_common_communicator  "${ROOT_PATH}/common/lib"   ${DESTDIR}     ${CPU_ARCH}
            ;;
        *) 
            echo -e "\e[1;31m [ERROR] Not Supported BUILD_TARGET common-lib.(${BUILD_TARGET}) \e[0m"
            exit 1
            ;;
    esac
}

function build_common_communicator() {
    echo ""
    echo "----- Build communicator. -----"
    local SRC_PATH=${1}/communicator
    local DESTDIR=${2}/communicator
    local CPU_ARCH=${3}

    if [ ! -d "${DESTDIR}" ]; then
        mkdir -p "${DESTDIR}"
        mkdir -p "${DESTDIR}/etc"
        mkdir -p "${DESTDIR}/include"
        mkdir -p "${DESTDIR}/lib"
    fi

    echo "cp -Rdp \"${SRC_PATH}/config/${CPU_ARCH}/*\"  \"${DESTDIR}/etc/\""
    cp -Rdp ${SRC_PATH}/config/${CPU_ARCH}/*  "${DESTDIR}/etc/"

    echo "cp -Rdp \"${SRC_PATH}/include/*\"  \"${DESTDIR}/include/\""
    cp -Rdp ${SRC_PATH}/include/*  "${DESTDIR}/include/"

    echo "cp -Rdp \"${SRC_PATH}/lib/${CPU_ARCH}/*\"  \"${DESTDIR}/lib/\""
    cp -Rdp ${SRC_PATH}/lib/${CPU_ARCH}/*  "${DESTDIR}/lib/"

    echo "----- Done build communicator. -----"
    echo ""
}

function build_common_dlt() {
    local BUILD_COMLIB_DIR=${1}/dlt
    local DESTDIR=${2}/dlt
    local SRC_PATH=${ROOT_PATH}/common/lib/dlt/dlt-daemon
    local OPTIONS=
    local INSTALL_OPT="-DCMAKE_INSTALL_PREFIX=${DESTDIR}"

    if [ ! -d "${BUILD_COMLIB_DIR}" ]; then
        mkdir -p ${BUILD_COMLIB_DIR}
    fi

    if [ ! -d "${DESTDIR}" ]; then
        mkdir -p ${DESTDIR}
    fi

    # if dlt source is not exist, then git clone version 2.18.8
    if [ ! -d "${SRC_PATH}" ]; then
        echo 
        echo "----- Git clone DLT-daemon -----"
        cd ${ROOT_PATH}/common/lib/dlt
        git clone https://github.com/COVESA/dlt-daemon.git
        cd ${SRC_PATH}
        git checkout -b 2.18.8 1438fcf8c88cd47b20b2984180a8457c3eb9193d
        sync
    fi

    # build source
    cd ${BUILD_COMLIB_DIR}
    echo 
    echo "----- Start to build DLT-daemon -----"
    if [ "${CPU_ARCH}" == "${DEF_CPU_ARCH}" ]; then
        echo "- CPU-ARCH = ${CPU_ARCH}"
        cmake ${SRC_PATH} ${OPTIONS} ${INSTALL_OPT}
    else
        echo "- CROSS-CPU-ARCH = ${CROSS_CPU_ARCH} (doing Cross-Compile)"
        cmake ${SRC_PATH} ${OPTIONS} ${INSTALL_OPT}
    fi
    make
    
    echo 
    echo "----- Start to install DLT-daemon -----"
    echo 
    make install
    cp -Rdp "${ROOT_PATH}/common/lib/dlt/dlt.conf"  "${DESTDIR}/etc/"
    cp -Rdp "${ROOT_PATH}/common/lib/dlt/dlt_logstorage.conf"  "${DESTDIR}/etc/"

    echo 
    echo "----- Done DLT-daemon -----"
    echo 
}

function build_common_sqlite() {
    local BUILD_COMLIB_DIR=${1}/sqlite
    local DESTDIR=${2}/sqlite
    local SRC_PATH=${ROOT_PATH}/common/lib/sqlite/sqlite
    local OPTIONS='--enable-threadsafe --enable-dynamic-extensions --enable-readline'
    local INSTALL_OPT="--prefix=${DESTDIR}"

    if [ ! -d "${BUILD_COMLIB_DIR}" ]; then
        mkdir -p ${BUILD_COMLIB_DIR}
    fi

    if [ ! -d "${DESTDIR}" ]; then
        mkdir -p ${DESTDIR}
    fi

    # # if sqlite3 source is not exist, then git clone version 3.36.0
    # if [ ! -d "${SRC_PATH}" ]; then
    #     cd ${ROOT_PATH}/common/lib/sqlite
    #     git clone https://github.com/sqlite/sqlite.git
    #     cd ${SRC_PATH}
    #     git checkout -b 3.36.0 6d72858e795481571a40c07675cc9d2c15b5b3ef
    #     sync
    # fi

    # make configuration
    echo 
    echo "----- Make SQLite configuration -----"
    cd ${SRC_PATH}
    autoreconf --force

    # build source
    cd ${BUILD_COMLIB_DIR}
    echo 
    echo "----- Start to build SQLite -----"
    if [ "${CPU_ARCH}" == "${DEF_CPU_ARCH}" ]; then
        echo "- CPU-ARCH = ${CPU_ARCH}"
        CFLAGS="-Os" ${SRC_PATH}/configure ${OPTIONS} ${INSTALL_OPT}
    else
        echo "- CROSS-CPU-ARCH = ${CROSS_CPU_ARCH} (doing Cross-Compile)"
        CFLAGS="-Os" ${SRC_PATH}/configure --host=${CROSS_CPU_ARCH} ${OPTIONS} ${INSTALL_OPT}
    fi
    make
    
    echo 
    echo "----- Start to install SQLite -----"
    echo 
    make install

    echo 
    echo "----- Done SQLite -----"
    echo 
}

function get_input_parameter() {
    echo "Get input parameters = $*"

    NONE_TYPE="none"
    IN_HEAD=${NONE_TYPE}
    for input in $*
    do
        case ${input} in
            "-m" )      IN_HEAD=${input};;
            "-t" )      IN_HEAD=${input};;
            "-arch" )   IN_HEAD=${input};;
            "-board" )  IN_HEAD=${input};;
            * )
                case ${IN_HEAD} in 
                    "-m" )  # for BUILD_MODE
                        BUILD_MODE=${input}
                        IN_HEAD=${NONE_TYPE};;
                    "-t" )  # for BUILD_TARGET
                        BUILD_TARGET=${input}
                        IN_HEAD=${NONE_TYPE};;
                    "-arch" ) # for CPU_ARCH
                        CPU_ARCH=${input}
                        IN_HEAD=${NONE_TYPE};;
                    "-board" ) # for BOARD_TARGET
                        BOARD_TARGET=${input}
                        IN_HEAD=${NONE_TYPE};;
                    ${NONE_TYPE} )
                        ;;
                    * )
                        echo -e "\e[1;31m [ERROR] Invalid input-head. (${IN_HEAD}) \e[0m"
                        exit 1
                esac
                ;;
        esac
    done
}

main $*