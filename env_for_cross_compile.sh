#!/bin/bash
__CUR_DIR__=${PWD}
ROOT_PATH_SCRIPT_FILE=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
CPU_ARCH=${1}
BOARD_TARGET=${2}

# Set Environment.
case ${CPU_ARCH} in
    "armv7" )
        echo "Set Environment for armv7 ARCH."
        echo

        # Set variables according to BOARD_TARGET.
        case ${BOARD_TARGET} in
            "orangepi-pc+" )    # valid tool-chain path !!
                TOOLCHAIN_PATH="/usr/local/gcc-linaro/arm-linux-gnueabihf/4.9.4_x86_64"
                CROSS_COMPILER_PREFIX="arm-linux-gnueabihf-"
                CROSS_CPU_ARCH="arm-linux-gnueabihf"
                ;;
            "orangepi-i96" )    # valid tool-chain path !!
                TOOLCHAIN_PATH="/usr/local/gcc-linaro/arm-linux-gnueabihf/4.9.4_x86_64"
                CROSS_COMPILER_PREFIX="arm-linux-gnueabihf-"
                CROSS_CPU_ARCH="arm-linux-gnueabihf"
                ;;
            * ) 
                echo -e "\e[1;31m [ERROR] Not Supported BOARD_TARGET.(${BOARD_TARGET}) \e[0m"
                ;;
        esac

        # Set Cross-Compiler Variables & Environment.
        export CROSS_CPU_ARCH="${CROSS_CPU_ARCH}"
        export CC="${CROSS_COMPILER_PREFIX}gcc"
        export CXX="${CROSS_COMPILER_PREFIX}g++"
        export LINK="${CROSS_COMPILER_PREFIX}g++"
        export PATH=${TOOLCHAIN_PATH}/bin:${PATH}
        # export PKG_CONFIG_PATH=""
        # export CUSTOM_MODULE_DIR=""
        ;;
    "aarch64" )
        echo "Set Environment for aarch64 ARCH."
        echo

        # Set variables according to BOARD_TARGET.
        case ${BOARD_TARGET} in
            "orangepi-zero+2H5" )   # valid tool-chain path !!
                TOOLCHAIN_PATH="/usr/local/gcc-linaro/aarch64-linux-gnu/8.3.0_x86_64"
                CROSS_COMPILER_PREFIX="aarch64-linux-gnu-"
                CROSS_CPU_ARCH="aarch64-linux-gnu"
                ;;
            "orangepi-zero2" )   # valid tool-chain path !!
                TOOLCHAIN_PATH="/usr/local/gcc-linaro/aarch64-linux-gnu/8.3.0_x86_64"
                CROSS_COMPILER_PREFIX="aarch64-linux-gnu-"
                CROSS_CPU_ARCH="aarch64-linux-gnu"
                ;;
            * ) 
                echo -e "\e[1;31m [ERROR] Not Supported BOARD_TARGET.(${BOARD_TARGET}) \e[0m"
                ;;
        esac

        # Set Cross-Compiler Variables & Environment.
        export CROSS_CPU_ARCH="${CROSS_CPU_ARCH}"
        export CC="${CROSS_COMPILER_PREFIX}gcc"
        export CXX="${CROSS_COMPILER_PREFIX}g++"
        export LINK="${CROSS_COMPILER_PREFIX}g++"
        export PATH=${TOOLCHAIN_PATH}/bin:${PATH}
        # export PKG_CONFIG_PATH=""
        # export CUSTOM_MODULE_DIR=""
        ;;
    * )
        echo -e "\e[1;31m [ERROR] Not Supported CPU_ARCH.(${CPU_ARCH}) \e[0m"
        exit 0
        ;;
esac


# Exit this script
cd ${__CUR_DIR__}
