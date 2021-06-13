TARGET = app_cmd_scheduler
TEMPLATE = app
QT -= gui core

VER_MAJ = 0
VER_MIN = 1
VER_PAT = 0
VERSION = $$VER_MAJ"."$$VER_MIN"."$$VER_PAT

!include ($$_PRO_FILE_PWD_/../common_config.pri) {
    message( "Not exist common_config.pri file." )
}

# for building
COMMON_LIB_ROOT=$$_PRO_FILE_PWD_/../common
EXTERNAL_LIB_ROOT=$$COMMON_LIB_ROOT/lib/external_lib
LIBS += -lpthread -lcommunicator -L$$EXTERNAL_LIB_ROOT/lib/$$CPU_ARCH

DEFINES += LOGGER_TAG=\\\"APP\\\"
DEFINES += VER_MAJ=$$VER_MAJ
DEFINES += VER_MIN=$$VER_MIN
DEFINES += VER_PAT=$$VER_PAT

equals(BUILD_MODE, "debug") {
    DEFINES += LOG_TIME_ENABLE
    DEFINES += LOG_DEBUG_MODE
    DEFINES += LOG_LEVEL=$$LOG_LEVEL_DEBUG
}

equals(BUILD_MODE, "release") {
    DEFINES += LOG_LEVEL=$$LOG_LEVEL_INFO
}

INCLUDEPATH += \
    $$EXTERNAL_LIB_ROOT/include    \
    $$COMMON_LIB_ROOT/lib/logger  \
    $$COMMON_LIB_ROOT/lib/exception   \
    $$COMMON_LIB_ROOT/lib/json    \
    $$COMMON_LIB_ROOT/lib/lock    \
    $$COMMON_LIB_ROOT/lib/sys_sigslot    \
    $$COMMON_LIB_ROOT/lib/time    \
    $$_PRO_FILE_PWD_/source    \
    $$_PRO_FILE_PWD_/source/include    \
    $$_PRO_FILE_PWD_/source/cmd_pkg    \
    $$_PRO_FILE_PWD_/source/monitor_pkg \
    $$_PRO_FILE_PWD_/source/monitor_pkg/base \
    $$_PRO_FILE_PWD_/source/monitor_pkg/monitor_data \
    $$_PRO_FILE_PWD_/source/proc_pkg    \
    $$_PRO_FILE_PWD_/source/proc_pkg/base


SOURCES += \
    $$files($$_PRO_FILE_PWD_/source/*.cpp)  \
    $$files($$_PRO_FILE_PWD_/source/cmd_pkg/*.cpp) \
    $$files($$_PRO_FILE_PWD_/source/monitor_pkg/*.cpp)  \
    $$files($$_PRO_FILE_PWD_/source/monitor_pkg/base/*.cpp)  \
    $$files($$_PRO_FILE_PWD_/source/monitor_pkg/monitor_data/*.cpp)  \
    $$files($$_PRO_FILE_PWD_/source/proc_pkg/*.cpp) \
    $$files($$_PRO_FILE_PWD_/source/proc_pkg/base/*.cpp)
    

# for installation.
EXTRA_BINFILES = \
    $$_PRO_FILE_PWD_/$$TARGET

# PKG_CONFIG_DESCRIPTION=Common-communicator SDK library that support transaction-orient & service-orient.

!include ($$_PRO_FILE_PWD_/../deploy.pri) {
    message( "Not exist sdk_deploy.pri file." )
}

# # for process, if exist.
# include ($$_PRO_FILE_PWD_/post_proc.pri)
