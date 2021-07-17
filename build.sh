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
    echo "BUILD_DIR=${BUILD_COMLIB_DIR}"
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
            ;;
        "sqlite")    # build sqlite
            build_common_sqlite  ${BUILD_COMLIB_DIR}   ${DESTDIR}
            ;;
        *) 
            echo -e "\e[1;31m [ERROR] Not Supported BUILD_TARGET common-lib.(${BUILD_TARGET}) \e[0m"
            exit 1
            ;;
    esac
}

function build_common_sqlite() {
    local BUILD_COMLIB_DIR=${1}/sqlite
    local DESTDIR=${2}/sqlite
    local SQLITE_SRC_PATH=${ROOT_PATH}/common/lib/sqlite/sqlite3_src
    local OPTIONS='--enable-threadsafe --enable-dynamic-extensions --enable-readline'
    local INSTALL_OPT="--prefix=${DESTDIR}"

    if [ ! -d "${BUILD_COMLIB_DIR}" ]; then
        mkdir -p ${BUILD_COMLIB_DIR}
    fi

    if [ ! -d "${DESTDIR}" ]; then
        mkdir -p ${DESTDIR}
    fi

    cd ${BUILD_COMLIB_DIR}
    echo 
    echo "----- Start to build SQLite -----"
    if [ "${CPU_ARCH}" == "${DEF_CPU_ARCH}" ]; then
        echo "- CPU-ARCH = ${CPU_ARCH}"
        CFLAGS="-Os" ${SQLITE_SRC_PATH}/configure ${OPTIONS} ${INSTALL_OPT}
    else
        echo "- CROSS-CPU-ARCH = ${CROSS_CPU_ARCH} (doing Cross-Compile)"
        CFLAGS="-Os" ${SQLITE_SRC_PATH}/configure --host=${CROSS_CPU_ARCH} ${OPTIONS} ${INSTALL_OPT}
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