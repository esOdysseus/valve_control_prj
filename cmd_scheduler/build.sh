#!/bin/bash
ROOT_PATH=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
export ROOT_PATH=${ROOT_PATH}

# Setting default-value.
BUILD_MODE=release
BUILD_TARGET="valve"
CPU_ARCH="x86"          # Available Values : x86 , armv7 , aarch64
BOARD_TARGET="none"     # Available Values : none , orangepi-i96 , orangepi-pc+ , orangepi-zero+2H5


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
    if [ "${CPU_ARCH}" != "x86" ]; then
        echo "Set environment for CROSS-COMPILE of ${CPU_ARCH}."
        source env_for_cross_compile.sh  ${CPU_ARCH}  ${BOARD_TARGET}
    fi

    # RUN task.
    case ${BUILD_TARGET} in
        "clean")    # build clean
            if [ ! -d "${BUILD_DIR}" ]; then
                echo -e "\e[1;31m Can't clean. because Not exist BUILD_DIR folder. \e[0m"
                exit 0
            fi

            echo ">>>> Clear all-data of installation & objects. <<<<"
            rm -rf ${BUILD_DIR}
            rm -rf ${INSTALL_DIR}
            ;;
        "server")     # build cmd_scheduler
            run_build_task  cmd_scheduler  ${INSTALL_DIR}/bin
            ;;
        "none")
            echo -e "\e[1;31m [ERROR] We need BUILD_TARGET. Please, insert -t option. \e[0m"
            exit 0
            ;;
        *) 
            echo -e "\e[1;31m [ERROR] Not Supported BUILD_TARGET.(${BUILD_TARGET}) \e[0m"
            exit 0
            ;;
    esac

    # Exit build-situation.
    cd ${ROOT_PATH}
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
    qmake ${ROOT_PATH} TARGET=${BUILD_TARGET} BUILD_MODE=${BUILD_MODE} DESTDIR=${DESTDIR} CPU_ARCH=${CPU_ARCH}
    make
    make install
}

function get_input_parameter()
{
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
                        exit 0
                esac
                ;;
        esac
    done
}

main $*